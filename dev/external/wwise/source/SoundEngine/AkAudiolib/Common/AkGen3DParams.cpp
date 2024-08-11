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
// AkGen3DParams.cpp
//
// alessard
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include "AkGen3DParams.h"
#include "AkMath.h"
#include "AkPathManager.h"
#include "AkDefault3DParams.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/Tools/Common/AkBankReadHelpers.h>

Gen3DParams::Gen3DParams()
	:m_pAttenuation( NULL )
{
}

Gen3DParams::~Gen3DParams()
{
	if( m_pAttenuation ) 
		m_pAttenuation->Release();
}

CAkGen3DParams::CAkGen3DParams()
{
	m_Params.m_ID						= AK_INVALID_UNIQUE_ID;
	m_Params.m_uAttenuationID			= AK_INVALID_UNIQUE_ID;
	m_Params.m_bIsSpatialized			= false;

	m_Params.m_bIsDynamic				= true;

	m_Params.m_ePathMode				= AkStepSequence;
	m_Params.m_bIsLooping				= false;
	m_Params.m_TransitionTime			= 0;

	m_Params.m_Position					= g_DefaultSoundPosition.Position;
	m_Params.m_bIsPanningFromRTPC		= false;
	m_Params.m_pArrayVertex				= NULL;
	m_Params.m_ulNumVertices			= 0;
	m_Params.m_pArrayPlaylist			= NULL;
	m_Params.m_ulNumPlaylistItem		= 0;
	m_Params.m_bFollowOrientation		= true;
	m_Params.m_bIsConeEnabled			= false;
	m_Params.m_fConeOutsideVolume		= 0.f;
	m_Params.m_fConeLoPass				= 0.f;
}

CAkGen3DParams::~CAkGen3DParams()
{
}

void CAkGen3DParams::Term()
{
	ClearPaths();
}

void CAkGen3DParams::ClearPaths()
{
	if(m_Params.m_pArrayVertex)
	{
		AkFree( g_DefaultPoolId, m_Params.m_pArrayVertex );
		m_Params.m_pArrayVertex = NULL;
	}
	if(m_Params.m_pArrayPlaylist)
	{
		AkFree( g_DefaultPoolId, m_Params.m_pArrayPlaylist );
		m_Params.m_pArrayPlaylist = NULL;
	}
	m_Params.m_ulNumVertices = 0;
	m_Params.m_ulNumPlaylistItem = 0;
}

void CAkGen3DParams::SetTransition( AkTimeMs in_TransitionTime )
{
	m_Params.m_TransitionTime = in_TransitionTime;
	UpdateTransitionTimeInVertex();
}

AKRESULT CAkGen3DParams::SetPathPlayList( CAkPath* in_pPath, AkPathState* in_pState)
{
	return g_pPathManager->SetPathsList(in_pPath, 
		m_Params.m_pArrayPlaylist,
		m_Params.m_ulNumPlaylistItem,
		m_Params.m_ePathMode,
		m_Params.m_bIsLooping,
		in_pState);
}

