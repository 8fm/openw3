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
#ifndef PROXYCENTRAL_CONNECTED

#include "ParameterNodeProxyLocal.h"
#include "IRanSeqContainerProxy.h"

class RanSeqContainerProxyLocal : public ParameterNodeProxyLocal
								, virtual public IRanSeqContainerProxy
{
public:
	RanSeqContainerProxyLocal( AkUniqueID in_id );
	virtual ~RanSeqContainerProxyLocal();

	virtual void Mode( AkContainerMode in_eMode	);
	virtual void IsGlobal( bool in_bIsGlobal );

	// Sequence mode related methods
	virtual void SetPlaylist( void* in_pvListBlock, AkUInt32 in_ulParamBlockSize );
	virtual void ResetPlayListAtEachPlay( bool in_bResetPlayListAtEachPlay );
	virtual void RestartBackward( bool in_bRestartBackward );
	virtual void Continuous( bool in_bIsContinuous );
	virtual void ForceNextToPlay( AkInt16 in_position, AkWwiseGameObjectID in_gameObjPtr, AkPlayingID in_playingID );
	virtual AkInt16 NextToPlay( AkWwiseGameObjectID in_gameObjPtr );

	// Random mode related methods
	virtual void RandomMode( AkRandomMode in_eRandomMode );
	virtual void AvoidRepeatingCount( AkUInt16 in_count );
	virtual void SetItemWeight( AkUniqueID in_itemID, AkUInt32 in_weight );
	virtual void SetItemWeight( AkUInt16 in_position, AkUInt32 in_weight );

	virtual void Loop( bool in_bIsLoopEnabled, bool in_bIsLoopInfinite, AkInt16 in_loopCount, AkInt16 in_loopModMin, AkInt16 in_loopModMax );
	virtual void TransitionMode( AkTransitionMode in_eTransitionMode );
	virtual void TransitionTime( AkTimeMs in_transitionTime, AkTimeMs in_rangeMin, AkTimeMs in_rangeMax );
};
#endif
#endif // #ifndef AK_OPTIMIZED
