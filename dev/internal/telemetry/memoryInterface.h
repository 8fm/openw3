#pragma once

namespace Telemetry
{
	class IMemory
	{
	public:
		virtual void* alloc( size_t size ) = 0;
		virtual void* realloc( void* mem, size_t size ) = 0;
		virtual void free( void* mem ) = 0;
	};
}