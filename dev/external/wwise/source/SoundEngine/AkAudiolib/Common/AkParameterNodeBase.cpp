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
// AkParameterNodeBase.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#define AK_DEFINE_AKPROPDEFAULT

#include "AkParameterNodeBase.h"
#include "AkTransitionManager.h"
#include "AkStateMgr.h"
#include "AkAudioLibIndex.h"
#include "AkEnvironmentsMgr.h"
#include "AkSIS.h"
#include "AkMonitor.h"
#include "AkRTPCMgr.h"
#include "AkBitArray.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include "AkGen3DParams.h"
#include "AkAudioLib.h"
#include "AkBanks.h"
#include "AkFxBase.h"
#include "AkFXMemAlloc.h"
#include "AkEffectsMgr.h"
#include "AkRegisteredObj.h"
#include "AkParameterNode.h"
#include "AkBus.h"
#include "AkURenderer.h"
#ifdef AK_MOTION
#include "AkFeedbackBus.h"
#endif // AK_MOTION
#include "AkBankMgr.h"
#include "ActivityChunk.h"

// -------------------------------------------------------------
// Globals
// -------------------------------------------------------------
#ifndef AK_OPTIMIZED
AkUInt32 CAkParameterNodeBase::g_uSoloCount = 0;
AkUInt32 CAkParameterNodeBase::g_uMuteCount = 0;
AkUInt32 CAkParameterNodeBase::g_uSoloCount_bus = 0;
AkUInt32 CAkParameterNodeBase::g_uMuteCount_bus = 0;
bool CAkParameterNodeBase::g_bIsMonitoringMuteSoloDirty = false;
#endif

///////////////////////////////////////////////////////////////////
// CAkLimiter
///////////////////////////////////////////////////////////////////

void CAkLimiter::Term()
{
	AKASSERT( m_sortedPBIList.IsEmpty() );
	m_sortedPBIList.Term();
}

AKRESULT CAkLimiter::Add( CAkPBI* in_pPBI, AKVoiceLimiterType in_Type )
{
	// Must call AddNoSetKey to avoir the key to be changed now.
	// Multiple limitors would all try to update the same key otherwise.
	bool bAddInSystemChecker = m_sortedPBIList.IsEmpty();
	CAkPBI** ppPBI = NULL;
	if( DoesKillNewest() )
	{
		ppPBI = m_sortedPBIList.AddNoSetKey( *reinterpret_cast<AkPriorityStruct_INC*>(&in_pPBI->GetPriorityKey()) );
	}
	else
	{
		ppPBI = m_sortedPBIList.AddNoSetKey( *reinterpret_cast<AkPriorityStruct_DEC*>(&in_pPBI->GetPriorityKey()) );
	}
	if( ppPBI )
	{
		if( bAddInSystemChecker )
		{
			// Remove the Limiter to the list to be checked at each frame.
			switch( in_Type )
			{
			case AKVoiceLimiter_AM:
				CAkURenderer::AddAMLimiter( this );
				break;

			case AKVoiceLimiter_Bus:
				CAkURenderer::AddBusLimiter( this );
				break;

			case AKVoiceLimiter_Global:
				//Not in a list, nothing to do.
				break;
			}
		}
		*ppPBI = in_pPBI;
		return AK_Success;
	}
	return AK_Fail;
}

void CAkLimiter::Remove( CAkPBI* in_pPBI, AKVoiceLimiterType in_Type )
{
	if( DoesKillNewest() )
	{
		m_sortedPBIList.Unset( *reinterpret_cast<AkPriorityStruct_INC*>(&in_pPBI->GetPriorityKey()) );
	}
	else
	{
		m_sortedPBIList.Unset( *reinterpret_cast<AkPriorityStruct_DEC*>(&in_pPBI->GetPriorityKey()) );
	}
	if( m_sortedPBIList.IsEmpty() )
	{
		// Remove the Limiter to the list to be checked at each frame.
		switch( in_Type )
		{
		case AKVoiceLimiter_AM:
			CAkURenderer::RemoveAMLimiter( this );
			break;

		case AKVoiceLimiter_Bus:
			CAkURenderer::RemoveBusLimiter( this );
			break;

		case AKVoiceLimiter_Global:
			//Not in a list, nothing to do.
			break;
		}
	}
}

void CAkLimiter::Update( AkReal32 in_NewPriority, CAkPBI* in_pPBI )
{
	if( DoesKillNewest() )
	{
		// Using IncreasingOrder
		AkPriorityStruct_INC newKey;
		newKey.priority = in_NewPriority;
		newKey.PBIID = in_pPBI->GetPriorityKey().PBIID;

		m_sortedPBIList.Reorder( 
			*reinterpret_cast<AkPriorityStruct_INC*>(&in_pPBI->GetPriorityKey()), 
			newKey, 
			in_pPBI );
	}
	else
	{
		AkPriorityStruct_DEC newKey;
		newKey.priority = in_NewPriority;
		newKey.PBIID = in_pPBI->GetPriorityKey().PBIID;

		m_sortedPBIList.Reorder( 
			*reinterpret_cast<AkPriorityStruct_DEC*>(&in_pPBI->GetPriorityKey()), 
			newKey, 
			in_pPBI );
	}
}

void CAkLimiter::UpdateFlags()
{
	AkUInt16 u16Max = GetMaxInstances();

	if( u16Max != 0 && m_sortedPBIList.Length() > u16Max )
	{
		AkUInt32 uAccepted = 0;
		AkSortedPBIPriorityList::Iterator it = m_sortedPBIList.Begin();
		AkSortedPBIPriorityList::Iterator itEnd = m_sortedPBIList.End();
		while( it != itEnd && uAccepted < u16Max )
		{
			CAkPBI* pPBI = *it;
			if( !pPBI->IsVirtual() || pPBI->WasForcedVirtualized() )
			{
				if( !pPBI->WasKicked() && !pPBI->IsForcedVirtualized() )// don't count if it was already kicked
					++uAccepted;
			}
			++it;
		}

		// The remaining are not allowed to play.
		while( it != itEnd )
		{
			CAkPBI* pPBI = *it;
			if( !pPBI->WasKicked() )// No use to virtualize a sound that was kicked: let it die.
			{
				KickFrom eReason = KickFrom_OverNodeLimit;
				if( this == &CAkURenderer::GetGlobalLimiter() )
					eReason = KickFrom_OverGlobalLimit;

				if( m_bAllowUseVirtualBehavior )
				{
					pPBI->ForceVirtualize( eReason );
				}
				else
				{
					pPBI->Kick( eReason );
				}
			}

			++it;
		}
	}
}

void CAkLimiter::SwapOrdering()
{
	m_bDoesKillNewest = !m_bDoesKillNewest;
	if( DoesKillNewest() )
	{
		m_sortedPBIList.ReSortArray<AkPriorityStruct_INC>();
	}
	else
	{
		m_sortedPBIList.ReSortArray<AkPriorityStruct_DEC>();
	}
}

#if defined(_DEBUG)
bool CAkLimiter::LookForCtx( CAkPBI* in_pCtx )
{
	AkSortedPBIPriorityList::Iterator it = m_sortedPBIList.Begin();
	for( ; it != m_sortedPBIList.End(); ++it )
	{
		if( *it == in_pCtx )
			return true;
	}
	return false;
}
#endif

///////////////////////////////////////////////////////////////////
// CAkParameterNodeBase::FXChunk
///////////////////////////////////////////////////////////////////

CAkParameterNodeBase::FXChunk::FXChunk()
{
	for ( AkUInt32 i = 0; i < AK_NUM_EFFECTS_PER_OBJ; ++i )
	{
		aFX[i].bRendered = false;
		aFX[i].bShareSet = false;
		aFX[i].id = AK_INVALID_UNIQUE_ID;
	}

	bitsMainFXBypass = 0;
}

CAkParameterNodeBase::FXChunk::~FXChunk()
{
}
		
///////////////////////////////////////////////////////////////////
// CAkParameterNodeBase
///////////////////////////////////////////////////////////////////

CAkParameterNodeBase::CAkParameterNodeBase(AkUniqueID in_ulID)
:CAkPBIAware( in_ulID )
,m_pGlobalSIS(NULL)
,m_pFXChunk( NULL )
,m_pActivityChunk( NULL )
,m_pParentNode(NULL)
,m_pBusOutputNode(NULL)
,m_u16MaxNumInstance( 0 ) //0 means no max
,m_bKillNewest( false )
,m_bUseVirtualBehavior( false )
,m_bIsVVoicesOptOverrideParent( false )
,m_bIsMaxNumInstOverrideParent( false )
,m_bIsGlobalLimit( true )
,m_bPriorityApplyDistFactor(false)
,m_bPriorityOverrideParent(false)
,m_bUseState( true )
,m_bIsInDestructor( false )
,m_bIsBusCategory( false )
,m_bPositioningEnabled( false )
,m_bIsHdrBus( false )
,m_bPositioningInfoOverrideParent( false )
,m_bPositioningEnablePanner( false )
,m_bIsFXOverrideParent( false )
,m_bIsContinuousValidation( false )
#ifndef AK_OPTIMIZED
,m_bIsSoloed( false )
,m_bIsMuted( false )
#endif
#ifdef AK_MOTION
,m_pFeedbackInfo( NULL )
#endif // AK_MOTION
{

}

CAkParameterNodeBase::~CAkParameterNodeBase()
{
	m_bIsInDestructor = true;

	if ( m_pFXChunk )
	{
		AkDelete( g_DefaultPoolId, m_pFXChunk );
	}

	AKASSERT( g_pRTPCMgr );

	for ( AkUInt32 iBit = 0; !m_RTPCBitArray.IsEmpty(); ++iBit )
	{
		if ( m_RTPCBitArray.IsSet( iBit ) )
		{
			g_pRTPCMgr->UnSubscribeRTPC( this, iBit );
			m_RTPCBitArray.UnsetBit( iBit );
		}
	}

	if(m_pGlobalSIS)
	{
		AkDelete(g_DefaultPoolId,m_pGlobalSIS);
	}

	if( !m_states.IsEmpty() )
	{
		FlushStateTransitions();
		RemoveStateGroups();
	}
	m_states.Term();

#ifdef AK_MOTION
	if (m_pFeedbackInfo != NULL)
	{
		if (m_pFeedbackInfo->m_pFeedbackBus != NULL)	//Will be null for the master bus
			m_pFeedbackInfo->m_pFeedbackBus->RemoveChild(this);
		AkDelete( g_DefaultPoolId, m_pFeedbackInfo );
	}
#endif // AK_MOTION

	AKASSERT( !IsActivityChunkEnabled() ); //Should have been freed by now...
	if( IsActivityChunkEnabled() )
	{
		DeleteActivityChunk();// But just in case, we will delete it anyway...
	}
}

void CAkParameterNodeBase::AddToIndex()
{
	AKASSERT( g_pIndex );
	g_pIndex->GetNodeIndex( IsBusCategory() ? AkNodeType_Bus : AkNodeType_Default ).SetIDToPtr(this);
}

void CAkParameterNodeBase::RemoveFromIndex()
{
	AKASSERT(g_pIndex);
	g_pIndex->GetNodeIndex( IsBusCategory() ? AkNodeType_Bus : AkNodeType_Default ).RemoveID(ID());
}

AkUInt32 CAkParameterNodeBase::AddRef() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->GetNodeLock( IsBusCategory() ? AkNodeType_Bus : AkNodeType_Default ) ); 
    return ++m_lRef; 
} 

AkUInt32 CAkParameterNodeBase::Release() 
{ 
    AkAutoLock<CAkLock> IndexLock( g_pIndex->GetNodeLock( IsBusCategory() ? AkNodeType_Bus : AkNodeType_Default ) ); 
    AkInt32 lRef = --m_lRef; 
    AKASSERT( lRef >= 0 ); 
    if ( !lRef ) 
    { 
        RemoveFromIndex();
		OnPreRelease();

		if(m_pParentNode != NULL)
		{
			m_pParentNode->RemoveChild(this);
		}

		if(m_pBusOutputNode != NULL)
		{
			m_pBusOutputNode->RemoveChild(this);
		}

        AkDelete( g_DefaultPoolId, this ); 
    } 
    return lRef; 
}

