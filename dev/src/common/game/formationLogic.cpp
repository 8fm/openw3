/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "formationLogic.h"

#include "aiFormationData.h"
#include "formation.h"
#include "formationMemberData.h"


IMPLEMENT_ENGINE_CLASS( IFormationLogic )

///////////////////////////////////////////////////////////////////////////////
// IFormationLogic
///////////////////////////////////////////////////////////////////////////////
CClass* IFormationLogic::GetFormationLeaderType() const
{
	return CFormationLeaderData::GetStaticClass();
}
void IFormationLogic::SetupLeaderData( CFormationLeaderData* leaderData, CActor* leader ) const
{
	leaderData->Initialize( this, leader );
}
CFormationMemberData* IFormationLogic::SpawnMemberData( CActor* actor ) const
{
	return new CFormationMemberData( actor );
}
void IFormationLogic::SetupMemberData( CFormationMemberData* memberData ) const
{
	memberData->InitializeMinCatchUpDistance( m_minCatchupDistance );
}
CFormationMemberData* IFormationLogic::CreateMemberData( CActor* actor ) const
{
	CFormationMemberData* member = SpawnMemberData( actor );
	SetupMemberData( member );
	return member;
}


CFormation* IFormationLogic::GetFormation() const
{
	return Cast< CFormation >( GetParent() );
}

