/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/enumBuilder.h"
#include "../core/classBuilder.h"
#include "../engine/extAnimEvent.h"

class CAnimatedComponent;
struct SBehaviorGraphOutput;
struct SBehaviorSampleContext;
class CSkeletalAnimationSetEntry;
class CExtAnimEvent;

/* Real animation event definition is in extAnimEventFile.h" */

enum EAnimationEventType : CEnum::TValueType
{
	AET_Tick,
	AET_DurationStart,
	AET_DurationStartInTheMiddle,
	AET_DurationEnd,
	AET_Duration
};

BEGIN_ENUM_RTTI( EAnimationEventType );
	ENUM_OPTION( AET_Tick );
	ENUM_OPTION( AET_DurationStart );
	ENUM_OPTION( AET_DurationStartInTheMiddle );
	ENUM_OPTION( AET_DurationEnd );
	ENUM_OPTION( AET_Duration );
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////

/// additional information about fired event that should be invisible inside script but can be handled to engine code through script
struct SAnimationEventAnimInfo
{
	DECLARE_RTTI_STRUCT( SAnimationEventAnimInfo );
public:
	CName								m_eventName;			//!< event name (it might be lost when only this struct is used)
	const CSkeletalAnimationSetEntry*	m_animation;			//!< animation that fired event
	Float								m_localTime;			//!< local time in animation
	Float								m_eventEndsAtTime;		//!< event ends at time in animation
	Float								m_eventDuration;

	SAnimationEventAnimInfo();
};

BEGIN_CLASS_RTTI( SAnimationEventAnimInfo );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

//! struct describing fired animation event
struct CAnimationEventFired
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_Animation, MC_PoseEvent )

	EAnimationEventType					m_type;					//!< type of event fired
	Float								m_alpha;				//!< alpha of animation that triggered event

	SAnimationEventAnimInfo				m_animInfo;				//!< information about animation that fired event

	// For new event system
	const CExtAnimEvent*				m_extEvent;

	CAnimationEventFired();

	//! Get event name (supports old and new event system)
	const CName& GetEventName() const;

	Bool CanMergeWith( const CAnimationEventFired& event ) const;

	void MergeWith( const CAnimationEventFired& event, Float alpha );

	Float GetProgress() const;

	RED_FORCE_INLINE Bool ReportToScript() const { return m_extEvent->ReportToScript( m_alpha ); }
	RED_FORCE_INLINE Bool ReportToAI() const { return ReportToScript() && m_extEvent->ReportToAI(); }
};

