#pragma once

#include "encounterTypes.h"

class CEncounterCreaturePool
{
protected:

	// List implementation
	struct SListE;

	struct SList : public Red::System::NonCopyable
	{
		typedef Uint16 ListId;
		static const ListId INVALID_LIST_ID = 0xffff;

		RED_INLINE SList( ListId listId = INVALID_LIST_ID )
			: m_next( NULL )
			, m_listId( listId )										{}

		RED_INLINE SList( SList&& l );								// NOTICE: List elemenets are non-copyable, but can use move semantics to be kept in dynarray
		RED_INLINE SList& operator=( SList&& l );

		void DebugValidate()											{ ASSERT( !m_next || m_next->m_prev == this ); }

		SListE*								m_next;
		ListId								m_listId;
	};
	struct SListH : public SList
	{
		RED_INLINE SListH( ListId listId = INVALID_LIST_ID )
			: SList( listId )											{}

		RED_INLINE SListH( SListH&& l )
			: SList( Move( l ) )										{}
		RED_INLINE SListH& operator=( SListH&& l )					{ SList::operator=( Move( l ) ); return *this;}

		void		ListInsert( SListE& e );							// insert new element on the front
	};
	struct SListE : public SList
	{
		RED_INLINE SListE()
			: SList()
			, m_prev( NULL )											{}

		RED_INLINE SListE( SListE&& l );
		RED_INLINE SListE& operator=( SListE&& l );

		void DebugValidate()											{ SList::DebugValidate(); ASSERT( !m_prev || m_prev->m_next == this ); }

		void		ListErase();										// erase element from list its currently on
		SList*								m_prev;
	};
	
public:
	// Entries- that are members of this list
	struct SCreature : public SListE
	{
		enum EFlags
		{
			FLAGS_DEFAULT												= 0,
			FLAG_DESPAWN_SCHEDULED										= FLAG( 0 ),
			FLAG_IS_BEING_STOLEN										= FLAG( 1 ),
			FLAG_IS_IN_PARTY											= FLAG( 2 ),
			FLAG_WAS_ACTIVATED_BY_ENTRY									= FLAG( 3 ),
			FLAG_IS_POOL_REQUESTED										= FLAG( 4 ),
			FLAG_PROCESSING_MARKING										= FLAG( 5 ),
		};
		SCreature()														{}
		SCreature( CActor* actor, Int16 definitionId, Uint8 spawnGroup )
			: m_actor( actor )
			, m_creatureDefId( definitionId )
			, m_lastOwningEntry( SList::INVALID_LIST_ID )
			, m_spawnGroup( spawnGroup )
			, m_stateFlags( FLAGS_DEFAULT )
			, m_despawnersId( SPAWN_TREE_INVALID_DESPAWNER_ID )			{}

		// Move semantics
		SCreature( SCreature&& e )
			: SListE( Move( e ) )
			, m_actor( e.m_actor )
			, m_creatureDefId( e.m_creatureDefId )
			, m_lastOwningEntry( e.m_lastOwningEntry )
			, m_spawnGroup( e.m_spawnGroup )
			, m_stateFlags( e.m_stateFlags )
			, m_despawnersId( e.m_despawnersId )						{}

		RED_INLINE SCreature& operator=( SCreature&& l )				{
																			SListE::operator=( Move( l ) );
																			m_actor = l.m_actor;
																			m_creatureDefId = l.m_creatureDefId;
																			m_lastOwningEntry = l.m_lastOwningEntry;
																			m_spawnGroup = l.m_spawnGroup;
																			m_stateFlags = l.m_stateFlags;
																			m_despawnersId = l.m_despawnersId;
																			
																			return *this;
																		}

		Bool								IsInParty() const			{ return (m_stateFlags & FLAG_IS_IN_PARTY) != 0; }
		
