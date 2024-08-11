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
// AkSegmentCtx.h
//
// Segment context.
//
//////////////////////////////////////////////////////////////////////
#ifndef _SEGMENT_CTX_H_
#define _SEGMENT_CTX_H_

#include "AkMusicCtx.h"
#include "AkRegisteredObj.h"
#include "PrivateStructures.h"
#include "AkContextualMusicSequencer.h"
#include "AkMonitorData.h"
#include <AK/Tools/Common/AkArray.h>

class CAkMusicSegment;
class CAkScheduledItem;
class CAkMusicTrack;

// -------------------------------------------------------------------
// Name: CAkSegmentCtx
// Desc: Low-level segment context. Never used alone, always in a 
//		 chain of scheduled items. Responsible for managing playback of 
//		 music PBIs.
// -------------------------------------------------------------------
class CAkSegmentCtx : public CAkMusicCtx
{
public:
    CAkSegmentCtx(
        CAkMusicSegment *   in_pSegmentNode,
        CAkMusicCtx *       in_pParentCtx
        );
    virtual ~CAkSegmentCtx();

    AKRESULT Init(
        CAkRegisteredObj *  in_GameObject,
        UserParams &        in_rUserparams
        );

    // Set owner.
    inline void SetOwner(
        CAkScheduledItem * in_pOwner	// Owner scheduled item.
        )
	{
		m_pOwner = in_pOwner;
	}

    // Set step track index.
    void StepTrack( 
        AkUInt16            in_uStepIdx         // Step track index.
        );

    //
	// Context commands
	//
    // Initialize context for playback.
    // Prepare the context for playback at the given position.
    // Audible playback will start at position in_iSourceOffset (relative to EntryCue).
    // Returns the exact amount of time (samples) that will elapse between call to _Play() and 
    // beginning of playback at position in_iPosition.
	AkInt32 Prepare(
		AkInt32		in_iPosition		// Desired position in samples relative to the Entry Cue.
        );

	// Process: 
    // Instantiate and play MusicPBIs in current time window [in_iNow, in_iNow+in_uNumSamples[, 
	// schedule and executes associated stop commands.
    void Process(
		AkInt32		in_iTime,			// Current time (in samples) relative to the Entry Cue.
		AkUInt32	in_uNumSamples		// Time window size.
		/// TEMP in_uNumSamples will always be REFILL until we implement time stretch.
		);

	// Get time when this segment becomes audible (relative to its entry cue).
	inline AkInt32 GetAudibleTime() { return m_iAudibleTime; }

    // Get remaining time during which the segment is silent. 
	// If returned value is negative, silent time is completely elapsed.
    inline AkInt32 GetRemainingSilentTime(
		AkInt32		in_iTime			// Current time (in samples) relative to the Entry Cue.
		)
	{ 
		return m_iAudibleTime - in_iTime; 
	}

    // Non-virtual counterpart, when user knows it is a segment.
    inline CAkMusicSegment * SegmentNode() { return m_pSegmentNode; }

	//
    // PBI notifications.
    //
    // Called when PBI destruction occurred from Lower engine without the higher-level hierarchy knowing it.
    // Remove all references to this target from this segment's sequencer.
    void RemoveAllReferences( 
        CAkPBI * in_pCtx   // Context whose reference must be removed.
        );

	// Computes source offset (in file) and frame offset (in pipeline time, relative to now) required to make a given source
	// restart (become physical) sample-accurately.
	// Returns false if restarting is not possible because of the timing constraints.
	bool GetSourceInfoForPlaybackRestart(
		const CAkMusicPBI * in_pCtx,	// Context which became physical.
		AkInt32 & out_iLookAhead,		// Returned required required look-ahead time (frame offset).
		AkInt32 & out_iSourceOffset		// Returned required source offset ("core" sample-accurate offset in source).
		);

protected:

	// 
	// CAkMusicCtx
	// 
	// Override MusicCtx OnPlayed(): Need to schedule audio clips.
    virtual void OnPlayed();
	
	// Override MusicCtx OnStopped(): Need to flush actions to release children and to notify for cursor.
    virtual void OnStopped();

	// Override MusicCtx OnPaused/Resumed on some platforms: Stop all sounds and prepare to restart them on resume.
	virtual void OnPaused();
#ifdef AK_STOP_MUSIC_ON_PAUSE
	virtual void OnResumed();
#endif	/// AK_STOP_MUSIC_ON_PAUSE

#ifndef AK_OPTIMIZED
	// Catch MusicCtx OnEditDirty(): Stop and reschedule all audio clips.
	virtual void OnEditDirty();
#endif

private:
	// Execute scheduled commands.
    void ExecuteScheduledCmds(
		AkInt32		in_iTime,			// Current time (relative to entry cue).
		AkUInt32	in_uNumSamples		// Time window size.
		);

	// Schedule clip play commands.
	void ScheduleAudioClips();

    // Computes context's look-ahead value (samples) according to the position specified.
    AkInt32 ComputeMinSrcLookAhead(
        AkInt32		in_iPosition		// Segment position, relative to entry cue.
        );

	// Convert clip data time (relative to beginning of pre-entry) to segment time (relative to entry cue).
	AkInt32 ClipDataToSegmentTime(
		AkInt32		in_iClipDataTime	// Time, relative to beginning of pre-entry.
		);

	// Convert segment time (relative to entry cue) to clip data time (relative to beginning of pre-entry).
	AkInt32 SegmentTimeToClipData(
		AkInt32		in_iClipDataTime	// Time, relative to entry cue.
		);

    // Access to ascendents' shared objects.
    CAkRegisteredObj *  GameObjectPtr();
    AkPlayingID		    PlayingID();
    UserParams &		GetUserParams();

	// Flush sequencer commands and automation.
	void Flush();

	// Re-schedule all audio clips to play now + look-ahead time corresponding to now.
	// Note: Sounds should have been stopped and dcheduler and automation should have been flushed already.
	void RescheduleAudioClipsNow();

private:

    CAkContextualMusicSequencer m_sequencer;

    CAkMusicSegment *   m_pSegmentNode;    

    CAkScheduledItem *  m_pOwner;

    // Time when this segment context will be audible, relative to its entry cue.
	// It is set by Prepare() as the desired segment start position. 
	// Audio clips are scheduled in such a way that this context is silent before m_iAudibleTime is reached.
    AkInt32             m_iAudibleTime;

	void NotifyAction( AkMonitorData::NotificationReason in_eReason );
#ifndef AK_OPTIMIZED

public:
	AkUniqueID GetPlayListItemID(){ return m_PlaylistItemID; }
	void SetPlayListItemID( AkUniqueID in_playlistItemID ){ m_PlaylistItemID = in_playlistItemID; }
private:
	AkUniqueID		m_PlaylistItemID;

#endif

	typedef AkArray<CAkMusicTrack*, CAkMusicTrack*, ArrayPoolDefault,LIST_POOL_BLOCK_SIZE/sizeof(CAkMusicTrack*)> TrackArray;
	TrackArray		m_arTracks;    

	typedef AkArray<AkUInt16, AkUInt16,ArrayPoolDefault,LIST_POOL_BLOCK_SIZE/sizeof(AkUInt16)> TrackRSArray;
    TrackRSArray    m_arTrackRS;

	typedef AkListBareLight<AkMusicAutomation> AutomationList;
	AutomationList		m_listAutomation;
};

#endif
