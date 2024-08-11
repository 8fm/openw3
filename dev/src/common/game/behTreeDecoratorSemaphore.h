/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeDecorator.h"

#include "behTreeCounterData.h"

class CBehTreeNodeDecoratorSemaphoreInstance;

class CBehTreeNodeDecoratorSemaphoreDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDecoratorSemaphoreDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeDecoratorSemaphoreInstance, Semaphore );
protected:
	CName		m_semaphoreName;
	Bool		m_raise;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeDecoratorSemaphoreDefinition()
		: m_raise( true )															{}

	String GetNodeCaption() const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeDecoratorSemaphoreDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
	PROPERTY_EDIT( m_semaphoreName, TXT("Name of aistorage counter") )
	PROPERTY_EDIT( m_raise, TXT("Raise or lower the counter") )
END_CLASS_RTTI()

class CBehTreeNodeDecoratorSemaphoreInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	CBehTreeCounterDataPtr			m_ptr;
	Bool							m_raise;
public:
	typedef CBehTreeNodeDecoratorSemaphoreDefinition Definition;

	CBehTreeNodeDecoratorSemaphoreInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool Activate() override;
	void Deactivate() override;
};