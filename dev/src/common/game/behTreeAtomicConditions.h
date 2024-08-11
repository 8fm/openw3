#pragma once

#include "behTreeNodeComplexCondition.h"

////////////////////////////////////////////////////////////////////////
// Base atomic condition used by complex conditions
class IBehTreeAtomicCondition : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IBehTreeAtomicCondition, CObject );
public:
	virtual IBehTreeAtomicConditionInstance* SpawnInstance( CBehTreeSpawnContext& context );
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeAtomicCondition );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();


class IBehTreeAtomicConditionInstance
{
public:
	typedef IBehTreeAtomicCondition Definition;

	IBehTreeAtomicConditionInstance( const Definition* def, CBehTreeSpawnContext& context )	{}

	virtual Bool		ConditionCheck( CBehTreeNodeComplexConditionInstance* node ); 
};

////////////////////////////////////////////////////////////////////////
// Base for binary conditions
class IBehTreeAtomicBinaryCondition : public IBehTreeAtomicCondition
{
	friend class IBehTreeAtomicBinaryConditionInstance;

	DECLARE_ENGINE_ABSTRACT_CLASS( IBehTreeAtomicBinaryCondition, IBehTreeAtomicCondition );
protected:
	IBehTreeAtomicCondition*			m_condition1;
	IBehTreeAtomicCondition*			m_condition2;
public:
	IBehTreeAtomicBinaryCondition()
		: m_condition1( NULL )
		, m_condition2( NULL )													{}
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeAtomicBinaryCondition );
	PARENT_CLASS( IBehTreeAtomicCondition );
	PROPERTY_INLINED( m_condition1, TXT("Condition 1") );
	PROPERTY_INLINED( m_condition2, TXT("Condition 2") );
END_CLASS_RTTI();

class IBehTreeAtomicBinaryConditionInstance : public IBehTreeAtomicConditionInstance
{
	typedef IBehTreeAtomicConditionInstance Super;
protected:
	IBehTreeAtomicConditionInstance*		m_condition1;
	IBehTreeAtomicConditionInstance*		m_condition2;
public:
	typedef IBehTreeAtomicBinaryCondition Definition;

	IBehTreeAtomicBinaryConditionInstance( const Definition* def, CBehTreeSpawnContext& context );
};
////////////////////////////////////////////////////////////////////////
// AND condition
class CBehTreeAtomicANDCondition : public IBehTreeAtomicBinaryCondition
{
	DECLARE_ENGINE_CLASS( CBehTreeAtomicANDCondition, IBehTreeAtomicBinaryCondition, 0 );
public:
	IBehTreeAtomicConditionInstance* SpawnInstance( CBehTreeSpawnContext& context ) override;
};

BEGIN_CLASS_RTTI( CBehTreeAtomicANDCondition );
	PARENT_CLASS( IBehTreeAtomicBinaryCondition );
END_CLASS_RTTI();

class CBehTreeAtomicANDConditionInstance : public IBehTreeAtomicBinaryConditionInstance
{
	typedef IBehTreeAtomicBinaryConditionInstance Super;
public:
	typedef CBehTreeAtomicANDCondition Definition;

	CBehTreeAtomicANDConditionInstance( const Definition* def, CBehTreeSpawnContext& context )
		: Super( def, context )											{}

	Bool		ConditionCheck( CBehTreeNodeComplexConditionInstance* node ) override; 
};

////////////////////////////////////////////////////////////////////////
// OR condition
class CBehTreeAtomicORCondition : public IBehTreeAtomicBinaryCondition
{
	DECLARE_ENGINE_CLASS( CBehTreeAtomicORCondition, IBehTreeAtomicBinaryCondition, 0 );
public:
	IBehTreeAtomicConditionInstance* SpawnInstance( CBehTreeSpawnContext& context ) override;
};

BEGIN_CLASS_RTTI( CBehTreeAtomicORCondition );
	PARENT_CLASS( IBehTreeAtomicBinaryCondition );
END_CLASS_RTTI();

class CBehTreeAtomicORConditionInstance : public IBehTreeAtomicBinaryConditionInstance
{
	typedef IBehTreeAtomicBinaryConditionInstance Super;
public:
	typedef CBehTreeAtomicORCondition Definition;

	CBehTreeAtomicORConditionInstance( const Definition* def, CBehTreeSpawnContext& context )
		: Super( def, context )											{}

	Bool		ConditionCheck( CBehTreeNodeComplexConditionInstance* node ) override; 
};

////////////////////////////////////////////////////////////////////////
// NOT condition
class CBehTreeAtomicNOTCondition : public IBehTreeAtomicCondition
{
	friend class CBehTreeAtomicNOTConditionInstance;

	DECLARE_ENGINE_CLASS( CBehTreeAtomicNOTCondition, IBehTreeAtomicCondition, 0 );
protected:
	IBehTreeAtomicCondition*			m_child;
public:
	IBehTreeAtomicConditionInstance* SpawnInstance( CBehTreeSpawnContext& context ) override;
};

BEGIN_CLASS_RTTI( CBehTreeAtomicNOTCondition );
	PARENT_CLASS( IBehTreeAtomicCondition );
	PROPERTY_EDIT( m_child, TXT("'Not' condition") );
END_CLASS_RTTI();

class CBehTreeAtomicNOTConditionInstance : public IBehTreeAtomicConditionInstance
{
	typedef IBehTreeAtomicConditionInstance Super;
protected:
	IBehTreeAtomicConditionInstance*	m_child;
public:
	typedef CBehTreeAtomicNOTCondition Definition;

	CBehTreeAtomicNOTConditionInstance( const Definition* def, CBehTreeSpawnContext& context );

	Bool		ConditionCheck( CBehTreeNodeComplexConditionInstance* node ) override; 
};

////////////////////////////////////////////////////////////////////////
// SUBTREE TEST condition
class CBehTreeAtomicTestSubtreeCondition : public IBehTreeAtomicCondition
{
	DECLARE_ENGINE_CLASS( CBehTreeAtomicTestSubtreeCondition, IBehTreeAtomicCondition, 0 );
public:
	IBehTreeAtomicConditionInstance* SpawnInstance( CBehTreeSpawnContext& context ) override;
};

BEGIN_CLASS_RTTI( CBehTreeAtomicTestSubtreeCondition );
	PARENT_CLASS( IBehTreeAtomicCondition );
END_CLASS_RTTI();

class CBehTreeAtomicTestSubtreeConditionInstance : public IBehTreeAtomicConditionInstance
{
	typedef IBehTreeAtomicConditionInstance Super;
public:
	typedef CBehTreeAtomicTestSubtreeCondition Definition;

	CBehTreeAtomicTestSubtreeConditionInstance( const Definition* def, CBehTreeSpawnContext& context )
		: Super( def, context )												{}

	Bool		ConditionCheck( CBehTreeNodeComplexConditionInstance* node ) override; 
};