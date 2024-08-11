/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

#include "behaviorGraphNode.h"
#include "animSyncInfo.h"
#include "extAnimEvent.h"

class CBehaviorGraphBlendNode;
class CBehaviorGraphValueNode;

class IBehaviorSyncMethod : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IBehaviorSyncMethod, CObject );

public:
	// Synchronize two animation
	virtual void Synchronize( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* firstInput, const CBehaviorGraphNode *secondInput, Float alpha, Float timeDelta ) const = 0;

	// Synchronize animation to sync data
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* target, const CSyncInfo &sourceSyncInfo ) const = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehaviorSyncMethod );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorSyncMethodNone : public IBehaviorSyncMethod
{
	DECLARE_ENGINE_CLASS( CBehaviorSyncMethodNone, IBehaviorSyncMethod, 0 );

public:
	virtual void Synchronize( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* firstInput, const CBehaviorGraphNode *secondInput, Float alpha, Float timeDelta ) const {}
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* target, const CSyncInfo &sourceSyncInfo ) const {}
};

DEFINE_SIMPLE_RTTI_CLASS( CBehaviorSyncMethodNone, IBehaviorSyncMethod );

//////////////////////////////////////////////////////////////////////////

class IBehaviorSyncMethodEvent  : public IBehaviorSyncMethod
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IBehaviorSyncMethodEvent, IBehaviorSyncMethod );

protected:
	CName m_eventName;

public:
	virtual void Synchronize( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* firstInput, const CBehaviorGraphNode *secondInput, Float alpha, Float timeDelta ) const = 0;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* target, const CSyncInfo &sourceSyncInfo ) const = 0;

protected:
	template < class EventType >
	const EventType* GetSyncEvent( const CSyncInfo& info ) const
	{
		const TDynArray< CExtAnimEvent* >& inputEvents = info.m_syncEvents;
		const EventType* syncEvent = NULL;

		for ( Uint32 i=0; i<inputEvents.Size(); ++i )
		{
			const CExtAnimEvent* event = inputEvents[i];

			if ( event->GetEventName() == m_eventName && IsType< EventType >( event ) &&
				( !syncEvent || ( syncEvent && syncEvent->GetStartTime() > event->GetStartTime() ) ) )
			{
				syncEvent = static_cast< const EventType* >( event );
				break;
			}
		}

		return syncEvent;
	}

	template < class EventType >
	const EventType* GetSyncEventInTime( const CSyncInfo& info ) const
	{
		Float start = info.m_prevTime;
		Float end = info.m_currTime;

		// Unwrap animation time
		if ( end < start )
		{
			// Loop
			end += info.m_totalTime;
		}

		const TDynArray< CExtAnimEvent* >& inputEvents = info.m_syncEvents;
		const EventType* syncEvent = NULL;

		for ( Uint32 i=0; i<inputEvents.Size(); ++i )
		{
			const CExtAnimEvent* event = inputEvents[i];

			const Float evtStart = event->GetStartTime();
			const Float evtEnd = event->GetEndTimeWithoutClamp();

			Float localStart = start;
			Float localEnd = end;

			// Unwrap event time
			//if ( evtEnd > info.m_totalTime )
			//{
			//	// Event is looped
			//	localStart += info.m_totalTime;
			//	localEnd += info.m_totalTime;
			//}

			if ( event->GetEventName() == m_eventName && IsType< EventType >( event ) &&
				( (	evtStart <= localStart && localStart <= evtEnd ) || (	evtStart <= localEnd	&& localEnd <= evtEnd ) ) && 
				( !syncEvent || ( syncEvent && syncEvent->GetStartTime() > event->GetStartTime() ) ) )
			{
				syncEvent = static_cast< const EventType* >( event );
				break;
			}
		}

		return syncEvent;
	}