void CAkParameterNodeBase::Parent(CAkParameterNodeBase* in_pParent)
{
	m_pParentNode = in_pParent;
}

void CAkParameterNodeBase::ParentBus(CAkParameterNodeBase* in_pParent)
{
	m_pBusOutputNode = in_pParent;
}



void CAkParameterNodeBase::Unregister(CAkRegisteredObj *)
{
}

AKRESULT CAkParameterNodeBase::AddChildInternal(
        CAkParameterNodeBase* /*in_pChild*/
		)
{
	AKASSERT(!"Addchild/removechild not defined for this node type");
	return AK_NotImplemented;
}

void CAkParameterNodeBase::RemoveChild(
        CAkParameterNodeBase* /*in_pChild*/
		)
{
	AKASSERT(!"Addchild/removechild not defined for this node type");
}

void CAkParameterNodeBase::GetChildren( AkUInt32&, AkObjectInfo*, AkUInt32&, AkUInt32 )
{
	//no children by default. Function will be overridden in AkParentNode.h
}

AKRESULT CAkParameterNodeBase::Stop( 
	CAkRegisteredObj * in_pGameObj, 
	AkPlayingID in_PlayingID /* = AK_INVALID_PLAYING_ID */, 
	AkTimeMs in_uTransitionDuration /*= 0*/,
	AkCurveInterpolation in_eFadeCurve /*= AkCurveInterpolation_Linear*/)
{
	ActionParams l_Params;
	l_Params.bIsFromBus = false;
	l_Params.bIsMasterResume = false;
	l_Params.transParams.eFadeCurve = in_eFadeCurve;
	l_Params.eType = ActionParamType_Stop;
	l_Params.pGameObj = in_pGameObj;
	l_Params.playingID = in_PlayingID;
	l_Params.transParams.TransitionTime = in_uTransitionDuration;
	l_Params.bIsMasterCall = false;
	return ExecuteAction( l_Params );
}

AKRESULT CAkParameterNodeBase::Pause( 
	CAkRegisteredObj * in_pGameObj, 
	AkPlayingID in_PlayingID /* = AK_INVALID_PLAYING_ID */,
	AkTimeMs in_uTransitionDuration /*= 0*/,
	AkCurveInterpolation in_eFadeCurve /*= AkCurveInterpolation_Linear*/ )
{
	ActionParams l_Params;
	l_Params.bIsFromBus = false;
	l_Params.bIsMasterResume = false;
	l_Params.transParams.eFadeCurve = in_eFadeCurve;
	l_Params.eType = ActionParamType_Pause;
	l_Params.pGameObj = in_pGameObj;
	l_Params.playingID = in_PlayingID;
	l_Params.transParams.TransitionTime = in_uTransitionDuration;
	l_Params.bIsMasterCall = false;
	return ExecuteAction( l_Params );
}

AKRESULT CAkParameterNodeBase::Resume( 
	CAkRegisteredObj * in_pGameObj, 
	AkPlayingID in_PlayingID /* = AK_INVALID_PLAYING_ID */,
	AkTimeMs in_uTransitionDuration /*= 0*/,
	AkCurveInterpolation in_eFadeCurve /*= AkCurveInterpolation_Linear*/ )
{
	ActionParams l_Params;
	l_Params.bIsFromBus = false;
	l_Params.bIsMasterResume = false;
	l_Params.transParams.eFadeCurve = in_eFadeCurve;
	l_Params.eType = ActionParamType_Resume;
	l_Params.pGameObj = in_pGameObj;
	l_Params.playingID = in_PlayingID;
	l_Params.transParams.TransitionTime = in_uTransitionDuration;
	l_Params.bIsMasterCall = false;
	return ExecuteAction( l_Params );
}

void CAkParameterNodeBase::Notification(
		AkRTPC_ParameterID in_ParamID, 
		AkReal32 in_fValue,				// Param variation
		CAkRegisteredObj * in_pGameObj,	// Target Game Object
		void* in_pExceptArray
		)
{
	if( ParamMustNotify( in_ParamID ) )
	{
		NotifParams l_Params;
		l_Params.eType = in_ParamID;
		l_Params.bIsFromBus = false;
		l_Params.pGameObj = in_pGameObj;
		l_Params.fValue = in_fValue;
		l_Params.pExceptObjects = in_pExceptArray;
		ParamNotification( l_Params );
	}
}

void CAkParameterNodeBase::PriorityNotification( AkReal32 in_Priority, CAkRegisteredObj * in_pGameObj, void* in_pExceptArray )
{
	NotifParams l_Params;
	l_Params.eType = RTPC_Priority;
	l_Params.bIsFromBus = false;
	l_Params.pGameObj = in_pGameObj;
	l_Params.fValue = in_Priority;
	l_Params.pExceptObjects = in_pExceptArray;
	PriorityNotification( l_Params );
}

//This function is virtual and will not be called on parent nodes.
void CAkParameterNodeBase::PriorityNotification( NotifParams& in_rParams )
{
	ParamNotification( in_rParams );
}

bool CAkParameterNodeBase::IsException( CAkParameterNodeBase* in_pNode, ExceptionList& in_rExceptionList )
{
	WwiseObjectID wwiseId( in_pNode->ID(), in_pNode->IsBusCategory() );
	return ( in_rExceptionList.Exists( wwiseId ) != NULL );
}

CAkBus* CAkParameterNodeBase::GetMixingBus()
{
	if(m_pBusOutputNode)
	{
		return m_pBusOutputNode->GetMixingBus();
	}
	else
	{
		if(m_pParentNode)
		{
			return m_pParentNode->GetMixingBus();
		}
		else
		{
			return NULL;//No mixing BUS found, should go directly to the master output.
		}
	}
}

CAkBus* CAkParameterNodeBase::GetLimitingBus()
{
	if( m_pBusOutputNode )
	{
		return m_pBusOutputNode->GetLimitingBus();
	}
	else
	{
		if( m_pParentNode )
		{
			return m_pParentNode->GetLimitingBus();
		}
		else
		{
			return NULL;//No Limiting BUS found, should go directly to the master output.
		}
	}
}

CAkBus* CAkParameterNodeBase::GetControlBus()
{
	if( m_pBusOutputNode )
	{
		return (CAkBus*)m_pBusOutputNode;
	}
	else
	{
		if( m_pParentNode )
		{
			return m_pParentNode->GetControlBus();
		}
		else
		{
			return NULL;
		}
	}
}

AKRESULT CAkParameterNodeBase::PrepareNodeData( AkUniqueID in_NodeID )
{
	AKRESULT eResult = AK_Fail;
	CAkParameterNodeBase* pNode = g_pIndex->GetNodePtrAndAddRef( in_NodeID, AkNodeType_Default );

	if( pNode )
	{
		eResult = pNode->PrepareData();
		if(eResult != AK_Success)
		{
			pNode->Release();
		}
		//pNode->Release();// must keep the AudionodeAlive as long as it is prepared, an the node is required to unprepare the data.
	}

	return eResult;
}

void CAkParameterNodeBase::UnPrepareNodeData( AkUniqueID in_NodeID )
{
	CAkParameterNodeBase* pNode = g_pIndex->GetNodePtrAndAddRef( in_NodeID, AkNodeType_Default );

	if( pNode )
	{
		pNode->UnPrepareData();
		pNode->Release();
		pNode->Release();// This is to compensate for the release that have not been done in PrepareData()
	}
}

void CAkParameterNodeBase::DeleteActivityChunk()
{
	if( m_pActivityChunk )// Because adding it while allocation failed would result in it never being added
	{
		if( m_pParentNode )
			m_pParentNode->UnsetFastActive( this );

		if( m_pBusOutputNode )
			m_pBusOutputNode->UnsetFastActive( this );

#ifdef AK_MOTION
		if( m_pFeedbackInfo != NULL && m_pFeedbackInfo->m_pFeedbackBus != NULL )
			m_pFeedbackInfo->m_pFeedbackBus->UnsetFastActive( this );
#endif
	}

	AkDelete( g_DefaultPoolId, m_pActivityChunk );
	m_pActivityChunk = NULL;
}

void CAkParameterNodeBase::FlushStateTransitions()
{
	for( StateList::Iterator iter = m_states.Begin(); iter != m_states.End(); ++iter )
		(*iter)->FlushStateTransitions();
}

bool CAkParameterNodeBase::OnNewActivityChunk( AkUInt16 in_flagForwardToBus )
{
	bool bRet = true;

	if( (in_flagForwardToBus & AK_ForwardToBusType_Normal) && m_pBusOutputNode ) 
	{
		if( !m_pBusOutputNode->SetFastActive( this ))
		{
			bRet = false;
		}
		in_flagForwardToBus &= ~AK_ForwardToBusType_Normal;
	}

#ifdef AK_MOTION
	if( (in_flagForwardToBus & AK_ForwardToBusType_Motion) && m_pFeedbackInfo != NULL)
	{
		if( m_pFeedbackInfo->m_pFeedbackBus != NULL )
		{
			if( !m_pFeedbackInfo->m_pFeedbackBus->SetFastActive( this ) )
			{
				bRet = false;
			}
			in_flagForwardToBus &= ~AK_ForwardToBusType_Motion;
		}
	}
#endif

	if( m_pParentNode && !m_pParentNode->SetFastActive( this, in_flagForwardToBus ) )
		bRet = false;

	return bRet;
}

bool CAkParameterNodeBase::SetFastActive( CAkParameterNodeBase* in_pChild, AkUInt16 in_flagForwardToBus )
{
	bool bRet = EnableActivityChunk( in_flagForwardToBus );
	if( IsActivityChunkEnabled() )
	{
		return m_pActivityChunk->SetFastActive( in_pChild );
	}
	return bRet;
}

void CAkParameterNodeBase::UnsetFastActive( CAkParameterNodeBase* in_pChild )
{
	if( IsActivityChunkEnabled() )
	{
		m_pActivityChunk->UnsetFastActive( in_pChild );
	}
}

//====================================================================================================
// figure out what we have to pause / resume
//====================================================================================================
void CAkParameterNodeBase::PauseTransitions(bool in_bPause)
{
	// should we pause ?
	for( StateList::Iterator iter = m_states.Begin(); iter != m_states.End(); ++iter )
	{
		AkStateGroupChunk* pStateGroupChunk = *iter;

		for ( AkStateValues::Iterator it = pStateGroupChunk->m_values.Begin(), itEnd = pStateGroupChunk->m_values.End(); it != itEnd; ++it )
		{
			if ( it.pValue->pTransition )
			{
				if( in_bPause )
					g_pTransitionManager->Pause( it.pValue->pTransition );
				else
					g_pTransitionManager->Resume( it.pValue->pTransition );
			}
		}
	}
}

static AkForceInline AkSISValue * _GetSISValue( CAkSIS * in_pSIS, AkPropID in_ePropID, AkReal32 in_fDefault = 0.0f )
{
	AkSISValue * pSISValue = in_pSIS->m_values.FindProp( in_ePropID );
	if ( !pSISValue )
	{
		pSISValue = in_pSIS->m_values.AddAkProp( in_ePropID );
		if ( !pSISValue )
			return NULL;

		pSISValue->fValue = in_fDefault;
		pSISValue->pTransition = NULL;
	}

	return pSISValue;
}

