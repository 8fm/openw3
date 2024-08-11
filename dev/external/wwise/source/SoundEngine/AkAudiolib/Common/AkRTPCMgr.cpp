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
// AkRTPCMgr.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkRTPCMgr.h"
#include "AkSwitchCntr.h"
#include "AkDefault3DParams.h"
#include "AkMonitor.h"
#include "AkEvent.h"
#include "AkActionSetSwitch.h"
#include "AkAudioLib.h"
#include "AkRegisteredObj.h"
#include "AkLayer.h"
#include "AkLayerCntr.h"
#include "AkPBI.h"
#include "AkTransitionManager.h"

#define MIN_SIZE_ENTRIES_LOW 16
#define MAX_SIZE_ENTRIES AK_NO_MAX_LIST_SIZE

#define DEFAULT_RTPC_VALUE 0
#define DEFAULT_SWITCH_TYPE 0

inline AkUIntPtr AkHash( CAkRegisteredObj *  in_obj ) { return (AkUIntPtr) in_obj; }


// CAkRTPCTransition

CAkRTPCMgr::CAkRTPCTransition::CAkRTPCTransition( AkRTPCEntry * in_pOwner, CAkRegisteredObj * in_pGameObject ) 
	: m_pTransition ( NULL )
	, m_pOwner( in_pOwner )
	, m_pGameObject( in_pGameObject )
	, m_bRemoveEntryWhenDone( false )
{
}

CAkRTPCMgr::CAkRTPCTransition::~CAkRTPCTransition()
{
	if ( m_pTransition )
		g_pTransitionManager->RemoveTransitionUser( m_pTransition, this );
}

void CAkRTPCMgr::CAkRTPCTransition::TransUpdateValue(AkIntPtr, AkReal32 in_unionValue, bool in_bIsTerminated)
{
	bool bRemoveEntry = m_bRemoveEntryWhenDone && in_bIsTerminated;
	// Note: We could have cached the value entry to avoid looking it up at each frame, but since they
	// are stored in a sorted key array, we can't...
	AkRTPCValue * pValueEntry = m_pOwner->ValueExists( m_pGameObject );
	m_pOwner->ApplyRTPCValue( pValueEntry, in_unionValue, m_pGameObject, bRemoveEntry );
	
	if ( in_bIsTerminated )
	{
		// Clean up transition.
		m_pOwner->transitions.Remove( this );
		m_pTransition = NULL;
		AkDelete( g_DefaultPoolId, this );
	}
}

AKRESULT CAkRTPCMgr::CAkRTPCTransition::Start( 
	AkReal32 in_fStartValue,
	AkReal32 in_fTargetValue,
	TransParams & in_transParams,
	bool in_bRemoveEntryWhenDone 
	)
{
	TransitionParameters params(
		this,
		TransTarget_RTPC,
		in_fStartValue,
		in_fTargetValue,
		in_transParams.TransitionTime,
		in_transParams.eFadeCurve,
		false,
		false );
	m_bRemoveEntryWhenDone = in_bRemoveEntryWhenDone;// Must be set _Before_ AddTransitionToList as AddTransitionToList can call update directly.

	m_pTransition = g_pTransitionManager->AddTransitionToList( params );

	return ( m_pTransition ) ? AK_Success : AK_Fail;
}

void CAkRTPCMgr::CAkRTPCTransition::Update(
	AkReal32 in_fNewTargetValue,
	TransParams & in_transParams,
	bool in_bRemoveEntryWhenDone 
	)
{
	g_pTransitionManager->ChangeParameter(
		m_pTransition,
		TransTarget_RTPC,
		in_fNewTargetValue,
		in_transParams.TransitionTime,
		in_transParams.eFadeCurve,
		AkValueMeaning_Default	// Absolute
		);

	// Update the object's behavior at the outcome of the transition. Think about a transitioning reset during a transitionning set...
	m_bRemoveEntryWhenDone = in_bRemoveEntryWhenDone;
}

AkReal32 CAkRTPCMgr::CAkRTPCTransition::GetTargetValue()
{ 
	AKASSERT( m_pTransition ); 
	return m_pTransition->GetTargetValue(); 
}


// AkRTPCEntry

CAkRTPCMgr::AkRTPCEntry::~AkRTPCEntry()
{
	RemoveGameObject( NULL );
	values.Term();
	subscriptions.Term();
}

// Remove RTPC value for specified game object.
// Important: If in_GameObj is NULL, ALL values are removed.
// IMPORTANT: Subscribers are not notified.
void CAkRTPCMgr::AkRTPCEntry::RemoveGameObject( CAkRegisteredObj * in_pGameObj )
{
	if ( in_pGameObj == NULL )
	{
		AkRTPCTransitions::IteratorEx it = transitions.BeginEx();
		while ( it != transitions.End() )
		{
			it = DestroyTransition( it );
		}
		values.RemoveAll();
	}
	else
	{
		AkRTPCTransitions::IteratorEx it;
		FindTransition( in_pGameObj, it );
		if ( it != transitions.End() )
		{
			it = DestroyTransition( it );
		}

		values.Unset( in_pGameObj );
	}
}

// Helpers.

// Returns valid iterator if found. Otherwise, iterator equals transitions.End(). 
void CAkRTPCMgr::AkRTPCEntry::FindTransition( CAkRegisteredObj * in_pGameObj, CAkRTPCMgr::AkRTPCTransitions::IteratorEx & out_iter )
{
	out_iter = transitions.BeginEx();
	while ( out_iter != transitions.End() )
	{
		if ( (*out_iter)->GameObject() == in_pGameObj )
			return;
		++out_iter;
	}
}

void CAkRTPCMgr::AkRTPCEntry::GetRTPCExceptions( GameObjExceptArray& io_ExceptArrayObj )
{
	io_ExceptArrayObj.Reserve( values.Length() );
	for( AkRTPCValues::Iterator it = values.Begin(), itEnd = values.End(); it != itEnd; ++it )
	{
		if( (*it).pGameObj != NULL )
			io_ExceptArrayObj.AddLast( (*it).pGameObj );
	}
}

CAkRTPCMgr::CAkRTPCMgr()
{
}

CAkRTPCMgr::~CAkRTPCMgr()
{
}

AKRESULT CAkRTPCMgr::Init()
{
	m_SwitchEntries.Init( g_DefaultPoolId );
	m_RTPCEntries.Init();
	m_RTPCSubscribers.Init();

	AKRESULT eResult = m_listSwitchSubscribers.Init( MIN_SIZE_ENTRIES_LOW, MAX_SIZE_ENTRIES );
	if( eResult == AK_Success )
	{
		eResult = m_listRTPCSwitch.Init( 0, MAX_SIZE_ENTRIES );
	}
	
	//something went wrong, Terms the lists and return the error code
	if(eResult != AK_Success)
	{
		Term();
	}

	return eResult;
}

