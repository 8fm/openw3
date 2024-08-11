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
#ifndef PROXYCENTRAL_CONNECTED

#include "TrackProxyLocal.h"
#include "AkMusicTrack.h"
#include "AkAudioLib.h"
#include "AkCritical.h"

namespace AK
{
	namespace WWISESOUNDENGINE_DLL
	{
		extern GlobalAnalysisSet g_setAnalysis;
	}
}

TrackProxyLocal::TrackProxyLocal( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_AudioNode );

	SetIndexable( pIndexable != NULL ? pIndexable : CAkMusicTrack::Create( in_id ) );
}

TrackProxyLocal::~TrackProxyLocal()
{
}

AKRESULT TrackProxyLocal::AddSource(
		AkUniqueID      in_srcID,
		AkPluginID      in_pluginID,
        const AkOSChar* in_pszFilename,
		AkFileID		in_uCacheID
        )
{
	CAkMusicTrack* pIndexable = static_cast<CAkMusicTrack*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
#ifdef AK_WIN

#pragma message( "Remove this when AL is built in non-unicode" )

#ifdef _UNICODE
		return pIndexable->AddSource( in_srcID, in_pluginID, in_pszFilename, in_uCacheID );
#else
		USES_CONVERSION;
		return pIndexable->AddSource( in_srcID, in_pluginID, A2CW( in_szFileName ), in_uCacheID );
#endif
#endif
	}
	return AK_Fail;
}

AKRESULT TrackProxyLocal::AddPluginSource( 
		AkUniqueID	in_srcID
		)
{
	CAkMusicTrack* pIndexable = static_cast<CAkMusicTrack*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		return pIndexable->AddPluginSource( in_srcID );
	}
	return AK_Fail;
}

void TrackProxyLocal::SetPlayList(
		AkUInt32		in_uNumPlaylistItem,
		AkTrackSrcInfo* in_pArrayPlaylistItems,
		AkUInt32		in_uNumSubTrack
		)
{
	CAkMusicTrack* pIndexable = static_cast<CAkMusicTrack*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->SetPlayList( in_uNumPlaylistItem, in_pArrayPlaylistItems, in_uNumSubTrack );
	}
}

void TrackProxyLocal::AddClipAutomation(
	AkUInt32				in_uClipIndex,
	AkClipAutomationType	in_eAutomationType,
	AkRTPCGraphPoint		in_arPoints[], 
	AkUInt32				in_uNumPoints 
	)
{
	CAkMusicTrack* pIndexable = static_cast<CAkMusicTrack*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->AddClipAutomation( in_uClipIndex, in_eAutomationType, in_arPoints, in_uNumPoints );
	}
}

void TrackProxyLocal::RemoveAllSources()
{
	CAkMusicTrack* pIndexable = static_cast<CAkMusicTrack*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->RemoveAllSources();
	}
}

void TrackProxyLocal::IsStreaming( bool /*in_bIsStreaming*/ )
{
	CAkMusicTrack* pIndexable = static_cast<CAkMusicTrack*>( GetIndexable() );
	if( pIndexable )
	{
		//pIndexable->IsStreaming(in_bIsStreaming);
	}
}

void TrackProxyLocal::IsZeroLatency( bool in_bIsZeroLatency )
{
	CAkMusicTrack* pIndexable = static_cast<CAkMusicTrack*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->IsZeroLatency(in_bIsZeroLatency);
	}
}

void TrackProxyLocal::LookAheadTime( AkTimeMs in_LookAheadTime )
{
	CAkMusicTrack* pIndexable = static_cast<CAkMusicTrack*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->LookAheadTime( in_LookAheadTime );
	}
}

void TrackProxyLocal::SetMusicTrackRanSeqType( AkUInt32 /*AkMusicTrackRanSeqType*/ in_eRSType )
{
	CAkMusicTrack* pIndexable = static_cast<CAkMusicTrack*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->SetMusicTrackRanSeqType( (AkMusicTrackRanSeqType) in_eRSType );
	}
}

void TrackProxyLocal::SetEnvelope( AkUniqueID in_sourceID, AkFileParser::EnvelopePoint * in_pEnvelope, AkUInt32 in_uNumPoints, AkReal32 in_fMaxEnvValue )
{
	CAkMusicTrack* pIndexable = static_cast<CAkMusicTrack*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		AK::WWISESOUNDENGINE_DLL::GlobalAnalysisSet::iterator it = AK::WWISESOUNDENGINE_DLL::g_setAnalysis.find( in_sourceID );
		if ( it != AK::WWISESOUNDENGINE_DLL::g_setAnalysis.end() )
		{
			// Already there. Update and notify sources.
			(*it).second = AK::WWISESOUNDENGINE_DLL::AnalysisInfo( 
				in_pEnvelope, 
				in_uNumPoints, 
				in_fMaxEnvValue );
			(*it).second.NotifyObservers();
		}
		else
		{
			// New envelope. 
			AK::WWISESOUNDENGINE_DLL::g_setAnalysis[ in_sourceID ] = AK::WWISESOUNDENGINE_DLL::AnalysisInfo( 
				in_pEnvelope, 
				in_uNumPoints, 
				in_fMaxEnvValue );
		}
	}
}
#endif
#endif // #ifndef AK_OPTIMIZED
