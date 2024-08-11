/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphValueNode.h"
#include "../engine/springDampers.h"

class CBehaviorGraphValueNode;

class CBehaviorGraphDampValueNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphDampValueNode, CBehaviorGraphValueBaseNode, "Float.Damps", "Damp value" );		

protected:
	Float						m_increaseSpeed;
	Float						m_decreaseSpeed;
	Bool						m_absolute;
	Bool						m_startFromDefault;
	Float						m_defaultValue;

protected:
	CBehaviorGraphValueNode*	m_cachedDefaultValNode;
	CBehaviorGraphValueNode*	m_cachedIncSpeedNode;
	CBehaviorGraphValueNode*	m_cachedDecSpeedNode;

public:
	CBehaviorGraphDampValueNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return String( TXT("Damp value") ); }
#endif

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();
};

BEGIN_CLASS_RTTI( CBehaviorGraphDampValueNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY_EDIT( m_increaseSpeed, TXT("Speed of variable change in positive direction") );
	PROPERTY_EDIT( m_decreaseSpeed, TXT("Speed of variable change in negative direction") );
	PROPERTY_EDIT( m_absolute, TXT("True if we consider Abs(value) of variable change when choosing between increase or decrease speed") ); 
	PROPERTY_EDIT( m_startFromDefault, TXT("") );
	PROPERTY_EDIT( m_defaultValue, TXT("") );
	PROPERTY( m_cachedDefaultValNode );
	PROPERTY( m_cachedIncSpeedNode );
	PROPERTY( m_cachedDecSpeedNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////////

class CBehaviorGraphDampAngularValueNode : public CBehaviorGraphValueBaseNode
{
    DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphDampAngularValueNode, CBehaviorGraphValueBaseNode, "Float.Damps", "Damp angular value" );	

protected:
    Float	m_speed;
    Bool    m_isDegree;

public:
    CBehaviorGraphDampAngularValueNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
    virtual String GetCaption() const { return String( TXT("Damp angular value") ); }
#endif

public:
    virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
    virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

};

BEGIN_CLASS_RTTI( CBehaviorGraphDampAngularValueNode );
PARENT_CLASS( CBehaviorGraphValueBaseNode );
PROPERTY_EDIT( m_speed, TXT("Speed of variable change") );
PROPERTY_EDIT( m_isDegree, TXT("Is in Degrees (-180 : 180) ? If no is in -1 : 1") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////////

class CBehaviorGraphDampAngularValueNodeDiff : public CBehaviorGraphValueBaseNode
{
    DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphDampAngularValueNodeDiff, CBehaviorGraphValueBaseNode, "Float.Damps", "Damp angular value Diff" );	

protected:
    Float	m_speed;
    Bool    m_isDegree;

public:
    CBehaviorGraphDampAngularValueNodeDiff();

#ifndef NO_EDITOR_GRAPH_SUPPORT
    virtual String GetCaption() const { return String( TXT("Damp angular value") ); }
#endif

public:
    virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
    virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

};

BEGIN_CLASS_RTTI( CBehaviorGraphDampAngularValueNodeDiff );
PARENT_CLASS( CBehaviorGraphValueBaseNode );
PROPERTY_EDIT( m_speed, TXT("Speed of variable change") );
PROPERTY_EDIT( m_isDegree, TXT("Is in Degrees (-180 : 180) ? If no is in -1 : 1") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphSpringDampValueNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphSpringDampValueNode, CBehaviorGraphValueBaseNode, "Float.Damps", "Damp spring" );

protected:
	Float						m_factor;
    Float						m_scale;

	Bool						m_forceInputValueOnActivate;

protected:
	TInstanceVar< Float >		i_vel;

public:
	CBehaviorGraphSpringDampValueNode();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String( TXT("Spring Damp") ); }
#endif

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

