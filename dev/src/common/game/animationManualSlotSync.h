
#pragma once

#include "../engine/behaviorGraphAnimationManualSlot.h"
#include "movementAdjustor.h"

enum EAnimationManualSyncType
{
	AMST_SyncBeginning,
	AMST_SyncEnd,
	AMST_SyncMatchEvents
};

BEGIN_ENUM_RTTI( EAnimationManualSyncType )
	ENUM_OPTION( AMST_SyncBeginning );
	ENUM_OPTION( AMST_SyncEnd );
	ENUM_OPTION( AMST_SyncMatchEvents );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

enum ESyncRotationUsingRefBoneType
{
	SRT_TowardsOtherEntity,
	SRT_ToMatchOthersRotation,
};

BEGIN_ENUM_RTTI( ESyncRotationUsingRefBoneType )
	ENUM_OPTION( SRT_TowardsOtherEntity );
	ENUM_OPTION( SRT_ToMatchOthersRotation );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SAnimationSequencePartDefinition
{
	DECLARE_RTTI_STRUCT( SAnimationSequencePartDefinition );

	CName							m_animation;
	EAnimationManualSyncType		m_syncType;			// Attribute ignored for master
	CName							m_syncEventName;	// Attribute ignored for master
	Bool							m_shouldSlide;
	Bool							m_shouldRotate;
	CName							m_useRefBone;
	ESyncRotationUsingRefBoneType	m_rotationTypeUsingRefBone; // instead of matching other's rotation
	Vector							m_finalPosition;
	Float							m_finalHeading;
	Float							m_blendTransitionTime;
	Float							m_blendInTime;
	Float							m_blendOutTime;
	Float							m_allowBreakAtStart;
	CName							m_allowBreakAtStartBeforeEventsEnd;
	Float							m_allowBreakBeforeEnd;
	CName							m_allowBreakBeforeAtAfterEventsStart;
	Int32							m_sequenceIndex;
	Bool							m_disableProxyCollisions;
};

BEGIN_CLASS_RTTI( SAnimationSequencePartDefinition );
	PROPERTY( m_animation );
	PROPERTY( m_syncType );
	PROPERTY( m_syncEventName );
	PROPERTY( m_shouldSlide );
	PROPERTY( m_shouldRotate );
	PROPERTY( m_useRefBone );
	PROPERTY( m_rotationTypeUsingRefBone );
	PROPERTY( m_finalPosition );
	PROPERTY( m_finalHeading );
	PROPERTY( m_blendTransitionTime );
	PROPERTY( m_blendInTime );
	PROPERTY( m_blendOutTime );
	PROPERTY( m_allowBreakAtStart );
	PROPERTY( m_allowBreakAtStartBeforeEventsEnd );
	PROPERTY( m_allowBreakBeforeEnd );
	PROPERTY( m_allowBreakBeforeAtAfterEventsStart );
	PROPERTY( m_sequenceIndex );
	PROPERTY( m_disableProxyCollisions );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SAnimationSequenceDefinition
{
	DECLARE_RTTI_STRUCT( SAnimationSequenceDefinition );

	THandle< CEntity >								m_entity;
	CName											m_manualSlotName;
	TDynArray< SAnimationSequencePartDefinition >	m_parts;
	Bool											m_freezeAtEnd;
	CName											m_startForceEvent;
	CName											m_raiseEventOnEnd;
	CName											m_raiseForceEventOnEnd;
};

