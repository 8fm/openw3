#pragma once

#include "behTreeMetanodeOnSpawn.h"

class CBehTreeMetanodeWorkInitializer : public IBehTreeMetanodeOnSpawnDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeMetanodeWorkInitializer, IBehTreeMetanodeOnSpawnDefinition, IBehTreeNodeInstance, InitializeWork );

protected:
	void					RunOnSpawn( CBehTreeSpawnContext& context, CBehTreeInstance* owner ) const override;

public:
	CBehTreeMetanodeWorkInitializer()											{}

};


BEGIN_CLASS_RTTI( CBehTreeMetanodeWorkInitializer );
	PARENT_CLASS( IBehTreeMetanodeOnSpawnDefinition );
END_CLASS_RTTI();