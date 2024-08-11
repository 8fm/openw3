/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphNode.h"
#include "behaviorGraphAnimationSwitchNode.h"
#include "behaviorGraphOutputNode.h"
#include "behaviorGraphParentInputNode.h"
#include "behaviorGraphStage.h"
#include "behaviorIncludes.h"

class CBehaviorGraphValueNode;

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphBaseMimicNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_ABSTRACT_CLASS( CBehaviorGraphBaseMimicNode, CBehaviorGraphBaseNode );

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }
#endif
	virtual void CacheConnections();

	virtual Bool IsMimic() const { return true; }
};

BEGIN_ABSTRACT_CLASS_RTTI( CBehaviorGraphBaseMimicNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EBehaviorMimicBlendType
{
	BMBT_Continues,
	BMBT_Max,
	BMBT_Min,
	BMBT_Add,
	BMBT_Sub,
	BMBT_Mul,
	BMBT_AbsMax,
};

BEGIN_ENUM_RTTI( EBehaviorMimicBlendType );
	ENUM_OPTION( BMBT_Continues );
	ENUM_OPTION( BMBT_Max );
	ENUM_OPTION( BMBT_Min );
	ENUM_OPTION( BMBT_Add );
	ENUM_OPTION( BMBT_Sub );
	ENUM_OPTION( BMBT_Mul );
	ENUM_OPTION( BMBT_AbsMax );
END_ENUM_RTTI()

class CBehaviorGraphMimicsBlendNode : public CBehaviorGraphNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicsBlendNode, CBehaviorGraphNode, "Mimic", "Blend" );

protected:
	EBehaviorMimicBlendType		m_type;

	Float						m_firstInputValue;
	Float						m_secondInputValue;

protected:
	TInstanceVar< Float >		i_controlValue;
	TInstanceVar< Float >		i_prevControlValue;

protected:

protected:
	CBehaviorGraphNode*			m_cachedFirstInputNode;
	CBehaviorGraphNode*			m_cachedSecondInputNode;
	CBehaviorGraphValueNode*	m_cachedControlVariableNode;

public:
	CBehaviorGraphMimicsBlendNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );
	virtual void OnRebuildSockets();

	virtual String GetCaption() const;
	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

	virtual Bool IsMimic() const { return true; }

protected:
	void ProcessActivations( CBehaviorGraphInstance& instance ) const;
	void UpdateControlValue( CBehaviorGraphInstance& instance ) const;
	Bool IsFirstInputActive( Float var ) const;
	Bool IsSecondInputActive( Float var ) const;
	void Blend( SBehaviorGraphOutput& output, const SBehaviorGraphOutput& poseA, const SBehaviorGraphOutput& poseB, const Float weight ) const;
	Float GetInitialControlValueFromType() const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicsBlendNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY( m_cachedFirstInputNode );
	PROPERTY( m_cachedSecondInputNode );
	PROPERTY( m_cachedControlVariableNode );
	PROPERTY_EDIT( m_type, TXT("Blend type") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicGainNode : public CBehaviorGraphBaseMimicNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicGainNode, CBehaviorGraphBaseMimicNode, "Mimic", "Gain" );

protected:
	Float							m_gain;
	Float							m_max;
	Float							m_min;

protected:
	CBehaviorGraphValueNode*		m_cachedGainValueNode;

public:
	CBehaviorGraphMimicGainNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();

	virtual String GetCaption() const { return String( TXT("Mimic gain") ); }
	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }
#endif

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

	virtual Bool IsMimic() const { return true; }
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicGainNode );
	PARENT_CLASS( CBehaviorGraphBaseMimicNode );
	PROPERTY_EDIT( m_gain, TXT("Gain") );
	PROPERTY_EDIT( m_min, TXT("Min") );
	PROPERTY_EDIT( m_max, TXT("Max") );
	PROPERTY( m_cachedGainValueNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EBehaviorMimicMathOp
{
	BMMO_Add,
	BMMO_Sub,
	BMMO_Mul,
	BMMO_Div,
	BMMO_Max,
	BMMO_Min,
};

BEGIN_ENUM_RTTI( EBehaviorMimicMathOp );
	ENUM_OPTION( BMMO_Add );
	ENUM_OPTION( BMMO_Sub );
	ENUM_OPTION( BMMO_Mul );
	ENUM_OPTION( BMMO_Div );
	ENUM_OPTION( BMMO_Max );
	ENUM_OPTION( BMMO_Min );
END_ENUM_RTTI()

class CBehaviorGraphMimicMathOpNode : public CBehaviorGraphBaseMimicNode
									, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicMathOpNode, CBehaviorGraphBaseMimicNode, "Mimic", "Math" );

