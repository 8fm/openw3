#include "build.h"
#include "ticketSystem.h"

#include "../../common/game/behTreeInstance.h"

//////////////////////////////////////////////////////////////////////////
// CTicketSource
//////////////////////////////////////////////////////////////////////////

CTicketSource::CTicketSource( CName name, Int16 ticketsPool )
	: m_name( name )
	, m_basePool( ticketsPool )
	, m_requestedPool( ticketsPool )
	, m_aquiredPool( ticketsPool )
	, m_lastOverrideId( 0 )
	, m_baseMinimalImportance( 100.f )
	, m_minimalImportance( 100.f )
	, m_lastAcquisitionTime( GGame->GetEngineTime() )
{

}
CTicketSource::~CTicketSource()
{
	RED_FATAL_ASSERT( m_queue.Empty() || SIsMainThread(), "Ticket system removal outside of main thread!" );

	for ( auto it = m_queue.Begin(), end = m_queue.End(); it != end; ++it )
	{
		(*it)->TicketSourceDestroyed();
	}
}

void CTicketSource::FixSourceIndexes( Uint32 from, Uint32 to )
{
	for ( Int16 i = Int16(from), n = Int16(to); i <= n; ++i )
	{
		m_queue[ i ]->m_sourceIndex = i;
	}
}

Int16 CTicketSource::CountTickets( Uint32 from, Uint32 to )
{
	Int16 ret = 0;
	for ( Int32 i = from, n = to; i <= n; ++i )
	{
		ret += m_queue[ i ]->m_tickets;
	}
	return ret;
}

void CTicketSource::AddTicket( CTicket* t )
{
	t->m_source = this;
	auto itFind = ::UpperBound( m_queue.Begin(), m_queue.End(), t );
	Uint64 index = itFind - m_queue.Begin();
	m_queue.Insert( static_cast< Uint32 >( index ), t );
	FixSourceIndexes( static_cast< Uint32 >( index ), m_queue.Size()-1 );
	m_requestedPool -= t->m_tickets;
}

void CTicketSource::FixTicketPosition( CTicket* t )
{
	Uint16 prevIndex = t->m_sourceIndex;
	Uint16 newIndex = prevIndex;
	Int32 queueSize = m_queue.Size();
	if ( newIndex > 0 && CompareTickets( m_queue[ newIndex ], m_queue[ newIndex-1 ] ) )
	{
		// ticket moves to front
		do 
		{
			::Swap( m_queue[ newIndex-1 ], m_queue[ newIndex ] );
			--newIndex;
		} while ( newIndex > 0 && CompareTickets(  m_queue[ newIndex ], m_queue[ newIndex-1 ] ) );
		FixSourceIndexes( newIndex, prevIndex );
	}
	else if ( newIndex+1 < queueSize && CompareTickets( m_queue[ newIndex+1 ],  m_queue[ newIndex ] ) )
	{
		// ticket moves to back
		do 
		{
			::Swap( m_queue[ newIndex ], m_queue[ newIndex+1 ] );
			++newIndex;
		} while ( newIndex+1 < queueSize && CompareTickets(  m_queue[ newIndex+1 ], m_queue[ newIndex ] ) );
		FixSourceIndexes( prevIndex, newIndex );
	}
}

void CTicketSource::DisposeTimeoutRequests( Uint32 from, Uint32 to )
{
	Int32 firstChangeIndex = -1;
	for ( Int32 i = from, n = to; i <= n; )
	{
		CTicket* t = m_queue[ i ];
		if ( !t->m_hasTicket && !t->HasValidRequest() )
		{
			t->m_source = NULL;
			m_requestedPool += t->m_tickets;
			m_queue.RemoveAt( i );
			if ( firstChangeIndex < 0 )
			{
				firstChangeIndex = i;
			}
			--n;
		}
		else
		{
			++i;
		}
	}
	if ( firstChangeIndex >= 0 )
	{
		FixSourceIndexes( firstChangeIndex, m_queue.Size()-1 );
	}
}

void CTicketSource::TryToFreeAquiredTickets( Int32 minEffectedIndex )
{
	for ( Int32 i = m_queue.Size() - 1; i >= minEffectedIndex; --i )
	{
		CTicket* ticket = m_queue[ i ];
		if ( !ticket->m_hasTicket || ticket->m_isLocked )
		{
			continue;
		}
		Free( ticket );
		ticket->NoticeTicketLost();
		if ( m_aquiredPool >= 0 )
		{
			break;
		}
	}
}

