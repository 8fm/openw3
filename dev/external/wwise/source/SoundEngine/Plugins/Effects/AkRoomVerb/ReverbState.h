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

// Reverb algorithm made up of 1 to 8 FDN 4x4 reverb unit
// Each reverb unit has an all-pass filter
// A DC filter is applied on the sum of the reverb path
// The reverb input is a constant power mixdown of all channels
// Low and High shelf filters are applied on the mixdown signal feeding the reverb units
// (Pre-)Delay is applied before entering the reverb units
// The output of the sum reverb units is decorrelated for each output channel
// The decorrelated channel output and the input are mixed and scaled according to wet/dry levels
// The delay lengths of all FDNs are spread evenly according to Density and Coloration Parameters
 
#ifndef _AKREVERBSTATE_H_
#define _AKREVERBSTATE_H_

#define NUMPROCESSFRAMES (256)

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>

#include "AlgoTunings.h"
#include "AkRoomVerbFXParams.h"
#include "ERUnitDual.h"
#include "AllPassFilter.h"
#include "OnePoleZeroHPFilter.h"
#include "DelayLine.h"
#include "FDN4.h"
#include "BiquadFilter.h"
#include <AK/Plugin/PluginServices/AkFXTailHandler.h>

// Reverb algorithm state
struct ReverbState
{
	DSP::AllpassFilter			DiffusionFilters[NUMDIFFUSIONALLPASSFILTERS];
	DSP::OnePoleZeroHPFilter	DCFilter[MAXNUMOUTPUTSIGNALS];
	DSP::DelayLine				ERDelay;
	DSP::DelayLine				ReverbDelay;
	DSP::DelayLine				ERFrontBackDelay[2];
	AkFXTailHandler				FXTailHandler;
	AkUInt32					uTailLength;
	AkReal32					fReverbUnitsMixGain;
	AkUInt32					uNumReverbUnits;
	AkUInt32					uSampleRate;
	AkUInt8						uTCFilterIndex[4];
	AkUInt8						uNumERSignals;
	bool						bIsSentMode;
} AK_ALIGN_DMA;

// Allocated per reverb unit used
struct ReverbUnitState
{
	DSP::FDN4			ReverbUnits;
	DSP::DelayLine		RUInputDelay;
};

// Allocated per output reverb signal to process
struct ToneControlsState
{	
	DSP::BiquadFilterMono	Filter;
	FilterInsertType	FilterPos;
};

#endif // _AKREVERBSTATE_H_
