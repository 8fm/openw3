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
// AkMusicRanSeqCntr.cpp
//
// Music Random/Sequence container definition.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkMusicRanSeqCntr.h"
#include "AkMusicRenderer.h"
#include "AkSequenceCtx.h"
#include "AkMatrixSequencer.h"
#include "AkPBI.h"
#include "AkMonitor.h"

CAkMusicRanSeqCntr::CAkMusicRanSeqCntr( AkUniqueID in_ulID )
:CAkMusicTransAware( in_ulID )
,m_playListRoot( NULL )
{
}
CAkMusicRanSeqCntr::~CAkMusicRanSeqCntr()
{
    Term();
}

// Thread safe version of the constructor.
CAkMusicRanSeqCntr * CAkMusicRanSeqCntr::Create(
    AkUniqueID in_ulID
    )
{
	CAkMusicRanSeqCntr * pSequence = AkNew( g_DefaultPoolId, CAkMusicRanSeqCntr( in_ulID ) );
    if( pSequence )
	{
		if( pSequence->Init() != AK_Success )
		{
			pSequence->Release();
			pSequence = NULL;
		}
	}
    return pSequence;
}

AKRESULT CAkMusicRanSeqCntr::SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize )
{
	AKRESULT eResult = SetMusicTransNodeParams( in_pData, in_ulDataSize, false );
	if ( eResult != AK_Success )
		return eResult;

	AkUInt32 numPlaylistItems = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
	if( numPlaylistItems )
	{
		AkMusicRanSeqPlaylistItem* pPlayList = (AkMusicRanSeqPlaylistItem*)AkAlloc( g_DefaultPoolId, numPlaylistItems*sizeof(AkMusicRanSeqPlaylistItem) );
		if( !pPlayList ) 
			return AK_Fail;
		for( AkUInt32 i = 0; i < numPlaylistItems; ++i )
		{
			pPlayList[i].m_SegmentID		= READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
			pPlayList[i].m_playlistItemID	= READBANKDATA( AkUInt32, in_pData, in_ulDataSize );

			pPlayList[i].m_NumChildren		= READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
			pPlayList[i].m_eRSType			= (RSType)READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
			pPlayList[i].m_Loop				= READBANKDATA( AkInt16, in_pData, in_ulDataSize );
			pPlayList[i].m_Weight			= READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
			pPlayList[i].m_wAvoidRepeatCount= READBANKDATA( AkUInt16, in_pData, in_ulDataSize );

			pPlayList[i].m_bIsUsingWeight	= READBANKDATA( AkUInt8, in_pData, in_ulDataSize ) != 0;
			pPlayList[i].m_bIsShuffle		= READBANKDATA( AkUInt8, in_pData, in_ulDataSize ) != 0;
		}
		SetPlayList( pPlayList );
		AkFree( g_DefaultPoolId, pPlayList );
	}

	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	return eResult;
}

AKRESULT CAkMusicRanSeqCntr::Init()
{
	AKRESULT eResult = CAkMusicNode::Init();
	return eResult;
}

void CAkMusicRanSeqCntr::Term()
{
	FlushPlaylist();
}

void CAkMusicRanSeqCntr::FlushPlaylist()
{
	for( AkRSList::Iterator iter = m_playListRoot.m_listChildren.Begin(); iter != m_playListRoot.m_listChildren.End(); ++iter )
	{
		if(*iter)
		{
			AkDelete( g_DefaultPoolId, *iter );
		}
	}
	m_playListRoot.m_listChildren.RemoveAll();
	m_playListRoot.Clear();
}

// Return the node category.
AkNodeCategory CAkMusicRanSeqCntr::NodeCategory()
{
    return AkNodeCategory_MusicRanSeqCntr;
}