void CAkParameterNodeBase::StartSISTransition(
		CAkSIS * in_pSIS,
		AkPropID in_ePropID,
		AkReal32 in_fTargetValue,
		AkValueMeaning in_eValueMeaning,
		AkCurveInterpolation in_eFadeCurve,
		AkTimeMs in_lTransitionTime )
{
	AKASSERT( in_eValueMeaning != AkValueMeaning_Default || in_fTargetValue == 0.0f );

	AkSISValue * pSISValue = _GetSISValue( in_pSIS, in_ePropID );
	if ( !pSISValue )
		return;
	
	// reuse existing transition ?
	if( pSISValue->pTransition != NULL )
	{
		AkReal32 fNewTarget = in_fTargetValue;
		if( in_eValueMeaning == AkValueMeaning_Independent )
		{
			fNewTarget -= m_props.GetAkProp( in_ePropID, g_AkPropDefault[ in_ePropID ] ).fValue;
		}

		g_pTransitionManager->ChangeParameter(
			pSISValue->pTransition,
			in_ePropID,
			fNewTarget,
			in_lTransitionTime,
			in_eFadeCurve,
			in_eValueMeaning );
	}
	else // new transition
	{
		AkReal32 fStartValue = pSISValue->fValue;
		AkReal32 fTargetValue = 0.0f;
		switch(in_eValueMeaning)
		{
		case AkValueMeaning_Independent:
			fTargetValue = in_fTargetValue - m_props.GetAkProp( in_ePropID, g_AkPropDefault[ in_ePropID ] ).fValue;
			break;
		case AkValueMeaning_Offset:
			fTargetValue = pSISValue->fValue + in_fTargetValue;
			break;
		case AkValueMeaning_Default:
			break;
		default:
			AKASSERT(!"Invalid Meaning type");
			break;
		}

		// do we really need to start a transition ?
		if((fStartValue == fTargetValue)
			|| (in_lTransitionTime == 0))
		{
			// no need to
			AkReal32 fNotifValue = pSISValue->fValue;
			pSISValue->fValue = fTargetValue;

			Notification( g_AkPropRTPCID[ in_ePropID ], pSISValue->fValue - fNotifValue, in_pSIS->m_pGameObj );
		}
		// start it
		else
		{
			TransitionParameters trans(
				in_pSIS,
				in_ePropID,
				fStartValue,
				fTargetValue,
				in_lTransitionTime,
				in_eFadeCurve,
				g_AkPropDecibel[ in_ePropID ],
				true );

			pSISValue->pTransition = g_pTransitionManager->AddTransitionToList( trans );
		}
	}
}

void CAkParameterNodeBase::StartSisMuteTransitions(CAkSIS*	in_pSIS,
													AkReal32	in_fTargetValue,
													AkCurveInterpolation	in_eFadeCurve,
													AkTimeMs	in_lTransitionTime)
{
	AKASSERT( in_pSIS );

	AkSISValue * pSISValue = _GetSISValue( in_pSIS, AkPropID_MuteRatio, AK_UNMUTED_RATIO );
	if ( !pSISValue )
		return;

	// reuse existing transition ?
	if( pSISValue->pTransition != NULL )
	{
		g_pTransitionManager->ChangeParameter(
			pSISValue->pTransition,
			AkPropID_MuteRatio,
			in_fTargetValue,
			in_lTransitionTime,
			in_eFadeCurve,
			AkValueMeaning_Default );
	}
	else // new transition
	{
		if( in_lTransitionTime != 0 )
		{
			TransitionParameters MuteParams(
				in_pSIS,
				AkPropID_MuteRatio,
				pSISValue->fValue,
				in_fTargetValue,
				in_lTransitionTime,
				in_eFadeCurve,
				g_AkPropDecibel[ AkPropID_MuteRatio ],
				true );
			pSISValue->pTransition = g_pTransitionManager->AddTransitionToList(MuteParams);
		}
		else
		{
			//Apply it directly, so there will be no delay, avoiding an annoying glitch that may apears in the worst cases.
			in_pSIS->TransUpdateValue( AkPropID_MuteRatio, in_fTargetValue, true );
		}
	}
}

//Adds one effect. Maintain in sync with RemoveFX and UpdateEffects
AKRESULT CAkParameterNodeBase::SetFX( 
	AkUInt32 in_uFXIndex,				// Position of the FX in the array
	AkUniqueID in_uID,					// Unique id of CAkFxShareSet or CAkFxCustom
	bool in_bShareSet					// Shared?
	)
{
	AKASSERT( in_uFXIndex < AK_NUM_EFFECTS_PER_OBJ );
	if( in_uFXIndex >= AK_NUM_EFFECTS_PER_OBJ )
	{
		return AK_InvalidParameter;
	}

	if ( !m_pFXChunk )
	{
		AkNew2( m_pFXChunk, g_DefaultPoolId, FXChunk, FXChunk() );
		if ( !m_pFXChunk )
			return AK_InsufficientMemory;
	}

	FXStruct & fx = m_pFXChunk->aFX[ in_uFXIndex ];

#ifndef AK_OPTIMIZED
	if ( fx.bRendered ) // Do not set an effect if effect has been rendered.
	{
		return AK_RenderedFX;
	}
#endif

	if ( fx.bShareSet != in_bShareSet 
		|| fx.id != in_uID )
	{
		fx.bShareSet = in_bShareSet;
		fx.id = in_uID;

		//Required only so that the volume gets calculated correctly after/before the BUS FX
		RecalcNotification();
		UpdateFx( in_uFXIndex );
	}

	return AK_Success;
}

//Removes one effect. Maintain in sync with SetFX and UpdateEffects
AKRESULT CAkParameterNodeBase::RemoveFX( 
	AkUInt32 in_uFXIndex					// Position of the FX in the array
	)
{
	// Check parameters.
	if( in_uFXIndex >= AK_NUM_EFFECTS_PER_OBJ )
	{
		return AK_InvalidParameter;
	}

	if ( !m_pFXChunk )
		return AK_Fail;

	FXStruct & fx = m_pFXChunk->aFX[ in_uFXIndex ];

	if ( fx.bShareSet
		|| fx.id != AK_INVALID_UNIQUE_ID )
	{
		fx.bShareSet = false;
		fx.id = AK_INVALID_UNIQUE_ID;

		//Required only so that the volume gets calculated correctly after/before the BUS FX
		RecalcNotification();
		UpdateFx( in_uFXIndex );
	}

	return AK_Success;
}

//Updates all effects in one operation.  Maintain in sync with SetFX and RemoveFX above.
AKRESULT CAkParameterNodeBase::UpdateEffects( AkUInt32 in_uCount, AkEffectUpdate* in_pUpdates )
{
	if ( !m_pFXChunk )
	{
		AkNew2( m_pFXChunk, g_DefaultPoolId, FXChunk, FXChunk() );
		if ( !m_pFXChunk )
			return AK_InsufficientMemory;
	}

	bool bRecalc = false;
	bool bUpdates[AK_NUM_EFFECTS_PER_OBJ];
	for(AkUInt32 i = 0; i < AK_NUM_EFFECTS_PER_OBJ; i++)
		bUpdates[i] = false;

	if (in_uCount == 0)
	{
		for(AkUInt32 i = 0; i < AK_NUM_EFFECTS_PER_OBJ; i++)
		{
			bUpdates[i] = m_pFXChunk->aFX[i].id != 0;
			bRecalc |= bUpdates[i];
			m_pFXChunk->aFX[i].id = 0;
		}
	}
	else
	{
		for(AkUInt32 uIndex = 0; uIndex < AK_NUM_EFFECTS_PER_OBJ; uIndex++)
		{
			FXStruct & fx = m_pFXChunk->aFX[uIndex];

			AkUInt32 i = 0;
			for(; i < in_uCount && in_pUpdates[i].uiIndex != uIndex; i++) 
				/*empty*/;

			if (i == in_uCount)
			{
				//Not found, effect deleted or not present.
				if (fx.id != AK_INVALID_UNIQUE_ID)
				{
					fx.bShareSet = false;
					fx.id = AK_INVALID_UNIQUE_ID;
					bUpdates[uIndex] = true;
					bRecalc = true;
				}
				continue;
			}
			
			AkEffectUpdate* pEffect = in_pUpdates + i;

			if ( fx.bShareSet != pEffect->bShared || fx.id != pEffect->ulEffectID )
			{
				fx.bShareSet = pEffect->bShared;
				fx.id = pEffect->ulEffectID;
				bUpdates[uIndex] = true;
				bRecalc = true;
			}
			else if ( pEffect->ulEffectID == AK_INVALID_UNIQUE_ID && (fx.bShareSet || fx.id != AK_INVALID_UNIQUE_ID) )
			{
				fx.bShareSet = false;
				fx.id = AK_INVALID_UNIQUE_ID;
				bUpdates[uIndex] = true;
				bRecalc = true;
			}
		}
	}

	//Required only so that the volume gets calculated correctly after/before the BUS FX
	if (bRecalc)
	{
		RecalcNotification();

		for(AkUInt32 i = 0; i < AK_NUM_EFFECTS_PER_OBJ; i++)
		{
			if (bUpdates[i])
				UpdateFx(i);
		}
	}
	return AK_Success;
}


AKRESULT CAkParameterNodeBase::RenderedFX( AkUInt32 in_uFXIndex, bool in_bRendered )
{
	AKASSERT( in_uFXIndex < AK_NUM_EFFECTS_PER_OBJ );

	if ( !m_pFXChunk )
	{
		if ( in_bRendered )
		{
			AkNew2( m_pFXChunk, g_DefaultPoolId, FXChunk, FXChunk() );
			if ( !m_pFXChunk )
				return AK_InsufficientMemory;
		}
		else
		{
			return AK_Success;
		}
	}

	m_pFXChunk->aFX[ in_uFXIndex ].bRendered = in_bRendered;

	if( in_bRendered && m_pFXChunk->aFX[ in_uFXIndex ].id != AK_INVALID_UNIQUE_ID )
	{
		MONITOR_ERRORMSG( AKTEXT("Warning: Bank contains rendered source effects which can't be edited in Wwise") );
		RemoveFX( in_uFXIndex );
	}

	return AK_Success;
}

AKRESULT CAkParameterNodeBase::MainBypassFX( 
	AkUInt32 in_bitsFXBypass,
	AkUInt32 in_uTargetMask /* = 0xFFFFFFFF */ )
{
	if( NodeCategory() == AkNodeCategory_Bus || NodeCategory() == AkNodeCategory_AuxBus)
	{
		MONITOR_BUSNOTIFICATION(
			ID(), 
			AkMonitorData::BusNotification_FXBypass,
			in_bitsFXBypass, in_uTargetMask	);
	}

	if ( !m_pFXChunk )
	{
		if ( in_bitsFXBypass )
		{
			AkNew2( m_pFXChunk, g_DefaultPoolId, FXChunk, FXChunk() );
			if ( !m_pFXChunk )
				return AK_InsufficientMemory;
		}
		else
		{
			return AK_Success;
		}
	}

	m_pFXChunk->bitsMainFXBypass = (AkUInt8) ( ( m_pFXChunk->bitsMainFXBypass & ~in_uTargetMask ) | (in_bitsFXBypass & in_uTargetMask) );

	//CheckBox prevail over the rest, should not happen when bound on RTPC since the UI will disable it.
	ResetFXBypass( in_bitsFXBypass, in_uTargetMask );
	
	NotifyBypass( in_bitsFXBypass, in_uTargetMask );

	return AK_Success;
}

void CAkParameterNodeBase::BypassFX(
	AkUInt32			in_bitsFXBypass,
	AkUInt32			in_uTargetMask,
	CAkRegisteredObj *	in_pGameObj /* = NULL */,
	bool			in_bIsFromReset /* = false */ )
{
	CAkSIS* pSIS = GetSIS( in_pGameObj );

	if( pSIS )
	{
		AkUInt8 prev = pSIS->m_bitsFXBypass;
		pSIS->m_bitsFXBypass = (AkUInt8) ( ( pSIS->m_bitsFXBypass & ~in_uTargetMask ) | ( in_bitsFXBypass & in_uTargetMask )  );
		if( prev != pSIS->m_bitsFXBypass && !in_bIsFromReset )
		{
			MONITOR_PARAMCHANGED( AkMonitorData::NotificationReason_BypassFXChanged, ID(), IsBusCategory(), in_pGameObj?in_pGameObj->ID():AK_INVALID_GAME_OBJECT );
		}
	}

	if( ( NodeCategory() == AkNodeCategory_Bus || NodeCategory() == AkNodeCategory_AuxBus ) 
		&& in_pGameObj == NULL )
	{
		MONITOR_BUSNOTIFICATION(
			ID(), 
			AkMonitorData::BusNotification_FXBypass,
			in_bitsFXBypass, in_uTargetMask );
	}
	if( in_pGameObj == NULL )
	{
		ResetFXBypass( in_bitsFXBypass, in_uTargetMask );
	}

	// Notify playing FXs
	NotifyBypass( in_bitsFXBypass, in_uTargetMask, in_pGameObj );
}

