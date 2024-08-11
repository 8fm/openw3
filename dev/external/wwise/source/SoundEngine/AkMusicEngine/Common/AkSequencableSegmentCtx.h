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

#ifndef _SEQUENCABLE_SEGMENT_CTX_H_
#define _SEQUENCABLE_SEGMENT_CTX_H_

#include "AkSegmentChain.h"

class CAkSequencableSegmentCtx : public CAkChainCtx
{
public:
    CAkSequencableSegmentCtx(
        CAkMusicSegment *   in_pSegmentNode,
        CAkMusicCtx *       in_pParentCtx
        );
    virtual ~CAkSequencableSegmentCtx();

    AKRESULT Init( 
        CAkRegisteredObj *  in_GameObject,
        UserParams &        in_rUserparams
        );

    // Matrix Aware Context implementation.
    // ----------------------------------------------------------

	// Change playback position.
	// The chain is destroyed and re-prepared at the correct position.
	virtual AKRESULT SeekTimeAbsolute( 
		AkTimeMs & io_position,	// Seek position, in ms, relative to Entry Cue (if applicable). Returned as effective position.
		bool in_bSnapToCue		// True if final position needs to be on a cue (Exit cue excluded).
		);
	virtual AKRESULT SeekPercent( 
		AkReal32 & io_fPercent,	// Seek position, in percentage, between Entry and Exit Cues ([0,1]). Returned as effective position.
		bool in_bSnapToCue		// True if final position needs to be on a cue (Exit cue excluded).
		);

	// Music object queries.
	// Overriden from CAkMatrixAwareCtx: Since there is only one segment in the segment chain of a 
	// SequencableSegment, its info is returned regardless of the fact that it is active or not.
	// Doing so, we get values during pre-entry and post-exit (negative values during the pre-entry).
	virtual AKRESULT GetPlayingSegmentInfo(
		AkSegmentInfo &	out_segmentInfo		
		);
	
    // For Music Renderer's music contexts look-up: concrete contexts must return their own node.
    virtual CAkMusicNode * Node();

	// CAkChainCtx implementation
    // ----------------------------------------------------------
	// Once a segment context is created successfully, it is valid.
	virtual bool IsValid() { return true; }

private:
	AKRESULT Seek( 
		AkInt32 in_iSeekPosition				// Seek position, relative to Entry Cue.
		);
	AKRESULT SetupSegmentChain( 
		CAkRegisteredObj *  in_GameObject,
		UserParams &        in_rUserparams,
		AkInt32				in_iStartPos,		// Start position in samples relative to the Entry Cue.
		AkInt32 &			out_iLookAheadDuration	// Look-ahead duration required at this position.
		);

private:
    // Note. Duplicates its "active" context's segment.
    CAkMusicSegment *   m_pSegmentNode;
};

#endif // _SEQUENCABLE_SEGMENT_CTX_H_
