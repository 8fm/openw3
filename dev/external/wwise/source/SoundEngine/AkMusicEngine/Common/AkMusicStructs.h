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
// AkMusicStructs.h
//
// Interactive Music specific structures definitions.
//
//////////////////////////////////////////////////////////////////////
#ifndef _MUSIC_STRUCTURES_H_
#define _MUSIC_STRUCTURES_H_

#include <AK/SoundEngine/Common/AkCommonDefs.h>
#include "PrivateStructures.h"

// Wwise specific types.
// ---------------------------------------

enum AkMusicMarkerType
{
    MarkerTypeEntry,
    MarkerTypeExit,
    MarkerTypeCustom
};

/* Not used.
enum AkNoteValue
{
    NoteValue_1,
    NoteValue_1_2,
    NoteValue_1_4,
    NoteValue_1_8,
    NoteValue_1_16,
    NoteValue_1_32,
    NoteValue_1_64,
    NoteValue_1_2_triplet,
    NoteValue_1_4_triplet,
    NoteValue_1_8_triplet,
    NoteValue_1_16_triplet,
    NoteValue_1_32_triplet,
    NoteValue_1_64_triplet,
    NoteValue_1_2_dotted,
    NoteValue_1_4_dotted,
    NoteValue_1_8_dotted,
    NoteValue_1_16_dotted,
    NoteValue_1_32_dotted,
    NoteValue_1_64_dotted
};
*/

// Music grid and tempo info, supplied by Wwise.
struct AkMeterInfo
{
    AkReal64    fGridPeriod;        // Grid period (1/frequency) (ms).
    AkReal64    fGridOffset;        // Grid offset (ms).
    AkReal32    fTempo;             // Tempo: Number of Quarter Notes per minute.
    AkUInt8     uTimeSigNumBeatsBar;// Time signature numerator.
    AkUInt8     uTimeSigBeatValue;  // Time signature denominator.
};

// Track audio source info, supplied by Wwise.
struct AkTrackSrcInfo
{
	AkUInt32	trackID;			// SubTrack index.
	AkUniqueID  sourceID;			// ID of the source 
    AkReal64    fPlayAt;            // Play At (ms).
    AkReal64    fBeginTrimOffset;   // Begin Trim offset (ms).
    AkReal64    fEndTrimOffset;     // End Trim offset (ms).
    AkReal64    fSrcDuration;       // Duration (ms).
};

enum AkClipAutomationType
{
	AutomationType_Volume			= 0,
	AutomationType_LPF				= 1,
	AutomationType_FadeIn			= 2,
	AutomationType_FadeOut			= 3
	// IMPORTANT: Unsafe to have value greater than 7, because of the way the cookie is set to CAkMusicPBI::SetPBIFade().
};

enum AkMusicTrackRanSeqType
{
	AkMusicTrackRanSeqType_Normal	= 0,
	AkMusicTrackRanSeqType_Random	= 1,
	AkMusicTrackRanSeqType_Sequence = 2
};


// Music transition structures.
// ---------------------------------------------------------

struct AkMusicFade
{
	AkTimeMs				transitionTime;		// how long this should take
	AkCurveInterpolation	eFadeCurve;			// what shape it should have
    AkInt32					iFadeOffset;		// Fade offset. Time on Wwise side, samples on sound engine side.
};

// Extra music object specified for some transitions.
struct AkMusicTransitionObject
{
    AkUniqueID  segmentID;          // Node ID. Can only be a segment.
    AkMusicFade fadeInParams;		// Fade in info. 
    AkMusicFade fadeOutParams;		// Fade out info.
    AkUInt32    bPlayPreEntry   :1;
    AkUInt32    bPlayPostExit   :1;
};

// Music transition rule: Common properties between source and destination rules.
class AkMusicTransRuleBase
{
public:
    AkMusicFade fadeParams;			// Fade out information.
	AkUniqueID	uCueFilterHash;
};

