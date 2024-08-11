/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CSimpleBufferWriter;
class CSimpleBufferReader;

///////////////////////////////////////////////////////////////////////////////
// General tree iterator
///////////////////////////////////////////////////////////////////////////////
// Whole concept of having customizable - yet general implementation of
// tree instance iterator. Its separated into multiple components, as
// this components can be highly customized to eg. hold additional information on
// the iteration stack. At the same time each component comes with default
// implementation, that just works for simple tree implementation.
///////////////////////////////////////////////////////////////////////////////


// base non-templated data
class CGeneralTreeIteratorDummyInterface : public Red::System::NonCopyable
{
protected:
	Uint16					m_currentNodeHash;

public:
	CGeneralTreeIteratorDummyInterface()
		: m_currentNodeHash( 0 )															{}

	void					operator++()													{ ++m_currentNodeHash; }

	Uint16					NodeHash()														{ return m_currentNodeHash; }
};

// customizable stack data (saved iteration state)
template < class Node >
struct SGeneralTreeIteratorStackData
{
	Node*					m_node;
	Uint32					m_childIndex;
	Uint32					m_childrenCount;
};

// customizable iteration data hub-object
template < class TBase, class TStackData >
class TGeneralTreeIteratorData : public CGeneralTreeIteratorDummyInterface
{
protected:
	typedef typename TBase::Node Node;
	typedef TStackData StackData;
	typedef TStaticArray< StackData, TBase::TREE_MAX_DEPTH > Stack;

	TBase&					m_interface;
	Node*					m_node;
	Stack					m_stack;
public:
	typedef TBase TreeInterface;
	TGeneralTreeIteratorData( TBase& treeInstance )
		: m_interface( treeInstance )
		, m_node( treeInstance.GetRootNode() )												{}

	void					PushStack( Node* node, Uint32 nodeChildrenCount );
	void					PopStack()														{ m_stack.PopBackFast(); }
	void					SetNode( Node* node )											{ m_node = node; }

							operator Node*() const											{ return m_node; }
							operator Bool() const											{ return m_node != nullptr || !m_stack.Empty(); }
	Node*					operator->() const												{ return m_node; }
};

// final iterator implementation
template < class TData >
class TGeneralTreePersistantIterator : public TData
{
	typedef typename TData::StackData StackData;
	typedef TData Super;
public:
	typedef typename TData::TreeInterface TreeInterface;
	TGeneralTreePersistantIterator( TreeInterface& treeInstance )
		: Super( treeInstance )																{}

	using					TData::PushStack;
	using					TData::PopStack;
	using					TData::SetNode;

	void					operator++();
};


///////////////////////////////////////////////////////////////////////////////
// General tree serializer
///////////////////////////////////////////////////////////////////////////////
// Based on iterator it iterates over tree nodes and serialize them when
// requested. If iteration requires to use some additional logic - when nodes
// processing requires some additional dynamic data, user can provide custom
// iterator implementation.
///////////////////////////////////////////////////////////////////////////////
template < class TIterator >
class TGeneralTreeSerializer : public Red::System::NonCopyable
{
protected:
	typedef typename TIterator::TreeInterface TBase;

	TBase&						m_imlementation;
public:
	TGeneralTreeSerializer( TBase& implementation )
		: m_imlementation( implementation )													{}

	Bool						IsSavingTreeState();

	void						SaveTreeState( IGameSaver* writer, Uint32* serializedNodesCount = nullptr );
	Bool						LoadTreeState( IGameLoader* reader, Uint32* serializedNodesCount = nullptr );
};

#include "generalTreeOperations.inl"

