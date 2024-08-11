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

#ifndef _AK_LENGINE_STRUCTS_COMMON_H_
#define _AK_LENGINE_STRUCTS_COMMON_H_

#include "AkCommon.h"

#if !defined (AK_WII_FAMILY_HW) && defined(AK_MOTION)
	#include "AkFeedbackStructs.h"
	#define FEEDBACK_STRUCTS
#endif

enum VPLNodeState
{
	NodeStateInit			= 0
	,NodeStatePlay			= 1
	,NodeStateStop			= 2
	,NodeStatePause			= 3

	,NodeStateIdle			= 4 // only used for busses for idle time due to virtual voices / wait for first output
	,NodeStateProcessing	= 5 // PS3-specific state to handle calling ProcessDone
};

struct AkSinkStats
{
	AkReal32 m_fOutMin;				// low peak of output
	AkReal32 m_fOutMax;				// high peak of output
	AkReal32 m_fOutSum;				// sum of output samples
	AkReal32 m_fOutSumOfSquares;	// sum of output samples^2
	AkUInt32 m_uOutNum;				// number of output samples
} AK_ALIGN_DMA;

#ifndef AK_3DS

struct AkVPLState : public AkPipelineBuffer
{	
	AKRESULT	  result;

	bool		  bPause; // VPL needs to pause at end of this frame
	bool		  bStop;  // VPL needs to stop at end of this frame
	bool          bIsAuxRoutable;
	bool		  bAudible;

#ifdef AK_PS3
	AKRESULT	  resultPrevious; // backup value when passthrough
	short         iPos;
	short		  iEnv;

	// Push control values to SPU.
	struct
	{
		AkReal32 fControlValue;
		AkReal32 fLastControlValue;

	} aMergedValues[ AK_MAX_AUX_SUPPORTED ];

	AkVPLState& operator=(const AkVPLState& B)
	{
		*(AkPipelineBuffer*)this = *(AkPipelineBuffer*)&B;
		result = B.result;

		bPause = B.bPause; 
		bStop = B.bStop;  
		bIsAuxRoutable = B.bIsAuxRoutable;
		bAudible = B.bAudible;
		resultPrevious = B.resultPrevious;
		iPos = B.iPos;
		iEnv = B.iEnv;
		//Don't copy environments.

		return *this;
	}
#endif
} AK_ALIGN_DMA;

#ifdef FEEDBACK_STRUCTS
struct AkFeedbackVPLData
{
	AkPipelineBufferBase LPFBuffer;			//Buffer for LPF processing.
#ifdef AK_PS3
	AkUInt8				iPlayerCount;		//Number of states
	AkVPLState			pStates[1];			//Per player state.	
#endif
};
#endif

class AkVPL;
class CAkVPLSrcCbxNode;

struct AkRunningVPL
{
	AkVPLState state;			// Must stay first.

#ifdef AK_PS3
	AkRunningVPL * pNextLightItem; // for AkListRunningVPL
	AkAudioMix*	pMotionMixes;		//Mixes, per player(ForMotion)
#endif

	CAkVPLSrcCbxNode * pCbx;

#ifdef FEEDBACK_STRUCTS
	AkFeedbackVPLData*	pFeedbackData;	//For Audio-To-Motion pipeline only.
#endif

#ifdef AK_MOTION
	bool				bFeedbackVPL;	//Is it for the feedback pipeline or audio pipeline
#endif
}AK_ALIGN_DMA;

#endif // #ifndef AK_3DS

#endif //_AK_LENGINE_STRUCTS_COMMON_H_
