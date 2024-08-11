/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeTicketData.h"
#include "ticketSourceProvider.h"
#include "ticketGlobalSource.h"

IMPLEMENT_RTTI_ENUM( EBehTreeTicketSourceProviderType );
IMPLEMENT_ENGINE_CLASS( CBehTreeTicketData )

//////////////////////////////////////////////////////////////////////////
// CBehTreeTicketData::CInitializer
//////////////////////////////////////////////////////////////////////////
CBehTreeTicketData::CInitializer::CInitializer( CBehTreeInstance* owner, CName ticketName, EBehTreeTicketSourceProviderType sourceProvider )
	: Super( ticketName )
	, m_owner( owner )
	, m_sourceProvider( sourceProvider )
{
}

void CBehTreeTicketData::CInitializer::InitializeItem( CAIStorageItem& item ) const
{
	CBehTreeTicketData* ticketData = static_cast< CBehTreeTicketData* >( item.Item() );

	ticketData->m_owner				= CR4BehTreeInstance::Get( m_owner );
	ticketData->m_ticketName		= m_storageName;								// NOTICE: once again notice we are using same name for storage AND ticket source name
	ticketData->m_sourceProvider	= m_sourceProvider;
	ticketData->m_ticket.InitializeSetOwner( m_owner->GetActor() );
}
IRTTIType* CBehTreeTicketData::CInitializer::GetItemType() const
{
	return CBehTreeTicketData::GetStaticClass();
}


//////////////////////////////////////////////////////////////////////////
// CBehTreeTicketData
//////////////////////////////////////////////////////////////////////////

CBehTreeTicketData::CBehTreeTicketData()
	: m_owner( NULL )
	, m_ticket( NULL, this )
	, m_lastTicketAquisition( 0.f )
	, m_request( false )												
{
}

CBehTreeTicketData::~CBehTreeTicketData()
{

}

ITicketSourceProvider* CBehTreeTicketData::CombatTicketsSourceProvider() const
{
	return m_owner->GetCombatTargetData();
}

ITicketSourceProvider* CBehTreeTicketData::GlobalTicketsSourceProvider() const
{
	return GR4Game->GetGlabalTicketSourceProvider();
}

CTicketSource* CBehTreeTicketData::UpdateTicketSource() const
{
	// get current ticket source provider - usualy targets combat data component
	ITicketSourceProvider* sourceProvider = GetCurrentProvider();
	if ( !sourceProvider )
	{
		return nullptr;
	}

	return sourceProvider->GetTicketSource( m_ticketName );
}

void CBehTreeTicketData::ManagerPersistantRequest()
{
	ManagerRequest( 1024.f );
}
void CBehTreeTicketData::ManagerRequest( Float validityTime )
{
	m_ticket.MakeRequest( validityTime );
	m_request = true;
}

void CBehTreeTicketData::ManagerClearRequest()
{
	m_ticket.ClearRequest();
	m_request = false;
}

Bool CBehTreeTicketData::CanAquireTicket( Float importance, Uint16 ticketsCount )
{
	CTicketSource* ticketSource = UpdateTicketSource();
	if ( !ticketSource )
	{
		return false;
	}
	return m_ticket.Check( ticketSource, ticketsCount, importance );
}
Bool CBehTreeTicketData::Aquire( Float importance, Uint16 ticketsCount, Bool force )
{
	CTicketSource* ticketSource = UpdateTicketSource();
	if ( !ticketSource )
	{
		return false;
	}
	if ( !force && !m_ticket.Check( ticketSource, ticketsCount, importance ) )
	{
		return false;
	}
	m_lastTicketAquisition = m_owner->GetLocalTime();
	m_ticket.Aquire( ticketSource, ticketsCount, importance );
	return true;
}

void CBehTreeTicketData::FreeTicket()
{
	if ( HasTicket() )
	{
		m_lastTicketAquisition = m_owner->GetLocalTime();
		m_ticket.Free();
	}
}

Float CBehTreeTicketData::GetTimeSinceMyLastAquisition() const
{
	return m_owner->GetLocalTime() - m_lastTicketAquisition;
}
Float CBehTreeTicketData::GetTimeSinceAnyoneLastAquisition() const
{
	CTicketSource* ticketSource = UpdateTicketSource();
	if ( !ticketSource )
	{
		return 0.f;
	}
	return ticketSource->GetTimeSinceLastAquisition();
}
void CBehTreeTicketData::NotifyOfTicketLost()
{
	m_lastTicketAquisition = m_owner->GetLocalTime();
	m_owner->GetActor()->SignalGameplayEvent( CNAME( AI_LostTicket ), m_ticketName );
}
void CBehTreeTicketData::OnTicketLost()
{
	NotifyOfTicketLost();
}

