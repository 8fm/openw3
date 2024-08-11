/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphNode.h"
#include "behaviorIncludes.h"
#include "../core/enumBuilder.h"
#include "../core/engineQsTransform.h"

class CBehaviorGraphValueNode;

enum EBehaviorConstraintDampType
{
	BCDT_Duration,
	BCDT_Speed,
};

BEGIN_ENUM_RTTI( EBehaviorConstraintDampType );
	ENUM_OPTION( BCDT_Duration );
	ENUM_OPTION( BCDT_Speed );
END_ENUM_RTTI();

class CBehaviorGraphConstraintNode	: public CBehaviorGraphNode
									, public IBehaviorGraphBonesPropertyOwner
{	
	DECLARE_BEHAVIOR_ABSTRACT_CLASS( CBehaviorGraphConstraintNode, CBehaviorGraphNode );

public:
	enum EConstraintState 
	{
		CS_PrepareToActivating,
		CS_Activating, 
		CS_Activated, 
		CS_PrepareToDeactivating,
		CS_Deactivating, 
		CS_Deactivated 
	};

private:
	TInstanceVar< Bool >		i_isLimited;			//! Is constraint limited

	TInstanceVar< Uint32 >		i_state;				//!< Constraint state
	TInstanceVar< Float	>		i_progress;				//!< Constraint progress [0 1]
	TInstanceVar< Float >		i_distToTarget;			//!< Distance to target

	TInstanceVar< Float	>		i_dampCurveTimer;		//!< Damp curve timer
	TInstanceVar< Float	>		i_dampTimeAxisScale;	//!< Time axis scale, curve is [0 1]
	TInstanceVar< Float >		i_dampSpeed;			//!< Damp speed

	TInstanceVar< Float	>		i_followCurveTimer;		//!< Follow curve timer
	TInstanceVar< Float	>		i_followTimeAxisScale;	//!< Time axis scale, curve is [0 1]
	TInstanceVar< Float >		i_followSpeed;			//!< Follow speed

	TInstanceVar< Float	>		i_controlValue;			//!< Node control value
	TInstanceVar< Float	>		i_cacheControlValue;	//!< Cache control value - for deactivation process

	TInstanceVar< EngineQsTransform >	i_endTransform;			//!< Destination transform
	TInstanceVar< EngineQsTransform >	i_currTransform;		//!< Current transform
	TInstanceVar< EngineQsTransform >	i_startTransform;		//!< Starting transform
	TInstanceVar< EngineQsTransform >	i_posShift;				//!< Shift - position

protected:
	EBehaviorConstraintDampType	m_dampType;

	Bool						m_useDampCurve;			//!< Use damp curve
	CCurve*						m_dampCurve;			//!< Damp curve object
	Float						m_dampTimeAxisScale;	//!< Time axis scale, curve is [0 1]
	Float						m_dampTimeSpeed;		//!< Dump time speed
	

	Bool						m_useFollowCurve;		//!< Use follow curve
	CCurve*						m_followCurve;			//!< Follow curve object
	Float						m_followTimeAxisScale;	//!< Time axis scale, curve is [0 1]
	Float						m_followTimeSpeed;		//!< Follow time speed

	IBehaviorConstraintObject*	m_targetObject;			//!< Target object

	static const Float			ACTIVATION_THRESHOLD;

protected:
	CBehaviorGraphNode*			m_cachedInputNode;
	CBehaviorGraphValueNode*	m_cachedControlValueNode;
	CBehaviorGraphValueNode*	m_cachedDurationValueNode;
	CBehaviorGraphValueNode*	m_cachedDurationFollowValueNode;
	CBehaviorGraphValueNode*	m_cachedSpeedDampValueNode;
	CBehaviorGraphValueNode*	m_cachedSpeedFollowValueNode;

public:
	CBehaviorGraphConstraintNode();

	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;
	virtual void CacheConnections();

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty *prop );
	virtual void OnSpawned( const GraphBlockSpawnInfo& info );
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
	virtual Color GetTitleColor() const;
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

