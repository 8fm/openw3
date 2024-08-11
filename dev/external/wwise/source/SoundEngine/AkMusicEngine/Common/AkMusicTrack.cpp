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
// AkMusicTrack.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include "AkMusicTrack.h"
#include "AkMusicSegment.h"
#include "AkSound.h"    // for dynamic sound creation.
#include "AudiolibDefs.h"
#include "AkRandom.h"
#include "AkMusicRenderer.h"
#include "AkBus.h"
#include "AkSource.h"

CAkMusicTrack::CAkMusicTrack( 
    AkUniqueID in_ulID
    )
	: CAkSoundBase(in_ulID)
	, m_uNumSubTrack( 0 )
	, m_iLookAheadTime( 0 )
	, m_eRSType( AkMusicTrackRanSeqType_Normal )
	, m_SequenceIndex( (AkUInt16) -1 )
{
}

CAkMusicTrack::~CAkMusicTrack()
{
    RemoveAllSourcesNoCheck();

    m_arSrcInfo.Term();
	m_arTrackPlaylist.Term();
	m_arClipAutomation.Term();
}

// Thread safe version of the constructor.
CAkMusicTrack * CAkMusicTrack::Create(
    AkUniqueID in_ulID
    )
{
	CAkMusicTrack * pTrack = AkNew( g_DefaultPoolId, CAkMusicTrack( in_ulID ) );
    if( pTrack )
	{
        if( pTrack->Init() != AK_Success )
		{
			pTrack->Release();
			pTrack = NULL;
		}
	}
    return pTrack;
}

AKRESULT CAkMusicTrack::SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize, CAkUsageSlot*, bool in_bIsPartialLoadOnly )
{	
	AKRESULT eResult = AK_Success;

	//Read ID
	// We don't care about the ID, just read/skip it
	SKIPBANKDATA( AkUInt32, in_pData, in_ulDataSize );

	AkUInt32 numSources = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );

	for( AkUInt32 i = 0; i < numSources; ++i )
	{
		//Read Source info
		if( eResult == AK_Success )
		{
			AkBankSourceData oSourceInfo;
			eResult = CAkBankMgr::LoadSource(in_pData, in_ulDataSize, oSourceInfo);
			if (eResult != AK_Success)
				return eResult;

			if (oSourceInfo.m_pParam == NULL)
			{
				//This is a file source
				eResult = AddSource( oSourceInfo.m_MediaInfo.sourceID, oSourceInfo.m_PluginID, oSourceInfo.m_MediaInfo );
			}
			else
			{
				//This is a plugin
				eResult = AddPluginSource( oSourceInfo.m_MediaInfo.sourceID );
			}

			if( eResult != AK_Success )
				return AK_Fail;
		}
	}

	//Read playlist
	AkUInt32 numPlaylistItem = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
	if( numPlaylistItem )
	{
		AkTrackSrcInfo* pPlaylist = ( AkTrackSrcInfo* )AkAlloc( g_DefaultPoolId, numPlaylistItem*sizeof( AkTrackSrcInfo ) );
		if( !pPlaylist )
			return AK_InsufficientMemory;
		for( AkUInt32 i = 0; i < numPlaylistItem; ++i )
		{
			pPlaylist[i].trackID			= READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
			pPlaylist[i].sourceID			= READBANKDATA( AkUniqueID, in_pData, in_ulDataSize );
			pPlaylist[i].fPlayAt			= READBANKDATA( AkReal64, in_pData, in_ulDataSize );
			pPlaylist[i].fBeginTrimOffset	= READBANKDATA( AkReal64, in_pData, in_ulDataSize );
			pPlaylist[i].fEndTrimOffset		= READBANKDATA( AkReal64, in_pData, in_ulDataSize );
			pPlaylist[i].fSrcDuration		= READBANKDATA( AkReal64, in_pData, in_ulDataSize );
		}
		AkUInt32 numSubTrack = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
		eResult = SetPlayList( numPlaylistItem, pPlaylist, numSubTrack );
		AkFree( g_DefaultPoolId, pPlaylist );
	}

	if( eResult != AK_Success )
		return eResult;

	//Read automation data
	m_arClipAutomation.Term();
	AkUInt32 numClipAutomationItem = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
	if ( numClipAutomationItem )
	{
		if ( m_arClipAutomation.Reserve( numClipAutomationItem ) != AK_Success )
			return AK_Fail;

		for( AkUInt32 i = 0; i < numClipAutomationItem; ++i )
		{
			CAkClipAutomation * pClipAutomation = m_arClipAutomation.AddLast();
			AKASSERT( pClipAutomation );	//reserved
			AkUInt32				uClipIndex	= READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
			AkClipAutomationType	eAutoType	= (AkClipAutomationType)READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
			AkUInt32				uNumPoints	= READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
			if ( uNumPoints )
			{
				if ( pClipAutomation->Set( uClipIndex, eAutoType, (AkRTPCGraphPoint*)in_pData, uNumPoints ) != AK_Success )
					return AK_Fail;
				
				const size_t sizeofAkRTPCGraphPoint = sizeof( AkReal32 ) + sizeof( AkReal32 ) + sizeof( AkInt32 );
				SKIPBANKBYTES( uNumPoints * sizeofAkRTPCGraphPoint, in_pData, in_ulDataSize );
			}
		}
	}

	
	//ReadParameterNode
	eResult = SetNodeBaseParams( in_pData, in_ulDataSize, in_bIsPartialLoadOnly );

	if( in_bIsPartialLoadOnly )
	{
		//Partial load has been requested, probable simply replacing the actual source created by the Wwise on load bank.
		return eResult;
	}

	if( eResult == AK_Success )
	{
		SetMusicTrackRanSeqType( ( AkMusicTrackRanSeqType )READBANKDATA( AkUInt32, in_pData, in_ulDataSize ) );
		LookAheadTime( READBANKDATA( AkTimeMs, in_pData, in_ulDataSize ) );
	}

	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	return eResult;
}
// Return the node category.
AkNodeCategory CAkMusicTrack::NodeCategory()
{
    return AkNodeCategory_MusicTrack;
}