AKRESULT CAkRTPCMgr::Term()
{
	for( AkMapRTPCEntries::IteratorEx it = m_RTPCEntries.BeginEx(); it != m_RTPCEntries.End(); )
	{
		AkRTPCEntry * pEntry = *it;
		it = m_RTPCEntries.Erase( it );

		AkDelete( g_DefaultPoolId, pEntry );
	}

	for( AkMapRTPCSubscribers::IteratorEx it = m_RTPCSubscribers.BeginEx(); it != m_RTPCSubscribers.End(); )
	{
		AkRTPCSubscription * pSubscription = *it;
		it = m_RTPCSubscribers.Erase( it );

		RemoveReferencesToSubscription( pSubscription );
		AkDelete( g_DefaultPoolId, pSubscription );
	}

	if ( m_listRTPCSwitch.IsInitialized() )
	{
		for( AkListRTPCSwitch::Iterator iter = m_listRTPCSwitch.Begin(); iter != m_listRTPCSwitch.End(); ++iter )
		{
			(*iter).listRTPCSwitchSubscribers.Term();
			(*iter).ConversionTable.Unset();
		}
	}

	m_SwitchEntries.Term();
	m_RTPCEntries.Term();
	m_RTPCSubscribers.Term();
	m_listSwitchSubscribers.Term();
	m_listRTPCSwitch.Term();

	return AK_Success;
}

AKRESULT CAkRTPCMgr::AddSwitchRTPC(
		AkSwitchGroupID				in_switchGroup, 
		AkRtpcID					in_RTPC_ID,
		AkRTPCGraphPointInteger*	in_pArrayConversion,//NULL if none
		AkUInt32						in_ulConversionArraySize//0 if none
		)
{
	AKRESULT eResult = AK_Success;

	for( AkListRTPCSwitch::Iterator iter = m_listRTPCSwitch.Begin(); iter != m_listRTPCSwitch.End(); ++iter )
	{
		if( (*iter).switchGroup == in_switchGroup )
		{
			//Here replace its content
			//Unset before setting here
			//then return directly

			(*iter).RTPC_ID = in_RTPC_ID;
			(*iter).ConversionTable.Unset();
			return (*iter).ConversionTable.Set( in_pArrayConversion, in_ulConversionArraySize, AkCurveScaling_None );
		}
	}

	//Was not found, then add it
	AkRTPCSwitchAssociation* p_switchLink = m_listRTPCSwitch.AddLast();
	if( p_switchLink )
	{
		p_switchLink->switchGroup = in_switchGroup;
		p_switchLink->RTPC_ID = in_RTPC_ID;
		eResult = p_switchLink->ConversionTable.Set( in_pArrayConversion, in_ulConversionArraySize, AkCurveScaling_None );
		if( eResult == AK_Success )
		{
			eResult = p_switchLink->listRTPCSwitchSubscribers.Init( 0, AK_NO_MAX_LIST_SIZE );
			if( eResult != AK_Success )
			{
				p_switchLink->ConversionTable.Unset();
				RemoveSwitchRTPC( in_switchGroup );
			}
		}
		else
		{
			RemoveSwitchRTPC( in_switchGroup );
		}
	}
	else
	{
		eResult = AK_Fail;
	}

	//Here, all switch that were on this switch must be redirected to RTPC subscription
	AkListSwitchSubscribers::IteratorEx iterEx = m_listSwitchSubscribers.BeginEx();
	while( iterEx != m_listSwitchSubscribers.End() )
    {
		if( (*iterEx).switchGroup == in_switchGroup )
		{
			CAkSwitchAware* pSwCntr = (*iterEx).pSwitch;
			iterEx = m_listSwitchSubscribers.Erase( iterEx );

			SubscribeSwitch( pSwCntr, in_switchGroup );
		}
		else
		{
			++iterEx;
		}
	}

	return eResult;
	
}

AKRESULT CAkRTPCMgr::RegisterLayer( CAkLayer* in_pLayer, AkRtpcID in_rtpcID )
{
	return SubscribeRTPC(in_pLayer, in_rtpcID, RTPC_MaxNumRTPC, 0, AkCurveScaling_None, NULL, 0, NULL, SubscriberType_CAkLayer);
}

void CAkRTPCMgr::UnregisterLayer( CAkLayer* in_pLayer, AkRtpcID in_rtpcID )
{
	UnSubscribeRTPC(in_pLayer, RTPC_MaxNumRTPC);
}

void CAkRTPCMgr::RemoveSwitchRTPC( AkSwitchGroupID in_switchGroup )
{
	for( AkListRTPCSwitch::IteratorEx iter = m_listRTPCSwitch.BeginEx(); iter != m_listRTPCSwitch.End(); ++iter )
	{
		if( (*iter).switchGroup == in_switchGroup )
		{
			(*iter).ConversionTable.Unset();

			//Shovel them in the normal path
			for( AkListRTPCSwitchSubscribers::Iterator iterSubs =  (*iter).listRTPCSwitchSubscribers.Begin(); iterSubs != (*iter).listRTPCSwitchSubscribers.End(); ++iterSubs )
			{
				SubscribeSwitch( *iterSubs, in_switchGroup, true );
			}

			(*iter).listRTPCSwitchSubscribers.Term();
			m_listRTPCSwitch.Erase( iter );
			return;
		}
	}
}

AKRESULT CAkRTPCMgr::SetRTPCInternal( 
	AkRtpcID in_RTPCid, 
	AkReal32 in_Value, 
	CAkRegisteredObj * in_GameObj,
	TransParams & in_transParams,
	AkValueMeaning in_eValueMeaning		// AkValueMeaning_Independent (absolute) or AkValueMeaning_Offset (relative)
	)
{
	AkRTPCValue * pOldValue;

	AkRTPCEntry * pEntry = m_RTPCEntries.Exists( in_RTPCid );
	if ( pEntry )
	{
		pOldValue = pEntry->ValueExists( in_GameObj );
	}
	else
	{
		pOldValue = NULL;

		AkNew2( pEntry, g_DefaultPoolId, AkRTPCEntry, AkRTPCEntry( in_RTPCid ) );
		if ( !pEntry )
			return AK_Fail;

		m_RTPCEntries.Set( pEntry );
	}

	if ( in_eValueMeaning == AkValueMeaning_Offset )
	{
		// Relative change: compute new absolute value based on current _target_ value
		// (that is, the transition's target value if applicable, the current value otherwise).
		in_Value += pEntry->GetCurrentTargetValue( pOldValue );
	}

	return pEntry->SetRTPC( pOldValue, in_Value, in_GameObj, in_transParams, false );
}

