/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "formationLogic.h"

class CSteeringFormationLogic : public IFormationLogic
{
	DECLARE_ENGINE_CLASS( CSteeringFormationLogic, IFormationLogic, 0 )
protected:
	CClass*							GetFormationLeaderType() const;
	CFormationMemberData*			SpawnMemberData( CActor* actor ) const;
public:

};

BEGIN_CLASS_RTTI( CSteeringFormationLogic )
	PARENT_CLASS( IFormationLogic )
END_CLASS_RTTI()

