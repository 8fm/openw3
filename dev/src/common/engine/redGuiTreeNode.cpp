/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiTreeNode.h"
#include "redGuiTreeView.h"
#include "redGuiImage.h"

namespace RedGui
{
	namespace
	{
		const Uint32 GDefaultHeight = 18;
	}

	CRedGuiTreeNode::CRedGuiTreeNode()
		: m_expanded( false )
		, m_selected( false )
		, m_visible( false )
		, m_index( -1 )
		, m_level( -1 )
		, m_backgroundColor( 45, 45, 45, 255 )
		, m_textColor( Color::WHITE )
		, m_text( String::EMPTY )
		, m_treeView( nullptr )
		, m_parentNode( nullptr )
		, m_image( nullptr )
	{
		/* intentionally empty */
	}

	CRedGuiTreeNode::CRedGuiTreeNode( const String& text )
		: m_expanded( false )
		, m_selected( false )
		, m_visible( false )
		, m_index( -1 )
		, m_level( -1 )
		, m_backgroundColor( 45, 45, 45, 255 )
		, m_textColor( Color::WHITE )
		, m_text( text )
		, m_treeView( nullptr )
		, m_parentNode( nullptr )
		, m_image( nullptr )
	{
		/* intentionally empty */
	}

	CRedGuiTreeNode::CRedGuiTreeNode( const String& text, const TDynArray< CRedGuiTreeNode* >& childrenNodes )
		: m_expanded( false )
		, m_selected( false )
		, m_visible( false )
		, m_index( -1 )
		, m_level( -1 )
		, m_backgroundColor( 45, 45, 45, 255 )
		, m_textColor( Color::WHITE )
		, m_text( String::EMPTY )
		, m_treeView( nullptr )
		, m_parentNode( nullptr )
		, m_image( nullptr )
		, m_childrenNodes( childrenNodes )
	{
		/* intentionally empty */
	}

	CRedGuiTreeNode::~CRedGuiTreeNode()
	{
		const Uint32 nodeCount = m_childrenNodes.Size();
		for( Uint32 i=0; i<nodeCount; ++i )
		{
			delete m_childrenNodes[i];
		}
		m_childrenNodes.Clear();

		// delete image
		if( m_image != nullptr )
		{
			m_image->Dispose();
			m_image = nullptr;
		}
	}

	CRedGuiTreeNode* CRedGuiTreeNode::AddNode( const String& name )
	{
		CRedGuiTreeNode* newNode = new CRedGuiTreeNode( name );
		newNode->SetLevel( m_level + 1 );
		newNode->SetParentNode( this );
		newNode->SetTreeView( m_treeView );
		newNode->SetIndex( m_childrenNodes.Size() );
		m_childrenNodes.PushBack( newNode );
		return m_childrenNodes[ m_childrenNodes.Size() -1 ];
	}

	CRedGuiTreeNode* CRedGuiTreeNode::AddNode( CRedGuiTreeNode* rootNode )
	{
		rootNode->SetLevel( m_level + 1 );
		rootNode->SetParentNode( this );
		rootNode->SetTreeView( m_treeView );
		rootNode->SetIndex( m_childrenNodes.Size() );
		m_childrenNodes.PushBack( rootNode );
		return rootNode;
	}

	Uint32 CRedGuiTreeNode::GetLevel() const
	{
		return m_level;
	}

	String CRedGuiTreeNode::GetFullPath() const
	{
		String path = String::EMPTY;
		Char separator = ( m_treeView != nullptr ) ? m_treeView->GetPathSeparator() : '\\';

		for( const CRedGuiTreeNode* node = this; node!=nullptr; node = node->GetParentNode() )
		{
			path = String::Printf( TXT("%s%c%s"), node->GetText().AsChar(), separator, path.AsChar() );
		}

		return path;
	}

	Int32 CRedGuiTreeNode::GetIndex() const
	{
		return m_index;
	}

	String CRedGuiTreeNode::GetText() const
	{
		return m_text;
	}

	void CRedGuiTreeNode::SetText( const String& text )
	{
		m_text = text;
	}

	Bool CRedGuiTreeNode::GetExpanded() const
	{
		return m_expanded;
	}

	void CRedGuiTreeNode::SetExpaned( Bool value )
	{
		m_expanded = value;
	}

	Bool CRedGuiTreeNode::GetSelected() const
	{
		return m_selected;
	}

