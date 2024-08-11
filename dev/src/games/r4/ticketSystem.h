#pragma once

class CTicket;

class CTicketSource
{
	friend class CTicket;															// all implementation is done on CTicket level
	friend class CTicketSourceConfiguration;

protected:
	struct SOverride
	{
		Float						m_minimalImportanceMod;
		Int16						m_ticketsCountMod;
		Int16						m_requestId;
	};

	typedef TDynArray< CTicket* > TicketQueue;
	typedef TDynArray< SOverride > OverridesList;



	CName						m_name;

	Int16						m_basePool;
	Int16						m_requestedPool;
	Int16						m_aquiredPool;
	Int16						m_lastOverrideId;

	Float						m_baseMinimalImportance;
	Float						m_minimalImportance;
	TicketQueue					m_queue;
	OverridesList				m_overrides;
	EngineTime					m_lastAcquisitionTime;

	RED_INLINE static Bool		CompareTickets( CTicket* t1, CTicket* t2 );

	void						AddTicket( CTicket* t );
	void						FixSourceIndexes( Uint32 from, Uint32 to );
	Int16						CountTickets( Uint32 from, Uint32 to );
	void						FixTicketPosition( CTicket* t );
	void						DisposeTimeoutRequests( Uint32 from, Uint32 to );
	void						TryToFreeAquiredTickets( Int32 minEffectedIndex = 0 );

	void						ApplyOverride( Int16 ticketsCountMod, Float minImportanceMod );
public:
	CTicketSource( CName name, Int16 ticketsPool = 100 );
	~CTicketSource();

	void						Aquire( CTicket* t );
	void						Free( CTicket* t );
	Bool						Check( CTicket* t );
	Bool						Update( CTicket* t );

	void						TicketsCountUpdated( CTicket* t, Int16 diff );

	// script interface
	Int32						Override( Int16 ticketsCountMod, Float minImportanceMod = 0.f );
	Bool						ClearOverride( Int32 overrideId );

	void						DisposeTimeoutRequests()					{ DisposeTimeoutRequests( 0, m_queue.Size()-1 ); }

	Float						GetTimeSinceLastAquisition();
	void						ForceImmediateImportanceUpdate() const;											// NOTICE: Computation heavy!
	void						CollectAquiredTicketOwners( TDynArray< THandle< CActor > >& outArray ) const;

	// Getters used by debugging interface
	CName						GetName() const								{ return m_name; }
	Int16						GetBasePool() const							{ return m_basePool; }
	Int16						GetRequests() const							{ return m_basePool - m_requestedPool; }
	Int16						GetAcquired() const							{ return m_basePool - m_aquiredPool; }
	Float						GetMinimalImportance() const				{ return m_minimalImportance; }
	Uint32						GetAcquisitionsCount() const;
	Uint32						GetQueuedRequestsCount() const				{ return m_queue.Size(); }
	Uint32						GetOverridesCount() const					{ return m_overrides.Size(); }

};

////////////////////////////////////////////////////////////////////////////
// This class represents all ticketing system interface
class CTicket
{
	friend class CTicketSource;
public:
	class IListener
	{
	public:
		virtual void OnTicketLost() = 0;
	};
protected:
	CActor*						m_owner;
	IListener*					m_listener;

	CTicketSource*				m_source;
	Uint16						m_sourceIndex;
	Int16						m_tickets;
	Bool						m_isLocked;
	Bool						m_hasTicket;
	Bool						m_request;
	Float						m_importance;
	EngineTime					m_requestTimeout;

	void						NoticeTicketLost();
	void						SetTicketsCount( Int16 tickets );
	void						SetSource( CTicketSource* source );
	void						Set( CTicketSource* source, Int16 tickets, Float importance );
public:
	CTicket( CActor* owner, IListener* listener = NULL )
		: m_owner( owner )
		, m_listener( listener )
		, m_source( NULL )
		, m_sourceIndex( 0 )
		, m_tickets( 0 )
		, m_isLocked( false )
		, m_importance( 0.f )														{}
	~CTicket();

	// Function called after default constructor to setup basics
	void						SetListener( IListener* owner )						{ m_listener = owner; }

	// NOTICE: Aquire, Free, and UpdateImportance are very low level implementations. So please think twice before modifying them.
	RED_INLINE void				Aquire( CTicketSource* source, Int16 tickets, Float importance ) { Set( source, tickets, importance ); source->Aquire( this ); }
	RED_INLINE void				Free()												{ if ( m_source ) { m_source->Free( this ); } }
	void						SetImportance( Float importance );
	RED_INLINE Bool				UpdateImportance( Float importance )				{ if ( m_source ) { SetImportance( importance ); return m_hasTicket ? m_source->Update( this ) : false; } return false; }
	void						MakeRequest( EngineTime requestValidTime );
	void						ClearRequest();
	void						Lock( Bool b );

	Bool						Check( CTicketSource* source, Int16 tickets, Float importance ) { Set( source, tickets, importance ); return source->Check( this ); }

	Bool						IsFree() const										{ return !m_hasTicket; }
	Bool						HasTicket() const									{ return m_hasTicket; }
	Bool						HasValidRequest();

	RED_INLINE void				Debug_ConsistencyCheck() const;

	CTicketSource*				GetCurrentSource() const							{ return m_source; }
	CActor*						GetOwner() const									{ return m_owner; }
	void						InitializeSetOwner( CActor* actor )					{ m_owner = actor; }
	void						TicketSourceDestroyed();
};

void CTicket::Debug_ConsistencyCheck() const
{
	ASSERT( !m_source
		|| ( m_source->m_queue[ m_sourceIndex ] == this
		&& ( m_sourceIndex == 0 || m_source->m_queue[ m_sourceIndex - 1 ]->m_importance >= m_importance )
		&& ( m_sourceIndex == m_source->m_queue.Size()-1 || m_source->m_queue[ m_sourceIndex + 1 ]->m_importance <= m_importance )
		) );
}

RED_INLINE Bool CTicketSource::CompareTickets( CTicket* t1, CTicket* t2 )
{
	return
		t1->m_importance > t2->m_importance;
}