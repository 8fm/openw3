/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../engine/extAnimBehaviorEvents.h"
#include "behaviorConstraintNode.h"
#include "behaviorGraphMimicNodes.h"
#include "behaviorIkSolverLookAt.h"
#include "behaviorIkSolverMimicLookAt.h"

class CBehaviorGraphValueNode;

class CBehaviorGraphLookAtSystemNode	: public CBehaviorGraphConstraintNode
										, public IQuatLookAtChainSolver
										, public IConstraintTarget
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphLookAtSystemNode, CBehaviorGraphConstraintNode, "Constraints", "Look at system" );

protected:
	String					m_firstBone;
	String					m_secondBone;
	String					m_thirdBone;

	Vector					m_localOffset;

	Float					m_deadZone;
	Float					m_deadZoneDist;

	Float					m_limitDampTime;
	Float					m_range;
	Float					m_levelDampTime;
	CCurve*					m_internalDampCurve;
	Float					m_dampForFirstTarget;

	Float					m_firstWeight;
	Float					m_secondWeight;
	Float					m_thirdWeight;

	static const Float		SWITCH_SMOOTH;

	static const Uint32		COMPRESSED_PARAM_DAMP_SPEED = 0;
	static const Uint32		COMPRESSED_PARAM_FOLLOW_SPEED = 1;
	static const Uint32		COMPRESSED_PARAM_AUTO_LIMIT_DEACT = 2;
	static const Uint32		COMPRESSED_PARAM_LIMIT_RANGE = 3;

public:
	static const CName		EVT_FORCE_STATE;

protected:
	TInstanceVar< Int32 >		i_firstBoneIndex;
	TInstanceVar< Int32 >		i_secondBoneIndex;
	TInstanceVar< Int32 >		i_thirdBoneIndex;

	TInstanceVar< Float >	i_currWeightFirst;
	TInstanceVar< Float >	i_destWeightFirst;
	TInstanceVar< Float >	i_currWeightSecAndThird;
	TInstanceVar< Float >	i_destWeightSecAndThird;

	TInstanceVar< Float >	i_limitCurrWeight;
	TInstanceVar< Float	>	i_limitDestWeight;
	TInstanceVar< Bool >	i_autoLimitDeact;
	TInstanceVar< Float >	i_limitStartRange;

	TInstanceVar< Vector >	i_weightsScale;

	TInstanceVar< Vector >	i_deadZoneTarget;
	TInstanceVar< Vector >	i_deadZoneSnappedTarget;
	TInstanceVar< Vector >	i_dampedTarget;

	TInstanceVar< Uint32 >	i_evtForceState;
	TInstanceVar< Bool >	i_forceState;

protected:
	CBehaviorGraphValueNode*		m_cachedLevelVariableNode;
	CBehaviorGraphVectorValueNode*	m_cachedWeightsVariableNode;
	CBehaviorGraphValueNode*		m_cachedLimitVariableNode;
	CBehaviorGraphVectorValueNode*	m_cachedCompressedDataVariableNode;

public:
	CBehaviorGraphLookAtSystemNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
	virtual void OnSpawned( const GraphBlockSpawnInfo& info );
	virtual void OnRebuildSockets();
#endif

	virtual void CacheConnections();

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

protected:
#ifdef USE_HAVOK_ANIMATION
	virtual hkQsTransform CalcTargetFromOutput( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output ) const;
#else
	virtual RedQsTransform CalcTargetFromOutput( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output ) const;
#endif
	virtual void OnConstraintActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnConstraintDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual Float GetDampSpeed( CBehaviorGraphInstance& instance ) const;
	virtual Float GetFollowSpeed( CBehaviorGraphInstance& instance ) const;

	virtual void ActivateInputs( CBehaviorGraphInstance& instance ) const;
	virtual void DeactivateInputs( CBehaviorGraphInstance& instance ) const;

	void UpdateDampedTarget( CBehaviorGraphInstance& instance, Float timeDelta ) const;
	void UpdateDeadZoneTarget( CBehaviorGraphInstance& instance ) const;
