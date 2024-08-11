#pragma once
#include "flyingCrittersLairEntity.h"


class CFlyingCrittersLairEntityScript : public CFlyingCrittersLairEntity
{
	DECLARE_ENGINE_CLASS( CFlyingCrittersLairEntityScript, CFlyingCrittersLairEntity, 0 );
public:
	CFlyingCrittersLairEntityScript();

	// Script :
	void OnScriptTick( CFlyingSwarmScriptInput & flyingGroupList, Bool isActive, Float deltaTime ) override;
	void OnBoidPointOfInterestReached( Uint32 count, CEntity *const entity, Float deltaTime ) override;

	// accessors
	Uint32 GetPoiCountByType( Boids::PointOfInterestType type )const;

	// script functions
	void funcGetPoiCountByType( CScriptStackFrame& stack, void* result );
	void funcGetSpawnPointArray( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CFlyingCrittersLairEntityScript );
	PARENT_CLASS( CFlyingCrittersLairEntity );
	NATIVE_FUNCTION( "GetPoiCountByType", funcGetPoiCountByType );
	NATIVE_FUNCTION( "GetSpawnPointArray", funcGetSpawnPointArray );
END_CLASS_RTTI();