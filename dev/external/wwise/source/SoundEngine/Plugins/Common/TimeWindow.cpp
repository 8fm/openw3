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

#include "TimeWindow.h"
#include <AK/Tools/Common/AkAssert.h>
#define _USE_MATH_DEFINES // M_PI
#include <math.h>

namespace DSP
{
#ifndef __SPU__

	AKRESULT CAkTimeWindow::Init(	
		AK::IAkPluginMemAlloc *	in_pAllocator,
		AkUInt32				in_uWindowSize,
		WindowType				in_eWindowType,
		bool					in_bWeighthedOLAWin, /* = false */
		bool					in_bZeroPhase /* = false */ )
	{
		AKASSERT( m_uWindowSize % 2 == 0 ); // Exploits symmetry of even size windows to save memory
		m_uWindowSize = in_uWindowSize;
		// Window are symmetrical around middle so only store half of it
		m_pfWindowData = (AkReal32 *)AK_PLUGIN_ALLOC( in_pAllocator, AK_ALIGN_SIZE_FOR_DMA(m_uWindowSize/2*sizeof(AkReal32)) );
		if ( m_pfWindowData == NULL )
			return AK_InsufficientMemory;

		switch ( in_eWindowType )
		{
		default:
		case WINDOWTYPE_RECTANGULAR:
			RectangularWindow( in_bWeighthedOLAWin, in_bZeroPhase );
			break;
		case WINDOWTYPE_HAMMING:
			HammingWindow( in_bWeighthedOLAWin, in_bZeroPhase );
			break;
		case WINDOWTYPE_HANN:
			HannWindow( in_bWeighthedOLAWin, in_bZeroPhase );
			break;
		case WINDOWTYPE_BLACKMAN:
			BlackmanWindow( in_bWeighthedOLAWin, in_bZeroPhase );
			break;
		}


		return AK_Success;
	}

	void CAkTimeWindow::Term( AK::IAkPluginMemAlloc * in_pAllocator )
	{
		if ( m_pfWindowData )
		{	
			AK_PLUGIN_FREE( in_pAllocator, m_pfWindowData );
			m_pfWindowData = NULL;
		}
	}

	// Note: On SPU lower level routine called directly with local storage pointers
	void CAkTimeWindow::Apply( AkReal32 * io_pfBuf, AkUInt32 in_uNumFrames, AkReal32 in_fWinGain /* = 1.f */ )
	{
		if ( in_fWinGain == 1.f )
			Apply( io_pfBuf, in_uNumFrames, m_pfWindowData );
		else
			Apply( io_pfBuf, in_uNumFrames, in_fWinGain, m_pfWindowData );

	}

	void CAkTimeWindow::RectangularWindow( bool in_bWeighthedOLAWin, bool in_bZeroPhase )
	{
		// This is just for sake of completness, and in prevision of zero padding support
		// Consider simply not applying the window for now!
		// Nothing to do for zero-phase window at the moment
		AkReal32 * AK_RESTRICT pfWin = m_pfWindowData;
		const AkUInt32 uWinSize = m_uWindowSize/2;
		for ( AkUInt32 i = 0; i < uWinSize; i++ )
			pfWin[i] = 1.f;

		m_fCummulativeSum = (AkReal32)m_uWindowSize;
	}

	void CAkTimeWindow::HammingWindow( bool in_bWeighthedOLAWin, bool in_bZeroPhase )
	{
		AkReal64 fPhase = in_bZeroPhase ? M_PI : 0.0;
		AkReal64 fCumSum = 0.0;
		AkReal32 * AK_RESTRICT pfWin = m_pfWindowData;
		const AkUInt32 uWinSize = m_uWindowSize/2;
		AkReal64 fPhaseDelta = (2.0 * M_PI / (m_uWindowSize-1.0));
		if ( in_bWeighthedOLAWin )
		{
			for ( AkUInt32 i = 0; i < uWinSize; i++ )
			{
				AkReal64 fSample = 0.54 - 0.46*cos(fPhase);
				pfWin[i] = (AkReal32)sqrt(fSample);
				fCumSum += fSample;
				fPhase += fPhaseDelta;
			}
		}
		else
		{
			for ( AkUInt32 i = 0; i < uWinSize; i++ )
			{
				AkReal64 fSample = 0.54 - 0.46*cos(fPhase);
				pfWin[i] = (AkReal32)fSample;
				fCumSum += fSample*fSample;
				fPhase += fPhaseDelta;
			}
		}
		m_fCummulativeSum = (AkReal32)fCumSum*2.f;
	}

