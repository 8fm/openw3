/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeNodeDecoratorFlight.h"

class CBehTreeNodeDecoratorGlideInstance;

class CBehTreeNodeDecoratorGlideDefinition : public IBehTreeNodeFlightDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDecoratorGlideDefinition, IBehTreeNodeFlightDecoratorDefinition, CBehTreeNodeDecoratorGlideInstance, GlideDecorator );
protected:
	Float						m_minChangeDelay;

	Float						m_glideChance;
	Float						m_stopGlideChance;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeDecoratorGlideDefinition()
		: m_minChangeDelay( 2.f )
		, m_glideChance( 0.25f )
		, m_stopGlideChance( 0.333f )										{}
};


BEGIN_CLASS_RTTI( CBehTreeNodeDecoratorGlideDefinition )
	PARENT_CLASS( IBehTreeNodeFlightDecoratorDefinition )
	PROPERTY_EDIT( m_minChangeDelay, TXT("Delay between changing flight state") )
	PROPERTY_EDIT( m_glideChance, TXT("Chance to go gliding") )
	PROPERTY_EDIT( m_stopGlideChance, TXT("Chance to stop gliding and start flapping") )
END_CLASS_RTTI()

class CBehTreeNodeDecoratorGlideInstance : public IBehTreeNodeFlightDecoratorInstance
{
	typedef IBehTreeNodeFlightDecoratorInstance Super;
protected:
	Bool						m_isGliding;
	Float						m_testTimeout;

	Float						m_minChangeDelay;

	Float						m_glideChance;
	Float						m_stopGlideChance;

	void SetBehaviorVal( Float v ) const;
public:
	typedef CBehTreeNodeDecoratorGlideDefinition Definition;

	CBehTreeNodeDecoratorGlideInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_isGliding( false )
		, m_testTimeout( 0.f )
		, m_minChangeDelay( def.m_minChangeDelay )
		, m_glideChance( def.m_glideChance )
		, m_stopGlideChance( def.m_stopGlideChance )					{}

	void Update() override;
	void Deactivate() override;
};