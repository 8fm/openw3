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
#include <AK/Tools/Common/AkAssert.h>
#include <AK/Tools/Common/AkObject.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

#include "AkDecisionTree.h"
#include "AkRandom.h"
#include "AkMonitor.h"

AkDecisionTree::AkDecisionTree()
	: m_uDepth( 0 )
	, m_pNodes( NULL )
	, m_uProbability(100)
	, m_uMode(Mode_BestMatch)
{
	AKASSERT( sizeof( Node ) == 12 ); // make sure we catch platforms where this ain't so. persistence depends on it.
}

AkDecisionTree::~AkDecisionTree()
{
	if ( m_pNodes )
		AkFree( g_DefaultPoolId, m_pNodes );
}

AKRESULT AkDecisionTree::SetTree( void* in_pData, AkUInt32 in_uSize, AkUInt32 in_uDepth )
{
	if ( m_pNodes ) 
	{
		AkFree( g_DefaultPoolId, m_pNodes );
		m_pNodes = NULL;
	}
	
	if( in_uSize )
	{
		m_pNodes = (AkDecisionTree::Node *) AkAlloc( g_DefaultPoolId, in_uSize );
		if ( !m_pNodes )
			return AK_InsufficientMemory;

		memcpy( m_pNodes, in_pData, in_uSize );
	}

	m_uDepth = in_uDepth;

	return AK_Success;
}

void AkDecisionTree::AddCandidate( Node * pNode, WeightedCandidates & io_candidates )
{
	Node ** ppNode = io_candidates.nodes.AddLast();
	if ( ppNode )
	{
		*ppNode = pNode;

		AkUInt16 uWeight = pNode->uWeight;

		if ( uWeight == 100 )
			++io_candidates.uCount100;
		else if ( uWeight == 0 )
			++io_candidates.uCount0;

		io_candidates.uTotalWeight += uWeight;
	}
}

AkDecisionTree::Node * AkDecisionTree::ResolvePathWeighted( AkArgumentValueID * in_pPath, AkUInt32 in_cPath, AkUniqueID in_idEvent, AkPlayingID in_idSequence, WeightedDecisionInfo& out_decisionInfo )
{
	AKASSERT( in_pPath );

	WeightedCandidates candidates;
	candidates.uCount100 = 0;
	candidates.uCount0 = 0;
	candidates.uTotalWeight = 0;

	_ResolvePathWeighted( m_pNodes, in_pPath, in_cPath, candidates );

	if( candidates.nodes.IsEmpty() )
	{
		out_decisionInfo.uWeightedDecisionType = (AkUInt8)AkMonitorData::DialogueWeightedDecisionType_NoCandidate;
	}
	else
	{
		if ( candidates.uCount100 ) // 100-weights have priority 
		{
			AkUInt32 uChoice = AKRANDOM::AkRandom() % candidates.uCount100;
			for ( NodeArray::Iterator it = candidates.nodes.Begin(); it != candidates.nodes.End(); ++it )
			{
				if ( (*it)->uWeight == 100 && uChoice-- == 0 )
				{
					out_decisionInfo.uWeightedDecisionType = (AkUInt8)AkMonitorData::DialogueWeightedDecisionType_100;
					out_decisionInfo.uWeightedPossibleCount = candidates.uCount100;
					out_decisionInfo.uWeightedTotalCount = candidates.nodes.Length();
					return *it;
				}
			}
		}
		else if ( candidates.nodes.Length() > candidates.uCount0 ) // Consider weight values from 1 to 99
		{
			AkUInt32 uChoice = AKRANDOM::AkRandom() % candidates.uTotalWeight;
			for ( NodeArray::Iterator it = candidates.nodes.Begin(); it != candidates.nodes.End(); ++it )
			{
				if ( uChoice < (*it)->uWeight )
				{
					out_decisionInfo.uWeightedDecisionType = (AkUInt8)AkMonitorData::DialogueWeightedDecisionType_1to99;
					out_decisionInfo.uWeightedPossibleCount = candidates.nodes.Length() - candidates.uCount0;
					out_decisionInfo.uWeightedTotalCount = candidates.nodes.Length();
					return *it;
				}
				uChoice -= (*it)->uWeight;
			}
		}
		else // only consider 0-weights when there are no candidates with >0 weight
		{
			AKASSERT( candidates.uCount0 );

			AkUInt32 uChoice = AKRANDOM::AkRandom() % candidates.uCount0;
			for ( NodeArray::Iterator it = candidates.nodes.Begin(); it != candidates.nodes.End(); ++it )
			{
				if ( (*it)->uWeight == 0 && uChoice-- == 0 )
				{
					out_decisionInfo.uWeightedDecisionType = (AkUInt8)AkMonitorData::DialogueWeightedDecisionType_0;
					out_decisionInfo.uWeightedPossibleCount = candidates.uCount0;
					out_decisionInfo.uWeightedTotalCount = candidates.nodes.Length();
					return *it;
				}
			}
		}
	}

	return NULL;
}

