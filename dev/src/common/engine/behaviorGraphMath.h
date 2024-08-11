/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

#include "behaviorGraphValueNode.h"

enum EBehaviorMathOp
{
	BMO_Add			=	0,
	BMO_Subtract	=	1,
	BMO_Multiply	=	2,
	BMO_Divide		=	3,
	BMO_SafeDivide	=	4,
	BMO_ATan		=	5,
	BMO_AngleDiff	=	6,
	BMO_Length		=	7,
	BMO_Abs			=	8
};

BEGIN_ENUM_RTTI( EBehaviorMathOp );
	ENUM_OPTION( BMO_Add )
	ENUM_OPTION( BMO_Subtract )
	ENUM_OPTION( BMO_Multiply )
	ENUM_OPTION( BMO_Divide )
	ENUM_OPTION( BMO_SafeDivide )
	ENUM_OPTION( BMO_ATan )
	ENUM_OPTION( BMO_AngleDiff )
	ENUM_OPTION( BMO_Length )
	ENUM_OPTION( BMO_Abs )
END_ENUM_RTTI();

class CBehaviorGraphMathNode : public CBehaviorGraphValueNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMathNode, CBehaviorGraphValueNode, "Float", "Math op" );			

protected:
	EBehaviorMathOp		m_operation;

protected:
	CBehaviorGraphValueNode*		m_cachedFirstInputNode;
	CBehaviorGraphValueNode*		m_cachedSecondInputNode;

public:
	CBehaviorGraphMathNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets();
	virtual String GetCaption() const;

#endif

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated(CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMathNode );
	PARENT_CLASS( CBehaviorGraphValueNode );
	PROPERTY_EDIT( m_operation, TXT("Operation type") );
	PROPERTY( m_cachedFirstInputNode );
	PROPERTY( m_cachedSecondInputNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphFloatValueNode : public CBehaviorGraphValueNode 
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphFloatValueNode, CBehaviorGraphValueNode, "Float", "Float value" );

protected:
	Float	m_value;

public:
	CBehaviorGraphFloatValueNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
	virtual void OnRebuildSockets();
#endif

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const {}
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}	

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphFloatValueNode );
	PARENT_CLASS( CBehaviorGraphValueNode );
	PROPERTY_EDIT( m_value, TXT("Value") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphEditorValueNode : public CBehaviorGraphValueNode 
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphEditorValueNode, CBehaviorGraphValueNode, "Float", "Editor" );

protected:
	TInstanceVar< Float >	i_value;

public:
	CBehaviorGraphEditorValueNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnOpenInEditor( CBehaviorGraphInstance& instance ) const;
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const {}
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}	

public:
	virtual String GetCaption() const { return TXT("Editor"); }
	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphEditorValueNode );
	PARENT_CLASS( CBehaviorGraphValueNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphRandomValueNode : public CBehaviorGraphValueNode 
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphRandomValueNode, CBehaviorGraphValueNode, "Float", "Random value" );

protected:
	Float	m_value;
	Bool	m_rand;
	Float	m_cooldown;
	Float	m_min;
	Float	m_max;
	Bool	m_randDefaultValue;

protected:
	TInstanceVar< Float > i_randValue;
	TInstanceVar< Float > i_timer;

public:
	CBehaviorGraphRandomValueNode();

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual void OnPropertyPostChange( IProperty* property );
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}
	
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

public:
	virtual String GetCaption() const;
	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphRandomValueNode );
	PARENT_CLASS( CBehaviorGraphValueNode );
	PROPERTY_EDIT( m_value, TXT("Default value") );
	PROPERTY_EDIT( m_randDefaultValue, TXT("Rand default value on activate") );
	PROPERTY_EDIT( m_rand, TXT("Use random value or default value") );
	PROPERTY_EDIT( m_cooldown, TXT("Cooldown") );
	PROPERTY_EDIT( m_min, TXT("Min") );
	PROPERTY_EDIT( m_max, TXT("Max") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphEventWatchdogNode	: public CBehaviorGraphValueNode
										, public IBehaviorGraphProperty
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphEventWatchdogNode, CBehaviorGraphValueNode, "Float", "Watchdog" );

protected:
	CName					m_eventName;
	Float					m_trueValue;
	Float					m_falseValue;
	Float					m_maxTime;
	Float					m_timeOut;

protected:
	TInstanceVar< Uint32 >	i_event;
	TInstanceVar< Bool >	i_value;
	TInstanceVar< Float >	i_timer;
	TInstanceVar< Bool >	i_eventOccured;
	TInstanceVar< Float >	i_timeoutTimer;

