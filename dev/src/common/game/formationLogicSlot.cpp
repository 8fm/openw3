/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "formationLogicSlot.h"

#include "formationMemberDataSlot.h"

IMPLEMENT_ENGINE_CLASS( CSlotFormationLogic )

///////////////////////////////////////////////////////////////////////////////
// CSlotFormationLogic
///////////////////////////////////////////////////////////////////////////////
CClass* CSlotFormationLogic::GetFormationLeaderType() const
{
	return CAISlotFormationLeaderData::GetStaticClass();
}
CFormationMemberData* CSlotFormationLogic::SpawnMemberData( CActor* actor ) const
{
	return new CSlotFormationMemberData( actor );
}

