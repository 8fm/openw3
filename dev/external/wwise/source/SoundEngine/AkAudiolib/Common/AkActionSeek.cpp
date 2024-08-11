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
// AkActionActive.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkActionSeek.h"
#include "AkAudioLibIndex.h"
#include "AkParameterNodeBase.h"
#include "AkBus.h"
#include "AkAudioMgr.h"
#include "AkModifiers.h"

extern CAkAudioMgr* g_pAudioMgr;

CAkActionSeek::CAkActionSeek(AkActionType in_eActionType, AkUniqueID in_ulID) 
: CAkActionExcept(in_eActionType, in_ulID)
, m_bIsSeekRelativeToDuration( false )
, m_bSnapToNearestMarker( false )
{
}

CAkActionSeek::~CAkActionSeek()
{
}

CAkActionSeek* CAkActionSeek::Create( AkActionType in_eActionType, AkUniqueID in_ulID )
{
	CAkActionSeek* pActionSeek = AkNew( g_DefaultPoolId, CAkActionSeek( in_eActionType, in_ulID ) );
	if( pActionSeek )
	{
		if( pActionSeek->Init() != AK_Success )
		{
			pActionSeek->Release();
			pActionSeek = NULL;
		}
	}
	return pActionSeek;
}

void CAkActionSeek::SetSeekToNearestMarker( bool in_bSeekToNearestMarker )
{
	m_bSnapToNearestMarker = in_bSeekToNearestMarker;
}

void CAkActionSeek::SetSeekPositionTimeAbsolute( AkTimeMs in_position, AkTimeMs in_rangeMin, AkTimeMs in_rangeMax )
{
	m_bIsSeekRelativeToDuration = false;
	// Note: Need to store absolute time values in floats.
	RandomizerModifier::SetModValue( m_position, (AkReal32)in_position, (AkReal32)in_rangeMin, (AkReal32)in_rangeMax );
}

void CAkActionSeek::SetSeekPositionPercent( AkReal32 in_position, AkReal32 in_rangeMin, AkReal32 in_rangeMax )
{
	m_bIsSeekRelativeToDuration = true;
	RandomizerModifier::SetModValue( m_position, in_position, in_rangeMin, in_rangeMax );
}

AKRESULT CAkActionSeek::SetActionParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	m_bIsSeekRelativeToDuration = READBANKDATA(AkUInt8, io_rpData, io_rulDataSize ) > 0;

	AkReal32 fSeekValue = READBANKDATA(AkReal32 , io_rpData, io_rulDataSize );
	AkReal32 fSeekValueMin = READBANKDATA(AkReal32 , io_rpData, io_rulDataSize );
	AkReal32 fSeekValueMax = READBANKDATA(AkReal32 , io_rpData, io_rulDataSize );
	RandomizerModifier::SetModValue( m_position, fSeekValue, fSeekValueMin, fSeekValueMax );

	AkUInt8 uSnaptoMarker = READBANKDATA(AkUInt8, io_rpData, io_rulDataSize );
	m_bSnapToNearestMarker = ( uSnaptoMarker > 0 );

	return SetExceptParams( io_rpData, io_rulDataSize );
}

//Execute the Action
//Must be called only by the audiothread
//
// Return - AKRESULT - AK_Success if all succeeded
AKRESULT CAkActionSeek::Execute(
	AkPendingAction * in_pAction
	)
{
	AKASSERT(g_pAudioMgr);

	AKRESULT eResult = AK_Success;
	
	CAkRegisteredObj * pGameObj = in_pAction->GameObj();

	switch(CAkAction::ActionType())
	{
		case AkActionType_Seek_E_O:
		case AkActionType_Seek_E:
			eResult = Exec( pGameObj, in_pAction->TargetPlayingID );
			break;

		case AkActionType_Seek_ALL_O:
		case AkActionType_Seek_ALL:
			AllExec( pGameObj, in_pAction->TargetPlayingID );
			break;

		case AkActionType_Seek_AE:
		case AkActionType_Seek_AE_O:
			AllExecExcept( pGameObj, in_pAction->TargetPlayingID );
			break;
		default:
			AKASSERT(!"Should not happen, unsupported condition");
			break;
	}

	return eResult;
}

