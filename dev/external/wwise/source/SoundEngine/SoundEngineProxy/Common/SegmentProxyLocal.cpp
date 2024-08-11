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

#include "SegmentProxyLocal.h"

#include "AkMusicSegment.h"
#include "AkAudioLib.h"
#include "AkRegistryMgr.h"
#include "AkCritical.h"

SegmentProxyLocal::SegmentProxyLocal( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_AudioNode );

	SetIndexable( pIndexable != NULL ? pIndexable : CAkMusicSegment::Create( in_id ) );
}

SegmentProxyLocal::~SegmentProxyLocal()
{
}

void SegmentProxyLocal::Duration(
        AkReal64 in_fDuration               // Duration in milliseconds.
        )
{
	CAkMusicSegment* pIndexable = static_cast<CAkMusicSegment*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->Duration( in_fDuration );
	}
}

void SegmentProxyLocal::StartPos(
        AkReal64 in_fStartPos               // PlaybackStartPosition in milliseconds.
        )
{
	CAkMusicSegment* pIndexable = static_cast<CAkMusicSegment*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->StartPos( in_fStartPos );
	}
}

void SegmentProxyLocal::SetMarkers(
		AkMusicMarkerWwise*     in_pArrayMarkers, 
		AkUInt32                in_ulNumMarkers
		)
{
	CAkMusicSegment* pIndexable = static_cast<CAkMusicSegment*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		// Allocate strings for marker names in sound engine default pool and copy all.
		// SetMarkers() takes ownership of these strings.
		for ( AkUInt32 uMarker=0; uMarker<in_ulNumMarkers; uMarker++ )
		{
			if ( in_pArrayMarkers[uMarker].pszName != NULL )
			{
				const char * pMarkerName = in_pArrayMarkers[uMarker].pszName;
				in_pArrayMarkers[uMarker].pszName = (char*)AkAlloc( g_DefaultPoolId, strlen( pMarkerName ) + 1 );
				if ( in_pArrayMarkers[uMarker].pszName )
					strcpy( in_pArrayMarkers[uMarker].pszName, pMarkerName );
			}
		}
		pIndexable->SetMarkers( in_pArrayMarkers, in_ulNumMarkers );
	}
}
#endif
#endif // #ifndef AK_OPTIMIZED
