#pragma once
#include "../../common/game/behTreeNodePredefinedPath.h"
#include "getRiderOrMountHelper.h"
///////////////////////////////////////////////////////////////////////////////////////



class CR4BehTreeNodePredefinedPathWithCompanionInstance;

class CR4BehTreeNodePredefinedPathWithCompanionDefinition : public CBehTreeNodePredefinedPathWithCompanionDefinition
{
	DECLARE_BEHTREE_NODE( CR4BehTreeNodePredefinedPathWithCompanionDefinition, CBehTreeNodePredefinedPathWithCompanionDefinition, CR4BehTreeNodePredefinedPathWithCompanionInstance, R4FollowPredefinedPathWithCompanion );

public:
	CR4BehTreeNodePredefinedPathWithCompanionDefinition() {}

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CR4BehTreeNodePredefinedPathWithCompanionDefinition );
	PARENT_CLASS( CBehTreeNodePredefinedPathWithCompanionDefinition);
END_CLASS_RTTI();


//////////////////////////////////////////////////////////////////////////
class CR4BehTreeNodePredefinedPathWithCompanionInstance : public CBehTreeNodePredefinedPathWithCompanionInstance
{
	typedef CBehTreeNodePredefinedPathWithCompanionInstance Super;
public:
	typedef CR4BehTreeNodePredefinedPathWithCompanionDefinition Definition;

	CR4BehTreeNodePredefinedPathWithCompanionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool  Activate() override;
protected:
	CEntity *const GetCompanion()const override;

private :
	CGetRiderOrMountHelper m_getRiderOrMountHelper;
};