// Override MusicObject::ExecuteAction() to catch Seek actions.
AKRESULT CAkMusicRanSeqCntr::ExecuteAction( ActionParams& in_rAction )
{
    if ( ActionParamType_Seek == in_rAction.eType )
	{
		return AK_Fail;
		/** NOT IMPLEMENTED, YET
		// No need to propagate this action on children; only segments can handle this.
		SeekActionParams & rSeekActionParams = (SeekActionParams&)in_rAction;
		if ( SeekTypeTimeAbsolute == rSeekActionParams.eSeekType )
		{
			// Note: Negative seeking is NOT allowed on playlists.
			AkTimeMs iSeekTime = rSeekActionParams.iSeekTime;
			if ( iSeekTime < 0 )
				iSeekTime = 0;
			CAkMusicRenderer::Get()->SeekTimeAbsolute( this, rSeekActionParams.pGameObj, iSeekTime, rSeekActionParams.bSnapToNearestMarker );
		}
		else if ( SeekTypePercent == rSeekActionParams.eSeekType )
		{
			AkReal32 fSeekPercent = rSeekActionParams.fSeekPercent;
			
			// Clamp to [0,1].
			if ( fSeekPercent < 0 )
				fSeekPercent = 0;
			else if ( fSeekPercent > 1 )
				fSeekPercent = 1;

			CAkMusicRenderer::Get()->SeekPercent( this, rSeekActionParams.pGameObj, fSeekPercent, rSeekActionParams.bSnapToNearestMarker );
		}
		else 
			AKASSERT( !"Invalid seek type" );
		return AK_Success;
		**/
	}
	return CAkMusicNode::ExecuteAction( in_rAction );
}	

// Hierarchy enforcement: Music RanSeq Cntr can only have Segments as parents.
AKRESULT CAkMusicRanSeqCntr::CanAddChild(
    CAkParameterNodeBase * in_pAudioNode 
    )
{
    AKASSERT( in_pAudioNode );

	AkNodeCategory eCategory = in_pAudioNode->NodeCategory();

	AKRESULT eResult = AK_Success;	
	if(Children() >= AK_MAX_NUM_CHILD)
	{
		MONITOR_ERRORMSG( AKTEXT("Too many children in one single container.") );
		eResult = AK_MaxReached;
	}
	else if(eCategory != AkNodeCategory_MusicSegment)
	{
		eResult = AK_NotCompatible;
	}
	else if(in_pAudioNode->Parent() != NULL)
	{
		eResult = AK_ChildAlreadyHasAParent;
	}
	else if(m_mapChildId.Exists(in_pAudioNode->ID()))
	{
		eResult = AK_AlreadyConnected;
	}
	else if(ID() == in_pAudioNode->ID())
	{
		eResult = AK_CannotAddItseflAsAChild;
	}
	return eResult;	
}

CAkMatrixAwareCtx * CAkMusicRanSeqCntr::CreateContext( 
    CAkMatrixAwareCtx * in_pParentCtx,
    CAkRegisteredObj * in_GameObject,
    UserParams &  in_rUserparams
    )
{
    return CreateSequenceCtx( in_pParentCtx, in_GameObject, in_rUserparams );
}

CAkSequenceCtx * CAkMusicRanSeqCntr::CreateSequenceCtx( 
    CAkMatrixAwareCtx * in_pParentCtx,
    CAkRegisteredObj * in_GameObject,
    UserParams &  in_rUserparams
    )
{
    CAkSequenceCtx * pCtx = AkNew( g_DefaultPoolId, CAkSequenceCtx( 
        this,
        in_pParentCtx ) );
    if ( pCtx )
    {
		pCtx->AddRef();
        if ( pCtx->Init( in_GameObject,
                         in_rUserparams ) == AK_Success )
		{
			pCtx->Release();
		}
		else
        {
            pCtx->_Cancel();
			pCtx->Release();
            pCtx = NULL;
        }
    }
    return pCtx;
}

AKRESULT CAkMusicRanSeqCntr::PlayInternal( AkPBIParams& in_rPBIParams )
{
    // Create a top-level sequence context (that is, attached to the Music Renderer).
    CAkSequenceCtx * pCtx = CreateSequenceCtx( NULL, in_rPBIParams.pGameObj, in_rPBIParams.userParams );
    if ( pCtx )
    {
 		// Complete initialization of the sequence.
		pCtx->EndInit();

        // Do not set source offset: let it start playback at the position specified by the sequence's 
        // transition rules.
        AkMusicFade fadeParams;
        fadeParams.transitionTime   = in_rPBIParams.pTransitionParameters->TransitionTime;
        fadeParams.eFadeCurve       = in_rPBIParams.pTransitionParameters->eFadeCurve;
        // Set fade offset to context's silent duration.
        fadeParams.iFadeOffset      = pCtx->GetSilentDuration();
		pCtx->_Play( fadeParams );
        return AK_Success;
    }
    return AK_Fail;
}

// Interface for Wwise
// ----------------------