AKRESULT CAkRTPCMgr::AkRTPCEntry::SetRTPC(
	AkRTPCValue * in_pValueEntry, // Pass in the value entry of the game object map. MUST be == ValueExists( in_GameObj ). NULL if there is none.
	AkReal32 in_Value, 
	CAkRegisteredObj * in_GameObj,
	TransParams & in_transParams,
	bool in_bUnsetWhenDone		// Remove game object's specific RTPC Value once value is set.
	)
{
	AKASSERT( in_pValueEntry == ValueExists( in_GameObj ) );

	bool bRequiresUpdate = ( !in_pValueEntry || in_pValueEntry->fValue != in_Value );

	if ( in_transParams.TransitionTime > 0 
		&& bRequiresUpdate )
	{
		// Requires a transition. Create or change transition target.
		AkReal32 fStartValue = GetCurrentValue( in_pValueEntry );

		// Create or modify transition. If a transition exists after returning from this function
		// (returns true), bail out: RTPC updates will be executed from the transition handler. 
		// If a transition does not exist, for any reason, proceed. We might want to set the value 
		// immediately, notify subscribers and/or unset the entry.
		if ( CreateOrModifyTransition( in_GameObj, fStartValue, in_Value, in_transParams, in_bUnsetWhenDone ) )
			return AK_Success;
	}
	else
	{
		// Does not require a transition, or no update is required. Destroy current transition if applicable.
		AkRTPCTransitions::IteratorEx itTrans;
		FindTransition( in_GameObj, itTrans );
		if ( itTrans != transitions.End() )
			DestroyTransition( itTrans );
	}

	if ( !bRequiresUpdate 
		&& !in_bUnsetWhenDone )
		return AK_Success;

	// If we did not bail out before, the RTPC value must be applied now. Otherwise, the transition handler will do it.
	// Also, pass in_bUnsetWhenDone directly so that the RTPC value gets unset at once.
	return ApplyRTPCValue( in_pValueEntry, in_Value, in_GameObj, in_bUnsetWhenDone );
}

// Returns the current target value if there is a transition, or the current value otherwise.
// Argument in_pValueEntry can be NULL: means that there is no specific value currently 
// assigned to the desired game object.
AkReal32 CAkRTPCMgr::AkRTPCEntry::GetCurrentTargetValue( AkRTPCValue * in_pValueEntry )
{
	if ( in_pValueEntry )
	{
		// An entry exists for this game object (which can be the global NULL game object).
		AkRTPCTransitions::IteratorEx itTrans;
		FindTransition( in_pValueEntry->pGameObj, itTrans );
		if ( itTrans != transitions.End() )
			return (*itTrans)->GetTargetValue();
	}
	// No transition found on this value entry. Return current (or default) value.
	return GetCurrentValue( in_pValueEntry );
}

// Returns the default value if current value does not exist.
// However, the global value is used instead if available.
AkReal32 CAkRTPCMgr::AkRTPCEntry::GetCurrentValue( AkRTPCValue * in_pValueEntry )
{
	AkReal32 fStartValue;
	if ( in_pValueEntry )
		fStartValue = in_pValueEntry->fValue;
	else
	{
		AkRTPCValue * pGlobalValue = ValueExists( NULL );
		if ( pGlobalValue )
			fStartValue = pGlobalValue->fValue;
		else
			fStartValue = fDefaultValue;	// this' default value
	}
	return fStartValue;
}

// Creates or modifies a game parameter transition. 
// Returns false if creation fails, or if no transition is required whatsoever (transition is 
// cleaned up inside). In such a case, the caller should apply the RTPC value immediately. 
// Otherwise, RTPC updates are handled by the transition.
bool CAkRTPCMgr::AkRTPCEntry::CreateOrModifyTransition(
	CAkRegisteredObj * in_pGameObj, 
	AkReal32 in_fStartValue,
	AkReal32 in_fTargetValue,
	TransParams & in_transParams,
	bool in_bRemoveEntryWhenDone
	)
{
	// Do not use a transition if target value equals start value. If one exists, destroy it.
	bool bRequiresTransition = ( in_fStartValue != in_fTargetValue );

	AkRTPCTransitions::IteratorEx itTrans;
	FindTransition( in_pGameObj, itTrans );
	if ( itTrans != transitions.End() )
	{
		// Transition exists. 
		if ( bRequiresTransition )
		{
			// A transition is required. Modify it.
			(*itTrans)->Update( in_fTargetValue, in_transParams, in_bRemoveEntryWhenDone );
			return true;
		}
		else
		{
			// Transition is not required. Destroy it.
			DestroyTransition( itTrans );
		}			
	}
	else
	{
		// Transition does not exist. Create one if required.
		if ( bRequiresTransition )
		{
			// Create transition. Store in_bRemoveEntryWhenDone: the handler will unset the value when target is reached.
			CAkRTPCTransition * pTrRTPC = AkNew( g_DefaultPoolId, CAkRTPCTransition( this, in_pGameObj ) );
			if ( pTrRTPC )
			{
				if ( pTrRTPC->Start( 
						in_fStartValue,
						in_fTargetValue,
						in_transParams,
						in_bRemoveEntryWhenDone ) == AK_Success )
				{
					// Successfully created transition. RTPC updates will occur from the transition handler.
					transitions.AddFirst( pTrRTPC );
					return true;
				}
				else
				{
					// Calling Start which returned failure DID delete the CAkRTPCTransition "pTrRTPC" object already.
					// Accessing pTrRTPC is therefore illegal ( and unnecessary ).
				}
			}
		}
	}

	// Transition was not be created.
	return false;
}

// Notifies the subscribers and manages the map of values.
AKRESULT CAkRTPCMgr::AkRTPCEntry::ApplyRTPCValue( 
	AkRTPCValue * in_pValueEntry, 
	AkReal32 in_NewValue, 
	CAkRegisteredObj * in_GameObj,
	bool in_bUnsetValue		// If true, the game object's specific RTPC entry is removed instead of being stored.
	)
{
	NotifyRTPCChange( in_pValueEntry, in_NewValue, in_GameObj );

	// Unset value (reset)
	if ( in_bUnsetValue )
	{
		if ( in_pValueEntry )
			values.Unset( in_GameObj ); // WG-16464: needs to be done after notification
		return AK_Success;
	}

	// Set new value
	if( in_pValueEntry )
	{
		in_pValueEntry->fValue = in_NewValue;
		return AK_Success;
	}
	else
	{
		in_pValueEntry = values.Set( in_GameObj );
		if ( in_pValueEntry )
		{
			in_pValueEntry->fValue = in_NewValue;
			return AK_Success;
		}
		else
		{
			return AK_Fail;
		}
	}
}

