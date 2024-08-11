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

//////////////////////////////////////////////////////////////////////
//
// AkSoundBase.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AKSOUND_BASE_H_
#define _AKSOUND_BASE_H_
#include "AkParameterNode.h"
#include <AK/Tools/Common/AkListBareLight.h>
#include "AkPBI.h"

class CAkSoundBase : public CAkParameterNode
{
public:

	virtual AKRESULT PlayToEnd( CAkRegisteredObj * in_pGameObj, CAkParameterNodeBase* in_pNodePtr, AkPlayingID in_PlayingID = AK_INVALID_PLAYING_ID );

	virtual void ParamNotification( NotifParams& in_rParams );

	// Notify the children PBI that a mute variation occured
	virtual void MuteNotification(
		AkReal32		in_fMuteRatio,		// New muting ratio
		AkMutedMapItem& in_rMutedItem,		// Instigator's unique key
		bool			in_bIsFromBus = false
		);

	// Notify the children PBI that a mute variation occured
	virtual void MuteNotification(
		AkReal32 in_fMuteRatio,				// New muting ratio
		CAkRegisteredObj *	in_pGameObj,	// Target Game Object
		AkMutedMapItem& in_rMutedItem,		// Instigator's unique key
		bool in_bPrioritizeGameObjectSpecificItems
		);

	virtual void ForAllPBI( 
		AkForAllPBIFunc in_funcForAll,
		CAkRegisteredObj * in_pGameObj, // If NULL, target PBIs playing on all game objects
		void * in_pCookie );

	// Notify the children PBI that a change int the Positioning parameters occured from RTPC
	virtual void PropagatePositioningNotification(
		AkReal32			in_RTPCValue,	// 
		AkRTPC_ParameterID	in_ParameterID,	// RTPC ParameterID, must be a Positioning ID.
		CAkRegisteredObj *		in_GameObj,		// Target Game Object
		void*				in_pExceptArray = NULL
		);

	// Notify the children PBI that a major change occured and that the 
	// Parameters must be recalculated
	virtual void RecalcNotification();

	virtual void NotifyBypass(
		AkUInt32 in_bitsFXBypass,
		AkUInt32 in_uTargetMask,
		CAkRegisteredObj * in_pGameObj,
		void* in_pExceptArray = NULL
		);

	virtual void UpdateFx(
		AkUInt32	   	in_uFXIndex
		);

#ifndef AK_OPTIMIZED
	virtual void InvalidatePaths();
#endif
	
	AkInt16 Loop();
	AkReal32 LoopStartOffset() const;
	AkReal32 LoopEndOffset() const;
	AkReal32 LoopCrossfadeDuration() const;
	void LoopCrossfadeCurveShape( AkCurveInterpolation& out_eCrossfadeUpType,  AkCurveInterpolation& out_eCrossfadeDownType) const;

	void GetTrim( AkReal32& out_fBeginTrimOffsetSec, AkReal32& out_fEndTrimOffsetSec ) const;
	void GetFade(	AkReal32& out_fBeginFadeOffsetSec, AkCurveInterpolation& out_eBeginFadeCurveType, 
					AkReal32& out_fEndFadeOffsetSec, AkCurveInterpolation& out_eEndFadeCurveType  ) const;

	void MonitorNotif(AkMonitorData::NotificationReason in_eNotifReason, AkGameObjectID in_GameObjID, UserParams& in_rUserParams, PlayHistory& in_rPlayHistory);

protected:
	CAkSoundBase( AkUniqueID in_ulID );
	virtual ~CAkSoundBase();
};

#endif //_AKSOUND_BASE_H_
