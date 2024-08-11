#include "build.h"
#include "behTreeWorkData.h"

#include "behTreeInstance.h"
#include "communitySystem.h"

IMPLEMENT_ENGINE_CLASS( CBehTreeWorkData );

const Float CBehTreeWorkData::SPAWN_TO_WORK_AP_DISTANCE = 1.f;

////////////////////////////////////////////////////////////////////////
// CBehTreeWorkData
////////////////////////////////////////////////////////////////////////
CBehTreeWorkData::~CBehTreeWorkData()
{
	FreeReservedAP();
}
void CBehTreeWorkData::FreeReservedAP()
{
	if ( m_reservedAP != ActionPointBadID )
	{
		CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
		if ( communitySystem )
		{
			CActionPointManager* actionPointManager = communitySystem->GetActionPointManager();
			actionPointManager->SetFree( m_reservedAP, CActionPointManager::REASON_SELECTION );
		}
		m_reservedAP = ActionPointBadID;
	}
}
void CBehTreeWorkData::ReserveSelectedAP()
{
	if ( m_selectedAP != ActionPointBadID )
	{
		m_reservedAP = m_selectedAP;
		CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
		if ( communitySystem )
		{
			CActionPointManager* actionPointManager = communitySystem->GetActionPointManager();
			actionPointManager->SetReserved( m_reservedAP, CActionPointManager::REASON_SELECTION, m_owner->GetNPC() );
		}
	}
}
void CBehTreeWorkData::SelectAP( const SActionPointId& apID, CName category, Bool reserveAP )
{
	if ( apID == m_selectedAP )
	{
		return;
	}
	if ( m_selectedAP != ActionPointBadID )
	{
		m_lastAP = m_selectedAP;
	}

	FreeReservedAP();
	
	m_selectedAP = apID;
	m_selectedAPCategory = category;

	if ( reserveAP )
	{
		ReserveSelectedAP();
	}
}

CName CBehTreeWorkData::GetStorageName()
{
	return CNAME( WorkData );
}

void CBehTreeWorkData::StartPerform()
{
	m_isBeingPerformed = true;
	m_isBeingInterrupted = false;
}
void CBehTreeWorkData::StopPerform()
{
	m_isBeingPerformed = false;
	m_workStoppedTime = GCommonGame->GetEngineTime();
}

Bool CBehTreeWorkData::WasJustWorking()
{
	return m_workStoppedTime + 1 > ( Float )GCommonGame->GetEngineTime(); 
}
Bool CBehTreeWorkData::IsTryingToSpawnToWork( CBehTreeInstance* owner )
{
	if ( m_trySpawnToWork )
	{
		if ( m_spawnToWorkTimeLimit >= owner->GetLocalTime()-owner->GetLocalTimeDelta() )
		{
			return true;
		}
		m_trySpawnToWork = false;
	}
	return false;
}
Bool CBehTreeWorkData::IsInImmediateActivation( CBehTreeInstance* owner )
{
	if ( m_tryImmediateActivation )
	{
		if ( m_spawnToWorkTimeLimit >= owner->GetLocalTime()-owner->GetLocalTimeDelta() )
		{
			return true;
		}
		m_tryImmediateActivation = false;
	}
	return false;
}


////////////////////////////////////////////////////////////////////////
// CBehTreeWorkData::CInitializer
////////////////////////////////////////////////////////////////////////
CName CBehTreeWorkData::CInitializer::GetItemName() const
{
	return CBehTreeWorkData::GetStorageName();
}
void CBehTreeWorkData::CInitializer::InitializeItem( CAIStorageItem& item ) const
{
	CBehTreeWorkData* workData = static_cast< CBehTreeWorkData* >( item.Item() );
	workData->m_owner = m_owner;
}
IRTTIType* CBehTreeWorkData::CInitializer::GetItemType() const
{
	return CBehTreeWorkData::GetStaticClass();
}