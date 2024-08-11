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
// AkAudioLibIndex.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkAudioLibIndex.h"
#include "AkCritical.h"
#include "AkRanSeqCntr.h"
#include "AkPrivateTypes.h"
#include "AkFxBase.h"
#include "AkAttenuationMgr.h"
#include "AkDynamicSequence.h"
#include "AkPlayingMgr.h"

extern CAkPlayingMgr* g_pPlayingMgr;

void CAkAudioLibIndex::Init()
{
	m_idxAudioNode.Init();
	m_idxBusses.Init();
	m_idxCustomStates.Init();
	m_idxEvents.Init();
	m_idxActions.Init();
	m_idxLayers.Init();
	m_idxAttenuations.Init();
	m_idxDynamicSequences.Init();
	m_idxDialogueEvents.Init();
	m_idxFxShareSets.Init();
	m_idxFxCustom.Init();
}

void CAkAudioLibIndex::Term()
{
	m_idxAudioNode.Term();
	m_idxBusses.Term();
	m_idxCustomStates.Term();
	m_idxEvents.Term();
	m_idxActions.Term();
	m_idxLayers.Term();
	m_idxAttenuations.Term();
	m_idxDynamicSequences.Term();
	m_idxDialogueEvents.Term();
	m_idxFxShareSets.Term();
	m_idxFxCustom.Term();
}

void CAkAudioLibIndex::ReleaseTempObjects()
{
	{
		CAkIndexItem<CAkParameterNodeBase*>& l_rIdx = m_idxAudioNode;

		AkAutoLock<CAkLock> IndexLock( l_rIdx.m_IndexLock );

		CAkIndexItem<CAkParameterNodeBase*>::AkMapIDToPtr::Iterator iter = l_rIdx.m_mapIDToPtr.Begin();
		while( iter != l_rIdx.m_mapIDToPtr.End() )
		{
			CAkParameterNodeBase* pNode = static_cast<CAkParameterNodeBase*>( *iter );
			++iter;
			if( pNode->ID() & AK_SOUNDENGINE_RESERVED_BIT )
			{
				pNode->Release();
			}
		}
	}

	{
		CAkIndexItem<CAkFxCustom*>& l_rIdx = m_idxFxCustom;

		CAkIndexItem<CAkFxCustom*>::AkMapIDToPtr::Iterator iter = l_rIdx.m_mapIDToPtr.Begin();
		while( iter != l_rIdx.m_mapIDToPtr.End() )
		{
			CAkFxCustom* pCustom = static_cast<CAkFxCustom*>( *iter );
			++iter;
			if( pCustom->ID() & AK_SOUNDENGINE_RESERVED_BIT )
			{
				pCustom->Release();
			}
		}
	}
}
void CAkAudioLibIndex::ReleaseDynamicSequences()
{
	CAkIndexItem<CAkDynamicSequence*>& l_rIdx = m_idxDynamicSequences;

	AkAutoLock<CAkLock> IndexLock( l_rIdx.m_IndexLock );

	CAkIndexItem<CAkDynamicSequence*>::AkMapIDToPtr::Iterator iter = l_rIdx.m_mapIDToPtr.Begin();
	while( iter != l_rIdx.m_mapIDToPtr.End() )
	{
		CAkDynamicSequence* pNode = static_cast<CAkDynamicSequence*>( *iter );
		++iter;
		g_pPlayingMgr->RemoveItemActiveCount( pNode->GetPlayingID() );
		pNode->Release();	
	}
}

CAkParameterNodeBase* CAkAudioLibIndex::GetNodePtrAndAddRef( AkUniqueID in_ID, AkNodeType in_NodeType )
{
	if( in_NodeType == AkNodeType_Default )
	{
		return m_idxAudioNode.GetPtrAndAddRef( in_ID );
	}
	else
	{
		AKASSERT( in_NodeType == AkNodeType_Bus );
		return m_idxBusses.GetPtrAndAddRef( in_ID );
	}
}

CAkParameterNodeBase* CAkAudioLibIndex::GetNodePtrAndAddRef( WwiseObjectIDext& in_rIDext )
{
	return GetNodePtrAndAddRef( in_rIDext.id, in_rIDext.GetType() );
}

CAkLock& CAkAudioLibIndex::GetNodeLock( AkNodeType in_NodeType )
{
	if( in_NodeType == AkNodeType_Default )
	{
		return m_idxAudioNode.GetLock();
	}
	else
	{
		AKASSERT( in_NodeType == AkNodeType_Bus );
		return m_idxBusses.GetLock();
	}
}

