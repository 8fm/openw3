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
// AkRSIterator.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkRSIterator.h"
#include "AkMusicRanSeqCntr.h"
#include "AkRanSeqBaseInfo.h"
#include "AkRandom.h"
#include "AkPlayingMgr.h"

///////////////
//CAkRSSub Class
//////////////

CAkRSSub::~CAkRSSub()
{
	Clear();
	for( AkRSList::Iterator iter = m_listChildren.Begin(); iter != m_listChildren.End(); ++iter )
	{
		if( (*iter) )
		{
			AkDelete( g_DefaultPoolId, (*iter) );
		}
	}
	m_listChildren.Term();
}

void CAkRSSub::Clear()
{
	if( m_pGlobalCntrBaseInfo )
	{
		m_pGlobalCntrBaseInfo->Destroy();
		m_pGlobalCntrBaseInfo = NULL;
	}
}

void CAkRSSub::WasSegmentLeafFound()
{
	if ( !m_bHasSegmentLeaves && Parent() )
	{
		AKASSERT( !Parent()->IsSegment() );
		static_cast<CAkRSSub*>(Parent())->WasSegmentLeafFound();
	}
	m_bHasSegmentLeaves = true;
}

AkRandomMode CAkRSSub::RandomMode()
{
	return m_bIsShuffle ? RandomMode_Shuffle : RandomMode_Normal;
}

AkUInt32 CAkRSSub::CalculateTotalWeight()
{
	AkUInt32 TotalWeigth = 0;
	for( AkRSList::Iterator iter = m_listChildren.Begin(); iter != m_listChildren.End(); ++iter )
	{
		TotalWeigth += (*iter)->GetWeight();
	}
	return TotalWeigth;
}

CAkContainerBaseInfo* CAkRSSub::GetGlobalRSInfo()
{
	if( !m_pGlobalCntrBaseInfo )
	{
		if( m_listChildren.Length() )
		{
			// none available, let's create it
			switch( GetType() )
			{
			case RSType_StepSequence:
				m_pGlobalCntrBaseInfo = AkNew( g_DefaultPoolId, CAkSequenceInfo() );
				break;
	
			case RSType_StepRandom:
				m_pGlobalCntrBaseInfo = AkNew( g_DefaultPoolId, CAkRandomInfo( (AkUInt16)m_listChildren.Length() ) );
				if( m_pGlobalCntrBaseInfo && static_cast<CAkRandomInfo*>( m_pGlobalCntrBaseInfo )->Init(m_wAvoidRepeatCount) != AK_Success )
				{
					m_pGlobalCntrBaseInfo->Destroy();
					m_pGlobalCntrBaseInfo = NULL;
				}
				if( m_pGlobalCntrBaseInfo && IsUsingWeight() )
				{
					static_cast<CAkRandomInfo*>( m_pGlobalCntrBaseInfo )->m_ulTotalWeight = static_cast<CAkRandomInfo*>( m_pGlobalCntrBaseInfo )->m_ulRemainingWeight = CalculateTotalWeight();
				}
				break;
	
			default:
				AKASSERT( !"Unhandled RSType" );
				break;
			}
		}
	}
	return m_pGlobalCntrBaseInfo;
}

void CAkRSSub::OverwriteGlobalRSInfo( CAkContainerBaseInfo * in_pRSInfo )
{
	AKASSERT( m_pGlobalCntrBaseInfo );
	m_pGlobalCntrBaseInfo->Destroy();
	m_pGlobalCntrBaseInfo = in_pRSInfo;
}

//////////////////////////////
// RSStackItem
//////////////////////////////
RSStackItem::RSStackItem() 
: pLocalRSInfo( NULL )
{
}

