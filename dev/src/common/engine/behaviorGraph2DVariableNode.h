#pragma once

#include "behaviorGraphValueNode.h"

#define BEHAVIOR_GRAPH_2DVARIABLE_NODEMARGIN 5

class CBehaviorGraph2DVariableNode : public CBehaviorGraphVectorValueNode
{
	DECLARE_BEHAVIOR_ABSTRACT_CLASS( CBehaviorGraph2DVariableNode, CBehaviorGraphVectorValueNode );

public:
	CBehaviorGraph2DVariableNode() {}

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual Bool IsDraggedByClickOnInnerArea() const { return false; }
	virtual const Vector GetSize() const { return Vector(170,175,0); }
	virtual Bool IsResizable() const { return true; }
	virtual Vector GetMinVal( CBehaviorGraphInstance& instance ) const = 0;
	virtual Vector GetMaxVal( CBehaviorGraphInstance& instance ) const = 0;
	virtual void SetVectorValue( CBehaviorGraphInstance& instance, const Vector& val ) = 0;
#endif

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const {}
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const { return GetVectorValue( instance ).X; }
};

BEGIN_ABSTRACT_CLASS_RTTI( CBehaviorGraph2DVariableNode );
	PARENT_CLASS( CBehaviorGraphVectorValueNode );
END_CLASS_RTTI();


class CBehaviorGraph2DVectorVariableNode : public CBehaviorGraph2DVariableNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraph2DVectorVariableNode, CBehaviorGraph2DVariableNode, "Vector", "Variable Vector 2D" );

protected:
	CName				m_variableName;		//!< Name of the variable

public:
	CBehaviorGraph2DVectorVariableNode() {}

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty *prop );
	virtual void OnRebuildSockets() {}

	virtual String GetCaption() const { return !m_variableName.Empty() ? String::Printf( TXT("Variable 2D [ %s ]"), m_variableName.AsChar() ) : TXT("Variable"); }
	
	virtual Vector GetMinVal( CBehaviorGraphInstance& instance ) const;
	virtual Vector GetMaxVal( CBehaviorGraphInstance& instance ) const;
	virtual void SetVectorValue( CBehaviorGraphInstance& instance, const Vector& val );

	virtual Bool IsDraggedByClickOnInnerArea() const { return false; }
	virtual Bool IsResizable() const { return true; }
#endif

public:

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const {}
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

	virtual Vector GetVectorValue( CBehaviorGraphInstance& instance ) const;

	virtual void GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const;

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const { return GetVectorValue( instance ).X; }
public:
	const CName& GetVariableName() const { return m_variableName; }
};

BEGIN_CLASS_RTTI( CBehaviorGraph2DVectorVariableNode );
PARENT_CLASS( CBehaviorGraph2DVariableNode );
PROPERTY_CUSTOM_EDIT( m_variableName, TXT("Variable name"), TXT("BehaviorVectorVariableSelection") );
END_CLASS_RTTI();


class CBehaviorGraph2DMultiVariablesNode : public CBehaviorGraph2DVariableNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraph2DMultiVariablesNode, CBehaviorGraph2DVariableNode, "Float", "2 Variables" );

protected:
	CName				m_variableName1;		//!< Name of the variable
	CName				m_variableName2;		//!< Name of the variable

public:
	CBehaviorGraph2DMultiVariablesNode() {}

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty *prop );
	virtual void OnRebuildSockets() {}
	virtual Color GetTitleColor() const { return Color( 64, 64, 255 ); }

	virtual String GetCaption() const { return String::Printf( TXT("Variable 2D [ %s, %s ]"), m_variableName1.AsChar(), m_variableName2.AsChar() ); }

	virtual Bool IsDraggedByClickOnInnerArea() const { return false; }
	virtual Bool IsResizable() const { return true; }

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const { return GetVectorValue( instance ).X; }
	virtual Vector GetMinVal( CBehaviorGraphInstance& instance ) const;
	virtual Vector GetMaxVal( CBehaviorGraphInstance& instance ) const;
	virtual void SetVectorValue( CBehaviorGraphInstance& instance, const Vector& val );
#endif

public:

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const {}
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

	virtual Vector GetVectorValue( CBehaviorGraphInstance& instance ) const;

	virtual void GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraph2DMultiVariablesNode );
PARENT_CLASS( CBehaviorGraph2DVariableNode );
PROPERTY_CUSTOM_EDIT( m_variableName1, TXT("Variable 1 name"), TXT("BehaviorVariableSelection") );
PROPERTY_CUSTOM_EDIT( m_variableName2, TXT("Variable 2 name"), TXT("BehaviorVariableSelection") );
END_CLASS_RTTI();