protected:
	CBehaviorGraphNode*		m_cachedInputNode;

public:
	CBehaviorGraphEventWatchdogNode();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnReset( CBehaviorGraphInstance& instance ) const;
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

	//! IBehaviorGraphProperty implementation
	virtual CBehaviorGraph* GetParentGraph();

	virtual void CacheConnections();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
	virtual void OnRebuildSockets();
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphEventWatchdogNode );
	PARENT_CLASS( CBehaviorGraphValueNode );
	PROPERTY_CUSTOM_EDIT( m_eventName, TXT("Event to trigger transition"), TXT("BehaviorEventEdition") );
	PROPERTY_EDIT( m_trueValue, TXT("") );
	PROPERTY_EDIT( m_falseValue, TXT("") );
	PROPERTY_EDIT( m_maxTime, TXT("") );
	PROPERTY_EDIT( m_timeOut, TXT("Time out") );
	PROPERTY( m_cachedInputNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphOneMinusNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphOneMinusNode, CBehaviorGraphValueBaseNode, "Float", "One minus" );

public:
	virtual Float GetValue( CBehaviorGraphInstance& instance ) const
	{
		return m_cachedInputNode ? 1.f - m_cachedInputNode->GetValue( instance ) : 0.f;
	}

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT("1 - x"); }
#endif

};

BEGIN_CLASS_RTTI( CBehaviorGraphOneMinusNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphValueClampNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphValueClampNode, CBehaviorGraphValueBaseNode, "Float", "Clamp" );
	
	Float		m_min;
	Float		m_max;

public:
	CBehaviorGraphValueClampNode() : m_min( 0.f ), m_max( 1.f ) {}

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const
	{
		return m_cachedInputNode ? Clamp( m_cachedInputNode->GetValue( instance ), m_min, m_max ) : m_min;
	}

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT("Clamp"); }
#endif

};

BEGIN_CLASS_RTTI( CBehaviorGraphValueClampNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY_EDIT( m_min, String::EMPTY );
	PROPERTY_EDIT( m_max, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EBehaviorValueModifierType
{
	BVM_OneMinus,
	BVM_AdditiveMap,
	BVM_Negative,
	BVM_11To01,
	BVM_11To02,
	BVM_01To11,
	BVM_01To02,
	BVM_11To180180,
	BVM_180180To11,
	BVM_RadToDeg,
	BVM_DegToRad,
	BVM_RadTo11,
	BVM_360To180180,
};

BEGIN_ENUM_RTTI( EBehaviorValueModifierType );
	ENUM_OPTION( BVM_OneMinus )
	ENUM_OPTION( BVM_AdditiveMap )
	ENUM_OPTION( BVM_Negative )
	ENUM_OPTION( BVM_11To01 )
	ENUM_OPTION( BVM_11To02 )
	ENUM_OPTION( BVM_01To11 )
	ENUM_OPTION( BVM_01To02 )
	ENUM_OPTION( BVM_11To180180 )
	ENUM_OPTION( BVM_180180To11 )
	ENUM_OPTION( BVM_RadToDeg )
	ENUM_OPTION( BVM_DegToRad )
	ENUM_OPTION( BVM_RadTo11 )
	ENUM_OPTION( BVM_360To180180 )
	END_ENUM_RTTI();

class CBehaviorGraphValueModifierNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphValueModifierNode, CBehaviorGraphValueBaseNode, "Float", "Value modifier" );

	EBehaviorValueModifierType	m_type;

public:
	CBehaviorGraphValueModifierNode();

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphValueModifierNode );
PARENT_CLASS( CBehaviorGraphValueBaseNode );
PROPERTY_EDIT( m_type, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphSelectionValueNode : public CBehaviorGraphValueNode 
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphSelectionValueNode, CBehaviorGraphValueNode, "Float", "Selection" );

protected:
	Float							m_threshold;

protected:
	CBehaviorGraphValueNode*		m_cachedSelNode;
	CBehaviorGraphValueNode*		m_cachedOneNode;
	CBehaviorGraphValueNode*		m_cachedTwoNode;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
	virtual void OnRebuildSockets();
#endif

	CBehaviorGraphSelectionValueNode();

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void CacheConnections();

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphSelectionValueNode );
	PARENT_CLASS( CBehaviorGraphValueNode );
	PROPERTY_EDIT( m_threshold, TXT("") );
	PROPERTY( m_cachedSelNode );
	PROPERTY( m_cachedOneNode );
	PROPERTY( m_cachedTwoNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphValueInterpolationNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphValueInterpolationNode, CBehaviorGraphValueBaseNode, "Float", "Interpolation" );