void CAkParameterNodeBase::ResetBypassFX(
		AkUInt32			in_uTargetMask,
		CAkRegisteredObj *	in_pGameObj /*= NULL*/
		)
{
	BypassFX( m_pFXChunk ? m_pFXChunk->bitsMainFXBypass : 0, in_uTargetMask, in_pGameObj, true );
}

///////////////////////////////////////////////////////////////////////////
//  STATES
///////////////////////////////////////////////////////////////////////////

AkStateGroupChunk* CAkParameterNodeBase::AddStateGroup(AkStateGroupID in_ulStateGroupID, bool in_bNotify)
{
	// Now this function simply adds and does not replace, a call with 0 would be suspiscious.
	AKASSERT( in_ulStateGroupID );

	AkStateGroupChunk* pChunk = GetStateGroupChunk( in_ulStateGroupID );
	if( pChunk )
		return pChunk;

	pChunk = AkNew( g_DefaultPoolId, AkStateGroupChunk( this, in_ulStateGroupID ) );
	if ( !pChunk )
		return NULL;

	AKRESULT eResult = g_pStateMgr->AddStateGroupMember( in_ulStateGroupID, pChunk );
	if( eResult != AK_Success )
	{
		AkDelete( g_DefaultPoolId, pChunk );
		return NULL;
	}

	m_states.AddFirst( pChunk );

	pChunk->m_ulActualState = g_pStateMgr->GetState( in_ulStateGroupID );

	if (in_bNotify)
		NotifyStateParametersModified();

	return pChunk;
}

void CAkParameterNodeBase::RemoveStateGroup( AkStateGroupID in_ulStateGroupID, bool in_bNotify )
{
	for ( StateList::IteratorEx it = m_states.BeginEx(); it != m_states.End(); ++it )
	{
		if ( (*it)->m_ulStateGroup == in_ulStateGroupID )
		{
			m_states.Erase( it );
			g_pStateMgr->RemoveStateGroupMember( in_ulStateGroupID, *it );

			while( (*it)->m_mapStates.Length() > 0 )
				(*it)->RemoveState( (*it)->m_mapStates.Begin().pItem->key );

			AKASSERT( (*it)->m_mapStates.IsEmpty() );
			AkDelete( g_DefaultPoolId, *it );
			if (in_bNotify)
				NotifyStateParametersModified();
			return;
		}
	}
}

void CAkParameterNodeBase::RemoveStateGroups(bool in_bNotify)
{
	if ( !m_states.IsEmpty() )
	{
		do
		{
			AkStateGroupChunk * pStateGroupChunk = m_states.First();
			m_states.RemoveFirst();

			g_pStateMgr->RemoveStateGroupMember( pStateGroupChunk->m_ulStateGroup, pStateGroupChunk );

			AkMapStates& mapStates = pStateGroupChunk->m_mapStates;
			while( mapStates.Length() > 0 )
				pStateGroupChunk->RemoveState( mapStates.Begin().pItem->key );

			AkDelete( g_DefaultPoolId, pStateGroupChunk );
		}
		while ( !m_states.IsEmpty() );

		if (in_bNotify)
			NotifyStateParametersModified();
	}
}

AkStateGroupChunk* CAkParameterNodeBase::GetStateGroupChunk( AkStateGroupID in_ulStateGroupID )
{
	for ( StateList::Iterator it = m_states.Begin(); it != m_states.End(); ++it )
	{
		if ( (*it)->m_ulStateGroup == in_ulStateGroupID )
			return *it;
	}

	return NULL;
}

bool CAkParameterNodeBase::UseState() const
{
	return m_bUseState;
}

void CAkParameterNodeBase::UseState(bool in_bUseState)
{
	m_bUseState = in_bUseState;
	NotifyStateParametersModified();
}

AKRESULT CAkParameterNodeBase::AddState( AkStateGroupID in_ulStateGroupID, AkUniqueID in_ulStateInstanceID, AkStateID in_ulStateID)
{
	AkStateGroupChunk* pChunk = GetStateGroupChunk( in_ulStateGroupID );
	AKASSERT( pChunk );
	if( pChunk )
		return pChunk->AddState( in_ulStateInstanceID, in_ulStateID );

	return AK_Fail;
}

void CAkParameterNodeBase::RemoveState( AkStateGroupID in_ulStateGroupID, AkStateID in_ulStateID )
{
	AkStateGroupChunk* pChunk = GetStateGroupChunk( in_ulStateGroupID );
	if( pChunk )
	{
		pChunk->RemoveState( in_ulStateID );
		NotifyStateParametersModified();
	}
}

AKRESULT CAkParameterNodeBase::UpdateStateGroups(AkUInt32 in_uGroups, AkStateGroupUpdate* in_pGroups, AkStateUpdate* in_pStates)
{
	AKRESULT res = AK_Success;
	AKRESULT resFinal = AK_Success;
	bool bNotify = false;

	//Gather all the group IDs so we can detect which ones to delete.
	AkUInt32 uGroupCount = 0;
	for ( StateList::Iterator it = m_states.Begin(); it != m_states.End(); ++it )
		uGroupCount++;

	AkUniqueID *pGroupsToDelete = NULL;
	if (uGroupCount > 0)
	{
		pGroupsToDelete = (AkUniqueID*)AkAlloca(sizeof(AkUniqueID)*uGroupCount);
		if (pGroupsToDelete == NULL)
			return AK_InsufficientMemory;

		for ( StateList::Iterator it = m_states.Begin(); it != m_states.End(); ++it )
		{
			*pGroupsToDelete = it.pItem->m_ulStateGroup;
			pGroupsToDelete++;
		}
		pGroupsToDelete -= uGroupCount;
	}

	//Go through all recieved groups
	for(; in_uGroups > 0; in_uGroups--, in_pGroups++)
	{
		AkStateGroupChunk* pChunk = NULL;

		AkUInt32 iGroup = 0;
		for(; pGroupsToDelete != NULL && iGroup < uGroupCount && pGroupsToDelete[iGroup] != in_pGroups->ulGroupID; iGroup++)
			/*Empty, searching the right slot*/;

		if (iGroup != uGroupCount)
		{
			pGroupsToDelete[iGroup] = pGroupsToDelete[uGroupCount-1];	//This group is used, don't delete it.  //Swap element with last of list.
			uGroupCount--;

			//Check if the sync state type changed.
			pChunk = GetStateGroupChunk(in_pGroups->ulGroupID);
			bNotify |= (pChunk->m_eStateSyncType != (AkUInt8)in_pGroups->eSyncType);	
		}
		else
		{
			bNotify = true;					//New group, we'll need to notify.

			pChunk = AddStateGroup(in_pGroups->ulGroupID);
			if (pChunk == NULL)
			{
				resFinal = AK_InsufficientMemory;
				break;
			}
		}
		pChunk->m_eStateSyncType = (AkUInt8)in_pGroups->eSyncType;

		//Find which states were deleted.
		AkUniqueID *pStatesToDelete = (AkUniqueID*)AkAlloca(sizeof(AkUniqueID)*pChunk->m_mapStates.Reserved());

		AkUInt32 uPreviousStatesCount = 0;
		AkUniqueID *pState = pStatesToDelete;
		for ( AkMapStates::Iterator it = pChunk->m_mapStates.Begin(); it != pChunk->m_mapStates.End(); ++it )
		{
			*pState = it.pItem->item.ulStateID;	
			pState++;
			uPreviousStatesCount++;
		}

		for(AkUInt32 i = 0; i < in_pGroups->ulStateCount; i++, in_pStates++)
		{
			//Find if this custom state was used already.
			AkUInt32 iState = 0;
			for(; iState < uPreviousStatesCount && pStatesToDelete[iState] != in_pStates->ulStateInstanceID; iState++)
				/*Empty, searching the right slot*/;

			if (iState != uPreviousStatesCount)
			{
				//This group is used, don't delete it.  
				pStatesToDelete[iState] = pStatesToDelete[uPreviousStatesCount-1];	//Swap element with last of list.
				uPreviousStatesCount--;
			}
			else
			{
				bNotify = true;	//New group, we'll need to notify.

				res = pChunk->AddState(in_pStates->ulStateInstanceID, in_pStates->ulStateID, false);
				if (res != AK_Success)
					resFinal = res;
			}
		}

		//Delete states that disappeared
		bNotify |= uPreviousStatesCount > 0;
		for(AkUInt32 iState = 0; iState < uPreviousStatesCount; iState++)
			pChunk->RemoveState(pStatesToDelete[iState]);
	}

	//Delete unused groups.
	bNotify |= uGroupCount > 0;
	for(AkUInt32 iGroup = 0; iGroup < uGroupCount; iGroup++)
		RemoveStateGroup(pGroupsToDelete[iGroup], false);

	if (bNotify)
		NotifyStateParametersModified();

	return resFinal;
}

void CAkParameterNodeBase::NotifyStateParametersModified()
{
	for ( StateList::Iterator iter = m_states.Begin(); iter != m_states.End(); ++iter )
	{
		AkStateGroupChunk* pStateGroupChunk = *iter;

		CAkState* pState = pStateGroupChunk->GetState();
		if( pState )
		{
			for ( AkPropBundle<AkReal32>::Iterator it = pState->Props().Begin(), itEnd = pState->Props().End(); it != itEnd; ++it )
			{
				AkReal32 fProp = *it.pValue;

				AkStateValue * pChunkValue = pStateGroupChunk->m_values.FindProp( (AkPropID) *it.pID );
				if ( pChunkValue )
				{
					if ( pChunkValue->pTransition )
					{
						g_pTransitionManager->ChangeParameter(
										pChunkValue->pTransition,
										*it.pID,
										fProp,
										0,
										AkCurveInterpolation_Linear,
										AkValueMeaning_Default);
					}
					else
					{
						pChunkValue->fValue = fProp;
					}
				}
				else
				{
					pChunkValue = pStateGroupChunk->m_values.AddAkProp( (AkPropID) *it.pID );
					if ( pChunkValue )
					{
						pChunkValue->fValue = fProp;
						pChunkValue->pTransition = NULL;
					}
				}
			}
		}
		else
		{
			pStateGroupChunk->FlushStateTransitions();// Only on the stateGroup.

			for ( AkStateValues::Iterator it = pStateGroupChunk->m_values.Begin(), itEnd = pStateGroupChunk->m_values.End(); it != itEnd; ++it )
				it.pValue->fValue = 0.0f;
		}
	}

	RecalcNotification();
}

void CAkParameterNodeBase::SetMaxReachedBehavior( bool in_bKillNewest )
{
	if( m_bKillNewest != in_bKillNewest )
	{
		m_bKillNewest = in_bKillNewest;
		if( IsActivityChunkEnabled() )
		{
			m_pActivityChunk->m_Limiter.SwapOrdering();
			for( AkPerObjPlayCount::Iterator iterMax = m_pActivityChunk->m_ListPlayCountPerObj.Begin(); 
				iterMax != m_pActivityChunk->m_ListPlayCountPerObj.End(); 
				++iterMax )
			{
				iterMax.pItem->item.SwapOrdering();
			}
		}
	}
}

void CAkParameterNodeBase::SetOverLimitBehavior( bool in_bUseVirtualBehavior )
{
	if( m_bUseVirtualBehavior != in_bUseVirtualBehavior )
	{
		m_bUseVirtualBehavior = in_bUseVirtualBehavior;
		if( IsActivityChunkEnabled() )
		{
			m_pActivityChunk->m_Limiter.SetUseVirtualBehavior( in_bUseVirtualBehavior );
			for( AkPerObjPlayCount::Iterator iterMax = m_pActivityChunk->m_ListPlayCountPerObj.Begin(); 
				iterMax != m_pActivityChunk->m_ListPlayCountPerObj.End(); 
				++iterMax )
			{
				iterMax.pItem->item.SetUseVirtualBehavior( in_bUseVirtualBehavior );
			}
		}
	}
}

