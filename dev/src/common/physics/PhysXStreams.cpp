#include "build.h"
#include "../physics/PhysXStreams.h"

#ifdef USE_PHYSX

using namespace physx;

MemoryWriteBuffer::MemoryWriteBuffer() : currentSize(0), maxSize(0), data(NULL)
{
}

MemoryWriteBuffer::~MemoryWriteBuffer()
{
	RED_MEMORY_FREE( MemoryPool_Physics, MC_PhysX, data );
}

void MemoryWriteBuffer::clear()
{
    currentSize = 0;
}

PxU32 MemoryWriteBuffer::write(const void* buffer, PxU32 size)
{
    PxU32 expectedSize = currentSize + size;
    if(expectedSize > maxSize)
    {
        maxSize = expectedSize + 4096;

		PxU8* newData = static_cast< PxU8* >( RED_MEMORY_ALLOCATE( MemoryPool_Physics, MC_PhysX, maxSize * sizeof(PxU8) ) );
		Red::System::MemorySet( newData, 0, maxSize * sizeof(PxU8) );
        PX_ASSERT(newData!=NULL);

        if(data)
        {
            memcpy(newData, data, currentSize);
            RED_MEMORY_FREE( MemoryPool_Physics, MC_PhysX, data );
        }
        data = newData;
    }
    memcpy(data+currentSize, buffer, size);
    currentSize += size;
    return size;;
}

MemoryReadBuffer::MemoryReadBuffer(const PxU8* data) : buffer(data)
{
}

MemoryReadBuffer::~MemoryReadBuffer()
{
    // We don't own the data => no delete
}

PxU32 MemoryReadBuffer::read(void* dest, PxU32 size)
{
    memcpy(dest, buffer, size);
    buffer += size;
	return size;
}

#endif