CAkIndexItem<CAkParameterNodeBase*>& CAkAudioLibIndex::GetNodeIndex( AkNodeType in_NodeType )
{
	if( in_NodeType == AkNodeType_Default )
	{
		return m_idxAudioNode;
	}
	else
	{
		AKASSERT( in_NodeType == AkNodeType_Bus );
		return m_idxBusses;
	}
}


#ifndef AK_OPTIMIZED
AKRESULT CAkAudioLibIndex::ResetRndSeqCntrPlaylists()
{
	for( CAkIndexItem<CAkParameterNodeBase*>::AkMapIDToPtr::Iterator iter = m_idxAudioNode.m_mapIDToPtr.Begin(); iter != m_idxAudioNode.m_mapIDToPtr.End(); ++iter )
	{
		CAkParameterNodeBase* pNode = static_cast<CAkParameterNodeBase*>( *iter );
		if( pNode->NodeCategory() == AkNodeCategory_RanSeqCntr )
		{
			static_cast<CAkRanSeqCntr*>( pNode )->ResetSpecificInfo();
		}
	}
	return AK_Success;
}
void CAkAudioLibIndex::ClearMonitoringSoloMute()
{
	if ( CAkParameterNodeBase::IsMonitoringMuteSoloActive() )
	{
		for( CAkIndexItem<CAkParameterNodeBase*>::AkMapIDToPtr::Iterator iter = m_idxAudioNode.m_mapIDToPtr.Begin(); iter != m_idxAudioNode.m_mapIDToPtr.End(); ++iter )
		{
			CAkParameterNodeBase* pNode = static_cast<CAkParameterNodeBase*>( *iter );
			pNode->MonitoringSolo( false );
			pNode->MonitoringMute( false );
		}
		CAkParameterNodeBase::SetMonitoringMuteSoloDirty();
	}
}
#endif

AKRESULT CAkIndexSiblingItem::SetIDToPtr( AkStateGroupID in_StateGroupID, CAkState* in_Ptr )
{
	AkAutoLock<CAkLock> IndexLock( m_IndexLock );

	CAkIndexItem<CAkState*>* pIndex = GetOrCreateStateGroup( in_StateGroupID );
	if( pIndex )
	{
		pIndex->SetIDToPtr( in_Ptr );
		return AK_Success;
	}
	else
	{
		return AK_InsufficientMemory;
	}
}

//Remove an ID from the index
void CAkIndexSiblingItem::RemoveID( AkStateGroupID in_StateGroupID, AkUniqueID in_ID )
{
	AkAutoLock<CAkLock> IndexLock( m_IndexLock );
	
	CAkIndexItem<CAkState*>* pIndex = GetStateGroup( in_StateGroupID );
	if( pIndex )
	{
		pIndex->RemoveID( in_ID );
	}
}

CAkState* CAkIndexSiblingItem::GetPtrAndAddRef( AkStateGroupID in_StateGroupID, AkUniqueID in_ID ) 
{ 
	AkAutoLock<CAkLock> IndexLock( m_IndexLock ); 

	CAkIndexItem<CAkState*>* pIndex = GetStateGroup( in_StateGroupID );
	if( pIndex )
	{
		return pIndex->GetPtrAndAddRef( in_ID );
	}
	else
	{
		return NULL;
	}
} 

void CAkIndexSiblingItem::Term()
{
	for( AkMapSibling::Iterator iter = m_ArrayStateGroups.Begin(); iter != m_ArrayStateGroups.End(); ++iter )
	{
		(*iter).item->Term();
		AkDelete( g_DefaultPoolId, (*iter).item );
	}

	//then term the array
	m_ArrayStateGroups.Term();
}

CAkIndexItem<CAkState*>* CAkIndexSiblingItem::GetStateGroup( AkStateGroupID in_StateGroupID )
{
	CAkIndexItem<CAkState*>** l_ppIndex = m_ArrayStateGroups.Exists( in_StateGroupID );
	if( l_ppIndex )
	{
		return *l_ppIndex;
	}
	return NULL;
}

CAkIndexItem<CAkState*>* CAkIndexSiblingItem::GetOrCreateStateGroup( AkStateGroupID in_StateGroupID )
{
	CAkIndexItem<CAkState*>* pIndex = GetStateGroup( in_StateGroupID );
	if( !pIndex )
	{
		AkNew2( pIndex, g_DefaultPoolId, CAkIndexItem<CAkState*>, CAkIndexItem<CAkState*>() );
		if( pIndex )
		{
			pIndex->Init();
			
			if( !m_ArrayStateGroups.Set( in_StateGroupID, pIndex ) )
			{
				pIndex->Term();
				AkDelete( g_DefaultPoolId, pIndex );
				pIndex = NULL;
			}
		}
	}	
	return pIndex;
}

