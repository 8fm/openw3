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
// AkMusicNode.cpp
//
// The Music node is meant to be a parent of all playable music objects (excludes tracks).
// Has the definition of the music specific Play method.
// Defines the method for grid query (music objects use a grid, either their own, or that of their parent).
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include "AkMusicNode.h"
#include "AkMusicRenderer.h"

#define QUARTER_NOTE_METER_VALUE    (4)



CAkMusicNode::CAkMusicNode( AkUniqueID in_ulID )
:CAkActiveParent<CAkParameterNode>( in_ulID )
,m_bOverrideParentGrid( false )
,m_pStingers( NULL )
{
}

CAkMusicNode::~CAkMusicNode()
{
	FlushStingers();
}

void CAkMusicNode::FlushStingers()
{
	if( m_pStingers )
	{
		m_pStingers->Term();
		AkDelete( g_DefaultPoolId, m_pStingers );
		m_pStingers = NULL;
	}
}

AKRESULT CAkMusicNode::PrepareData()
{
	AKRESULT eResult = PrepareMusicalDependencies();
	if( eResult == AK_Success )
	{
		eResult = CAkActiveParent<CAkParameterNode>::PrepareData();
		if( eResult != AK_Success )
		{
			UnPrepareMusicalDependencies();
		}
	}

	return eResult;
}

void CAkMusicNode::UnPrepareData()
{
	CAkActiveParent<CAkParameterNode>::UnPrepareData();
	UnPrepareMusicalDependencies();
}

AKRESULT CAkMusicNode::PrepareMusicalDependencies()
{
	AKRESULT eResult = AK_Success;
	if( m_pStingers )
	{
		StingerArray& rStingerArray = m_pStingers->GetStingerArray();

		for( StingerArray::Iterator iter = rStingerArray.Begin(); iter != rStingerArray.End(); ++iter )
		{
			eResult = PrepareNodeData( (*iter).SegmentID() );

			if( eResult != AK_Success )
			{
				// iterate to undo what has been done up to now
				for( StingerArray::Iterator iterFlush = rStingerArray.Begin(); iterFlush != iter; ++iterFlush )
				{
					UnPrepareNodeData( (*iterFlush).SegmentID() );
				}
				break;	
			}
		}
	}
	return eResult;
}

void CAkMusicNode::UnPrepareMusicalDependencies()
{
	if( m_pStingers )
	{
		StingerArray& rStingerArray = m_pStingers->GetStingerArray();

		for( StingerArray::Iterator iter = rStingerArray.Begin(); iter != rStingerArray.End(); ++iter )
		{
			UnPrepareNodeData( (*iter).SegmentID() );
		}
	}
}

AKRESULT CAkMusicNode::SetMusicNodeParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize, bool /*in_bPartialLoadOnly*/ )
{
	//Read ID
	// We don't care about the ID, just skip it
	SKIPBANKDATA( AkUInt32, io_rpData, io_rulDataSize );

	//ReadParameterNode
	AKRESULT eResult = SetNodeBaseParams( io_rpData, io_rulDataSize, false );
	if ( eResult != AK_Success )
		return eResult;

	eResult = SetChildren( io_rpData, io_rulDataSize );
	if ( eResult != AK_Success )
		return eResult;

	AkMeterInfo l_meterInfo;
	l_meterInfo.fGridPeriod		= READBANKDATA( AkReal64, io_rpData, io_rulDataSize );        
	l_meterInfo.fGridOffset		= READBANKDATA( AkReal64, io_rpData, io_rulDataSize );         
	l_meterInfo.fTempo			= READBANKDATA( AkReal32, io_rpData, io_rulDataSize );             
	l_meterInfo.uTimeSigNumBeatsBar		= READBANKDATA( AkUInt8, io_rpData, io_rulDataSize ); 
	l_meterInfo.uTimeSigBeatValue		= READBANKDATA( AkUInt8, io_rpData, io_rulDataSize ); 

	if( READBANKDATA( AkUInt8, io_rpData, io_rulDataSize ) )// If override parent enabled
	{
		MeterInfo( &l_meterInfo );
	}

	AkUInt32 l_NumStingers			 = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
	if( l_NumStingers )
	{
		CAkStinger* pStingers = (CAkStinger*)AkAlloc( g_DefaultPoolId, sizeof( CAkStinger )* l_NumStingers );
		if( !pStingers )
			return AK_Fail;
		for( AkUInt32 i = 0; i < l_NumStingers; ++i )
		{
			pStingers[i].m_TriggerID = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
			pStingers[i].m_SegmentID = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
			pStingers[i].m_SyncPlayAt = (AkSyncType)READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
			pStingers[i].m_uCueFilterHash = READBANKDATA( AkUniqueID, io_rpData, io_rulDataSize );
			pStingers[i].m_DontRepeatTime = READBANKDATA( AkInt32, io_rpData, io_rulDataSize );
			pStingers[i].m_numSegmentLookAhead = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
		}
		eResult = SetStingers( pStingers, l_NumStingers );
		AkFree( g_DefaultPoolId, pStingers );
	}
	else
	{
		eResult = SetStingers( NULL, 0 );
	}

	return eResult;
}