void CAkParameterNodeBase::SetMaxNumInstances( AkUInt16 in_u16MaxNumInstance )
{
	if( !m_RTPCBitArray.IsSet( RTPC_MaxNumInstances ) )
		ApplyMaxNumInstances( in_u16MaxNumInstance );
	else
		m_u16MaxNumInstance = in_u16MaxNumInstance;
}

void CAkParameterNodeBase::SetIsGlobalLimit( bool in_bIsGlobalLimit )
{
	m_bIsGlobalLimit = in_bIsGlobalLimit;
}

void CAkParameterNodeBase::SetMaxNumInstOverrideParent( bool in_bOverride )
{
	m_bIsMaxNumInstOverrideParent = in_bOverride;
}

void CAkParameterNodeBase::SetVVoicesOptOverrideParent( bool in_bOverride )
{
	m_bIsVVoicesOptOverrideParent = in_bOverride;
}

PriorityInfo CAkParameterNodeBase::GetPriority( CAkRegisteredObj * in_GameObjPtr )
{
	if( Parent() && !m_bPriorityOverrideParent)
		return Parent()->GetPriority( in_GameObjPtr );
	else
	{
		PriorityInfo prInfo;
		GetPropAndRTPCExclusive( prInfo.priority, AkPropID_Priority, in_GameObjPtr );
		prInfo.distanceOffset = m_bPriorityApplyDistFactor ? m_props.GetAkProp( AkPropID_PriorityDistanceOffset, g_AkPropDefault[ AkPropID_PriorityDistanceOffset ] ).fValue : 0.0f;
		return prInfo;
	}
}

void CAkParameterNodeBase::SetAkProp( AkPropID in_eProp, AkReal32 in_fValue, AkReal32 /*in_fMin*/, AkReal32 /*in_fMax*/ )
{
	AkReal32 fProp = m_props.GetAkProp( in_eProp, g_AkPropDefault[ in_eProp ] ).fValue;
	if( fProp != in_fValue )
	{
		m_props.SetAkProp( in_eProp, in_fValue );
		RecalcNotification();
	}
}

void CAkParameterNodeBase::SetAkProp( AkPropID in_eProp, AkInt32 in_iValue, AkInt32 /*in_iMin*/, AkInt32 /*in_iMax*/ )
{
	AkInt32 iProp = m_props.GetAkProp( in_eProp, g_AkPropDefault[ in_eProp ] ).iValue;
	if( iProp != in_iValue )
	{
		m_props.SetAkProp( in_eProp, in_iValue );
		RecalcNotification();
	}
}

AkPropValue * CAkParameterNodeBase::FindCustomProp( AkUInt32 in_uPropID )
{
	return m_props.FindProp( (AkPropID) ( in_uPropID + AkPropID_NUM ) );
}

void CAkParameterNodeBase::SetPriorityApplyDistFactor( bool in_bApplyDistFactor )
{
	if( m_bPriorityApplyDistFactor != in_bApplyDistFactor )
	{
		m_bPriorityApplyDistFactor = in_bApplyDistFactor;
		RecalcNotification();
	}
}

void CAkParameterNodeBase::SetPriorityOverrideParent( bool in_bOverrideParent )
{
	if( m_bPriorityOverrideParent != in_bOverrideParent )
	{
		m_bPriorityOverrideParent = in_bOverrideParent;
		RecalcNotification();
	}
}

void CAkParameterNodeBase::RecalcNotification()
{

}

void CAkParameterNodeBase::PosSetPannerEnabled( bool in_bIsPannerEnabled )
{
	m_bPositioningEnablePanner = in_bIsPannerEnabled;
	PositioningChangeNotification( in_bIsPannerEnabled, (AkRTPC_ParameterID) POSID_2DPannerEnabled, NULL );
}

void CAkParameterNodeBase::SetPositioningEnabled( bool in_bIsPosEnabled )
{
	m_bPositioningEnabled = in_bIsPosEnabled;
	// PositioningChangeNotification too could be possible to make a step foward live editing, investigate... see function above...
}

AKRESULT CAkParameterNodeBase::SetPositioningParams( AkUInt8*& /*io_rpData*/, AkUInt32& /*io_rulDataSize*/ )
{
	AKASSERT(!"Dummy  virtual function");
	return AK_NotImplemented;
}

AKRESULT CAkParameterNodeBase::SetAuxParams( AkUInt8*& /*io_rpData*/, AkUInt32& /*io_rulDataSize*/ )
{
	AKASSERT(!"Dummy  virtual function");
	return AK_NotImplemented;
}

AKRESULT CAkParameterNodeBase::SetAdvSettingsParams( AkUInt8*& /*io_rpData*/, AkUInt32& /*io_rulDataSize*/ )
{
	AKASSERT(!"Dummy  virtual function");
	return AK_NotImplemented;
}

AKRESULT CAkParameterNodeBase::ReadStateChunk( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AkUInt32 ulNumStateGroups = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );

	for( AkUInt32 groupIdx = 0 ; groupIdx < ulNumStateGroups ; ++groupIdx )
	{
		AkStateGroupID ulStateGroupID = READBANKDATA( AkStateGroupID, io_rpData, io_rulDataSize );
		AKASSERT( ulStateGroupID );

		AkStateGroupChunk* pChunk = AddStateGroup( ulStateGroupID );
		if(!pChunk)
			return AK_Fail;

		pChunk->m_eStateSyncType = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

		// Read Num States
		AkUInt32 ulNumStates = READBANKDATA( AkUInt16, io_rpData, io_rulDataSize );

		for( AkUInt32 i = 0 ; i < ulNumStates ; ++i )
		{
			AkUInt32 state = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);
			AkUInt32 iD = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);

			AKRESULT eResult = pChunk->AddState( iD, state );
			if(eResult != AK_Success)
				return eResult;
		}
	}

	return AK_Success;
}

AKRESULT CAkParameterNodeBase::SetNodeBaseParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize, bool in_bPartialLoadOnly )
{
	AKRESULT eResult = AK_Success;

	//Read FX
	eResult = SetInitialFxParams(io_rpData, io_rulDataSize, in_bPartialLoadOnly);
	if( eResult != AK_Success || in_bPartialLoadOnly )
	{
		return eResult;
	}

	AkUniqueID OverrideBusId = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);

	if(OverrideBusId)
	{
		CAkParameterNodeBase* pBus = g_pIndex->GetNodePtrAndAddRef( OverrideBusId, AkNodeType_Bus );
		if(pBus)
		{
			eResult = pBus->AddChildByPtr( this );
			pBus->Release();
		}
		else
		{
			// It is now an error to not load the bank content in the proper order.
			MONITOR_ERRORMSG( AKTEXT("Master bus structure not loaded: make sure that the first bank to be loaded contains the master bus information") );
			eResult = AK_Fail;
		}

		if( eResult != AK_Success )
		{	
			return eResult;
		}
	}

	AkUniqueID DirectParentID = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );

	if( DirectParentID != AK_INVALID_UNIQUE_ID )
	{
		CAkParameterNodeBase* pParent = g_pIndex->GetNodePtrAndAddRef( DirectParentID, AkNodeType_Default );
		if( pParent )
		{
			eResult = pParent->AddChildByPtr( this );
			pParent->Release();
			if( eResult != AK_Success )
			{	
				return eResult;
			}
		}
	}

	AkUInt8 bPriorityOverrideParent = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
	AkUInt8 bPriorityApplyDistFactor = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

	SetPriorityOverrideParent( bPriorityOverrideParent != 0 );
	SetPriorityApplyDistFactor( bPriorityApplyDistFactor != 0 );

	if(eResult == AK_Success)
	{
		eResult = SetInitialParams( io_rpData, io_rulDataSize );
	}

	if(eResult == AK_Success)
	{
		eResult = SetPositioningParams( io_rpData, io_rulDataSize );
	}

	if(eResult == AK_Success)
	{
		eResult = SetAuxParams( io_rpData, io_rulDataSize );
	}

	if(eResult == AK_Success)
	{
		eResult = SetAdvSettingsParams( io_rpData, io_rulDataSize );
	}

	if(eResult == AK_Success)
	{
		eResult = ReadStateChunk(io_rpData, io_rulDataSize);
	}

	if(eResult == AK_Success)
	{
		eResult = SetInitialRTPC(io_rpData, io_rulDataSize);
	}

	//Read feedback info
	if (eResult == AK_Success)
	{
		eResult = ReadFeedbackInfo(io_rpData, io_rulDataSize);
	}

	return eResult;
}

AKRESULT CAkParameterNodeBase::ReadFeedbackInfo(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize)
{
	if (!CAkBankMgr::BankHasFeedback())
		return AK_Success;

	AkUniqueID BusId = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);

	if(BusId != AK_INVALID_UNIQUE_ID)
	{

#ifdef AK_MOTION

		if (m_pFeedbackInfo == NULL)
		{
			AkNew2( m_pFeedbackInfo, g_DefaultPoolId, AkFeedbackInfo, AkFeedbackInfo());
			if (m_pFeedbackInfo == NULL)
				return AK_InsufficientMemory;
		}

		CAkParameterNodeBase* pBus = g_pIndex->GetNodePtrAndAddRef( BusId, AkNodeType_Bus );
		AKRESULT eResult = AK_Fail;
		if(pBus)
		{
			eResult = pBus->AddChildByPtr( this );
			pBus->Release();
		}
		
		if( eResult != AK_Success )
			return eResult;

#endif // AK_MOTION

	}
	return AK_Success;
}

AKRESULT CAkParameterNodeBase::SetInitialRTPC(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	// Read Num RTPC
	AkUInt32 ulNumRTPC = READBANKDATA( AkUInt16, io_rpData, io_rulDataSize );

	for(AkUInt32 i = 0; i < ulNumRTPC; ++i)
	{
		AkRtpcID l_RTPCID = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
		AkRTPC_ParameterID l_ParamID = (AkRTPC_ParameterID)READBANKDATA(AkUInt32, io_rpData, io_rulDataSize); // Volume, Pitch, LFE...
		const AkUniqueID rtpcCurveID = (AkUniqueID)READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);
		AkCurveScaling eScaling = (AkCurveScaling) READBANKDATA(AkUInt8, io_rpData, io_rulDataSize);

		// ArraySize //i.e.:number of conversion points
		AkUInt32 ulSize = READBANKDATA(AkUInt16, io_rpData, io_rulDataSize);

		SetRTPC( l_RTPCID, l_ParamID, rtpcCurveID, eScaling, (AkRTPCGraphPoint*)io_rpData, ulSize );

		//Skipping the Conversion array
		io_rpData += ( ulSize*sizeof(AkRTPCGraphPoint) );
		io_rulDataSize -= ( ulSize*sizeof(AkRTPCGraphPoint) );
	}

	return AK_Success;
}

