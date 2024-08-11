/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorIkSolverMimicLookAt.h"

class CBehaviorGraphValueNode;
struct SMimicPostProcessData;
//////////////////////////////////////////////////////////////////////////

class IBehaviorMimicConstraint : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IBehaviorMimicConstraint, CObject );

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void RebuildSockets( CBehaviorGraphNode* parent ) {}
#endif

public:
	virtual void CacheConnections( CBehaviorGraphNode* parent ) {}

	virtual void BuildDataLayout( InstanceDataLayoutCompiler& compiler ) {}
	virtual void InitInstance( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* parent ) const {}
	virtual void ReleaseInstance( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* parent ) const {}

	virtual void Update( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const = 0;
	virtual void PreSample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &animPose, SBehaviorGraphOutput &mimicPose ) const = 0;
	virtual void PostSample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &animPose, SBehaviorGraphOutput &floatTrackPose, SBehaviorGraphOutput &mimicPose ) const = 0;
	virtual void Reset( CBehaviorGraphInstance& instance ) const {}

	virtual void Activate( CBehaviorGraphInstance& instance ) const {}
	virtual void Deactivate( CBehaviorGraphInstance& instance ) const {}
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const {}
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const { return false; }

	virtual void GenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const {}
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehaviorMimicConstraint );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

class CBehaviorMimicLookAtConstraint	: public IBehaviorMimicConstraint
										, public IMimicLookAtSolver
{
	DECLARE_ENGINE_CLASS( CBehaviorMimicLookAtConstraint, IBehaviorMimicConstraint, 0 );

	enum ELongTransitionState
	{
		LTS_None,
		LTS_0_StartAndWait,
		LTS_1_WaitForBlink,
	};

private:
	String						m_eyeHorLLeftTrack;
	String						m_eyeHorRLeftTrack;
	String						m_eyeHorLRightTrack;
	String						m_eyeHorRRightTrack;

	String						m_eyeVerULeftTrack;
	String						m_eyeVerDLeftTrack;
	String						m_eyeVerURightTrack;
	String						m_eyeVerDRightTrack;

	String						m_eyeLeftPlacerBone;
	String						m_eyeRightPlacerBone;

	Float						m_eyeHorMax;
	Float						m_eyeVerMin;
	Float						m_eyeVerMax;
	Float						m_eyeVerOffset;

	Float						m_eyesTrackClamp;

	CName						m_blinkAnimName;
	Float						m_blinkTimeOffset;
	Float						m_blinkSpeed;

	Float						m_longTransitionAngleDeg;
	Float						m_longTransitionThrDeg;

	Float						m_dampTime;

private:
	TInstanceVar< Bool >		i_tracksValid;
	TInstanceVar< Float >		i_timeDelta;

	TInstanceVar< Int32 >		i_eyeHorLLeft;
	TInstanceVar< Int32 >		i_eyeHorRLeft;
	TInstanceVar< Int32 >		i_eyeHorLRight;
	TInstanceVar< Int32 >		i_eyeHorRRight;

	TInstanceVar< Int32 >		i_eyeVerULeft;
	TInstanceVar< Int32 >		i_eyeVerDLeft;
	TInstanceVar< Int32 >		i_eyeVerURight;
	TInstanceVar< Int32 >		i_eyeVerDRight;

	TInstanceVar< Int32 >		i_eyeLeftPlacer;
	TInstanceVar< Int32 >		i_eyeRightPlacer;
	TInstanceVar< Vector >		i_eyeLeftDirLS;
	TInstanceVar< Vector >		i_eyeRightDirLS;
	TInstanceVar< Vector >		i_target;
	TInstanceVar< Vector >		i_targetPrev;
	TInstanceVar< Float >		i_weight;
	TInstanceVar< Float >		i_weightPrev;
	TInstanceVar< Vector >		i_eyesCompressedData;
	TInstanceVar< Float >		i_shift;
	TInstanceVar< Float >		i_eyeHorLeftValue;
	TInstanceVar< Float >		i_eyeHorRightValue;
	TInstanceVar< Float >		i_eyeVerLeftValue;
	TInstanceVar< Float >		i_eyeVerRightValue;
	TInstanceVar< Float >		i_eyeHorLeftValue_Cached;
	TInstanceVar< Float >		i_eyeHorRightValue_Cached;
	TInstanceVar< Float >		i_eyeVerLeftValue_Cached;
	TInstanceVar< Float >		i_eyeVerRightValue_Cached;

	TInstanceVar< Int32 >		i_longTransitionMode;
	TInstanceVar< Vector >		i_longTransitionCachedTarget;
	TInstanceVar< Float >		i_longTransitionBlinkTimer;
	TInstanceVar< Float >		i_longTransitionBlinkAnimTime;
	TInstanceVar< CSkeletalAnimationSetEntry* > i_longTransitionBlinkAnim;
	TInstanceVar< Uint32 >		i_longTransitionEvtTargetChanged;
	TInstanceVar< Bool >		i_longTransitionTargetChangedFlag;
	
private:
	CBehaviorGraphVectorValueNode*	m_cachedTargetNode;
	CBehaviorGraphValueNode*		m_cachedControlVariableNode;
	CBehaviorGraphVectorValueNode*	m_cachedControlEyesDataNode;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void RebuildSockets( CBehaviorGraphNode* parent );
#endif

public:
	CBehaviorMimicLookAtConstraint();

	virtual void CacheConnections( CBehaviorGraphNode* parent ) override;

	virtual void BuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void InitInstance( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* parent ) const override;

	virtual void Update( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const override;
	virtual void PreSample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &animPose, SBehaviorGraphOutput &mimicPose ) const override;
	virtual void PostSample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &animPose, SBehaviorGraphOutput &floatTrackPose, SBehaviorGraphOutput &mimicPose ) const override {}
	virtual void Reset( CBehaviorGraphInstance& instance ) const override;

	virtual void Activate( CBehaviorGraphInstance& instance ) const override;
	virtual void Deactivate( CBehaviorGraphInstance& instance ) const override;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const override;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void GenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const override;