void CAkMusicNode::MeterInfo(
    const AkMeterInfo * in_pMeterInfo         // Music grid info. NULL if inherits that of parent.
    )
{
    if (!in_pMeterInfo)
    {
        m_bOverrideParentGrid = false;
        return;
    }

    m_bOverrideParentGrid = true;

    AKASSERT( in_pMeterInfo->fTempo > 0
			&& in_pMeterInfo->uTimeSigBeatValue > 0 
			&& in_pMeterInfo->uTimeSigNumBeatsBar > 0 
			&& in_pMeterInfo->fGridPeriod > 0 );

    // NOTE (WG-4233) This will change in order to handle complex time signatures.
    m_grid.uBeatDuration = AkTimeConv::SecondsToSamples( ( 60.0 / (AkReal64) in_pMeterInfo->fTempo ) * ( (AkReal64)QUARTER_NOTE_METER_VALUE / (AkReal64)in_pMeterInfo->uTimeSigBeatValue ) );
    m_grid.uBarDuration = m_grid.uBeatDuration * in_pMeterInfo->uTimeSigNumBeatsBar;
    m_grid.uGridDuration = AkTimeConv::MillisecondsToSamples( in_pMeterInfo->fGridPeriod );
    m_grid.uGridOffset = AkTimeConv::MillisecondsToSamples( in_pMeterInfo->fGridOffset );
}

const AkMusicGrid & CAkMusicNode::GetMusicGrid()
{
	if ( !m_bOverrideParentGrid )
	{
		if( Parent() )
		{
			return static_cast<CAkMusicNode*>( Parent() )->GetMusicGrid();
		}
		// Error message to pad the known situation where the user can add a transition segment that is not part of the
		// same music tree, then explicitely exclude the parent of the transition segment.( WG-19847 )
		MONITOR_ERRORMSG( AKTEXT("Missing music node parent. Make sure your banks using music structure are completely loaded.") );
	}
	return m_grid;
}

// Music implementation of game triggered actions handling ExecuteAction(): 
// For Stop/Pause/Resume, call the music renderer, which searches among its
// contexts (music renderer's contexts are the "top-level" contexts).
// Other actions (actions on properties) are propagated through node hierarchy.
AKRESULT CAkMusicNode::ExecuteAction( ActionParams& in_rAction )
{
	// Note: temporarily add ref as CAkMusicRenderer::Stop() may destroy this.
	AddRef();

    // Catch Stop/Pause/Resume actions.
    switch( in_rAction.eType )
	{
	case ActionParamType_Stop:
		CAkMusicRenderer::Get( )->Stop( this, in_rAction.pGameObj, in_rAction.transParams, in_rAction.playingID );
		break;
	case ActionParamType_Pause:
		CAkMusicRenderer::Get( )->Pause( this, in_rAction.pGameObj, in_rAction.transParams, in_rAction.playingID );
		break;
	case ActionParamType_Resume:
		CAkMusicRenderer::Get( )->Resume( this, in_rAction.pGameObj, in_rAction.transParams, in_rAction.bIsMasterResume, in_rAction.playingID );
		break;
	}
		 
	AKRESULT eResult = CAkActiveParent<CAkParameterNode>::ExecuteAction( in_rAction );
	
	Release();
	
	return eResult;
}

