#pragma once

#include "../physics/compiledCollision.h"

class ICollisionContent
{
public:
	virtual CompiledCollisionPtr CompileCollision( CObject* parent ) const = 0;
};
