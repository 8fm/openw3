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
// Music Segment.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _MUSIC_SEGMENT_H_
#define _MUSIC_SEGMENT_H_

#include "AkMusicNode.h"

#define MAX_SRC_SEGMENT_LOOK_AHEAD		(64)	// Maximum number of segments of the current chain that 
												// can be looked-ahead in order to schedule a transition.

class CAkMusicCtx;
class CAkSegmentCtx;
class CAkMusicTrack;
class CAkSequencableSegmentCtx;

class CAkMusicSegment : public CAkMusicNode
{
public:

    // Thread safe version of the constructor.
	static CAkMusicSegment * Create(
        AkUniqueID in_ulID = 0
        );

	AKRESULT SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize );

	// Return the node category.
	virtual AkNodeCategory NodeCategory();

    virtual AKRESULT CanAddChild(
        CAkParameterNodeBase * in_pAudioNode 
        );

	// Override MusicObject::ExecuteAction() to catch Seek actions.
	virtual AKRESULT ExecuteAction( ActionParams& in_rAction );
	virtual AKRESULT ExecuteActionExcept( ActionParamsExcept& in_rAction );

	virtual AKRESULT PlayInternal( AkPBIParams& in_rPBIParams );

    // Context factory. 
    // Creates a sequencable segment context, usable by switch containers or as a top-level instance.
    // Returns NULL and cleans up if failed.
	virtual CAkMatrixAwareCtx * CreateContext( 
        CAkMatrixAwareCtx * in_pParentCtx,
        CAkRegisteredObj * in_GameObject,
        UserParams &  in_rUserparams
        );
	CAkSequencableSegmentCtx * CreateSegmentContext( 
		CAkMatrixAwareCtx * in_pParentCtx,
		CAkRegisteredObj * in_GameObject,
		UserParams &  in_rUserparams
		);

    // Creates a simple segment context and addrefs it. Caller needs to release it. 
	// Returns NULL and cleans up if failed.
    CAkSegmentCtx * CreateLowLevelSegmentCtxAndAddRef( 
        CAkMatrixAwareCtx * in_pParentCtx,
        CAkRegisteredObj * in_GameObject,
        UserParams &  in_rUserparams
        );

    void Duration(
        AkReal64 in_fDuration               // Duration in milliseconds.
        );
#ifndef AK_OPTIMIZED
	void StartPos(
        AkReal64 in_fStartPos               // PlaybackStartPosition.
        );

    AkUInt32 StartPos() { return m_uStartPos; };
#endif

	AKRESULT SetMarkers(
		AkMusicMarkerWwise*     in_pArrayMarkers, 
		AkUInt32                 in_ulNumMarkers
		);

    // Interface for Contexts
    // ----------------------

	AkUInt16 NumTracks() { return Children(); }
	// IMPORTANT: This function is not safe to call out of initialization.
    CAkMusicTrack * Track(
        AkUInt16 in_uIndex
        );

    // Returns the segment's total duration in samples (at the native sample rate).
    AkUInt32 Duration() { return m_uDuration; };

    // Music transition query. 
    // Returns the delta time (in samples) between the position supplied and the exit Sync point.
    // Minimum time supplied must be relative to the Entry Marker.
    // If no possible exit point is found the method returns AK_Fail. AK_Success if it was found.
    AKRESULT GetExitSyncPos(
        AkUInt32		in_uSrcMinTime,         // Minimal time-to-sync constraint related to source, relative to the Entry Marker (always >= 0).
        AkSyncType		in_eSyncType,           // Sync type.
		AkUniqueID &	io_uCueFilterHash,		// Cue filter name hash. Returned as filter hash if sync point was found on a cue, AK_INVALID_UNIQUE_ID otherwise.
        bool			in_bSkipEntryCue,		// If true, will not consider Entry cue.
        AkUInt32 &		out_uExitSyncPos        // Returned Exit Sync position (always >= 0), relative to the Entry Marker.
        );

    // Returns the position of the entry Sync point defined by the rule.
    // Position is relative to the Entry marker.
    void GetEntrySyncPos(
		const AkMusicTransDestRule & in_rule,   // Transition destination (arrival) rule.
		AkUInt32	in_uMinimumEntryPosition,	// Minimum entry position. Always >= 0.
		AkUniqueID	in_uCueHashForMatchSrc,		// Hash of the cue which was used in the source segment. Used only if rule is bDestMatchCueName.
		AkUniqueID & out_uSelectedCueHash,		// Returned cue hash of selected cue. AK_INVALID_UNIQUE_ID if not on a cue.
		AkUInt32 &	out_uRequiredEntryPosition	// Returned entry position. Always >= 0.
        );

    // Returns the duration of the pre-entry.
    AkInt32 PreEntryDuration();  // Note. Returns an int because sometimes used with unary operator-.

    // Returns the duration of the post exit.
    AkUInt32 PostExitDuration();

    // Returns the active duration, the length between the Entry and Exit markers.
    AkUInt32 ActiveDuration();

	// Returns the position (relative to entry cue) of the cue that is the
	// closest to the position provided (exit cue is ignored).
	AkInt32 GetClosestCuePosition(
		AkInt32 in_iPosition	// Position, relative to entry cue
		);

    /*
    // Returns true if at least one node of the hierarchy is registered to the state group and has a sync that is
    // not immediate.
    bool DoesRequireDelayedStateChange(
        AkStateGroupID  in_stateGroupID
        );
    */

    bool GetStateSyncTypes( AkStateGroupID in_stateGroupID, CAkStateSyncArray* io_pSyncTypes );