AKRESULT CAkParameterNodeBase::SetParamComplexFromRTPCManager( 
		void * in_pToken,
		AkUInt32 in_Param_id, 
		AkRtpcID in_RTPCid,
		AkReal32 in_value, 
		CAkRegisteredObj * in_GameObj,
		void* in_pGameObjExceptArray
		)
{
	AKASSERT( m_RTPCBitArray.IsSet( in_Param_id ) );

	AKRESULT eResult = AK_Success;

	AkReal32 l_fromValue = 0;
	switch(in_Param_id)
	{
	case RTPC_Volume:
	case RTPC_Pitch:
	case RTPC_LPF:
	case RTPC_BusVolume:
	case RTPC_FeedbackVolume:
	case RTPC_FeedbackLowpass:
	case RTPC_UserAuxSendVolume0:
    case RTPC_UserAuxSendVolume1:
    case RTPC_UserAuxSendVolume2:
    case RTPC_UserAuxSendVolume3:
	case RTPC_GameAuxSendVolume:
    case RTPC_OutputBusVolume:
    case RTPC_OutputBusLPF:
	case RTPC_HDRBusThreshold:
	case RTPC_HDRBusReleaseTime:
	case RTPC_HDRBusRatio:
	case RTPC_HDRActiveRange:
	case RTPC_MakeUpGain:
		l_fromValue = g_pRTPCMgr->GetRTPCConvertedValue( in_pToken, in_GameObj, in_RTPCid  );
		Notification( (AkRTPC_ParameterID) in_Param_id, in_value - l_fromValue, in_GameObj, in_pGameObjExceptArray );
		break;

	case RTPC_Priority:
		if( PriorityOverrideParent() || !Parent() )
		{
			PriorityNotification( in_value, in_GameObj, in_pGameObjExceptArray );
		}
		break;

	case RTPC_BypassFX0:
		NotifyBypass( ( ( in_value != 0 ) ? 1 : 0 ) << 0, 1 << 0, in_GameObj, in_pGameObjExceptArray );
		break;

	case RTPC_BypassFX1:
		NotifyBypass( ( ( in_value != 0 ) ? 1 : 0 ) << 1, 1 << 1, in_GameObj, in_pGameObjExceptArray );
		break;

	case RTPC_BypassFX2:
		NotifyBypass( ( ( in_value != 0 ) ? 1 : 0 ) << 2, 1 << 2, in_GameObj, in_pGameObjExceptArray );
		break;

	case RTPC_BypassFX3:
		NotifyBypass( ( ( in_value != 0 ) ? 1 : 0 ) << 3, 1 << 3, in_GameObj, in_pGameObjExceptArray );
		break;
	
	case RTPC_BypassAllFX:
		NotifyBypass( ( ( in_value != 0 ) ? 1 : 0 ) << AK_NUM_EFFECTS_BYPASS_ALL_FLAG, 1 << AK_NUM_EFFECTS_BYPASS_ALL_FLAG, in_GameObj, in_pGameObjExceptArray );
		break;

	case RTPC_MaxNumInstances:
		if( m_bIsMaxNumInstOverrideParent || Parent() == NULL )
		{
			ApplyMaxNumInstances( (AkUInt16)(in_value), in_GameObj, in_pGameObjExceptArray, true );
		}
		
		break;

	case POSID_PositioningType:
	case POSID_Positioning_Cone_LPF:
	case POSID_Positioning_Divergence_Center_PCT:
	case POSID_Positioning_Cone_Attenuation_ON_OFF:
	case POSID_Positioning_Cone_Attenuation:
	
	case POSID_Position_PAN_X_2D:
	case POSID_Position_PAN_Y_2D:
	case POSID_Position_PAN_X_3D:
	case POSID_Position_PAN_Y_3D:
	case POSID_IsPositionDynamic:
	case POSID_IsLooping:
	case POSID_Transition:
	case POSID_PathMode:
		PositioningChangeNotification( in_value, (AkRTPC_ParameterID)in_Param_id, in_GameObj, in_pGameObjExceptArray );
		break;

	case RTPC_InitialDelay:
		//nothing to be done.
		break;

	default:
		AKASSERT( !"Receiving an unexpected RTPC notification, ignoring the unknown notification" );
		eResult = AK_Fail;
		break;
	}

	return eResult;
}

void CAkParameterNodeBase::SetRTPC(
		AkRtpcID			in_RTPC_ID,
		AkRTPC_ParameterID	in_ParamID,
		AkUniqueID			in_RTPCCurveID,
		AkCurveScaling		in_eScaling,
		AkRTPCGraphPoint*	in_pArrayConversion,		// NULL if none
		AkUInt32			in_ulConversionArraySize	// 0 if none
		)
{
	AKASSERT(g_pRTPCMgr);

	AKASSERT( in_ParamID < sizeof(AkUInt64)*8 );

	m_RTPCBitArray.SetBit( in_ParamID );

	if( g_pRTPCMgr )
	{
		g_pRTPCMgr->SubscribeRTPC( 
			this,
			in_RTPC_ID, 
			in_ParamID, 
			in_RTPCCurveID,
			in_eScaling,
			in_pArrayConversion, 
			in_ulConversionArraySize,
			NULL,
			GetRTPCSubscriberType()
			);

		UpdateBusBypass( in_ParamID );
	}
}

void CAkParameterNodeBase::UnsetRTPC( AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID )
{
	AKASSERT( in_ParamID < sizeof(AkUInt64)*8 );

	bool bMoreCurvesRemaining = false;

	if( g_pRTPCMgr )
	{
		g_pRTPCMgr->UnSubscribeRTPC(
			this,
			in_ParamID,
			in_RTPCCurveID,
			&bMoreCurvesRemaining
			);
	}

	if ( ! bMoreCurvesRemaining )
		m_RTPCBitArray.UnsetBit( in_ParamID );

	RecalcNotification();

	// Notify Positioning if required.
	{
		AkPropID id;

		switch( in_ParamID )
		{
		case RTPC_Position_PAN_X_2D:
			id = AkPropID_PAN_LR;
			break;
		case RTPC_Position_PAN_Y_2D:
			id = AkPropID_PAN_FR;
			break;
		case RTPC_Position_PAN_X_3D:
		case RTPC_Position_PAN_Y_3D:
			PositioningChangeNotification( 0.0f, in_ParamID, NULL ); // There is no actual property for 3d panning
			return;
		default:
			return;
		}

		AkReal32 fValue = m_props.GetAkProp( id, g_AkPropDefault[ id ] ).fValue;

		PositioningChangeNotification( fValue, in_ParamID, NULL );
	}
}

void CAkParameterNodeBase::SetStateSyncType( AkStateGroupID in_stateGroupID, AkUInt32/*AkSyncType*/ in_eSyncType )
{
	AkStateGroupChunk* pChunk = AddStateGroup( in_stateGroupID );
	if( pChunk )
	{
		pChunk->m_eStateSyncType = (AkUInt8)in_eSyncType;
	}
}

bool CAkParameterNodeBase::GetStateSyncTypes( AkStateGroupID in_stateGroupID, CAkStateSyncArray* io_pSyncTypes, bool in_bBusChecked /*=false*/ )
{
	if( CheckSyncTypes( in_stateGroupID, io_pSyncTypes ) )
		return true;

	if( !in_bBusChecked && ParentBus() )
	{
		in_bBusChecked = true;
		if( static_cast<CAkBus*>( ParentBus() )->GetStateSyncTypes( in_stateGroupID, io_pSyncTypes ) )
		{
			return true;
		}
	}
	if( Parent() )
	{
		return Parent()->GetStateSyncTypes( in_stateGroupID, io_pSyncTypes, in_bBusChecked );
	}
	return false;
}

bool CAkParameterNodeBase::CheckSyncTypes( AkStateGroupID in_stateGroupID, CAkStateSyncArray* io_pSyncTypes )
{
	AkStateGroupChunk* pStateGroupChunk = GetStateGroupChunk( in_stateGroupID );
	if( pStateGroupChunk )
	{
		AkSyncType syncType = (AkSyncType)pStateGroupChunk->m_eStateSyncType;
		if( syncType == SyncTypeImmediate )
		{
			io_pSyncTypes->RemoveAllSync();
            io_pSyncTypes->GetStateSyncArray().AddLast( syncType );
			return true;
		}
		else
		{
			bool bFound = false;
			for( StateSyncArray::Iterator iter = io_pSyncTypes->GetStateSyncArray().Begin(); iter != io_pSyncTypes->GetStateSyncArray().End(); ++iter )
			{
				if( *iter == syncType )
				{
					bFound = true;
					break;
				}
			}
			if( !bFound )
			{
				io_pSyncTypes->GetStateSyncArray().AddLast( syncType );
			}
		}
	}
	return false;
}

CAkRTPCMgr::SubscriberType CAkParameterNodeBase::GetRTPCSubscriberType() const
{
	return CAkRTPCMgr::SubscriberType_CAkParameterNodeBase;
}

#ifdef AK_MOTION
// Set the feedback volume
void CAkParameterNodeBase::SetFeedbackVolume(
	CAkRegisteredObj *	in_GameObjPtr,				//Game object associated to the action
	AkValueMeaning	in_eValueMeaning,		//Target value meaning
	AkReal32			in_fTargetValue,	// Volume target value
	AkCurveInterpolation in_eFadeCurve ,
	AkTimeMs		in_lTransitionTime)
{

#ifndef AK_OPTIMIZED
	MONITOR_SETPARAMNOTIF_FLOAT( AkMonitorData::NotificationReason_FeedbackVolumeChanged, ID(), IsBusCategory(), in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT, in_fTargetValue, in_eValueMeaning, in_lTransitionTime );

	if(    ( in_eValueMeaning == AkValueMeaning_Offset && in_fTargetValue != 0 )
		|| ( in_eValueMeaning == AkValueMeaning_Independent && in_fTargetValue != m_props.GetAkProp( AkPropID_FeedbackVolume, 0.0f ).fValue ) )
	{
		MONITOR_PARAMCHANGED(AkMonitorData::NotificationReason_FeedbackVolumeChanged, ID(), IsBusCategory(), in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT );
	}
#endif

	CAkSIS* pSIS = GetSIS( in_GameObjPtr );
	if ( pSIS )
		StartSISTransition( pSIS, AkPropID_FeedbackVolume, in_fTargetValue, in_eValueMeaning, in_eFadeCurve, in_lTransitionTime );
}

// Set the output bus for a specific feedback device
void CAkParameterNodeBase::FeedbackParentBus(CAkFeedbackBus* in_pParent)
{
	// Avoid creating the structure if we are not connected.
	if (in_pParent == NULL && m_pFeedbackInfo == NULL)
		return;

	if (m_pFeedbackInfo == NULL)
	{
		AkNew2( m_pFeedbackInfo, g_DefaultPoolId, AkFeedbackInfo, AkFeedbackInfo() );
		if (m_pFeedbackInfo == NULL)
			return;
	}

	m_pFeedbackInfo->m_pFeedbackBus = in_pParent;
}

CAkFeedbackBus* CAkParameterNodeBase::FeedbackParentBus()
{
	if (m_pFeedbackInfo == NULL)
		return NULL;

	return m_pFeedbackInfo->m_pFeedbackBus;
}

CAkFeedbackBus* CAkParameterNodeBase::GetFeedbackParentBusOrDefault()
{
	CAkFeedbackBus* pParent = FeedbackParentBus();

	if( !pParent )
	{
		// linked with: WG-10155 and WG-9702
		// Hack : fixing a whole bunch of bugs with this hack.
		// Actually seems bullet proof, but the user may not have realized that in the project at this point and maybe
		// outputting directy in the motion bus is not what the user wanted.
		pParent = CAkFeedbackBus::GetMasterMotionBusAndAddRef();
		if (pParent != NULL)
		{
			pParent->AddChildByPtr( this );// ignoring error code, it simply has to work.
			pParent->Release();
		}
	}
	
	AKASSERT( pParent );
	return pParent;
}

