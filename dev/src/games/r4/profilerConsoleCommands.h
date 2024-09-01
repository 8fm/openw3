#pragma once

#include "../../common/game/scriptRegistrationManager.h"

class CR4ProfilerScriptRegistration: public CScriptRegistration
{
public:
	CR4ProfilerScriptRegistration();
	virtual void RegisterScriptFunctions() const;
};