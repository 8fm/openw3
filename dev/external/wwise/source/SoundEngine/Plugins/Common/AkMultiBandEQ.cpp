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

#include "AkMultiBandEQ.h"
#include <AK/Tools/Common/AkObject.h> // Placement new
#include <AK/Tools/Common/AkAssert.h>
#include <stdlib.h>

namespace DSP
{

#ifndef __SPU__

CAkMultiBandEQ::CAkMultiBandEQ():
	m_pFilters(NULL),
	m_uNumFilters(0),
	m_uEnabledBandMask( 0xFFFFFFFF ),
	m_uNumBands(0),
	m_uNumChannels(0)
{

}

AKRESULT CAkMultiBandEQ::Init( AK::IAkPluginMemAlloc * in_pAllocator, AkUInt16 in_uNumChannels, AkUInt16 in_uNumBands )
{
	m_uNumBands = in_uNumBands;
	m_uNumChannels = in_uNumChannels;
	m_uNumFilters = in_uNumBands * in_uNumChannels;
	AKASSERT( in_uNumBands < sizeof(m_uEnabledBandMask)*8 );
	if (m_uNumFilters)
	{
		m_pFilters = (DSP::BiquadFilterMono*) AK_PLUGIN_ALLOC( in_pAllocator, AK_ALIGN_SIZE_FOR_DMA( m_uNumFilters*sizeof(DSP::BiquadFilterMono) ) );
		if ( !m_pFilters )
			return AK_InsufficientMemory;
		for ( AkUInt32 i = 0; i < m_uNumFilters; ++i )
		{
			AkPlacementNew( &m_pFilters[i]) DSP::BiquadFilterMono();
		}		
	}
	return AK_Success;
}

void CAkMultiBandEQ::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	if ( m_pFilters )
	{
		AK_PLUGIN_FREE( in_pAllocator, m_pFilters );
		// No need to call filter's destructor
		m_pFilters = NULL;
	}
}

void CAkMultiBandEQ::Reset()
{
	for (AkUInt32 i = 0; i < m_uNumFilters; i++)
	{
		m_pFilters[i].Reset( );
	}
}

// Compute filter coefficients for a given band
void CAkMultiBandEQ::SetCoefficients( 
									 AkUInt32 in_uBand,
									 AkUInt32 in_uSampleRate, 
									 BiquadFilterMono::FilterType in_eCurve, 			
									 AkReal32 in_fFreq, 
									 AkReal32 in_fGain /*= 0.f*/, 
									 AkReal32 in_fQ /*= 1.f*/ )
{
	// Note: Possible improvement: Share coefficients accross channels
	for (AkUInt32 uChannel = 0; uChannel < m_uNumChannels; uChannel++)
	{
		AkUInt32 uFilterIndex = (uChannel*m_uNumBands) + in_uBand;
		m_pFilters[uFilterIndex].ComputeCoefs(in_eCurve, (AkReal32)in_uSampleRate, in_fFreq, in_fGain, in_fQ);
	}
}

// Bypass/unbypass a given band
void CAkMultiBandEQ::SetBandActive( AkUInt32 in_uBand, bool in_bActive )
{
	AkUInt32 BandMask = (1<<in_uBand);
	if ( in_bActive )
		m_uEnabledBandMask |= BandMask;
	else
		m_uEnabledBandMask &= ~BandMask;
}

#endif

#ifndef AK_PS3

// All channels
void CAkMultiBandEQ::ProcessBuffer(	AkAudioBuffer * io_pBuffer ) 
{
	ProcessBufferInternal( m_pFilters, io_pBuffer ); 
}

#endif

#ifdef __SPU__
// All channels, position independent code takes in local storage array of filter objects
void CAkMultiBandEQ::ProcessBuffer(			
								   AkAudioBuffer * io_pBuffer, 
								   DSP::BiquadFilterMono * pFilter ) 
{
	ProcessBufferInternal( pFilter, io_pBuffer ); 
}
#endif

// All channels
void CAkMultiBandEQ::ProcessBufferInternal(	
	DSP::BiquadFilterMono * pFilter,
	AkAudioBuffer * io_pBuffer ) 
{
	const AkUInt32 uNumChannels = io_pBuffer->NumChannels();
	const AkUInt32 uNumFrames = io_pBuffer->uValidFrames;
	AKASSERT( uNumChannels <= m_uNumChannels );
	for ( AkUInt32 i = 0; i < uNumChannels; ++i )
	{
		AkReal32 * pfChannel = io_pBuffer->GetChannel( i );
		const AkUInt32 uFilterIndexOffset = i*m_uNumBands;
		for (AkUInt32 uTCBands = 0; uTCBands < m_uNumBands; uTCBands++)
		{
			if ( (1<<uTCBands) & m_uEnabledBandMask )
				pFilter[uFilterIndexOffset+uTCBands].ProcessBuffer(pfChannel, uNumFrames);
		}
	}
}

} // namespace DSP
