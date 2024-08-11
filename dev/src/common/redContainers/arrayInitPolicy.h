/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef RED_CONTAINER_ARRAY_INIT_POLICY_H
#define RED_CONTAINER_ARRAY_INIT_POLICY_H
#pragma once

#include "containersCommon.h"
#include "../redSystem/crt.h"

namespace Red { namespace Containers {

	// Element initialisation policy
	template <typename T, Red::System::Bool is_POD>
	class TInitPolicy
	{
	public:
		static RED_INLINE void Construct( T* buf, Red::System::MemSize num );							// Create elements in the buffer
		static RED_INLINE void Destruct( T* buf, Red::System::MemSize num );							// Destroy elements in a buffer
		static RED_INLINE void CopyConstruct( T* dstBuf, const T* srcBuf, Red::System::MemSize num );	// Copy elements via copy constructor
		static RED_INLINE void MoveConstruct( T* dstBuf, T* srcBuf, Red::System::MemSize num );			// Move elements via move constructor
		static RED_INLINE void SwapElements(T* elem1, T* elem2);							// Swap 2 elements data
	};

	// Non-POD element initialisation
	template <typename T>
	class TInitPolicy<T, false >
	{
	public:
		static RED_INLINE void Construct( T* buf, Red::System::MemSize num )
		{
			while( num-- > 0 )
			{
				::new(buf++) T;	
			}
		}

		static RED_INLINE void Destruct( T* buf, Red::System::MemSize num )
		{
			// loop uses a descending order to preserve the canonical destruction order of C++ 
			if( num == 0 )
			{
				return;
			}
			buf += num - 1;
			while(num-- > 0)
			{
				buf--->~T();
			}
		}

		static RED_INLINE void CopyConstruct( T* dstBuf, const T* srcBuf, Red::System::MemSize num )
		{
			while(num-- > 0)
			{
				::new(dstBuf++) T(*srcBuf++);
			}
		}

		static RED_INLINE void MoveConstruct( T* dstBuf, T* srcBuf, Red::System::MemSize num )
		{
			while(num-- > 0)
			{
				::new(dstBuf++) T( ElementMoveConstruct(*srcBuf++) );
			}
		}

		static RED_INLINE void SwapElements(T* elem1, T* elem2)
		{
			T tmp = *elem1;
			*elem1 = *elem2;
			*elem2 = tmp;
		}
	};

	// POD element initialisation
	template <typename T>
	class TInitPolicy<T, true>
	{
	public:
		static RED_INLINE void Construct(T*, Red::System::MemSize)
		{
		}

		static RED_INLINE void CopyConstruct(T* dstBuf, const T* srcBuf, Red::System::MemSize num)
		{
			Red::System::MemoryCopy(dstBuf, srcBuf, num*sizeof(T));
		}

		static RED_INLINE void MoveConstruct(T* dstBuf, T* srcBuf, Red::System::MemSize num)
		{
			CopyConstruct( dstBuf, srcBuf, num );
		}

		static RED_INLINE void Destruct(T*, Red::System::MemSize)
		{
		}

		static RED_INLINE void SwapElements(T* elem1, T* elem2)
		{
			Red::System::Uint8 tmpBuf[sizeof(T)];
			Red::System::MemoryCopy(tmpBuf, elem1, sizeof(T));
			Red::System::MemoryCopy(elem1, elem2, sizeof(T));
			Red::System::MemoryCopy(elem2, tmpBuf, sizeof(T));
		}
	};

} }

#endif