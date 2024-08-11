#include "build.h"
#include "ticketSystemConfiguration.h"

#include "ticketSystem.h"

IMPLEMENT_ENGINE_CLASS( CTicketSourceConfiguration );
IMPLEMENT_ENGINE_CLASS( CTicketConfigurationParam );
IMPLEMENT_ENGINE_CLASS( CTicketsDefaultConfiguration );

////////////////////////////////////////////////////////////////////////////
// CTicketSourceConfiguration
////////////////////////////////////////////////////////////////////////////

void CTicketSourceConfiguration::SetupTicketSource( CTicketSource& outSource )
{
	ASSERT( outSource.m_queue.Empty(), TXT("CTicketSourceConfiguration::SetupTicketSource should be run only on ticket source initialization code.") );
	outSource.m_baseMinimalImportance = m_minimalImportance;
	outSource.m_minimalImportance = m_minimalImportance;
	outSource.m_requestedPool = m_ticketsPoolSize;
	outSource.m_aquiredPool = m_ticketsPoolSize;
	outSource.m_basePool = m_ticketsPoolSize;
}

////////////////////////////////////////////////////////////////////////////
// CTicketingConfiguration
////////////////////////////////////////////////////////////////////////////

namespace
{
	struct TicketSortOrder
	{
		Bool operator()( const CTicketSourceConfiguration& t1, const CTicketSourceConfiguration& t2 ) const
		{
			return t1.m_name < t2.m_name; 
		}
		Bool operator()( const CTicketSourceConfiguration& t1, CName t2 ) const
		{
			return t1.m_name < t2; 
		}
	};
};


CTicketSourceConfiguration* CTicketingConfiguration::GetConfiguration( CName name )
{
	auto itFind = ::LowerBound( m_tickets.Begin(), m_tickets.End(), name, ::TicketSortOrder() );
	if ( itFind != m_tickets.End() && itFind->m_name == name )
	{
		return &(*itFind);
	}
	return NULL;
}

void CTicketingConfiguration::PostEditionSort()
{
	::Sort( m_tickets.Begin(), m_tickets.End(), ::TicketSortOrder() );
}

////////////////////////////////////////////////////////////////////////////
// CTicketConfigurationParam
////////////////////////////////////////////////////////////////////////////

void CTicketConfigurationParam::OnPreSave()
{
	TBaseClass::OnPreSave();

	PostEditionSort();
}


////////////////////////////////////////////////////////////////////////////
// CTicketsDefaultConfiguration
////////////////////////////////////////////////////////////////////////////

void CTicketsDefaultConfiguration::Initialize()
{
	TBaseClass::OnPreSave();

	CallFunction( this, CNAME( Init ) );

	PostEditionSort();
}

void CTicketsDefaultConfiguration::funcSetupTicketSource( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( Int32, ticketPoolSize, 1 );
	GET_PARAMETER( Float, minimalImportance, 0.f );
	FINISH_PARAMETERS;
	
	if ( !name.Empty() )
	{
		CTicketSourceConfiguration conf;
		conf.m_name = name;
		conf.m_ticketsPoolSize = Uint16( ticketPoolSize );
		conf.m_minimalImportance = minimalImportance;
		m_tickets.PushBack( conf );
	}
}