	void CAkTimeWindow::HannWindow( bool in_bWeighthedOLAWin, bool in_bZeroPhase )
	{
		AkReal64 fPhase = in_bZeroPhase ? M_PI : 0.0;
		AkReal64 fCumSum = 0.0;
		AkReal32 * AK_RESTRICT pfWin = m_pfWindowData;
		const AkUInt32 uWinSize = m_uWindowSize/2;
		AkReal64 fPhaseDelta = (2.0 * M_PI / (m_uWindowSize-1.0));
		if ( in_bWeighthedOLAWin )
		{
			for ( AkUInt32 i = 0; i < uWinSize; i++ )
			{
				AkReal64 fSample = 0.5 * (1.0 - cos(fPhase));
				pfWin[i] = (AkReal32)sqrt(fSample);
				fCumSum += fSample;
				fPhase += fPhaseDelta;
			}
		}
		else
		{
			for ( AkUInt32 i = 0; i < uWinSize; i++ )
			{
				AkReal64 fSample = 0.5 * (1.0 - cos(fPhase));
				pfWin[i] = (AkReal32)fSample;
				fCumSum += fSample*fSample;
				fPhase += fPhaseDelta;
			}
		}
		m_fCummulativeSum = (AkReal32)fCumSum*2.f;
	}

	void CAkTimeWindow::BlackmanWindow( bool in_bWeighthedOLAWin, bool in_bZeroPhase )
	{
		AkReal64 fPhase = in_bZeroPhase ? M_PI : 0.0;
		AkReal64 fCumSum = 0.0;
		AkReal32 * AK_RESTRICT pfWin = m_pfWindowData;
		const AkUInt32 uWinSize = m_uWindowSize/2;
		AkReal64 fPhaseDelta = (2.0 * M_PI / (m_uWindowSize-1.0));
		if ( in_bWeighthedOLAWin )
		{
			for ( AkUInt32 i = 0; i < uWinSize; i++ )
			{
				AkReal64 fSample = 0.42 - 0.5*cos(fPhase) + 0.08*cos(2.0*fPhase);
				pfWin[i] = (AkReal32)sqrt(fSample);
				fCumSum += fSample;
				fPhase += fPhaseDelta;
			}
		}
		else
		{
			for ( AkUInt32 i = 0; i < uWinSize; i++ )
			{
				AkReal64 fSample = 0.42 - 0.5*cos(fPhase) + 0.08*cos(2.0*fPhase);
				pfWin[i] = (AkReal32)fSample;
				fCumSum += fSample*fSample;
				fPhase += fPhaseDelta;
			}
		}
		m_fCummulativeSum = (AkReal32)fCumSum*2.f;
	}

#endif

	void CAkTimeWindow::Apply( 
		AkReal32 * AK_RESTRICT io_pfBuf, 
		AkUInt32 in_uNumFrames, 
		AkReal32 * AK_RESTRICT in_pfWindow )
	{
		AKASSERT( io_pfBuf && in_pfWindow );
		AKASSERT( in_uNumFrames == m_uWindowSize );
		const AkUInt32 uWindowSize = m_uWindowSize;
		AkUInt32 i = 0;
		const AkUInt32 uHalfWindowSize = uWindowSize/2;
		for (  ; i < uHalfWindowSize; i++ )
			io_pfBuf[i] *= in_pfWindow[i];
		for (  ; i < uWindowSize; i++ )
			io_pfBuf[i] *= in_pfWindow[uWindowSize-i-1];
	}

	void CAkTimeWindow::Apply( 
		AkReal32 * AK_RESTRICT io_pfBuf, 
		AkUInt32 in_uNumFrames, 
		AkReal32 in_fWinGain,
		AkReal32 * AK_RESTRICT in_pfWindow )
	{
		AKASSERT( io_pfBuf && in_pfWindow );
		AKASSERT( in_uNumFrames == m_uWindowSize );
		const AkUInt32 uWindowSize = m_uWindowSize;
		AkUInt32 i = 0;
		const AkUInt32 uHalfWindowSize = uWindowSize/2;
		for (  ; i < uHalfWindowSize; i++ )
			io_pfBuf[i] *= in_fWinGain * in_pfWindow[i];
		for (  ; i < uWindowSize; i++ )
			io_pfBuf[i] *= in_fWinGain * in_pfWindow[uWindowSize-i-1];
	}
	
} // namespace DSP