protected:
	EBehaviorMimicMathOp		m_mathOp;
	String						m_trackName;
	Float						m_value;

protected:
	TInstanceVar< Float	>		i_value;
	TInstanceVar< Int32 >			i_trackIndex;

protected:
	CBehaviorGraphValueNode*	m_cachedValueNode;

public:
	CBehaviorGraphMimicMathOpNode();

	virtual CSkeleton* GetBonesSkeleton( CAnimatedComponent* component ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets();

	virtual String GetCaption() const { return String( TXT("Mimic math") ); }
	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }

#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

	virtual Bool IsMimic() const { return true; }

protected:
	void CacheTrack( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicMathOpNode );
	PARENT_CLASS( CBehaviorGraphBaseMimicNode );
	PROPERTY( m_cachedValueNode );
	PROPERTY_EDIT( m_mathOp, TXT("Math operation") );
	PROPERTY_EDIT( m_value , TXT("Value") );
	PROPERTY_CUSTOM_EDIT( m_trackName, TXT("Name of the float track"), TXT("BehaviorTrackSelection"));
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicFilterNode : public CBehaviorGraphBaseMimicNode
									, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicFilterNode, CBehaviorGraphBaseMimicNode, "Mimic", "Filter" );

protected:
	TDynArray< SBehaviorGraphTrackInfo >		m_tracks;

protected:
	TInstanceVar< TDynArray< Int32 > >			i_tracksIndex;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String( TXT("Filter") ); }
	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }
	//virtual void OnSpawned( const GraphBlockSpawnInfo& info );
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual CSkeleton* GetBonesSkeleton( CAnimatedComponent* component ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicFilterNode );
	PARENT_CLASS( CBehaviorGraphBaseMimicNode );
	PROPERTY_EDIT( m_tracks, TXT("Tracks") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

int compareSBehaviorGraphTrackInfo( const void* a, const void* b );

class CBehaviorGraphMimicFilterNodeInvert : public CBehaviorGraphMimicFilterNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicFilterNodeInvert, CBehaviorGraphMimicFilterNode, "Mimic", "FilterInvert" );

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual String GetCaption() const { return String( TXT("FilterInvert") ); }

#endif

	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicFilterNodeInvert );
	PARENT_CLASS( CBehaviorGraphMimicFilterNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicOutputNode : public CBehaviorGraphOutputNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicOutputNode, CBehaviorGraphOutputNode, "Mimic", "Output" )		

public:
	CBehaviorGraphMimicOutputNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();

	virtual String GetCaption() const { return TXT("Mimic output"); }
	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }
#endif

	virtual void CacheConnections();

	virtual Bool IsMimic() const { return true; }
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicOutputNode );
	PARENT_CLASS( CBehaviorGraphOutputNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicStageNode : public CBehaviorGraphStageNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicStageNode, CBehaviorGraphStageNode, "Mimic", "Stage" );		

public:
	CBehaviorGraphMimicStageNode();	

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnSpawned( const GraphBlockSpawnInfo& info );
	virtual void OnRebuildSockets();

	virtual String GetCaption() const;
	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }

#endif

	virtual void CacheConnections();

	virtual Bool IsMimic() const { return true; }
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicStageNode );
	PARENT_CLASS( CBehaviorGraphStageNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicParentInputNode : public CBehaviorGraphParentInputNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicParentInputNode, CBehaviorGraphParentInputNode, "Parent input", "Mimic" );

public:
	CBehaviorGraphMimicParentInputNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();

	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }
#endif

	virtual void CacheConnections();

	virtual Bool IsMimic() const { return true; }
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicParentInputNode );
	PARENT_CLASS( CBehaviorGraphParentInputNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicAnimationManualSwitchNode : public CBehaviorGraphAnimationManualSwitchNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicAnimationManualSwitchNode, CBehaviorGraphAnimationManualSwitchNode, "Mimic", "Switch" );

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }
#endif
	virtual Bool IsMimic() const { return true; }

