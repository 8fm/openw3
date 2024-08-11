#include "build.h"
#include "spawnCondition.h"

#include "encounter.h"

IMPLEMENT_ENGINE_CLASS( ISpawnCondition );
IMPLEMENT_ENGINE_CLASS( ISpawnScriptCondition );


//////////////////////////////////////////////////////////////
/// ENCOUNTER CONDITION
//////////////////////////////////////////////////////////////

Bool ISpawnCondition::Test( CSpawnTreeInstance& instance )
{
	return true;
}

Bool ISpawnScriptCondition::Test( CSpawnTreeInstance& instance )
{
	Bool res = true;
	THandle< CEncounter > encounter = instance.GetEncounter();
	CallFunctionRet< Bool >( this, CNAME( TestCondition ), encounter, res );
	return res;
}

