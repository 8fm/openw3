/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "aiFormationData.h"

#include "behTreeMachine.h"
#include "behTreeInstance.h"
#include "formation.h"
#include "formationLogic.h"

IMPLEMENT_ENGINE_CLASS( CAIFormationData )


////////////////////////////////////////////////////////////////////////
// CFormationLeaderDataPtr::CInitializer
////////////////////////////////////////////////////////////////////////
CFormationLeaderDataPtr::CInitializer::CInitializer( IFormationLogic* logic, CActor* leader, CName uniqueName )
	: CAIStorageItem::CNamedInitializer( uniqueName )
	, m_logic( logic )
	, m_leader( leader )
{

}
void CFormationLeaderDataPtr::CInitializer::InitializeItem( CAIStorageItem& item ) const
{
	CFormationLeaderData* data = static_cast< CFormationLeaderData* >( item.Item() );
	m_logic->SetupLeaderData( data, m_leader );
}
IRTTIType* CFormationLeaderDataPtr::CInitializer::GetItemType() const
{
	return m_logic->GetFormationLeaderType();
}

////////////////////////////////////////////////////////////////////////
// CFormationLeaderDataPtr
////////////////////////////////////////////////////////////////////////
Bool CFormationLeaderDataPtr::Setup( CAIStorage* aiStorage, CFormation* formation, CActor* leader )
{
	IFormationLogic* logic = formation->GetFormationLogic();
	if ( !logic )
	{
		return false;
	}
	CInitializer initializer( logic, leader, formation->GetUniqueName() );

	Super::operator=( Super( initializer, aiStorage ) );
	return true;
}

////////////////////////////////////////////////////////////////////////
// CAIFormationData
////////////////////////////////////////////////////////////////////////
Bool CAIFormationData::Test( CFormation* formation, CActor* leader )
{
	CBehTreeMachine* leaderBehTreeMachine = leader->GetBehTreeMachine();
	if ( !leaderBehTreeMachine )
	{
		return false;
	}

	CBehTreeInstance* leaderAI = leaderBehTreeMachine->GetBehTreeInstance();
	// test if this guy have
	CAIStorageItem* item = leaderAI->GetItem( formation->GetUniqueName() );
	if ( !item )
	{
		return false;
	}
	IFormationLogic* logic = formation->GetFormationLogic();
	if ( !logic )
	{
		return false;
	}
	return item->SupportType( logic->GetFormationLeaderType() );
}
Bool CAIFormationData::Setup( CFormation* formation, CActor* leader, CActor* member, Bool force )
{
	Clear();

	m_formation = formation;
	if ( !m_formation )
	{
		return false;
	}


	CBehTreeMachine* leaderBehTreeMachine = leader->GetBehTreeMachine();
	if ( !leaderBehTreeMachine )
	{
		return false;
	}

	IFormationLogic* formationLogic = formation->GetFormationLogic();
	if ( !formationLogic )
	{
		return false;
	}
	CBehTreeInstance* leaderAI = leaderBehTreeMachine->GetBehTreeInstance();

	if ( force )
	{
		CFormationLeaderDataPtr::CInitializer initializer( formationLogic, leader, m_formation->GetUniqueName() );

		m_leaderData = CFormationLeaderDataExternalPtr( initializer, leaderAI );
	}
	else
	{
		m_leaderData = CFormationLeaderDataExternalPtr( formation->GetUniqueName(), formationLogic->GetFormationLeaderType(), leaderAI );
	}
	
	if ( !m_leaderData )
	{
		return false;
	}
	m_memberData = m_leaderData->RegisterMember( member );
	return true;
}

void CAIFormationData::Clear()
{
	CFormationLeaderData* leaderData = m_leaderData;
	if ( leaderData )
	{
		leaderData->UnregisterMember( m_memberData->GetOwner() );
		m_leaderData.Clear();
	}
	m_memberData.Clear();
	m_formation = NULL;
}
void CAIFormationData::Update( CBehTreeInstance* ai )
{
	CFormationMemberData* memberData = m_memberData.Get();
	if ( memberData )
	{
		memberData->Update();
	}
}

void CAIFormationData::OnDetached()
{
	Clear();
}

////////////////////////////////////////////////////////////////////////
// CAIFormationDataPtr
////////////////////////////////////////////////////////////////////////
CName CAIFormationDataPtr::CInitializer::GetItemName() const
{
	return CNAME( CURRENT_FORMATION );
}
IRTTIType* CAIFormationDataPtr::CInitializer::GetItemType() const
{
	return CAIFormationData::GetStaticClass();
}


CAIFormationDataPtr::CAIFormationDataPtr( CAIStorage* storage )
	: Super( CInitializer(), storage )
{

}