AKRESULT CAkActionSeek::Exec( CAkRegisteredObj * in_pGameObj, AkPlayingID in_TargetPlayingID )
{
	CAkParameterNodeBase* pNode = GetAndRefTarget();
	if(pNode)
	{
		SeekActionParams l_Params;
		l_Params.bIsFromBus = false;
		l_Params.bIsMasterResume = false;
		l_Params.transParams.eFadeCurve = AkCurveInterpolation_Linear;
		l_Params.pGameObj = in_pGameObj;
		l_Params.playingID = in_TargetPlayingID;
		l_Params.transParams.TransitionTime = 0;
		l_Params.bIsMasterCall = false;
		l_Params.bIsSeekRelativeToDuration = m_bIsSeekRelativeToDuration;
		if ( l_Params.bIsSeekRelativeToDuration )
			l_Params.fSeekPercent = RandomizerModifier::GetModValue( m_position );
		else
			l_Params.iSeekTime = (AkTimeMs)RandomizerModifier::GetModValue( m_position );
		l_Params.bSnapToNearestMarker = m_bSnapToNearestMarker;
		l_Params.eType = ActionParamType_Seek;

		AKRESULT eResult = pNode->ExecuteAction( l_Params );

		pNode->Release();

		return eResult;
	}
	else
	{
		return AK_IDNotFound;
	}
}

void CAkActionSeek::AllExec( CAkRegisteredObj * in_pGameObj, AkPlayingID in_TargetPlayingID )
{
	// Nothing particular to do for dynamic sequences: cannot seek in previous
	// or next sequence item.
	
	SeekActionParams l_Params;
	l_Params.bIsFromBus = false;
	l_Params.bIsMasterResume = false;
	l_Params.transParams.eFadeCurve = (AkCurveInterpolation)m_eFadeCurve;
	l_Params.pGameObj = in_pGameObj;
	l_Params.playingID = in_TargetPlayingID;
	l_Params.transParams.TransitionTime = 0;
	l_Params.bIsSeekRelativeToDuration = m_bIsSeekRelativeToDuration;
	if ( l_Params.bIsSeekRelativeToDuration )
		l_Params.fSeekPercent = RandomizerModifier::GetModValue( m_position );
	else
		l_Params.iSeekTime = (AkTimeMs)RandomizerModifier::GetModValue( m_position );
	l_Params.bSnapToNearestMarker = m_bSnapToNearestMarker;
	l_Params.eType = ActionParamType_Seek;
	l_Params.bIsMasterCall = ( l_Params.pGameObj == NULL );

	CAkBus::ExecuteMasterBusAction(l_Params);
}

void CAkActionSeek::AllExecExcept( CAkRegisteredObj * in_pGameObj, AkPlayingID in_TargetPlayingID )
{
	SeekActionParamsExcept l_Params;
	l_Params.bIsFromBus = false;
	l_Params.bIsMasterResume = false;
    l_Params.transParams.eFadeCurve = (AkCurveInterpolation)m_eFadeCurve;
	l_Params.pGameObj = in_pGameObj;
	l_Params.playingID = in_TargetPlayingID;
	l_Params.transParams.TransitionTime = 0;
	l_Params.bIsSeekRelativeToDuration = m_bIsSeekRelativeToDuration;
	if ( l_Params.bIsSeekRelativeToDuration )
		l_Params.fSeekPercent = RandomizerModifier::GetModValue( m_position );
	else
		l_Params.iSeekTime = (AkTimeMs)RandomizerModifier::GetModValue( m_position );
	l_Params.bSnapToNearestMarker = m_bSnapToNearestMarker;
	l_Params.eType = ActionParamType_Seek;
	l_Params.pExeceptionList = &m_listElementException;

	CAkBus::ExecuteMasterBusActionExcept(l_Params);
}

