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
// AkSwitchCntr.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _SWITCH_CNTR_H_
#define _SWITCH_CNTR_H_

#include "AkMultiPlayNode.h"
#include "AkSwitchAware.h"
#include "AkKeyList.h"
#include "AkCntrHistory.h"
#include "AkPreparationAware.h"

enum AkOnSwitchMode
{
	AkOnSwitchMode_PlayToEnd,
	AkOnSwitchMode_Stop
#define ONSWITCHMODE_NUM_STORAGE_BIT 3
};

struct AkSwitchNodeParams
{
	AkTimeMs		FadeOutTime;
	AkTimeMs		FadeInTime;
	AkUInt8			eOnSwitchMode		:ONSWITCHMODE_NUM_STORAGE_BIT;
	AkUInt8			bIsFirstOnly		:1;
	AkUInt8			bContinuePlayback	:1;
};

class CAkSwitchPackage
{
public:
	typedef AkArray< AkUniqueID, AkUniqueID, ArrayPoolDefault, 1 > AkIDList;

	AkIDList m_list;

	void	 Term(){ m_list.Term(); }
};

// class corresponding to a Switch container
//
// Author:  alessard
class CAkSwitchCntr : public CAkMultiPlayNode
                     ,public CAkSwitchAware
					 ,public CAkPreparationAware
{
public:

	// Thread safe version of the constructor
	static CAkSwitchCntr* Create( AkUniqueID in_ulID = 0 );

	AKRESULT	Init();
	void		Term();

	virtual void OnPreRelease();

	//Return the node category
	virtual AkNodeCategory NodeCategory();

	// Play the specified node
    //
    // Return - AKRESULT - Ak_Success if succeeded
	virtual AKRESULT PlayInternal( AkPBIParams& in_rPBIParams );

	virtual AKRESULT ExecuteAction( ActionParams& in_rAction );
	virtual AKRESULT ExecuteActionExcept( ActionParamsExcept& in_rAction );

	// This function is redefined because the current input 
	// must be removed in authoring mode when the current input is removed	
	virtual void RemoveChild(
		CAkParameterNodeBase* in_pChild
		);

	using CAkMultiPlayNode::RemoveChild; //This will remove warnings on 3DS, WiiU and Vita compilers

	virtual void SetSwitch( AkUInt32 in_Switch, CAkRegisteredObj * in_GameObj = NULL );

	virtual AKRESULT ModifyActiveState( AkUInt32 in_stateID, bool in_bSupported );
	virtual AKRESULT PrepareData();
	virtual void UnPrepareData();

	CAkSwitchPackage * AddSwitch( AkSwitchStateID in_Switch );

	void RemoveSwitch( AkSwitchStateID in_Switch );

    void SetDefaultSwitch( AkUInt32 in_Switch ) { m_ulDefaultSwitch = in_Switch; }

    AKRESULT SetSwitchGroup( 
        AkUInt32 in_ulGroup, 
        AkGroupType in_eGroupType 
        );

	AKRESULT AddNodeInSwitch(
		AkUInt32			in_Switch,
		AkUniqueID		in_NodeID
		);

	AKRESULT AddNodeInSwitch(
		CAkSwitchPackage * in_pPack,
		AkUniqueID		in_NodeID
		);

	AKRESULT RemoveNodeFromSwitch(
		AkUInt32			in_Switch,
		AkUniqueID		in_NodeID
		);

	void ClearSwitches();

	AKRESULT SetContinuousValidation( bool in_bIsContinuousCheck );

	AKRESULT SetContinuePlayback( AkUniqueID in_NodeID, bool in_bContinuePlayback );
	AKRESULT SetFadeInTime( AkUniqueID in_NodeID, AkTimeMs in_time );
	AKRESULT SetFadeOutTime( AkUniqueID in_NodeID, AkTimeMs in_time );
	AKRESULT SetOnSwitchMode( AkUniqueID in_NodeID, AkOnSwitchMode in_bSwitchMode );
	AKRESULT SetIsFirstOnly( AkUniqueID in_NodeID, bool in_bIsFirstOnly );

	AKRESULT SetAllParams( AkUniqueID in_NodeID, AkSwitchNodeParams& in_rParams );

	AKRESULT SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize );

