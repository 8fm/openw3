#pragma once

#include "../../common/game/behTreeDecorator.h"
#include "../../common/game/behTreeInstance.h"
#include "../../common/game/behTreeVars.h"
#include "../../common/game/sceneDefinitionNode.h"


#include "../../common/game/behTreeCustomMoveData.h"

class CBehTreeNodeGreetingReactionSceneDecoratorInstance;

////////////////////////////////////////////////////////////////////////
// Definition
class CBehTreeNodeGreetingReactionSceneDecoratorDefinition : public CBehTreeNodeReactionSceneDefinitionDecorator
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeGreetingReactionSceneDecoratorDefinition, CBehTreeNodeReactionSceneDefinitionDecorator, CBehTreeNodeGreetingReactionSceneDecoratorInstance, GreetingReactionScene );
protected:	
	float m_maxDistance;
	float m_minDistance;

public:
	CBehTreeNodeGreetingReactionSceneDecoratorDefinition( ) : m_maxDistance( 5 ), m_minDistance( 1 ){}

protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;	
};

BEGIN_CLASS_RTTI( CBehTreeNodeGreetingReactionSceneDecoratorDefinition );
	PARENT_CLASS( CBehTreeNodeReactionSceneDefinitionDecorator );		
	PROPERTY_EDIT( m_maxDistance, TXT("") );	
	PROPERTY_EDIT( m_minDistance, TXT("") );
END_CLASS_RTTI();


class CBehTreeNodeGreetingReactionSceneDecoratorInstance : public CBehTreeNodeReactionSceneDefinitionDecoratorInstance
{
	typedef CBehTreeNodeReactionSceneDefinitionDecoratorInstance Super;
protected:	
	float m_maxDistanceSqr;
	float m_minDistanceSqr;

public:
	typedef CBehTreeNodeGreetingReactionSceneDecoratorDefinition Definition;

	CBehTreeNodeGreetingReactionSceneDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_maxDistanceSqr( def.m_maxDistance * def.m_maxDistance )
		, m_minDistanceSqr( def.m_minDistance * def.m_minDistance )
		{}


	Bool ConditionCheck() override;
	Bool CanBeAssignedToScene( CBehTreeReactionEventData* reactionData ) override;
	Bool CheckPositioning( CBehTreeReactionEventData* reactionData );
};