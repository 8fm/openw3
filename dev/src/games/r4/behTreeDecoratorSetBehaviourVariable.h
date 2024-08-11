#pragma once
#include "../../common/game/behTreeDecorator.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorSetBehaviorVariableDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeDecoratorSetBehaviorVariableInstance;
class CBehTreeDecoratorSetBehaviorVariableDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeDecoratorSetBehaviorVariableDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeDecoratorSetBehaviorVariableInstance, SetBehaviorVariable );
protected:
	CName	m_VarName;
	Float	m_valueActivate;
	Float	m_valueDeactivate;
	Bool	m_setVarActivate;
	Bool	m_setVarDeactivate;

public:
	CBehTreeDecoratorSetBehaviorVariableDefinition() 
		: m_valueActivate( 0.0f )
		, m_valueDeactivate( 0.0f )
		, m_setVarActivate( true )
		, m_setVarDeactivate( false )
	{
	}
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeDecoratorSetBehaviorVariableDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );	
	PROPERTY_EDIT( m_VarName,					TXT("The name of the variable you want to set on the active anim set") );
	PROPERTY_EDIT( m_setVarActivate,			TXT("If true the variable will be set to valueActivate during the Activate call") )
	PROPERTY_EDIT( m_valueActivate,				TXT("The value of the variable you want to set on the active behaviour graph, this value is set during activate") );
	PROPERTY_EDIT( m_setVarDeactivate,			TXT("If true the variable will be set to m_valueDeactivate during the Deactivate call") )
	PROPERTY_EDIT( m_valueDeactivate,			TXT("The value of the variable you want to set on the active behaviour graph, this value is set during deactivate") );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorSetBehaviorVariableInstance
class CBehTreeDecoratorSetBehaviorVariableInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	CName	m_varName;
	Float	m_valueActivate;
	Float	m_valueDeactivate;
	Bool	m_setVarActivate;
	Bool	m_setVarDeactivate;
public:
	typedef CBehTreeDecoratorSetBehaviorVariableDefinition Definition;

	CBehTreeDecoratorSetBehaviorVariableInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: IBehTreeNodeDecoratorInstance( def, owner, context, parent ) 
		, m_varName( def.m_VarName )
		, m_valueActivate( def.m_valueActivate )
		, m_valueDeactivate( def.m_valueDeactivate )
		, m_setVarActivate( def.m_setVarActivate )
		, m_setVarDeactivate( def.m_setVarDeactivate )
	{}

	Bool Activate() override;
	void Deactivate() override;
};


////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorOverrideBehaviorVariableDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeDecoratorOverrideBehaviorVariableInstance;
class CBehTreeDecoratorOverrideBehaviorVariableDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeDecoratorOverrideBehaviorVariableDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeDecoratorOverrideBehaviorVariableInstance, OverrideBehaviorVariable );
protected:
	CName	m_varName;
	Float	m_value;

public:
	CBehTreeDecoratorOverrideBehaviorVariableDefinition() 
		: m_varName( )
		, m_value( 0.0f )
	{
	}
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeDecoratorOverrideBehaviorVariableDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );	
	PROPERTY_EDIT( m_varName,		TXT("The name of the variable you want to set on the active anim set, this var will be reset to its previous value when the node will be deactivated") );
	PROPERTY_EDIT( m_value,			TXT("The value of the variable you want to set") )
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorSetBehaviorVariableInstance
class CBehTreeDecoratorOverrideBehaviorVariableInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	CName	m_varName;
	Float	m_value;
	Float	m_previousValue;
public:
	typedef CBehTreeDecoratorOverrideBehaviorVariableDefinition Definition;

	CBehTreeDecoratorOverrideBehaviorVariableInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: IBehTreeNodeDecoratorInstance( def, owner, context, parent ) 
		, m_varName( def.m_varName )
		, m_value( def.m_value )
		, m_previousValue( 0.0f )
	{}

	Bool Activate() override;
	void Deactivate() override;
};


