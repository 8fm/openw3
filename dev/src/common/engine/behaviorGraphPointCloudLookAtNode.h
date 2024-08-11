
#pragma once

class CBehaviorGraphValueNode;
class CBehaviorGraphVectorValueNode;

#include "behaviorGraphNode.h"
#include "behaviorIncludes.h"

//////////////////////////////////////////////////////////////////////////

class IBehaviorGraphPointCloudLookAtTransition : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IBehaviorGraphPointCloudLookAtTransition, CObject );

public:
	struct Input
	{
		Vector	m_targetA;
		Vector	m_targetB;
		Float	m_targetBlend;
		Float	m_weight;
		Float	m_transitionWeight;
		Vector	m_boneDirLS;
		AnimQsTransform m_boneMS;
	};

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) {}

	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const {}
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const {}

	virtual void Update( CBehaviorGraphInstance& instance, Float timeDelta ) const {}
	
	virtual Vector CalcTarget( CBehaviorGraphInstance& instance, const Input& input ) const { return Vector::ZERO_3D_POINT; }
	virtual Vector GetTarget( CBehaviorGraphInstance& instance ) const { return Vector::ZERO_3D_POINT; }
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehaviorGraphPointCloudLookAtTransition );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphPointCloudLookAtSecMotion : public CObject
{
	DECLARE_ENGINE_CLASS( CBehaviorGraphPointCloudLookAtSecMotion, CObject, 0 );

private:
	Bool					m_isEnabled;

	TDynArray< Int32 >		m_masterBones;
	EAxis					m_masterBoneAxis;
	Float					m_maxMasterMotionAngleDeg;

	CName					m_defaultAnimation;

private:
	TInstanceVar< CSkeletalAnimationSetEntry* >	i_animation;

public:
	CBehaviorGraphPointCloudLookAtSecMotion();

	void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	void SampleTransitionPre( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &poseA, const SBehaviorGraphOutput& poseB, Float progress ) const;
	void SampleTransitionPost( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float progress ) const;

#ifndef NO_EDITOR
	void OnCreatedInEditor() override;
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphPointCloudLookAtSecMotion );
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_isEnabled, String::EMPTY );
	PROPERTY_EDIT( m_masterBones, String::EMPTY );
	PROPERTY_EDIT( m_masterBoneAxis, String::EMPTY );
	PROPERTY_EDIT( m_maxMasterMotionAngleDeg, String::EMPTY );
	PROPERTY_EDIT( m_defaultAnimation, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphPointCloudLookAtNode	: public CBehaviorGraphBaseNode
											, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphPointCloudLookAtNode, CBehaviorGraphBaseNode, "Look ats", "Point cloud" );

public:
	struct Params
	{
		Float	m_progress;
		Float	m_weightA;
		Float	m_weightB;
		Float	m_duration;
		Bool	m_useSecBlendCloudA;
		Bool	m_useSecBlendCloudB;
		Bool	m_useDeformationMS;
		
		Params() : m_progress( 0.f ), m_weightA( 0.f ), m_useSecBlendCloudA( false ), m_weightB( 0.f ), m_useSecBlendCloudB( false ), m_duration(0.f), m_useDeformationMS(true) {}
	};

private:
	enum EPoseNum
	{
		Pose_A,
		Pose_B,

		COUNT
	};

protected:
	CName							m_lookAtName;

	String							m_boneName;
	CName							m_animationA;
	CName							m_animationB;

	Bool							m_additiveMode;
	EAdditiveType					m_additiveType;
	Bool							m_writeToPoseLikeAdditiveNode;

	Bool							m_convertAnimationToAdditiveFlagA;
	Int32							m_convertAnimationToAdditiveRefFrameNumA;
	Bool							m_convertAnimationToAdditiveFlagB;
	Int32							m_convertAnimationToAdditiveRefFrameNumB;

	TDynArray< SBehaviorGraphBoneInfo >			m_lowerBodyPartBones;

	CCurve*										m_targetWeightCurve;
	CCurve*										m_targetWeightCurve2;
	CCurve*										m_headDownCurve;
	CCurve*										m_headProgressCurve;
	CCurve*										m_handDragCurve;
	String										m_handL;
	String										m_handR;
	Bool										m_useBlendInsteadOfTargetTransition;
	IBehaviorGraphPointCloudLookAtTransition*	m_transition;
	Bool										m_useTransitionWeightPred;
	TDynArray< CName >							m_transitionBonesPred;
	CCurve*										m_transitionPredCurve;

	CBehaviorGraphPointCloudLookAtSecMotion	*	m_secondaryMotion;

