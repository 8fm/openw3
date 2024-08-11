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

#ifndef _AKALGOTUNINGS_H_
#define _AKALGOTUNINGS_H_

// Settings to tune the behavior of many modules in the reverb algorithm

#include <AK/SoundEngine/Common/AkTypes.h>

static const AkUInt32 MAXNUMREVERBUNITS = 16;
static const AkUInt32 NUMDIFFUSIONALLPASSFILTERS = 4;
static const AkUInt32 MAXNUMOUTPUTSIGNALS = 6;
static const AkUInt32 MAXNUMTONECONTROLFILTERS = 6;

struct AlgorithmTunings
{
	AkReal32	fDensityDelayMin; //ms 
	AkReal32	fDensityDelayMax; //ms
	AkReal32	fDensityDelayRdmPerc; // %
	AkReal32	fRoomShapeMin;
	AkReal32	fRoomShapeMax;
	AkReal32	fDiffusionDelayScalePerc; //ms
	AkReal32	fDiffusionDelayMax; //ms
	AkReal32	fDiffusionDelayRdmPerc; // %
	AkReal32	fDCFilterCutFreq; // Hz
	AkReal32	fReverbUnitInputDelay; // ms
	AkReal32	fReverbUnitInputDelayRmdPerc; // %
};

// Default tunings (used only when parameter block is invalid)
static struct AlgorithmTunings g_AlgoTunings = {
	8.f,		// fDensityDelayMin (ms) 
	50.f,		// fDensityDelayMax (ms)
	2.f,		// fDensityDelayRdmPerc (%)
	0.1f,		// fRoomShapeMin // governs Tmax to Tmin ratio
	0.8f,		// fRoomShapeMax // corresponds to Schroeder 1.5:1 Tmax:Tmin ratio
	66.f,		// fDiffusionDelayScalePerc (ms) // Allpass delay lengths are scaled from 5ms to 12 ms
	15.f,		// fDiffusionDelayMax (ms)
	5.f,		// fDiffusionDelayRdmPerc (%)
	40.f,		// fDCFilterCutFreq (Hz)
	0.5f,		// fReverbUnitInputDelay (ms)
	50.f,		// fReverbUnitInputDelayRmdPerc (%)
};


#endif // _AKALGOTUNINGS_H_
