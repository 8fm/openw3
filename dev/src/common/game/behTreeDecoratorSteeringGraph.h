#pragma once

#include "behTreeDecorator.h"
#include "behTreeSteeringGraphBase.h"

class CBehTreeDecoratorSteeringGraphInstance;

class CBehTreeDecoratorSteeringGraphDefinition : public IBehTreeNodeDecoratorDefinition, public CBehTreeSteeringGraphCommonDef
{
	friend class CBehTreeDecoratorSteeringGraphInstance;

	DECLARE_BEHTREE_NODE( CBehTreeDecoratorSteeringGraphDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeDecoratorSteeringGraphInstance, SteeringGraphDecorator );

protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeDecoratorSteeringGraphDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );
	PROPERTY_EDIT( m_steeringGraph, TXT("Steering graph resource") );
END_CLASS_RTTI();


class CBehTreeDecoratorSteeringGraphInstance : public IBehTreeNodeDecoratorInstance, public CBehTreeSteeringGraphCommonInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
public:
	typedef CBehTreeDecoratorSteeringGraphDefinition Definition;

	CBehTreeDecoratorSteeringGraphInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
	~CBehTreeDecoratorSteeringGraphInstance();

	Bool			Activate() override;
	void			Deactivate() override;
};