AKRESULT RSStackItem::Init( CAkRSSub * in_pSub )
{
	pRSNode = in_pSub;
	m_Loop.bIsInfinite = ( in_pSub->GetLoop() == 0 ); 
	m_Loop.bIsEnabled = true;
	m_Loop.lLoopCount = in_pSub->GetLoop();

	switch( in_pSub->GetType() )
	{
	case RSType_ContinuousRandom:
		if( in_pSub->m_listChildren.Length() != 0 )
		{
			pLocalRSInfo = AkNew( g_DefaultPoolId, CAkRandomInfo( (AkUInt16)( in_pSub->m_listChildren.Length() ) ) );
		}
		if( !pLocalRSInfo )
		{
			return AK_Fail;
		}
		if( static_cast<CAkRandomInfo*>( pLocalRSInfo )->Init(in_pSub->AvoidRepeatCount()) != AK_Success )
		{
			pLocalRSInfo->Destroy();
			pLocalRSInfo = NULL;
			return AK_Fail;
		}
		else if( in_pSub->IsUsingWeight() )
		{
			static_cast<CAkRandomInfo*>( pLocalRSInfo )->m_ulTotalWeight = static_cast<CAkRandomInfo*>( pLocalRSInfo )->m_ulRemainingWeight = in_pSub->CalculateTotalWeight();
		}
		break;

	case RSType_ContinuousSequence:
		if( in_pSub->m_listChildren.Length() != 0 )
		{
			pLocalRSInfo = AkNew( g_DefaultPoolId, CAkSequenceInfo() );
		}
		if( !pLocalRSInfo )
		{
			return AK_Fail;
		}
		break;

	case RSType_StepRandom:
	case RSType_StepSequence:
		m_Loop.bIsInfinite = true;
		break;

	default:
		AKASSERT( !"Unhandled RSType" );
		return AK_Fail;
	}

	return AK_Success;
}

void RSStackItem::Clear()
{
	if ( pRSNode->GetType() == RSType_ContinuousSequence
		|| pRSNode->GetType() == RSType_ContinuousRandom )
	{
		if ( pLocalRSInfo )
			pLocalRSInfo->Destroy();
	}
}

///////////////
//AkRSIterator Class
//////////////
AkRSIterator::AkRSIterator( CAkMusicRanSeqCntr* in_pRSCntr )
	:m_pRSCntr( in_pRSCntr )
	,m_actualSegment( AK_INVALID_UNIQUE_ID )
	,m_LastSegmentPlayingID( AK_INVALID_UNIQUE_ID )
	,m_bIsSegmentValid( false )
	,m_bDoSaveOriginalGlobalRSInfo( true )
	,m_uSegmentLoopCount( 0 )
{
}

AkRSIterator::~AkRSIterator()
{
}

AKRESULT AkRSIterator::Init( AkPlayingID in_playingID )
{
	m_playingID = in_playingID;
	m_actualSegment = AK_MUSIC_TRANSITION_RULE_ID_NONE;
	m_bIsSegmentValid = true;

	m_bDoSaveOriginalGlobalRSInfo = true;

	CAkRSNode* pNode = m_pRSCntr->GetPlaylistRoot();
	while( pNode && !pNode->IsSegment() )
	{
		CAkRSSub* pSub = static_cast<CAkRSSub*>(pNode);
		if( !pSub->m_listChildren.Length()
			|| !pSub->HasSegmentLeaves() )
		{
			// The list item is empty, we Go back to parent and we won't stack it again.
			pNode = pNode->Parent();
			pSub = static_cast<CAkRSSub*>(pNode);
			if( !pNode )
				break;// Apparently nothing playable, keep AK_MUSIC_TRANSITION_RULE_ID_NONE
		}
		else if( StackItem( pSub ) != AK_Success )
		{
			Term();
			return AK_Fail;
		}

		bool bIsEnd = false;
		do
		{
			AkUInt16 newIndex = Select( m_stack.Last(), bIsEnd );
			
			if( !bIsEnd )
			{
				pNode = ( pSub->m_listChildren )[newIndex];
			}
			else
			{
				// We must go back one step, and take back the parent as curent node, if there is no parent, there is nothing to play.
				// If we are in step mode, no next.
				pNode = pSub->Parent();
				PopLast();
			}
		}while( bIsEnd && pNode );
	}

	return SetCurrentSegmentToNode( pNode );
}

// Call when no more JumpTo will be called.
// Flushes the map of original global RanInfo that were saved.
void AkRSIterator::EndInit()
{
	GlobalRSInfoMap::Iterator it = m_arOriginalGlobalRSInfo.Begin();
	while ( it != m_arOriginalGlobalRSInfo.End() )
	{
		AKASSERT( (*it).item );
		(*it).item->Destroy();
		++it;
	}

	m_arOriginalGlobalRSInfo.RemoveAll();

	m_bDoSaveOriginalGlobalRSInfo = false;
}