void CAkRTPCMgr::AkRTPCEntry::NotifyRTPCChange( 
	AkRTPCValue * in_pValueEntry, 
	AkReal32 in_NewValue, 
	CAkRegisteredObj * in_GameObj )
{
	AkRtpcID idRTPC = key;

	GameObjExceptArray l_ExceptArray;
	bool l_bCheckedExcept = false;

	AkRTPCSubscription * pPrevSubs = NULL;

	for ( AkRTPCSubscriptions::Iterator it = subscriptions.Begin(), itEnd = subscriptions.End(); it != itEnd; ++it )
	{
		// Subscriptions array may contain the same subscription many times.
		if ( AK_EXPECT_FALSE(*it == pPrevSubs) )
			continue;
		pPrevSubs = *it;

		AkRTPCSubscription& rItem = **it;

		if( (in_GameObj == rItem.TargetGameObject) 
			|| (in_GameObj == NULL) 
			|| (rItem.TargetGameObject == NULL)
			)
		{
			AKASSERT( rItem.key.pSubscriber );
			AkReal32 l_value = 0;

			if( rItem.eType == SubscriberType_CAkParameterNodeBase || rItem.eType == SubscriberType_CAkBus )
			{
				// Consider only global RTPCs for busses
				if ( in_GameObj != NULL && rItem.eType == SubscriberType_CAkBus )
					continue;

				CAkParameterNodeBase* pNode = reinterpret_cast<CAkParameterNodeBase*>(rItem.key.pSubscriber);
				if ( pNode->IsActiveOrPlaying() 
					|| 
					(
						// On busses, we need to send notifies for Volume, LFE and Bypass even though no sound
						// is playing, as there might still be a bus in the lower engine (in the case of an effect tail,
						// for instance)

						// Technically, the IsActiveOrPlaying() check should have solved the problem on most platform, but on Wii and 3DS, 
						// special cases while we dont know when we are in the tail forces us to make the nitification anyway.
						rItem.eType == SubscriberType_CAkBus 
						&&
						( 
							( ( rItem.key.ParamID >= RTPC_BypassFX0 ) && ( rItem.key.ParamID <= RTPC_BypassAllFX ) )
							|| 
							rItem.key.ParamID == RTPC_Volume
							||
							rItem.key.ParamID == RTPC_BusVolume
							||
							rItem.key.ParamID == RTPC_Position_PAN_X_2D
							||
							rItem.key.ParamID == RTPC_Position_PAN_Y_2D
							||
							rItem.key.ParamID == RTPC_Positioning_Divergence_Center_PCT
						)
					) )
				{
					// Evaluate the subscription's curves that are based on idRTPC, add their values
					for( RTPCCurveArray::Iterator iterCurves = rItem.Curves.Begin(); iterCurves != rItem.Curves.End(); ++iterCurves )
					{
						if( iterCurves.pItem->RTPC_ID == idRTPC )
						{
							l_value += iterCurves.pItem->ConversionTable.Convert( in_NewValue );
						}
					}

					GameObjExceptArray* pExceptArray = NULL;

					// Build exception array, but not for busses
					if( in_GameObj == NULL && rItem.eType != SubscriberType_CAkBus )
					{
						// Ensure to check for exceptions only once since it is a painful process.
						if( l_bCheckedExcept == false )
						{
							GetRTPCExceptions( l_ExceptArray );
							l_bCheckedExcept = true;
						}

						pExceptArray = &l_ExceptArray;
					}

					pNode->SetParamComplexFromRTPCManager(
						(void *) &rItem,
						rItem.key.ParamID,
						idRTPC,
						l_value,
						in_GameObj,
						pExceptArray
						);
				}
			}
			else if ( rItem.eType == SubscriberType_IAkRTPCSubscriber )
			{
				// For each subscriber, the rule is:
				//   * If the game object is the one you registered for (either NULL or a specific object), then
				//     this is for you
				//   * If not, then if the game object is NULL (aka global RTPC), and *your* game object
				//     doesn't have a value for this RTPC, then this is for you too.
				if ( (in_GameObj == rItem.TargetGameObject) || ( in_GameObj == NULL && !values.Exists( rItem.TargetGameObject ) ) )
				{
					// Evaluate the subscription's curves that are based on idRTPC, add their values
					// NOTE: Right now these objects support only one RTPC curve on each parameter. If they
					// ever support multiple curves, they will also probably support additive RTPC, in which
					// case this code will need to be modified to work in this new context.
					AKASSERT( rItem.Curves.Length() == 1 && (rItem.Curves.Begin()).pItem->RTPC_ID == idRTPC );
					l_value = (rItem.Curves.Begin()).pItem->ConversionTable.Convert( in_NewValue );

					// Notify the registered user of the change
					reinterpret_cast<IAkRTPCSubscriber*>( rItem.key.pSubscriber )->SetParam( static_cast<AkPluginParamID>(rItem.key.ParamID), &l_value, sizeof(l_value) );
				}
			}
			else if( rItem.eType == SubscriberType_PBI )
			{
				// For each subscriber, the rule is:
				//   * If the game object is the one you registered for (either NULL or a specific object), then
				//     this is for you
				//   * If not, then if the game object is NULL (aka global RTPC), and *your* game object
				//     doesn't have a value for this RTPC, then this is for you too.
				if ( (in_GameObj == rItem.TargetGameObject) || ( in_GameObj == NULL && !values.Exists( rItem.TargetGameObject ) ) )
				{
					for( RTPCCurveArray::Iterator iterCurves = rItem.Curves.Begin(); iterCurves != rItem.Curves.End(); ++iterCurves )
					{
						if( iterCurves.pItem->RTPC_ID == idRTPC )
						{
							l_value += iterCurves.pItem->ConversionTable.Convert( in_NewValue );
						}
					}

					// Notify the registered user of the change
					static_cast<CAkPBI*>( rItem.key.pSubscriber )->SetParam( static_cast<AkPluginParamID>(rItem.key.ParamID), &l_value, sizeof(l_value) );
				}
			}
			else
			{
				AKASSERT( rItem.eType == SubscriberType_CAkLayer );

				CAkLayer* pLayer = reinterpret_cast<CAkLayer*>(rItem.key.pSubscriber);
				const CAkLayerCntr * pOwner = pLayer->GetOwner();

				if (AK_EXPECT_FALSE(pOwner && pOwner->IsPlaying()))
				{
					if( pLayer->IsPlaying() )
					{
						// Evaluate the subscription's curves that are based on idRTPC, add their values
						for( RTPCCurveArray::Iterator iterCurves = rItem.Curves.Begin(); iterCurves != rItem.Curves.End(); ++iterCurves )
						{
							if( iterCurves.pItem->RTPC_ID == idRTPC )
							{
								l_value += iterCurves.pItem->ConversionTable.Convert( in_NewValue );
							}
						}

						if( in_GameObj == NULL )
						{
							// Ensure to check for exceptions only once since it is a painful process.
							if( l_bCheckedExcept == false )
							{
								GetRTPCExceptions( l_ExceptArray );
								l_bCheckedExcept = true;
							}
						}

						if (rItem.key.ParamID == RTPC_MaxNumRTPC)	//Used for the crossfade RTPC of Blend containers
							pLayer->OnRTPCChanged( in_GameObj, in_NewValue );
						else
							pLayer->SetParamComplexFromRTPCManager(
								(void *) &rItem,
								rItem.key.ParamID,
								idRTPC,
								l_value,
								in_GameObj,
								&l_ExceptArray
								);
					}
				}
			}
		}
	}

	l_ExceptArray.Term();

	g_pRTPCMgr->NotifyClientRTPCChange( idRTPC, in_pValueEntry, in_NewValue, in_GameObj );
}