#ifdef USE_HAVOK_ANIMATION
	hkVector4 GetDampedTarget( CBehaviorGraphInstance& instance ) const;
	hkVector4 GetDeadZoneTarget( CBehaviorGraphInstance& instance ) const;
	hkVector4 GetDeadZoneSnappedTarget( CBehaviorGraphInstance& instance ) const;
	hkVector4 GetLookAtTarget( CBehaviorGraphInstance& instance ) const;
#else
	RedVector4 GetDampedTarget( CBehaviorGraphInstance& instance ) const;
	RedVector4 GetDeadZoneTarget( CBehaviorGraphInstance& instance ) const;
	RedVector4 GetDeadZoneSnappedTarget( CBehaviorGraphInstance& instance ) const;
	RedVector4 GetLookAtTarget( CBehaviorGraphInstance& instance ) const;
#endif
	Bool HasDeadZone() const;
	Bool HasDeadZoneSnapping() const;
	Bool HasDampedTarget() const;
	
	ChainSolverData GetSolverData( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &pose ) const;

	Float GetLimitWeight( CBehaviorGraphInstance& instance ) const;
	Float GetSolverWeightFirst( CBehaviorGraphInstance& instance ) const;
	Float GetSolverWeightSecAndThird( CBehaviorGraphInstance& instance ) const;
	Float GetLimitStartFromRange( Float limitRange ) const;

	void InternalReset( CBehaviorGraphInstance& instance ) const;
	void CheckLimits( CBehaviorGraphInstance& instance ) const;
	void CheckWeights( CBehaviorGraphInstance& instance ) const;
	void UpdateTimers( CBehaviorGraphInstance& instance, Float timeDelta ) const;
	Float CalcLookAtWeight( const Float dest, const Float curr, const Float timeDelta, const Float timeScale ) const;
	Float GetDeadZoneWeight( Float min, Float max, SolverData& data ) const;

	void ForceActivatedState( CBehaviorGraphInstance& instance, Bool flag ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphLookAtSystemNode );
	PARENT_CLASS( CBehaviorGraphConstraintNode );
	PROPERTY( m_cachedLevelVariableNode );
	PROPERTY( m_cachedWeightsVariableNode );
	PROPERTY( m_cachedLimitVariableNode );
	PROPERTY( m_cachedCompressedDataVariableNode );
	PROPERTY_CUSTOM_EDIT( m_firstBone, TXT("First bone name"), TXT("BehaviorBoneSelection"));
	PROPERTY_CUSTOM_EDIT( m_secondBone, TXT("Second bone name"), TXT("BehaviorBoneSelection"));
	PROPERTY_CUSTOM_EDIT( m_thirdBone, TXT("Third bone name ( head )"), TXT("BehaviorBoneSelection"));
	PROPERTY_EDIT( m_localOffset, TXT("Local offset ( eyes )") );
	PROPERTY_EDIT( m_deadZone, TXT("Dead zone") );
	PROPERTY_EDIT( m_deadZoneDist, TXT("Dead zone") );
	PROPERTY_EDIT_RANGE( m_limitDampTime, TXT("[0.01, 10.0]"), 0.01f, 10.f );
	PROPERTY_EDIT_RANGE( m_levelDampTime, TXT("[0.01, 10.0]"), 0.01f, 10.f );
	PROPERTY_CUSTOM_EDIT( m_internalDampCurve, TXT("Damp curve for limit and level"), TXT("CurveSelection") );
	PROPERTY_EDIT( m_dampForFirstTarget, TXT("Damp for target for first bone") );
	PROPERTY_EDIT( m_range, TXT("Limit range (0-360)") );
	PROPERTY_EDIT( m_firstWeight, TXT("") );
	PROPERTY_EDIT( m_secondWeight, TXT("") );
	PROPERTY_EDIT( m_thirdWeight, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicLookAtSystemNode	: public CBehaviorGraphBaseMimicNode
											, public IBehaviorGraphBonesPropertyOwner
											, public IMimicLookAtSolver
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicLookAtSystemNode, CBehaviorGraphBaseMimicNode, "Mimic", "Look at system" );

