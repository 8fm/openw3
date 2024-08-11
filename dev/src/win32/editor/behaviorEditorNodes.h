/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphAnimationTrajDrawNode	: public CBehaviorGraphVectorValueNode
											, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphAnimationTrajDrawNode, CBehaviorGraphVectorValueNode, "Editor", "Draw traj" );	

protected:
	CName								m_animationName;

	String								m_bone;
	Bool								m_applyMotion;
	Bool								m_applyOffsets;

	Float								m_timeA;
	Float								m_timeB;

	Bool								m_generateRot;

	Bool								m_showLine;
	Bool								m_showRot;
	Bool								m_showVel;
	Bool								m_showCurrent;
	Bool								m_showPoints;

protected:
	TInstanceVar< CSkeletalAnimationSetEntry* > i_animation;

	TInstanceVar< Int32 >					i_bone;
	TInstanceVar< Float >				i_time;

	TInstanceVar< TDynArray< Vector > > i_allPositions;
	TInstanceVar< TDynArray< Vector > > i_allRotations;
	TInstanceVar< TDynArray< Vector > > i_allVels;

	TInstanceVar< Vector >				i_position;
	TInstanceVar< Vector >				i_rotation;

	TInstanceVar< Float >				i_minVel;
	TInstanceVar< Float >				i_maxVel;

	TInstanceVar< Vector >				i_offsetPosA;
	TInstanceVar< Vector >				i_offsetPosB;
	TInstanceVar< Vector >				i_offsetRotA;
	TInstanceVar< Vector >				i_offsetRotB;

	TInstanceVar< Matrix >				i_pointA;
	TInstanceVar< Matrix >				i_pointB;

protected:
	CBehaviorGraphNode*					m_cachedInputNode;
	CBehaviorGraphVectorValueNode*		m_cachedOffsetPosA;
	CBehaviorGraphVectorValueNode*		m_cachedOffsetPosB;
	CBehaviorGraphVectorValueNode*		m_cachedOffsetRotA;
	CBehaviorGraphVectorValueNode*		m_cachedOffsetRotB;

public:
	virtual void CacheConnections();

	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const { return 0.f; }
	virtual Vector GetVectorValue( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual String GetCaption() const { return TXT("Draw traj"); }
	virtual void OnRebuildSockets();
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphAnimationTrajDrawNode );
	PARENT_CLASS( CBehaviorGraphVectorValueNode );
	PROPERTY( m_cachedInputNode );
	PROPERTY( m_cachedOffsetPosA );
	PROPERTY( m_cachedOffsetPosB );
	PROPERTY( m_cachedOffsetRotA );
	PROPERTY( m_cachedOffsetRotB );
	PROPERTY_CUSTOM_EDIT( m_animationName, TXT("Animation name"), TXT("BehaviorAnimSelection") );
	PROPERTY_CUSTOM_EDIT( m_bone, TXT(""), TXT("BehaviorBoneSelection"));
	PROPERTY_EDIT( m_showLine, TXT("") );
	PROPERTY_EDIT( m_showRot, TXT("") );
	PROPERTY_EDIT( m_showVel, TXT("") );
	PROPERTY_EDIT( m_showCurrent, TXT("") );
	PROPERTY_EDIT( m_showPoints, TXT("") );
	PROPERTY_EDIT( m_generateRot, TXT("") );
	PROPERTY_EDIT( m_applyMotion, TXT("") );
	PROPERTY_EDIT( m_applyOffsets, TXT("") );
	PROPERTY_EDIT( m_timeA, TXT("") );
	PROPERTY_EDIT( m_timeB, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphRootMotionNode	: public CBehaviorGraphBaseNode
									, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphRootMotionNode, CBehaviorGraphBaseNode, "Editor", "Root motion" );

protected:
	CName				m_animationName;
	Bool				m_running;

protected:
	TInstanceVar< CSkeletalAnimationSetEntry* > i_animation;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String( TXT("Root motion") ); }
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphRootMotionNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_CUSTOM_EDIT( m_animationName, TXT("Animation name"), TXT("BehaviorAnimSelection") );
	PROPERTY_EDIT( m_running, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
#ifdef USE_HAVOK_ANIMATION
class FootIkEditorRaycastInterface : public hkaRaycastInterface
{
public:
	virtual hkBool castRay( const hkVector4& fromWS, const hkVector4& toWS, hkReal& hitFractionOut, hkVector4& normalWSOut );
};

class CBehaviorGraphIkFootNode	: public CBehaviorGraphBaseNode
								, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphIkFootNode, CBehaviorGraphBaseNode, "Constraints.Simple", "Foot" );

	enum ELeg
	{
		LeftLeg,
		RightLeg,
		MaxLegs
	};

	enum EBone
	{
		HipBoneLeft,
		KneeBoneLeft,
		AnkleBoneLeft,
		HipBoneRight,
		KneeBoneRight,
		AnkleBoneRight,
		BoneLast
	};

