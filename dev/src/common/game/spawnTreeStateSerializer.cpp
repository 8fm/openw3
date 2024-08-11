/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "spawnTreeStateSerializer.h"

#include "encounter.h"
#include "generalTreeOperations.h"
#include "spawnTree.h"
#include "../core/gameSave.h"


///////////////////////////////////////////////////////////////////////////////
// CSpawnTreeGeneralIterator
///////////////////////////////////////////////////////////////////////////////
struct SSpawnTreeTreeIteratorStackData : public SGeneralTreeIteratorStackData< CSpawnTreeGeneralDescription::Node >
{
public:
	// hold additional instance buffer pointer on stack, as spawn tree can use multiple instance buffer (additional for each included file)
	CSpawnTreeInstance*				m_instance;
};

class CSpawnTreeIteratorData : public TGeneralTreeIteratorData< CSpawnTreeGeneralDescription, SSpawnTreeTreeIteratorStackData >
{
	typedef TGeneralTreeIteratorData< CSpawnTreeGeneralDescription, SSpawnTreeTreeIteratorStackData > Super;
protected:
	CSpawnTreeInstance*				m_currentInstance;
public:
	CSpawnTreeIteratorData( CSpawnTreeGeneralDescription& interface )
		: Super( interface )
		, m_currentInstance( interface.GetRootInstanceBuffer() )				{}

	CSpawnTreeInstance*		GetInstanceData()									{ return m_currentInstance; }

	void					PushStack( Node* node, Uint32 nodeChildrenCount )
	{
		// push stack
		Super::PushStack( node, nodeChildrenCount );
		// push instance buffer data on stack
		StackData& s = m_stack.Back();
		s.m_instance = m_currentInstance;

		// update instance buffer
		CSpawnTreeInstance* newInstance = node->GetInstanceBuffer( m_currentInstance );
		if ( newInstance )
		{
			m_currentInstance = newInstance;
		}
	}
	void					PopStack()
	{
		m_currentInstance = m_stack.Back().m_instance;
		Super::PopStack();
	}
};

class CSpawnTreeGeneralIterator : public TGeneralTreePersistantIterator< CSpawnTreeIteratorData >
{
	typedef TGeneralTreePersistantIterator< CSpawnTreeIteratorData > Super;

public:
	CSpawnTreeGeneralIterator( CSpawnTreeGeneralDescription& def )
		: Super( def )															{}

};


typedef TGeneralTreeSerializer< CSpawnTreeGeneralIterator > SpawnTreeGeneralSerializer;

///////////////////////////////////////////////////////////////////////////////
// CSpawnTreeGeneralDescription
///////////////////////////////////////////////////////////////////////////////
CSpawnTreeGeneralDescription::CSpawnTreeGeneralDescription( CEncounter* encounter )
	: m_rootNode( encounter->GetRootNode() )
	, m_instanceBuffer( encounter->GetEncounterInstanceBuffer() )
{

}
Bool CSpawnTreeGeneralDescription::IsNodeStateSaving( CSpawnTreeGeneralIterator& iterator )
{
	return iterator->IsNodeStateSaving( *iterator.GetInstanceData() );
}
void CSpawnTreeGeneralDescription::SaveNodeState( CSpawnTreeGeneralIterator& iterator, IGameSaver* writer )
{
	iterator->SaveNodeState( *iterator.GetInstanceData(), writer );
}
Bool CSpawnTreeGeneralDescription::LoadNodeState(CSpawnTreeGeneralIterator& iterator, IGameLoader* reader )
{
	return iterator->LoadNodeState( *iterator.GetInstanceData(), reader );
}

///////////////////////////////////////////////////////////////////////////////
// CSpawnTreeStateSerializer
///////////////////////////////////////////////////////////////////////////////
CSpawnTreeStateSerializer::CSpawnTreeStateSerializer( CEncounter* encounter )
	: m_desc( encounter )
	, m_serializedNodes( 0 )
{

}
void CSpawnTreeStateSerializer::Save( IGameSaver* writer )
{
	SpawnTreeGeneralSerializer serializer( m_desc );
	serializer.SaveTreeState( writer, &m_serializedNodes );
}
Bool CSpawnTreeStateSerializer::Load( IGameLoader* reader )
{
	SpawnTreeGeneralSerializer serializer( m_desc );
	return serializer.LoadTreeState( reader, &m_serializedNodes );
}