// Music transition rule: Source.
class AkMusicTransSrcRule : public AkMusicTransRuleBase
{
public:
    AkUInt32 /*AkSyncType*/  eSyncType :NUM_BITS_SYNC_TYPE; // Sync type.
    AkUInt32    bPlayPostExit   :1;
};

enum AkEntryType
{
	EntryTypeEntryMarker,
    EntryTypeSameTime,   // (than the current segment playing, in seconds, from the entry marker).
	EntryTypeRandomMarker,
	EntryTypeRandomUserMarker
#define NUM_BITS_ENTRY_TYPE (3)
};

class AkMusicTransDestRule : public AkMusicTransRuleBase
{
public:

	// Return true for transition destination that depend on the position chosen
	// in source : SameTime and CueMatch transitions.
	inline bool RequiresIterativePreparing() const 
	{ 
		return ( eEntryType == EntryTypeSameTime )
				|| ( bDestMatchSourceCueName 
						&& ( eEntryType == EntryTypeRandomMarker
							|| eEntryType == EntryTypeRandomUserMarker ) );
	}

    AkUniqueID		uJumpToID;				// JumpTo ID (applies to Sequence Containers only).
    AkUInt16 /*AkEntryType*/ eEntryType :NUM_BITS_ENTRY_TYPE; // Entry type. 
    AkUInt16		bPlayPreEntry		:1;
	AkUInt16		bDestMatchSourceCueName	:1;	// Destination cue filter rule: Match cue name in source.
};

//-------------------------------------------------------------------
// Defines.
//-------------------------------------------------------------------
// Used for <ANY> and <NONE> target node IDs. Node IDs generated by
// Wwise must never be one or the other.
#define AK_MUSIC_TRANSITION_RULE_ID_ANY     (AK_UINT_MAX)
#define AK_MUSIC_TRANSITION_RULE_ID_NONE    (0)

// Transition rule.
class AkMusicTransitionRule
{
public:
	AkMusicTransitionRule()
		: pTransObj( NULL )
	{}

	~AkMusicTransitionRule()
	{
		if ( pTransObj )
			AkFree( g_DefaultPoolId, pTransObj );

		srcIDs.Term();
		destIDs.Term();
	}

	AkMusicTransitionObject * AllocTransObject()
	{
		pTransObj = (AkMusicTransitionObject*)AkAlloc( g_DefaultPoolId, sizeof(AkMusicTransitionObject) );
		return pTransObj;
	}

	typedef AkArray< AkUniqueID, AkUniqueID, ArrayPoolDefault, DEFAULT_POOL_BLOCK_SIZE/sizeof(AkUniqueID)> TransitionNodeArray;
    TransitionNodeArray  srcIDs;    // Source (departure) node ID.
    TransitionNodeArray  destIDs;   // Destination (arrival) node ID.

    AkMusicTransSrcRule         srcRule;
    AkMusicTransDestRule        destRule;
    AkMusicTransitionObject *   pTransObj;   // Facultative. NULL if not used.
#ifndef AK_OPTIMIZED
	AkUInt32	index;		// Rule index (for profiling).
#endif
};

// Transition rule.
struct AkWwiseMusicTransitionRule
{
	AkUInt32	 uNumSrc;
    AkUniqueID*  srcIDs;    // Source (departure) node ID.
    AkUInt32	 uNumDst;
	AkUniqueID*  destIDs;   // Destination (arrival) node ID.

	AkMusicFade				srcFade;
	AkUInt32				eSrcSyncType;			// Sync type.
	AkUInt32				uSrcCueFilterHash;		// Source cue filter name hash.
    AkUInt8					bSrcPlayPostExit;

	AkMusicFade				destFade;
    AkUInt32				uDestCueFilterHash;		// Dest cue filter name hash. Applies to EntryTypeUserMarker entry type only. 0 == <ANY>
    AkUniqueID				uDestJumpToID;          // JumpTo ID (applies to Sequence Containers only).
	AkUInt16				eDestEntryType;			// Entry type. 
    AkUInt8					bDestPlayPreEntry;
	AkUInt8					bDestMatchSourceCueName;// Destination cue filter rule: Match cue name in source.

