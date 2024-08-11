/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorConstraintNode.h"
#include "behaviorConstraintNodeLookAt.h"
#include "behaviorIkSolverLookAt.h"

class CBehaviorGraphConstraintNodeLookAt	: public CBehaviorGraphConstraintNode
											, public IEulerLookAtSolver
											, public IQuatLookAtSolver
											, public IConstraintTarget
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphConstraintNodeLookAt, CBehaviorGraphConstraintNode, "Constraints", "Look at" );

protected:
	String					m_bone;
	String					m_parentBone;
	EAxis					m_forwardDir;
	Vector					m_localOffset;

	Float					m_horizontalLimitAngle;
	Float					m_upLimitAngle;
	Float					m_downLimitAngle;
	EAxis					m_rangeLimitUpAxis;

	ELookAtSolverType		m_solverType;
	Float					m_deadZone;
	Float					m_deadZoneDist;
	
protected:
	TInstanceVar< Int32 >		i_boneIndex;
	TInstanceVar< Int32 >		i_parentBoneIndex;
	TInstanceVar< Vector >	i_deadZoneTarget;
	TInstanceVar< Vector >	i_deadZoneSnappedTarget;

public:
	CBehaviorGraphConstraintNodeLookAt();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
#endif

	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

protected:
#ifdef USE_HAVOK_ANIMATION
	virtual hkQsTransform CalcTargetFromOutput( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output ) const;
#else
	virtual RedQsTransform CalcTargetFromOutput( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output ) const;
#endif
	void UpdateDeadZoneTarget( CBehaviorGraphInstance& instance ) const;
#ifdef USE_HAVOK_ANIMATION
	hkVector4 GetDeadZoneTarget( CBehaviorGraphInstance& instance ) const;
	hkVector4 GetDeadZoneSnappedTarget( CBehaviorGraphInstance& instance ) const;
	hkVector4 GetLookAtTarget( CBehaviorGraphInstance& instance ) const;
#else
	RedVector4 GetDeadZoneTarget( CBehaviorGraphInstance& instance ) const;
	RedVector4 GetDeadZoneSnappedTarget( CBehaviorGraphInstance& instance ) const;
	RedVector4 GetLookAtTarget( CBehaviorGraphInstance& instance ) const;
#endif
	Bool HasDeadZone() const;
	Bool HasDeadZoneSnapping() const;

	IEulerLookAtSolver::SolverData GetBoneDataEuler( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &pose ) const;
	IQuatLookAtSolver::SolverData GetBoneDataQuat( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &pose ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphConstraintNodeLookAt );
	PARENT_CLASS( CBehaviorGraphConstraintNode );
	PROPERTY_CUSTOM_EDIT( m_bone, TXT("Bone name"), TXT("BehaviorBoneSelection"));
	PROPERTY_CUSTOM_EDIT( m_parentBone, TXT("Parent bone name"), TXT("BehaviorBoneSelection"));
	PROPERTY_EDIT( m_forwardDir, TXT("Forward bone direction") );
	PROPERTY_EDIT( m_localOffset, TXT("Translation offset ( e.g. eye position for head)") );
	PROPERTY_EDIT_RANGE( m_horizontalLimitAngle, TXT("Max horizontal angle [-180 180]"), -180.0f, 180.0f);
	PROPERTY_EDIT_RANGE( m_upLimitAngle, TXT("Max angle up [-90 90]"), -90.0f, 90.0f );
	PROPERTY_EDIT_RANGE( m_downLimitAngle, TXT("Max angle down [-90 90]"), -90.0f, 90.0f );
	PROPERTY_EDIT( m_rangeLimitUpAxis, TXT("Up axis") );
	PROPERTY_EDIT( m_solverType, TXT("Solver type") );
	PROPERTY_EDIT( m_deadZone, TXT("Dead zone") );
	PROPERTY_EDIT( m_deadZoneDist, TXT("Dead zonde max dist") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphLookAtNode	: public CBehaviorGraphBaseNode
								, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphLookAtNode, CBehaviorGraphBaseNode, "Constraints.Simple", "Look at" );

protected:
	String					m_boneName;
	Vector					m_axis;
	Bool					m_useLimits;
	Float					m_limitAngle;

protected:
	TInstanceVar< Int32 >		i_boneIndex;
	TInstanceVar< Float >	i_weight;
	TInstanceVar< Vector >	i_target;

#ifndef USE_GAME_SETTINGS
	TInstanceVar< Vector >	i_dirMS;
	TInstanceVar< Vector >	i_boneMS;
	TInstanceVar< Vector >	i_vecToTarget;
	TInstanceVar< Vector >	i_vecToTargetLimited;
#endif

protected:
	CBehaviorGraphValueNode*		m_cachedValueNode;
	CBehaviorGraphVectorValueNode*	m_cachedTargetNode;

public:
	CBehaviorGraphLookAtNode();

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return String( TXT("Look at") ); }
	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();
};