protected:
	TInstanceVar< CSkeletalAnimationSetEntry* >	i_animationA;
	TInstanceVar< CSkeletalAnimationSetEntry* >	i_animationB;
	TInstanceVar< CAnimPointCloudLookAtParam* >	i_paramsA;
	TInstanceVar< CAnimPointCloudLookAtParam* >	i_paramsB;
	TInstanceVar< Int32 >						i_boneIndex;
	TInstanceVar< CAllocatedBehaviorGraphOutput > i_cachedPoseForAdditiveModeA;
	TInstanceVar< CAllocatedBehaviorGraphOutput > i_cachedPoseForAdditiveModeB;
	TInstanceVar< IBehaviorGraphPointCloudLookAtTransition* >	i_transition;
	TInstanceVar< TDynArray< Int32 > >							i_transitionBonesPred;

#ifndef NO_EDITOR
	TInstanceVar< TDynArray< Matrix > >				i_lastBoneWS_Pre;
	TInstanceVar< TDynArray< Matrix > >				i_lastBoneWS_Post;
	TInstanceVar< TDynArray< Vector > >				i_lastBoneDir;
	TInstanceVar< TDynArray< Vector > >				i_lastPointPosOnSphere;
	TInstanceVar< TDynArray< TDynArray< Int32 > > >	i_lastTri;
	TInstanceVar< TDynArray< TDynArray< Float > > >	i_lastWeights;
	TInstanceVar< TDynArray< Int32 > >				i_lastNNpoint;
	TInstanceVar< TDynArray< Vector > >				i_handPoints;
#endif

	TInstanceVar< Float >						i_progress;
	TInstanceVar< Float >						i_weightA;
	TInstanceVar< Float >						i_weightB;
	TInstanceVar< Bool >						i_useSecBlendCloudA;
	TInstanceVar< Bool >						i_useSecBlendCloudB;
	TInstanceVar< Float >						i_lowerBodyPartsWeight;
	TInstanceVar< TDynArray< Int32 > >			i_lowerBodyPartBones;
	TInstanceVar< Vector >						i_targetA;
	TInstanceVar< Vector >						i_targetB;
	TInstanceVar< Bool >						i_targetSetA;
	TInstanceVar< Bool >						i_targetSetB;
	TInstanceVar< Int32 >						i_cookieA;
	TInstanceVar< Int32 >						i_cookieB;
	TInstanceVar< Int32 >						i_handL;
	TInstanceVar< Int32 >						i_handR;
	TInstanceVar< Float >						i_duration;
	TInstanceVar< Bool >						i_useDeformationMS;

	// Warning! This is the guard for animation streaming and at the same time encodes 
	// information about which exactly animation we are waiting for - in a form of bit mask.
	TInstanceVar< Uint32 >						i_waitingForAnimationFullyLoad;

protected:
	CBehaviorGraphValueNode*		m_cachedProgressNode;
	CBehaviorGraphValueNode*		m_cachedWeightANode;
	CBehaviorGraphValueNode*		m_cachedWeightBNode;
	CBehaviorGraphValueNode*		m_cachedUseSecBlendANode;
	CBehaviorGraphValueNode*		m_cachedUseSecBlendBNode;
	CBehaviorGraphValueNode*		m_cachedDurationNode;
	CBehaviorGraphNode*				m_cachedFallbackNode;
	CBehaviorGraphVectorValueNode*	m_cachedTargetNodeA;
	CBehaviorGraphVectorValueNode*	m_cachedTargetNodeB;

public:
	CBehaviorGraphPointCloudLookAtNode();

	virtual void OnPostLoad();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return TXT("Look at (Point Cloud)"); }
	virtual void OnSpawned(const GraphBlockSpawnInfo& info );
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

	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

	virtual TDynArray<SBehaviorGraphBoneInfo>* GetBonesProperty() override { return &m_lowerBodyPartBones; }

public:
	RED_INLINE const CName& GetLookAtName() const { return m_lookAtName; }

	void SetAnimations( CBehaviorGraphInstance& instance, const CName& animationA, const CName& animationB ) const;
	void SetTargetA( CBehaviorGraphInstance& instance, const Vector& point ) const;
	void SetTargetB( CBehaviorGraphInstance& instance, const Vector& point ) const;
	void ResetTargetA( CBehaviorGraphInstance& instance ) const;
	void ResetTargetB( CBehaviorGraphInstance& instance ) const;
	//void SetWeight( CBehaviorGraphInstance& instance, Float progress, Float weightValue ) const;
	//void SetBlendWeight( CBehaviorGraphInstance& instance, Float w ) const;
	//void SetTargetWeight( CBehaviorGraphInstance& instance, Float w ) const;
	//void SetTransitionWeight( CBehaviorGraphInstance& instance, Float w ) const;
	void SetLowerBodyPartsWeight( CBehaviorGraphInstance& instance, Float w ) const;
	void ResetParams( CBehaviorGraphInstance& instance ) const;
	void SetParams( CBehaviorGraphInstance& instance, const Params& params ) const;