// Get the compounded feedback parameters.  There is currenly only the volume.
AKRESULT CAkParameterNodeBase::GetFeedbackParameters( AkFeedbackParams &io_Params, CAkSource *in_pSource, CAkRegisteredObj * in_GameObjPtr, bool in_bDoBusCheck /*= true*/ )
{
	bool bFirst = io_Params.m_usPluginID == AkFeedbackParams::UNINITIALIZED;

	if (m_pFeedbackInfo != NULL)
	{
		// Get the volume of this node
		AkVolumeValue l_Volume = GetEffectiveFeedbackVolume(in_GameObjPtr);
		AkLPFType fLPF = GetEffectiveFeedbackLPF(in_GameObjPtr);

		// The bus check is done only once, for the first object (the leaf in the tree).
		// We need to prime the structure with the first volume and the busses		
		if(in_bDoBusCheck)
		{
			CAkFeedbackBus* pBus = m_pFeedbackInfo->m_pFeedbackBus;
			if (bFirst)
			{
				//The param structure isn't initialized yet.  It is the first time
				//we compute the volume on this object.
				io_Params.m_pOutput = pBus;					
				io_Params.m_NewVolume = l_Volume;
				io_Params.m_LPF = fLPF;
				io_Params.m_usPluginID = AkFeedbackParams::ALL_DEVICES;
			}
			else
			{
				//This is an update of the parameters.  Keep the old values.
				io_Params.m_NewVolume = l_Volume;
				io_Params.m_LPF = fLPF;
			}

			//Walk up the feedback busses
			pBus->GetFeedbackParameters(io_Params, in_pSource, in_GameObjPtr, false);

			//Remove the audio bus volume, up until we see an effect.  This the point where 
			//the bus volumes stop being collapsed into the source.
			CAkParameterNodeBase* pAudioBus = NULL;
			CAkParameterNodeBase* pParent = this;
			do
			{
				pAudioBus = pParent->m_pBusOutputNode;
				pParent = pParent->m_pParentNode;
			}while (pAudioBus == NULL && pParent != NULL);

			while(pAudioBus != NULL)
			{
				bool bEffect = false;
				if ( pAudioBus->m_pFXChunk )
				{
					for(AkUInt32 i = 0; i < AK_NUM_EFFECTS_PER_OBJ; i++)
						bEffect |= (pAudioBus->m_pFXChunk->aFX[i].id != AK_INVALID_UNIQUE_ID);
				}

				if (bEffect)
					break;
				else
					io_Params.m_AudioBusVolume += pAudioBus->m_props.GetAkProp( AkPropID_Volume, 0.0f ).fValue;

				pAudioBus = pAudioBus->m_pBusOutputNode;
			}
		}
		else
		{
			// Apply the volume of this node.  It is not related to a particular bus, so
			// we simply apply to all.
			io_Params.m_NewVolume += l_Volume;
			io_Params.m_LPF += fLPF;
		}
	}

	// Get the parent's volumes
	if(m_pParentNode != NULL)
		m_pParentNode->GetFeedbackParameters(io_Params, in_pSource, in_GameObjPtr, false);	

	return AK_Success;
}

AkVolumeValue CAkParameterNodeBase::GetEffectiveFeedbackVolume( CAkRegisteredObj * in_GameObjPtr )
{
	AkVolumeValue l_Volume = 0.0f;
	GetPropAndRTPCExclusive( l_Volume, AkPropID_FeedbackVolume, in_GameObjPtr );

	return l_Volume;
}

AkLPFType CAkParameterNodeBase::GetEffectiveFeedbackLPF( CAkRegisteredObj * in_GameObjPtr )
{
	AkLPFType fLPF = 0.0f;
	GetPropAndRTPC( fLPF, AkPropID_FeedbackLPF, in_GameObjPtr );

	return fLPF;
}
#endif // AK_MOTION

bool CAkParameterNodeBase::IncrementActivityCount( AkUInt16 in_flagForwardToBus /*= AK_ForwardToBusType_ALL*/ )
{
	bool bIsSuccessful = IncrementActivityCountValue( in_flagForwardToBus );
	// We must continue and go up to the top even if bIsSuccessful failed.
	// Decrement will soon be called and will go to the top too, so it must be done all the way up even in case of failure.

	if( in_flagForwardToBus & AK_ForwardToBusType_Normal )
	{
		if( m_pBusOutputNode )
		{
			in_flagForwardToBus &= ~AK_ForwardToBusType_Normal;
			bIsSuccessful &= m_pBusOutputNode->IncrementActivityCount();
		}
	}

#ifdef AK_MOTION
	if( in_flagForwardToBus & AK_ForwardToBusType_Motion )
	{
		if ( m_pFeedbackInfo != NULL && m_pFeedbackInfo->m_pFeedbackBus != NULL )
		{
			in_flagForwardToBus &= ~AK_ForwardToBusType_Motion;
			bIsSuccessful &= m_pFeedbackInfo->m_pFeedbackBus->IncrementActivityCount();
		}
	}
#endif // AK_MOTION

	if( m_pParentNode )
	{
		bIsSuccessful &= m_pParentNode->IncrementActivityCount( in_flagForwardToBus );
	}

	return bIsSuccessful;
}

void CAkParameterNodeBase::DecrementActivityCount( AkUInt16 in_flagForwardToBus /*= AK_ForwardToBusType_ALL*/ )
{
	DecrementActivityCountValue();

	if( in_flagForwardToBus & AK_ForwardToBusType_Normal )
	{
		if( m_pBusOutputNode )
		{
			in_flagForwardToBus &= ~AK_ForwardToBusType_Normal;
			m_pBusOutputNode->DecrementActivityCount();
		}
	}

#ifdef AK_MOTION
	if( in_flagForwardToBus & AK_ForwardToBusType_Motion )
	{
		if ( m_pFeedbackInfo != NULL && m_pFeedbackInfo->m_pFeedbackBus != NULL )
		{
			in_flagForwardToBus &= ~AK_ForwardToBusType_Motion;
			m_pFeedbackInfo->m_pFeedbackBus->DecrementActivityCount();
		}
	}
#endif // AK_MOTION

	if( m_pParentNode )
	{
		m_pParentNode->DecrementActivityCount( in_flagForwardToBus );
	}
}

void CAkParameterNodeBase::GetAudioStateParams( AkSoundParams &io_Parameters, AkUInt32 in_uParamSelect )
{
	if( m_bUseState )
	{
		for( StateList::Iterator iter = m_states.Begin(); iter != m_states.End(); ++iter )
		{
			AkStateGroupChunk* pStateGroupChunk = *iter;

			if(in_uParamSelect & PT_Volume)
			{
				AkStateValue * pValue = pStateGroupChunk->m_values.FindProp( AkPropID_Volume );
				if ( pValue )
					io_Parameters.Volume += pValue->fValue;
			}
			if(in_uParamSelect & PT_Pitch)
			{
				AkStateValue * pValue = pStateGroupChunk->m_values.FindProp( AkPropID_Pitch );
				if ( pValue )
					io_Parameters.Pitch += pValue->fValue;
			}
			if(in_uParamSelect & PT_LPF)
			{
				AkStateValue * pValue = pStateGroupChunk->m_values.FindProp( AkPropID_LPF );
				if ( pValue )
					io_Parameters.LPF += pValue->fValue;
			}
			if(in_uParamSelect & PT_BusVolume)
			{
				AkStateValue * pValue = pStateGroupChunk->m_values.FindProp( AkPropID_BusVolume );
				if ( pValue )
					io_Parameters.BusVolume += pValue->fValue;
			}
		}
	}
}

AkUInt16 CAkParameterNodeBase::GetMaxNumInstances( CAkRegisteredObj * in_GameObjPtr )
{ 
	AkUInt16 u16Max = m_u16MaxNumInstance;

	if( m_RTPCBitArray.IsSet( RTPC_MaxNumInstances ) 
		&& u16Max != 0 // if u16Max == 0, the RTPC must be ignored and there is no limit.
		)
	{
		// Replace with the real value based on RTPCs.
		u16Max = (AkUInt16)g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_MaxNumInstances, in_GameObjPtr );
	}

	return u16Max;
}

AKRESULT CAkParameterNodeBase::IncrementPlayCountGlobal( AkReal32 in_fPriority, AkUInt16& io_ui16NumKickedOrRevived, CAkLimiter*& io_pLimiter )
{
	AKRESULT eResult = AK_Success;
	IncrementPlayCountValid();

	AkUInt16 u16Max = GetMaxNumInstances();
	if( u16Max )
	{
		io_pLimiter = &m_pActivityChunk->m_Limiter;
		if( io_ui16NumKickedOrRevived == 0 && (GetPlayCountValid() - GetVirtualCountValid() > u16Max) )
		{
			CAkParameterNodeBase* pKicked = NULL;
			eResult = CAkURenderer::Kick( io_pLimiter, u16Max, in_fPriority, NULL, m_bKillNewest, m_bUseVirtualBehavior, pKicked, KickFrom_OverNodeLimit );
			++io_ui16NumKickedOrRevived;
		}
	}

	return eResult;
}

void CAkParameterNodeBase::DecrementPlayCountGlobal()
{
	if( IsActivityChunkEnabled() )
	{
		DecrementPlayCountValid();

		if( m_pActivityChunk->ChunkIsUseless() )
		{
			DeleteActivityChunk();
		}
	}
}

void CAkParameterNodeBase::DecrementVirtualCountGlobal( AkUInt16& io_ui16NumKickedOrRevived, bool in_bAllowKick )
{
	if( IsActivityChunkEnabled() )
	{
		DecrementVirtualCountValid();

		if( in_bAllowKick )
		{
			AkUInt16 u16Max = GetMaxNumInstances();

			if( u16Max != 0 && (( GetPlayCountValid() - GetVirtualCountValid()) - io_ui16NumKickedOrRevived ) > u16Max )
			{
				CAkParameterNodeBase* pKicked = NULL;
				CAkURenderer::Kick( &m_pActivityChunk->m_Limiter, u16Max, AK_MAX_PRIORITY + 1, NULL, m_bKillNewest, m_bUseVirtualBehavior, pKicked, KickFrom_OverNodeLimit );
				++io_ui16NumKickedOrRevived;
			}
		}

		if( m_pActivityChunk->ChunkIsUseless() )
		{
			DeleteActivityChunk();
		}
	}
}

AKRESULT CAkParameterNodeBase::IncrementPlayCountGameObject( AkReal32 in_fPriority, AkUInt16& io_ui16NumKickedOrRevived, CAkRegisteredObj * in_pGameObj, CAkLimiter*& io_pLimiter )
{
	AKRESULT eResult = AK_Success;

	AkUInt16 u16Max = 0;
	StructMaxInst* pPerObjCount = m_pActivityChunk->m_ListPlayCountPerObj.Exists( in_pGameObj );
	if( pPerObjCount )
	{
		pPerObjCount->Increment();
		u16Max = pPerObjCount->GetMax();
		if( pPerObjCount->IsMaxNumInstancesActivated() && ((pPerObjCount->GetCurrent() - pPerObjCount->GetCurrentVirtual()) - io_ui16NumKickedOrRevived ) > u16Max )
		{
			CAkParameterNodeBase* pKicked = NULL;
			eResult = CAkURenderer::Kick( pPerObjCount->m_pLimiter, u16Max, in_fPriority, in_pGameObj, m_bKillNewest, m_bUseVirtualBehavior, pKicked, KickFrom_OverNodeLimit );
			// We must increment the kick count only if the kicked one was bound by the same bus restrictions.
			if( !pKicked || pKicked->GetLimitingBus() == GetLimitingBus() )
			{
				++(io_ui16NumKickedOrRevived);
			}
		}
	}
	else
	{
		AKASSERT( in_pGameObj );
		
		u16Max = GetMaxNumInstances( in_pGameObj );

		StructMaxInst structMaxInst = StructMaxInst( 1, u16Max, DoesKillNewest(), m_bUseVirtualBehavior );
		pPerObjCount = m_pActivityChunk->m_ListPlayCountPerObj.Set( 
			in_pGameObj, 
			structMaxInst 
			);
		if( !pPerObjCount || structMaxInst.m_pLimiter == NULL )
		{
			structMaxInst.DisableLimiter();
			//Don't allow this sound to play in low memory situations if you cannot limit it.
			eResult = AK_Fail;
		}
	}

	if( pPerObjCount && u16Max !=0 )
	{
		io_pLimiter = pPerObjCount->m_pLimiter;
	}

	return eResult;
}

void CAkParameterNodeBase::DecrementPlayCountGameObject( CAkRegisteredObj * in_pGameObj )
{
	StructMaxInst* pPerObjCount = m_pActivityChunk->m_ListPlayCountPerObj.Exists( in_pGameObj );
	if( pPerObjCount )
	{
		pPerObjCount->Decrement();

		if( pPerObjCount->GetCurrent() == 0 && pPerObjCount->GetCurrentVirtual() == 0 )
		{
			pPerObjCount->DisableLimiter();
			m_pActivityChunk->m_ListPlayCountPerObj.Unset( in_pGameObj );
		}

		if( m_pActivityChunk->ChunkIsUseless() )
		{
			DeleteActivityChunk();
		}
	}
}

void CAkParameterNodeBase::IncrementVirtualCountGameObject( CAkRegisteredObj * in_pGameObj )
{
	StructMaxInst* pPerObjCount = m_pActivityChunk->m_ListPlayCountPerObj.Exists( in_pGameObj );
	if( pPerObjCount )
	{
		pPerObjCount->IncrementVirtual();
	}
}