AKRESULT CAkMusicRanSeqCntr::SetPlayListChecked( AkMusicRanSeqPlaylistItem*	in_pArrayItems )
{
    if( CheckPlaylistHasChanged( in_pArrayItems ) )
    {
		if ( m_playListRoot.m_listChildren.Length() > 0 )
		{
	        // Resetting the playlist is not safe when music is playing.
		    // We must stop the music before proceeding.
	        if( CAkMusicRenderer::StopAll() )
			{
				//Send the message only if something was stopped.
				MONITOR_ERRORMESSAGE( AK::Monitor::ErrorCode_PlaylistStoppedForEditing );
			}
		}

        return SetPlayList( in_pArrayItems );
    }
    else
    {
        return AK_Success;
    }
}

bool CAkMusicRanSeqCntr::CheckPlaylistHasChanged( AkMusicRanSeqPlaylistItem* in_pArrayItems )
{
    AkMusicRanSeqPlaylistItem* pItem = in_pArrayItems++;
    if( 
           m_playListRoot.AvoidRepeatCount()    != pItem->m_wAvoidRepeatCount 
        || m_playListRoot.GetLoop()             != pItem->m_Loop
        || m_playListRoot.GetWeight()           != pItem->m_Weight
        || m_playListRoot.GetType()             != pItem->m_eRSType
        || m_playListRoot.RandomMode()          != (pItem->m_bIsShuffle?RandomMode_Shuffle:RandomMode_Normal)
        || m_playListRoot.PlaylistID()          != pItem->m_playlistItemID
        )
    {
        return true;
    }
    else
    {
        if( m_playListRoot.m_listChildren.Length() != (*pItem).m_NumChildren )
        {
            return true;
        }
        if( (*pItem).m_NumChildren )
        {
            return CheckPlaylistChildrenHasChanged( &m_playListRoot, in_pArrayItems, pItem->m_NumChildren );
        }
        else
        {
            return false;
        }
    }
}

AKRESULT CAkMusicRanSeqCntr::SetPlayList(
		AkMusicRanSeqPlaylistItem*	in_pArrayItems
		)
{
	///////////////////////////////////////////////////
	// If you change this function content, you most likely
	// will need to change the content of the function 
	// CheckPlaylistHasChanged()
	///////////////////////////////////////////////////
	AKASSERT( in_pArrayItems );

	FlushPlaylist();

	AkMusicRanSeqPlaylistItem* pItem = in_pArrayItems++;
	m_playListRoot.AvoidRepeatCount( pItem->m_wAvoidRepeatCount );
	m_playListRoot.SetLoop( pItem->m_Loop );
	m_playListRoot.SetWeight( pItem->m_Weight );

	m_playListRoot.SetType( pItem->m_eRSType );
	m_playListRoot.IsUsingWeight( false );// will be set to true later on if at least one child is not set to default value
	m_playListRoot.RandomMode( pItem->m_bIsShuffle );
	m_playListRoot.PlaylistID( pItem->m_playlistItemID );
	if( (*pItem).m_NumChildren )
	{
		return AddPlaylistChildren( &m_playListRoot, in_pArrayItems, pItem->m_NumChildren );
	}
	return AK_Success;
}

bool CAkMusicRanSeqCntr::CheckPlaylistChildrenHasChanged(
		CAkRSSub*					in_pParent,	
		AkMusicRanSeqPlaylistItem*&	in_pArrayItems, 
		AkUInt32					in_ulNumItems 
		)
{
    for( AkUInt32 i = 0; i < in_ulNumItems; ++i )
	{
		// Checking recursively
		AkMusicRanSeqPlaylistItem* pItem = in_pArrayItems++;
		if( pItem->m_SegmentID == AK_INVALID_UNIQUE_ID ) //if not segment
		{
            CAkRSNode* pRSNode = in_pParent->m_listChildren[ i ];
            // If it was a segment and it is now a node, it changed!
            if( pRSNode->IsSegment() )
            {
                return true;
            }
            CAkRSSub* pSub = (CAkRSSub*)pRSNode;

            AKASSERT( pSub );

            if( 
                   pSub->AvoidRepeatCount()    != pItem->m_wAvoidRepeatCount 
                || pSub->GetLoop()             != pItem->m_Loop
                || pSub->GetWeight()           != pItem->m_Weight
                || pSub->GetType()             != pItem->m_eRSType
                || pSub->RandomMode()          != (pItem->m_bIsShuffle?RandomMode_Shuffle:RandomMode_Normal)
                || pSub->PlaylistID()          != pItem->m_playlistItemID
                )
            {
                return true;
            }
            else
            {
                if( pSub->m_listChildren.Length() != (*pItem).m_NumChildren )
                {
                    return true;
                }
                if( (*pItem).m_NumChildren )
                {
                    if( CheckPlaylistChildrenHasChanged( pSub, in_pArrayItems, pItem->m_NumChildren ) )
                    {
                        return true;
                    }
                    else
                    {
                        //we continue looping.
                    }
                }
                else
                {
                    return false;
                }
            }
		}
		else
		{
            CAkRSNode* pRSNode = in_pParent->m_listChildren[ i ];
            // If it was a node and it is now a segment, it changed!
            if( !pRSNode->IsSegment() )
            {
                return true;
            }
            CAkRSSegment* pSeg = (CAkRSSegment*)pRSNode;

            if( 
                   pSeg->GetLoop()             != pItem->m_Loop
                || pSeg->GetWeight()           != pItem->m_Weight
                || pSeg->GetSegmentID()        != pItem->m_SegmentID
                || pSeg->PlaylistID()          != pItem->m_playlistItemID
                )
            {
                return true;
            }
		}
	}
	return false;
}

