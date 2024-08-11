/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef RED_CONTAINER_ARRAY_COPY_POLICY_H
#define RED_CONTAINER_ARRAY_COPY_POLICY_H
#pragma once

#include "containersCommon.h"
#include "../redSystem/crt.h"

namespace Red { namespace Containers {

	// Copy policy interface. 
	template <typename T, Red::System::Bool is_POD>
	class TCopyPolicy
	{
	public:
		static RED_INLINE void MoveBackwards(T* buf, Red::System::Int32 offset, Red::System::Int32 num);
		static RED_INLINE void MoveForwardsInsert(T* dstBuf, Red::System::Int32 dstNum, const T* srcBuf, Red::System::Int32 srcNum);
		static RED_INLINE void CopyNonOverlapping(T* dstBuf, const T* srcBuf, Red::System::Int32 num );
	};

	// Copy policy for non-POD types.
	template <typename T>
	class TCopyPolicy<T, false>
	{
	public:
		static RED_INLINE void MoveBackwards(T* buf, Red::System::Int32 offset, Red::System::Int32 num)
		{
			// move backwards num elements from (buf+offset) address to (buf) address
			T* from	= buf + offset;
			T* to	= buf;
			for (Red::System::Int32 it	= num; it; --it)
				*to++ = ElementMoveConstruct( *from++ );

			// destruct offset elements in reverse order starting from (buf+offset+num) ending at (buf+num)
			T* ptr	= buf + offset + num - 1;
			for (Red::System::Int32 it	= offset; it; --it)
				ptr--->~T();
		}

		static RED_INLINE void MoveForwardsInsert(T* dstBuf, Red::System::Int32 dstNum, const T* srcBuf, Red::System::Int32 srcNum)
		{
			// copy dst buffer by srcNum elements in reverse order
			while(dstNum-->0)
				*(dstBuf+dstNum+srcNum) = ElementMoveConstruct(*(dstBuf+dstNum));

			// copy src buffer into dst buffer
			while(srcNum-->0)
				*dstBuf++ = ElementMoveConstruct(*srcBuf++);
		}

		static RED_INLINE void CopyNonOverlapping(T* dstBuf, const T* srcBuf, Red::System::Int32 num )
		{
			// copy src buffer into dst buffer
			while( num-- > 0 )
				*dstBuf++ = *srcBuf++;
		}
	};

	// If type is POD, use memcopys for speed
	template <typename T>
	class TCopyPolicy<T, true>
	{
	public:
		static RED_INLINE void MoveBackwards(T* buf, Red::System::Int32 offset, Red::System::Int32 num)
		{
			Red::System::MemoryCopy(buf, buf + offset, num*sizeof(T));
		}

		static RED_INLINE void MoveForwardsInsert(T* dstBuf, Red::System::Int32 dstNum, const T* srcBuf, Red::System::Int32 srcNum)
		{
			Red::System::MemoryMove(dstBuf + srcNum, dstBuf, dstNum*sizeof(T));
			Red::System::MemoryCopy(dstBuf, srcBuf, srcNum*sizeof(T));
		}

		static RED_INLINE void CopyNonOverlapping(T* dstBuf, const T* srcBuf, Red::System::Int32 num )
		{
			Red::System::MemoryCopy( dstBuf, srcBuf, num*sizeof(T) );
		}
	};

} }

#endif