void CAkRTPCMgr::NotifyClientRTPCChange(
	AkRtpcID in_idRTPC,
	AkRTPCValue * in_pValueEntry, 
	AkReal32 in_NewValue, 
	CAkRegisteredObj * in_GameObj )
{
	if( in_GameObj != NULL )
	{
		for( AkListRTPCSwitch::Iterator iter = m_listRTPCSwitch.Begin(); iter != m_listRTPCSwitch.End(); ++iter )
		{
			AkRTPCSwitchAssociation& rItem = *iter;

			if( rItem.RTPC_ID == in_idRTPC )
			{
				AkUInt32 l_newValue = rItem.ConversionTable.Convert( in_NewValue );

				//If there were no previous value or if the new value gives a different result, proceed.
				if( !in_pValueEntry || rItem.ConversionTable.Convert( in_pValueEntry->fValue ) != l_newValue )
				{
					for( AkListRTPCSwitchSubscribers::Iterator iterSubs =  rItem.listRTPCSwitchSubscribers.Begin(); iterSubs != rItem.listRTPCSwitchSubscribers.End(); ++iterSubs )
					{
						(*iterSubs)->SetSwitch( (AkUInt32)l_newValue, in_GameObj );
					}
				}

				//TODO here; //+ Capture log RTPC value + RTPCID + SwitchSelected
				// There will be flooding problems here with this notifications, 
				// to be Discussed with PMs.
			}

		}
	}
}

void CAkRTPCMgr::RemoveReferencesToSubscription( AkRTPCSubscription * in_pSubscription )
{
	bool bFound = false;
	for( RTPCCurveArray::Iterator it = in_pSubscription->Curves.Begin(); it != in_pSubscription->Curves.End(); ++it )
	{
		AkRTPCEntry * pEntry = m_RTPCEntries.Exists( (*it).RTPC_ID );
		if ( pEntry )
		{
			pEntry->subscriptions.Unset( in_pSubscription );
			bFound = true;
		}

		(*it).ConversionTable.Unset();
	}
	in_pSubscription->Curves.RemoveAll();

	if (!bFound)
	{
		//The RTPC doesn't have any curves.  Search in the whole lot even if it's longer.
		AkMapRTPCEntries::Iterator it = m_RTPCEntries.Begin();
		for(; it != m_RTPCEntries.End(); ++it)
		{
			it.pItem->subscriptions.Unset(in_pSubscription);
		}
	}
}

AKRESULT CAkRTPCMgr::SetSwitchInternal( AkSwitchGroupID in_SwitchGroup, AkSwitchStateID in_SwitchState, CAkRegisteredObj *  in_GameObj )
{
	AkSwitchKey l_key;
	l_key.m_pGameObj = in_GameObj;
	l_key.m_SwitchGroup = in_SwitchGroup;
	MONITOR_SWITCHCHANGED( in_SwitchGroup, in_SwitchState, in_GameObj?in_GameObj->ID():AK_INVALID_GAME_OBJECT );

	AKRESULT eResult;
	AkSwitchStateID * pSwitchState = m_SwitchEntries.Set( l_key );
	if ( pSwitchState )
	{
		*pSwitchState = in_SwitchState;
		eResult = AK_Success;
	}
	else
	{
		eResult = AK_Fail;
	}

	for( AkListSwitchSubscribers::Iterator iter = m_listSwitchSubscribers.Begin(); iter != m_listSwitchSubscribers.End(); ++iter )
    {
		if( (*iter).switchGroup == in_SwitchGroup )
		{
			(*iter).pSwitch->SetSwitch( in_SwitchState, in_GameObj );
		}
	}

	return eResult;
}

AkSwitchStateID CAkRTPCMgr::GetSwitch( AkSwitchGroupID in_SwitchGroup, CAkRegisteredObj *  in_GameObj /*= NULL*/ )
{
	for( AkListRTPCSwitch::Iterator iter = m_listRTPCSwitch.Begin(); iter != m_listRTPCSwitch.End(); ++iter )
	{
		if( (*iter).switchGroup == in_SwitchGroup )
		{
			AkReal32 l_RTPCValue;
			bool bIsValueGameObjectSpecific;
			if (!GetRTPCValue((*iter).RTPC_ID, in_GameObj, l_RTPCValue, bIsValueGameObjectSpecific ))
				l_RTPCValue = GetDefaultValue((*iter).RTPC_ID);

			return static_cast<AkSwitchStateID>( (*iter).ConversionTable.Convert( l_RTPCValue ) );
		}
	}

	// Not found in RTPCSwitches, proceed normal way.

	AkSwitchKey l_key;
	l_key.m_pGameObj = in_GameObj;
	l_key.m_SwitchGroup = in_SwitchGroup;
	AkSwitchStateID* pSwitchState = m_SwitchEntries.Exists( l_key );

	AkSwitchStateID l_SwitchState = AK_DEFAULT_SWITCH_STATE;
	if(pSwitchState)
	{
		l_SwitchState = *pSwitchState;
	}
	else if( l_key.m_pGameObj != NULL )
	{
		l_key.m_pGameObj = NULL;
		pSwitchState = m_SwitchEntries.Exists( l_key );
		if(pSwitchState)
		{
			l_SwitchState = *pSwitchState;
		}
	}
	return l_SwitchState;
}

AkReal32 CAkRTPCMgr::GetRTPCConvertedValue(
	void *					in_pToken,
	CAkRegisteredObj *		in_GameObj,
	AkRtpcID				in_RTPCid
	)
{
	AkRTPCSubscription * pItem = (AkRTPCSubscription *) in_pToken;

	AkReal32 rtpcValue;
	bool bGameObjectSpecificValue;
	const bool bGotValue = ( in_RTPCid != AK_INVALID_RTPC_ID ) && GetRTPCValue( in_RTPCid, in_GameObj, rtpcValue, bGameObjectSpecificValue );

	AkReal32 fResult = 0;
	for( RTPCCurveArray::Iterator iterCurves = pItem->Curves.Begin(); iterCurves != pItem->Curves.End(); ++iterCurves )
	{
		if( in_RTPCid == AK_INVALID_RTPC_ID || iterCurves.pItem->RTPC_ID == in_RTPCid )
		{
			if ( ! bGotValue )
			{
				if ( in_RTPCid == AK_INVALID_RTPC_ID )
				{
					if ( ! GetRTPCValue( iterCurves.pItem->RTPC_ID, in_GameObj, rtpcValue, bGameObjectSpecificValue ) )
						rtpcValue = GetDefaultValue(iterCurves.pItem->RTPC_ID);
				}
				else
					rtpcValue = GetDefaultValue(iterCurves.pItem->RTPC_ID);
			}

			fResult += iterCurves.pItem->ConversionTable.Convert( rtpcValue );
		}
	}

	return fResult;
}