BEGIN_CLASS_RTTI( SAnimationSequenceDefinition );
	PROPERTY( m_entity );
	PROPERTY( m_manualSlotName );
	PROPERTY( m_parts );
	PROPERTY( m_freezeAtEnd );
	PROPERTY( m_startForceEvent );
	PROPERTY( m_raiseEventOnEnd );
	PROPERTY( m_raiseForceEventOnEnd );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CAnimationManualSlotSyncInstance : public CObject
{
	DECLARE_ENGINE_CLASS( CAnimationManualSlotSyncInstance, CObject, 0 );

private:

	struct SAnimationSequenceInstance;
	struct SAnimationSequencePartInstance
	{
		Float							m_startTime;
		Float							m_endTime;
		SAnimationState					m_animState;
		Float							m_movementStartTime;
		Float							m_movementDuration;
		CName							m_useRefBone;
		Bool							m_shouldSlide;
		Bool							m_shouldRotate;
		ESyncRotationUsingRefBoneType	m_rotationTypeUsingRefBone;
		Vector							m_finalPosition;
		Float							m_finalHeading;
		Float							m_blendTransitionTime;
		Float							m_blendInTime;
		Float							m_blendOutTime;
		Float							m_allowBreakAtStart;
		Float							m_allowBreakBeforeEnd;
		Uint32							m_sequenceIndex;
		Bool							m_disableProxyCollisions;

		Bool CanBreakOut() const;
		Float CalculateAnimationBlend();
		void Update( Float currentTime, Float deltaTime, Bool isMaster, CAnimationManualSlotSyncInstance& syncInstance, SAnimationSequenceInstance& sequence, SAnimationSequencePartInstance* nextPart );
	};

	typedef TDynArray< SAnimationSequencePartInstance >::iterator TSequencePartIterator;

	struct SAnimationSequenceInstance
	{
		THandle< CEntity >							m_entity;
		Bool										m_brokeOut;
		Bool										m_hasFinished;
		CBehaviorManualSlotInterface				m_slot;
		TDynArray< SAnimationSequencePartInstance >	m_parts;
		Bool										m_freezeAtEnd;
		Int32										m_currentPartIndex;
		CName										m_startForceEvent;
		Bool										m_startForceEventSent;
		CName										m_raiseEventOnEnd;
		CName										m_raiseForceEventOnEnd;
		Bool										m_endEventRaised;
		Bool										m_endForceEventRaised;
		Bool										m_disabledProxyCollisions;
		SMovementAdjustmentRequestTicket			m_movementAdjustTicket;

		void Stop();
		void Update( Float currentTime, Float deltaTime, Bool isMaster, CAnimationManualSlotSyncInstance& syncInstance );
		void OnSwitchPart( SAnimationSequencePartInstance* toPart );
		Bool HasStarted( Float currentTime ) const;
		Bool HasEnded() const;
		Bool BreakIfPossible( Bool isMaster, CAnimationManualSlotSyncInstance& syncInstance );
		Bool RaiseEndEvent();
	};

	TDynArray< SAnimationSequenceInstance >		m_sequenceInstances;	// Fist element is always the master

	Float									m_currentTime;				// Relative time to master animation ( can be negative - master animation is always started at 0.0f )
	Float									m_endTime;					// Time at which all the animations will be finished

	TDynArray< TDynArray<CExtAnimEvent*> >	m_cachedMasterEvents;

public:
	CAnimationManualSlotSyncInstance();

	Bool RegisterMaster( const SAnimationSequenceDefinition& masterDefinition );
	Bool RegisterSlave( const SAnimationSequenceDefinition& slaveDefinition );

	void Update( Float deltaTime );

	Bool HasEnded() const;

	void RemoveSlavesIfNotStarted();

	CEntity const * GetOtherEntity( Bool isMaster ) const;

	void BreakSlavesIfPossible();
	void BreakMasterIfPossible();

	Bool BreakIfPossible( CEntity const * entity ); // break from synchronization if possible

	Float GetCurrentTime() const { return m_currentTime; }

private:
	Bool InitNewSequenceInstance( const SAnimationSequenceDefinition& definition );

	void funcRegisterMaster( CScriptStackFrame& stack, void* result );
	void funcRegisterSlave( CScriptStackFrame& stack, void* result );
	void funcStopSequence( CScriptStackFrame& stack, void* result );
	void funcIsSequenceFinished( CScriptStackFrame& stack, void* result );
	void funcHasEnded( CScriptStackFrame& stack, void* result );
	void funcUpdate( CScriptStackFrame& stack, void* result );
	void funcBreakIfPossible( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CAnimationManualSlotSyncInstance );
PARENT_CLASS( CObject );
	NATIVE_FUNCTION( "RegisterMaster", funcRegisterMaster );
	NATIVE_FUNCTION( "RegisterSlave", funcRegisterSlave );
	NATIVE_FUNCTION( "StopSequence", funcStopSequence );
	NATIVE_FUNCTION( "IsSequenceFinished", funcIsSequenceFinished );
	NATIVE_FUNCTION( "HasEnded", funcHasEnded );
	NATIVE_FUNCTION( "Update", funcUpdate );
	NATIVE_FUNCTION( "BreakIfPossible", funcBreakIfPossible );
END_CLASS_RTTI();
