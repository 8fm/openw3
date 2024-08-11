/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef RED_CONTAINER_ARRAY_COMPARE_POLICY_H
#define RED_CONTAINER_ARRAY_COMPARE_POLICY_H
#pragma once

#include "containersCommon.h"
#include "../redSystem/crt.h"

namespace Red { namespace Containers {

	// Policy used for element comparison
	template <typename T, Red::System::Bool is_POD>
	class TComparePolicy
	{
	public:
		static RED_INLINE Red::System::Bool Equal( const T* buf1, const T* buf2, Red::System::MemSize num );
		static RED_INLINE Red::System::Bool Less( const T* buf1, Red::System::MemSize num1, const T* buf2, Red::System::MemSize num2 );
	};

	// Non-POD types comparisons
	template <typename T>
	class TComparePolicy<T, false>
	{
	public:
		static RED_INLINE Red::System::Bool Equal( const T* buf1, const T* buf2, Red::System::MemSize num )
		{
			while (num-- > 0)
			{
				if ( !(*buf1++ == *buf2++)  )
				{
					return false;
				}
			}
			return true;
		}

		static RED_INLINE Red::System::Bool Less( const T* buf1, Red::System::MemSize num1, const T* buf2, Red::System::MemSize num2 )
		{
			while (num1-- > 0 && num2-- > 0)
			{
				if (*buf1 < *buf2)
				{
					return true;
				}
				else if (*buf2 < *buf1)
				{
					return false;
				}
				buf1 ++;
				buf2 ++;
			}

			return (num1 <= 0 && num2 > 0);
		}
	};

	// POD type comparison
	template <typename T>
	class TComparePolicy<T, true>
	{
	public:
		static RED_INLINE Red::System::Bool Equal( const T* buf1, const T* buf2, Red::System::MemSize num )
		{
			return 0 == Red::System::MemoryCompare( buf1, buf2, num*sizeof(T) );
		}

		static RED_INLINE Red::System::Bool Less( const T* buf1, Red::System::MemSize num1, const T* buf2, Red::System::MemSize num2 )
		{
			Red::System::Int32 cmp = Red::System::MemoryCompare(buf1, buf2, num1 < num2 ? num1*sizeof(T) : num2*sizeof(T));
			return (cmp < 0 || (cmp == 0 && num1 < num2));
		}
	};

} }

#endif