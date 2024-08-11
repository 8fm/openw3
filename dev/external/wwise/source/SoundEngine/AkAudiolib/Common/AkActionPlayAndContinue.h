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
// AkActionPlayAndContinue.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ACTION_PLAY_AND_CONTINUE_H_
#define _ACTION_PLAY_AND_CONTINUE_H_

#include "AkActionPlay.h"
#include "AkCntrHistory.h"
#include "PrivateStructures.h"
#include "AkPBI.h"

class CAkTransition;
class CAkContinuationList;
class CAkPBI;
class CAkContinuousPBI;
class CAkPBIAware;
class CAkPath;
struct AkPendingAction;
class CAkParameterNodeBase;

class CAkActionPlayAndContinue : public CAkActionPlay
{
public:
	//Thread safe version of the constructor
	static CAkActionPlayAndContinue* Create(AkActionType in_eActionType, AkUniqueID in_ulID, CAkSmartPtr<CAkContinuationList>& in_spContList );
protected:
	CAkActionPlayAndContinue(AkActionType in_eActionType, AkUniqueID in_ulID, CAkSmartPtr<CAkContinuationList>& in_spContList );
	virtual ~CAkActionPlayAndContinue();
public:
	//Execute the action
	//Must be called only by the audiothread
	//
	//Return - AKRESULT - AK_Success if all succeeded
	virtual AKRESULT Execute(
		AkPendingAction * in_pAction
		);

	// access to the private ones for the continuous pbi
	AKRESULT SetPlayStopTransition( CAkTransition* pTransition, bool in_bTransitionFading, AkPendingAction* in_pTransitionOwner );
	AKRESULT SetPauseResumeTransition( CAkTransition* pTransition, bool in_bTransitionFading, AkPendingAction* in_pTransitionOwner );
	void UnsetPlayStopTransition();
	void UnsetPauseResumeTransition();
	CAkTransition* GetPlayStopTransition( bool& out_rbIsFading );
	CAkTransition* GetPauseResumeTransition( bool& out_rbIsFading );

	void SetPathInfo(AkPathInfo* in_pPathInfo);
	//void UnsetPath();
	AkPathInfo* GetPathInfo() { return &m_PathInfo; }

	virtual void GetHistArray( AkCntrHistArray& out_rHistArray );

	void SetHistory(PlayHistory& in_rPlayHistory);
	void SetInitialPlaybackState( AkPlaybackState in_eInitialPlaybackState );
    void SetInstigator( CAkPBIAware* in_pInstigator );

	bool NeedNotifyDelay();

	void SetIsFirstPlay( bool in_bIsFirstPlay );

	void SetPauseCount( AkUInt32 in_ulPauseCount ){ m_ulPauseCount = in_ulPauseCount; }
	AkUInt32 GetPauseCount(){ return m_ulPauseCount; }

	PlayHistory& History();

	// Tell the action who it must cross fade with
	void SetFadeBack( CAkContinuousPBI* in_pPBIToNotify, AkTimeMs in_CrossFadeTime );

	// Tell the action to not perform the query
	void UnsetFadeBack( CAkContinuousPBI* in_pPBIToCheck );

	// Tell the action to be sample accurate with the specified PBI ID
	void SetSAInfo( AkUniqueID in_seqID );

	void StartAsPaused();

	void Resume();

	// Called when breaking a pending item.
	// return true if impossible to fix.
	bool BreakToNode( CAkParameterNodeBase * in_pNodeToTest, CAkRegisteredObj* in_pGameObj, AkPendingAction* in_pPendingAction );

	PlaybackTransition	m_PBTrans;

	AkPathInfo			m_PathInfo;

	const CAkSmartPtr<CAkContinuationList>& GetContinuationList(){ return m_spContList; }

private:
	CAkSmartPtr<CAkContinuationList> m_spContList;

	PlayHistory			m_PlayHistory;

	//For fading out last one on cross fade
	CAkContinuousPBI*	m_pPreviousPBI;
	AkTimeMs			m_FadeOutTimeForLast;

	AkPlaybackState m_InitialPlaybackState;

	// ID of the previous PBI to synch with (used only in sample accuracy transitions)
	AkUniqueID		m_SA_PBIID;

	bool m_bNeedNotifyDelay;
	bool m_bIsfirstPlay;

public:
	bool m_bSkipDelay;
	AkPBIParams::ePBIType m_ePBIType;

private:
	AkPendingAction* m_pTransitionOwner;

    CAkPBIAware* m_pInstigator; // Audio object that caused the creation of the ContinuousPBI

	AkUInt32 m_ulPauseCount;

	CAkParameterNodeBase* m_pPathOwner;

};
#endif
