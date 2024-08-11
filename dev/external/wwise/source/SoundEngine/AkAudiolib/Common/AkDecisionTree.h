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

#ifndef AK_DECISION_TREE_H_
#define AK_DECISION_TREE_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Tools/Common/AkArray.h>
#include "AkKeyArray.h"

class AkDecisionTree
{
public:
	enum Mode
	{
		Mode_BestMatch	= 0,
		Mode_Weighted	= 1
	};

	struct Children
	{
		AkUInt16 uIdx;
		AkUInt16 uCount;
	};

	struct Node
	{
		AkArgumentValueID key; // Not AkUInt16 to make alignment explicit

		union
		{
			Children children;
			AkUniqueID audioNodeID;
		};

		AkUInt16 uWeight;
		AkUInt16 uProbability;
	};

	struct WeightedDecisionInfo
	{
		AkUInt8 uWeightedDecisionType;	// AkMonitorData::DialogueWeightedDecisionType
		AkUInt32 uWeightedPossibleCount;
		AkUInt32 uWeightedTotalCount;
	};

	AkDecisionTree();
	~AkDecisionTree();

	AKRESULT SetTree( void* in_pData, AkUInt32 in_uSize, AkUInt32 in_uDepth );

	AkUniqueID ResolvePath( AkUniqueID in_idEvent, AkArgumentValueID * in_pPath, AkUInt32 in_cPath, AkPlayingID in_idSequence );

	AkUInt32 Depth() const { return m_uDepth; }

	void SetProbability( AkUInt16 in_uProbability ) { m_uProbability = in_uProbability; }
	void SetMode( AkUInt8 in_uMode ) { m_uMode = in_uMode; }

	//For backwards compatibility with the old music switch container types.  
	// Theses two functions will fail and return false if the tree depth is greater than 1.
	typedef CAkKeyArray<AkUInt32,AkUniqueID> SwitchNodeAssoc;
	bool GetSwitchNodeAssoc(SwitchNodeAssoc& out_switchNodeAssoc) const;
	AkUniqueID GetAudioNodeForState( AkUInt32 in_stateID ) const;

private:
	typedef AkArray<Node *, Node *, ArrayPoolDefault, 4> NodeArray;

	struct WeightedCandidates
	{
		~WeightedCandidates() { nodes.Term(); }

		NodeArray nodes;
		AkUInt32 uCount100;
		AkUInt32 uCount0;
		AkUInt32 uTotalWeight;
	};

	void AddCandidate( Node * pNode, WeightedCandidates & io_candidates );
	Node * ResolvePathWeighted( AkArgumentValueID * in_pPath, AkUInt32 in_cPath, AkUniqueID in_idEvent, AkPlayingID in_idSequence, WeightedDecisionInfo& out_decisionInfo );
	void _ResolvePathWeighted( Node * in_pRootNode, AkArgumentValueID * in_pPath, AkUInt32 in_cPath, WeightedCandidates & io_candidates );

	Node * _ResolvePath( Node * in_pRootNode, AkArgumentValueID * in_pPath, AkUInt32 in_cPath );

	Node * BinarySearch( Node * in_pNodes, AkUInt32 in_cNodes, AkArgumentValueID in_key );

	AkUInt32 m_uDepth;
	Node * m_pNodes;
	AkUInt16 m_uProbability;
	AkUInt8 m_uMode; // enum Mode
};

#endif
