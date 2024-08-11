#include "build.h"
#include "encounterCreaturePool.h"


//////////////////////////////////////////////////////////////
// CEncounterCreaturePool::SList
//////////////////////////////////////////////////////////////
void CEncounterCreaturePool::SListH::ListInsert( SListE& e )
{
	ASSERT( !e.m_next && !e.m_prev );
	if ( m_next )
	{
		m_next->m_prev = &e;
	}
	e.m_prev = this;
	e.m_next = m_next;
	e.m_listId = m_listId;
	m_next = &e;
	DebugValidate();
}

void CEncounterCreaturePool::SListE::ListErase()
{
	DebugValidate();
	if ( m_prev )
	{
		m_prev->m_next = m_next;
	}
	if ( m_next )
	{
		m_next->m_prev = m_prev;
	}
	m_next = NULL;
	m_prev = NULL;
	m_listId = INVALID_LIST_ID;
}

CEncounterCreaturePool::Iterator CEncounterCreaturePool::Iterator::Erase()
{
	Iterator ret = m_p->m_next;

	m_p->ListErase();

	return ret;
}
//////////////////////////////////////////////////////////////
// CEncounterCreaturePool::CPartiesManager
//////////////////////////////////////////////////////////////
CEncounterCreaturePool::CPartiesManager::CPartiesManager()
	: m_nextUniqueId( 0 )
{
}
CEncounterCreaturePool::CPartiesManager::~CPartiesManager()
{
	ASSERT( m_parties.Empty() && m_partyActors.Empty() );
}

void CEncounterCreaturePool::CPartiesManager::DisbandParty( CEncounterCreaturePool& owner, PartiesMap::iterator itParty )
{
	Party& party = itParty->m_second;
	for ( Uint32 i = 0, n = party.Size(); i != n; ++i )
	{
		CActor* actor = party[ i ].m_actor;
		CEncounterCreaturePool::SCreature* entry = owner.GetCreatureEntry( actor );
		entry->m_stateFlags &= ~SCreature::FLAG_IS_IN_PARTY;
	}
	m_parties.Erase( itParty );
}

const CEncounterCreaturePool::CPartiesManager::Party* CEncounterCreaturePool::CPartiesManager::CreateParty( CEncounterCreaturePool& owner, Party&& members )
{
	PartyId id = ++m_nextUniqueId;
	for ( Uint32 i = 0, n = members.Size(); i != n; ++i )
	{
		CActor* actor = members[ i ].m_actor;
		SCreature* entry = owner.GetCreatureEntry( actor );
		// assume entry != NULL
		entry->m_stateFlags |= SCreature::FLAG_IS_IN_PARTY;
		m_partyActors.Insert( members[ i ].m_actor, id );
	}
	auto itInsert = m_parties.Insert( id, Move( members ) );
	return &itInsert->m_second;
}
void CEncounterCreaturePool::CPartiesManager::RemoveFromParty( CEncounterCreaturePool& owner, SCreature& entry )
{
	CActor* actor = entry.m_actor;
	auto itFindActor = m_partyActors.Find( actor );
	if( itFindActor == m_partyActors.End() )
	{
		return;
	}
	PartyId id = itFindActor->m_second;
	m_partyActors.Erase( itFindActor );
	auto itFindParty = m_parties.Find( id );
	ASSERT( itFindParty != m_parties.End() );
	Party& party = itFindParty->m_second;
	for( Uint32 i = 0, n = party.Size(); i != n; ++i )
	{
		if ( party[ i ].m_actor == actor )
		{
			party.RemoveAtFast( i );
			if ( party.Empty() )
			{
				m_parties.Erase( itFindParty );
			}
			entry.m_stateFlags &= ~SCreature::FLAG_IS_IN_PARTY;

			
			return;
		}
	}
}
Int32 CEncounterCreaturePool::CPartiesManager::GetPartyIndex( CActor* actor ) const
{
	auto itFindActor = m_partyActors.Find( actor );
	if( itFindActor == m_partyActors.End() )
	{
		return -1;
	}
	PartyId id = itFindActor->m_second;
	auto itFindParty = m_parties.Find( id );
	if ( itFindParty == m_parties.End() )
	{
		return -1;
	}
	const auto& party = itFindParty->m_second;
	for ( auto it = party.Begin(), end = party.End(); it != end; ++it )
	{
		if ( it->m_actor == actor )
		{
			return it->m_partyIndex;
		}
	}
	return -1;
}
Int32 CEncounterCreaturePool::CPartiesManager::GetPartyIndex( CActor* actor, const Party* party )
{
	for ( auto it = party->Begin(), end = party->End(); it != end; ++it )
	{
		if ( it->m_actor == actor )
		{
			return it->m_partyIndex;
		}
	}
	return -1;
}
const CEncounterCreaturePool::CPartiesManager::Party* CEncounterCreaturePool::CPartiesManager::GetParty( CActor* actor ) const
{
	auto itFindActor = m_partyActors.Find( actor );
	if( itFindActor == m_partyActors.End() )
	{
		return NULL;
	}
	PartyId id = itFindActor->m_second;
	auto itFindParty = m_parties.Find( id );
	ASSERT( itFindParty != m_parties.End() );
	return &itFindParty->m_second;
}
void CEncounterCreaturePool::CPartiesManager::Clear()
{
	m_parties.ClearFast();
	m_partyActors.ClearFast();
	m_nextUniqueId = 0;
}

