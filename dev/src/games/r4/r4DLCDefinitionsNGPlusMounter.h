/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "r4DLCDefinitionsMounter.h"

class CR4DefinitionsNGPlusDLCMounter : public CR4DefinitionsDLCMounter
{
	DECLARE_ENGINE_CLASS( CR4DefinitionsNGPlusDLCMounter, CR4DefinitionsDLCMounter, 0 );

private:
	virtual void Activate() override;
	virtual void Deactivate() override;

	virtual Bool ShouldLoad() override;
};

BEGIN_CLASS_RTTI( CR4DefinitionsNGPlusDLCMounter );
PARENT_CLASS( CR4DefinitionsDLCMounter );
END_CLASS_RTTI();