		CActor*								m_actor;
		Int16								m_creatureDefId;
		Int16								m_lastOwningEntry;
		Uint8								m_spawnGroup;
		Uint8								m_stateFlags;
		SpawnTreeDespawnerId				m_despawnersId;
	};

	struct SCreatureComperator
	{
		static RED_INLINE Bool Less( const SCreature& e1, const SCreature& e2 )	{ return e1.m_actor < e2.m_actor; }
		static RED_INLINE Bool Less( const CActor* a, const SCreature& e )		{ return a < e.m_actor; }
		static RED_INLINE Bool Less( const SCreature& e, const CActor* a )		{ return e.m_actor < a; }
	};

	struct Iterator
	{
		Iterator( SListH* h )
			: m_p( h->m_next )											{}
		Iterator( SListE* p )
			: m_p( p )													{}
		SListE*								m_p;

		Bool operator==( const Iterator it ) const						{ return it.m_p == m_p; }
		Bool operator!=( const Iterator it ) const						{ return it.m_p != m_p; }
		Iterator& operator++()											{ m_p = m_p->m_next; return *this; }

		SCreature* operator->() const									{ return static_cast< SCreature* >( m_p ); }
		SCreature& operator*() const									{ return *static_cast< SCreature* >( m_p ); }

		Iterator							Erase();
	};

	struct ConstIterator
	{
		ConstIterator( const SListH* h )
			: m_p( h->m_next )											{}
		ConstIterator( const SListE* p )
			: m_p( p )													{}
		const SListE*						m_p;

		Bool operator==( const ConstIterator it ) const					{ return it.m_p == m_p; }
		Bool operator!=( const ConstIterator it ) const					{ return it.m_p != m_p; }
		ConstIterator& operator++()										{ m_p = m_p->m_next; return *this; }

		const SCreature* operator->() const								{ return static_cast< const SCreature* >( m_p ); }
		const SCreature& operator*() const								{ return *static_cast< const SCreature* >( m_p ); }
	};

	struct SCreatureList : public SListH
	{
		SCreatureList()
			: SListH()													{}
		SCreatureList( ListId listId )
			: SListH( listId )											{}

		SCreatureList( SCreatureList&& l )
			: SListH( Move( l ) )										{}
		SCreatureList& operator=( SCreatureList&& l )					{ SListH::operator=( Move( l ) ); return *this; }

		Bool Empty() const												{ return m_next == NULL; }
		SCreature* First()												{ return static_cast< SCreature* >( m_next ); }

		Iterator Begin()												{ return Iterator( this ); }
		Iterator End()													{ return Iterator( (SListE*)(NULL) ) ; }
		ConstIterator CBegin() const									{ return ConstIterator( this ); }
		ConstIterator CEnd() const										{ return ConstIterator( (SListE*)(NULL) ) ; }
	};

	// Parties support
	class CPartiesManager
	{
	public:
		typedef Int16 PartyIndex;
		struct PartyMember
		{
			CActor*			m_actor;
			CName			m_memberName;
			PartyIndex		m_partyIndex;
		};

		CPartiesManager();
		~CPartiesManager();
		
		typedef TDynArray< PartyMember > Party;

		const Party*	CreateParty( CEncounterCreaturePool& owner, Party&& members );
		void			RemoveFromParty( CEncounterCreaturePool& owner, SCreature& creature );
		Int32			GetPartyIndex( CActor* actor ) const;
		static Int32	GetPartyIndex( CActor* actor, const Party* party );

		const Party*	GetParty( CActor* actor ) const;
		void			Clear();
	protected:
		typedef Uint32 PartyId;
		typedef TArrayMap< CActor*, PartyId > ActorsMap;
		typedef TArrayMap< PartyId, Party > PartiesMap;

		void			DisbandParty( CEncounterCreaturePool& owner, PartiesMap::iterator itParty );


		ActorsMap									m_partyActors;
		PartiesMap									m_parties;
		PartyId										m_nextUniqueId;
	};