protected:
	void InternalReset( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphSpringDampValueNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY_EDIT( m_factor, TXT( "Spring factor" ) );
	PROPERTY_EDIT( m_scale, TXT( "" ) );
	PROPERTY_EDIT( m_forceInputValueOnActivate, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphOptSpringDampValueNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphOptSpringDampValueNode, CBehaviorGraphValueBaseNode, "Float.Damps", "Damp spring (opt)" );

protected:
	Float						m_smoothTime;
	Float						m_scale;
	Float						m_maxSpeed;
	Float						m_maxDiff;
	Float						m_defaultValue;

	Bool						m_forceInputValueOnActivate;
	Bool						m_forceDefaultValueOnActivate;

protected:
	TInstanceVar< Float >		i_vel;

protected:
	CBehaviorGraphValueNode*	m_cachedSmoothTimeNode;

public:
	CBehaviorGraphOptSpringDampValueNode();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String( TXT("Spring Damp (opt)") ); }
	virtual void OnRebuildSockets();
#endif

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void CacheConnections();

protected:
	void InternalReset( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphOptSpringDampValueNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY( m_cachedSmoothTimeNode );
	PROPERTY_EDIT( m_smoothTime, String::EMPTY );
	PROPERTY_EDIT( m_scale, String::EMPTY );
	PROPERTY_EDIT( m_maxSpeed, TXT( "0 means do not use it." ) );
	PROPERTY_EDIT( m_maxDiff, TXT( "0 means do not use it." ) );
	PROPERTY_EDIT( m_defaultValue, String::EMPTY );
	PROPERTY_EDIT( m_forceInputValueOnActivate, String::EMPTY );
	PROPERTY_EDIT( m_forceDefaultValueOnActivate, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphSpringAngularDampValueNode : public CBehaviorGraphSpringDampValueNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphSpringAngularDampValueNode, CBehaviorGraphSpringDampValueNode, "Float.Damps", "Damp angular spring" );
protected:
	Bool                        m_isDegree;

public:
	CBehaviorGraphSpringAngularDampValueNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String( TXT("Angular Spring Damp") ); }
#endif

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphSpringAngularDampValueNode );
	PARENT_CLASS( CBehaviorGraphSpringDampValueNode );
	PROPERTY_EDIT( m_isDegree, TXT("Is in Degrees (-180 : 180) ? If no is in -1 : 1") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EBehaviorCustomDampType
{
	BGCDT_DirectionalAcc,
	BGCDT_FilterLowPass,
};

BEGIN_ENUM_RTTI( EBehaviorCustomDampType );
	ENUM_OPTION( BGCDT_DirectionalAcc )
	ENUM_OPTION( BGCDT_FilterLowPass )
END_ENUM_RTTI();

class CBehaviorGraphCustomDampValueNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphCustomDampValueNode, CBehaviorGraphValueBaseNode, "Float.Damps", "Custom damp" );

protected:
	EBehaviorCustomDampType	m_type;

	// BGCDT_DirectionalAcc
	Float					m_directionalAcc_MaxAccDiffFromZero;
	Float					m_directionalAcc_MaxAccDiffToZero;

	// BGCDT_FilterLowPass
	Float					m_filterLowPass_RC;

protected:
	TInstanceVar< Float >	i_varA;
	TInstanceVar< Float >	i_varB;

public:
	CBehaviorGraphCustomDampValueNode();

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

	Float CalcDirectionalAcc( CBehaviorGraphInstance& instance, Float inputVal, Float timeDelta ) const;
	Float CalcFilterLowPass( CBehaviorGraphInstance& instance, Float inputVal, Float timeDelta ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphCustomDampValueNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY_EDIT( m_type, String::EMPTY );
	PROPERTY_EDIT( m_directionalAcc_MaxAccDiffFromZero, String::EMPTY );
	PROPERTY_EDIT( m_directionalAcc_MaxAccDiffToZero, String::EMPTY );
	PROPERTY_EDIT( m_filterLowPass_RC, String::EMPTY );
END_CLASS_RTTI();