protected:
	String				m_hipBoneLeft;
	String				m_kneeBoneLeft;
	String				m_ankleBoneLeft;

	String				m_hipBoneRight;
	String				m_kneeBoneRight;
	String				m_ankleBoneRight;

	EAxis				m_kneeAxis;
	Vector				m_footEndLS;

	Float				m_minAnkleHeightMS;
	Float				m_maxAnkleHeightMS;

	Float				m_maxKneeAngle;
	Float				m_minKneeAngle;

	Float				m_footPlantedAnkleHeightMS;
	Float				m_footRaisedAnkleHeightMS;

	Float				m_raycastDistanceUp;
	Float				m_raycastDistanceDown;

protected:
	TInstanceVar< Float >				i_originalGroundHeightMS;
	TInstanceVar< TDynArray< Int32 > >	i_bones;
	TInstanceVar< Float >				i_weight;

	TInstanceVar< TGenericPtr >			i_raycast;
	TInstanceVar< TGenericPtr >			i_footPlacementSolverRight;
	TInstanceVar< TGenericPtr >			i_footPlacementSolverLeft;

protected:
	CBehaviorGraphValueNode*		m_cachedValueNode;
	CBehaviorGraphVectorValueNode*	m_cachedTargetNode;

public:
	CBehaviorGraphIkFootNode();

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return String( TXT("IkFoot") ); }
	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsOnReleaseInstanceManuallyOverridden() const override { return true; }

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

private:
	void CreateRaycast( CBehaviorGraphInstance& instance ) const;
	void DestroyRaycast( CBehaviorGraphInstance& instance ) const;
	FootIkEditorRaycastInterface* GetRaycast( CBehaviorGraphInstance& instance ) const;

	void CreateAndSetupFootPlacementSolvers( CBehaviorGraphInstance& instance ) const;
	void DestroyFootPlacementSolvers( CBehaviorGraphInstance& instance ) const;
	hkaFootPlacementIkSolver* GetFootPlacementSolver( CBehaviorGraphInstance& instance, ELeg leg ) const;

	void SyncPoseFromOutput( hkaPose& pose, SBehaviorGraphOutput &output ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphIkFootNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_CUSTOM_EDIT( m_hipBoneLeft, TXT("First bone name"), TXT("BehaviorBoneSelection"));
	PROPERTY_CUSTOM_EDIT( m_kneeBoneLeft, TXT("Second bone name"), TXT("BehaviorBoneSelection"));
	PROPERTY_CUSTOM_EDIT( m_ankleBoneLeft, TXT("End bone name"), TXT("BehaviorBoneSelection"));
	PROPERTY_CUSTOM_EDIT( m_hipBoneRight, TXT("First bone name"), TXT("BehaviorBoneSelection"));
	PROPERTY_CUSTOM_EDIT( m_kneeBoneRight, TXT("Second bone name"), TXT("BehaviorBoneSelection"));
	PROPERTY_CUSTOM_EDIT( m_ankleBoneRight, TXT("End bone name"), TXT("BehaviorBoneSelection"));
	PROPERTY_EDIT( m_kneeAxis, TXT("Hinge axis"));
	PROPERTY_EDIT( m_footEndLS, TXT(""));
	PROPERTY_EDIT( m_minAnkleHeightMS, TXT(""));
	PROPERTY_EDIT( m_maxAnkleHeightMS, TXT(""));
	PROPERTY_EDIT( m_maxKneeAngle, TXT(""));
	PROPERTY_EDIT( m_minKneeAngle, TXT(""));
	PROPERTY_EDIT( m_footPlantedAnkleHeightMS, TXT(""));
	PROPERTY_EDIT( m_footRaisedAnkleHeightMS, TXT(""));
	PROPERTY_EDIT( m_raycastDistanceUp, TXT(""));
	PROPERTY_EDIT( m_raycastDistanceDown, TXT(""));
END_CLASS_RTTI();
#endif