/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeNodeAtomicAction.h"


class CBehTreeNodePCReleaseControlInstance;

class CBehTreeNodePCReleaseControlDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodePCReleaseControlDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodePCReleaseControlInstance, PC_ReleaseControl );
public:
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodePCReleaseControlDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition );
END_CLASS_RTTI();

class CBehTreeNodePCReleaseControlInstance : public CBehTreeNodeAtomicActionInstance
{
	typedef CBehTreeNodeAtomicActionInstance Super;
public:
	typedef CBehTreeNodePCReleaseControlDefinition Definition;

	CBehTreeNodePCReleaseControlInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )										{}

	Bool Activate() override;
	void Deactivate() override;
};

