/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

#include "behaviorGraphValueNode.h"

class CBehaviorGraphValueNode;

// --------------------------------------------------------------------------------------------------------------------
enum EBehaviorVectorMathOp
{
	BVMO_Add,
	BVMO_Subtract,
	BVMO_Multiply,
	BVMO_Divide,
	BVMO_CrossProduct,
	BVMO_DotProduct,
	BVMO_Length,
	BVMO_XComponent,
	BVMO_YComponent,
	BVMO_ZComponent,
	BVMO_WComponent,
};

BEGIN_ENUM_RTTI( EBehaviorVectorMathOp );
	ENUM_OPTION( BVMO_Add )
	ENUM_OPTION( BVMO_Subtract )
	ENUM_OPTION( BVMO_Multiply )
	ENUM_OPTION( BVMO_Divide )
	ENUM_OPTION( BVMO_CrossProduct )
	ENUM_OPTION( BVMO_DotProduct )
	ENUM_OPTION( BVMO_Length )
	ENUM_OPTION( BVMO_XComponent )
	ENUM_OPTION( BVMO_YComponent )
	ENUM_OPTION( BVMO_ZComponent )
	ENUM_OPTION( BVMO_WComponent )
END_ENUM_RTTI();

// --------------------------------------------------------------------------------------------------------------------
// class defining variable value
class CBehaviorGraphVectorVariableBaseNode : public CBehaviorGraphVectorValueNode
{
	DECLARE_BEHAVIOR_ABSTRACT_CLASS( CBehaviorGraphVectorVariableBaseNode, CBehaviorGraphVectorValueNode );

public:
	CBehaviorGraphVectorVariableBaseNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets();

#endif

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const {}
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const { return 0.f; }
	virtual Vector GetVectorValue( CBehaviorGraphInstance& instance ) const = 0;

public:
	virtual const CName& GetVariableName() const = 0;

};

BEGIN_ABSTRACT_CLASS_RTTI( CBehaviorGraphVectorVariableBaseNode );
	PARENT_CLASS( CBehaviorGraphVectorValueNode );
END_CLASS_RTTI();


// --------------------------------------------------------------------------------------------------------------------
class CBehaviorGraphConstantVectorValueNode : public CBehaviorGraphVectorValueNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphConstantVectorValueNode, CBehaviorGraphVectorValueNode, "Vector", "Vector value" );

protected:
	Vector m_value;

public:
	CBehaviorGraphConstantVectorValueNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
#endif

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const {}
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const { return 0.f; }
	virtual Vector GetVectorValue( CBehaviorGraphInstance& instance ) const { return m_value; }

};

BEGIN_CLASS_RTTI( CBehaviorGraphConstantVectorValueNode );
	PARENT_CLASS( CBehaviorGraphVectorValueNode );
	PROPERTY_EDIT( m_value, TXT("Value") );
END_CLASS_RTTI();


// --------------------------------------------------------------------------------------------------------------------
class CBehaviorGraphVectorMathNode	: public CBehaviorGraphVectorValueNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphVectorMathNode, CBehaviorGraphVectorValueNode, "Vector", "Vector math op" );			

protected:
	EBehaviorVectorMathOp				m_operation;

protected:
	CBehaviorGraphVectorValueNode*		m_cachedFirstInputNode;
	CBehaviorGraphVectorValueNode*		m_cachedSecondInputNode;
	CBehaviorGraphValueNode*			m_cachedScalarInputNode;

public:
	CBehaviorGraphVectorMathNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty *prop );
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
#endif

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;
	virtual Vector GetVectorValue( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();
};

BEGIN_CLASS_RTTI( CBehaviorGraphVectorMathNode );
	PARENT_CLASS( CBehaviorGraphVectorValueNode );
	PROPERTY_EDIT( m_operation, TXT("Operation type") );
	PROPERTY( m_cachedFirstInputNode );
	PROPERTY( m_cachedSecondInputNode );
	PROPERTY( m_cachedScalarInputNode );
END_CLASS_RTTI();