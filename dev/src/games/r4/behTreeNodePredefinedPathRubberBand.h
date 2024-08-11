#pragma once 
#include "../../common/game/behTreeNodePredefinedPath.h"



///////////////////////////////////////////////////////////////////////////////////////
// CBehTreeNodePredefinedPathRuberBandDefinition
class CBehTreeNodePredefinedPathRuberBandInstance;

class CBehTreeNodePredefinedPathRuberBandDefinition : public CBehTreeNodePredefinedPathDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodePredefinedPathRuberBandDefinition, CBehTreeNodePredefinedPathDefinition, CBehTreeNodePredefinedPathRuberBandInstance, FollowPredefinedPathRuberBand );
protected:

public:
	CBehTreeNodePredefinedPathRuberBandDefinition() {}

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodePredefinedPathRuberBandDefinition );
PARENT_CLASS( CBehTreeNodePredefinedPathDefinition);
END_CLASS_RTTI();


//////////////////////////////////////////////////////////////////////////
// CBehTreeNodePredefinedPathRuberBandInstance
class CBehTreeNodePredefinedPathRuberBandInstance : public CBehTreeNodePredefinedPathInstance
{
	typedef CBehTreeNodePredefinedPathInstance Super;
protected:


public:
	typedef CBehTreeNodePredefinedPathRuberBandDefinition Definition;

	CBehTreeNodePredefinedPathRuberBandInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	// IMovementTargeter interface
	void					UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) override;
};