void CAkParameterNodeBase::DecrementVirtualCountGameObject( AkUInt16& io_ui16NumKickedOrRevived, bool in_bAllowKick, CAkRegisteredObj * in_pGameObj )
{
	StructMaxInst* pPerObjCount = m_pActivityChunk->m_ListPlayCountPerObj.Exists( in_pGameObj );
	if( pPerObjCount )
	{
		pPerObjCount->DecrementVirtual();

		if( in_bAllowKick )
		{
			AkUInt16 u16Max = pPerObjCount->GetMax();
			if( pPerObjCount->IsMaxNumInstancesActivated() && ((pPerObjCount->GetCurrent() - pPerObjCount->GetCurrentVirtual()) - io_ui16NumKickedOrRevived ) > u16Max )
			{
				CAkParameterNodeBase* pKicked = NULL;
				CAkURenderer::Kick( pPerObjCount->m_pLimiter, u16Max, AK_MAX_PRIORITY + 1, in_pGameObj, m_bKillNewest, m_bUseVirtualBehavior, pKicked, KickFrom_OverNodeLimit );
				if( !pKicked || pKicked->GetLimitingBus() == GetLimitingBus() )
				{
					++(io_ui16NumKickedOrRevived);
				}
			}
		}
		
		if( pPerObjCount->GetCurrent() == 0 && pPerObjCount->GetCurrentVirtual() == 0 )
		{
			pPerObjCount->DisableLimiter();
			m_pActivityChunk->m_ListPlayCountPerObj.Unset( in_pGameObj );
			if( m_pActivityChunk->ChunkIsUseless() )
			{
				DeleteActivityChunk();
			}
		}
	}
}

AKRESULT CAkParameterNodeBase::IncrementPlayCountValue( AkUInt16 in_flagForwardToBus )
{
	bool bRet = EnableActivityChunk( in_flagForwardToBus );
	if( IsActivityChunkEnabled() )
	{
		m_pActivityChunk->IncrementPlayCount();
	}
	return bRet ? AK_Success : AK_Fail;
}

void CAkParameterNodeBase::DecrementPlayCountValue()
{
	if( IsActivityChunkEnabled() )
	{
		m_pActivityChunk->DecrementPlayCount();
		if( m_pActivityChunk->ChunkIsUseless() )
		{
			DeleteActivityChunk();
		}
	}
}

bool CAkParameterNodeBase::IncrementActivityCountValue( AkUInt16 in_flagForwardToBus )
{
	bool ret = EnableActivityChunk( in_flagForwardToBus );
	if( IsActivityChunkEnabled() )
	{
		m_pActivityChunk->IncrementActivityCount();
	}
	return ret;
}

void CAkParameterNodeBase::DecrementActivityCountValue()
{
	if( IsActivityChunkEnabled() )
	{
		m_pActivityChunk->DecrementActivityCount();
		if( m_pActivityChunk->ChunkIsUseless() )
		{
			DeleteActivityChunk();
		}
	}
}

bool CAkParameterNodeBase::Get2DParams( CAkRegisteredObj * in_GameObj, BaseGenParams* out_pBasePosParams )
{
	AKASSERT( out_pBasePosParams );
	AKASSERT( g_pRTPCMgr );

	bool bRTPC_Position_PAN_X = m_RTPCBitArray.IsSet( RTPC_Position_PAN_X_2D );
	bool bRTPC_Position_PAN_Y = m_RTPCBitArray.IsSet( RTPC_Position_PAN_Y_2D );

	if( bRTPC_Position_PAN_X | bRTPC_Position_PAN_Y )
	{
		if( bRTPC_Position_PAN_X )
		{
			out_pBasePosParams->m_fPAN_X_2D = g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_Position_PAN_X_2D, in_GameObj );
		}
		else
		{
			out_pBasePosParams->m_fPAN_X_2D = 0.0f;
		}

		if( bRTPC_Position_PAN_Y )
		{
			out_pBasePosParams->m_fPAN_Y_2D = g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_Position_PAN_Y_2D, in_GameObj );
		}
		else
		{
			out_pBasePosParams->m_fPAN_Y_2D = 0.0f;
		}
	}
	else
	{
		out_pBasePosParams->m_fPAN_X_2D = m_props.GetAkProp( AkPropID_PAN_LR, 0.0f ).fValue;
		out_pBasePosParams->m_fPAN_Y_2D = m_props.GetAkProp( AkPropID_PAN_FR, 0.0f ).fValue;
	}

	out_pBasePosParams->m_fCenterPCT = m_props.GetAkProp( AkPropID_CenterPCT, 0.0f ).fValue;
	out_pBasePosParams->bIsPannerEnabled = m_bPositioningEnablePanner;

	return bRTPC_Position_PAN_X | bRTPC_Position_PAN_Y;
}

bool CAkParameterNodeBase::Get3DPanning( CAkRegisteredObj * in_GameObj, AkVector & out_posPan )
{
	AKASSERT( Has3DParams() );

	bool bRTPC_Position_PAN_X = m_RTPCBitArray.IsSet( RTPC_Position_PAN_X_3D );
	bool bRTPC_Position_PAN_Y = m_RTPCBitArray.IsSet( RTPC_Position_PAN_Y_3D );

	AkReal32 fMaxRadius = 0.0f;
	if ( bRTPC_Position_PAN_X | bRTPC_Position_PAN_Y )
	{
		if( bRTPC_Position_PAN_X )
		{
			out_posPan.X = g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_Position_PAN_X_3D, in_GameObj );
			if( GetMaxRadius( fMaxRadius ) )
				out_posPan.X *= fMaxRadius / 100.0f;
		}
		else
		{
			out_posPan.X = AK_DEFAULT_SOUND_POSITION_X;
		}
	
		out_posPan.Y = AK_DEFAULT_SOUND_POSITION_Y;

		if( bRTPC_Position_PAN_Y )
		{
			out_posPan.Z = g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_Position_PAN_Y_3D, in_GameObj );
			if( fMaxRadius != 0.0f || GetMaxRadius( fMaxRadius ) )
				out_posPan.Z *= fMaxRadius / 100.0f;
		}
		else
		{
			out_posPan.Z = AK_DEFAULT_SOUND_POSITION_Z;
		}

	}
	return bRTPC_Position_PAN_X | bRTPC_Position_PAN_Y;
}

// Monitoring solo/mute (profiling only)
#ifndef AK_OPTIMIZED
void CAkParameterNodeBase::MonitoringSolo( bool in_bSolo )
{
	_MonitoringSolo( in_bSolo, g_uSoloCount );
}

void CAkParameterNodeBase::MonitoringMute( bool in_bMute )
{
	_MonitoringMute( in_bMute, g_uMuteCount );
}

void CAkParameterNodeBase::_MonitoringSolo( bool in_bSolo, AkUInt32& io_ruSoloCount )
{
	if ( in_bSolo && !m_bIsSoloed )
		++io_ruSoloCount;
	else if ( !in_bSolo && m_bIsSoloed )
		--io_ruSoloCount;

	m_bIsSoloed = in_bSolo;
	
	g_bIsMonitoringMuteSoloDirty = true;
}

void CAkParameterNodeBase::_MonitoringMute( bool in_bMute, AkUInt32& io_ruMuteCount )
{
	if ( in_bMute && !m_bIsMuted )
		++io_ruMuteCount;
	else if ( !in_bMute && m_bIsMuted )
		--io_ruMuteCount;
	m_bIsMuted = in_bMute;

	g_bIsMonitoringMuteSoloDirty = true;
}

bool CAkParameterNodeBase::IsRefreshMonitoringMuteSoloNeeded() 
{ 
	bool bDirty = g_bIsMonitoringMuteSoloDirty; 
	g_bIsMonitoringMuteSoloDirty = false;
	return bDirty; 
}

void CAkParameterNodeBase::GetMonitoringMuteSoloState( 
	bool in_bCheckBus,	// Pass true. When an overridden bus is found, it is set to false.
	bool & io_bSolo,	// Pass false. Bit is OR'ed against each node of the signal flow.
	bool & io_bMute		// Pass false. Bit is OR'ed against each node of the signal flow.
	)
{
	io_bSolo = io_bSolo || m_bIsSoloed;
	io_bMute = io_bMute || m_bIsMuted;
	if ( io_bMute )	// Mute wins. Bail out.
		return;

	// Ask parent of the actor-mixer hierarchy. If this node has its bus overriden, or if one of our
	// descendents already had its bus overriden, do not check our parent's bus.
	if ( m_pParentNode )
		m_pParentNode->GetMonitoringMuteSoloState( in_bCheckBus && !m_pBusOutputNode, io_bSolo, io_bMute );

	// First overriden bus?
	if ( in_bCheckBus && m_pBusOutputNode )
	{
		// Yes. Check master-mixer hierarchy.
		m_pBusOutputNode->GetMonitoringMuteSoloState( true, io_bSolo, io_bMute );
	}
}

#endif

////////////////////////////////////
// AkStateGroupChunk
////////////////////////////////////

CAkState* AkStateGroupChunk::GetState( AkStateID in_StateTypeID )
{
	AkStateLink* l_pStateLink = m_mapStates.Exists( in_StateTypeID );
	if( l_pStateLink )
	{
		return ( *l_pStateLink ).pState;
	}

	return NULL;
}

AKRESULT AkStateGroupChunk::AddState(AkUniqueID in_ulStateInstanceID, AkStateID in_ulStateID, bool in_bNotify)
{
	CAkState* pState = g_pIndex->m_idxCustomStates.GetPtrAndAddRef( in_ulStateInstanceID );

	AkStateLink* l_pStateLink = m_mapStates.Exists( in_ulStateID );
	if( l_pStateLink )
	{
		if( pState == l_pStateLink->pState )// If we are replacing it by the exact same state.
		{
			// Nothing to Do.
			if( pState )
				pState->Release();

			return AK_Success;
		}

		// Remove the existing one.
		l_pStateLink->pState->TermNotificationSystem();
		l_pStateLink->pState->Release();

		m_mapStates.Unset( in_ulStateID );
	}

	AKASSERT( in_ulStateID );

	if(!pState)
	{
		return AK_InvalidInstanceID;
	}

	AkStateLink Link;
	Link.pState = pState;
	Link.ulStateID = in_ulStateInstanceID;

	if ( m_mapStates.Set( in_ulStateID, Link ) )
	{
		pState->InitNotificationSystem( m_pOwner );

		if (in_bNotify)
			m_pOwner->NotifyStateParametersModified();
		return AK_Success;
	}
	else
	{
		pState->Release();
		return AK_InsufficientMemory;
	}
}

void AkStateGroupChunk::RemoveState(
		AkStateID in_ulStateID //SwitchState
		)
{
	AkStateLink* l_pStateLink = m_mapStates.Exists(in_ulStateID);
	if( !l_pStateLink )
		return;

	l_pStateLink->pState->TermNotificationSystem();
	l_pStateLink->pState->Release();

	m_mapStates.Unset( in_ulStateID );
}

void AkStateGroupChunk::FlushStateTransitions()
{
	for ( AkStateValues::Iterator it = m_values.Begin(), itEnd = m_values.End(); it != itEnd; ++it )
	{
		if ( it.pValue->pTransition )
		{
			g_pTransitionManager->RemoveTransitionUser( it.pValue->pTransition, this );
			it.pValue->pTransition = NULL;
			m_pOwner->DecrementActivityCount();
		}
	}
}

//====================================================================================================
// this is where transitions report changes
//====================================================================================================
void AkStateGroupChunk::TransUpdateValue(AkIntPtr in_eTarget, AkReal32 in_fValue, bool in_bIsTerminated)
{
	AkPropID ePropID = (AkPropID) in_eTarget;

	AkStateValue * pChunkValue = m_values.FindProp( ePropID );
	if( pChunkValue )
	{
		pChunkValue->fValue = in_fValue;

		// if transition is done clear the pointer
		if( in_bIsTerminated && pChunkValue->pTransition )
		{
			m_pOwner->DecrementActivityCount();
			pChunkValue->pTransition = NULL;
		}
	}

	m_pOwner->RecalcNotification();
}
