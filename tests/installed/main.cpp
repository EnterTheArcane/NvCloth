#ifdef PHYSX_FIRST
#include <PxPhysicsAPI.h>
#include <foundation/PxAllocator.h>
#endif

#include <NvCloth/Cloth.h>
#include <NvCloth/Fabric.h>
#include <NvCloth/Factory.h>
#include <NvCloth/Solver.h>
#include <NvClothExt/ClothFabricCooker.h>

#ifndef PHYSX_FIRST
#include <PxPhysicsAPI.h>
#include <foundation/PxAllocator.h>
#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

#ifdef _WIN32
#include <malloc.h>
#endif

namespace
{
using namespace physx;
using namespace nv::cloth;

class Allocator final : public physx::PxAllocatorCallback
{
public:
    void* allocate(size_t size, const char*, const char*, int) override
    {
#ifdef _WIN32
        return _aligned_malloc(size, 16);
#else
        void* ptr = nullptr;
        if (posix_memalign(&ptr, 16, size) != 0)
        {
            return nullptr;
        }
        return ptr;
#endif
    }

    void deallocate(void* ptr) override
    {
#ifdef _WIN32
        _aligned_free(ptr);
#else
        std::free(ptr);
#endif
    }
};

class Errors final : public physx::PxErrorCallback
{
public:
    void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line) override
    {
        std::fprintf(stderr, "%s:%d [%d] %s\n", file, line, static_cast<int>(code), message);
        failed |= code != PxErrorCode::eDEBUG_INFO && code != PxErrorCode::eDEBUG_WARNING;
    }

    bool failed = false;
};

class ClothAssertions final : public nv::cloth::PxAssertHandler
{
public:
    void operator()(const char* expression, const char* file, int line, bool&) override
    {
        std::fprintf(stderr, "%s:%d cloth assertion: %s\n", file, line, expression);
        std::abort();
    }
};

void releaseFabric(Fabric* fabric)
{
    fabric->decRefCount();
}

bool simulateCloth()
{
    constexpr uint32_t width = 12;
    constexpr uint32_t pinned = width - 1;
    constexpr uint32_t count = width * width;
    const PxVec3 gravity(0.0f, -9.81f, 0.0f);
    std::vector<PxVec4> particles;
    std::vector<uint32_t> triangles;
    particles.reserve(count);
    for (uint32_t y = 0; y < width; ++y)
    {
        for (uint32_t x = 0; x < width; ++x)
        {
            const uint32_t i = y * width + x;
            float invMass = 1.0f;
            if (i == 0 || i == pinned)
            {
                invMass = 0.0f;
            }
            particles.emplace_back(x * 0.1f, 1.5f, y * 0.1f, invMass);
            if (x + 1 < width && y + 1 < width)
            {
                triangles.insert(triangles.end(), {i, i + width, i + 1, i + 1, i + width, i + width + 1});
            }
        }
    }

    ClothMeshDesc mesh;
    mesh.points.data = particles.data();
    mesh.points.count = count;
    mesh.points.stride = sizeof(PxVec4);
    mesh.invMasses.data = &particles.front().w;
    mesh.invMasses.count = count;
    mesh.invMasses.stride = sizeof(PxVec4);
    mesh.triangles.data = triangles.data();
    mesh.triangles.count = static_cast<uint32_t>(triangles.size() / 3);
    mesh.triangles.stride = 3 * sizeof(uint32_t);

    std::unique_ptr<Factory, decltype(&NvClothDestroyFactory)> factory(NvClothCreateFactoryCPU(), NvClothDestroyFactory);
    if (!factory)
    {
        return false;
    }
    std::unique_ptr<Fabric, decltype(&releaseFabric)> fabric(NvClothCookFabricFromMesh(factory.get(), mesh, gravity), releaseFabric);
    if (!fabric)
    {
        return false;
    }
    std::unique_ptr<Cloth> cloth(factory->createCloth({particles.data(), particles.data() + particles.size()}, *fabric));
    std::unique_ptr<Solver> solver(factory->createSolver());
    if (!cloth || !solver)
    {
        return false;
    }

    std::vector<PhaseConfig> phases;
    for (uint32_t i = 0; i < fabric->getNumPhases(); ++i)
    {
        phases.emplace_back(static_cast<uint16_t>(i));
    }
    cloth->setPhaseConfig({phases.data(), phases.data() + phases.size()});
    cloth->setGravity(gravity);
    // Tethers apply positional corrections independently of inverse mass.
    cloth->setTetherConstraintStiffness(0.0f);
    const PxVec4 sphere(0.55f, 0.65f, 0.4f, 0.35f);
    cloth->setSpheres({&sphere, &sphere + 1}, 0, 0);
    solver->addCloth(cloth.get());

    bool valid = true;
    bool moved = false;
    for (int step = 0; step < 600 && valid; ++step)
    {
        if (!solver->beginSimulation(1.0f / 60.0f))
        {
            valid = false;
            break;
        }
        for (int chunk = 0; chunk < solver->getSimulationChunkCount(); ++chunk)
        {
            solver->simulateChunk(chunk);
        }
        solver->endSimulation();

        const auto current = readCurrentParticles(*cloth);
        for (uint32_t i = 0; i < count; ++i)
        {
            const PxVec4& p = current.begin()[i];
            if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z) || !std::isfinite(p.w)
                || std::fabs(p.x) > 10.0f || std::fabs(p.y) > 10.0f || std::fabs(p.z) > 10.0f
                || ((i == 0 || i == pinned)
                    && (p.x != particles[i].x || p.y != particles[i].y || p.z != particles[i].z)))
            {
                std::fprintf(stderr, "Invalid particle %u at step %d: (%g, %g, %g, %g)\n", i, step, p.x, p.y, p.z, p.w);
                valid = false;
                break;
            }
            if (i == count - 1 && std::fabs(p.y - particles[i].y) > 0.05f)
            {
                moved = true;
            }
        }
    }
    solver->removeCloth(cloth.get());
    return valid && moved;
}
} // namespace

int main()
{
    Allocator allocator;
    Errors errors;
    ClothAssertions assertions;
    nv::cloth::InitializeNvCloth(&allocator, &errors, &assertions, nullptr);
    NV_CLOTH_LOG_INFO("Checking the installed NvCloth SDK");
    NV_CLOTH_PROFILE_ZONE("NvCloth installed consumer", 0);
    if (!simulateCloth())
    {
        std::fprintf(stderr, "Standalone NvCloth simulation failed\n");
        return 1;
    }

    PxFoundation* foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errors);
    if (!foundation)
    {
        return 1;
    }
    void* allocation = PX_ALLOC(16, "NvCloth installed consumer");
    if (!allocation)
    {
        foundation->release();
        return 1;
    }
    PX_FREE(allocation);
    if (allocation)
    {
        std::fprintf(stderr, "PhysX PX_FREE did not clear the pointer\n");
        foundation->release();
        return 1;
    }

    PxPhysics* physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale());
    bool passed = false;
    if (physics)
    {
        PxRigidDynamic* body = physics->createRigidDynamic(PxTransform(PxIdentity));
        if (body)
        {
            passed = simulateCloth();
            body->release();
        }
        physics->release();
    }
    foundation->release();
    if (!passed || errors.failed)
    {
        std::fprintf(stderr, "NvCloth and PhysX coexistence test failed\n");
        return 1;
    }
    std::puts("NvCloth and PhysX installed consumer passed");
    return 0;
}
