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
// AkAction.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkAction.h"
#include "AkAudioLibIndex.h"
#include "AkActions.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include "AkModifiers.h"
#ifndef AK_OPTIMIZED
#include "AkActionEvent.h"
#endif

CAkAction::CAkAction(AkActionType in_eActionType, AkUniqueID in_ulID)
	: CAkIndexable(in_ulID)
	, pNextLightItem(NULL)
	, m_ulElementID(0)
	, m_eActionType(in_eActionType)
	, m_eFadeCurve(AkCurveInterpolation_Linear)
	, m_bWasLoadedFromBank( false )
	, m_bIsBusElement( false )
{
	//MUST STAY EMPTY, use Init() instead.
}

CAkAction::~CAkAction()
{
}

CAkAction* CAkAction::Create(AkActionType in_eActionType, AkUniqueID in_ulID)
{
	switch( in_eActionType & ACTION_TYPE_ACTION )
	{
	case ACTION_TYPE_PLAY:
		return CAkActionPlay::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_STOP:
		return CAkActionStop::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_PAUSE:
		return CAkActionPause::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_RESUME:
		return CAkActionResume::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_SETPITCH:
	case ACTION_TYPE_RESETPITCH:
		return CAkActionSetAkProp::Create(in_eActionType, in_ulID, AkPropID_Pitch);

	case ACTION_TYPE_SETVOLUME:
	case ACTION_TYPE_RESETVOLUME:
		return CAkActionSetAkProp::Create(in_eActionType, in_ulID, AkPropID_Volume);

	case ACTION_TYPE_SETBUSVOLUME:
	case ACTION_TYPE_RESETBUSVOLUME:
		return CAkActionSetAkProp::Create(in_eActionType, in_ulID, AkPropID_BusVolume);

	case ACTION_TYPE_SETLPF:
	case ACTION_TYPE_RESETLPF:
		return CAkActionSetAkProp::Create(in_eActionType, in_ulID, AkPropID_LPF);

	case ACTION_TYPE_MUTE:
	case ACTION_TYPE_UNMUTE:
		return CAkActionMute::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_SETSTATE:
		return CAkActionSetState::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_SETGAMEPARAMETER:
	case ACTION_TYPE_RESETGAMEPARAMETER:
		return CAkActionSetGameParameter::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_USESTATE:
	case ACTION_TYPE_UNUSESTATE:
		return CAkActionUseState::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_SETSWITCH:
		return CAkActionSetSwitch::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_BYPASSFX:
	case ACTION_TYPE_RESETBYPASSFX:
		return CAkActionBypassFX::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_BREAK:
		return CAkActionBreak::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_TRIGGER:
		return CAkActionTrigger::Create( in_eActionType, in_ulID );

#ifndef AK_OPTIMIZED
	case ACTION_TYPE_STOPEVENT:
	case ACTION_TYPE_PAUSEEVENT:	
	case ACTION_TYPE_RESUMEEVENT:
		return CAkActionEvent::Create(in_eActionType, in_ulID);

#endif
		
	case ACTION_TYPE_SEEK:
		return CAkActionSeek::Create(in_eActionType, in_ulID);

	default:
		AKASSERT(!"Unknown Action Type");
		return NULL;
	}
}

void CAkAction::AddToIndex()
{
	AKASSERT(g_pIndex);
	if( ID() != AK_INVALID_UNIQUE_ID ) // Check if the action has an ID, it does not have an ID if was created by the sound engine
		g_pIndex->m_idxActions.SetIDToPtr( this );
}

void CAkAction::RemoveFromIndex()
{
	AKASSERT(g_pIndex);
	if( ID() != AK_INVALID_UNIQUE_ID ) // Check if the action has an ID, it does not have an ID if was created by the sound engine
		g_pIndex->m_idxActions.RemoveID( ID() );
}

AkUInt32 CAkAction::AddRef() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxActions.GetLock() ); 
    return ++m_lRef; 
} 

AkUInt32 CAkAction::Release() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxActions.GetLock() ); 
    AkInt32 lRef = --m_lRef; 
    AKASSERT( lRef >= 0 ); 
    if ( !lRef ) 
    { 
        RemoveFromIndex(); 
        AkDelete( g_DefaultPoolId, this ); 
    } 
    return lRef; 
}

void CAkAction::ActionType(AkActionType in_ActionType)
{
	AKASSERT( !"Unpredictable behavior to change the action type of this Action" );
	m_eActionType = in_ActionType;
}

void CAkAction::SetElementID( WwiseObjectIDext in_Id )
{
	m_ulElementID = in_Id.id;
	m_bIsBusElement = in_Id.bIsBus;
}