	void CRedGuiTreeNode::SetSelected( Bool value )
	{
		m_selected = value;
	}

	Bool CRedGuiTreeNode::GetVisible() const
	{
		if( m_parentNode == nullptr || ( m_parentNode != nullptr && m_parentNode->GetExpanded() == true ) )
		{
			return true;
		}

		return false;
	}

	void CRedGuiTreeNode::SetVisible()
	{
		for( CRedGuiTreeNode* node = m_parentNode; node != nullptr; node = node->GetParentNode() )
		{
			node->Expand( true );
		}
	}

	const Color& CRedGuiTreeNode::GetBackgroundColor() const
	{
		return m_backgroundColor;
	}

	void CRedGuiTreeNode::SetBackgroundColor( const Color& color )
	{
		m_backgroundColor = color;
	}

	const Color& CRedGuiTreeNode::GetTextColor() const
	{
		return m_textColor;
	}

	void CRedGuiTreeNode::SetTextColor( const Color& color )
	{
		m_textColor = color;
	}

	CRedGuiTreeNode* CRedGuiTreeNode::GetFirstChildNode() const
	{
		if( m_childrenNodes.Size() > 0 )
		{
			return m_childrenNodes[0];
		}

		return nullptr;
	}

	CRedGuiTreeNode* CRedGuiTreeNode::GetLastChildNode() const
	{
		if( m_childrenNodes.Size() > 0 )
		{
			return m_childrenNodes[m_childrenNodes.Size() - 1];
		}

		return nullptr;
	}

	CRedGuiTreeNode* CRedGuiTreeNode::GetNextNode() const
	{
		Int32 index = GetIndex();
		if( index != -1 )
		{
			if( m_parentNode != nullptr )
			{
				const Int32 nodeCount = (Int32)m_parentNode->GetNodeCount();
				TreeNodeCollection& nodes = m_parentNode->GetChildrenNodes();
				if( index < ( nodeCount - 1 ) )
				{
					return nodes[ index + 1];
				}
				else
				{
					return m_parentNode->GetNextNode();
				}
			}
			else
			{
				if( m_treeView != nullptr )
				{
					const Int32 nodeCount = (Int32)m_treeView->GetRootNodeCount();
					TreeNodeCollection& nodes = m_treeView->GetRootNodes();
					if( index < ( nodeCount - 1 ) )
					{
						return nodes[ index + 1];
					}
				}
			}
		}

		return nullptr;
	}

	CRedGuiTreeNode* CRedGuiTreeNode::GetPreviousNode() const
	{
		Int32 index = GetIndex();
		if( index != -1 )
		{
			if( m_parentNode != nullptr )
			{
				const Uint32 nodeCount = m_parentNode->GetNodeCount();
				TreeNodeCollection& nodes = m_parentNode->GetChildrenNodes();
				if( index > 0 )
				{
					return nodes[ index - 1];
				}
			}
			else
			{
				if( m_treeView != nullptr )
				{
					const Uint32 nodeCount = m_treeView->GetRootNodeCount();
					TreeNodeCollection& nodes = m_treeView->GetRootNodes();
					if( index > 0 )
					{
						return nodes[ index - 1];
					}
				}
			}
		}

		return nullptr;
	}


	CRedGuiTreeNode* CRedGuiTreeNode::GetNextVisibleNode() const
	{
		if( GetNodeCount() > 0 )
		{
			if( GetExpanded() == true )
			{
				return GetFirstChildNode();
			}
		}
		return GetNextNode();
	}

	CRedGuiTreeNode* CRedGuiTreeNode::GetPreviousVisibleNode() const
	{
		CRedGuiTreeNode* previousNode = GetPreviousNode();
		if( previousNode != nullptr )
		{
			if( previousNode->GetExpanded() == true && previousNode->GetNodeCount() > 0 )
			{
				return previousNode->GetLastChildNode();
			}
			return previousNode;
		}

		return GetParentNode();
	}

	CRedGuiTreeNode* CRedGuiTreeNode::GetParentNode() const
	{
		return m_parentNode;
	}

	CRedGuiTreeNode* CRedGuiTreeNode::GetNodeAt( Uint32 index ) const
	{
		if( index < m_childrenNodes.Size() )
		{
			return m_childrenNodes[index];
		}

		return nullptr;
	}

	CRedGuiTreeView* CRedGuiTreeNode::GetTreeView() const
	{
		return m_treeView;
	}

