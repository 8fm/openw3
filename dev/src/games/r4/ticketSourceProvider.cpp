#include "build.h"

#include "ticketSourceProvider.h"
#include "ticketSystem.h"
#include "ticketSystemConfiguration.h"

ITicketSourceProvider::~ITicketSourceProvider()
{
	ASSERT( m_ticketSources.Empty() );
}

CTicketSource* ITicketSourceProvider::GetTicketSource( CName name, Bool lazyCreate )
{
	auto it = m_ticketSources.Find( name );
	if ( it != m_ticketSources.End() )
	{
		return it->m_second;
	}
	if ( !lazyCreate )
	{
		return NULL;
	}

	CTicketSource* newTicketSource = new CTicketSource( name );
	m_ticketSources.Insert( name, newTicketSource );
	
	CTicketSourceConfiguration* ticketConf = GetCustomConfiguration( name );

	if ( !ticketConf && GR4Game )
	{
		ticketConf = GR4Game->GetDefaultTicketsConfiguration()->GetConfiguration( name );		
	}

	if ( ticketConf )
	{
		ticketConf->SetupTicketSource( *newTicketSource );		
	}

	return newTicketSource;
}

void ITicketSourceProvider::ClearTickets()
{
	for( auto it = m_ticketSources.Begin(), end = m_ticketSources.End(); it != end; ++it )
	{
		delete it->m_second;
	}
	m_ticketSources.ClearFast();	
}