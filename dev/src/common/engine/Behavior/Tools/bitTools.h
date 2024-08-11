#pragma once
#include "..\..\..\core\types.h"

namespace BehaviorGraphTools
{
	// Tools that helps managing bit mask, ver 0.000001
	// unfortunately, right know it is very easy to use this tools incorrectly, so be warned!
	// TO DO:
	// - add managing of 'array mask'
	// - add checks to make sure we don't break the tools

	namespace BitTools
	{
		template < typename TBitMask > RED_INLINE void SetFlag( TBitMask& outMask, TBitMask flag );		//< Sets given bit flag(s) in mask
		template < typename TBitMask > RED_INLINE void ClearFlag( TBitMask& outMask, TBitMask flag );	//< Clears given bit flag(s) in mask
		template < typename TBitMask > RED_INLINE Bool IsFlagSet( TBitMask& outMask, TBitMask flag );	//< Return true if unless one flag is set in mask
		template < typename TBitMask > RED_INLINE void ClearMask( TBitMask& outMask );					//< Sets all bit to 0;
		template < typename TBitMask > RED_INLINE Bool IsMaskCleared( TBitMask mask );					//< True, if all mask bits are equal to 0
		
		template < typename TBitMask > RED_INLINE TBitMask NumberToFlag( TBitMask number );				//< Convert a number to flag ( so max number is 32! )
		template < typename TBitMaskType >  RED_INLINE Bool IsNumberConvertibleToFlag( TBitMaskType number );	//< Returns true if number can be converted to flag


		//////////////////////////////////////////////////////////////////////////
		//
		// IMPLEMENTATION (INLINES):
		//
		//////////////////////////////////////////////////////////////////////////

		template < typename TBitMask > 
		RED_INLINE void SetFlag( TBitMask& outMask, TBitMask flag )
		{
			outMask |= flag;
		}

		template < typename TBitMask > 
		RED_INLINE void ClearFlag( TBitMask& outMask, TBitMask flag )
		{
			outMask &= ~flag;
		}

		template < typename TBitMask > 
		RED_INLINE Bool IsFlagSet( TBitMask& outMask, TBitMask flag )
		{
			return ( outMask & flag ) != 0;
		}

		template < typename TBitMask > 
		RED_INLINE void ClearMask( TBitMask& outMask )
		{
			outMask = 0;
		}

		template < typename TBitMask > 
		RED_INLINE Bool IsMaskCleared( TBitMask mask )
		{
			return mask == 0;
		}

		template < typename TBitMask > 
		RED_INLINE TBitMask NumberToFlag( TBitMask number )
		{
			RED_ASSERT( IsNumberConvertibleToFlag( number ) );
			return (TBitMask)1 << number;
		}

		template < typename TBitMaskType> 
		RED_INLINE Bool IsNumberConvertibleToFlag( TBitMaskType number )
		{
			// TODO: rewrite this, and think how this should be done!
			return number < sizeof( TBitMaskType ) * CHAR_BIT; 
		}
	}
}

