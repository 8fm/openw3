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
#include "AkMarkers.h"
#include "AkPlayingMgr.h"
#include "AudiolibDefs.h"
#include "AkPBI.h"
#include "AkMath.h"
#include <stdlib.h>

CAkMarkers::CAkMarkers()
	: m_pMarkers( NULL )
{
	m_hdrMarkers.uNumMarkers = 0;  // Markers header.
}

CAkMarkers::~CAkMarkers()
{
	AKASSERT( m_pMarkers == NULL );
}

AKRESULT CAkMarkers::Allocate( AkUInt32 in_uNumMarkers )
{
	AKASSERT( in_uNumMarkers > 0 );
	m_hdrMarkers.uNumMarkers = in_uNumMarkers;
	
	m_pMarkers = (AkAudioMarker*)AkAlloc( AK_MARKERS_POOL_ID, sizeof(AkAudioMarker) * in_uNumMarkers );
	if ( !m_pMarkers )
	{
		// Could not allocate enough cue points.
		m_hdrMarkers.uNumMarkers = 0;
		return AK_InsufficientMemory;
	}
	return AK_Success;
}

void CAkMarkers::Free()
{
	// Clean markers.
    if ( m_pMarkers )
    {
		for( AkUInt32 i=0; i<m_hdrMarkers.uNumMarkers; i++ )
		{
			if( m_pMarkers[i].strLabel )
			{
				AkFree( AK_MARKERS_POOL_ID, m_pMarkers[i].strLabel );
				m_pMarkers[i].strLabel = NULL;
			}
		}

        AkFree( AK_MARKERS_POOL_ID, m_pMarkers );
        m_pMarkers = NULL;
    }
	m_hdrMarkers.uNumMarkers = 0;
}

AKRESULT CAkMarkers::SetLabel( AkUInt32 in_idx, char * in_psLabel, AkUInt32 in_uStrSize )
{
	AKASSERT( in_uStrSize > 0 );
	char * strLabel = (char*)AkAlloc( AK_MARKERS_POOL_ID, in_uStrSize + 1 );
	if ( strLabel )
	{
		AKPLATFORM::AkMemCpy( strLabel, in_psLabel, in_uStrSize );
		strLabel[in_uStrSize] = '\0'; //append final NULL character
		m_pMarkers[in_idx].strLabel = strLabel;
		return AK_Success;
	}
	return AK_InsufficientMemory;
}

void CAkMarkers::CopyRelevantMarkers(
	CAkPBI* in_pCtx,
	AkPipelineBuffer & io_buffer, 
	AkUInt32 in_ulBufferStartPos )
{
	if ( NeedMarkerNotification( in_pCtx ) )
	{
		AkUInt32 uNumFrames = io_buffer.uValidFrames;

		// First, count the number of markers relevant to this buffer
		io_buffer.pMarkers = NULL;
		io_buffer.uNumMarkers = 0;

		for( unsigned int i = 0; i < m_hdrMarkers.uNumMarkers; i++ )
		{
			if( ( m_pMarkers[i].dwPosition >= in_ulBufferStartPos ) &&
				( m_pMarkers[i].dwPosition < in_ulBufferStartPos + uNumFrames ) )
			{
				io_buffer.uNumMarkers++;
			}
		}
	
		// Now, copy the relevant markers
		if( io_buffer.uNumMarkers )
		{
			io_buffer.pMarkers = (AkBufferMarker*)AkAlloc( AK_MARKERS_POOL_ID, sizeof(AkBufferMarker) * io_buffer.uNumMarkers );
			if ( io_buffer.pMarkers )
			{
				AkBufferMarker* l_pCurrBufferMarker = io_buffer.pMarkers;
				
				for( unsigned int i = 0; i < m_hdrMarkers.uNumMarkers; i++ )
				{
					if( ( m_pMarkers[i].dwPosition >= in_ulBufferStartPos ) &&
						( m_pMarkers[i].dwPosition < in_ulBufferStartPos + uNumFrames ) )
					{
						l_pCurrBufferMarker->pContext = in_pCtx;
						l_pCurrBufferMarker->dwPositionInBuffer = m_pMarkers[i].dwPosition - in_ulBufferStartPos;
						l_pCurrBufferMarker->marker   = m_pMarkers[i];
						l_pCurrBufferMarker++;
					}
				}
			}
			else
			{
				// Failed allocating buffer for markers. They are lost forever.
				io_buffer.uNumMarkers = 0;
			}
		}
	}
}

void CAkMarkers::NotifyRelevantMarkers( CAkPBI * in_pCtx, AkUInt32 in_uStartSample, AkUInt32 in_uStopSample )
{
	if( NeedMarkerNotification( in_pCtx ) )
	{
		for( unsigned int i = 0; i < m_hdrMarkers.uNumMarkers; i++ )
		{
			if( ( m_pMarkers[i].dwPosition >= in_uStartSample ) &&
				( m_pMarkers[i].dwPosition < in_uStopSample ) )
			{
				g_pPlayingMgr->NotifyMarker( in_pCtx, &m_pMarkers[i] );
			}
		}
	}
}

const AkAudioMarker * CAkMarkers::GetClosestMarker( AkUInt32 in_uPosition ) const
{
	AkAudioMarker * pMarker = NULL;
	AkUInt32 uSmallestDistance = 0;

	// Note: we have no guarantee that the markers are sorted.
	for( unsigned int i = 0; i < m_hdrMarkers.uNumMarkers; i++ )
	{
		AkUInt32 uDistance = abs( (int)( m_pMarkers[i].dwPosition - in_uPosition ) );
		if ( !pMarker 
			|| uDistance < uSmallestDistance )
		{
			pMarker = &m_pMarkers[i];
			uSmallestDistance = uDistance;
		}
	}

	return pMarker;
}

