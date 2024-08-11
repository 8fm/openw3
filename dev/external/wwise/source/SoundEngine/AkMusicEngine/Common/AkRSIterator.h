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
// AkRSIterator.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _MUSIC_RS_ITERATOR_H_
#define _MUSIC_RS_ITERATOR_H_

#include <AK/Tools/Common/AkArray.h>
#include "AkKeyArray.h"
#include "AkPoolSizes.h"
#include "AkRanSeqBaseInfo.h"
#include "AkParameters.h"
#include "AkMusicStructs.h"

class CAkMusicRanSeqCntr;
class CAkRSNode;
typedef AkArray<CAkRSNode*, CAkRSNode*, ArrayPoolDefault, DEFAULT_POOL_BLOCK_SIZE/sizeof(CAkRSNode*)> AkRSList;

//////////////////////////////
//
//////////////////////////////
class CAkRSNode
{
public:
	CAkRSNode( CAkRSNode* in_Parent )
		:m_pParent( in_Parent )
		, m_ID( AK_INVALID_UNIQUE_ID )
		, m_Weight( DEFAULT_RANDOM_WEIGHT )	
	{}

	virtual ~CAkRSNode(){}
	AkUInt16 GetLoop()    { return m_Loop; }
	void SetLoop( AkInt16 in_Loop )  { m_Loop = in_Loop; }

	virtual bool IsSegment(){ return false; }

	AkUInt32 GetWeight(){ return m_Weight; }
	void SetWeight( AkUInt32 in_Weight ){ m_Weight = in_Weight; }

	CAkRSNode* Parent(){ return m_pParent; }

	void PlaylistID( AkUniqueID in_ID ){ m_ID = in_ID; }
	AkUniqueID PlaylistID(){ return m_ID; }

private:
	CAkRSNode* m_pParent;
	AkUniqueID m_ID;		// for jump to direct item in sequence after switch.
	AkUInt16 m_Loop;
	AkUInt32 m_Weight;
};

//////////////////////////////
//
//////////////////////////////
class CAkRSSub : public CAkRSNode
{
	
public:
	CAkRSSub( CAkRSNode* in_Parent )
		:CAkRSNode( in_Parent )
		,m_eRSType( RSType_ContinuousSequence )
		,m_pGlobalCntrBaseInfo( NULL )
		,m_bIsUsingWeight( false )
		,m_bIsShuffle( true )
		,m_bHasSegmentLeaves( false )
		,m_wAvoidRepeatCount( 0 )		
	{}

	virtual ~CAkRSSub();

	void Clear();

	RSType GetType(){ return m_eRSType; }
	void SetType( RSType in_Type ){ m_eRSType = in_Type; }

	bool HasSegmentLeaves() { return m_bHasSegmentLeaves; }
	void WasSegmentLeafFound();

	bool IsContinuous()
	{ 
		return (
			m_eRSType == RSType_ContinuousSequence
			|| m_eRSType == RSType_ContinuousRandom );
	}

	bool IsUsingWeight(){ return m_bIsUsingWeight; }
	void IsUsingWeight( bool in_UseWeight ){ m_bIsUsingWeight = in_UseWeight; }

	AkUInt16 AvoidRepeatCount(){ return m_wAvoidRepeatCount; }
	void AvoidRepeatCount( AkUInt16 in_wAvoidRepeatCount ){ m_wAvoidRepeatCount = in_wAvoidRepeatCount; }

	AkRandomMode RandomMode();
	void RandomMode( bool in_bIsShuffle ){m_bIsShuffle = in_bIsShuffle;}

	AkUInt32 CalculateTotalWeight();

	CAkContainerBaseInfo* GetGlobalRSInfo();
	bool IsGlobalRSInfo( CAkContainerBaseInfo * in_pRSInfo ) { return in_pRSInfo == m_pGlobalCntrBaseInfo; }
	void OverwriteGlobalRSInfo( CAkContainerBaseInfo * in_pRSInfo );
	
	AkRSList m_listChildren;

private:
	
	RSType m_eRSType;
	CAkContainerBaseInfo* m_pGlobalCntrBaseInfo;//for Step Global information
	bool m_bIsUsingWeight;
	bool m_bIsShuffle;
	bool m_bHasSegmentLeaves;
	AkUInt16 m_wAvoidRepeatCount;
};

//////////////////////////////
//
//////////////////////////////
class CAkRSSegment : public CAkRSNode
{
public:

	CAkRSSegment( CAkRSNode* in_Parent )
		:CAkRSNode( in_Parent )
	{
	}

	virtual ~CAkRSSegment()
	{
	}

	virtual bool IsSegment(){ return true; }

	AkUniqueID GetSegmentID(){ return m_SegmentID; }
	void SetSegmentID( AkUniqueID in_uSegmentID ){ m_SegmentID = in_uSegmentID; }

private:
	AkUniqueID m_SegmentID;
};

//////////////////////////////
//
//////////////////////////////
class RSStackItem
{
public:
	RSStackItem();

	AKRESULT Init( CAkRSSub * in_pSub );
	void Clear();

