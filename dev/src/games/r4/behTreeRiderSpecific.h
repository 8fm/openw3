#pragma once
#include "../../common/game/behTreeNodeAtomicPursue.h"
#include "../../common/game/behTreeDecorator.h"
#include "../../common/game/behTreeNodeAtomicRotate.h"
#include "ridingAiStorage.h"


////////////////////////////////////////////////////////////////
// CBehTreeNodeRiderPursueHorseDefinition
class CBehTreeNodeRiderPursueHorseInstance;
class CBehTreeNodeRiderPursueHorseDefinition : public CBehTreeNodeBaseAtomicPursueTargetDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeRiderPursueHorseDefinition, CBehTreeNodeBaseAtomicPursueTargetDefinition, CBehTreeNodeRiderPursueHorseInstance, RiderPursueHorse );
private:									

public:
	CBehTreeNodeRiderPursueHorseDefinition() {}

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeRiderPursueHorseDefinition );
PARENT_CLASS( CBehTreeNodeBaseAtomicPursueTargetDefinition);
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicPursueTargetInstance
class CBehTreeNodeRiderPursueHorseInstance : public CBehTreeNodeBaseAtomicPursueTargetInstance
{
	typedef CBehTreeNodeBaseAtomicPursueTargetInstance Super;
protected:
	CAIStorageRiderData::CStoragePtr		m_riderData;
	CNode*const ComputeTarget()override;
public:
	typedef CBehTreeNodeRiderPursueHorseDefinition Definition;

	CBehTreeNodeRiderPursueHorseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
};

//////////////////////////////////////////////////////////////////////////
// CBehTreeNodeRiderRotateToHorseDefinition
class CBehTreeNodeRiderRotateToHorseInstance;
class CBehTreeNodeRiderRotateToHorseDefinition : public CBehTreeNodeBaseRotateToTargetDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeRiderRotateToHorseDefinition, CBehTreeNodeBaseRotateToTargetDefinition, CBehTreeNodeRiderRotateToHorseInstance, RiderRotateToHorse );

public:
	CBehTreeNodeRiderRotateToHorseDefinition() {}

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeRiderRotateToHorseDefinition );
	PARENT_CLASS( CBehTreeNodeBaseRotateToTargetDefinition);
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehTreeNodeRiderRotateToHorseInstance : public CBehTreeNodeBaseRotateToTargetInstance
{
	typedef CBehTreeNodeBaseRotateToTargetInstance Super;
private:
	CAIStorageRiderData::CStoragePtr		m_riderData;

public:
	typedef CBehTreeNodeRiderRotateToHorseDefinition Definition;
	CBehTreeNodeRiderRotateToHorseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
private:
	CNode*const GetTarget()const override;
};
