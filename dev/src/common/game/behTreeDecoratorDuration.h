#pragma once

#include "behTreeDecorator.h"

class CBehTreeNodeDurationDecoratorInstance;
class CBehTreeNodeDurationRangeDecoratorInstance;


////////////////////////////////////////////////////////////////////////
// Simple decorator that breaks behavior in given time
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeDurationDecoratorDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDurationDecoratorDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeDurationDecoratorInstance, Duration );

protected:
	CBehTreeValFloat					m_duration;
	CBehTreeValFloat					m_chance;
	Bool								m_endWithFailure;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeDurationDecoratorDefinition()
		: m_duration( 10.f )
		, m_chance( 1.00001f )
		, m_endWithFailure( false )														{}

};


BEGIN_CLASS_RTTI( CBehTreeNodeDurationDecoratorDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );
	PROPERTY_EDIT( m_duration, TXT("Duration after which behavior will be 'completed'") );
	PROPERTY_EDIT( m_chance, TXT("Completion chance") );
	PROPERTY_EDIT( m_endWithFailure, TXT("Should duration trigger failure or success completion") );
END_CLASS_RTTI();


class CBehTreeNodeDurationDecoratorInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	Float								m_duration;
	Float								m_chance;
	Bool								m_endWithFailure;
	Float								m_delay;
public:
	typedef CBehTreeNodeDurationDecoratorDefinition Definition;

	CBehTreeNodeDurationDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool Activate() override;
	void Update() override;
};


////////////////////////////////////////////////////////////////////////
// Simple decorator that breaks behavior in given time
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeDurationRangeDecoratorDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDurationRangeDecoratorDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeDurationRangeDecoratorInstance, DurationRange );

protected:
	CBehTreeValFloat					m_durationMin;
	CBehTreeValFloat					m_durationMax;
	Bool								m_endWithFailure;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeDurationRangeDecoratorDefinition()
		: m_durationMin( 5.f )
		, m_durationMax( 10.f )
		, m_endWithFailure( false )														{}

};


BEGIN_CLASS_RTTI( CBehTreeNodeDurationRangeDecoratorDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );
	PROPERTY_EDIT( m_durationMin, TXT("Duration randomization range min") );
	PROPERTY_EDIT( m_durationMax, TXT("Duration randomization range max") );
	PROPERTY_EDIT( m_endWithFailure, TXT("Should duration trigger failure or success completion") );
END_CLASS_RTTI();


class CBehTreeNodeDurationRangeDecoratorInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	Float								m_durationMin;
	Float								m_durationDiff;
	Bool								m_endWithFailure;
	Float								m_delay;
public:
	typedef CBehTreeNodeDurationRangeDecoratorDefinition Definition;

	CBehTreeNodeDurationRangeDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool Activate() override;
	void Update() override;
};


