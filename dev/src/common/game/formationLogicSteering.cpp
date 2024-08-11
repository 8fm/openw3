/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "formationLogicSteering.h"

#include "formationMemberDataSteering.h"

IMPLEMENT_ENGINE_CLASS( CSteeringFormationLogic )

///////////////////////////////////////////////////////////////////////////////
// CSteeringFormationLogic
///////////////////////////////////////////////////////////////////////////////
CClass* CSteeringFormationLogic::GetFormationLeaderType() const
{
	return CAISteeringFormationLeaderData::GetStaticClass();
}
CFormationMemberData* CSteeringFormationLogic::SpawnMemberData( CActor* actor ) const
{
	return new CSteeringFormationMemberData( actor );
}

