/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/engine/physicsBatchQueryManager.h"

typedef CPhysicsBatchQueryManager::SQueryId VisibilityQueryId;

class CNewNpcSensesManager
{
public:

	enum EVisibilityQueryState
	{
		QS_NotFound, // not found or timed out
		QS_NotReady,
		QS_True,
		QS_False,
	};

	CNewNpcSensesManager();

	VisibilityQueryId SubmitQuery( CEntity* caster, const Vector& startPos, const Vector& endPos );
	EVisibilityQueryState GetQueryState( const VisibilityQueryId& queryId );
#ifndef RED_FINAL_BUILD
	Bool GetQueryDebugData( const VisibilityQueryId& queryId, SRaycastDebugData& debugData ) const;
#endif

private:

	static CPhysicsEngine::CollisionMask	LOS_COLLISION_MASK;
	static Uint16							LOS_RAYCAST_FLAGS;
};