// Music implementation of game triggered actions handling ExecuteAction(): 
// For Stop/Pause/Resume, call the music renderer, which searches among its
// contexts (music renderer's contexts are the "top-level" contexts).
// Other actions (actions on properties) are propagated through node hierarchy.
AKRESULT CAkMusicNode::ExecuteActionExcept( 
	ActionParamsExcept& in_rAction 
	)
{
    // Note: temporarily add ref as CAkMusicRenderer::Stop() may destroy this.
	AddRef();

    // Catch Stop/Pause/Resume actions.
    switch( in_rAction.eType )
	{
	case ActionParamType_Stop:
		CAkMusicRenderer::Get( )->Stop( this, in_rAction.pGameObj, in_rAction.transParams, in_rAction.playingID );
		break;
	case ActionParamType_Pause:
		CAkMusicRenderer::Get( )->Pause( this, in_rAction.pGameObj, in_rAction.transParams, in_rAction.playingID );
		break;
	case ActionParamType_Resume:
		CAkMusicRenderer::Get( )->Resume( this, in_rAction.pGameObj, in_rAction.transParams, in_rAction.bIsMasterResume, in_rAction.playingID );
		break;
	}
		 
	AKRESULT eResult = CAkActiveParent<CAkParameterNode>::ExecuteActionExcept( in_rAction );
	
	Release();
	
	return eResult;
}

// Block some parameters notifications (pitch)
void CAkMusicNode::ParamNotification( NotifParams& in_rParams )
{
	if ( in_rParams.eType != RTPC_Pitch )
		CAkActiveParent<CAkParameterNode>::ParamNotification( in_rParams );
}

AkObjectCategory CAkMusicNode::Category()
{
	return ObjCategory_MusicNode;
}

AKRESULT CAkMusicNode::SetStingers( CAkStinger* in_pStingers, AkUInt32 in_NumStingers )
{
	if( !in_NumStingers )
	{
		FlushStingers();
	}
	else
	{
		if( !m_pStingers )
		{
			m_pStingers = AkNew( g_DefaultPoolId, CAkStingers );
			if( !m_pStingers )
				return AK_Fail;
			if( m_pStingers->GetStingerArray().Reserve( in_NumStingers ) != AK_Success )
				return AK_Fail;
		}
		else
		{
			m_pStingers->RemoveAllStingers();
		}
		for( AkUInt32 i = 0; i < in_NumStingers; ++i )
		{
			if( !m_pStingers->GetStingerArray().AddLast( *( in_pStingers++ ) ) )
				return AK_Fail;
		}
	}
	return AK_Success;
}

void CAkMusicNode::GetStingers( CAkStingers* io_pStingers )
{
	//  Example usage:
	//
	//	CAkMusicNode::CAkStingers Stingers;
	//  GetStingers( &Stingers );
	//  ...
	//	Use Stingers 
	//	...
	//  Stingers.Term()

	AKASSERT( io_pStingers );// the caller must provide the CAkMusicNode::CAkStingers object
	if( Parent() )
	{
		static_cast<CAkMusicNode*>( Parent() )->GetStingers( io_pStingers );
	}
	if( m_pStingers )
	{
		StingerArray& rStingerArray = m_pStingers->GetStingerArray();
		StingerArray& rStingerArrayIO = io_pStingers->GetStingerArray();

		for( StingerArray::Iterator iter = rStingerArray.Begin(); iter != rStingerArray.End(); ++iter )
		{
			CAkStinger& stinger = *iter;

			StingerArray::Iterator iterIO = rStingerArrayIO.Begin();
			while( iterIO != rStingerArrayIO.End() )
			{
				CAkStinger& stingerIO = *iterIO;
				if( stingerIO.m_TriggerID == stinger.m_TriggerID )
				{
					iterIO = rStingerArrayIO.EraseSwap( iterIO );
				}
				else
				{
					++iterIO;
				}
			}

			rStingerArrayIO.AddLast( stinger );
		}
	}
}
