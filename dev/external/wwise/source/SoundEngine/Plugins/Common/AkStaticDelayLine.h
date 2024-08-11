/***********************************************************************
The content of this file includes source code for the sound engine
portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
Two Source Code" as defined in the Source Code Addendum attached
with this file.  Any use of the Level Two Source Code shall be
subject to the terms and conditions outlined in the Source Code
Addendum and the End User License Agreement for Wwise(R).

Version: v2013.2.9  Build: 4872
Copyright (c) 2006-2014 Audiokinetic Inc.
***********************************************************************/

// Static delay line (where delay line is known at compile time and controlled through template)
// Meant to be used for control rate delay lines
// Could be used to create high order filtering classes

#ifndef _AK_STATICDELAYLINE_H_
#define _AK_STATICDELAYLINE_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Tools/Common/AkAssert.h>

namespace DSP
{
	template < class T_TYPE, AkUInt32 T_DELAYLENGTH > 
	class CAkStaticDelayLine
	{
	public:
		CAkStaticDelayLine()
			: uCurOffset( 0 ) {}
		void Reset( );

		T_TYPE ReadAndWriteValue( T_TYPE in_In )
		{
			T_TYPE Out = Delay[uCurOffset];		
			Delay[uCurOffset] = in_In;
			++uCurOffset;
			if ( uCurOffset == T_DELAYLENGTH )
				uCurOffset = 0;
			return Out;
		}

		AkReal32 ReadPastValueAt( AkUInt32 in_uDelay )
		{
			AKASSERT ( in_uDelay <= T_DELAYLENGTH );
			AkUInt32 uDelayIndex = ( uCurOffset + T_DELAYLENGTH - in_uDelay ) % T_DELAYLENGTH;
			return Delay[uCurOffset];
		}

		AkReal32 ReadOldestValue( )
		{
			return Delay[uCurOffset];
		}

		void WriteValue( T_TYPE in_In )
		{
			Delay[uCurOffset] = in_In;
			++uCurOffset;
			if ( uCurOffset == T_DELAYLENGTH )
				uCurOffset = 0;
		}

	protected:
		T_TYPE Delay[T_DELAYLENGTH];
		AkUInt32 uCurOffset;			// DelayLine line read/write position	
	};
} // namespace DSP


#endif // _AK_STATICDELAYLINE_H_
