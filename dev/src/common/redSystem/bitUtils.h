/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/
#ifndef RED_SYSTEM_BIT_UTILS_H
#define RED_SYSTEM_BIT_UTILS_H
#pragma once

#include "types.h"
#include "error.h"

namespace Red
{
	namespace System
	{
		namespace BitUtils
		{
			/////////////////////////////////////////////////////////////////////
			// BitScanForward
			//	Return the index of the first bit set (from LSB -> MSB)
			//	Similar to ctz
			template< class MaskType >
			RED_INLINE MaskType BitScanForward( MaskType mask )
			{
				RED_UNUSED( mask );
				RED_HALT( "Type not supported!" );
				return 0;
			}

			//////////////////////////////////////////////////////////////////////
			// BitScanReverse
			//	Return the index of the last set bit (from MSB -> LSB)
			//	Note, the result is an absolute bit index (not relative to MSB)
			template< class MaskType >
			RED_INLINE MaskType BitScanReverse( MaskType mask )
			{
				RED_UNUSED( mask );
				RED_HALT( "Type not supported!" );
				return 0;
			}

			//////////////////////////////////////////////////////////////////////
			// CountLeadingZeros
			template< class MaskType >
			RED_INLINE MaskType CountLeadingZeros( MaskType mask )
			{
				RED_UNUSED( mask );
				RED_HALT( "Type not supported!" );
				return 0;
			}

			//////////////////////////////////////////////////////////////////////
			// Log2
			//	Fast integer log2
			template< class ValueType > 
			RED_INLINE ValueType Log2( ValueType value )
			{
				return BitScanReverse< ValueType >( value );
			}

			/////////////////////////////////////////////////////////////////////
			// BitScanForward <Uint32>
			//	
			template<> RED_INLINE Uint32 BitScanForward<Uint32>( Uint32 mask )
			{
#ifdef RED_COMPILER_MSC
				if( mask != 0 )		// Result is undefined if mask = 0
				{
					DWORD theIndex = 0;
					::_BitScanForward( &theIndex, mask );
					return static_cast<Uint32>( theIndex );	
				}
				else
				{
					return 0;
				}
#else
				return mask == 0 ? 0 : __tzcnt_u32( mask );
#endif
			}

#ifdef RED_ARCH_X64
			/////////////////////////////////////////////////////////////////////
			// CountTrailingZeros <Uint64>
			//	
			template <> RED_INLINE Uint64 BitScanForward<Uint64>( Uint64 mask )
			{
#if defined( RED_COMPILER_MSC )
				if( mask != 0 )
				{
					DWORD theIndex = 0;
					::_BitScanForward64( &theIndex, mask );
					return static_cast<Uint64>( theIndex );		// Returns 0 if mask is 0
				}
				else
				{
					return 0;
				}
#else
				return mask == 0 ? 0 : __tzcnt_u64( mask );
#endif
			}
#endif

			//////////////////////////////////////////////////////////////////////
			// BitScanReverse <Uint32>
			//	
			template <> RED_INLINE Uint32 BitScanReverse<Uint32>( Uint32 mask )
			{
#ifdef RED_COMPILER_MSC
				if( mask != 0 )
				{
					DWORD theIndex = 0;
					::_BitScanReverse( &theIndex, mask );
					return static_cast<Uint32>( theIndex );
				}
				else
				{
					return 0;
				}
#else
				return mask == 0 ? 0 : ( 31 - __lzcnt32( mask ) );
#endif
			}

#ifdef RED_ARCH_X64
			//////////////////////////////////////////////////////////////////////
			// BitScanReverse <Uint64>
			//	
			template <> RED_INLINE Uint64 BitScanReverse<Uint64>( Uint64 mask )
			{
#if defined( RED_COMPILER_MSC )
				if( mask != 0 )
				{
					DWORD theIndex = 0;
					::_BitScanReverse64( &theIndex, mask );
					return static_cast<Uint64>( theIndex );
				}
				else
				{
					return 0;
				}
#else
				return mask == 0 ? 0 : ( 63 - __lzcnt64( mask ) );
#endif
			}

#endif	//	RED_ARCH_X64

			template<> RED_INLINE Uint32 CountLeadingZeros<Uint32>( Uint32 mask )
			{
				if ( mask != 0 )
				{
					Uint32 index = BitScanReverse( mask );
					return 31 - index;
				}
				return 0;
			}

#ifdef RED_ARCH_X64
			template<> RED_INLINE Uint64 CountLeadingZeros<Uint64>( Uint64 mask )
			{
				if ( mask != 0 )
				{
					Uint64 index = BitScanReverse( mask );
					return 63 - index;
				}
				return 0;
			}
#endif	//	RED_ARCH_X64

			RED_INLINE Uint64 RotateLeft( Uint64 value, Int32 shift )
			{
#if defined( RED_COMPILER_MSC )
				return _rotl64( value, shift );
#else
				return ( value << shift ) | ( value >> ( 64 - shift ));
#endif // RED_COMPILER_MSC
			}
		}
	}
}

#endif