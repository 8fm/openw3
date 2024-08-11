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

/////////////////////////////////////////////////////////////////////
//
// AkVPLNode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkVPLNode.h"
#include "AudiolibDefs.h"

//-----------------------------------------------------------------------------
// Name: Connect
// Desc: Connects the specified effect as input.
//
// Parameters:
//	CAkVPLNode * in_pInput : Pointer to the effect to connect.
//-----------------------------------------------------------------------------
void CAkVPLNode::Connect( CAkVPLNode * in_pInput )
{
	AKASSERT( in_pInput != NULL );

	m_pInput = in_pInput;
} // Connect

//-----------------------------------------------------------------------------
// Name: Disconnect
// Desc: Disconnects the specified effect as input.
//-----------------------------------------------------------------------------
void CAkVPLNode::Disconnect( )
{
	m_pInput = NULL;
} // Disconnect

void CAkVPLNode::CopyRelevantMarkers( 
	AkPipelineBuffer*	in_pInputBuffer,
	AkPipelineBuffer*	io_pBuffer, 
	AkUInt32			in_ulBufferStartOffset, 
	AkUInt32			in_ulNumFrames
	)
{
	if( in_pInputBuffer->pMarkers )
	{
		AKASSERT( in_pInputBuffer->uNumMarkers > 0 );

		// First, count the number of markers relevant to this buffer
		AkUInt16 l_usNbMarkersToCopy = 0;
		AkBufferMarker* l_pCurrInputMarker = in_pInputBuffer->pMarkers;
		for( unsigned int i = 0; i < in_pInputBuffer->uNumMarkers; i++ )
		{
			if( ( l_pCurrInputMarker->dwPositionInBuffer >= in_ulBufferStartOffset ) &&
				( l_pCurrInputMarker->dwPositionInBuffer < in_ulBufferStartOffset + in_ulNumFrames ) )
			{
				l_usNbMarkersToCopy++;
			}

			l_pCurrInputMarker++;
		}

		// Now, copy the relevant markers
		if( l_usNbMarkersToCopy )
		{
			AkBufferMarker* l_pNewList = (AkBufferMarker*)AkAlloc( AK_MARKERS_POOL_ID, sizeof(AkBufferMarker) * ( io_pBuffer->uNumMarkers + l_usNbMarkersToCopy ) );
			if ( l_pNewList )
			{
				if( io_pBuffer->pMarkers )
					AKPLATFORM::AkMemCpy( l_pNewList, io_pBuffer->pMarkers, sizeof(AkBufferMarker) * io_pBuffer->uNumMarkers );

				AkBufferMarker* l_pCurrBufferMarker = l_pNewList + io_pBuffer->uNumMarkers;
				l_pCurrInputMarker = in_pInputBuffer->pMarkers; //reset pointer
				for( unsigned int i = 0; i < in_pInputBuffer->uNumMarkers; i++ )
				{
					if( ( l_pCurrInputMarker->dwPositionInBuffer >= in_ulBufferStartOffset ) &&
						( l_pCurrInputMarker->dwPositionInBuffer < in_ulBufferStartOffset + in_ulNumFrames ) )
					{
						l_pCurrBufferMarker->pContext = l_pCurrInputMarker->pContext;
						l_pCurrBufferMarker->dwPositionInBuffer = 0; //TODO: Find accurate position in output buffer
						l_pCurrBufferMarker->marker   = l_pCurrInputMarker->marker;
						l_pCurrBufferMarker++;
					}

					l_pCurrInputMarker++;
				}

				io_pBuffer->FreeMarkers();

				io_pBuffer->pMarkers = l_pNewList;
				io_pBuffer->uNumMarkers += l_usNbMarkersToCopy;
			}
			else
			{
				io_pBuffer->FreeMarkers();
			}
		}
	}
}