protected:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual CGraphSocket* CreateInputSocket( const CName& name );
	virtual CGraphSocket* CreateOutputSocket( const CName& name );
#endif
	virtual CBehaviorGraphNode* CacheInputBlock( const String& name );

	virtual Bool GetPoseType() const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicAnimationManualSwitchNode );
	PARENT_CLASS( CBehaviorGraphAnimationManualSwitchNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicAnimationEnumSwitchNode : public CBehaviorGraphAnimationEnumSwitchNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicAnimationEnumSwitchNode, CBehaviorGraphAnimationEnumSwitchNode, "Mimic", "Switch enum" );

protected:
	Bool			m_useCurve;
	CCurve*			m_curve;

public:
	CBehaviorGraphMimicAnimationEnumSwitchNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }
	virtual void OnSpawned( const GraphBlockSpawnInfo& info );
#endif
	virtual Bool IsMimic() const { return true; }

protected:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual CGraphSocket* CreateInputSocket( const CName& name );
	virtual CGraphSocket* CreateOutputSocket( const CName& name );
#endif
	virtual Float GetBlendWeight( CBehaviorGraphInstance& instance ) const;
	virtual CBehaviorGraphNode* CacheInputBlock( const String& name );

	Bool GetPoseType() const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicAnimationEnumSwitchNode );
	PARENT_CLASS( CBehaviorGraphAnimationEnumSwitchNode );
	PROPERTY_EDIT( m_useCurve, TXT("") );
	PROPERTY_CUSTOM_EDIT( m_curve, TXT("Curve"), TXT("CurveSelection") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

#include "behaviorGraphRandomNode.h"

class CBehaviorGraphMimicRandomNode : public CBehaviorGraphRandomNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicRandomNode, CBehaviorGraphRandomNode, "Mimic", "Random" );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }

#endif

	virtual Bool IsMimic() const { return true; }

public:
	virtual CBehaviorGraphNode* CacheInputBlock( const String& socketName );

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void CreateOutputSocket();
	virtual void CreateInputSocket( const CName& socketName );

#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicRandomNode );
	PARENT_CLASS( CBehaviorGraphRandomNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicRandomBlendNode : public CBehaviorGraphMimicRandomNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicRandomBlendNode, CBehaviorGraphMimicRandomNode, "Mimic", "Random Blend" );

protected:
	Float									m_blendDuration;

protected:
	TInstanceVar< Float >					i_alpha;
	TInstanceVar< Float >					i_delta;
	TInstanceVar< CBehaviorGraphNode* >		i_previnput;

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual String GetCaption() const { return TXT("Random Blend"); }

#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void SelectRandomInput( CBehaviorGraphInstance& instance ) const;

public:
	virtual void OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicRandomBlendNode );
	PARENT_CLASS( CBehaviorGraphMimicRandomNode );
	PROPERTY_EDIT( m_blendDuration , TXT("BlendDuration") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicEyesCorrectionNode : public CBehaviorGraphBaseMimicNode, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicEyesCorrectionNode, CBehaviorGraphBaseMimicNode, "Mimic.Constraints", "Eyes correction" );

protected:
	String							m_trackEyeLeft_Left;
	String							m_trackEyeLeft_Right;
	String							m_trackEyeRight_Left;
	String							m_trackEyeRight_Right;

protected:
	TInstanceVar< Int32 >			i_trackEyeLeft_Left;
	TInstanceVar< Int32 >			i_trackEyeLeft_Right;
	TInstanceVar< Int32 >			i_trackEyeRight_Left;
	TInstanceVar< Int32 >			i_trackEyeRight_Right;