AKRESULT CAkMusicTrack::PlayInternal( AkPBIParams& )
{
    AKASSERT( !"Cannot play music tracks" );
	return AK_NotImplemented;
}

AKRESULT CAkMusicTrack::ExecuteAction( ActionParams& in_rAction )
{
	AKRESULT eResult = AK_Success;

	if( in_rAction.bIsMasterCall )
	{
		//Only global pauses should Pause a state transition
		bool bPause = in_rAction.eType == ActionParamType_Pause;
		PauseTransitions( bPause );
	}

	return eResult;
}

AKRESULT CAkMusicTrack::ExecuteActionExcept( ActionParamsExcept& in_rAction )
{
	AKRESULT eResult = AK_Success;
	if( in_rAction.pGameObj == NULL )
	{
		//Only global pauses should Pause a state transition
		bool bPause = in_rAction.eType == ActionParamType_Pause;
		PauseTransitions( bPause );
	}

	return eResult;
}

// Wwise specific interface.
// -----------------------------------------
AKRESULT CAkMusicTrack::AddPlaylistItem(
		AkTrackSrcInfo &in_srcInfo
		)
{
	AkReal64 fClipDuration = in_srcInfo.fSrcDuration + in_srcInfo.fEndTrimOffset - in_srcInfo.fBeginTrimOffset;
	//AKASSERT( fClipDuration >= 0 );
	// Note: the UI sometimes pushes negative (or ~0) source duration. If it happens, ignore this playlist item.
	if ( fClipDuration <= 0 )
		return AK_Success;


	AkTrackSrc * pPlaylistRecord = m_arTrackPlaylist.AddLast();
	if ( !pPlaylistRecord )
		return AK_Fail;

	pPlaylistRecord->uSubTrackIndex =	in_srcInfo.trackID;
	pPlaylistRecord->srcID =			in_srcInfo.sourceID;

	AKASSERT( in_srcInfo.fPlayAt + in_srcInfo.fBeginTrimOffset > - ((AkReal64)(1000.f/(AkReal64)AK_CORE_SAMPLERATE)) );
	pPlaylistRecord->uClipStartPosition =	AkTimeConv::MillisecondsToSamples( in_srcInfo.fPlayAt + in_srcInfo.fBeginTrimOffset );

    pPlaylistRecord->uClipDuration =	AkTimeConv::MillisecondsToSamples( fClipDuration );

	pPlaylistRecord->uSrcDuration =		AkTimeConv::MillisecondsToSamples( in_srcInfo.fSrcDuration );

	AkInt32 iBeginTrimOffset = AkTimeConv::MillisecondsToSamples( in_srcInfo.fBeginTrimOffset );
	pPlaylistRecord->iSourceTrimOffset = iBeginTrimOffset % (AkInt32)pPlaylistRecord->uSrcDuration;
	AKASSERT( abs( (int)pPlaylistRecord->iSourceTrimOffset ) < (AkInt32)pPlaylistRecord->uSrcDuration );
	if ( pPlaylistRecord->iSourceTrimOffset < 0 )
		pPlaylistRecord->iSourceTrimOffset += pPlaylistRecord->uSrcDuration;
	AKASSERT( pPlaylistRecord->iSourceTrimOffset >= 0 );
	
	return AK_Success;
}

