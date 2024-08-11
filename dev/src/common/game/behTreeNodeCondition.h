#pragma once

#include "behTreeDecorator.h"

class CBehTreeNodeConditionInstance;

////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( CBehTreeNodeConditionDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeConditionInstance, Condition );
protected:
	Bool								m_forwardAvailability;
	Bool								m_forwardTestIfNotAvailable;
	Bool								m_invertAvailability;
	Bool								m_skipIfActive;

	String DecorateCaption( const String& baseCaption ) const;
public:
	CBehTreeNodeConditionDefinition( Bool forwardAvailability = false, Bool forwardTestIfNotAvailable = false, Bool invertAvailability = false, Bool skipIfActive = false )
		: m_forwardAvailability( forwardAvailability )
		, m_forwardTestIfNotAvailable( forwardTestIfNotAvailable )
		, m_invertAvailability( invertAvailability )
		, m_skipIfActive( skipIfActive )								{}

	String GetNodeCaption() const override;
};

BEGIN_ABSTRACT_CLASS_RTTI(CBehTreeNodeConditionDefinition);
	PARENT_CLASS(IBehTreeNodeDecoratorDefinition);	
	PROPERTY_EDIT( m_forwardAvailability, TXT("Forward availability computation") );
	PROPERTY_EDIT( m_forwardTestIfNotAvailable, TXT("Forward test to children if node is not-available") );
	PROPERTY_EDIT( m_invertAvailability, TXT("Invert availability test") );
	PROPERTY_EDIT( m_skipIfActive, TXT("Do not re-execute child if it is already running"));
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeNodeConditionInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	Bool								m_forwardAvailability;
	Bool								m_forwardTestIfNotAvailable;
	Bool								m_invertAvailability;
	Bool								m_skipIfActive;

	virtual Bool ConditionCheck()										= 0;
public:
	typedef CBehTreeNodeConditionDefinition Definition;

	CBehTreeNodeConditionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_forwardAvailability( def.m_forwardAvailability )
		, m_forwardTestIfNotAvailable( def.m_forwardTestIfNotAvailable )
		, m_invertAvailability( def.m_invertAvailability )
		, m_skipIfActive( def.m_skipIfActive )							{}

	Bool IsAvailable() override;
	Int32 Evaluate() override;

};