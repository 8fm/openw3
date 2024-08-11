#pragma once

#include "../../common/game/behTreeDecoratorSteeringTargeter.h"

#include "r4BehTreeVarEnums.h"


////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorHorseSpeedManagerDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeDecoratorHorseSpeedManagerInstance;
class CBehTreeDecoratorHorseSpeedManagerDefinition : public IBehTreeNodeDecoratorSteeringTargeterDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeDecoratorHorseSpeedManagerDefinition, IBehTreeNodeDecoratorSteeringTargeterDefinition, CBehTreeDecoratorHorseSpeedManagerInstance, HorseSpeedManager );
protected:
	CBehTreeValEHorseMoveType m_moveType; 	
public:
	CBehTreeDecoratorHorseSpeedManagerDefinition() 
		: m_moveType ( HMT_Gallop ){}

	String							GetNodeCaption() const override;
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeDecoratorHorseSpeedManagerDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorSteeringTargeterDefinition )
	PROPERTY_EDIT( m_moveType, TXT("horse move type") )
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorHorseSpeedManagerInstance
class CBehTreeDecoratorHorseSpeedManagerInstance : public IBehTreeNodeDecoratorSteeringTargeterInstance
{
	typedef IBehTreeNodeDecoratorSteeringTargeterInstance Super;
protected:
	EHorseMoveType m_moveType;
private:

public:
	typedef CBehTreeDecoratorHorseSpeedManagerDefinition Definition;

	CBehTreeDecoratorHorseSpeedManagerInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );

	// IBehTreeNodeDecoratorInstance
	void UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )override;
};