protected:
    CAkMusicSegment( 
        AkUniqueID in_ulID
        );
    virtual ~CAkMusicSegment();
    AKRESULT Init();
    void Term();

    // Helpers.
	
    // Return marker positions, relative to the Entry marker.
    AkUInt32 ExitMarkerPosition();

    // Find next Bar, Beat, Grid absolute position.
    // Returns AK_Success if a grid position was found before the Exit Marker.
    // NOTE This computation is meant to change with compound time signatures.
    AKRESULT GetNextMusicGridValue(
        AkUInt32		in_uMinPosition,	// Start search position (in samples), relative to Entry cue.
		AkUInt32		in_uGridDuration,	// Grid (beat, bar) duration.
		AkUInt32		in_uOffset,			// Grid offset.
		bool			in_bSkipEntryCue,	// Do not return 0 if true.
		AkUInt32 &		out_uExitPosition	// Returned position (relative to Entry cue).
        );
    
    // Find position of first marker after supplied in_iPosition.
    AkUInt32 GetNextMarkerPosition(
        AkInt32			in_iPosition,		// Position from which to start search, relative to Entry marker (>=0).
		AkUniqueID &	io_uCueFilterHash,	// Cue filter name hash. Returned as ID of selected marker.
        bool			in_bDoSkipEntryCue	// If true, will not consider Entry cue.
        );
    // Find position of first marker after supplied in_iPosition, excluding Entry and Exit marker.
    // Note. Might fail if there are no user marker before Exit marker.
    AKRESULT GetNextUserMarkerPosition(
        AkUInt32		in_uPosition,		// Position from which to start search, relative to Entry marker (>=0).
		AkUniqueID &	io_uCueFilterHash,	// Cue filter name hash. Returned as ID of selected marker.
		AkUInt32 &		out_uMarkerPosition	// Returned cue position.
        );

	inline bool CueMatchesFilter( 
		AkMusicMarker &	in_cue,
		AkUniqueID		in_uCueFilterHash
		)
	{
		return ( in_uCueFilterHash == AK_INVALID_UNIQUE_ID || in_cue.id == in_uCueFilterHash );
	}

	// Select random cue. 
	AkInt32 GetRandomCue(
		AkUInt32		in_uMinPosition, 
		AkUniqueID		in_uCueFilter,
		bool			in_bAvoidEntryCue,
		AkUniqueID &	out_uSelectedCueHash	// Returned cue hash of selected cue. 
		);

	// Return number of cues that match filter. Entry and Exit cues are ALWAYS counted in.
	AkUInt32 GetNumCuesWithFilter(
		AkUInt32		in_uCueStartIdx,// Cue index (among all cues) from which to start search (inclusively).
		AkUniqueID		in_uCueFilter	// Filter hash.
		);

	// Return real index in array from an index among cues that match the provided filter hash (including Entry cue).
	AkUInt32 SelectCueWithFilter(
		AkUInt32		in_uCueStartIdx,// Cue index (among all cues) from which to start search (inclusively).
		AkUInt32		in_uIndex,		// Index among cues that match the filter hash (including Entry cue).
		AkUniqueID		in_uCueFilter	// Filter hash.
		);



public:
	// Notify user cues in the specified range: [in_iStartPosition, in_iStartPosition+in_uRangeSize[.
	void NotifyUserCuesInRange(
		AkPlayingID in_playingID,		// Playing ID
		const AkMusicGrid& in_rGrid,	// Grid for notification info
		AkInt32		in_iStartPosition,	// Start Position relative to entry cue (In Samples)
		AkUInt32	in_uRangeSize		// Range size (In Samples)
		);

	// Return the number of beat, bar, and grid boundaries in the specified range: [in_iStartPosition, in_iStartPosition+in_uRangeSize[.
	void GetNumMusicGridInRange(
		AkInt32		in_iStartPosition,	// Start Position relative to entry cue (In Samples)
		AkUInt32	in_uRangeSize,		// Range size (In Samples)
		AkUInt32 &	out_uNumBars,		// Returned number of bar boundaries.
		AkUInt32 &	out_uNumBeats,		// Returned number of beat boundaries.
		AkUInt32 &	out_uNumGrids		// Returned number of grid boundaries.
		);

	// Return the number of either grid, beat or bar boundaries in the specified range: [in_iStartPosition, in_iStartPosition+in_uRangeSize[.
	AkUInt32 GetNumMusicGridInRange(
		AkUInt32	in_uStartPosition,	// Start search position (in samples), relative to Entry cue (>=0).
		AkUInt32	in_uRangeSize,		// Range size (In Samples)
		AkUInt32	in_uGridDuration,	// Grid (beat, bar) duration.
		AkUInt32	in_uOffset			// Grid offset.
		);

private:
    // Array of markers. Markers are stored by their position (in samples).
    // The first marker (index 0) is always the entry marker.
    // The last marker is always the exit marker.
    typedef AkArray<AkMusicMarker,const AkMusicMarker&,ArrayPoolDefault,0> MarkersArray;
    MarkersArray    m_markers;

    // Segment duration (in samples at the native sample rate).
    AkUInt32    m_uDuration;

#ifndef AK_OPTIMIZED
	AkUInt32	m_uStartPos;
#endif
};

#endif //_MUSIC_SEGMENT_H_
