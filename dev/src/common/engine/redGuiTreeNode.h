/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiUserData.h"

namespace RedGui
{
	class CRedGuiTreeNode;
	class CRedGuiTreeView;
	typedef TDynArray< CRedGuiTreeNode* > TreeNodeCollection;

	class CRedGuiTreeNode : public CRedGuiUserData
	{
		friend class CRedGuiTreeView;
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiTreeNode();
		CRedGuiTreeNode( const String& text );
		CRedGuiTreeNode( const String& text, const TDynArray< CRedGuiTreeNode* >& childrenNodes );
		~CRedGuiTreeNode();

		CRedGuiTreeNode* AddNode( const String& name );
		CRedGuiTreeNode* AddNode( CRedGuiTreeNode* rootNode );

		Uint32 GetLevel() const;
		Int32 GetIndex() const;

		String GetFullPath() const;

		String GetText() const;
		void SetText( const String& text );

		Bool GetExpanded() const;
		void SetExpaned( Bool value );

		Bool GetSelected() const;
		void SetSelected( Bool value );

		Bool GetVisible() const;
		void SetVisible();

		CRedGuiImage* GetImage() const;
		void SetImage( const String& path );

		const Color& GetBackgroundColor() const;
		void SetBackgroundColor( const Color& color );

		const Color& GetTextColor() const;
		void SetTextColor( const Color& color );

		CRedGuiTreeNode* GetFirstChildNode() const;
		CRedGuiTreeNode* GetLastChildNode() const;

		CRedGuiTreeNode* GetNextNode() const;
		CRedGuiTreeNode* GetPreviousNode() const;

		CRedGuiTreeNode* GetNextVisibleNode() const;
		CRedGuiTreeNode* GetPreviousVisibleNode() const;

		CRedGuiTreeNode* GetParentNode() const;
		CRedGuiTreeNode* GetNodeAt( Uint32 index ) const;

		CRedGuiTreeView* GetTreeView() const;

		Uint32 GetNodeCount( Bool includeSubTrees = false ) const;
		TreeNodeCollection& GetChildrenNodes();
		void GetAllNodes( TreeNodeCollection& nodes );

		void FindNodeByName( const String& nodeName, TreeNodeCollection& nodes ) const;

		void Toggle();
		void Expand( Bool ignoreChildren = true );
		void Collapse( Bool ignoreChildren = true );

		void Dispose();
		void RemoveChildNode( CRedGuiTreeNode* node );

		template< typename Pred >
		void SortChildren( Pred& pred )
		{
			Sort( m_childrenNodes.Begin(), m_childrenNodes.End(), pred );
			for( Uint32 i = 0 ; i < m_childrenNodes.Size(); ++i)
			{
				m_childrenNodes[i]->SetIndex( i );
			}
		}

	private:
		void SetLevel( Uint32 value );
		void SetIndex( Uint32 value );
		void SetTreeView( CRedGuiTreeView* treeView );
		void SetParentNode( CRedGuiTreeNode* node );

		void GetVisibleNodeReqursive( CRedGuiTreeNode* node, Uint32& count );
		void DrawChildrenReqursively( CRedGuiTreeNode* node, Vector2 deltaSpace );

		Bool SearchVisibleNodeIndex( CRedGuiTreeNode* wantedNode, Int32& foundIndex );

	private:
		Bool				m_expanded;				//!< 
		Bool				m_selected;				//!< 
		Bool				m_visible;				//!< 

		Int32				m_index;				//!< 
		Int32				m_level;				//!< 

		Color				m_backgroundColor;		//!< 
		Color				m_textColor;			//!< 

		String				m_text;					//!< 
		
		CRedGuiTreeView*	m_treeView;				//!< 
		CRedGuiTreeNode*	m_parentNode;			//!< 
		
		CRedGuiImage*		m_image;				//!< 

		TreeNodeCollection	m_childrenNodes;		//!< 
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
