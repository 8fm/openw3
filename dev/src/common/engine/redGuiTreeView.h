/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"
#include "redGuiTreeNode.h"

namespace RedGui
{
	class CRedGuiTreeNode;
	typedef TDynArray< CRedGuiTreeNode* > TreeNodeCollection;


	// IMPORTANT! - ViewTree not refreshes yourself after adding or removing nodes,
	//				we must refresh control manually
	class CRedGuiTreeView : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiTreeView(Uint32 x, Uint32 y, Uint32 width, Uint32 height);
		virtual ~CRedGuiTreeView();

#pragma region Events

		// Events
		// param 1 - sender = tree view
		// param 2 - collapsed node
		Event2_PackageTreeNode EventNodeCollapsed;

		// param 1 - sender = tree view
		// param 2 - expanded node
		Event2_PackageTreeNode EventNodeExpanded;

		// param 1 - sender = tree view
		// param 2 - selected node
		// param 3 - selected value
		Event2_PackageTreeNode EventSelectedNode;

		// param 1 - sender = tree view
		// param 2 - double clicked node
		Event2_PackageTreeNode EventDoubleClickedNode;

#pragma endregion

		CRedGuiTreeNode* AddRootNode( const String& name );
		CRedGuiTreeNode* AddRootNode( CRedGuiTreeNode* rootNode );

		Uint32 GetRootNodeCount();
		TreeNodeCollection& GetRootNodes();
		void GetAllNodes( TreeNodeCollection& nodes );

		Uint32 GetIndent() const;
		void SetIndent( Uint32 value );

		Char GetPathSeparator() const;
		void SetPathSeparator( Char separator = '\\' );

		Bool GetShowPlusMinus() const;
		void SetShowPlusMinus( Bool value );

		Bool GetEnableTabbedColumns() const;
		void EnableTabbedColumns( Bool value );

		void RemoveColumns();
		void AddColumn( const Uint32 width, const String& title );
		void SetColumnTitle( const Uint32 columnIndex, const String& title );
		void SetColumnWidth( const Uint32 columnIndex, const Uint32 width );
		const String& GetColumnTitle( const Uint32 columnIndex ) const;
		const Uint32 GetColumnWidth( const Uint32 columnIndex ) const;

		CRedGuiTreeNode* GetSelectedNode() const;
		void SetSelectedNode( CRedGuiTreeNode* node );
		
		Bool FindNodeByName( const String& nodeName, TreeNodeCollection& nodes ) const;

		Int32 FindNodeIndex( CRedGuiTreeNode* node );
		void SetFirstNode( CRedGuiTreeNode* node );

		Uint32 GetVisibleNodeCount() const;
		Uint32 GetNodeCount( Bool includeSubTrees = false ) const;

		void SetHeaderVisible( Bool value );
		Bool GetHeaderVisible() const;
		void SetHeaderCaption( const String& caption );
		String GetHeaderCaption() const;

		void CollapseAll();
		void ExpandAll();

		void RemoveChildNode( CRedGuiTreeNode* node );
		void RemoveAllNodes();

		void Refresh();

		virtual void Draw();
		virtual void SetToolTip( CRedGuiControl* tooltip );		

	private:
		Vector2 GetTextSize( const String& text );		
		CRedGuiTreeNode* CheckPoint( const Vector2& position );

		void RecaculateTreeView();
		void RecaculateTreeViewReqursive( CRedGuiTreeNode* node );

		CRedGuiTreeNode* GetVisibleNodeAt( Uint32 index );
		CRedGuiTreeNode* GetVisibleNodeAtReqursive( CRedGuiTreeNode* node, Uint32& visibleNodeIndex, Uint32 index );
		
		void DrawNode( CRedGuiTreeNode* node, Uint32& drawnNodeCount );		
		void DrawNodeText( const Vector2& position, const String& text, const Color& color );

		void ExpandNode( CRedGuiTreeNode* node );
		void CollapseNode( CRedGuiTreeNode* node );

		void UpdateHighlightedRow( CRedGuiTreeNode* newHighlightedRow );

		void UpdateView();
		void UpdateScrollPosition();
		void UpdateScrollSize();

		void UpdateSelection( CRedGuiTreeNode* node );

		void NotifyHeaderClicked( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyScrollChangePosition( RedGui::CRedGuiEventPackage& eventPackage, Uint32 value );
		void NotifyMouseMove( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition );
		void NotifyMouseWheel( RedGui::CRedGuiEventPackage& eventPackage, Int32 delta );
		void NotifyMouseButtonDoubleClick( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button );

		void OnDoubleClickOnRow();

		void NotifyMouseButtonClick( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button );

		void OnClickOnRow();

		void NotifyClientMouseLostFocus( RedGui::CRedGuiEventPackage& eventPackage, CRedGuiControl* newFocusedCtrl );
		void NotifyClientSizeChanged( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& oldSize, const Vector2& newSize );
		void NotifyClientMouseButtonPressed( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button );
		void NotifyClientMouseButtonReleased( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button );

		Bool OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data );

		virtual void OnPendingDestruction() override final;

	private:
		struct ColumnInfo
		{
			String		m_caption;
			Uint32		m_width;
		};

		typedef TDynArray< ColumnInfo, MC_RedGuiControls, MemoryPool_RedGui >	TColumns;

		CRedGuiButton*		m_header;					//!< 
		CRedGuiPanel*		m_croppClient;				//!< 
		CRedGuiScrollBar*	m_horizontalBar;			//!< 
		CRedGuiScrollBar*	m_verticalBar;				//!< 
		CRedGuiImage*		m_plusIcon;					//!< 
		CRedGuiImage*		m_minusIcon;				//!< 

		Uint32				m_horizontalRange;			//!< 
		Uint32				m_verticalRange;			//!< 

		Uint32				m_maxItemWidth;				//!< 
		Uint32				m_maxVerticalItemCount;		//!< 
		Uint32				m_visibleNodeCount;			//!< 
		Uint32				m_invisibleFromTop;			//!< 
		Vector2				m_firstItemPosition;		//!< 
		Vector2				m_firstVisibleItemPosition;	//!< 

		Uint32				m_indentSize;				//!< 
		Char				m_pathSeparator;			//!< 
		Bool				m_showPlusMinus;			//!< 
		Bool				m_enableTabbedColumns;		//!<

		TColumns			m_columns;					//!<

		CRedGuiTreeNode*	m_firstVisibleNode;			//!<
		CRedGuiTreeNode*	m_pushedNode;				//!< 
		CRedGuiTreeNode*	m_highlightNode;			//!< 
		CRedGuiTreeNode*	m_selectedNode;				//!< 

		TreeNodeCollection	m_rootNodes;				//!< 

	};
}	// namespace RedGui

#endif	// NO_RED_GUI