AkReal32 CAkRTPCMgr::GetRTPCConvertedValue(
		void*				in_pSubscriber,
		AkUInt32			in_ParamID,
		CAkRegisteredObj * 	in_GameObj
		)
{
	AkRTPCSubscriptionKey key;
	key.pSubscriber = in_pSubscriber;
	key.ParamID = (AkRTPC_ParameterID) in_ParamID;

	AkRTPCSubscription* pSubscription = m_RTPCSubscribers.Exists( key );
	if ( pSubscription )
		return GetRTPCConvertedValue( (void*) pSubscription, in_GameObj, AK_INVALID_RTPC_ID /* Process curves for all RTPC IDs */ );

	return DEFAULT_RTPC_VALUE;
}

bool CAkRTPCMgr::GetRTPCValue(
	AkRtpcID in_RTPC_ID,
	CAkRegisteredObj* in_GameObj,
	AkReal32& out_value,
	bool& out_bGameObjectSpecificValue
)
{
	AkRTPCEntry * pEntry = m_RTPCEntries.Exists( in_RTPC_ID );
	if ( pEntry )
	{
		AkRTPCValue * pValue = pEntry->ValueExists( in_GameObj );
		if ( pValue )
		{
			out_value = pValue->fValue;
			out_bGameObjectSpecificValue = ( in_GameObj != NULL );
			return true;
		}
		else if ( !pValue && in_GameObj != NULL )
		{
			out_bGameObjectSpecificValue = false;
			pValue = pEntry->ValueExists( NULL );
			if ( pValue )
			{
				out_value = pValue->fValue;
				return true;
			}
		}
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////
// Subscription Functions
///////////////////////////////////////////////////////////////////////////

AKRESULT CAkRTPCMgr::SubscribeRTPC(
		void*						in_pSubscriber,
		AkRtpcID					in_RTPC_ID,
		AkRTPC_ParameterID			in_ParamID,		//# of the param that must be notified on change
		AkUniqueID					in_RTPCCurveID,
		AkCurveScaling				in_eScaling,
		AkRTPCGraphPoint*			in_pArrayConversion,
		AkUInt32					in_ulConversionArraySize,
		CAkRegisteredObj * 			in_TargetGameObject,
		SubscriberType				in_eType
		)
{
	AKASSERT(in_pSubscriber);

	//Suppose parameters are wrongs
	AKRESULT eResult = AK_InvalidParameter;

	if(in_pSubscriber)
	{
		AkRTPCSubscriptionKey key;
		key.pSubscriber = in_pSubscriber;
		key.ParamID = in_ParamID;

		AkRTPCSubscription* pSubscription = m_RTPCSubscribers.Exists( key );
		if ( pSubscription )
		{
			// Remove existing entry for the specified curve
			for( RTPCCurveArray::Iterator iterCurves = pSubscription->Curves.Begin(); iterCurves != pSubscription->Curves.End(); ++iterCurves )
			{
				if( iterCurves.pItem->RTPCCurveID == in_RTPCCurveID )
				{
					AkRTPCEntry * pEntry = m_RTPCEntries.Exists( iterCurves.pItem->RTPC_ID );
					if ( pEntry )
						pEntry->subscriptions.Unset( pSubscription );

					iterCurves.pItem->ConversionTable.Unset();
					pSubscription->Curves.Erase( iterCurves );
					break;
				}
			}
		}
		else
		{
			// Create a new subscription
			AkNew2( pSubscription, g_DefaultPoolId, AkRTPCSubscription, AkRTPCSubscription() );
			if ( ! pSubscription )
			{
				eResult = AK_InsufficientMemory;
			}
			else
			{
				pSubscription->key.pSubscriber = in_pSubscriber;
				pSubscription->key.ParamID = in_ParamID;
				pSubscription->eType = in_eType;
				pSubscription->TargetGameObject = in_TargetGameObject;
				m_RTPCSubscribers.Set( pSubscription );
			}
		}

		if ( pSubscription )
		{
			if( in_pArrayConversion && in_ulConversionArraySize )
			{
				RTPCCurve* pNewCurve = pSubscription->Curves.AddLast();
				if ( ! pNewCurve )
				{
					eResult = AK_InsufficientMemory;
				}
				else
				{
					pNewCurve->RTPC_ID = in_RTPC_ID;
					pNewCurve->RTPCCurveID = in_RTPCCurveID;
					eResult = pNewCurve->ConversionTable.Set( in_pArrayConversion, in_ulConversionArraySize, in_eScaling );
					if ( eResult != AK_Success )
						pSubscription->Curves.RemoveLast();
				}
			}
			else if (in_eType == SubscriberType_CAkLayer && in_ParamID == RTPC_MaxNumRTPC)	//This case is for the RTPC controlling the crossfade in blends.
				eResult = AK_Success;

			if ( eResult == AK_Success )
			{
				AkRTPCEntry * pEntry = GetRTPCEntry( in_RTPC_ID );
				if ( pEntry )
					pEntry->subscriptions.Add( pSubscription );
				else
					eResult = AK_InsufficientMemory;
			}
		}

		if( eResult == AK_Success )
		{
			eResult = UpdateRTPCSubscriberInfo( in_pSubscriber );
		}
		else if ( pSubscription && pSubscription->Curves.IsEmpty() )
		{
			m_RTPCSubscribers.Unset( key );
			RemoveReferencesToSubscription( pSubscription );
			AkDelete( g_DefaultPoolId, pSubscription );
		}
	}

	return eResult;
}

AKRESULT CAkRTPCMgr::SubscribeSwitch(
		CAkSwitchAware*		in_pSwitch,
		AkSwitchGroupID		in_switchGroup,
		bool				in_bForceNoRTPC /*= false*/
		)
{
	AKASSERT(in_pSwitch);

	if( !in_bForceNoRTPC )
	{
		for( AkListRTPCSwitch::Iterator iter = m_listRTPCSwitch.Begin(); iter != m_listRTPCSwitch.End(); ++iter )
		{
			if( (*iter).switchGroup == in_switchGroup )
			{
				return (*iter).listRTPCSwitchSubscribers.AddLast( in_pSwitch ) ? AK_Success : AK_Fail;
			}
		}
	}

	//Suppose parameters are wrongs
	AKRESULT eResult = AK_InvalidParameter;

	if(in_pSwitch)
	{
		//First, remove the old version
		UnSubscribeSwitch( in_pSwitch );

		AkSwitchSubscription Item;
		Item.pSwitch = in_pSwitch;
		Item.switchGroup = in_switchGroup;

		eResult = m_listSwitchSubscribers.AddLast( Item ) ? AK_Success : AK_Fail;
	}

	return eResult;
}

void CAkRTPCMgr::UnSubscribeRTPC( void* in_pSubscriber, AkUInt32 in_ParamID, AkUniqueID in_RTPCCurveID, bool* out_bMoreCurvesRemaining /* = NULL */ )
{
	AkRTPCSubscriptionKey key;
	key.pSubscriber = in_pSubscriber;
	key.ParamID = (AkRTPC_ParameterID) in_ParamID;

	AkRTPCSubscription* pSubscription = m_RTPCSubscribers.Exists( key );
	if ( pSubscription )
	{
		if ( out_bMoreCurvesRemaining )
			*out_bMoreCurvesRemaining = !pSubscription->Curves.IsEmpty();

		for( RTPCCurveArray::Iterator iterCurves = pSubscription->Curves.Begin(); iterCurves != pSubscription->Curves.End(); ++iterCurves )
		{
			if( iterCurves.pItem->RTPCCurveID == in_RTPCCurveID )
			{
				AkRTPCEntry * pEntry = m_RTPCEntries.Exists( iterCurves.pItem->RTPC_ID );
				if ( pEntry )
					pEntry->subscriptions.Unset( pSubscription );

				iterCurves.pItem->ConversionTable.Unset();
				pSubscription->Curves.Erase( iterCurves );

				if ( pSubscription->Curves.IsEmpty() )
				{
					if ( out_bMoreCurvesRemaining )
						*out_bMoreCurvesRemaining = false;

					m_RTPCSubscribers.Unset( key );
					// Note: No need to remove references to subscription. We just did. 
					AkDelete( g_DefaultPoolId, pSubscription );
				}

				break;
			}
		}

		// Found the subscription but not the curve
		return;
	}

	if ( out_bMoreCurvesRemaining )
		*out_bMoreCurvesRemaining = false;
}

void CAkRTPCMgr::UnSubscribeRTPC( void* in_pSubscriber, AkUInt32 in_ParamID )
{
	AkRTPCSubscriptionKey key;
	key.pSubscriber = in_pSubscriber;
	key.ParamID = (AkRTPC_ParameterID) in_ParamID;

	AkRTPCSubscription* pSubscription = m_RTPCSubscribers.Exists( key );
	if ( pSubscription )
	{
		m_RTPCSubscribers.Unset( key );
		RemoveReferencesToSubscription( pSubscription );
		AkDelete( g_DefaultPoolId, pSubscription );
	}
}

void CAkRTPCMgr::UnSubscribeRTPC( void* in_pSubscriber )
{
	AkMapRTPCSubscribers::IteratorEx iterRTPCSubs = m_RTPCSubscribers.BeginEx();
	while( iterRTPCSubs != m_RTPCSubscribers.End() )
    {
		AkRTPCSubscription * pSubscription = *iterRTPCSubs;

		if( pSubscription->key.pSubscriber == in_pSubscriber )
		{
			iterRTPCSubs = m_RTPCSubscribers.Erase( iterRTPCSubs );
			RemoveReferencesToSubscription( pSubscription );
			AkDelete( g_DefaultPoolId, pSubscription );
		}
		else
		{
			++iterRTPCSubs;
		}
	}
}

void CAkRTPCMgr::UnSubscribeSwitch( CAkSwitchAware* in_pSwitch )
{
	AkListSwitchSubscribers::IteratorEx iter = m_listSwitchSubscribers.BeginEx();
	while( iter != m_listSwitchSubscribers.End() )
    {
		AkSwitchSubscription& rItem = *iter;
		if( rItem.pSwitch == in_pSwitch )
		{
			iter = m_listSwitchSubscribers.Erase( iter );
			return;
		}
		else
		{
			++iter;
		}
	}

	for( AkListRTPCSwitch::Iterator iterRTPCSwitch = m_listRTPCSwitch.Begin(); iterRTPCSwitch != m_listRTPCSwitch.End(); ++iterRTPCSwitch )
    {
		AkRTPCSwitchAssociation& rItem = *iterRTPCSwitch;

		AkListRTPCSwitchSubscribers::IteratorEx iter2 = rItem.listRTPCSwitchSubscribers.BeginEx();
		while( iter2 != rItem.listRTPCSwitchSubscribers.End() )
		{
			if( (*iter2) == in_pSwitch )
			{
				iter2 = rItem.listRTPCSwitchSubscribers.Erase( iter2 );
				return;
			}
			else
			{
				++iter2;
			}
		}
	}
}

void CAkRTPCMgr::UnregisterGameObject(CAkRegisteredObj * in_GameObj)
{
	AKASSERT( in_GameObj != NULL );
	if( in_GameObj == NULL )
	{
		return;
	}

	//Clear the subscriber list
	AkMapRTPCSubscribers::IteratorEx iterRTPCSubs = m_RTPCSubscribers.BeginEx();
	while( iterRTPCSubs != m_RTPCSubscribers.End() )
    {
		AkRTPCSubscription * pSubscription = *iterRTPCSubs;

		if( pSubscription->TargetGameObject == in_GameObj )
		{
			iterRTPCSubs = m_RTPCSubscribers.Erase( iterRTPCSubs );
			RemoveReferencesToSubscription( pSubscription );
			AkDelete( g_DefaultPoolId, pSubscription );
		}
		else
		{
			++iterRTPCSubs;
		}
	}

	//Clear its specific switches
	AkMapSwitchEntries::IteratorEx iterSwitch = m_SwitchEntries.BeginEx();
	while( iterSwitch != m_SwitchEntries.End() )
	{
		if( (*iterSwitch).key.m_pGameObj == in_GameObj )
		{
			iterSwitch = m_SwitchEntries.Erase( iterSwitch );
		}
		else
		{
			++iterSwitch;
		}
	}

	//Clear its specific RTPCs
	for ( AkMapRTPCEntries::Iterator iterRTPC = m_RTPCEntries.Begin(); iterRTPC != m_RTPCEntries.End(); ++iterRTPC )
	{
		AkRTPCEntry * pEntry = *iterRTPC;
		pEntry->RemoveGameObject( in_GameObj );
	}
}

AKRESULT CAkRTPCMgr::UpdateRTPCSubscriberInfo( void* in_pSubscriber )
{
//TODO Alessard Add an option to avoid calling UpdateRTPCSubscriberInfo too many times on load banks when miltiple RTPC are available
//The solution is probably to add two parameters to this function, including paramID and GameObject(and bool if necessary)

	AKRESULT eResult = AK_Success;

	//get what he subscribed for

	for( AkMapRTPCSubscribers::Iterator iter = m_RTPCSubscribers.Begin(); iter != m_RTPCSubscribers.End(); ++iter )
	{
		if( (*iter)->key.pSubscriber == in_pSubscriber )
		{
			UpdateSubscription( **iter );
		}
	}

	return eResult;
}

void CAkRTPCMgr::UpdateSubscription( AkRTPCSubscription& in_rSubscription )
{
	if( in_rSubscription.eType == SubscriberType_CAkParameterNodeBase || in_rSubscription.eType == SubscriberType_CAkBus )
	{
		// Simply tell playing instances to recalc, way faster and way simplier than starting a serie of endless notifications
		reinterpret_cast<CAkParameterNodeBase*>(in_rSubscription.key.pSubscriber)->RecalcNotification();
	}
	else if ( in_rSubscription.eType == SubscriberType_IAkRTPCSubscriber )
	{
		AkReal32 l_RTPCValue = GetRTPCConvertedValue( &in_rSubscription, in_rSubscription.TargetGameObject, AK_INVALID_RTPC_ID );

		reinterpret_cast<IAkRTPCSubscriber*>( in_rSubscription.key.pSubscriber )->SetParam( 
				static_cast<AkPluginParamID>( in_rSubscription.key.ParamID ), 
				&l_RTPCValue,
				sizeof( l_RTPCValue )
				);
	}
	else if( in_rSubscription.eType == SubscriberType_PBI )
	{
		AkReal32 l_RTPCValue = GetRTPCConvertedValue( &in_rSubscription, in_rSubscription.TargetGameObject, AK_INVALID_RTPC_ID );

		static_cast<CAkPBI*>( in_rSubscription.key.pSubscriber )->SetParam( 
				static_cast<AkPluginParamID>( in_rSubscription.key.ParamID ), 
				&l_RTPCValue,
				sizeof( l_RTPCValue )
				);
	}
	else
	{
		AKASSERT( in_rSubscription.eType == SubscriberType_CAkLayer );

		// Simply tell playing instances to recalc, way faster and way simplier than starting a serie of endless notifications
		reinterpret_cast<CAkLayer*>(in_rSubscription.key.pSubscriber)->RecalcNotification();
	}
}

void CAkRTPCMgr::ResetSwitches( CAkRegisteredObj * in_GameObj )
{
	if( in_GameObj == NULL )
	{
		m_SwitchEntries.RemoveAll();
	}
	else
	{
		AkMapSwitchEntries::IteratorEx iterSwitch = m_SwitchEntries.BeginEx();
		while( iterSwitch != m_SwitchEntries.End() )
		{
			if( (*iterSwitch).key.m_pGameObj == in_GameObj )
			{
				iterSwitch = m_SwitchEntries.Erase( iterSwitch );	
			}
			else
			{
				++iterSwitch;
			}
		}
	}

	for( AkListSwitchSubscribers::Iterator iter = m_listSwitchSubscribers.Begin(); iter != m_listSwitchSubscribers.End(); ++iter )
    {
		AKASSERT( (*iter).pSwitch );
		(*iter).pSwitch->SetSwitch( DEFAULT_SWITCH_TYPE, in_GameObj );
	}

}

void CAkRTPCMgr::ResetRTPC( CAkRegisteredObj * in_GameObj )
{
	// Reset all values and then update everybody...
	for ( AkMapRTPCEntries::Iterator it = m_RTPCEntries.Begin(); it != m_RTPCEntries.End(); ++it )
		(*it)->RemoveGameObject( in_GameObj );

	for( AkMapRTPCSubscribers::Iterator iter = m_RTPCSubscribers.Begin(); iter != m_RTPCSubscribers.End(); ++iter )
	{
		UpdateSubscription( **iter );
	}

	for( AkListRTPCSwitch::Iterator iter = m_listRTPCSwitch.Begin(); iter != m_listRTPCSwitch.End(); ++iter )
    {
		for( AkListRTPCSwitchSubscribers::Iterator iter2 = (*iter).listRTPCSwitchSubscribers.Begin(); iter2 != (*iter).listRTPCSwitchSubscribers.End(); ++iter2 )
		{
			(*iter2)->SetSwitch( DEFAULT_SWITCH_TYPE, in_GameObj );
		}
	}
}

CAkRTPCMgr::AkRTPCEntry * CAkRTPCMgr::GetRTPCEntry( AkRtpcID in_RTPCid )
{
	AkRTPCEntry * pEntry = m_RTPCEntries.Exists( in_RTPCid );
	if ( pEntry )
		return pEntry;

	AkNew2( pEntry, g_DefaultPoolId, AkRTPCEntry, AkRTPCEntry( in_RTPCid ) );
	if ( !pEntry )
		return NULL;

	m_RTPCEntries.Set( pEntry );

	return pEntry;
}

void CAkRTPCMgr::SetDefaultParamValue( AkRtpcID in_RTPCid, AkReal32 in_fValue )
{
	AkRTPCEntry * pEntry = GetRTPCEntry( in_RTPCid );
	if ( pEntry )
		pEntry->fDefaultValue = in_fValue;
}

AkReal32 CAkRTPCMgr::GetDefaultValue( AkRtpcID in_RTPCid, bool* out_pbWasFound )
{
	AkRTPCEntry * pEntry = m_RTPCEntries.Exists( in_RTPCid );
	if( out_pbWasFound )
	{
		*out_pbWasFound = (pEntry != NULL);
	}
	return pEntry ? pEntry->fDefaultValue : 0.0f;
}

void CAkRTPCMgr::ResetRTPCValue(AkRtpcID in_RTPCid, CAkRegisteredObj * in_GameObj, TransParams & in_transParams)
{
	//Remove the local value of the parameter for this game object.  It will default back
	//to the default value automatically, if there is no global value.

	AkRTPCEntry * pEntry = m_RTPCEntries.Exists( in_RTPCid );
	if ( pEntry )
	{
		AkRTPCValue * pGlobalValue = pEntry->ValueExists( NULL );
		AkRTPCValue * pValue = pGlobalValue;

		AkReal32 fNewValue;

		if ( in_GameObj )
		{
			pValue = in_GameObj ? pEntry->ValueExists( in_GameObj ) : NULL;
			if ( !pValue ) // No previous value ? No change.
				return;

			if ( pGlobalValue )
				fNewValue = pGlobalValue->fValue;
			else
				fNewValue = pEntry->fDefaultValue;
		}
		else
		{
			if ( !pGlobalValue ) // No previous value ? No change.
				return;

			fNewValue = pEntry->fDefaultValue;
		}
	
		AKASSERT( in_GameObj || pGlobalValue == pValue );	// if reset occurs on global scope, pValue == pGlobalValue.

		AKVERIFY( pEntry->SetRTPC(
			pValue,
			fNewValue, 
			in_GameObj,
			in_transParams,
			true // Remove game object's specific RTPC Value once target value is reached.
			) == AK_Success || !"Reset RTPC must succeed" );
	}
}