public:
	virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue );

	const CName& GetEventName() const;
	void SetEventName( const CName& name );
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehaviorSyncMethodEvent );
	PARENT_CLASS( IBehaviorSyncMethod );
	PROPERTY_EDIT( m_eventName, TXT("Event for synchronization") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorSyncMethodEventStart : public IBehaviorSyncMethodEvent
{
	DECLARE_ENGINE_CLASS( CBehaviorSyncMethodEventStart, IBehaviorSyncMethodEvent, 0 );

protected:
	Bool m_startAtRandomEvent;

public:
	virtual void Synchronize( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* firstInput, const CBehaviorGraphNode *secondInput, Float alpha, Float timeDelta ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* target, const CSyncInfo &sourceSyncInfo ) const;
};

BEGIN_CLASS_RTTI( CBehaviorSyncMethodEventStart );
	PARENT_CLASS( IBehaviorSyncMethodEvent );
	PROPERTY_EDIT( m_startAtRandomEvent, TXT("Start at random event, not the first one") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorSyncMethodEventProp : public IBehaviorSyncMethodEvent
{
	DECLARE_ENGINE_CLASS( CBehaviorSyncMethodEventProp, IBehaviorSyncMethodEvent, 0 );

public:
	CBehaviorSyncMethodEventProp();

	virtual void Synchronize( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* firstInput, const CBehaviorGraphNode *secondInput, Float alpha, Float timeDelta ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* target, const CSyncInfo &sourceSyncInfo ) const;

protected:
	Float GetDurationProgress( const CExtAnimDurationEvent* event, const CSyncInfo &info ) const;
	Float GetTimeFromDurationProgress( const CExtAnimDurationEvent* event, const Float progress, const Float totalTime ) const;

protected:
	Float m_offset;
};

BEGIN_CLASS_RTTI( CBehaviorSyncMethodEventProp );
	PARENT_CLASS( IBehaviorSyncMethodEvent );
	PROPERTY_EDIT_RANGE( m_offset, TXT("Offset in per cent [-100 100]"), -100.f, 100.f );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

// synchronizes time since start between two nodes (current time will be the same in both)

class CBehaviorSyncMethodTime : public IBehaviorSyncMethod
{
	DECLARE_ENGINE_CLASS( CBehaviorSyncMethodTime, IBehaviorSyncMethod, 0 );

public:
	virtual void Synchronize( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* firstInput, const CBehaviorGraphNode *secondInput, Float alpha, Float timeDelta ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* target, const CSyncInfo &sourceSyncInfo ) const;
};

DEFINE_SIMPLE_RTTI_CLASS( CBehaviorSyncMethodTime, IBehaviorSyncMethod );

//////////////////////////////////////////////////////////////////////////

class CBehaviorSyncMethodDuration : public IBehaviorSyncMethod
{
	DECLARE_ENGINE_CLASS( CBehaviorSyncMethodDuration, IBehaviorSyncMethod, 0 );

public:
	virtual void Synchronize( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* firstInput, const CBehaviorGraphNode *secondInput, Float alpha, Float timeDelta ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* target, const CSyncInfo &sourceSyncInfo ) const;
};

DEFINE_SIMPLE_RTTI_CLASS( CBehaviorSyncMethodDuration, IBehaviorSyncMethod );

//////////////////////////////////////////////////////////////////////////

class CBehaviorSyncMethodOffset : public IBehaviorSyncMethod
{
	DECLARE_ENGINE_CLASS( CBehaviorSyncMethodOffset, IBehaviorSyncMethod, 0 );

public:
	CBehaviorSyncMethodOffset();

	virtual void Synchronize( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* firstInput, const CBehaviorGraphNode *secondInput, Float alpha, Float timeDelta ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* target, const CSyncInfo &sourceSyncInfo ) const;

protected:
	Float m_timeOffset;
};

BEGIN_CLASS_RTTI( CBehaviorSyncMethodOffset );
	PARENT_CLASS( IBehaviorSyncMethod );
	PROPERTY_EDIT( m_timeOffset, TXT("Where to start anim") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorSyncMethodProp : public IBehaviorSyncMethod
{
	DECLARE_ENGINE_CLASS( CBehaviorSyncMethodProp, IBehaviorSyncMethod, 0 );

public:
	virtual void Synchronize( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* firstInput, const CBehaviorGraphNode *secondInput, Float alpha, Float timeDelta ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* target, const CSyncInfo &sourceSyncInfo ) const;
};

DEFINE_SIMPLE_RTTI_CLASS( CBehaviorSyncMethodProp, IBehaviorSyncMethod );

//////////////////////////////////////////////////////////////////////////

class CBehaviorSyncMethodSyncPoints : public IBehaviorSyncMethod 
{
	DECLARE_ENGINE_CLASS( CBehaviorSyncMethodSyncPoints, IBehaviorSyncMethod, 0 );

public:
	virtual void OnPostLoad();
	virtual void Synchronize( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* firstInput, const CBehaviorGraphNode *secondInput, Float alpha, Float timeDelta ) const {}
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* target, const CSyncInfo &sourceSyncInfo ) const {}
};

DEFINE_SIMPLE_RTTI_CLASS( CBehaviorSyncMethodSyncPoints, IBehaviorSyncMethod );

class CBehaviorSyncMethodSyncPointsStartOnly : public CBehaviorSyncMethodSyncPoints 
{
	DECLARE_ENGINE_CLASS( CBehaviorSyncMethodSyncPointsStartOnly, CBehaviorSyncMethodSyncPoints, 0 );

public:
	virtual void OnPostLoad();
	virtual void Synchronize( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* firstInput, const CBehaviorGraphNode *secondInput, Float alpha, Float timeDelta ) const {}
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* target, const CSyncInfo &sourceSyncInfo ) const {}
};

DEFINE_SIMPLE_RTTI_CLASS( CBehaviorSyncMethodSyncPointsStartOnly, IBehaviorSyncMethod );

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphBlendNode : public CBehaviorGraphNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphBlendNode, CBehaviorGraphNode, "Blends", "Blend 2" );