AKRESULT CAkMusicTrack::SetPlayList(
		AkUInt32		in_uNumPlaylistItem,
		AkTrackSrcInfo* in_pArrayPlaylistItems,
		AkUInt32		in_uNumSubTrack
		)
{
	NOTIFY_EDIT_DIRTY();
		
	m_arTrackPlaylist.Term();

	m_uNumSubTrack = in_uNumSubTrack;

	if ( m_arTrackPlaylist.Reserve( in_uNumPlaylistItem ) != AK_Success )
		return AK_Fail;

	for( AkUInt32 i = 0; i < in_uNumPlaylistItem; ++i )
	{
		// Add playlist item. Must succeed because we just reserved memory for the array.
		AKVERIFY( AddPlaylistItem( in_pArrayPlaylistItems[i] ) == AK_Success );
	}
	return AK_Success;
}

AKRESULT CAkMusicTrack::AddClipAutomation(
	AkUInt32				in_uClipIndex,
	AkClipAutomationType	in_eAutomationType,
	AkRTPCGraphPoint		in_arPoints[], 
	AkUInt32				in_uNumPoints 
	)
{
	ClipAutomationArray::Iterator it = m_arClipAutomation.FindByKey( in_uClipIndex, in_eAutomationType );
	if ( it != m_arClipAutomation.End() )
	{
		// Free automation curve (see note in CAkClipAutomation).
		(*it).ClearCurve();
		m_arClipAutomation.EraseSwap( it );

		NOTIFY_EDIT_DIRTY();
	}

	if ( in_uNumPoints > 0 )
	{
		CAkClipAutomation * pAutomation = m_arClipAutomation.AddLast();
		if ( pAutomation )
		{
			NOTIFY_EDIT_DIRTY();
			return pAutomation->Set( in_uClipIndex, in_eAutomationType, in_arPoints, in_uNumPoints );
		}
	}
	return AK_Success;
}

AKRESULT CAkMusicTrack::AddSource( 
	AkUniqueID      in_srcID,
	AkPluginID      in_pluginID,
    const AkOSChar* in_pszFilename,
	AkFileID		in_uCacheID
    )
{
	CAkMusicSource** ppSource = m_arSrcInfo.Exists( in_srcID );
	if( ppSource )
	{
		//Already there, if the source is twice in the same playlist, it is useless to copy it twice.
		return AK_Success;
	}
	else
	{
		ppSource = m_arSrcInfo.Set( in_srcID );
	}
    if ( ppSource )
    {   
		*ppSource = AkNew( g_DefaultPoolId, CAkMusicSource() );
		if(*ppSource)
		{
			(*ppSource)->SetSource( in_srcID, in_pluginID, in_pszFilename, in_uCacheID, 
				false,		// Called from authoring: cannot be from RSX.
				false );	// External sources not yet supported in music tracks.
            (*ppSource)->StreamingLookAhead( m_iLookAheadTime );
		}
		else
		{
			m_arSrcInfo.Unset( in_srcID );
		}
    }
	return ( ppSource && *ppSource ) ? AK_Success : AK_Fail;
}

AKRESULT CAkMusicTrack::AddSource( 
		AkUniqueID in_srcID, 
		AkUInt32 in_pluginID, 
		AkMediaInformation in_MediaInfo
		)
{
    CAkMusicSource** ppSource = m_arSrcInfo.Exists( in_srcID );
	if( ppSource )
	{
		//Already there, if the source is twice in the same playlist, it is useless to copy it twice.
		return AK_Success;
	}
	else
	{
		ppSource = m_arSrcInfo.Set( in_srcID );
	}
    if ( ppSource )
    {   
		*ppSource = AkNew( g_DefaultPoolId, CAkMusicSource() );
		if(*ppSource)
		{
			(*ppSource)->SetSource( in_pluginID, in_MediaInfo );
            (*ppSource)->StreamingLookAhead( m_iLookAheadTime );
		}
		else
		{
			m_arSrcInfo.Unset( in_srcID );
		}
    }
	return ( ppSource && *ppSource ) ? AK_Success : AK_Fail;
}

AKRESULT CAkMusicTrack::AddPluginSource( 
		AkUniqueID	in_srcID )
{
	CAkMusicSource** ppSource = m_arSrcInfo.Set( in_srcID );
    if ( ppSource )
    {   
		*ppSource = AkNew( g_DefaultPoolId, CAkMusicSource() );
		if(*ppSource)
		{
			(*ppSource)->SetSource( in_srcID );
		}
		else
		{
			m_arSrcInfo.Unset( in_srcID );
		}
    }
	return ( ppSource && *ppSource ) ? AK_Success : AK_Fail;
}

