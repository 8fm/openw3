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

#include "SegmentProxyConnected.h"
#include "AkMusicSegment.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"
#include "ISegmentProxy.h"

SegmentProxyConnected::SegmentProxyConnected( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_AudioNode );
	if ( !pIndexable )
		pIndexable = CAkMusicSegment::Create( in_id );

	SetIndexable( pIndexable );
}

SegmentProxyConnected::~SegmentProxyConnected()
{
}

void SegmentProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	CAkMusicSegment * pSegment = static_cast<CAkMusicSegment *>( GetIndexable() );

	switch( in_uMethodID )
	{
	case ISegmentProxy::MethodDuration:
		{
			MusicSegmentProxyCommandData::Duration duration;
			if( in_rSerializer.Get( duration ) )
				pSegment->Duration( duration.m_fDuration );

			break;
		}
	case ISegmentProxy::MethodStartPos:
		{
			MusicSegmentProxyCommandData::StartPos startPos;
			if( in_rSerializer.Get( startPos ) )
				pSegment->StartPos( startPos.m_fStartPos );

			break;
		}

	case ISegmentProxy::MethodSetMarkers:
		{
			MusicSegmentProxyCommandData::SetMarkers setMarkers;
			if( in_rSerializer.Get( setMarkers ) )
			{
				// Allocate strings for marker names in sound engine default pool and copy all.
				// SetMarkers() takes ownership of these strings.
				for ( AkUInt32 uMarker=0; uMarker<setMarkers.m_ulNumMarkers; uMarker++ )
				{
					if ( setMarkers.m_pArrayMarkers[uMarker].pszName != NULL )
					{
						const char * pMarkerName = setMarkers.m_pArrayMarkers[uMarker].pszName;
						setMarkers.m_pArrayMarkers[uMarker].pszName = (char*)AkAlloc( g_DefaultPoolId, strlen( pMarkerName ) + 1 );
						if ( setMarkers.m_pArrayMarkers[uMarker].pszName )
							strcpy( setMarkers.m_pArrayMarkers[uMarker].pszName, pMarkerName );
					}
				}
				pSegment->SetMarkers( setMarkers.m_pArrayMarkers, setMarkers.m_ulNumMarkers );
			}

			break;
		}

	default:
		__base::HandleExecute( in_uMethodID, in_rSerializer, out_rReturnSerializer );
	}
}
#endif // #ifndef AK_OPTIMIZED