void AkRSIterator::Term()
{	
	FlushStack();
	m_stack.Term();
	GlobalRSInfoMap::Iterator it = m_arOriginalGlobalRSInfo.Begin();
	while ( it != m_arOriginalGlobalRSInfo.End() )
	{
		AKASSERT( (*it).item );
		(*it).item->Destroy();
		++it;
	}
	m_arOriginalGlobalRSInfo.Term();
}

void AkRSIterator::FlushStack()
{
	for( IteratorStack::Iterator iter = m_stack.Begin(); iter != m_stack.End(); ++iter )
	{
		(*iter).Clear();
	}
	m_stack.RemoveAll();
}

void AkRSIterator::PopLast()
{	
	RSStackItem & l_Item = m_stack.Last();
	l_Item.Clear();
	m_stack.RemoveLast();
}

void AkRSIterator::JumpNext()
{
	if( m_uSegmentLoopCount > 1 )
	{
		--m_uSegmentLoopCount;
		return;
	}
	if( m_uSegmentLoopCount == 0 )//Infinite means we stay stuck at this position until something happpens.
	{
		return;
	}
	if( m_actualSegment == AK_MUSIC_TRANSITION_RULE_ID_NONE )
	{
		m_bIsSegmentValid = false;
		return;
	}
	m_actualSegment = AK_MUSIC_TRANSITION_RULE_ID_NONE;
	m_bIsSegmentValid = true;

	CAkRSNode* pNode = NULL;

	if( !m_stack.IsEmpty() )
		pNode = m_stack.Last().pRSNode;
	else
		return;

	bool bIsEnd = true;

	pNode = PopObsoleteStackedItems( pNode );

	while( bIsEnd && pNode )
	{
		AkUInt16 newIndex = Select( m_stack.Last(), bIsEnd );
		
		if( !bIsEnd )
		{
			pNode = ( static_cast<CAkRSSub*>(pNode)->m_listChildren )[newIndex];
			AKASSERT( pNode );
			if( pNode->IsSegment() )
			{
				break;
			}
			else 
			{
				CAkRSSub* pSub = static_cast<CAkRSSub*>(pNode);
				if( !pSub->m_listChildren.Length()
					|| !pSub->HasSegmentLeaves() )
				{
					pNode = pNode->Parent();
				}
				else if( StackItem( pSub ) != AK_Success )
				{
					Term();
					return;
				}
			}
			bIsEnd = true; // Reset since we are on a new node
		}
		else
		{
			// We must go back one step, and take back the parent as curent node, if there is no parent, there is nothing to play.
			// If we are in step mode, no next.
			pNode = static_cast<CAkRSSub*>(pNode)->Parent();
			PopLast();
			pNode = PopObsoleteStackedItems( pNode );
		}
	}

	SetCurrentSegmentToNode( pNode );
}

AKRESULT AkRSIterator::JumpNextInternal()
{
	m_actualSegment = AK_MUSIC_TRANSITION_RULE_ID_NONE;
	m_bIsSegmentValid = true;

	CAkRSNode* pNode = NULL;

	if( !m_stack.IsEmpty() )
		pNode = m_stack.Last().pRSNode;
	else
		return AK_Fail;

	bool bIsEnd = true;

	while( bIsEnd && pNode )
	{
		AkUInt16 newIndex = Select( m_stack.Last(), bIsEnd );
		
		if( !bIsEnd )
		{
			pNode = ( static_cast<CAkRSSub*>(pNode)->m_listChildren )[newIndex];
			AKASSERT( pNode );
			if( pNode->IsSegment() )
			{
				break;
			}
			else if( StackItem( static_cast<CAkRSSub*>(pNode) ) != AK_Success )
			{
				Term();
				return AK_Fail;
			}
			bIsEnd = true; // Reset since we are on a new node
		}
		else
		{
			// We must go back one step, and take back the parent as curent node, if there is no parent, there is nothing to play.
			// If we are in step mode, no next.
			pNode = static_cast<CAkRSSub*>(pNode)->Parent();
			PopLast();
			pNode = PopObsoleteStackedItems( pNode );
		}
	}

	return SetCurrentSegmentToNode( pNode );
}