AKRESULT CAkGen3DParams::SetPath(
	AkPathVertex*           in_pArrayVertex, 
	AkUInt32                 in_ulNumVertices, 
	AkPathListItemOffset*   in_pArrayPlaylist, 
	AkUInt32                 in_ulNumPlaylistItem 
	)
{
	AKRESULT eResult = AK_Success;

	ClearPaths();

	// If there is something valid
	if( ( in_ulNumVertices != 0 ) 
		&& ( in_ulNumPlaylistItem != 0 ) 
		&& ( in_pArrayVertex != NULL ) 
		&& ( in_pArrayPlaylist != NULL ) 
		)
	{
		AkUInt32 ulnumBytesVertexArray = in_ulNumVertices * sizeof( AkPathVertex );
		m_Params.m_pArrayVertex = (AkPathVertex*)AkAlloc( g_DefaultPoolId, ulnumBytesVertexArray );

		if(m_Params.m_pArrayVertex)
		{
			AKPLATFORM::AkMemCpy( m_Params.m_pArrayVertex, in_pArrayVertex, ulnumBytesVertexArray );
			m_Params.m_ulNumVertices = in_ulNumVertices;

			AkUInt32 ulnumBytesPlayList = in_ulNumPlaylistItem * sizeof( AkPathListItem );
			m_Params.m_pArrayPlaylist = (AkPathListItem*)AkAlloc( g_DefaultPoolId, ulnumBytesPlayList );

			if(m_Params.m_pArrayPlaylist)
			{
				m_Params.m_ulNumPlaylistItem = in_ulNumPlaylistItem;
				AkUInt32 verticesSize = in_ulNumPlaylistItem * sizeof(AkPathListItemOffset);

				AkUInt8 *pPlaylist = (AkUInt8*)in_pArrayPlaylist;

				for( AkUInt32 i = 0; i < in_ulNumPlaylistItem; ++i )
				{
					AkUInt32 ulOffset = READBANKDATA(AkUInt32, pPlaylist, verticesSize);
					m_Params.m_pArrayPlaylist[i].iNumVertices = READBANKDATA(AkInt32, pPlaylist, verticesSize);
					AKASSERT(sizeof(AkPathListItemOffset) == 8);	//If the struct is larger, this code needs to be adapted.

					//Applying the offset to the initial value
					if(in_pArrayPlaylist[i].ulVerticesOffset < in_ulNumVertices )
					{
						m_Params.m_pArrayPlaylist[i].pVertices = m_Params.m_pArrayVertex + ulOffset;
						m_Params.m_pArrayPlaylist[i].fRangeX = 0.f;
						m_Params.m_pArrayPlaylist[i].fRangeY = 0.f;
					}
					else
					{
						AKASSERT( !"Trying to access out of range vertex" );
						eResult = AK_Fail;
						break;
					}
				}
			}
			else
			{
				eResult = AK_InsufficientMemory;
			}
		}
		else
		{
			eResult = AK_InsufficientMemory;
		}
	}
	else
	{
		eResult = AK_InvalidParameter;
	}

	UpdateTransitionTimeInVertex();

	return eResult;
}

AKRESULT CAkGen3DParams::UpdatePathPoint(
		AkUInt32 in_ulPathIndex,
		AkUInt32 in_ulVertexIndex,
		AkVector in_newPosition,
		AkTimeMs in_DelayToNext
		)
{
	AKRESULT eResult = AK_Success;

	AKASSERT( m_Params.m_pArrayVertex != NULL );
	AKASSERT( m_Params.m_pArrayPlaylist != NULL );
	if( ( m_Params.m_pArrayVertex != NULL )
		&& ( m_Params.m_pArrayPlaylist != NULL ) 
		&& ( in_ulPathIndex < m_Params.m_ulNumPlaylistItem )
		&& ( m_Params.m_pArrayPlaylist[in_ulPathIndex].iNumVertices > 0 )// Not useless, done to validate the signed unsigned cast that will be performed afterward
		&& ( in_ulVertexIndex < (AkUInt32)(m_Params.m_pArrayPlaylist[in_ulPathIndex].iNumVertices) ) )
	{
		AkPathVertex& l_rPathVertex = m_Params.m_pArrayPlaylist[in_ulPathIndex].pVertices[in_ulVertexIndex];
		l_rPathVertex.Duration = in_DelayToNext;
		l_rPathVertex.Vertex = in_newPosition;
		UpdateTransitionTimeInVertex();
	}
	else
	{
		eResult = AK_InvalidParameter;
		AKASSERT(!"It is useless to call UpdatePoints() on 3D Parameters if no points are set yet");
	}
	return eResult;
}

void CAkGen3DParams::UpdateTransitionTimeInVertex()
{
	for( AkUInt32 uli = 0; uli < m_Params.m_ulNumPlaylistItem; ++uli )
	{
		if(m_Params.m_pArrayPlaylist[uli].iNumVertices > 0)
		{
			m_Params.m_pArrayPlaylist[uli].pVertices[m_Params.m_pArrayPlaylist[uli].iNumVertices - 1].Duration = m_Params.m_TransitionTime;
		}
	}
}

CAkGen3DParamsEx::~CAkGen3DParamsEx()
{
	FreePathInfo();
}

void CAkGen3DParamsEx::FreePathInfo()
{
	if(m_PathState.pbPlayed != NULL)
	{
		AkFree(g_DefaultPoolId,m_PathState.pbPlayed);
		m_PathState.pbPlayed = NULL;
	}
}

#ifndef AK_OPTIMIZED
void CAkGen3DParams::InvalidatePaths()
{
	m_Params.m_pArrayVertex = NULL;
	m_Params.m_ulNumVertices = 0;
	m_Params.m_pArrayPlaylist = NULL;
	m_Params.m_ulNumPlaylistItem = 0;
}
#endif