protected:
	void ActivateConstraint( CBehaviorGraphInstance& instance ) const;
	void DeactivateConstraint( CBehaviorGraphInstance& instance ) const;

	void ResetCurveTimersAndProgress( CBehaviorGraphInstance& instance ) const;
	Float UpdateFollowValue( CBehaviorGraphInstance& instance, Float dt ) const;
	Float UpdateDampValue( CBehaviorGraphInstance& instance, Float dt ) const;
	void ResetFollowTimer( CBehaviorGraphInstance& instance ) const;
	void ResetDampTimer( CBehaviorGraphInstance& instance ) const;
	void SetFollowTimer( CBehaviorGraphInstance& instance ) const;
	void SetDampTimer( CBehaviorGraphInstance& instance ) const;
	void SetProgress( CBehaviorGraphInstance& instance, Float progress ) const;

	void SetDistanceToTarget( CBehaviorGraphInstance& instance, Float dist ) const;
	Float GetDistanceToTarget( CBehaviorGraphInstance& instance ) const;

	void SetLimitFlag( CBehaviorGraphInstance& instance, Bool flag ) const;
	Bool GetLimitFlag( const CBehaviorGraphInstance& instance ) const;

	Uint32 GetState( CBehaviorGraphInstance& instance ) const;
	Bool ShouldUseInputs( CBehaviorGraphInstance& instance ) const;

	EngineQsTransform GetTargetStart( const CBehaviorGraphInstance& instance ) const;
	EngineQsTransform GetTargetEnd( const CBehaviorGraphInstance& instance ) const;
	EngineQsTransform GetTargetCurr( const CBehaviorGraphInstance& instance ) const;

	void SetTargetStart( CBehaviorGraphInstance& instance, const EngineQsTransform& trans ) const;
	void SetTargetEnd( CBehaviorGraphInstance& instance, const EngineQsTransform& trans ) const;
	void SetTargetCurr( CBehaviorGraphInstance& instance, const EngineQsTransform& trans ) const;

	void UpdateActionPrepareToActivating( CBehaviorGraphInstance& instance, Float dt ) const;
	void UpdateActionActivating( CBehaviorGraphInstance& instance, Float dt ) const;
	void UpdateActionActivated( CBehaviorGraphInstance& instance, Float dt ) const;
	void UpdateActionPrepareToDeactivating( CBehaviorGraphInstance& instance, Float dt ) const;
	void UpdateActionDeactivating( CBehaviorGraphInstance& instance, Float dt ) const;
	void UpdateActionDeactivated( CBehaviorGraphInstance& instance, Float dt ) const;

	void SampleActionPrepareToActivating( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
	void SampleActionActivating( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
	void SampleActionActivated( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
	void SampleActionPrepareToDeactivating( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
	void SampleActionDeactivating( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
	void SampleActionDeactivated( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	void UpdateTarget( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float dt ) const;

	void CalcPosShift( const EngineQsTransform& start, const EngineQsTransform& end, EngineQsTransform& shiftInOut ) const;
	void ApplyShift( const EngineQsTransform& start, const EngineQsTransform& shift, Float progress, const CCurve* curve, EngineQsTransform& transformInOut ) const;

	Bool IsConstraintActive( CBehaviorGraphInstance& instance ) const;
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform GetCurrentConstraintTransform( CBehaviorGraphInstance& instance ) const;
#else
	RedQsTransform GetCurrentConstraintTransform( CBehaviorGraphInstance& instance ) const;
#endif
	void SetState( CBehaviorGraphInstance& instance, EConstraintState state ) const;
#ifdef USE_HAVOK_ANIMATION
	void SyncPoseFromOutput( CBehaviorGraphInstance& instance, hkaPose& pose, SBehaviorGraphOutput &output) const;
#endif

public:
#ifdef USE_HAVOK_ANIMATION
	virtual void TransformToTrajectorySpace(SBehaviorGraphOutput &output, hkQsTransform& transform) const;
#else
	virtual void TransformToTrajectorySpace(SBehaviorGraphOutput &output, RedQsTransform& transform) const;
#endif
	Float GetControlValue( CBehaviorGraphInstance& instance ) const;
	Float GetSingedProgress( CBehaviorGraphInstance& instance ) const;
	Float GetProgress( CBehaviorGraphInstance& instance ) const;

	Bool IsTargetDamping( CBehaviorGraphInstance& instance ) const;
	Bool IsTagetFollowing( CBehaviorGraphInstance& instance ) const;
#ifdef USE_HAVOK_ANIMATION
	void LogPositon(const String& label, hkQsTransform& transform, Bool details = false) const; // For debug
#else
	void LogPositon(const String& label, RedQsTransform& transform, Bool details = false) const; // For debug
#endif
	void LogPositon(const String& label, Vector& vector, Bool details = false) const; // For debug

protected:
#ifdef USE_HAVOK_ANIMATION
	virtual hkQsTransform CalcTargetFromOutput( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output ) const = 0;
#else
	virtual RedQsTransform CalcTargetFromOutput( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output ) const = 0;
#endif

	virtual Float GetDampSpeed( CBehaviorGraphInstance& instance ) const;
	virtual Float GetFollowSpeed( CBehaviorGraphInstance& instance ) const;

	virtual void OnConstraintActivated( CBehaviorGraphInstance& /*instance*/ ) const {}
	virtual void OnConstraintDeactivated( CBehaviorGraphInstance& /*instance*/ ) const {}

	virtual void ActivateInputs( CBehaviorGraphInstance& instance ) const;
	virtual void DeactivateInputs( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	Color GetColorFromProgress( Float progress ) const;
#endif
};

BEGIN_ABSTRACT_CLASS_RTTI( CBehaviorGraphConstraintNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY_EDIT( m_useDampCurve, TXT("Use damp curve") );
	PROPERTY_CUSTOM_EDIT(m_dampCurve, TXT("Damp curve"), TXT("CurveSelection") );
	PROPERTY_EDIT( m_dampTimeAxisScale, TXT("Time axis scale for damp curve - for BCDT_Duration") );
	PROPERTY_EDIT( m_dampTimeSpeed, TXT("Damp time speed - for BCDT_Speed") );
	PROPERTY_EDIT( m_useFollowCurve, TXT("Use follow curve") );
	PROPERTY_CUSTOM_EDIT(m_followCurve, TXT("Follow curve"), TXT("CurveSelection") );
	PROPERTY_EDIT( m_followTimeAxisScale, TXT("Time axis scale for follow curve - for BCDT_Duration") );
	PROPERTY_EDIT( m_followTimeSpeed, TXT("Follow time speed - for BCDT_Speed") );
	PROPERTY_INLINED( m_targetObject, TXT("Target object") );
	PROPERTY_EDIT( m_dampType, TXT("") );
	PROPERTY( m_cachedInputNode );
	PROPERTY( m_cachedControlValueNode );
	PROPERTY( m_cachedDurationValueNode );
	PROPERTY( m_cachedDurationFollowValueNode );
	PROPERTY( m_cachedSpeedFollowValueNode );
	PROPERTY( m_cachedSpeedDampValueNode );
END_CLASS_RTTI();
