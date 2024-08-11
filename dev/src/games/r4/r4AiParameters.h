#pragma once

#include "../../common/game/aiParameters.h"
class IRiderActionParameters : public CAIParameters
{
	typedef CAIParameters TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( IRiderActionParameters );
public:
	IRiderActionParameters() {}
};

BEGIN_CLASS_RTTI( IRiderActionParameters )
	PARENT_CLASS( CAIParameters )
END_CLASS_RTTI()

class IRiderActionTree : public CAITree
{
	typedef CAITree TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( IRiderActionTree );
public:
	IRiderActionTree() {}
};

BEGIN_CLASS_RTTI( IRiderActionTree )
	PARENT_CLASS( CAITree )
END_CLASS_RTTI()