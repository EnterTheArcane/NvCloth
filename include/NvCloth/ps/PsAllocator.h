// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2020 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#ifndef PSFOUNDATION_PSALLOCATOR_H
#define PSFOUNDATION_PSALLOCATOR_H

#include "../Callbacks.h"
#include "Ps.h"

#include <foundation/PxAllocatorCallback.h>
#include <foundation/PxIO.h>

#include <cstdlib>
#include <new>
#if PX_WINDOWS_FAMILY || PX_XBOXONE || PX_APPLE_FAMILY
#include <typeinfo>
#endif

/** \brief NVidia namespace */
namespace nv
{
/** \brief nvcloth namespace */
namespace cloth
{
namespace ps
{
//NV_CLOTH_IMPORT PxAllocatorCallback& getAllocator();

/**
Allocator used to access the global PxAllocatorCallback instance without providing additional information.
*/

class NV_CLOTH_IMPORT Allocator
{
  public:
	Allocator(const char* = 0)
	{
	}
	void* allocate(size_t size, const char* file, int line);
	void deallocate(void* ptr);
};

/*
 * Bootstrap allocator using malloc/free.
 * Don't use unless your objects get allocated before foundation is initialized.
 */
class RawAllocator
{
  public:
	RawAllocator(const char* = 0)
	{
	}
	void* allocate(size_t size, const char*, int)
	{
		// malloc returns valid pointer for size==0, no need to check
		return ::malloc(size);
	}
	void deallocate(void* ptr)
	{
		// free(0) is guaranteed to have no side effect, no need to check
		::free(ptr);
	}
};

/*
 * Allocator that simply calls straight back to the application without tracking.
 * This is used by the heap (Foundation::mNamedAllocMap) that tracks allocations
 * because it needs to be able to grow as a result of an allocation.
 * Making the hash table re-entrant to deal with this may not make sense.
 */
class NonTrackingAllocator
{
  public:
	PX_FORCE_INLINE NonTrackingAllocator(const char* = 0)
	{
	}
	PX_FORCE_INLINE void* allocate(size_t size, const char* file, int line)
	{
		return !size ? 0 : GetNvClothAllocator()->allocate(size, "NonTrackedAlloc", file, line);
	}
	PX_FORCE_INLINE void deallocate(void* ptr)
	{
		if(ptr)
			GetNvClothAllocator()->deallocate(ptr);
	}
};

/*
\brief	Virtual allocator callback used to provide run-time defined allocators to foundation types like Array or Bitmap.
        This is used by VirtualAllocator
*/
class VirtualAllocatorCallback
{
  public:
	VirtualAllocatorCallback()
	{
	}
	virtual ~VirtualAllocatorCallback()
	{
	}
	virtual void* allocate(const size_t size, const char* file, const int line) = 0;
	virtual void deallocate(void* ptr) = 0;
};

/*
\brief Virtual allocator to be used by foundation types to provide run-time defined allocators.
Due to the fact that Array extends its allocator, rather than contains a reference/pointer to it, the VirtualAllocator
must
be a concrete type containing a pointer to a virtual callback. The callback may not be available at instantiation time,
therefore
methods are provided to set the callback later.
*/
class VirtualAllocator
{
  public:
	VirtualAllocator(VirtualAllocatorCallback* callback = NULL) : mCallback(callback)
	{
	}

	void* allocate(const size_t size, const char* file, const int line)
	{
		NV_CLOTH_ASSERT(mCallback);
		if(size)
			return mCallback->allocate(size, file, line);
		return NULL;
	}
	void deallocate(void* ptr)
	{
		NV_CLOTH_ASSERT(mCallback);
		if(ptr)
			mCallback->deallocate(ptr);
	}

	void setCallback(VirtualAllocatorCallback* callback)
	{
		mCallback = callback;
	}
	VirtualAllocatorCallback* getCallback()
	{
		return mCallback;
	}

  private:
	VirtualAllocatorCallback* mCallback;
	VirtualAllocator& operator=(const VirtualAllocator&);
};

/**
Allocator used to access the global PxAllocatorCallback instance using a static name derived from T.
*/

template <typename T>
class ReflectionAllocator
{
	static const char* getName()
	{
		if(true)
			return "<allocation names disabled>";
#if PX_GCC_FAMILY
		return __PRETTY_FUNCTION__;
#else
		// name() calls malloc(), raw_name() wouldn't
		return typeid(T).name();
#endif
	}

  public:
	ReflectionAllocator(const physx::PxEMPTY)
	{
	}
	ReflectionAllocator(const char* = 0)
	{
	}
	inline ReflectionAllocator(const ReflectionAllocator&)
	{
	}
	void* allocate(size_t size, const char* filename, int line)
	{
		return size ? GetNvClothAllocator()->allocate(size, getName(), filename, line) : 0;
	}
	void deallocate(void* ptr)
	{
		if(ptr)
			GetNvClothAllocator()->deallocate(ptr);
	}
};

template <typename T>
struct AllocatorTraits
{
	typedef ReflectionAllocator<T> Type;
};

// Restrict the global allocator placement new overloads to plain-old-types.
template <typename T, typename X>
union EnableIfPod
{
	int i;
	T t;
	typedef X Type;
};

} // namespace ps
} // namespace cloth
} // namespace nv

// Storage for pointers and built-in types allocated this way must be returned to the NvCloth allocator.
template <typename T>
PX_INLINE void* operator new(size_t size, nv::cloth::ps::ReflectionAllocator<T> alloc, const char* fileName,
                             typename nv::cloth::ps::EnableIfPod<T, int>::Type line)
{
	return alloc.allocate(size, fileName, line);
}

template <typename T>
PX_INLINE void* operator new [](size_t size, nv::cloth::ps::ReflectionAllocator<T> alloc, const char* fileName,
                                typename nv::cloth::ps::EnableIfPod<T, int>::Type line)
{ return alloc.allocate(size, fileName, line); }

// If construction after placement new throws, this placement delete is being called.
template <typename T>
PX_INLINE void operator delete(void* ptr, nv::cloth::ps::ReflectionAllocator<T> alloc, const char* fileName,
                               typename nv::cloth::ps::EnableIfPod<T, int>::Type line)
{
	PX_UNUSED(fileName);
	PX_UNUSED(line);

	alloc.deallocate(ptr);
}

// If construction after placement new throws, this placement delete is being called.
template <typename T>
PX_INLINE void operator delete [](void* ptr, nv::cloth::ps::ReflectionAllocator<T> alloc, const char* fileName,
                                  typename nv::cloth::ps::EnableIfPod<T, int>::Type line)
{
	PX_UNUSED(fileName);
	PX_UNUSED(line);

	alloc.deallocate(ptr);
}

#endif // #ifndef PSFOUNDATION_PSALLOCATOR_H