protected:

	// Constructors
    CAkSwitchCntr( AkUniqueID in_ulID );

	// Destructor
    virtual ~CAkSwitchCntr( void );

	// AkMultiPlayNode interface implementation:
	virtual bool IsContinuousPlayback();

private:
	// Helpers

	AKRESULT PerformSwitchChange( AkSwitchStateID in_SwitchTo, CAkRegisteredObj * in_GameObj = NULL );
	AKRESULT PerformSwitchChangeContPerObject( AkSwitchStateID in_SwitchTo, CAkRegisteredObj * in_GameObj );

	bool IsAContinuousSwitch( CAkSwitchPackage* in_pSwitchPack, AkUniqueID in_ID );

	AKRESULT	PrepareNodeList( const CAkSwitchPackage::AkIDList& in_rNodeList );
	void		UnPrepareNodeList( const CAkSwitchPackage::AkIDList& in_rNodeList );

	bool			GetContinuePlayback( AkUniqueID in_NodeID );
	AkTimeMs		GetFadeInTime( AkUniqueID in_NodeID );
	AkTimeMs		GetFadeOutTime( AkUniqueID in_NodeID );
	AkOnSwitchMode	GetOnSwitchMode( AkUniqueID in_NodeID );
	bool			GetIsFirstOnly( AkUniqueID in_NodeID );

	void			GetAllParams( AkUniqueID in_NodeID, AkSwitchNodeParams& out_rParams );	

	struct SwitchContPlaybackItem
	{
		UserParams				UserParameters;
		CAkRegisteredObj *		GameObject;
		PlayHistory				PlayHist;
		AkPlaybackState			ePlaybackState;
	};

	AKRESULT PlayOnSwitch( AkUniqueID in_ID, SwitchContPlaybackItem& in_rContItem );
	AKRESULT StopOnSwitch( AkUniqueID in_ID, AkSwitchNodeParams& in_rSwitchNodeParams, CAkRegisteredObj * in_GameObj );
	AKRESULT StopPrevious( CAkSwitchPackage* in_pPreviousSwitchPack, CAkSwitchPackage* in_pNextSwitchPack, CAkRegisteredObj * in_GameObj );

	void CleanSwitchContPlayback();

	void StopContSwitchInst( CAkRegisteredObj * in_pGameObj = NULL, AkPlayingID in_PlayingID = AK_INVALID_PLAYING_ID );
	void PauseContSwitchInst( CAkRegisteredObj * in_pGameObj = NULL, AkPlayingID in_PlayingID = AK_INVALID_PLAYING_ID );
	void ResumeContSwitchInst( CAkRegisteredObj * in_pGameObj = NULL, AkPlayingID in_PlayingID = AK_INVALID_PLAYING_ID );

	void NotifyEndContinuous( SwitchContPlaybackItem& in_rSwitchContPlayback );
	void NotifyPausedContinuous( SwitchContPlaybackItem& in_rSwitchContPlayback );
	void NotifyResumedContinuous( SwitchContPlaybackItem& in_rSwitchContPlayback );
	void NotifyPausedContinuousAborted( SwitchContPlaybackItem& in_rSwitchContPlayback );
	
private:
	
    AkGroupType m_eGroupType;				// Is it binded to state or to switches
	AkUInt32	m_ulGroupID;				// May either be a state group or a switch group
	AkUInt32	m_ulDefaultSwitch;			// Default value, to be used if none is available, 

	typedef CAkKeyList< AkUInt32, CAkSwitchPackage, AkAllocAndKeep > AkSwitchList;
	AkSwitchList m_SwitchList;

	CAkKeyList< AkUniqueID, AkSwitchNodeParams, AkAllocAndKeep > m_listParameters;

	typedef CAkList2< SwitchContPlaybackItem, const SwitchContPlaybackItem&, AkAllocAndKeep > AkListSwitchContPlayback;
	AkListSwitchContPlayback m_listSwitchContPlayback;
};
#endif //_SWITCH_CNTR_H_
