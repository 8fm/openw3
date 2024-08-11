/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeNodeSelectTargetByTag.h"
#include "../../common/game/behTreeVarsEnums.h"

class CBehTreeNodeInputDecoratorInstance;








enum EInputDecoratorCondition
{
	IDC_IN_RANGE,
	IDC_OUT_OF_RANGE,
	IDC_RANGE_CHANGED,
	IDC_GOT_IN_RANGE,
	IDC_GOT_OUT_OF_RANGE
};

BEGIN_ENUM_RTTI( EInputDecoratorCondition );
	ENUM_OPTION_DESC( TXT( "The input value is in range." )				, IDC_IN_RANGE			);
	ENUM_OPTION_DESC( TXT( "The input value is out of range." )			, IDC_OUT_OF_RANGE		);
	ENUM_OPTION_DESC( TXT( "The input value got in or out of range." )	, IDC_RANGE_CHANGED		);
	ENUM_OPTION_DESC( TXT( "The input value got in to the range." )		, IDC_GOT_IN_RANGE		);
	ENUM_OPTION_DESC( TXT( "The input value got out of the range." )	, IDC_GOT_OUT_OF_RANGE	);
END_ENUM_RTTI();



class CBehTreeValEInputDecoratorCondition : public TBehTreeValEnum< EInputDecoratorCondition, IDC_IN_RANGE >
{
	DECLARE_RTTI_STRUCT( CBehTreeValEInputDecoratorCondition );

private:
	typedef TBehTreeValEnum< EInputDecoratorCondition, IDC_IN_RANGE > TBaseClass;

public:
	CBehTreeValEInputDecoratorCondition( EInputDecoratorCondition e = IDC_IN_RANGE )
		: TBaseClass( e )
	{}
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeValEInputDecoratorCondition );
	PROPERTY_EDIT( m_varName, TXT("Variable") );
	PROPERTY_EDIT( m_value	, TXT("Move type") );
END_CLASS_RTTI();







enum EInputDecoratorAction
{
	IDA_PERFORM_ACTION,
	IDA_START_ACTION,
	IDA_STOP_ACTION
};


BEGIN_ENUM_RTTI( EInputDecoratorAction );
	ENUM_OPTION_DESC( TXT( "Action will be performed only while condition is fulfilled." )	, IDA_PERFORM_ACTION	);
	ENUM_OPTION_DESC( TXT( "Action will be started when condition will be fulfilled." )		, IDA_START_ACTION		);
	ENUM_OPTION_DESC( TXT( "Action will be stopped when condition will be fulfilled." )		, IDA_STOP_ACTION		);
END_ENUM_RTTI();





class CBehTreeValEInputDecoratorAction : public TBehTreeValEnum< EInputDecoratorAction, IDA_PERFORM_ACTION >
{
	DECLARE_RTTI_STRUCT( CBehTreeValEInputDecoratorAction );

private:
	typedef TBehTreeValEnum< EInputDecoratorAction, IDA_PERFORM_ACTION > TBaseClass;

public:
	CBehTreeValEInputDecoratorAction( EInputDecoratorAction e = IDA_PERFORM_ACTION )
		: TBaseClass( e )
	{}
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeValEInputDecoratorAction );
	PROPERTY_EDIT( m_varName, TXT("Variable") );
	PROPERTY_EDIT( m_value	, TXT("Move type") );
END_CLASS_RTTI();







////////////////////////////////////////////////////////////////////////
// Decorator that sets sub-action target, and resets it on deactivation.
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeInputDecoratorDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeInputDecoratorDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeInputDecoratorInstance, InputDecorator );
	DECLARE_AS_R6_ONLY

protected:
	CBehTreeValCName						m_inputName;

	CBehTreeValFloat						m_rangeMin;
	CBehTreeValFloat						m_rangeMax;

	CBehTreeValEInputDecoratorCondition		m_condition;
	CBehTreeValEInputDecoratorAction		m_action;
public:
	CBehTreeNodeInputDecoratorDefinition();
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};


BEGIN_CLASS_RTTI( CBehTreeNodeInputDecoratorDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );
	PROPERTY_EDIT( m_inputName				, TXT( "Name of input to be evaluated - i.e. 'GI_JUMP' etc." ) );
	PROPERTY_EDIT( m_rangeMin				, TXT( "If value of input is below this value, this node is unavailable." ) );
	PROPERTY_EDIT( m_rangeMax				, TXT( "If value of input is above this value, this node is unavailable." ) );
	PROPERTY_EDIT( m_condition				, TXT( "Condition to be fulfilled to perform the action." ) );
	PROPERTY_EDIT( m_action					, TXT( "The way of performing the action when condition is fulfilled." ) );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeInputDecoratorInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;

	CName						m_inputName;

	Float						m_rangeMin;
	Float						m_rangeMax;

	EInputDecoratorCondition	m_condition;
	EInputDecoratorAction		m_action;

	Bool						m_prevInRange;

public:
	typedef CBehTreeNodeInputDecoratorDefinition Definition;

	CBehTreeNodeInputDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	////////////////////////////////////////////////////////////////////
	//! Evaluation
	Bool IsAvailable() override;


private:

	Bool CheckCondition();

	Bool CheckInRangeCondition( Float val );

	const CResource* GetOwnerResource() const;
};

