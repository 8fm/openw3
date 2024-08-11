/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeDecorator.h"

class CBehTreeNodeDecoratorSnapToNavigationInstance;

class CBehTreeNodeDecoratorSnapToNavigationDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDecoratorSnapToNavigationDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeDecoratorSnapToNavigationInstance, SnapToNavigation )
protected:
	Bool							m_performActivation;
	Bool							m_snapOnActivation;
	Bool							m_performDeactivation;
	Bool							m_snapOnDeactivation;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeDecoratorSnapToNavigationDefinition()
		: m_performActivation( true )
		, m_snapOnActivation( false )
		, m_performDeactivation( true )
		, m_snapOnDeactivation( true )													{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeDecoratorSnapToNavigationDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
	PROPERTY_EDIT( m_performActivation, TXT("Perform snap code on activation") )
	PROPERTY_EDIT( m_snapOnActivation, TXT("Snap or un/snap on activation") )
	PROPERTY_EDIT( m_performDeactivation, TXT("Perform snap code on deactivation") )
	PROPERTY_EDIT( m_snapOnDeactivation, TXT("Snap or un/snap on deactivation") )
END_CLASS_RTTI()


class CBehTreeNodeDecoratorSnapToNavigationInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	Bool							m_performActivation;
	Bool							m_snapOnActivation;
	Bool							m_performDeactivation;
	Bool							m_snapOnDeactivation;
public:
	typedef CBehTreeNodeDecoratorSnapToNavigationDefinition Definition;
	
	CBehTreeNodeDecoratorSnapToNavigationInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_performActivation( def.m_performActivation )
		, m_snapOnActivation( def.m_snapOnActivation )
		, m_performDeactivation( def.m_performDeactivation )
		, m_snapOnDeactivation( def.m_snapOnDeactivation )							{}

	Bool Activate() override;
	void Deactivate() override;
};