//////////////////////////////////////////////////////////////
// CEncounterCreaturePool
//////////////////////////////////////////////////////////////
CEncounterCreaturePool::CEncounterCreaturePool()
	: m_detachedCreaturesPool( 0 )
	, m_nextFreeListId( 1 )
{
}
CEncounterCreaturePool::~CEncounterCreaturePool()
{

}

void CEncounterCreaturePool::AttachCreatureToList( SCreature& e, SCreatureList& list )
{
	e.ListErase();
	list.ListInsert( e );
}
void CEncounterCreaturePool::DetachCreature( SCreature& e )
{
	e.ListErase();
	m_detachedCreaturesPool.ListInsert( e );
}

CEncounterCreaturePool::SCreature* CEncounterCreaturePool::AddCreature( CActor* actor, Int16 creatureDefId, SCreatureList& l, Int32 spawnGroup )
{
	// HACK! Way to bypas memory realocation on m_creatures grow
	if ( m_creatures.Capacity() <= m_creatures.Size() )
	{		
		if ( m_creatures.Size() > 512 )
		{
			// Special (rare) case with dynamic memory allocation
			TDynArray< SListE > tmpArray;

			tmpArray.Resize( m_creatures.Size() );
			for ( Uint32 i = 0, n = m_creatures.Size(); i != n; ++i )
			{
				tmpArray[ i ] = Move( m_creatures[ i ] );
			}

			m_creatures.Reserve( Max( 10U, m_creatures.Size() * 2 ) );

			for ( Uint32 i = 0, n = m_creatures.Size(); i != n; ++i )
			{
				(SListE&)(m_creatures[ i ]) = Move( tmpArray[ i ] );
			}
		}
		else
		{
			TStaticArray< SListE, 512 > tmpArray;

			tmpArray.Resize( m_creatures.Size() );
			for ( Uint32 i = 0, n = m_creatures.Size(); i != n; ++i )
			{
				tmpArray[ i ] = Move( m_creatures[ i ] );
			}

			m_creatures.Reserve( Max( 10U, m_creatures.Size() * 2 ) );

			for ( Uint32 i = 0, n = m_creatures.Size(); i != n; ++i )
			{
				(SListE&)(m_creatures[ i ]) = Move( tmpArray[ i ] );
			}
		}
	}

	auto it = m_creatures.Insert( Move( SCreature( actor, creatureDefId, spawnGroup ) ) );
	l.ListInsert( *it );
	return &(*it);
}
void CEncounterCreaturePool::RemoveCreature( CActor* actor )
{
	auto itFind = m_creatures.Find( actor );
	if ( itFind != m_creatures.End() )
	{
		RemoveCreature( *itFind );
	}
}
void CEncounterCreaturePool::RemoveCreature( SCreature& entry )
{
	ASSERT( (entry.m_stateFlags & SCreature::FLAG_IS_IN_PARTY) == 0 );
	entry.ListErase();

	m_creatures.Erase( &entry );
}
CEncounterCreaturePool::SCreature* CEncounterCreaturePool::GetCreatureEntry( CActor* actor )
{
	auto itFind = m_creatures.Find( actor );
	if ( itFind == m_creatures.End() )
	{
		return NULL;
	}
	return &(*itFind);
}

Uint32 CEncounterCreaturePool::GetCreaturePartySize( CActor* actor )
{
	const auto* party = m_parties.GetParty( actor );
	if ( !party )
	{
		return 1;
	}
	return party->Size();
}
CActor* CEncounterCreaturePool::GetCreaturePartyMember( CActor* actor, CName memberName )
{
	const auto* party = m_parties.GetParty( actor );
	if ( !party )
	{
		return NULL;
	}
	for ( auto it = party->Begin(), end = party->End(); it != end; ++it )
	{
		if ( it->m_memberName == memberName )
		{
			return it->m_actor;
		}
	}
	return NULL;
}

Bool CEncounterCreaturePool::GetPartyIterator( CActor* actor, PartyIterator& outIterator )
{
	auto* party = m_parties.GetParty( actor );
	if ( !party )
	{
		return false;
	}
	outIterator = PartyIterator( *party );
	return true;
}

void CEncounterCreaturePool::Clear()
{
	m_creatures.ClearFast();
	m_detachedCreaturesPool.m_next = NULL;
	m_nextFreeListId = 1;
	m_parties.Clear();
}