AKRESULT AkRSIterator::JumpTo( AkUniqueID in_playlistElementID )
{
	RevertGlobalRSInfo();
	FlushStack();
	m_actualSegment = AK_MUSIC_TRANSITION_RULE_ID_NONE;
	m_bIsSegmentValid = true;

	JumpToList jumpList;

	bool bFound = false;
	
	AKRESULT eResult = FindAndSelect( m_pRSCntr->GetPlaylistRoot(), in_playlistElementID, jumpList, bFound );

	if( bFound && eResult == AK_Success )
	{
		CAkRSNode* pNode = NULL;
		JumpToList::Iterator iter = jumpList.Begin();
		while( iter != jumpList.End() )
		{
			pNode = *iter;
			AKASSERT( pNode );
			if( pNode->IsSegment() )
			{
				break;
			}
			else
			{
				CAkRSSub* pSub = static_cast<CAkRSSub*>(pNode);

				if( StackItem( pSub ) != AK_Success )
				{
					Term();
					eResult = AK_Fail;
					break;
				}
				++iter;//We increment before the end of the loop since we will need the next iterator in the curent loop.

				if( iter != jumpList.End() )
				{
					// We will simulate a fisrt selection on the next item
					ForceSelect( *iter );
				}
				else
				{
					// The targeted item was a Sub
					eResult = JumpNextInternal();
					jumpList.Term();
					return eResult;
				}
			}
		}
		
		if( eResult == AK_Success )
		{
			eResult = SetCurrentSegmentToNode( pNode );
		}
	}
	else
	{
		eResult = AK_Fail;
	}

	jumpList.Term();

	return eResult;
}

AKRESULT AkRSIterator::FindAndSelect( 
						CAkRSNode* in_pNode, 
						AkUniqueID in_playlistElementID, 
						JumpToList& io_jmpList, 
						bool& io_bFound 
						)
{
	AKRESULT eResult = AK_Success;
	if( !io_jmpList.AddLast( in_pNode ) )
	{
		return AK_Fail;
	}
	else if( in_pNode->PlaylistID() == in_playlistElementID )
	{
		io_bFound = true;
	}
	else if( !in_pNode->IsSegment() )
	{
		// check on children
		CAkRSSub* pSub = static_cast<CAkRSSub*>( in_pNode );
		AkRSList::Iterator iter = pSub->m_listChildren.Begin();
		while( !io_bFound && iter != pSub->m_listChildren.End() )
		{
			eResult = FindAndSelect( *iter, in_playlistElementID, io_jmpList, io_bFound );
			if( eResult != AK_Success )
				return eResult;
			++iter;
		}
	}
	if( !io_bFound )
	{
		io_jmpList.RemoveLast();
	}
	return eResult;
}

CAkRSNode* AkRSIterator::PopObsoleteStackedItems( CAkRSNode* in_pNode )
{
	while( in_pNode && !static_cast<CAkRSSub*>(in_pNode)->IsContinuous() )
	{
		if( m_stack.Last().m_Loop.lLoopCount == 0 ) // Step looping infinitely, we keep it
		{
			break;
		}
		else if( m_stack.Last().m_Loop.lLoopCount > 1 )
		{
			--(m_stack.Last().m_Loop.lLoopCount);
			break;
		}
		else
		{
			in_pNode = static_cast<CAkRSSub*>(in_pNode)->Parent();
			PopLast();
		}
	}
	return in_pNode;
}

AKRESULT AkRSIterator::StackItem( CAkRSSub* in_pSub )
{ 
	RSStackItem item;
	if ( item.Init( in_pSub ) != AK_Success
		|| !m_stack.AddLast( item ) )
	{
		item.Clear();
		return AK_Fail;
	}
	return AK_Success;
}