protected:
	Float GetShiftSpeed( CBehaviorGraphInstance& instance ) const;
	void UpdateShift( CBehaviorGraphInstance& instance, Float timeDelta ) const;
	void InternalReset( CBehaviorGraphInstance& instance ) const;
	Bool GetSolversData( SolverData& leftEye,  SolverData& rightEye, CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &animPose, const SBehaviorGraphOutput &mimicPose, const Vector& targetMS ) const;
	void ApplyEyesCoWeight( SolverData& leftEyeData, SolverData& rightEyeData, Float coWeight ) const;
};

BEGIN_CLASS_RTTI( CBehaviorMimicLookAtConstraint );
	PARENT_CLASS( IBehaviorMimicConstraint );
	PROPERTY( m_cachedTargetNode );
	PROPERTY( m_cachedControlVariableNode );
	PROPERTY( m_cachedControlEyesDataNode );
	PROPERTY_EDIT( m_eyeHorLLeftTrack, TXT("Name of the left eye float track") );
	PROPERTY_EDIT( m_eyeHorRLeftTrack, TXT("Name of the left eye float track") );
	PROPERTY_EDIT( m_eyeHorLRightTrack, TXT("Name of the left eye float track") );
	PROPERTY_EDIT( m_eyeHorRRightTrack, TXT("Name of the left eye float track") );
	PROPERTY_EDIT( m_eyeVerULeftTrack, TXT("Name of the left eye float track") );
	PROPERTY_EDIT( m_eyeVerDLeftTrack, TXT("Name of the left eye float track") );
	PROPERTY_EDIT( m_eyeVerURightTrack, TXT("Name of the left eye float track") );
	PROPERTY_EDIT( m_eyeVerDRightTrack, TXT("Name of the left eye float track") );
	PROPERTY_EDIT( m_eyeLeftPlacerBone, TXT("Placer for left eye") );
	PROPERTY_EDIT( m_eyeRightPlacerBone, TXT("Placer for right eye") );
	PROPERTY_EDIT( m_eyeHorMax, TXT("Max horizontal angle for pose") );
	PROPERTY_EDIT( m_eyeVerMin, TXT("Min vertical angle for pose") );
	PROPERTY_EDIT( m_eyeVerMax, TXT("Max vertical angle for pose") );
	PROPERTY_EDIT( m_eyeVerOffset, TXT("Vertical angle offset for pose") );
	PROPERTY_EDIT( m_eyesTrackClamp, TXT("") );
	PROPERTY_EDIT( m_dampTime, TXT("Damp time") );
	PROPERTY_EDIT( m_blinkAnimName, TXT("") );
	PROPERTY_EDIT( m_blinkTimeOffset, TXT("") );
	PROPERTY_EDIT( m_blinkSpeed, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorMimicHeadConstraint : public IBehaviorMimicConstraint
{
	DECLARE_ENGINE_CLASS( CBehaviorMimicHeadConstraint, IBehaviorMimicConstraint, 0 );

private:
	String						m_headTrack;

private:
	TInstanceVar< Float >		i_weight;
	TInstanceVar< Int32 >		i_headTrack;

private:
	CBehaviorGraphValueNode*	m_cachedControlVariableNode;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void RebuildSockets( CBehaviorGraphNode* parent ) override;
#endif

public:
	CBehaviorMimicHeadConstraint();

	virtual void CacheConnections( CBehaviorGraphNode* parent ) override;

	virtual void BuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void InitInstance( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* parent ) const override;

	virtual void Update( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const override;
	virtual void PreSample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &animPose, SBehaviorGraphOutput &mimicPose ) const override;
	virtual void PostSample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &animPose, SBehaviorGraphOutput &floatTrackPose, SBehaviorGraphOutput &mimicPose ) const override {}
	virtual void Reset( CBehaviorGraphInstance& instance ) const override;

	virtual void Activate( CBehaviorGraphInstance& instance ) const override;
	virtual void Deactivate( CBehaviorGraphInstance& instance ) const override;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const override;

protected:
	void InternalReset( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorMimicHeadConstraint );
	PARENT_CLASS( IBehaviorMimicConstraint );
	PROPERTY( m_cachedControlVariableNode );
	PROPERTY_EDIT( m_headTrack, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorMimiLipsyncCorrectionConstraint : public IBehaviorMimicConstraint
{
	DECLARE_ENGINE_CLASS( CBehaviorMimiLipsyncCorrectionConstraint, IBehaviorMimicConstraint, 0 );

private:
	Int32						m_controlTrack;

	Int32						m_trackBegin;
	Int32						m_trackEnd;

public:
	CBehaviorMimiLipsyncCorrectionConstraint();

	virtual void Update( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const override {}
	virtual void PreSample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &animPose, SBehaviorGraphOutput &mimicPose ) const override;
	virtual void PostSample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &animPose, SBehaviorGraphOutput &floatTrackPose, SBehaviorGraphOutput &mimicPose ) const override {}
};

BEGIN_CLASS_RTTI( CBehaviorMimiLipsyncCorrectionConstraint );
	PARENT_CLASS( IBehaviorMimicConstraint );
	PROPERTY_EDIT( m_controlTrack, TXT("") );
	PROPERTY_EDIT( m_trackBegin, TXT("") );
	PROPERTY_EDIT( m_trackEnd, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorMimicCloseEyesConstraint : public IBehaviorMimicConstraint
{
	DECLARE_ENGINE_CLASS( CBehaviorMimicCloseEyesConstraint, IBehaviorMimicConstraint, 0 );

private:
	Int32						m_eyeClosedTrack_Left;
	Int32						m_eyeClosedTrack_Right;

	TDynArray< String >			m_bonesToOverride_Left;
	TDynArray< String >			m_bonesToOverride_Right;

private:
	TInstanceVar< TDynArray< Int32 > >		i_bonesToOverride_Left;
	TInstanceVar< TDynArray< Int32 > >		i_bonesToOverride_Right;

public:
	CBehaviorMimicCloseEyesConstraint();

	virtual void BuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void InitInstance( CBehaviorGraphInstance& instance, const CBehaviorGraphNode* parent ) const override;

	virtual void Update( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const override {}
	virtual void PreSample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &animPose, SBehaviorGraphOutput &mimicPose ) const override {}
	virtual void PostSample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &animPose, SBehaviorGraphOutput &floatTrackPose, SBehaviorGraphOutput &mimicPose ) const override;

#ifndef NO_EDITOR
	virtual void OnCreatedInEditor() override;
#endif

private:
	void OverrideBones( const CMimicFace* face, SBehaviorGraphOutput& pose, Float controlValue, Int32 eyeClosedTrack, const TDynArray< Int32 >& bonesToOverride, SBehaviorGraphOutput &mimicPose ) const;
};

BEGIN_CLASS_RTTI( CBehaviorMimicCloseEyesConstraint );
	PARENT_CLASS( IBehaviorMimicConstraint );
	PROPERTY_EDIT( m_eyeClosedTrack_Left, TXT("Name of the left eye float track") );
	PROPERTY_EDIT( m_eyeClosedTrack_Right, TXT("Name of the left eye float track") );
	PROPERTY_EDIT( m_bonesToOverride_Left, TXT("Name of the left eye float track") );
	PROPERTY_EDIT( m_bonesToOverride_Right, TXT("Name of the left eye float track") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicsConverterNode : public CBehaviorGraphNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicsConverterNode, CBehaviorGraphNode, "Mimic.Converter", "Mimics Converter" );

public:
	static const Float						POSE_THRESHOLD;

protected:
	Uint32									m_poseNum;
	Float									m_weight;
	String									m_placerPrefix;
	TDynArray< IBehaviorMimicConstraint* >	m_mimicsConstraints;

	Int32									m_mimicLipsyncOffset;
	Int32									m_normalBlendTracksBegin;

protected:
	TInstanceVar< TDynArray< Int32 > >		i_placerBones;
	TInstanceVar< TDynArray< Int32 > >		i_mimicsBones;

protected:
	CBehaviorGraphNode*						m_cachedAnimInputNode;
	CBehaviorGraphNode*						m_cachedMimicBaseInputNode;

public:
	CBehaviorGraphMimicsConverterNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual void OnPropertyPostChange( IProperty* property );
	virtual String GetCaption() const { return String( TXT("Mimic converter") ); }
	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsOnReleaseInstanceManuallyOverridden() const override { return true; }

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();
	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

	Bool CanWork( CBehaviorGraphInstance& instance ) const;

protected:
	void FindPlacersChildBones( CBehaviorGraphInstance& instance ) const;
	void FillMimicData( SMimicPostProcessData* data, const SBehaviorGraphOutput &output ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicsConverterNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY( m_cachedAnimInputNode );
	PROPERTY( m_cachedMimicBaseInputNode );
	PROPERTY_EDIT( m_placerPrefix, TXT("") );
	PROPERTY_EDIT( m_normalBlendTracksBegin, TXT("") );
	PROPERTY_EDIT( m_mimicLipsyncOffset, TXT("") );
	PROPERTY_INLINED( m_mimicsConstraints, TXT("Mimics constraint") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicsBoneConverterNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicsBoneConverterNode, CBehaviorGraphBaseNode, "Mimic.Converter", "Bones converter" );

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String( TXT("Bones converter") ); }
#endif

public:
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicsBoneConverterNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicsLookAtMediator
{
public:
	const static CName&			VAR_ISENABLED;
	const static CName&			VAR_TARGET_A;
	const static CName&			VAR_TARGET_B;
	const static CName&			VAR_PROGRESS;
	const static CName&			VAR_WEIGHT;
	const static CName&			EVT_TARGET_CHANGED;

private:
	THandle< CBehaviorGraphInstance > m_instanceH;

public:
	CBehaviorGraphMimicsLookAtMediator();
	CBehaviorGraphMimicsLookAtMediator( CEntity* e );
	CBehaviorGraphMimicsLookAtMediator( CBehaviorGraphInstance& instance );

	Bool Init( CEntity* e );
	void Deinit();
	Bool IsValid() const;

	void SetEnabled( Bool f );
	Bool IsEnabled() const;

	void SetData( Float weight, const Vector& pointA, const Vector& pointB, Float progress );
	void GetData( Float& weight, Vector& pointA, Vector& pointB, Float& progress ) const;

	Bool NotifyTargetWasChanged();
};

//////////////////////////////////////////////////////////////////////////