BEGIN_CLASS_RTTI( CBehaviorGraphLookAtNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_CUSTOM_EDIT( m_boneName, TXT("Name of the bone we want to translate"), TXT("BehaviorBoneSelection"));
	PROPERTY_EDIT( m_axis, TXT("Rotation axis") );
	PROPERTY_EDIT( m_useLimits, TXT("") );
	PROPERTY_EDIT( m_limitAngle, TXT("[Deg]") );
	PROPERTY( m_cachedValueNode );
	PROPERTY( m_cachedTargetNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphIk2Node	: public CBehaviorGraphBaseNode
							, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphIk2Node, CBehaviorGraphBaseNode, "Constraints.Simple", "Ik2" );

protected:
	String				m_firstBone;
	String				m_secondBone;
	String				m_endBone;

	EAxis				m_hingeAxis;

	Float				m_angleMax;
	Float				m_angleMin;

	Float				m_firstJointGain;
	Float				m_secondJointGain;
	Float				m_endJointGain;

	Bool				m_enforceEndPosition;
	Bool				m_enforceEndRotation;

	Vector				m_positionOffset;
	EulerAngles			m_rotationOffset;

protected:
	TInstanceVar< Int32 >		i_firstJointIdx;
	TInstanceVar< Int32 >		i_secondJointIdx;
	TInstanceVar< Int32 >		i_endBoneIdx;

	TInstanceVar< Float >	i_weight;

	TInstanceVar< Vector >	i_targetPos;
	TInstanceVar< Vector >	i_targetRot;

#ifndef USE_GAME_SETTINGS
	
#endif

protected:
	CBehaviorGraphValueNode*		m_cachedValueNode;
	CBehaviorGraphVectorValueNode*	m_cachedTargetPosNode;
	CBehaviorGraphVectorValueNode*	m_cachedTargetRotNode;

public:
	CBehaviorGraphIk2Node();

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return String( TXT("Ik2") ); }
	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

private:
	void CheckBones( CBehaviorGraphInstance& instance ) const;
#ifdef USE_HAVOK_ANIMATION
	void SetupSolver( CBehaviorGraphInstance& instance, hkaTwoJointsIkSolver::Setup& solver ) const;
	void SyncPoseFromOutput( hkaPose& pose, SBehaviorGraphOutput &output ) const;
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphIk2Node );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY( m_cachedValueNode );
	PROPERTY( m_cachedTargetPosNode );
	PROPERTY( m_cachedTargetRotNode );
	PROPERTY_CUSTOM_EDIT( m_firstBone, TXT("First bone name"), TXT("BehaviorBoneSelection"));
	PROPERTY_CUSTOM_EDIT( m_secondBone, TXT("Second bone name"), TXT("BehaviorBoneSelection"));
	PROPERTY_CUSTOM_EDIT( m_endBone, TXT("End bone name"), TXT("BehaviorBoneSelection"));
	PROPERTY_EDIT( m_hingeAxis, TXT("Hinge axis"));
	PROPERTY_EDIT_RANGE( m_angleMax, TXT("Max angle [0 180]"), 0.0f, 180.0f);
	PROPERTY_EDIT_RANGE( m_angleMin, TXT("Min nagle [0 180]"), 0.0f, 180.0f);
	PROPERTY_EDIT_RANGE( m_firstJointGain, TXT("First joint gain [0 1]"), 0.0f , 1.0f);
	PROPERTY_EDIT_RANGE( m_secondJointGain, TXT("Second joint gain [0 1]"), 0.0f, 1.0f);
	PROPERTY_EDIT_RANGE( m_endJointGain, TXT("End joint gain [0 1]"), 0.0f, 1.0f);
	PROPERTY_EDIT( m_enforceEndPosition, TXT("Enforce end position"));
	PROPERTY_EDIT( m_enforceEndRotation, TXT("Enforce end rotation"));
	PROPERTY_EDIT( m_positionOffset, TXT("Position local offset"));
	PROPERTY_EDIT( m_rotationOffset, TXT("Rotation local offset"));
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphIk3Node	: public CBehaviorGraphBaseNode
							, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphIk3Node, CBehaviorGraphBaseNode, "Constraints.Simple", "Ik3" );

protected:
	String				m_firstBone;
	String				m_secondBone;
	String				m_thirdBone;
	String				m_endBone;

	EAxis				m_hingeAxis;

protected:
	TInstanceVar< Int32 >		i_firstJointIdx;
	TInstanceVar< Int32 >		i_secondJointIdx;
	TInstanceVar< Int32 >		i_thirdJointIdx;
	TInstanceVar< Int32 >		i_endBoneIdx;

	TInstanceVar< Float >	i_weight;

	TInstanceVar< Vector >	i_targetPos;

#ifndef USE_GAME_SETTINGS

#endif

protected:
	CBehaviorGraphValueNode*		m_cachedValueNode;
	CBehaviorGraphVectorValueNode*	m_cachedTargetPosNode;

public:
	CBehaviorGraphIk3Node();

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return String( TXT("Ik3") ); }
	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

private:
	void CheckBones( CBehaviorGraphInstance& instance ) const;
#ifdef USE_HAVOK_ANIMATION
	void SetupSolver( CBehaviorGraphInstance& instance, const hkaPose &pose, hkaThreeJointsIkSolver::Setup& solver ) const;
	void SyncPoseFromOutput( hkaPose& pose, SBehaviorGraphOutput &output ) const;
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphIk3Node );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY( m_cachedValueNode );
	PROPERTY( m_cachedTargetPosNode );
	PROPERTY_CUSTOM_EDIT( m_firstBone, TXT("First bone name"), TXT("BehaviorBoneSelection"));
	PROPERTY_CUSTOM_EDIT( m_secondBone, TXT("Second bone name"), TXT("BehaviorBoneSelection"));
	PROPERTY_CUSTOM_EDIT( m_thirdBone, TXT("Second bone name"), TXT("BehaviorBoneSelection"));
	PROPERTY_CUSTOM_EDIT( m_endBone, TXT("End bone name"), TXT("BehaviorBoneSelection"));
	PROPERTY_EDIT( m_hingeAxis, TXT("Hinge axis"));
END_CLASS_RTTI();
