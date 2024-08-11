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
// AkContextualMusicSequencer.h
//
// Action sequencer for music contexts.
// Holds a list of pending musical actions, stamped with sample-based
// timing. 
// For example, a sequence context would enqueue actions on children 
// nodes that are scheduled to play or stop in the near future.
//
//////////////////////////////////////////////////////////////////////
#ifndef _CTX_MUSIC_SEQUENCER_H_
#define _CTX_MUSIC_SEQUENCER_H_

#include <AK/Tools/Common/AkListBare.h>
#include "AkMusicStructs.h"
#include "AudiolibDefs.h"
#include "AkMusicTrack.h"
#include "AkMusicPBI.h"

struct AkMusicAutomation
{
	AkMusicAutomation( CAkClipAutomation * in_pAutomationData, AkInt32 in_iTimeStart )
	: pAutomationData( in_pAutomationData )
	, pPBI( NULL )
	, iTimeStart( in_iTimeStart )
	{}

	inline void Apply( 
		AkInt32 in_iCurrentClipTime		// Current time relative to clip data (beginning of pre-entry).
		)
	{
		pPBI->SetAutomationValue( 
			pAutomationData->Type(), 
			pAutomationData->GetValue( in_iCurrentClipTime - iTimeStart ) );
	}

	CAkClipAutomation *	pAutomationData;
	CAkMusicPBI	*		pPBI;
	AkInt32				iTimeStart;
	AkMusicAutomation *	pNextLightItem;
};

enum AkMusicActionType
{
	MusicActionTypePlay,
	MusicActionTypeStop
};

class AkMusicAction
{
public:
	virtual ~AkMusicAction() {}

	virtual AkMusicActionType Type() = 0;
	inline AkInt32 Time() { return m_iTime; }

	AkMusicAction *		pNextItem;
protected:
	AkMusicAction( AkInt32 in_iTime ) : m_iTime( in_iTime ) {}

    AkInt32             m_iTime;
};

class AkMusicActionPlay : public AkMusicAction
{
public:
	AkMusicActionPlay( 
		AkInt32			in_iTime,		// Action scheduled time.
		CAkMusicTrack *	in_pTrack,		// Owner track.
		const AkTrackSrc & in_rTrackSrc,	// Track source info.
		AkUInt32		in_uSourceOffset,// Source start position.
		AkUInt32		in_uPlayOffset	// Lower engine source start look-ahead.
		)
	: AkMusicAction( in_iTime )
	, m_pTrack( in_pTrack )
	, m_rTrackSrc( in_rTrackSrc )
	, m_uSourceOffset( in_uSourceOffset )
	, m_uPlayOffset( in_uPlayOffset ) {}

	virtual ~AkMusicActionPlay() 
	{
		while ( !m_listAutomation.IsEmpty() )
		{
			AkMusicAutomation * pAutomation = m_listAutomation.First();
			m_listAutomation.RemoveFirst();
			AkDelete( g_DefaultPoolId, pAutomation );
		}
	}

	virtual AkMusicActionType Type() { return MusicActionTypePlay; }

	inline CAkMusicTrack * Track() { return m_pTrack; }
	inline const AkTrackSrc & TrackSrc() { return m_rTrackSrc; }
	inline AkUInt32	SourceOffset() { return m_uSourceOffset; }
	inline AkUInt32	PlayOffset() { return m_uPlayOffset; }
	inline CAkMusicSource * Source() { return Track()->GetSourcePtr( TrackSrc().srcID ); }

	// Query track for automation data and attach to action if applicable.
	inline void AttachClipAutomation(
		AkUInt32				in_uClipIndex,
		AkClipAutomationType	in_eType,
		AkInt32					in_iTimeStart
		)
	{
		CAkClipAutomation * pAutomationData = Track()->GetClipAutomation( in_uClipIndex, in_eType );
		if ( pAutomationData )
		{
			AkMusicAutomation * pAutomation = AkNew( g_DefaultPoolId, AkMusicAutomation( pAutomationData, in_iTimeStart ) );
			if ( pAutomation )
				m_listAutomation.AddFirst( pAutomation );
		}
	}

	inline AkMusicAutomation * PopAutomation()
	{
		AkMusicAutomation * pAutomation = m_listAutomation.First();
		if ( pAutomation )
			m_listAutomation.RemoveFirst();
		return pAutomation;
	}


protected:
	CAkMusicTrack *		m_pTrack;		// Owner track.
	const AkTrackSrc &	m_rTrackSrc;	// Track source info.
	AkUInt32			m_uSourceOffset;// Source start position.
	AkUInt32			m_uPlayOffset;	// Lower engine source start look-ahead.
	typedef AkListBareLight<AkMusicAutomation> AutomationList;
	AutomationList		m_listAutomation;
};

class AkMusicActionStop : public AkMusicAction
{
public:
	AkMusicActionStop( 
		AkInt32			in_iTime,		// Action scheduled time.
		CAkMusicPBI *	in_pTargetPBI	// Target PBI.
		) 
	: AkMusicAction( in_iTime )
	, pTargetPBI( in_pTargetPBI ) {}

	virtual AkMusicActionType Type() { return MusicActionTypeStop; }

    CAkMusicPBI * 	    pTargetPBI;		// Target PBI.
	bool				bHasAutomation;
};

// TODO Rename this class as a Music PBI sequencer.
class CAkContextualMusicSequencer : public AkListBare<AkMusicAction>
{
public:
    CAkContextualMusicSequencer();
    virtual ~CAkContextualMusicSequencer();

    // Schedule an action.
    void ScheduleAction( 
        AkMusicAction * in_pAction			// Action to be scheduled. Allocated by user.
        );

    // Returns AK_NoMoreData when there is no action to be executed in next frame (out_action is invalid).
    // Otherwise, returns AK_DataReady.
    // NOTE: When actions are dequeued with this method, they are still referenced. Caller needs to
    // release them explicitly.
    AKRESULT PopImminentAction(
		AkInt32 in_iNow,					// Current time (samples).
		AkInt32 in_iFrameDuration,			// Number of samples to process.
        AkMusicAction *& out_pAction		// Returned action. Freed by user.
        );

    // Removes from sequencer and frees all actions that reference the specified PBI. 
    void ClearActionsByTarget( 
        CAkMusicPBI * in_pTarget
        );

    // Remove all actions from sequencer (ref counted targets are released).
    void Flush();
};

#endif //_CTX_MUSIC_SEQUENCER_H_