Int32 CTicketSource::Override( Int16 ticketsCountMod, Float minImportanceMod )
{
	
	if ( minImportanceMod )
	{
		m_minimalImportance += minImportanceMod;
	}

	if ( ticketsCountMod )
	{
		m_basePool += ticketsCountMod;
		m_requestedPool += ticketsCountMod;
		m_aquiredPool += ticketsCountMod;

		if ( m_aquiredPool < 0 && ticketsCountMod < 0 )
		{
			// free up some stuff
			TryToFreeAquiredTickets();
		}
	}
	SOverride o;
	o.m_minimalImportanceMod = minImportanceMod;
	o.m_ticketsCountMod = ticketsCountMod;
	o.m_requestId = ++m_lastOverrideId;
	
	m_overrides.PushBack( o );

	return o.m_requestId;
}

Bool CTicketSource::ClearOverride( Int32 overrideId )
{
	for ( auto it = m_overrides.Begin(), end = m_overrides.End(); it != end; ++it )
	{
		SOverride& o = *it;

		if ( it->m_requestId == overrideId )
		{
			SOverride o = *it;
			m_overrides.EraseFast( it );
			if ( o.m_minimalImportanceMod )
			{
				// recalculate all to prevent floating point precistion problems
				m_minimalImportance = m_baseMinimalImportance;
				for ( auto it = m_overrides.Begin(), end = m_overrides.End(); it != end; ++it )
				{
					m_minimalImportance += it->m_minimalImportanceMod;
				}
			}
			if ( o.m_ticketsCountMod )
			{
				m_basePool -= o.m_ticketsCountMod;
				m_requestedPool -= o.m_ticketsCountMod;
				m_aquiredPool -= o.m_ticketsCountMod;

				if ( m_aquiredPool < 0 && o.m_ticketsCountMod > 0 )
				{
					// free up some stuff
					TryToFreeAquiredTickets();
				}
			}
			
			return true;
		}
		
	}
	return false;
}


void CTicketSource::Aquire( CTicket* t )
{
	if ( !t->m_hasTicket )
	{
		t->m_hasTicket = true;
		m_aquiredPool -= t->m_tickets;

		// free less important tickets
		if ( m_aquiredPool < 0 )
		{
			TryToFreeAquiredTickets( t->m_sourceIndex+1 );
		}
	}
}

void CTicketSource::Free( CTicket* t )
{
	ASSERT( t->m_source == this );
	if ( t->m_hasTicket )
	{
		t->m_hasTicket = false;
		t->m_isLocked = false;
		m_aquiredPool += t->m_tickets;

		m_lastAcquisitionTime = GGame->GetEngineTime();
	}
	m_requestedPool += t->m_tickets;

	Uint16 ticketIndex = t->m_sourceIndex;
	m_queue.RemoveAt( ticketIndex );

	FixSourceIndexes( ticketIndex, m_queue.Size()-1 );

	t->m_source = NULL;
}
Bool CTicketSource::Check( CTicket* t )
{
	if ( t->m_importance < m_minimalImportance )
	{
		return false;
	}
	if ( m_requestedPool < 0 )
	{
		DisposeTimeoutRequests( 0, t->m_sourceIndex-1 );
		DisposeTimeoutRequests( t->m_sourceIndex+1, m_queue.Size()-1 );

		Int16 missingTickets = -m_requestedPool;

		if ( missingTickets <= 0 )
		{
			return true;
		}

		Bool hasLockedTickets = false;
		// check if we can dispose some ticket
		for ( Uint32 i = t->m_sourceIndex+1, n = m_queue.Size(); i < n; ++i )
		{
			CTicket* t = m_queue[ i ];
			if ( t->m_isLocked )
			{
				hasLockedTickets = true;
				continue;
			}
			missingTickets -= t->m_tickets;
		}
		
		if ( missingTickets > 0 )
		{
			// if we have highest priority AND we can indeed dispose every other ticket owner we can own a ticket even if we require more tickets that there are in pool
			if ( hasLockedTickets != false || t->m_sourceIndex > 0 )
			{
				return false;
			}
		}
	}

	return true;
}
Bool CTicketSource::Update( CTicket* t )
{
	ASSERT( t->m_source == this && t->m_hasTicket );
	
	if ( m_requestedPool < 0 && !t->m_isLocked )
	{
		Int16 upperTickets = CountTickets( 0, t->m_sourceIndex );
		if ( m_basePool - upperTickets < 0 )
		{
			Free( t );
			return false;
		}
	}
	return true;
}