AkUInt16 AkRSIterator::Select( RSStackItem & in_rStackItem, bool & out_bIsEnd )
{
	AkUInt16 newIndex = 0;

	CAkRSSub* pSub = in_rStackItem.pRSNode;

	switch( pSub->GetType() )
	{
	case RSType_ContinuousRandom:
	case RSType_StepRandom:
		newIndex = SelectRandomly( in_rStackItem, out_bIsEnd );
		break;
	case RSType_ContinuousSequence:
	case RSType_StepSequence:
		newIndex = SelectSequentially( in_rStackItem, out_bIsEnd );
		break;

	default:
		AKASSERT( !"Unhandled RSType" );
		break;
	}

	if ( pSub && !out_bIsEnd )
	{
		AkUInt32 numPlaylistItems = pSub->m_listChildren.Length();
		AkUInt32 uSelection = newIndex, uItemDone = out_bIsEnd;
		g_pPlayingMgr->MusicPlaylistCallback( m_playingID, pSub->PlaylistID(), numPlaylistItems, uSelection, uItemDone );
		if ( uSelection < numPlaylistItems )
			newIndex = (AkUInt16)uSelection;
		out_bIsEnd = ( uItemDone != 0 );
	}

	return newIndex;
}

void AkRSIterator::ForceSelect( CAkRSNode* in_pForcedNode )
{
	switch( static_cast<CAkRSSub*>( in_pForcedNode->Parent() )->GetType() )
	{
	case RSType_ContinuousRandom:
	case RSType_StepRandom:
		ForceSelectRandomly( in_pForcedNode );
		break;
	case RSType_ContinuousSequence:
	case RSType_StepSequence:
		ForceSelectSequentially( in_pForcedNode );
		break;

	default:
		AKASSERT( !"Unhandled RSType" );
		break;
	}
}

AkUInt16 AkRSIterator::SelectSequentially( RSStackItem & in_rStackItem, bool & out_bIsEnd )
{
	out_bIsEnd = false;
	CAkSequenceInfo* pSeqInfo = static_cast<CAkSequenceInfo*>( in_rStackItem.GetRSInfo() );
	if ( !pSeqInfo )
	{
		out_bIsEnd = true;
		return 0;
	}

	// Save a copy of the RS info if required.
	CAkRSSub* pSub = in_rStackItem.pRSNode;
	if ( m_bDoSaveOriginalGlobalRSInfo 
		&& pSub->IsGlobalRSInfo( pSeqInfo ) )
	{
		// Save RS info for possible reversal.
		SaveOriginalGlobalRSInfo( pSub, pSeqInfo );
	}

	if( ( pSeqInfo->m_i16LastPositionChosen + 1 ) == pSub->m_listChildren.Length() )//reached the end of the sequence
	{
		pSeqInfo->m_i16LastPositionChosen = 0;
		if(!CanContinueAfterCompleteLoop( &in_rStackItem.m_Loop ))
		{
			out_bIsEnd = true;
			return 0;
		}
	}
	else//not finished sequence
	{
		++( pSeqInfo->m_i16LastPositionChosen );
	}

	return pSeqInfo->m_i16LastPositionChosen;
}