protected:
	String						m_eyeHorLeftTrack;
	String						m_eyeHorRightTrack;
	String						m_eyeVerLeftTrack;
	String						m_eyeVerRightTrack;
	String						m_eyeLeftPlacerBone;
	String						m_eyeRightPlacerBone;
	Float						m_eyeHorMax;
	Float						m_eyeVerMax;

	CCurve*						m_dampCurve;
	Float						m_dampTime;

protected:
	TInstanceVar< Int32 >			i_eyeHorLeft;
	TInstanceVar< Int32 >			i_eyeHorRight;
	TInstanceVar< Int32 >			i_eyeVerLeft;
	TInstanceVar< Int32 >			i_eyeVerRight;
	TInstanceVar< Int32 >			i_eyeLeftPlacer;
	TInstanceVar< Int32 >			i_eyeRightPlacer;
	TInstanceVar< Vector >		i_eyeLeftDirLS;
	TInstanceVar< Vector >		i_eyeRightDirLS;
	TInstanceVar< Vector >		i_target;
	TInstanceVar< Float >		i_destWeight;
	TInstanceVar< Float >		i_currWeight;
	TInstanceVar< Float >		i_destWeightThird;
	TInstanceVar< Float >		i_currWeightThird;

protected:
	CBehaviorGraphVectorValueNode*	m_cachedTargetNode;
	CBehaviorGraphValueNode*		m_cachedControlVariableNode;
	CBehaviorGraphValueNode*		m_cachedLevelVariableNode;

public:
	CBehaviorGraphMimicLookAtSystemNode();

	virtual CSkeleton* GetBonesSkeleton( CAnimatedComponent* component ) const;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
	virtual void OnRebuildSockets();
	virtual void OnSpawned( const GraphBlockSpawnInfo& info );
#endif
	virtual void CacheConnections();

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

protected:
	void CheckLookAtLevel( CBehaviorGraphInstance& instance )const;
	Float GetLookAtWeight( CBehaviorGraphInstance& instance )const;
	void UpdateWeight( CBehaviorGraphInstance& instance, Float timeDelta )const;
	void InternalReset( CBehaviorGraphInstance& instance )const;
	Bool GetSolversData( SolverData& leftEye,  SolverData& rightEye, CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &animPose ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicLookAtSystemNode );
	PARENT_CLASS( CBehaviorGraphBaseMimicNode );
	PROPERTY( m_cachedTargetNode );
	PROPERTY( m_cachedControlVariableNode );
	PROPERTY( m_cachedLevelVariableNode );
	PROPERTY_CUSTOM_EDIT( m_eyeHorLeftTrack, TXT("Name of the left eye float track"), TXT("BehaviorTrackSelection") );
	PROPERTY_CUSTOM_EDIT( m_eyeVerLeftTrack, TXT("Name of the left eye float track"), TXT("BehaviorTrackSelection") );
	PROPERTY_CUSTOM_EDIT( m_eyeHorRightTrack, TXT("Name of the right eye float track"), TXT("BehaviorTrackSelection") );
	PROPERTY_CUSTOM_EDIT( m_eyeVerRightTrack, TXT("Name of the left eye float track"), TXT("BehaviorTrackSelection") );
	PROPERTY_EDIT( m_eyeLeftPlacerBone, TXT("Placer for left eye") );
	PROPERTY_EDIT( m_eyeRightPlacerBone, TXT("Placer for right eye") );
	PROPERTY_EDIT( m_eyeHorMax, TXT("Max horizontal angle for pose") );
	PROPERTY_EDIT( m_eyeVerMax, TXT("Max vertical angle for pose") );
	PROPERTY_EDIT( m_dampTime, TXT("Damp time") );
	PROPERTY_CUSTOM_EDIT( m_dampCurve, TXT("Damp curve"), TXT("CurveSelection") );
END_CLASS_RTTI();