void CTicketSource::TicketsCountUpdated( CTicket* t, Int16 diff )
{
	if ( t->m_hasTicket )
	{
		m_aquiredPool -= diff;
	}
	m_requestedPool -= diff;
}
Float CTicketSource::GetTimeSinceLastAquisition()
{
	if ( m_aquiredPool < m_basePool )
	{
		return 0.f;
	}
	return GGame->GetEngineTime() - m_lastAcquisitionTime;
}

void CTicketSource::ForceImmediateImportanceUpdate() const
{
	for ( auto it = m_queue.Begin(), end = m_queue.End(); it != end; ++it )
	{
		CTicket* t = *it;
		CActor* owner = t->GetOwner();
		if ( owner )
		{
			owner->SignalGameplayEvent( CNAME( AI_ForceImmediateTicketImportanceUpdate ), m_name );
		}
	}
}

void CTicketSource::CollectAquiredTicketOwners( TDynArray< THandle< CActor > >& outArray ) const
{
	for ( auto it = m_queue.Begin(), end = m_queue.End(); it != end; ++it )
	{
		CTicket* t = *it;
		if ( t->HasTicket() )
		{
			CActor* owner = t->GetOwner();
			if ( owner )
			{
				outArray.PushBack( owner );
			}
		}
	}
}

Uint32 CTicketSource::GetAcquisitionsCount() const
{
	Uint32 cnt = 0;
	for ( auto it = m_queue.Begin(), end = m_queue.End(); it != end; ++it )
	{
		CTicket* t = *it;
		if ( t->HasTicket() )
		{
			++cnt;
		}
	}
	return cnt;
}


//////////////////////////////////////////////////////////////////////////
// CTicket
//////////////////////////////////////////////////////////////////////////
CTicket::~CTicket()
{
	RED_FATAL_ASSERT( m_source == nullptr || SIsMainThread(), "Ticket system removal outside of main thread!" );
	Free();
}

void CTicket::NoticeTicketLost()
{
	if ( m_listener )
	{
		m_listener->OnTicketLost();
	}
}

void CTicket::SetTicketsCount( Int16 tickets )
{
	if ( m_tickets != tickets )
	{
		if ( m_source )
		{
			m_source->TicketsCountUpdated( this, tickets - m_tickets );
		}
		m_tickets = tickets;
	}
}

void CTicket::SetSource( CTicketSource* source )
{
	if ( m_source != source )
	{
		if ( m_source )
		{
			m_source->Free( this );
		}
		
		m_source = source;
		m_source->AddTicket( this );
	}
}
void CTicket::SetImportance( Float importance )
{
	if ( m_importance != importance )
	{
		m_importance = importance;
		if ( m_source )
		{
			m_source->FixTicketPosition( this );
		}
	}
	
}
void CTicket::Set( CTicketSource* source, Int16 tickets, Float importance )
{
	SetSource( source );
	SetTicketsCount( tickets );
	SetImportance( importance );
}
void CTicket::Lock( Bool b )
{
	if ( m_source && m_isLocked != b )
	{
		m_isLocked = b;
		m_source->FixTicketPosition( this );
	}
}

void CTicket::TicketSourceDestroyed()
{
	m_source = nullptr;
	m_isLocked = false;
	m_sourceIndex = 0xffff;
	m_tickets = 0;
	m_importance = 0.f;
}

void CTicket::MakeRequest( EngineTime requestValidTime )
{
	m_request = true;
	m_requestTimeout = GGame->GetEngineTime() + requestValidTime;
}
void CTicket::ClearRequest()
{
	m_request = false;
}
Bool CTicket::HasValidRequest()
{
	if ( m_request )
	{
		if ( m_requestTimeout >= GGame->GetEngineTime() )
		{
			return true;
		}

		m_request = false;
	}
	return false;
}


