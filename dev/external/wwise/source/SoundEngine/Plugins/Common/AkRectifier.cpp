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

#include "AkRectifier.h"
#include <AK/Tools/Common/AkAssert.h>
#include <AK/SoundEngine/Common/AkFPUtilities.h>

// Rectification of signals
// 0 is none, 50 is half wave and 100 is full wave

namespace DSP
{

#ifndef __SPU__ 

CAkRectifier::CAkRectifier() :	
	m_fHWRThresh(0.f),
	m_fPrevHWRThresh(0.f),
	m_fFWRGain(0.f),
	m_fPrevFWRGain(0.f),
	m_eRectifierMode(RectifierMode_Bypass)
{

}

void CAkRectifier::SetRectification( AkReal32 in_fRectification, bool in_bFirstSet /* = false */)
{	
	AKASSERT( in_fRectification >= 0.f && in_fRectification <= 100.f );
	if ( in_fRectification == 0.f )
		m_eRectifierMode = RectifierMode_Bypass;
	else if ( in_fRectification <= 50.f )
		m_eRectifierMode = RectifierMode_HalfWave;
	else
		m_eRectifierMode = RectifierMode_FullWave;

	m_fHWRThresh = -(50.f-AK_FPMin(in_fRectification,50.f))/50.f; // Map (0,50) to (-1,0)
	m_fFWRGain = (in_fRectification-50.f)/50.f; // Map (50,100) to (0,1)
	if ( in_bFirstSet )	
	{
		// Avoid initial ramping
		m_fPrevHWRThresh = m_fHWRThresh; 
		m_fPrevFWRGain = m_fFWRGain;
	}
}

#endif

void CAkRectifier::ProcessBuffer( AkAudioBuffer * io_pBuffer ) 
{
	if ( m_eRectifierMode != RectifierMode_Bypass )
	{
		const AkUInt32 uNumChannels = io_pBuffer->NumChannels();
		const AkUInt32 uNumFrames = io_pBuffer->uValidFrames;
		for ( AkUInt32 i = 0; i < uNumChannels; ++i )
		{
			AkReal32 * pfChannel = io_pBuffer->GetChannel( i );
			ProcessChannel( pfChannel, uNumFrames );	
		}
	}
	// Ramp complete
	m_fPrevHWRThresh = m_fHWRThresh; 
	m_fPrevFWRGain = m_fFWRGain;
}

// Rectification DSP process
void CAkRectifier::ProcessChannel( AkReal32 * io_pfBuffer, AkUInt32 in_uNumFrames )
{
	AkReal32 * AK_RESTRICT pfBuf = (AkReal32 * AK_RESTRICT) io_pfBuffer;
	const AkReal32 * pfEnd = io_pfBuffer + in_uNumFrames;
	if ( m_eRectifierMode == RectifierMode_HalfWave )
	{
		// Half wave rectification only	
		const AkReal32 fTargetHWRThresh = m_fHWRThresh;
		const AkReal32 fPrevHWRThresh = m_fPrevHWRThresh;
		const AkReal32 fHWRThreshInc = (fTargetHWRThresh-fPrevHWRThresh)/in_uNumFrames;
		AkReal32 fCurHWRThresh = fPrevHWRThresh;
		while ( pfBuf < pfEnd )
		{	
			AkReal32 fIn = *pfBuf;
			AkReal32 fHWR = AK_FPMax( fIn, fCurHWRThresh );
			fCurHWRThresh += fHWRThreshInc;
			*pfBuf++ = fHWR;			
		}
	}
	else
	{
		// Half + full wave rectification only
		const AkReal32 fTargetHWRThresh = m_fHWRThresh;
		const AkReal32 fPrevHWRThresh = m_fPrevHWRThresh;
		const AkReal32 fHWRThreshInc = (fTargetHWRThresh-fPrevHWRThresh)/in_uNumFrames;
		AkReal32 fCurHWRThresh = fPrevHWRThresh;
		/*const AkReal32 fFWRGain = m_fFWRGain;*/
		const AkReal32 fTargetFWRGain = m_fFWRGain;
		const AkReal32 fPrevFWRGain = m_fPrevFWRGain;
		const AkReal32 fFWRGainInc = (fTargetFWRGain-fPrevFWRGain)/in_uNumFrames;
		AkReal32 fCurFWRGain = fPrevFWRGain;
		while ( pfBuf < pfEnd )
		{
			AkReal32 fIn = *pfBuf;
			AkReal32 fHWR = AK_FPMax( fIn, fCurHWRThresh );
			fCurHWRThresh += fHWRThreshInc;
			AkReal32 fOut = fHWR;	
			/*if ( fIn <= 0.f )
				fOut = fOut - fIn*fFWRGain;*/
			AK_FPSetValLTE( fIn, 0.f, fOut, fOut - fIn*fCurFWRGain ); 
			fCurFWRGain += fFWRGainInc;
			*pfBuf++ = fOut;			
		}
	}
}

} // namespace DSP
