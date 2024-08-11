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

#include "stdafx.h"
#ifndef AK_OPTIMIZED

#include "TrackProxyConnected.h"
#include "AkMusicTrack.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"
#include "ITrackProxy.h"

TrackProxyConnected::TrackProxyConnected( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_AudioNode );
	if ( !pIndexable )
		pIndexable = CAkMusicTrack::Create( in_id );

	SetIndexable( pIndexable );
}

TrackProxyConnected::~TrackProxyConnected()
{
}

void TrackProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	CAkMusicTrack * pTrack = static_cast<CAkMusicTrack *>( GetIndexable() );

	switch( in_uMethodID )
	{
	case ITrackProxy::MethodAddSource:
		{	
			//no remote set source
			break;
		}
	case ITrackProxy::MethodRemoveAllSources:
		{	
			//no remote set source
			break;
		}
	case ITrackProxy::MethodSetPlayList:
		{
			//no remote set playlist due to source collapse by SB generator.
			/**
			TrackProxyCommandData::SetPlayList setPlayList;
			in_rSerializer.Get( setPlayList );

			pTrack->SetPlayList( setPlayList.m_uNumPlaylistItem, setPlayList.m_pArrayPlaylistItems, setPlayList.m_uNumSubTrack );
			**/
			break;
		}

	case ITrackProxy::MethodAddClipAutomation:
		{
			TrackProxyCommandData::AddClipAutomation addClipAutomation;
			if( in_rSerializer.Get( addClipAutomation ) )
				pTrack->AddClipAutomation( addClipAutomation.m_uClipIndex, addClipAutomation.m_eAutomationType, addClipAutomation.m_pArrayPoints, addClipAutomation.m_uNumPoints );

			break;
		}

	case ITrackProxy::MethodIsStreaming:
		{
			TrackProxyCommandData::IsStreaming isStreaming;
			if( in_rSerializer.Get( isStreaming ) )
			{
//				pTrack->IsStreaming( isStreaming.m_param1 );
			}

			break;
		}

	case ITrackProxy::MethodIsZeroLatency:
		{
			TrackProxyCommandData::IsZeroLatency isZeroLatency;
			if( in_rSerializer.Get( isZeroLatency ) )
				pTrack->IsZeroLatency( isZeroLatency.m_param1 );

			break;
		}
	case ITrackProxy::MethodLookAheadTime:
		{
			TrackProxyCommandData::LookAheadTime lookAheadTime;
			if( in_rSerializer.Get( lookAheadTime ) )
				pTrack->LookAheadTime( lookAheadTime.m_param1 );

			break;
		}
	case ITrackProxy::MethodSetMusicTrackRanSeqType:
		{
			TrackProxyCommandData::SetMusicTrackRanSeqType setMusicTrackRanSeqType;
			if( in_rSerializer.Get( setMusicTrackRanSeqType ) )
				pTrack->SetMusicTrackRanSeqType( (AkMusicTrackRanSeqType) setMusicTrackRanSeqType.m_param1 );

			break;
		}

	default:
		__base::HandleExecute( in_uMethodID, in_rSerializer, out_rReturnSerializer );
	}
}
#endif // #ifndef AK_OPTIMIZED