AKRESULT CAkAction::SetAkProp( AkPropID in_eProp, AkReal32 in_fValue, AkReal32 in_fMin, AkReal32 in_fMax )
{
	AKRESULT eResult = AK_Success;
	AkReal32 fProp = m_props.GetAkProp( in_eProp, g_AkPropDefault[ in_eProp ] ).fValue;
	if( fProp != in_fValue )
		eResult = m_props.SetAkProp( in_eProp, in_fValue );

	if( eResult == AK_Success && ( in_fMin || in_fMax || m_ranges.FindProp( in_eProp ) ) )
	{
		RANGED_MODIFIERS<AkPropValue> range;
		range.m_min.fValue = in_fMin;
		range.m_max.fValue = in_fMax;

		eResult = m_ranges.SetAkProp( in_eProp, range );
	}

	return eResult;
}

AKRESULT CAkAction::SetAkProp( AkPropID in_eProp, AkInt32 in_iValue, AkInt32 in_iMin, AkInt32 in_iMax )
{
	AKRESULT eResult = AK_Success;
	AkInt32 iProp = m_props.GetAkProp( in_eProp, g_AkPropDefault[ in_eProp ] ).iValue;
	if( iProp != in_iValue )
		eResult = m_props.SetAkProp( in_eProp, in_iValue );

	if(  eResult == AK_Success && ( in_iMin || in_iMax || m_ranges.FindProp( in_eProp ) ) )
	{
		RANGED_MODIFIERS<AkPropValue> range;
		range.m_min.iValue = in_iMin;
		range.m_max.iValue = in_iMax;

		eResult = m_ranges.SetAkProp( in_eProp, range );
	}

	return eResult;
}

AkTimeMs CAkAction::GetTransitionTime()
{
	AkTimeMs time = m_props.GetAkProp( AkPropID_TransitionTime, g_AkPropDefault[ AkPropID_TransitionTime ] ).iValue;
	ApplyRange<AkInt32>( AkPropID_TransitionTime, time );
	return time;
}

AkInt32 CAkAction::GetDelayTime()
{
	AkInt32 time = m_props.GetAkProp( AkPropID_DelayTime, g_AkPropDefault[ AkPropID_DelayTime ] ).iValue;
	ApplyRange<AkInt32>( AkPropID_DelayTime, time );
	return time;
}

AKRESULT CAkAction::SetInitialValues(AkUInt8* in_pData, AkUInt32 in_ulDataSize)
{
	AKRESULT eResult = AK_Success;

	// Read ID
	// We don't care about the ID, just skip it
	SKIPBANKDATA( AkUInt32, in_pData, in_ulDataSize);

	AkActionType ulActionType = (AkActionType) READBANKDATA( AkUInt16, in_pData, in_ulDataSize);
	AKASSERT(m_eActionType == ulActionType);

	{
		WwiseObjectIDext idExt;
		idExt.id = READBANKDATA(AkUInt32, in_pData, in_ulDataSize);
		idExt.bIsBus = READBANKDATA(AkUInt8, in_pData, in_ulDataSize) != 0;

		SetElementID( idExt );
	}

	// As this can be called upon an already-existing object.
	m_props.RemoveAll();
	m_ranges.RemoveAll();

	eResult = m_props.SetInitialParams( in_pData, in_ulDataSize );

	if( eResult == AK_Success )
		eResult = m_ranges.SetInitialParams( in_pData, in_ulDataSize );

	if( eResult == AK_Success )
		eResult = SetActionParams(in_pData, in_ulDataSize);

	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	// Fix sample-based properties:

	AkPropValue * pValue = m_props.FindProp( AkPropID_DelayTime );
	if ( pValue )
		pValue->iValue = AkTimeConv::MillisecondsToSamples( pValue->iValue );

	RANGED_MODIFIERS<AkPropValue> * pRange = m_ranges.FindProp( AkPropID_DelayTime );
	if ( pRange )
	{
		pRange->m_min.iValue = AkTimeConv::MillisecondsToSamples( pRange->m_min.iValue );
		pRange->m_max.iValue = AkTimeConv::MillisecondsToSamples( pRange->m_max.iValue );
	}

	return eResult;
}

AKRESULT CAkAction::SetActionParams(AkUInt8*&, AkUInt32& )
{
	return AK_Success;
}

AKRESULT CAkAction::SetActionSpecificParams(AkUInt8*& , AkUInt32& )
{
	return AK_Success;
}

CAkParameterNodeBase* CAkAction::GetAndRefTarget()
{
	return g_pIndex->GetNodePtrAndAddRef( m_ulElementID, IsBusElement() ? AkNodeType_Bus : AkNodeType_Default );
}