	struct PartyIterator
	{
	protected:
		CPartiesManager::Party::const_iterator		m_it;
		CPartiesManager::Party::const_iterator		m_itEnd;
	public:
		PartyIterator()													{}
		PartyIterator( const CPartiesManager::Party& party )
			: m_it( party.Begin() )
			, m_itEnd( party.End() )									{}
		RED_INLINE CName			Name() const						{ return m_it->m_memberName; }
		RED_INLINE CActor*		Actor() const						{ return m_it->m_actor; }

		RED_INLINE void			Next()								{ ++m_it; }

		RED_INLINE operator CActor*()									{ return m_it->m_actor; }
		RED_INLINE operator Bool()									{ return m_it != m_itEnd; }
	};

	typedef TSortedArray< SCreature, SCreatureComperator > CreatureArray;

protected:
	CreatureArray									m_creatures;
	SCreatureList									m_detachedCreaturesPool;
	SCreatureList::ListId							m_nextFreeListId;
	CPartiesManager									m_parties;

public:
	typedef CPartiesManager::Party Party;
	typedef CPartiesManager::PartyMember PartyMember;

	CEncounterCreaturePool();
	~CEncounterCreaturePool();

	CreatureArray&			GetCreatures()								{ return m_creatures; }
	Uint32					GetCreaturesCount()							{ return m_creatures.Size(); }
	void					AttachCreatureToList( SCreature& e, SCreatureList& list );
	void					DetachCreature( SCreature& e );
	const SCreatureList&	GetDetachedCreaturesList() const			{ return m_detachedCreaturesPool; }
	SCreatureList&			GetDetachedCreaturesList()					{ return m_detachedCreaturesPool; }
	SCreatureList::ListId	GetUniqueListId()							{ return m_nextFreeListId++; }

	const CPartiesManager&	GetPartiesManager() const					{ return m_parties; }
	CPartiesManager&		GetPartiesManager()							{ return m_parties; }

	SCreature*				AddCreature( CActor* actor, Int16 creatureDefId, SCreatureList& list, Int32 spawnGroup );
	void					RemoveCreature( CActor* actor );
	void					RemoveCreature( SCreature& entry );
	SCreature*				GetCreatureEntry( CActor* actor );

	Uint32					GetCreaturePartySize( CActor* actor );
	CActor*					GetCreaturePartyMember( CActor* actor, CName memberName );
	Bool					GetPartyIterator( CActor* actor, PartyIterator& outIterator );

	void					Clear();
};

RED_INLINE CEncounterCreaturePool::SList::SList( SList&& l )
	: m_next( l.m_next )
	, m_listId( l.m_listId )
{
	l.m_next = NULL;
	if ( m_next )
	{
		m_next->m_prev = this;
	}
	DebugValidate();
}

RED_INLINE CEncounterCreaturePool::SListE::SListE( SListE&& l )
	: SList( Move( l ) )
	, m_prev( l.m_prev )
{
	l.m_prev = NULL;
	if ( m_prev )
	{
		m_prev->m_next = this;
	}
	DebugValidate();
}

RED_INLINE CEncounterCreaturePool::SList& CEncounterCreaturePool::SList::operator=( SList&& l )
{
	if ( m_next )
	{
		m_next->m_prev = NULL;
	}
	m_next = l.m_next;
	l.m_next = NULL;
	m_listId = l.m_listId;
	if ( m_next )
	{
		m_next->m_prev = this;
	}
	DebugValidate();

	return *this;
}

RED_INLINE CEncounterCreaturePool::SListE& CEncounterCreaturePool::SListE::operator=( SListE&& l )
{
	SList::operator=( Move( l ) );

	if ( m_prev )
	{
		m_prev->m_next = NULL;
	}
	m_prev = l.m_prev;
	l.m_prev = NULL;
	if ( m_prev )
	{
		m_prev->m_next = this;
	}

	DebugValidate();

	return *this;
}