AkUInt16 AkRSIterator::SelectRandomly( RSStackItem & in_rStackItem, bool & out_bIsEnd )
{
	out_bIsEnd = false;
	CAkRandomInfo* pRanInfo = static_cast<CAkRandomInfo*>( in_rStackItem.GetRSInfo() );
	if ( !pRanInfo )
	{
		out_bIsEnd = true;
		return 0;
	}

	AkUInt16 wPosition = 0;
	AkInt iValidCount = -1;
	AkInt iCycleCount = 0;

	CAkRSSub* pSub = in_rStackItem.pRSNode;
	AkRSList* pRSList = &( pSub->m_listChildren );

	if(!pRanInfo->m_wCounter)
	{
		if(!CanContinueAfterCompleteLoop( &in_rStackItem.m_Loop ))
		{
			out_bIsEnd = true;
			return 0;
		}
		pRanInfo->m_wCounter = (AkUInt16)pRSList->Length();
		pRanInfo->ResetFlagsPlayed(pRSList->Length());

		pRanInfo->m_ulRemainingWeight = pRanInfo->m_ulTotalWeight;
		for( CAkRandomInfo::AkAvoidList::Iterator iter = pRanInfo->m_listAvoid.Begin(); iter != pRanInfo->m_listAvoid.End(); ++iter )
		{
			pRanInfo->m_ulRemainingWeight -= ((*pRSList)[*iter])->GetWeight();
		}
		pRanInfo->m_wRemainingItemsToPlay -= (AkUInt16)pRanInfo->m_listAvoid.Length();
	}

	AKASSERT(pRanInfo->m_wRemainingItemsToPlay);//Should never be here if empty...

	if( pSub->IsUsingWeight() )
	{
		if( pRanInfo->m_ulRemainingWeight == 0 )
		{
			// This is and If and Assert Situation.
			// The reason: This crashed once on a divide by 0, but apparently no repro, and the callstack 
			// does not help identifying the problem.
			// Just in case it happens again, don't crash.
			AKASSERT( pRanInfo->m_ulRemainingWeight != 0 );
			pRanInfo->m_wCounter = 0; //Force reset next time
			return 0;
		}
		
		AkInt iRandomValue = pRanInfo->GetRandomValue();
		while(iValidCount < iRandomValue)
		{
			if(CanPlayPosition( pSub, pRanInfo, iCycleCount ))
			{
				iValidCount += ((*pRSList)[iCycleCount])->GetWeight();
			}
			++iCycleCount;
			AKASSERT(((size_t)(iCycleCount-1)) < pRSList->Length());
		}
	}
	else
	{
		if( pRanInfo->m_wRemainingItemsToPlay == 0 )
		{
			// This is an If and Assert Situation.
			// The reason: This crashed once on a divide by 0, but apparently no repro, and the callstack
			// does not help identifying the problem.
			// Just in case it happens again, don't crash.
			AKASSERT( pRanInfo->m_wRemainingItemsToPlay != 0 );
			pRanInfo->m_wCounter = 0; //Force reset next time
			return 0;
		}

		AkInt iRandomValue = (AkUInt16)(AKRANDOM::AkRandom() % pRanInfo->m_wRemainingItemsToPlay);
		while(iValidCount < iRandomValue)
		{
			if( CanPlayPosition( pSub, pRanInfo, iCycleCount ) )
			{
				++iValidCount;
			}
			++iCycleCount;
			AKASSERT(((size_t)(iCycleCount-1)) < pRSList->Length());
		}
	}
	wPosition = iCycleCount - 1;
	
	UpdateRandomItem( pSub, wPosition, pRSList, pRanInfo );

	return wPosition;
}

void AkRSIterator::ForceSelectRandomly( CAkRSNode* in_pForcedNode )
{
	CAkRSSub* pSub = static_cast<CAkRSSub*>( in_pForcedNode->Parent() );
	AkUInt16 wPosition = 0;
	for( AkRSList::Iterator iter = pSub->m_listChildren.Begin(); iter != pSub->m_listChildren.End(); ++iter )
	{
		if( *iter == in_pForcedNode )
		{
			break;
		}
		++wPosition;
	}

	CAkRandomInfo* pRanInfo = static_cast<CAkRandomInfo*>( m_stack.Last().GetRSInfo() );
	if ( !pRanInfo )
		return;	// Error failed to get RS info (out-of-memory).

	AkRSList* pRSList = &( pSub->m_listChildren );


	// Reset random info:
	// "Forcing Random" is arguably not legal: nothing prevents the caller
	// from selecting an node that should not have been selected through the
	// shuffle logic. The only thing we can do is to reset it...
	// But let's keep the avoid list, though.
	
	// Remove from Avoid List if applicable.
	pRanInfo->FlagAsUnBlocked( wPosition );
	pRanInfo->m_listAvoid.Remove( wPosition );
	
	pRanInfo->m_wCounter = (AkUInt16)pRSList->Length();
	pRanInfo->ResetFlagsPlayed(pRSList->Length());

	pRanInfo->m_ulRemainingWeight = pRanInfo->m_ulTotalWeight;
	for( CAkRandomInfo::AkAvoidList::Iterator iter = pRanInfo->m_listAvoid.Begin(); iter != pRanInfo->m_listAvoid.End(); ++iter )
	{
		pRanInfo->m_ulRemainingWeight -= ((*pRSList)[*iter])->GetWeight();
	}
	pRanInfo->m_wRemainingItemsToPlay -= (AkUInt16)pRanInfo->m_listAvoid.Length();
	
	UpdateRandomItem( pSub, wPosition, pRSList, pRanInfo );
}

