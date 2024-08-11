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
#pragma once

#ifndef AK_OPTIMIZED

#include "IParameterNodeProxy.h"

#include "AkRanSeqCntr.h"

class CAkRanSeqCntr;

class IRanSeqContainerProxy : virtual public IParameterNodeProxy
{
	DECLARE_BASECLASS( IParameterNodeProxy );
public:
	virtual void Mode( AkContainerMode in_eMode	) = 0;
	virtual void IsGlobal( bool in_bIsGlobal ) = 0;

	// Sequence mode related methods
    virtual void SetPlaylist( void* in_pvListBlock, AkUInt32 in_ulParamBlockSize ) = 0;
	virtual void ResetPlayListAtEachPlay( bool in_bResetPlayListAtEachPlay ) = 0;
	virtual void RestartBackward( bool in_bRestartBackward ) = 0;
	virtual void Continuous( bool in_bIsContinuous ) = 0;
	virtual void ForceNextToPlay( AkInt16 in_position, AkWwiseGameObjectID in_gameObjPtr = NULL, AkPlayingID in_playingID = NO_PLAYING_ID ) = 0;
	virtual AkInt16 NextToPlay( AkWwiseGameObjectID in_gameObjPtr = NULL ) = 0;

	// Random mode related methods
	virtual void RandomMode( AkRandomMode in_eRandomMode ) = 0;
	virtual void AvoidRepeatingCount( AkUInt16 in_count ) = 0;
	virtual void SetItemWeight( AkUniqueID in_itemID, AkUInt32 in_weight ) = 0;
	virtual void SetItemWeight( AkUInt16 in_position, AkUInt32 in_weight ) = 0;

	virtual void Loop( 
		bool in_bIsLoopEnabled, 
		bool in_bIsLoopInfinite, 
		AkInt16 in_loopCount,
		AkInt16 in_loopModMin, 
		AkInt16 in_loopModMax ) = 0;

	virtual void TransitionMode( AkTransitionMode in_eTransitionMode ) = 0;
	virtual void TransitionTime( AkTimeMs in_transitionTime, AkTimeMs in_rangeMin = 0, AkTimeMs in_rangeMax = 0 ) = 0;


	enum MethodIDs
	{
		MethodMode = __base::LastMethodID,
		MethodIsGlobal,
        MethodSetPlaylist,
		MethodResetPlayListAtEachPlay,
		MethodRestartBackward,
		MethodContinuous,
		MethodForceNextToPlay,
		MethodNextToPlay,
		MethodRandomMode,
		MethodAvoidRepeatingCount,
		MethodSetItemWeight_withID,
		MethodSetItemWeight_withPosition,

		MethodLoop,
		MethodTransitionMode,
		MethodTransitionTime,

		LastMethodID
	};
};

#endif // #ifndef AK_OPTIMIZED