void AkDecisionTree::_ResolvePathWeighted( Node * in_pRootNode, AkArgumentValueID * in_pPath, AkUInt32 in_cPath, WeightedCandidates & io_candidates )
{
	if( m_pNodes )
	{
		Node * pChildrenNodes = m_pNodes + in_pRootNode->children.uIdx;
		Node * pNode = BinarySearch( pChildrenNodes, in_pRootNode->children.uCount, *in_pPath );

		// handle direct match

		if ( pNode ) 
		{
			if ( in_cPath == 1 )
			{
				AddCandidate( pNode, io_candidates );
			}
			else
			{
				_ResolvePathWeighted( pNode, in_pPath + 1, in_cPath - 1, io_candidates );
			}
		}

		// search for *
		// guaranteed to be at index 0 if present

		if ( pChildrenNodes[ 0 ].key == AK_FALLBACK_ARGUMENTVALUE_ID
			&& *in_pPath != AK_FALLBACK_ARGUMENTVALUE_ID )
		{
			if ( in_cPath == 1 )
			{
				AddCandidate( pChildrenNodes + 0, io_candidates );
			}
			else
			{
				_ResolvePathWeighted( pChildrenNodes, in_pPath + 1, in_cPath - 1, io_candidates );
			}
		}
	}
}

AkUniqueID AkDecisionTree::ResolvePath( AkUniqueID in_idEvent, AkArgumentValueID * in_pPath, AkUInt32 in_cPath, AkPlayingID in_idSequence )
{
	AKASSERT( in_cPath == 0 || in_pPath );

	if ( in_cPath != Depth() )
		return AK_INVALID_UNIQUE_ID;

	Node * pResolved = NULL;

	WeightedDecisionInfo weightedDecisionInfo;
	weightedDecisionInfo.uWeightedDecisionType = (AkUInt8)AkMonitorData::DialogueWeightedDecisionType_NotWeighted;
	weightedDecisionInfo.uWeightedPossibleCount = 0;
	weightedDecisionInfo.uWeightedTotalCount = 0;

	if ( in_cPath == 0 )
	{
		pResolved = m_pNodes;
	}
	else if ( m_uMode == Mode_BestMatch )
	{
		pResolved = _ResolvePath( m_pNodes, in_pPath, in_cPath );
	}
	else
	{
		AKASSERT( m_uMode == Mode_Weighted );

		pResolved = ResolvePathWeighted( in_pPath, in_cPath, in_idEvent, in_idSequence, weightedDecisionInfo );
	}

	const AkUInt16 kMaxProbability = 100*100;	// dialogue event probability * path probability
	AkUInt16 uRandom = kMaxProbability;
	AkUInt16 uTotalProbability = kMaxProbability;// 0..10000
	bool bRejected = false;

	// Apply playback probability, which is a combination of tree-level and node-level probabilities.
	if ( pResolved && ( m_uProbability < 100 || pResolved->uProbability < 100 ) )
	{
		uRandom = (AkUInt16)(((double) AKRANDOM::AkRandom() / (double) AKRANDOM::AK_RANDOM_MAX) * (double)kMaxProbability);
		uTotalProbability = m_uProbability * pResolved->uProbability;// 0..10000
		if ( uTotalProbability <= uRandom )
		{
			// Just remember it was resolved but luck decided otherwise.
			bRejected = true;
		}
	}

	AkUniqueID audioNodeID = pResolved ? pResolved->audioNodeID : AK_INVALID_UNIQUE_ID;

	MONITOR_RESOLVEDIALOGUE( in_idEvent, audioNodeID, in_cPath, in_pPath, in_idSequence, uRandom, uTotalProbability, weightedDecisionInfo.uWeightedDecisionType, weightedDecisionInfo.uWeightedPossibleCount, weightedDecisionInfo.uWeightedTotalCount );

	return bRejected ? 0 : audioNodeID;
}