	AkUInt8					bIsTransObjectEnabled;
	AkUniqueID				segmentID;						// Node ID. Can only be a segment.
	AkMusicFade				transFadeIn;
	AkMusicFade				transFadeOut;
    AkUInt8					bPlayPreEntry;
    AkUInt8					bPlayPostExit;
};

//-------------------------------------------------------------------
// Defines.
//-------------------------------------------------------------------
// Music marker. In Editing mode, markers are identified by ID.
// OPTIMISATION: In game mode, could be identified by index.
class AkMusicMarker
{
public:
	AkMusicMarker()
		: id( 0 )
		, uPosition( 0 )
		, pszName( NULL )
	{}
	AkMusicMarker( AkUniqueID in_id, AkUInt32 in_uPosition )
		: id( in_id )
		, uPosition( in_uPosition )
		, pszName( NULL )
	{}
	~AkMusicMarker()
	{
		if ( pszName )
			AkFree( g_DefaultPoolId, pszName );
	}

	// = performs shallow copy
	/**
	AkMusicMarker & operator=( const AkMusicMarker & in_other )
	{
		id = in_other.id;
		uPosition = in_other.uPosition;
		pszName = in_other.pszName;
		return *this;
	}
	**/

	inline char * GetName() { return pszName; }

	void SetNameAndTakeOwnership( char * in_pszName )
	{
		pszName = in_pszName;
	}

    AkUniqueID			id;
    AkUInt32            uPosition;
private:
	char *				pszName;	// UTF-8 cue name.
};

struct AkMusicMarkerWwise // in ms stored on a double
{
    AkUniqueID			id;
    AkReal64            fPosition;
	char *				pszName;	// UTF-8 cue name.
};

enum RSType // must match enum PlayMode, do not reorder
{
	RSType_ContinuousSequence	= 0,
	RSType_StepSequence			= 1,
	RSType_ContinuousRandom		= 2,
	RSType_StepRandom			= 3
};

// Wwise specific interface
struct AkMusicRanSeqPlaylistItem
{
	AkUniqueID m_SegmentID;
	AkUniqueID m_playlistItemID;

	AkUInt32 m_NumChildren;
	RSType	 m_eRSType;
	AkInt16  m_Loop;
	AkUInt32 m_Weight;
	AkUInt16 m_wAvoidRepeatCount;

	bool m_bIsUsingWeight;
	bool m_bIsShuffle;
};

struct AkMusicSwitchAssoc
{
	AkSwitchStateID switchID;
	AkUniqueID		nodeID;
};

// Used to signify that there is an infinite number of stinger that can play at a time.
#define AK_NO_MAX_NUM_STINGERS AK_UINT_MAX

class CAkStinger
{
public:
	//Gets() for backward compatibility only, now considered as a struct, so no Get()functions required anumore
	inline AkTriggerID	TriggerID()	const { return m_TriggerID; }
	inline AkUniqueID	SegmentID()	const { return m_SegmentID; }
	inline AkSyncType	SyncPlayAt() const { return m_SyncPlayAt; }
	inline AkUniqueID	CueFilter() const { return m_uCueFilterHash; }
    inline AkInt32		DontRepeatTime() const { return AkTimeConv::MillisecondsToSamples( m_DontRepeatTime ); } // Returns samples.
    // OPTIM. Convert once at SetStingers().

	AkTriggerID	m_TriggerID;
	AkUniqueID	m_SegmentID;
	AkSyncType	m_SyncPlayAt;
	AkUniqueID	m_uCueFilterHash;
	AkTimeMs	m_DontRepeatTime;

	AkUInt32	m_numSegmentLookAhead;
};

// Seeking info.
struct AkSeekingInfo
{
	union
	{
		AkInt32		iSeekPosition;	// Minimum position in destination (absolute).
		AkReal32	fSeekPercent;	// Minimum position in destination (percent).
	};
	bool	bRelative;				// Absolute or relative seeking.
};

#endif //_MUSIC_STRUCTURES_H_