void AkRSIterator::UpdateRandomItem( CAkRSSub* in_pSub, AkUInt16 in_wPosition, AkRSList* in_pRSList, CAkRandomInfo* in_pRanInfo )
{
	// Save a copy of the RS info if required.
	if ( m_bDoSaveOriginalGlobalRSInfo 
		&& in_pSub->IsGlobalRSInfo( in_pRanInfo ) )
	{
		// Save RS info for possible reversal.
		SaveOriginalGlobalRSInfo( in_pSub, in_pRanInfo );
	}

	if( in_pSub->RandomMode() == RandomMode_Normal )//Normal
	{
		if(!in_pRanInfo->IsFlagSetPlayed(in_wPosition))
		{
			in_pRanInfo->FlagSetPlayed( in_wPosition );
			in_pRanInfo->m_wCounter -=1;
		}
		if( in_pSub->AvoidRepeatCount() )
		{
			in_pRanInfo->m_wRemainingItemsToPlay -= 1;
			if( in_pRanInfo->m_listAvoid.AddLast(in_wPosition) == NULL )
			{
				// Reset counter( will force refresh on next play ).
				// and return position 0.
				in_pRanInfo->m_wCounter = 0;
				return;
			}
			in_pRanInfo->FlagAsBlocked( in_wPosition );
			in_pRanInfo->m_ulRemainingWeight -= ((*in_pRSList)[in_wPosition])->GetWeight();
			AkUInt16 uVal = (AkUInt16)( in_pRSList->Length()-1 );
			if(in_pRanInfo->m_listAvoid.Length() > AkMin( in_pSub->AvoidRepeatCount(), uVal ))
			{
				AkUInt16 wToBeRemoved = in_pRanInfo->m_listAvoid[0];
				in_pRanInfo->FlagAsUnBlocked(wToBeRemoved);
				in_pRanInfo->m_ulRemainingWeight += ((*in_pRSList)[wToBeRemoved])->GetWeight();
				in_pRanInfo->m_wRemainingItemsToPlay += 1;
				in_pRanInfo->m_listAvoid.Erase(0);
			}
		}
	}
	else//Shuffle
	{
		AkUInt16 wBlockCount = in_pSub->AvoidRepeatCount() ? in_pSub->AvoidRepeatCount() : 1 ;

		in_pRanInfo->m_ulRemainingWeight -= ((*in_pRSList)[in_wPosition])->GetWeight();
		in_pRanInfo->m_wRemainingItemsToPlay -= 1;
		in_pRanInfo->m_wCounter -=1;
		in_pRanInfo->FlagSetPlayed(in_wPosition);
		if( in_pRanInfo->m_listAvoid.AddLast( in_wPosition ) == NULL )
		{
			// Reset counter( will force refresh on next play ).
			// and return position 0.
			in_pRanInfo->m_wCounter = 0;
			return;
		}
		in_pRanInfo->FlagAsBlocked( in_wPosition );

		AkUInt16 uVal = (AkUInt16)( in_pRSList->Length()-1 );
		if(in_pRanInfo->m_listAvoid.Length() > AkMin( wBlockCount, uVal ))
		{
			AkUInt16 wToBeRemoved = in_pRanInfo->m_listAvoid[0];
			in_pRanInfo->m_listAvoid.Erase(0);
			in_pRanInfo->FlagAsUnBlocked(wToBeRemoved);
			if(!in_pRanInfo->IsFlagSetPlayed(wToBeRemoved))
			{
				in_pRanInfo->m_ulRemainingWeight += ((*in_pRSList)[wToBeRemoved])->GetWeight();
				in_pRanInfo->m_wRemainingItemsToPlay += 1;
			}
		}
	}
}

// Updates frozen global random info if required.
void AkRSIterator::RevertGlobalRSInfo()
{
	AKASSERT( m_bDoSaveOriginalGlobalRSInfo );

	// Overwrite RanInfo of all subs with orignal RanInfo stored in this map.
	GlobalRSInfoMap::Iterator it = m_arOriginalGlobalRSInfo.Begin();
	while ( it != m_arOriginalGlobalRSInfo.End() )
	{
		CAkRSSub * pSub = ( (*it).key );
		AKASSERT( pSub );
		pSub->OverwriteGlobalRSInfo( (*it).item );
		++it;
	}
	m_arOriginalGlobalRSInfo.RemoveAll();
}

