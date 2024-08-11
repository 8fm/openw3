/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../core/gameSave.h"

///////////////////////////////////////////////////////////////////////////////
// TGeneralTreePersistantIterator
///////////////////////////////////////////////////////////////////////////////
template < class TBase, class TStackData >
void TGeneralTreeIteratorData< TBase, TStackData >::PushStack( Node* node, Uint32 nodeChildrenCount )
{
	m_stack.PushBack( StackData() );
	StackData& s = m_stack.Back();
	s.m_node = m_node;
	s.m_childIndex = 1;
	s.m_childrenCount = nodeChildrenCount;
}

template < class TData >
void TGeneralTreePersistantIterator< TData >::operator++()
{
	// TODO: Support null children nodes
	Uint32 nodeChildrenCount = TData::m_node ? TData::m_interface.GetPersistantChildsCount( TData::m_node ) : 0;
	if ( nodeChildrenCount == 0 )
	{
		// go back (looped until we get to non-visited node
		while( true )
		{
			// check if we can go back one level
			if ( TData::m_stack.Empty() )
			{
				TData::m_node = nullptr;
				return;
			}

			StackData& st = TData::m_stack.Back();
			if ( st.m_childIndex < st.m_childrenCount )
			{
				SetNode( TData::m_interface.GetPersistantChild( st.m_node, st.m_childIndex ) );
				//m_node = ;
				++st.m_childIndex;
				break;
			}

			PopStack();
		}
	}
	else
	{
		// go deep
		PushStack( TData::m_node, nodeChildrenCount );
		
		SetNode( TData::m_interface.GetPersistantChild( TData::m_node, 0 ) );
		//m_node = ;
	}
	Super::operator++();
}

///////////////////////////////////////////////////////////////////////////////
// TGeneralTreeSerializer
///////////////////////////////////////////////////////////////////////////////

template < class TIterator >
Bool TGeneralTreeSerializer< TIterator >::IsSavingTreeState()
{
	{
		TIterator it( m_imlementation );

		while ( it )
		{
			if ( m_imlementation.IsNodeStateSaving( it ) )
			{
				return true;
			}

			++it;
		}
	}
	return false;
}

template < class TIterator >
void TGeneralTreeSerializer< TIterator >::SaveTreeState( IGameSaver* writer, Uint32* serializedNodesCount )
{
	Uint16 savedNodesCount = 0;

	{
		TIterator it( m_imlementation );

		while ( it )
		{
			if ( m_imlementation.IsNodeStateSaving( it ) )
			{
				++savedNodesCount;
			}

			++it;
		}
	}

	writer->WriteValue< Uint16 >( CNAME( savedNodesCount ), savedNodesCount );

	if ( savedNodesCount > 0 )
	{
		Uint16 processedNodesCount = 0;

		{
			TIterator it( m_imlementation );

			while ( it )
			{
				if ( m_imlementation.IsNodeStateSaving( it ) )
				{
					Uint16 hash = it.NodeHash();
					writer->WriteValue< Uint16 >( CNAME( hash ), hash );
					m_imlementation.SaveNodeState( it, writer );
					if ( ++processedNodesCount == savedNodesCount )
					{
						break;
					}
				}

				++it;
			}
		}
	}

	if ( serializedNodesCount )
	{
		*serializedNodesCount = savedNodesCount;
	}
}
template < class TIterator >
Bool TGeneralTreeSerializer< TIterator >::LoadTreeState( IGameLoader* reader, Uint32* serializedNodesCount )
{
	Uint16 savedNodesCount = 0;

	reader->ReadValue< Uint16 >( CNAME( savedNodesCount ), savedNodesCount );

	if ( savedNodesCount )
	{
		Uint16 processedNodes = 0;

		Uint16 expectedHash = 0xffff;
		reader->ReadValue< Uint16 >( CNAME( hash ), expectedHash );
		if ( expectedHash == 0xffff )
		{
			return false;
		}

		TIterator it( m_imlementation );

		while ( it )
		{
			if ( it.NodeHash() == expectedHash )
			{
				if ( !m_imlementation.LoadNodeState( it, reader ) )
				{
					return false;
				}

				if ( ++processedNodes == savedNodesCount )
				{
					break;
				}
				expectedHash = 0xffff;
				reader->ReadValue< Uint16 >( CNAME( hash ), expectedHash );
				if ( expectedHash == 0xffff )
				{
					return false;
				}
			}

			++it;
		}

		// final test if we have load everything properly
		if ( processedNodes != savedNodesCount )
		{
			return false;
		}
	}

	if ( serializedNodesCount )
	{
		*serializedNodesCount = savedNodesCount;
	}

	return true;
}
