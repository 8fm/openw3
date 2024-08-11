/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "r6npc.h"
#include "../../common/game/movingPhysicalAgentComponent.h"
#include "../../common/game/movableRepresentationPathAgent.h"
#include "../../common/engine/pathlibWorld.h"

IMPLEMENT_ENGINE_CLASS( CR6NPC );

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
CR6NPC::CR6NPC()
{
}

//---------------------------------------------------------------------------------------------------------------------------------
void CR6NPC::funcEnableCharacterControllerPhysics( CScriptStackFrame& stack, void* result )
{
	// TODO: this function is copied from CR6Player. Should be put into common base class at some point.

	GET_PARAMETER( Bool, enable, true );
	FINISH_PARAMETERS;

	CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent > ( GetMovingAgentComponent() );
	if ( mpac )
	{
		mpac->SetGravity( enable );
		mpac->EnableStaticCollisions( enable );
		mpac->EnableCharacterCollisions( enable );
		mpac->EnableDynamicCollisions( enable );
		mpac->InvalidatePhysicsCache();
	}
}

void CR6NPC::funcGetClosestWalkableSpot( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, _targetPos, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;

	CMovingAgentComponent* mac = GetMovingAgentComponent();
	CPathAgent* pathAgent  = ( mac != NULL ? mac->GetPathAgent() : NULL );
	CPathLibWorld* pathWorld = ( pathAgent != NULL ? pathAgent->GetPathLib() : NULL );
	Vector3 resultPosition = _targetPos;

	// try to find the closest walkable position on the nav mesh
	if( pathWorld && pathAgent )
	{
		Box bbox( _targetPos, 10.0f );
		PathLib::AreaId areaId;
		pathWorld->FindClosestWalkableSpotInArea( bbox, _targetPos, pathAgent->GetPersonalSpace(), resultPosition, areaId );
	}

	RETURN_STRUCT( Vector, resultPosition );
}