// Updates frozen global random info if required.
void AkRSIterator::SaveOriginalGlobalRSInfo( CAkRSSub * in_pSub, CAkContainerBaseInfo * in_pRSInfo )
{
	AKASSERT( m_bDoSaveOriginalGlobalRSInfo );

	// Create, clone and save in map if the RSSub key does not exist already.
	if ( !m_arOriginalGlobalRSInfo.Exists( in_pSub ) )
	{
		// Try allocate new RanInfo structure.
		CAkContainerBaseInfo * pClone = in_pRSInfo->Clone( (AkUInt16)( in_pSub->m_listChildren.Length() ) );
		if ( pClone )
		{
			// Store.
			if ( !m_arOriginalGlobalRSInfo.Set( in_pSub, pClone ) )
			{
				// Failed populating or adding in map. Clean up.
				pClone->Destroy();
			}
		}
	}
}

void AkRSIterator::ForceSelectSequentially( CAkRSNode* in_pForcedNode )
{
	CAkRSSub* pParentSub = static_cast<CAkRSSub*>( in_pForcedNode->Parent() );
	AkUInt16 uIndex = 0;
	for( AkRSList::Iterator iter = pParentSub->m_listChildren.Begin(); iter != pParentSub->m_listChildren.End(); ++iter )
	{
		if( *iter == in_pForcedNode )
		{
			break;
		}
		++uIndex;
	}

	CAkSequenceInfo* pSeqInfo = static_cast<CAkSequenceInfo*>( m_stack.Last().GetRSInfo() );
	if ( !pSeqInfo )
		return;	// Error failed to get RS info (out-of-memory).

	// Save a copy of the RS info if required.
	CAkRSSub* pSub = m_stack.Last().pRSNode;
	if ( m_bDoSaveOriginalGlobalRSInfo 
		&& pSub->IsGlobalRSInfo( pSeqInfo ) )
	{
		// Save RS info for possible reversal.
		SaveOriginalGlobalRSInfo( pSub, pSeqInfo );
	}

	pSeqInfo->m_i16LastPositionChosen = uIndex;
}

bool AkRSIterator::CanContinueAfterCompleteLoop( AkLoop* io_pLoopingInfo/*= NULL*/ )
{
	bool bAnswer = false;
	if(!io_pLoopingInfo)
	{
		bAnswer = true;
	}
	else if(io_pLoopingInfo->bIsEnabled)
	{
		if(io_pLoopingInfo->bIsInfinite)
		{
			bAnswer = true;
		}
		else
		{
			--(io_pLoopingInfo->lLoopCount);
			bAnswer = (io_pLoopingInfo->lLoopCount)? true:false; 
		}
	}
	return bAnswer;
}

bool AkRSIterator::CanPlayPosition( CAkRSSub* in_pSub, CAkRandomInfo* in_pRandomInfo, AkUInt16 in_wPosition )
{
	if( in_pSub->RandomMode() == RandomMode_Normal )
	{
		if( in_pSub->AvoidRepeatCount() )
		{
			return !in_pRandomInfo->IsFlagBlocked(in_wPosition);
		}
		else
		{
			return true;
		}
	}
	else//Shuffle
	{
		return !in_pRandomInfo->IsFlagSetPlayed(in_wPosition) &&
			!in_pRandomInfo->IsFlagBlocked(in_wPosition);
	}
}

AKRESULT AkRSIterator::SetCurrentSegmentToNode( CAkRSNode* in_pNode )
{
	if( in_pNode )
	{
		m_actualSegment = static_cast<CAkRSSegment*>( in_pNode )->GetSegmentID();
		m_bIsSegmentValid = ( m_actualSegment != AK_INVALID_UNIQUE_ID );
		m_LastSegmentPlayingID = in_pNode->PlaylistID();
		m_uSegmentLoopCount = in_pNode->GetLoop();
		return AK_Success;
	}
	else
	{
		return AK_Fail;
	}
}