protected:
	Float	m_x1;
	Float	m_y1;
	Float	m_x2;
	Float	m_y2;

public:
	CBehaviorGraphValueInterpolationNode();

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );
	virtual String GetCaption() const { return TXT("Interpolation"); }
#endif

};

BEGIN_CLASS_RTTI( CBehaviorGraphValueInterpolationNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY_EDIT( m_x1, TXT("") );
	PROPERTY_EDIT( m_y1, TXT("") );
	PROPERTY_EDIT( m_x2, TXT("") );
	PROPERTY_EDIT( m_y2, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EBehaviorValueLatchType
{
	BVLT_Activation,
	BVLT_Max,
	BVLT_Min,
};

BEGIN_ENUM_RTTI( EBehaviorValueLatchType );
	ENUM_OPTION( BVLT_Activation )
	ENUM_OPTION( BVLT_Max )
	ENUM_OPTION( BVLT_Min )
END_ENUM_RTTI();

class CBehaviorGraphLatchValueNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphLatchValueNode, CBehaviorGraphValueBaseNode, "Float", "Latch" );

protected:
	EBehaviorValueLatchType	m_type;

public:
	CBehaviorGraphLatchValueNode();

	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
#endif

};

BEGIN_CLASS_RTTI( CBehaviorGraphLatchValueNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY_EDIT( m_type, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphLatchVectorValueNode : public CBehaviorGraphVectorValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphLatchVectorValueNode, CBehaviorGraphVectorValueBaseNode, "Vector", "Latch" );

public:
	CBehaviorGraphLatchVectorValueNode();

	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT("Latch"); }
#endif

};

BEGIN_CLASS_RTTI( CBehaviorGraphLatchVectorValueNode );
	PARENT_CLASS( CBehaviorGraphVectorValueBaseNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphLatchForSomeTimeValueNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphLatchForSomeTimeValueNode, CBehaviorGraphValueBaseNode, "Float", "Latch for some time" );

protected:
	Float m_minTime;
	Float m_maxTime;

protected:
	TInstanceVar< Float > i_timeLeft;

public:
	CBehaviorGraphLatchForSomeTimeValueNode();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const;
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );
	virtual String GetCaption() const { return TXT("Latch (time)"); }
#endif

protected:
	void LatchValueAndResetTime( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphLatchForSomeTimeValueNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY_EDIT( m_minTime, TXT("") );
	PROPERTY_EDIT( m_maxTime, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EBehaviorValueTimerType
{
	BVTT_Activation,
	BVTT_ConditionTimer,
	BVTT_ConditionReset,
};

BEGIN_ENUM_RTTI( EBehaviorValueTimerType );
	ENUM_OPTION( BVTT_Activation )
	ENUM_OPTION( BVTT_ConditionTimer )
	ENUM_OPTION( BVTT_ConditionReset )
END_ENUM_RTTI();

class CBehaviorGraphTimerValueNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphTimerValueNode, CBehaviorGraphValueBaseNode, "Float", "Timer" );

protected:
	EBehaviorValueTimerType		m_type;
	Float						m_maxValue;
	Float						m_timeScale;
	Float						m_threshold;

protected:
	CBehaviorGraphValueNode*	m_cachedFirstInputNode;

public:
	CBehaviorGraphTimerValueNode();

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const;

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void CacheConnections();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );
	virtual String GetCaption() const { return TXT("Timer"); }
	virtual void OnRebuildSockets();
#endif

};

BEGIN_CLASS_RTTI( CBehaviorGraphTimerValueNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY_EDIT( m_type, TXT("") );
	PROPERTY_EDIT( m_maxValue, TXT("") );
	PROPERTY_EDIT( m_timeScale, TXT("") );
	PROPERTY_EDIT( m_threshold, TXT("") );
	PROPERTY( m_cachedFirstInputNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphWrapNode : public CBehaviorGraphValueNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphWrapNode, CBehaviorGraphValueNode, "Float", "Wrap" );

protected:
	Float	m_minValue;
	Float	m_maxValue;

	CBehaviorGraphValueNode*		m_cachedFirstInputNode;

public:
	CBehaviorGraphWrapNode();

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated(CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return TXT("Wrap"); }
#endif

};