public:
	CBehaviorGraphMimicEyesCorrectionNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String( TXT("Eyes correction") ); }
	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual CSkeleton* GetBonesSkeleton( CAnimatedComponent* component ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicEyesCorrectionNode );
	PARENT_CLASS( CBehaviorGraphBaseMimicNode );
	PROPERTY_CUSTOM_EDIT( m_trackEyeLeft_Left, TXT("Name of the float track"), TXT("BehaviorTrackSelection"));
	PROPERTY_CUSTOM_EDIT( m_trackEyeLeft_Right, TXT("Name of the float track"), TXT("BehaviorTrackSelection"));
	PROPERTY_CUSTOM_EDIT( m_trackEyeRight_Left, TXT("Name of the float track"), TXT("BehaviorTrackSelection"));
	PROPERTY_CUSTOM_EDIT( m_trackEyeRight_Right, TXT("Name of the float track"), TXT("BehaviorTrackSelection"));
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicBlinkControllerNode : public CBehaviorGraphBaseMimicNode, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicBlinkControllerNode, CBehaviorGraphBaseMimicNode, "Mimic.Constraints", "Blink controller" );

protected:
	String					m_trackEyeLeft_Down;
	String					m_trackEyeRight_Down;

	CName					m_variableNameLeft;
	CName					m_variableNameRight;

protected:
	TInstanceVar< Int32 >	i_trackEyeLeft_Down;
	TInstanceVar< Int32 >	i_trackEyeRight_Down;

public:
	CBehaviorGraphMimicBlinkControllerNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String( TXT("Blink controller") ); }
	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }
	virtual Bool IsManualCreationAllowed() const override { return false; }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual CSkeleton* GetBonesSkeleton( CAnimatedComponent* component ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

protected:
	virtual void InternalReset( CBehaviorGraphInstance& instance ) const;
	Bool IsValid( const CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicBlinkControllerNode );
	PARENT_CLASS( CBehaviorGraphBaseMimicNode );
	PROPERTY_CUSTOM_EDIT( m_trackEyeLeft_Down, TXT("Name of the float track"), TXT("BehaviorTrackSelection"));
	PROPERTY_CUSTOM_EDIT( m_trackEyeRight_Down, TXT("Name of the float track"), TXT("BehaviorTrackSelection"));
	PROPERTY_CUSTOM_EDIT( m_variableNameLeft, TXT("Variable name"), TXT("BehaviorInternalVariableSelection") );
	PROPERTY_CUSTOM_EDIT( m_variableNameRight, TXT("Variable name"), TXT("BehaviorInternalVariableSelection") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicBlinkControllerNode_Watcher : public CBehaviorGraphMimicBlinkControllerNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicBlinkControllerNode_Watcher, CBehaviorGraphMimicBlinkControllerNode, "Mimic.Constraints", "Blink controller - watcher" );

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String( TXT("Blink controller - watcher") ); }
#endif

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicBlinkControllerNode_Watcher );
	PARENT_CLASS( CBehaviorGraphMimicBlinkControllerNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicBlinkControllerNode_Setter : public CBehaviorGraphMimicBlinkControllerNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicBlinkControllerNode_Setter, CBehaviorGraphMimicBlinkControllerNode, "Mimic.Constraints", "Blink controller - setter" );

public:
	enum EState
	{
		S_None,
		S_BlinkInProgress,
		S_BlinkBlocked,
		S_BlinkBlockedToNone,
	};

protected:
	Float					m_blinkValueThr;
	Float					m_blinkCooldown;

protected:
	TInstanceVar< Int32 >	i_state;
	TInstanceVar< Float >	i_timer;
	TInstanceVar< Bool >	i_blocked;

public:
	CBehaviorGraphMimicBlinkControllerNode_Setter();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String( TXT("Blink controller - setter") ); }
	Bool GetBlinkStateDesc( CBehaviorGraphInstance& instance, String& desc ) const;
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

private:
	virtual void InternalReset( CBehaviorGraphInstance& instance ) const override;
	void SetTimer( CBehaviorGraphInstance& instance, Float timeDuration ) const;
	void SetState( CBehaviorGraphInstance& instance, EState state ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicBlinkControllerNode_Setter );
	PARENT_CLASS( CBehaviorGraphMimicBlinkControllerNode );
	PROPERTY_EDIT( m_blinkValueThr, String::EMPTY );
	PROPERTY_EDIT( m_blinkCooldown, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicBlinkControllerNode_Blend : public CBehaviorGraphNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicBlinkControllerNode_Blend, CBehaviorGraphNode, "Mimic.Constraints", "Blink controller - blend" );