protected:
	Float						m_firstInputValue;
	Float						m_secondInputValue;
	Bool						m_synchronize;
	IBehaviorSyncMethod*		m_syncMethod;
	Bool						m_takeEventsFromMostImportantInput; // Instead of blending events, take them from the most important input

protected:
	TInstanceVar< Float	>		i_controlValue;
	TInstanceVar< Float	>		i_prevControlValue;

protected:
	CBehaviorGraphNode*			m_cachedFirstInputNode;
	CBehaviorGraphNode*			m_cachedSecondInputNode;
	CBehaviorGraphValueNode*	m_cachedControlVariableNode;

public:
	CBehaviorGraphBlendNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets();

	virtual String GetCaption() const { return TXT("Blend"); }

#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void Synchronize( CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;	

	virtual void CacheConnections();

protected:
	Float GetAlphaValue( Float varValue ) const;
	
	void ProcessActivations( CBehaviorGraphInstance& instance ) const;
	void UpdateControlValue( CBehaviorGraphInstance& instance ) const;

	Bool IsFirstInputActive( Float var ) const;
	Bool IsSecondInputActive( Float var ) const;

	static const Float ACTIVATION_THRESHOLD;
};

BEGIN_CLASS_RTTI( CBehaviorGraphBlendNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY_EDIT( m_synchronize, TXT("Synchronize child playback") );
	PROPERTY_INLINED( m_syncMethod, TXT("Synchronization method") );
	PROPERTY_EDIT( m_takeEventsFromMostImportantInput, TXT("Instead of blending events, take them just from more important input") );
	PROPERTY_EDIT( m_firstInputValue, TXT("Variable value representing first input") );
	PROPERTY_EDIT( m_secondInputValue, TXT("Variable value representing second input") );		
	PROPERTY( m_cachedFirstInputNode );
	PROPERTY( m_cachedSecondInputNode );
	PROPERTY( m_cachedControlVariableNode );
END_CLASS_RTTI();