	CAkContainerBaseInfo * GetRSInfo()
	{
		if ( pRSNode->GetType() == RSType_StepSequence
			|| pRSNode->GetType() == RSType_StepRandom )
		{
			AKASSERT( !pLocalRSInfo );	// This field is always cleared when Sub's type is StepXX (uses global info).
			return pRSNode->GetGlobalRSInfo();
		}
		return pLocalRSInfo;
	}

	CAkRSSub* pRSNode;
	AkLoop m_Loop;

private:
	RSStackItem( RSStackItem& );
	CAkContainerBaseInfo* pLocalRSInfo;
};

//////////////////////////////
//
//////////////////////////////
class AkRSIterator
{
public:
	AkRSIterator( CAkMusicRanSeqCntr* in_pRSCntr );
	~AkRSIterator();

	AKRESULT Init( AkPlayingID in_playingID );
	void EndInit();	// Call when no more JumpTo will be called.

	void Term();

	AkUniqueID operator++()
	{
		JumpNext();
		return GetCurrentSegment();
	}

	AkUniqueID operator*()
	{
		return GetCurrentSegment();
	}

	AKRESULT JumpTo( AkUniqueID in_playlistElementID );

	// IsValid() returns false if there was a memory error OR if the end of the playlist was reached.
	inline bool IsValid(){ return m_bIsSegmentValid; }
	
	// IMPORTANT. This should only be used when an error occurs.
	void SetAsInvalid(){ m_bIsSegmentValid = false; }

	AkUniqueID GetCurrentSegment(){ return m_actualSegment; }

	AkUniqueID GetPlaylistItem(){ return m_LastSegmentPlayingID; }

private:

	void FlushStack();
	void PopLast();

	void RevertGlobalRSInfo();
	void SaveOriginalGlobalRSInfo( CAkRSSub * in_pSub, CAkContainerBaseInfo * in_pRanInfo );

	typedef AkArray< CAkRSNode*, CAkRSNode*,ArrayPoolDefault, DEFAULT_POOL_BLOCK_SIZE/sizeof(CAkRSNode*)> JumpToList;

	void JumpNext();

	AKRESULT JumpNextInternal();

	AKRESULT FindAndSelect( CAkRSNode* in_pNode, AkUniqueID in_playlistElementID, JumpToList& io_jmpList, bool& io_bFound );

	CAkRSNode* PopObsoleteStackedItems( CAkRSNode* in_pNode );

	AKRESULT StackItem( CAkRSSub* in_pSub );

	AkUInt16 Select( RSStackItem & in_rStackItem, bool & out_bIsEnd );

	//Used in JumpTo feature.
	void ForceSelect( CAkRSNode* in_pForcedNode );

	// Select Randomly a sound to play from the specified set of parameters
	//
	// Return - AkUInt16 - Position in the playlist to play
	AkUInt16 SelectRandomly( RSStackItem & in_rStackItem, bool & out_bIsEnd );

	// Select SelectSequentially a sound to play from the specified set of parameters
	//
	// Return - AkUInt16 - Position in the playlist to play
	AkUInt16 SelectSequentially( RSStackItem & in_rStackItem, bool & out_bIsEnd );

	//Used in JumpTo feature.
	void ForceSelectRandomly( CAkRSNode* in_pForcedNode );
	void ForceSelectSequentially( CAkRSNode* in_pForcedNode );

	void UpdateRandomItem( CAkRSSub* in_pSub, AkUInt16 in_wPosition, AkRSList* in_pRSList, CAkRandomInfo* in_pRanInfo );

	// Function called once a complete loop is completed.
	// 
	// Return - bool - true if the loop must continue, false if nothing else has to be played
	bool CanContinueAfterCompleteLoop(
		AkLoop* io_pLoopingInfo			// Looping information (not const)
		);

	// Gets if the playlist position can be played(already played and avoid repeat)
	//
	// Return - bool - true if can play it, false otherwise
	bool CanPlayPosition(
		CAkRSSub* in_pSub,
		CAkRandomInfo* in_pRandomInfo,	// Container set of parameters
		AkUInt16 in_wPosition				// Position in the list
		);

	AKRESULT SetCurrentSegmentToNode( CAkRSNode* in_pNode );

private:
	CAkMusicRanSeqCntr* m_pRSCntr; // required to access globally scoped items.

	AkPlayingID m_playingID;

	typedef AkArray<RSStackItem, const RSStackItem&, ArrayPoolDefault, DEFAULT_POOL_BLOCK_SIZE/sizeof(RSStackItem)> IteratorStack;
	IteratorStack m_stack;
	AkUniqueID m_actualSegment;
	AkUniqueID m_LastSegmentPlayingID;
	bool m_bIsSegmentValid;
	
	// Global random info delayed update.
	typedef CAkKeyArray<CAkRSSub*, CAkContainerBaseInfo*> GlobalRSInfoMap;
	GlobalRSInfoMap m_arOriginalGlobalRSInfo;
	AkUInt16 m_bDoSaveOriginalGlobalRSInfo		:1;
	
	AkInt16 m_uSegmentLoopCount;	
};

#endif //_MUSIC_RS_ITERATOR_H_


