#pragma once

#include "physXEngine.h"

#ifdef USE_PHYSX

class MemoryReadBuffer : public physx::PxInputStream
{
public:
    MemoryReadBuffer(const physx::PxU8* data);
    virtual						~MemoryReadBuffer();
    virtual		physx::PxU32			read(void* buffer, physx::PxU32 size);

	const physx::PxU8*					buffer;
};

class MemoryWriteBuffer : public physx::PxOutputStream
{
public:
    MemoryWriteBuffer();
    virtual						~MemoryWriteBuffer();
    void						clear();
    virtual		physx::PxU32			write(const void* buffer, physx::PxU32 size);

    physx::PxU32			currentSize;
    physx::PxU32			maxSize;
    physx::PxU8*			data;
};

#endif