public:
	enum EState
	{
		S_None,
		S_BlinkInProgress_Idle,
		S_BlinkInProgress_Rest,
		S_BlinkBlocked_Idle,
		S_BlinkBlocked_Rest,
		S_BlinkBlockedToNone_Idle,
		S_BlinkBlockedToNone_Rest,
	};

	enum EBlinkState
	{
		BS_None,
		BS_BlinkDown,
		BS_BlinkUp,
	};

protected:
	String					m_trackEyeLeft_Down;
	String					m_trackEyeRight_Down;

	Float					m_blinkValueThr;
	Float					m_blinkCooldown;

protected:
	TInstanceVar< Int32 >	i_trackEyeLeft_Down;
	TInstanceVar< Int32 >	i_trackEyeRight_Down;
	TInstanceVar< Int32 >	i_state;
	TInstanceVar< Int32 >	i_state_Idle;
	TInstanceVar< Int32 >	i_state_Rest;
	TInstanceVar< Float >	i_timer;
	TInstanceVar< Bool >	i_blocked;
	TInstanceVar< Float >	i_trackEyeLeft_Idle_CurrValue;
	TInstanceVar< Float >	i_trackEyeRight_Idle_CurrValue;
	TInstanceVar< Float >	i_trackEyeLeft_Idle_PrevValue;
	TInstanceVar< Float >	i_trackEyeRight_Idle_PrevValue;
	TInstanceVar< Float >	i_trackEyeLeft_Rest_CurrValue;
	TInstanceVar< Float >	i_trackEyeRight_Rest_CurrValue;
	TInstanceVar< Float >	i_trackEyeLeft_Rest_PrevValue;
	TInstanceVar< Float >	i_trackEyeRight_Rest_PrevValue;

protected:
	CBehaviorGraphNode*		m_cachedInputIdle;
	CBehaviorGraphNode*		m_cachedInputRest;

public:
	CBehaviorGraphMimicBlinkControllerNode_Blend();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String( TXT("Blink controller - blend") ); }
	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }

	virtual void OnRebuildSockets();

	Bool GetBlinkStateDesc( CBehaviorGraphInstance& instance, String& desc ) const;
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

	virtual Bool IsMimic() const { return true; }

private:
	void InternalReset( CBehaviorGraphInstance& instance ) const;
	void SetTimer( CBehaviorGraphInstance& instance, Float timeDuration ) const;
	void SetState( CBehaviorGraphInstance& instance, EState state ) const;
	void ProcessBlinkState( Int32& state, Bool isClosing, Bool isClosed ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicBlinkControllerNode_Blend );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY_CUSTOM_EDIT( m_trackEyeLeft_Down, TXT("Name of the float track"), TXT("BehaviorTrackSelection"));
	PROPERTY_CUSTOM_EDIT( m_trackEyeRight_Down, TXT("Name of the float track"), TXT("BehaviorTrackSelection"));
	PROPERTY_EDIT( m_blinkValueThr, String::EMPTY );
	PROPERTY_EDIT( m_blinkCooldown, String::EMPTY );
	PROPERTY( m_cachedInputIdle );
	PROPERTY( m_cachedInputRest );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphLipsyncControlValueCorrectionNode : public CBehaviorGraphBaseMimicNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphLipsyncControlValueCorrectionNode, CBehaviorGraphBaseMimicNode, "Mimic.Constraints", "Lipsync control value corr" );

protected:
	Int32	m_lipsyncControlTrack;
	Float	m_smoothTime;

	CName	m_startCorrEventName;

private:
	TInstanceVar< Float >				i_timer;
	//TInstanceVar< Float >				i_lastTimeDelta;
	//TInstanceVar< TDynArray< Float > >	i_values;
	TInstanceVar< Uint32 >				i_startCorrEventId;

public:
	CBehaviorGraphLipsyncControlValueCorrectionNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String( TXT("Lipsync control value corr") ); }
	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const override;

private:
	void InternalReset( CBehaviorGraphInstance& instance ) const;
	void StartTimer( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphLipsyncControlValueCorrectionNode );
	PARENT_CLASS( CBehaviorGraphBaseMimicNode );
	PROPERTY_EDIT( m_lipsyncControlTrack, String::EMPTY );
	PROPERTY_EDIT( m_smoothTime, String::EMPTY );
	PROPERTY_EDIT( m_startCorrEventName, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