private:
	CSkeletalAnimationSetEntry* FindAnimation( CBehaviorGraphInstance& instance, const CName& animationName ) const;

	void SampleAnimation( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Int32 pose, EPoseNum poseNum ) const;
	void SampleInternal( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const AnimQsTransform& boneMS, const AnimQsTransform& l2w, Float weight, const Vector& vPoint, Float blendWeight ) const;
	void SamplePointCloud( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const AnimQsTransform& boneMS, const AnimQsTransform& l2w, Float weight, const Vector& vPoint, EPoseNum poseNum ) const;

	void ConvertPoseToAdditive( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, EPoseNum poseNum ) const;

	// Returns true if pose have been cached, false otherwise and encodes that we are still waiting for the pose to be cached.
	Bool TryCachePoseForAdditiveMode( CBehaviorGraphInstance& instance, EPoseNum poseNum ) const;

	// Returns false if we still have to wait for animation streaming.
	Bool IsNodeReadyToSample( CBehaviorGraphInstance& instance ) const;

	void FreePose( CBehaviorGraphInstance& instance, EPoseNum poseNum ) const;

	void BlendPoses3( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Int32 poses[3], Float weights[3], Float mainWeight, EPoseNum poseNum ) const;
	void BlendPoses2( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Int32 poseA, Int32 poseB, Float weightA, Float weightB, Float mainWeight, EPoseNum poseNum ) const;

	void BlendOutLowerBodyParts( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	void RefreshSingleAnimation( CBehaviorGraphInstance& instance, const CName& animation, EPoseNum poseNum ) const;
	void RefreshBothAnimations( CBehaviorGraphInstance& instance, const CName& animationA, const CName& animationB ) const;

	void AddAnimationUsage( CSkeletalAnimationSetEntry* animEntry ) const;
	void ReleaseAnimationUsage( CSkeletalAnimationSetEntry* animEntry ) const;

	Bool ConvertAnimationToAdditiveFlag( EPoseNum poseNum ) const;

	//Vector CalcTarget( CBehaviorGraphInstance& instance, const AnimQsTransform& boneMS ) const;
	//Float GetTargetWeight( CBehaviorGraphInstance& instance ) const;
	Float MapTargetWeight( CBehaviorGraphInstance& instance, Float weight ) const;
	//Vector GetTarget( CBehaviorGraphInstance& instance ) const;

	void DrawPointCloundData( CBehaviorGraphInstance& instance, CRenderFrame* frame, const Vector& vPoint, EPoseNum poseNum ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphPointCloudLookAtNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_lookAtName, TXT("Look at node name") );
	PROPERTY_CUSTOM_EDIT( m_boneName, TXT("Name of the bone we want to rotate"), TXT("BehaviorBoneSelection"));
	PROPERTY_CUSTOM_EDIT( m_animationA, TXT("Animation name"), TXT("BehaviorAnimSelection") );
	PROPERTY_CUSTOM_EDIT( m_animationB, TXT("Animation name"), TXT("BehaviorAnimSelection") );
	PROPERTY_EDIT( m_additiveMode, TXT("") );
	PROPERTY_EDIT( m_additiveType, TXT("") );
	PROPERTY_EDIT( m_writeToPoseLikeAdditiveNode, TXT("") );
	PROPERTY_EDIT( m_convertAnimationToAdditiveFlagA, TXT("Use with care") );
	PROPERTY_EDIT( m_convertAnimationToAdditiveRefFrameNumA, TXT("Use with care") );
	PROPERTY_EDIT( m_convertAnimationToAdditiveFlagB, TXT("Use with care") );
	PROPERTY_EDIT( m_convertAnimationToAdditiveRefFrameNumB, TXT("Use with care") );
	PROPERTY_CUSTOM_EDIT_NAME( m_lowerBodyPartBones, TXT("Lower body part bones"), String::EMPTY, TXT("BehaviorBoneMultiSelection") );	
	PROPERTY_CUSTOM_EDIT( m_targetWeightCurve, String::EMPTY, TXT("CurveSelection") );
	PROPERTY_CUSTOM_EDIT( m_targetWeightCurve2, String::EMPTY, TXT("CurveSelection") );
	PROPERTY_CUSTOM_EDIT( m_headDownCurve, String::EMPTY, TXT("CurveSelection") );
	PROPERTY_CUSTOM_EDIT( m_headProgressCurve, String::EMPTY, TXT("CurveSelection") );
	PROPERTY_CUSTOM_EDIT( m_handL, TXT(""), TXT("BehaviorBoneSelection"));
	PROPERTY_CUSTOM_EDIT( m_handR, TXT(""), TXT("BehaviorBoneSelection"));
	PROPERTY_CUSTOM_EDIT( m_handDragCurve, String::EMPTY, TXT("CurveSelection") );
	PROPERTY_EDIT( m_useBlendInsteadOfTargetTransition, TXT("") );
	PROPERTY_INLINED( m_transition, TXT("") );
	PROPERTY_EDIT( m_useTransitionWeightPred, TXT("") );
	PROPERTY_EDIT( m_transitionBonesPred, TXT("") );
	PROPERTY_CUSTOM_EDIT( m_transitionPredCurve, String::EMPTY, TXT("CurveSelection") );
	PROPERTY_INLINED( m_secondaryMotion, TXT("") );
	PROPERTY( m_cachedTargetNodeA );
	PROPERTY( m_cachedTargetNodeB );
	PROPERTY( m_cachedFallbackNode );
	PROPERTY( m_cachedProgressNode );
	PROPERTY( m_cachedWeightANode );
	PROPERTY( m_cachedWeightBNode );
	PROPERTY( m_cachedUseSecBlendANode );
	PROPERTY( m_cachedUseSecBlendBNode );
	PROPERTY( m_cachedDurationNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorPointCloudLookAtInterface
{
	THandle< CBehaviorGraphPointCloudLookAtNode >	m_node;
	THandle< CBehaviorGraphInstance >				m_instance;

public:
	CBehaviorPointCloudLookAtInterface();
	~CBehaviorPointCloudLookAtInterface();

	void Init( CBehaviorGraphPointCloudLookAtNode* node, CBehaviorGraphInstance* instance );
	void Clear();

	Bool IsValid() const;

public:
	void SetAnimations( const CName& animationA, const CName& animationB );
	void SetTargetA( const Vector& point );
	void SetTargetB( const Vector& point );
	void ResetTargetA();
	void ResetTargetB();
	//void SetWeight( Float progress, Float weightValue );
	//void SetBlendWeight( Float w );
	//void SetTargetWeight( Float w );
	//void SetTransitionWeight( Float w );
	void SetLowerBodyPartsWeight( Float w );

	void ResetParams();
	void SetParams( const CBehaviorGraphPointCloudLookAtNode::Params& params );

private:
	Bool IsValid( CBehaviorGraphPointCloudLookAtNode*& node, CBehaviorGraphInstance*& instance ) const;
};

//////////////////////////////////////////////////////////////////////////

class IBehaviorGraphPointCloudLookAtTransition_Vector : public IBehaviorGraphPointCloudLookAtTransition
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IBehaviorGraphPointCloudLookAtTransition_Vector, IBehaviorGraphPointCloudLookAtTransition );

protected:
	TInstanceVar< Vector > i_target;

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;

	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const override;

	virtual Vector CalcTarget( CBehaviorGraphInstance& instance, const Input& input ) const override;
	virtual Vector GetTarget( CBehaviorGraphInstance& instance ) const override;
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehaviorGraphPointCloudLookAtTransition_Vector );
	PARENT_CLASS( IBehaviorGraphPointCloudLookAtTransition );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphPointCloudLookAtTransition_Vertical : public IBehaviorGraphPointCloudLookAtTransition_Vector
{
	DECLARE_ENGINE_CLASS( CBehaviorGraphPointCloudLookAtTransition_Vertical, IBehaviorGraphPointCloudLookAtTransition_Vector, 0 );

private:
	Float	m_maxAngleDiffDeg;
	Float	m_scale;
	Float	m_minAngle;
	Float	m_maxAngle;
	CCurve*	m_curve;

public:
	CBehaviorGraphPointCloudLookAtTransition_Vertical();
	
	virtual Vector CalcTarget( CBehaviorGraphInstance& instance, const Input& input ) const override;

#ifndef NO_EDITOR
	virtual void OnCreatedInEditor() override;
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphPointCloudLookAtTransition_Vertical );
	PARENT_CLASS( IBehaviorGraphPointCloudLookAtTransition_Vector );
	PROPERTY_EDIT( m_maxAngleDiffDeg, String::EMPTY );
	PROPERTY_EDIT( m_scale, String::EMPTY );
	PROPERTY_EDIT( m_minAngle, String::EMPTY );
	PROPERTY_EDIT( m_maxAngle, String::EMPTY );
	PROPERTY_CUSTOM_EDIT( m_curve, String::EMPTY, TXT("CurveSelection") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphPointCloudLookAtTransition_Spherical : public IBehaviorGraphPointCloudLookAtTransition_Vector
{
	DECLARE_ENGINE_CLASS( CBehaviorGraphPointCloudLookAtTransition_Spherical, IBehaviorGraphPointCloudLookAtTransition_Vector, 0 );

public:
	virtual Vector CalcTarget( CBehaviorGraphInstance& instance, const Input& input ) const override;
};

BEGIN_CLASS_RTTI( CBehaviorGraphPointCloudLookAtTransition_Spherical );
	PARENT_CLASS( IBehaviorGraphPointCloudLookAtTransition_Vector );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