bool CAkMusicTrack::HasBankSource()
{ 
	for( SrcInfoArray::Iterator iter = m_arSrcInfo.Begin(); iter != m_arSrcInfo.End(); ++iter )
	{
		if( iter.pItem->item->HasBankSource() )
			return true;
	}
	return false;
}

void CAkMusicTrack::RemoveAllSources()
{
	// WG-20999: Cannot remove sources while editing if we're playing. Sources will "leak",
	// in that if a new source with the same ID is added, it will be ignored, and other sources
	// could exist for a while although they are not needed. This is an authoring issue only, since
	// this function is always only called by the WAL through the local proxy.
	if ( !IsPlaying() )
		RemoveAllSourcesNoCheck();
}

void CAkMusicTrack::RemoveAllSourcesNoCheck()
{
	m_uNumSubTrack = 0;
	m_arTrackPlaylist.RemoveAll();
	for( SrcInfoArray::Iterator iter = m_arSrcInfo.Begin(); iter != m_arSrcInfo.End(); ++iter )
	{
		AkDelete( g_DefaultPoolId, iter.pItem->item );
	}
	m_arSrcInfo.RemoveAll();
}

void CAkMusicTrack::IsZeroLatency( bool in_bIsZeroLatency )
{
	for( SrcInfoArray::Iterator iter = m_arSrcInfo.Begin(); iter != m_arSrcInfo.End(); ++iter )
	{
		iter.pItem->item->IsZeroLatency( in_bIsZeroLatency );
	}
}

void CAkMusicTrack::LookAheadTime( AkTimeMs in_LookAheadTime )
{
	m_iLookAheadTime = AkTimeConv::MillisecondsToSamples( in_LookAheadTime * CAkMusicRenderer::StreamingLookAheadRatio() );
	for( SrcInfoArray::Iterator iter = m_arSrcInfo.Begin(); iter != m_arSrcInfo.End(); ++iter )
	{
		iter.pItem->item->StreamingLookAhead( m_iLookAheadTime );
	}
}

AkObjectCategory CAkMusicTrack::Category()
{
	return ObjCategory_Track;
}

// Like ParameterNodeBase's, but does not check parent.
bool CAkMusicTrack::GetStateSyncTypes( AkStateGroupID in_stateGroupID, CAkStateSyncArray* io_pSyncTypes )
{
	if( CheckSyncTypes( in_stateGroupID, io_pSyncTypes ) )
		return true;
	
	if( ParentBus() )
	{
		if( static_cast<CAkBus*>( ParentBus() )->GetStateSyncTypes( in_stateGroupID, io_pSyncTypes ) )
		{
			return true;
		}
	}
	return false;
}

// Interface for Contexts
// ----------------------

CAkMusicSource* CAkMusicTrack::GetSourcePtr( AkUniqueID SourceID )
{
	CAkMusicSource ** ppSource = m_arSrcInfo.Exists( SourceID );
	if( ppSource )
		return *ppSource;
	return NULL;
}

AkUInt16 CAkMusicTrack::GetNextRS()
{
	AkUInt16 uIndex = 0;
	switch( m_eRSType )
	{
	case AkMusicTrackRanSeqType_Normal:
		break;
		
	case AkMusicTrackRanSeqType_Random:
		if( m_uNumSubTrack )
			uIndex = (AkUInt16)( AKRANDOM::AkRandom() % m_uNumSubTrack );
		break;

	case AkMusicTrackRanSeqType_Sequence:
		++m_SequenceIndex;
		if( m_SequenceIndex >= m_uNumSubTrack )
		{
			m_SequenceIndex = 0;
		}
		uIndex = m_SequenceIndex;
		break;
	
	default:
		AKASSERT( !"Unknown MusicTrackRanSeqType" );
	}
	return uIndex;
}

AKRESULT CAkMusicTrack::PrepareData()
{
	AKRESULT eResult = AK_Success;
	for( SrcInfoArray::Iterator iter = m_arSrcInfo.Begin(); iter != m_arSrcInfo.End(); ++iter )
	{
		eResult = iter.pItem->item->PrepareData();
		if( eResult != AK_Success )
		{
			// undo what has been prepared up to now.
			for( SrcInfoArray::Iterator iterFlush = m_arSrcInfo.Begin(); iterFlush != iter; ++iterFlush )
			{
				iterFlush.pItem->item->UnPrepareData();
			}
			break;
		}
	}
	
	return eResult;
}

void CAkMusicTrack::UnPrepareData()
{
	for( SrcInfoArray::Iterator iter = m_arSrcInfo.Begin(); iter != m_arSrcInfo.End(); ++iter )
	{
		iter.pItem->item->UnPrepareData();
	}
}