BEGIN_CLASS_RTTI( CBehaviorGraphWrapNode );
PARENT_CLASS( CBehaviorGraphValueNode );
	PROPERTY_EDIT( m_minValue, TXT("Minimum value") );
	PROPERTY_EDIT( m_maxValue, TXT("Maximum value") );	
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EBehaviorWaveValueType
{
	BWVT_Sin,
	BWVT_Step01,
	BWVT_Step11,
	BWVT_Triangle01,
};

BEGIN_ENUM_RTTI( EBehaviorWaveValueType );
	ENUM_OPTION( BWVT_Sin )
	ENUM_OPTION( BWVT_Step01 )
	ENUM_OPTION( BWVT_Step11 )
	ENUM_OPTION( BWVT_Triangle01 )
END_ENUM_RTTI();

class CBehaviorGraphWaveValueNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphWaveValueNode, CBehaviorGraphValueBaseNode, "Float", "Wave" );

protected:
	EBehaviorWaveValueType	m_type;

	Float					m_freq;
	Float					m_amp;

protected:
	TInstanceVar< Float >	i_var;

public:
	CBehaviorGraphWaveValueNode();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
#endif

protected:
	void ResetAllVariables( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphWaveValueNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY_EDIT( m_type, String::EMPTY );
	PROPERTY_EDIT( m_freq, String::EMPTY );
	PROPERTY_EDIT( m_amp, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMapWorldSpaceDirectionToModelSpaceRangeNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMapWorldSpaceDirectionToModelSpaceRangeNode, CBehaviorGraphValueBaseNode, "Float.Map", "WS 360 direction to MS range" );

protected:
	Float					m_minOutValue;
	Float					m_maxOutValue;
	Bool					m_leftToRight; // left to right means min value is left, max is right

public:
	CBehaviorGraphMapWorldSpaceDirectionToModelSpaceRangeNode();

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphMapWorldSpaceDirectionToModelSpaceRangeNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY_EDIT( m_minOutValue, String::EMPTY );
	PROPERTY_EDIT( m_maxOutValue, String::EMPTY );
	PROPERTY_EDIT( m_leftToRight, TXT("Left to right means min value is left, max is right") )
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMapRangeNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMapRangeNode, CBehaviorGraphValueBaseNode, "Float.Map", "Simple map (without clamp)" );

protected:
	Float					m_minInValue;
	Float					m_maxInValue;
	Float					m_minOutValue;
	Float					m_maxOutValue;

	Float					m_base;
	Float					m_bias;

public:
	CBehaviorGraphMapRangeNode();

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
	virtual void OnPropertyPostChange( IProperty* property );
#endif

private:
	void CalculateBaseAndBias();
};

BEGIN_CLASS_RTTI( CBehaviorGraphMapRangeNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY_EDIT( m_minInValue, String::EMPTY );
	PROPERTY_EDIT( m_maxInValue, String::EMPTY );
	PROPERTY_EDIT( m_minOutValue, String::EMPTY );
	PROPERTY_EDIT( m_maxOutValue, String::EMPTY );
	PROPERTY( m_base );
	PROPERTY( m_bias );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SBehaviorGraphMapToDiscreteMapper
{
	DECLARE_RTTI_STRUCT( SBehaviorGraphMapToDiscreteMapper );

	Vector2 m_inRange;
	Float m_outValue;

	SBehaviorGraphMapToDiscreteMapper()
	: m_inRange( 0.0f, 1.0f )
	, m_outValue( 0.0f )
	{
	}
};

BEGIN_CLASS_RTTI( SBehaviorGraphMapToDiscreteMapper );
	PROPERTY_EDIT_NAME( m_inRange.X, TXT("Min input value"), String::EMPTY );
	PROPERTY_EDIT_NAME( m_inRange.Y, TXT("Max input value"), String::EMPTY );
	PROPERTY_EDIT( m_outValue, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMapToDiscreteNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMapToDiscreteNode, CBehaviorGraphValueBaseNode, "Float.Map", "Map ranges to discrete values" );

protected:
	TDynArray<SBehaviorGraphMapToDiscreteMapper> m_ranges;

public:
	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphMapToDiscreteNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY_EDIT( m_ranges, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphSnapToZeroNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphSnapToZeroNode, CBehaviorGraphValueBaseNode, "Float", "Snap to zero" );

protected:
	Float	m_epsilon;

public:
	CBehaviorGraphSnapToZeroNode();

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT("Snap to zero"); }
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphSnapToZeroNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY_EDIT( m_epsilon, String::EMPTY );
END_CLASS_RTTI();