AKRESULT CAkMusicRanSeqCntr::AddPlaylistChildren(	
		CAkRSSub*					in_pParent,	
		AkMusicRanSeqPlaylistItem*&	in_pArrayItems, 
		AkUInt32					in_ulNumItems 
		)
{
	///////////////////////////////////////////////////
	// If you change this function content, you most likely
	// will need to change the content of the function 
	// CheckPlaylistChildrenHasChanged()
	///////////////////////////////////////////////////
	for( AkUInt32 i = 0; i < in_ulNumItems; ++i )
	{
		AkMusicRanSeqPlaylistItem* pItem = in_pArrayItems++;
		if( pItem->m_SegmentID == AK_INVALID_UNIQUE_ID ) //if not segment
		{
			CAkRSSub* pSub = AkNew( g_DefaultPoolId, CAkRSSub( in_pParent ) );
			if( !pSub )
			{
				return AK_Fail;
			}
			if( !in_pParent->m_listChildren.AddLast( pSub ) )
			{
				AkDelete( g_DefaultPoolId, pSub );
				return AK_Fail;
			}

			pSub->AvoidRepeatCount( pItem->m_wAvoidRepeatCount );
			pSub->SetLoop( pItem->m_Loop );
			pSub->SetWeight( pItem->m_Weight );
			pSub->SetType( pItem->m_eRSType );
			pSub->IsUsingWeight( false );// will be set to true if one child is not set to default value
			pSub->RandomMode( pItem->m_bIsShuffle );
			pSub->PlaylistID( pItem->m_playlistItemID );
			if( (*pItem).m_NumChildren )
			{
				if( AddPlaylistChildren( pSub, in_pArrayItems, pItem->m_NumChildren ) != AK_Success )
				{
					return AK_Fail;
				}
			}
		}
		else
		{
			//this is a segment
			CAkRSSegment* pSeg = AkNew( g_DefaultPoolId, CAkRSSegment( in_pParent ) );
			if( !pSeg )
			{
				return AK_Fail;
			}
			if( !in_pParent->m_listChildren.AddLast( pSeg ) )
			{
				AkDelete( g_DefaultPoolId, pSeg );
				return AK_Fail;
			}
			pSeg->SetLoop( pItem->m_Loop );
			pSeg->SetWeight( pItem->m_Weight );
			pSeg->SetSegmentID( pItem->m_SegmentID );
			pSeg->PlaylistID( pItem->m_playlistItemID );

			in_pParent->WasSegmentLeafFound();
		}

		if( pItem->m_Weight != DEFAULT_RANDOM_WEIGHT )
		{
			in_pParent->IsUsingWeight( true );
		}
	}
	return AK_Success;
}

// Interface for Contexts
// ----------------------

// Get first level node by index.
AKRESULT CAkMusicRanSeqCntr::GetNodeAtIndex( 
    AkUInt16        in_index, 
    AkUInt16 &      io_uPlaylistIdx    // TODO Replace with Multiplaylist iterator.
    )
{
    // TMP.
    /*if ( in_index >= m_playlist.Length() )
    {
        AKASSERT( !"Invalid playlist index" );
        return AK_InvalidParameter;
    }*/
    io_uPlaylistIdx = in_index;
    return AK_Success;
}
