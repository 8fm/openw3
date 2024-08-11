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
// AkSequenceCtx.h
//
// Music RanSeq Container Context.
//
//////////////////////////////////////////////////////////////////////
#ifndef _SEQUENCE_CTX_H_
#define _SEQUENCE_CTX_H_

#include "AkSegmentChain.h"
#include "AkRegisteredObj.h"
#include "PrivateStructures.h"
#include "AkRSIterator.h"

#include "AkMatrixSequencer.h"

class CAkMusicRanSeqCntr;

class CAkSequenceCtx : public CAkChainCtx
{
public:
    CAkSequenceCtx(
        CAkMusicRanSeqCntr *in_pSequenceNode,
        CAkMusicCtx *       in_pParentCtx
        );
    virtual ~CAkSequenceCtx();

    // Initializes the context. Automatically performs a one-segment look-ahead.
    // Returns first segment that was scheduled in the sequencer.
	// IMPORTANT. OnStops itself on failure.
    AKRESULT Init(
        CAkRegisteredObj *  in_GameObject,
        UserParams &        in_rUserparams
        );
	// Called by parent (switches): completes the initialization.
	// Sequences set their iterator ready.
	virtual void EndInit();


    // Matrix Aware Context implementation.
    // ----------------------------------------------------------

    // For Music Renderer's music contexts look-up.
    virtual CAkMusicNode * Node();

    CAkMusicRanSeqCntr * SequenceNode() { return m_pSequenceNode; }
    // ----------------------------------------------------------

    // Initializes playlist iterator so that it points to the top-level node of the playlist that corresponds
    // to the in_uJumpToIdx. Flushes sequencer look-ahead if necessary.
    // Can only be called before the sequence starts playing.
    // Returns the new first segment bucket (of the sequence).
    CAkScheduledItem * JumpToSegment(
        AkUniqueID in_uJumpToID					// Top-level playlist jump index.
        );

    
    // CAkChainCtx implementation
    // ----------------------------------------------------------
	// Schedule a new item and append it to the chain.
    virtual bool Grow();

	// Chain is grown until something has been scheduled at in_iDesiredPosition, and there are 
	// at least AK_MINIMAL_PLAYLIST_LOOK_AHEAD items ahead of this position.
	virtual void UpdateChainLength(
		AkInt64 in_iDesiredPosition			// Desired position.
		);

	// Put the chain in a state where it will not be able to grow anymore, after a fatal error, 
	// and return the last valid item (NULL if none): set the iterator to invalid.
	virtual CAkScheduledItem * HandleFatalError();

	// Returns false if the playlist iterator was made invalid because of a memory failure.
	virtual bool IsValid();

private:
    // Returns new the first new item that was scheduled.
    CAkScheduledItem * ScheduleNextSegment( 
		bool & out_bPlayPreEntry				// True if rule required playing first item's pre-entry.
		);

    // Appends a bucket to the chain following a music transition rule.
    CAkScheduledItem * AppendItem( 
        const AkMusicTransitionRule & in_rule,  // Transition rule between source bucket (in_pSrcBucket) and next one.
        CAkScheduledItem * in_pSrcItem,			// Item after which a new item is scheduled (NULL if first).
        AkUniqueID in_nextID,                   // Segment ID of the bucket to append to the chain.
        AkUniqueID in_playlistItemID            // Playlist item ID of the bucket to append to the chain.
        );

	// Grow.
	CAkScheduledItem * _Grow();

private:
    CAkMusicRanSeqCntr *        m_pSequenceNode;
    
    // Current playlist iterator
	AkRSIterator				m_PlayListIterator;

	bool						m_bIsChainValid;	// False if growth of chain has failed because of a memory failure.
};

#endif //_SEQUENCE_CTX_H_
