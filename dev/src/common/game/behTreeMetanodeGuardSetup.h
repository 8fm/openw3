/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeMetanodeOnSpawn.h"

class CBehTreeMetanodeSetupGuardDefinition : public IBehTreeMetanodeOnSpawnDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeMetanodeSetupGuardDefinition, IBehTreeMetanodeOnSpawnDefinition, IBehTreeNodeInstance, SetupGuardArea );
protected:
	void					RunOnSpawn( CBehTreeSpawnContext& context, CBehTreeInstance* owner ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeMetanodeSetupGuardDefinition );
	PARENT_CLASS( IBehTreeMetanodeOnSpawnDefinition );
END_CLASS_RTTI();