	TDynArray< CRedGuiTreeNode* >& CRedGuiTreeNode::GetChildrenNodes()
	{
		return m_childrenNodes;
	}

	Uint32 CRedGuiTreeNode::GetNodeCount( Bool includeSubTrees /*= false*/ ) const
	{
		Uint32 nodeCount = 0;

		nodeCount = m_childrenNodes.Size();

		if( includeSubTrees == true )
		{
			const Uint32 childNodeCount = m_childrenNodes.Size();
			for( Uint32 i=0; i<childNodeCount; ++i )
			{
				nodeCount += m_childrenNodes[i]->GetNodeCount( true );
			}
		}

		return nodeCount;
	}

	void CRedGuiTreeNode::Toggle()
	{
		if( m_expanded == true )
		{
			Collapse();
		}
		else
		{
			Expand();
		}
	}

	void CRedGuiTreeNode::Expand( Bool ignoreChildren /*= true*/ )
	{
		m_expanded = true;

		if( ignoreChildren == false )
		{
			const Uint32 nodeCount = m_childrenNodes.Size();
			for( Uint32 i=0; i<nodeCount; ++i )
			{
				m_childrenNodes[i]->Expand( false );
			}
		}
	}

	void CRedGuiTreeNode::Collapse( Bool ignoreChildren /*= true */ )
	{
		m_expanded = false;

		if( ignoreChildren == false )
		{
			const Uint32 nodeCount = m_childrenNodes.Size();
			for( Uint32 i=0; i<nodeCount; ++i )
			{
				m_childrenNodes[i]->Collapse( false );
			}
		}
	}

	void CRedGuiTreeNode::Dispose()
	{
		if( m_parentNode != nullptr )
		{
			m_parentNode->RemoveChildNode( this );
		}
		else if( m_treeView != nullptr )
		{
			m_treeView->RemoveChildNode( this );
		}
	}

	void CRedGuiTreeNode::RemoveChildNode( CRedGuiTreeNode* node )
	{
		if( node != nullptr )
		{
			m_childrenNodes.Remove( node );
			delete node;
		}
	}

	CRedGuiImage* CRedGuiTreeNode::GetImage() const
	{
		return m_image;
	}

	void CRedGuiTreeNode::SetImage( const String& path )
	{
		m_image = new CRedGuiImage( 0, 0, GDefaultHeight, GDefaultHeight );
		m_image->SetImage( path );
	}

	void CRedGuiTreeNode::SetLevel( Uint32 value )
	{
		m_level = value;
	}

	void CRedGuiTreeNode::SetTreeView( CRedGuiTreeView* treeView )
	{
		m_treeView = treeView;
	}

	void CRedGuiTreeNode::SetParentNode( CRedGuiTreeNode* node )
	{
		m_parentNode = node;
	}

	void CRedGuiTreeNode::SetIndex( Uint32 value )
	{
		m_index = value;
	}

	void CRedGuiTreeNode::FindNodeByName( const String& nodeName, TreeNodeCollection& nodes ) const
	{
		const Uint32 nodeCount = m_childrenNodes.Size();
		for( Uint32 i=0; i<nodeCount; ++i )
		{
			if( m_childrenNodes[i]->GetText().ContainsSubstring( nodeName ) == true )
			{
				nodes.PushBack( m_childrenNodes[i] );
			}
			m_childrenNodes[i]->FindNodeByName( nodeName, nodes );
		}
	}

	void CRedGuiTreeNode::GetAllNodes( TreeNodeCollection& nodes )
	{
		const Uint32 nodeCount = m_childrenNodes.Size();
		for( Uint32 i=0; i<nodeCount; ++i )
		{
			nodes.PushBack( m_childrenNodes[i] );
			m_childrenNodes[i]->GetAllNodes( nodes );
		}
	}

	Bool CRedGuiTreeNode::SearchVisibleNodeIndex( CRedGuiTreeNode* wantedNode, Int32& foundIndex )
	{
		++foundIndex;

		if( wantedNode == this )
		{
			return true;
		}

		if( m_expanded == true )
		{
			const Uint32 nodeCount = m_childrenNodes.Size();
			for( Uint32 i=0; i<nodeCount; ++i )
			{
				if( m_childrenNodes[i]->SearchVisibleNodeIndex( wantedNode, foundIndex ) == true )
				{
					return true;
				}
			}
		}

		return false;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