AkDecisionTree::Node * AkDecisionTree::_ResolvePath( Node * in_pRootNode, AkArgumentValueID * in_pPath, AkUInt32 in_cPath )
{
	if( m_pNodes )
	{
		Node * pChildrenNodes = m_pNodes + in_pRootNode->children.uIdx;
		Node * pNode = BinarySearch( pChildrenNodes, in_pRootNode->children.uCount, *in_pPath );

		// handle direct match

		if ( pNode ) 
		{
			if ( in_cPath == 1 )
				return pNode;

			Node * pResolved = _ResolvePath( pNode, in_pPath + 1, in_cPath - 1 );
			if ( pResolved )
				return pResolved;
		}

		// if no direct match, search for *
		// guaranteed to be at index 0 if present

		if ( pChildrenNodes[ 0 ].key == AK_FALLBACK_ARGUMENTVALUE_ID
			&& *in_pPath != AK_FALLBACK_ARGUMENTVALUE_ID )
		{
			if ( in_cPath == 1 )
				return pChildrenNodes + 0;

			Node * pResolved = _ResolvePath( pChildrenNodes, in_pPath + 1, in_cPath - 1 );
			if ( pResolved )
				return pResolved;
		}
	}

	return NULL;
}

AkDecisionTree::Node * AkDecisionTree::BinarySearch( Node * in_pNodes, AkUInt32 in_cNodes, AkArgumentValueID in_key )
{
	AKASSERT( in_pNodes );

	AkInt32 uTop = 0, uBottom = in_cNodes-1;

	// binary search for key
	do
	{
		AkInt32 uThis = ( uBottom - uTop ) / 2 + uTop; 
		if ( in_pNodes[ uThis ].key > in_key ) 
			uBottom = uThis - 1;
		else if ( in_pNodes[ uThis ].key < in_key ) 
			uTop = uThis + 1;
		else
			return in_pNodes + uThis;
	}
	while ( uTop <= uBottom );

	return NULL;
}


bool AkDecisionTree::GetSwitchNodeAssoc(AkDecisionTree::SwitchNodeAssoc& out_switchNodeAssoc) const
{
	if (AK_EXPECT_FALSE(Depth() != 1))
		return false;

	AkUInt32 uCount = m_pNodes->children.uCount;
	for (AkUInt32 i=0; i<uCount; ++i)
	{
		out_switchNodeAssoc.Set( m_pNodes[i].key, m_pNodes[i].audioNodeID );
	}

	return true;
}

AkUniqueID AkDecisionTree::GetAudioNodeForState( AkUInt32 in_stateID ) const
{
	if ( Depth() == 1 )
	{
		AkUInt32 uCount = m_pNodes->children.uCount;
		for (AkUInt32 i=0; i<uCount; ++i)
		{
			if (in_stateID == m_pNodes[i].key)
				return m_pNodes[i].audioNodeID;
		}
	}
	return AK_INVALID_UNIQUE_ID;
}
