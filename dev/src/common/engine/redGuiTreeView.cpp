/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiManager.h"
#include "redGuiTreeView.h"
#include "redGuiScrollBar.h"
#include "redGuiPanel.h"
#include "redGuiButton.h"
#include "redGuiImage.h"
#include "fonts.h"

namespace RedGui
{
	namespace
	{
		const Uint32 GDefaultListItemHeight = 18;
		const Uint32 GScrollPageSize = 16;
		const Uint32 GScrollWheelValue = 50;
		const Color GHighlight( 255,170, 0, 100 );
		const Color GPushed( 255, 150, 0, 100 );
		const Color GNormal( 45,45,45,255 );
		const Color GBorder( 112, 112, 112, 255 );
	}

	CRedGuiTreeView::CRedGuiTreeView( Uint32 x, Uint32 y, Uint32 width, Uint32 height )
		: CRedGuiControl(x, y, width, height)
		, m_header( nullptr )
		, m_croppClient( nullptr )
		, m_horizontalBar( nullptr )
		, m_verticalBar( nullptr )
		, m_firstVisibleNode( nullptr )
		, m_pushedNode( nullptr )
		, m_highlightNode( nullptr )
		, m_maxItemWidth( 0 )
		, m_maxVerticalItemCount( 0 )
		, m_indentSize( 20 )
		, m_pathSeparator( '\\' )
		, m_showPlusMinus( true )
		, m_horizontalRange( 0 )
		, m_verticalRange( 0 )
		, m_selectedNode( nullptr )
		, m_enableTabbedColumns( false )
	{
		SetNeedKeyFocus(true);

		// create header button
		m_header = new CRedGuiButton( 0, 0, width, GDefaultListItemHeight );
		m_header->EventButtonClicked.Bind( this, &CRedGuiTreeView::NotifyHeaderClicked );
		m_header->SetBackgroundColor( Color( 30, 30, 30, 255 ) );
		m_header->SetDock( DOCK_Top );
		m_header->SetTextAlign( IA_MiddleCenter );
		m_header->SetText( TXT("Header") );
		AddChild( m_header );

		// create scrollbars
		m_horizontalBar = new CRedGuiScrollBar( 0, height, width, 20, false );
		m_horizontalBar->EventScrollChangePosition.Bind( this, &CRedGuiTreeView::NotifyScrollChangePosition );
		m_horizontalBar->SetMinTrackSize( 20 );
		m_horizontalBar->SetDock( DOCK_Bottom );
		m_horizontalBar->SetVisible( false );
		AddChild( m_horizontalBar );

		m_verticalBar = new CRedGuiScrollBar( width, 0, 20, height );
		m_verticalBar->EventScrollChangePosition.Bind( this, &CRedGuiTreeView::NotifyScrollChangePosition );
		m_verticalBar->SetMinTrackSize( 20 );
		m_verticalBar->SetDock( DOCK_Right );
		m_verticalBar->SetVisible( false );
		AddChild( m_verticalBar );

		// create cropped client
		m_croppClient = new CRedGuiPanel( 0, 0, width, height );
		m_croppClient->SetBorderVisible( false );
		m_croppClient->SetBackgroundColor( Color::CLEAR );
		m_croppClient->SetForegroundColor( Color::CLEAR );
		m_croppClient->EventMouseWheel.Bind( this, &CRedGuiTreeView::NotifyMouseWheel );
		m_croppClient->EventMouseButtonClick.Bind( this, &CRedGuiTreeView::NotifyMouseButtonClick );
		m_croppClient->EventMouseButtonDoubleClick.Bind( this, &CRedGuiTreeView::NotifyMouseButtonDoubleClick );
		m_croppClient->EventMouseMove.Bind( this, &CRedGuiTreeView::NotifyMouseMove );
		m_croppClient->EventMouseLostFocus.Bind( this, &CRedGuiTreeView::NotifyClientMouseLostFocus );
		m_croppClient->EventSizeChanged.Bind( this, &CRedGuiTreeView::NotifyClientSizeChanged );
		m_croppClient->EventMouseButtonPressed.Bind( this, &CRedGuiTreeView::NotifyClientMouseButtonPressed );
		m_croppClient->EventMouseButtonReleased.Bind( this, &CRedGuiTreeView::NotifyClientMouseButtonReleased );
		m_croppClient->SetDock(DOCK_Fill);
		AddChild( m_croppClient );

		// load images
		m_plusIcon = new CRedGuiImage( 0, 0, GDefaultListItemHeight, GDefaultListItemHeight );
		m_plusIcon->SetImage( Resources::GPlusIcon );
		m_minusIcon = new CRedGuiImage( 0, 0, GDefaultListItemHeight, GDefaultListItemHeight );
		m_minusIcon->SetImage( Resources::GMinusIcon );

		m_maxItemWidth = (Uint32)( m_croppClient->GetSize().X );
		m_maxVerticalItemCount = (Uint32)( m_croppClient->GetHeight() / GDefaultListItemHeight );
	}

	CRedGuiTreeView::~CRedGuiTreeView()
	{
		
	}

	void CRedGuiTreeView::OnPendingDestruction()
	{
		const Uint32 nodeCount = m_rootNodes.Size();
		for( Uint32 i=0; i<nodeCount; ++i )
		{
			delete m_rootNodes[i];
		}
		m_rootNodes.Clear();

		// dispose images
		if( m_minusIcon != nullptr )
		{
			m_minusIcon->Dispose();
			m_minusIcon = nullptr;
		}
		if( m_plusIcon != nullptr )
		{
			m_plusIcon->Dispose();
			m_plusIcon = nullptr;
		}
	}

	void CRedGuiTreeView::Draw()
	{
		GetTheme()->DrawPanel(this);
		GetTheme()->SetCroppedParent( m_croppClient );

		if( m_firstVisibleNode != nullptr )
		{
			Uint32 drawnNodeCount = 0;
			
			for( CRedGuiTreeNode* currentNode = m_firstVisibleNode; currentNode!= nullptr && drawnNodeCount<( m_maxVerticalItemCount+1 ); currentNode = currentNode->GetNextNode() )
			{
				DrawNode( currentNode, drawnNodeCount );
			}
		}

		GetTheme()->ResetCroppedParent();
	}

	void CRedGuiTreeView::DrawNode( CRedGuiTreeNode* node, Uint32& drawnNodeCount )
	{
		Uint32 currentIndent = node->GetLevel() * GetIndent();
		Uint32 xPos = m_croppClient->GetAbsoluteLeft() + currentIndent;
		Uint32 yPos = m_croppClient->GetAbsoluteTop() + (Uint32)m_firstVisibleItemPosition.Y + ( drawnNodeCount * GDefaultListItemHeight );

		xPos += GDefaultListItemHeight;
		// draw node row background
		GetTheme()->DrawRawFilledRectangle( Vector2( (Float)xPos, (Float)yPos ), Vector2( (Float)m_maxItemWidth, (Float)GDefaultListItemHeight ), node->GetBackgroundColor() );

		// on mouse reactions
		if( node->GetSelected() == true )
		{
			GetTheme()->DrawRawFilledRectangle( Vector2( (Float)m_croppClient->GetAbsoluteLeft(), (Float)yPos ), Vector2( (Float)m_croppClient->GetWidth(), (Float)GDefaultListItemHeight ), GHighlight );
		}

		if( node == m_highlightNode )
		{
			GetTheme()->DrawRawFilledRectangle( Vector2( (Float)m_croppClient->GetAbsoluteLeft(), (Float)yPos ), Vector2( (Float)m_croppClient->GetWidth(), (Float)GDefaultListItemHeight ), GHighlight );
		}

		if( node == m_pushedNode )
		{
			GetTheme()->DrawRawFilledRectangle( Vector2( (Float)m_croppClient->GetAbsoluteLeft(), (Float)yPos ), Vector2( (Float)m_croppClient->GetWidth(), (Float)GDefaultListItemHeight ), GPushed );
		}

		xPos -= GDefaultListItemHeight;
		// draw pus/minus icon
		if( node->GetNodeCount() > 0  && m_showPlusMinus == true )
		{
			if( node->GetExpanded() == true )
			{
				GetTheme()->DrawRawImage( Vector2( (Float)xPos + 4, (Float)yPos + 5 ), Vector2( (Float)GDefaultListItemHeight - 8, (Float)GDefaultListItemHeight - 10 ), m_minusIcon->GetImage(), Color::WHITE );
			}
			else
			{
				GetTheme()->DrawRawImage( Vector2( (Float)xPos + 4, (Float)yPos + 5 ), Vector2( (Float)GDefaultListItemHeight - 8, (Float)GDefaultListItemHeight - 10 ), m_plusIcon->GetImage(), Color::WHITE );
			}
		}
		xPos += GDefaultListItemHeight;
		
		// draw custom image for node
		CRedGuiImage* customNodeImage = node->GetImage();
		if( customNodeImage != nullptr )
		{
			GetTheme()->DrawRawImage( Vector2( (Float)xPos, (Float)yPos ), Vector2( (Float)GDefaultListItemHeight, (Float)GDefaultListItemHeight ), customNodeImage->GetImage(), Color::WHITE );
			xPos += GDefaultListItemHeight;
		}

		// draw node text
		DrawNodeText( Vector2( (Float)xPos, (Float)yPos ), node->GetText(), node->GetTextColor() );

		++drawnNodeCount;

		// check children
		if( node->GetExpanded() == true )
		{
			currentIndent += m_indentSize;

			const Uint32 childNodeCount = node->GetNodeCount();
			TreeNodeCollection& childrenNodes = node->GetChildrenNodes();
			for( Uint32 i=0; i<childNodeCount && drawnNodeCount<( m_maxVerticalItemCount+1 ); ++i )
			{
				DrawNode( childrenNodes[i], drawnNodeCount );
			}
		}
	}

	void CRedGuiTreeView::DrawNodeText( const Vector2& position, const String& text, const Color& color )
	{
		const Vector2 textSize = GetTextSize( text );

		// calculate horizontal position
		const Float horizontalDelta = m_croppClient->GetPosition().X + 10.0f;	// item with 10 pixels padding looks better

		// calculate vertical position
		const Float verticalDelta = ( GDefaultListItemHeight - textSize.Y ) / 2.0f;

		// compose final position
		const Vector2 finalPosition = position + Vector2( horizontalDelta, verticalDelta );

		// text crap
		if ( m_enableTabbedColumns && m_columns.Size() >= 1 )
		{
			// split text into parts, based on the tab
			// TODO: sucks ass...
			TDynArray< String > parts;
			text.Slice( parts, TXT("\t") );

			// draw each part
			Float xOffset = 0;
			const Uint32 numParts = Red::Math::NumericalUtils::Min< Uint32 >( parts.Size(), m_columns.Size() );
			for ( Uint32 i=0; i<numParts; ++i )
			{
				GetTheme()->DrawRawText( finalPosition + Vector2( xOffset, 0.0f ), parts[i], color );
				xOffset += m_columns[i].m_width;
			}
		}
		else
		{
			// whole string
			GetTheme()->DrawRawText( finalPosition, text, color );
		}
	}

	void CRedGuiTreeView::SetToolTip( CRedGuiControl* tooltip )
	{
		m_croppClient->SetNeedToolTip( true );
		m_croppClient->SetToolTip( tooltip );
	}

	void CRedGuiTreeView::RemoveChildNode( CRedGuiTreeNode* node )
	{
		if( node != nullptr )
		{
			m_rootNodes.Remove( node );
			delete node;
		}
		RecaculateTreeView();
	}

	CRedGuiTreeNode* CRedGuiTreeView::CheckPoint( const Vector2& position )
	{		
		const Vector2 relativePosition = position - m_croppClient->GetAbsolutePosition();
		if( relativePosition.X > 0.0 && relativePosition.Y > 0.0 )
		{
			Uint32 deltaPosition = (Uint32)-m_firstItemPosition.Y;
			Uint32 visibleItemIndex = (Uint32)( ( relativePosition.Y + deltaPosition ) / (Float)GDefaultListItemHeight );

			if( visibleItemIndex > GetVisibleNodeCount() )
			{
				return nullptr;
			}

			return GetVisibleNodeAt( visibleItemIndex );
		}

		return nullptr;
	}

	Vector2 CRedGuiTreeView::GetTextSize( const String& text )
	{
		CFont* font = GetFont();
		if(font == nullptr)
		{
			return Vector2( 0.0f, 0.0f );
		}

		Int32 unusedX, unusedY;
		Uint32 textWidth, textHeight;
		font->GetTextRectangle( text, unusedX, unusedY, textWidth, textHeight );

		return Vector2( (Float)textWidth, (Float)textHeight );
	}

	void CRedGuiTreeView::UpdateView()
	{
		UpdateScrollSize();
		UpdateScrollPosition();
	}

	void CRedGuiTreeView::UpdateScrollSize()
	{
		Vector2 viewSize = m_croppClient->GetSize();
		Vector2 contentSize = Vector2( (Float)m_maxItemWidth, (Float)( GetVisibleNodeCount()* GDefaultListItemHeight ) );

		// horizontal content doesn't fit
		if(contentSize.Y > viewSize.Y)
		{
			if( m_verticalBar != nullptr )
			{
				if( m_verticalBar->GetVisible() == false )
				{
					m_verticalBar->SetVisible( true );

					if(m_horizontalBar != nullptr)
					{
						// recalculate horizontal bar after add vertical bar
						if( ( contentSize.X > viewSize.X ) && ( m_horizontalBar->GetVisible() == false ) )
						{
							m_horizontalBar->SetVisible( true );
						}
					}
				}
			}
		}
		else
		{
			if( m_verticalBar != nullptr )
			{
				if( m_verticalBar->GetVisible() == true )
				{
					m_verticalBar->SetVisible( false );

					if( m_horizontalBar != nullptr )
					{
						// recalculate horizontal bar after remove vertical bar
						if( ( contentSize.X <= viewSize.X ) && ( m_horizontalBar->GetVisible() == true ) )
						{
							m_horizontalBar->SetVisible( false );
						}
					}
				}
			}
		}

		// vertical content doesn't fit
		if( contentSize.X > viewSize.X )
		{
			if( m_horizontalBar != nullptr )
			{
				if( m_horizontalBar->GetVisible() == false )
				{
					m_horizontalBar->SetVisible( true );

					if( m_verticalBar != nullptr )
					{
						// recalculate vertical bar after add horizontal bar
						if( ( contentSize.Y > viewSize.Y ) && ( m_verticalBar->GetVisible() == false ) )
						{
							m_verticalBar->SetVisible( true );
						}
					}
				}
			}
		}
		else
		{
			if( m_horizontalBar != nullptr )
			{
				if( m_horizontalBar->GetVisible() == true )
				{
					m_horizontalBar->SetVisible( false );

					if( m_verticalBar != nullptr )
					{
						// recalculate vertical bar after remove horizontal bar
						if( ( contentSize.Y <= viewSize.Y ) && ( m_verticalBar->GetVisible() == true ) )
						{
							m_verticalBar->SetVisible( false );
						}
					}
				}
			}
		}

		// calculate ranges
		m_verticalRange = ( viewSize.Y >= contentSize.Y ) ? 0 : (Uint32)( contentSize.Y - viewSize.Y );
		m_horizontalRange = ( viewSize.X >= contentSize.X ) ? 0 : (Uint32)( contentSize.X - viewSize.X );

		// set new values
		if( m_verticalBar != nullptr )
		{
			m_verticalBar->SetScrollPage( GScrollPageSize );
			m_verticalBar->SetScrollRange( m_verticalRange + 1 );
			if( contentSize.Y > 0 )
			{
				m_verticalBar->SetTrackSize( (Int32)( (Float)( m_verticalBar->GetLineSize() * viewSize.Y ) / (Float)( contentSize.Y ) ) );
			}
		}
		if( m_horizontalBar != nullptr )
		{
			m_horizontalBar->SetScrollPage( GScrollPageSize );
			m_horizontalBar->SetScrollRange( m_horizontalRange + 1 );
			if( contentSize.X > 0 )
			{
				m_horizontalBar->SetTrackSize( (Int32)( (Float)( m_horizontalBar->GetLineSize() * viewSize.X ) / (Float)( contentSize.X ) ) );
			}
		}
	}

	void CRedGuiTreeView::UpdateScrollPosition()
	{
		if( m_verticalRange != 0 )
		{
			Vector2 position = m_firstItemPosition;
			Int32 offset = -(Int32)position.Y;

			if( offset < 0 )
			{
				offset = 0;
			}
			else if( offset > (Int32)m_verticalRange )
			{
				offset = m_verticalRange;
			}

			if( offset != position.Y )
			{
				position.Y = -(Float)offset;
				if( m_verticalBar != nullptr )
				{
					m_verticalBar->SetScrollPosition( offset );
				}

				// calculate first and last render item
				m_firstItemPosition = position;
			}
		}
		else if( m_horizontalRange != 0 )
		{
			Vector2 position = m_firstItemPosition;
			Int32 offset = -(Int32)position.X;

			if( offset < 0 )
			{
				offset = 0;
			}
			else if( offset > (Int32)m_horizontalRange )
			{
				offset = m_horizontalRange;
			}

			if( offset != position.X )
			{
				position.X = -(Float)offset;
				if( m_horizontalBar != nullptr )
				{
					m_horizontalBar->SetScrollPosition( offset );
				}
				// calculate first and last render item
				m_firstItemPosition = position;
			}
		}

		// set current visible items
		m_invisibleFromTop = (Uint32)( (-m_firstItemPosition.Y) / GDefaultListItemHeight );
		m_firstVisibleItemPosition = m_firstItemPosition.Y + (Float)( m_invisibleFromTop * GDefaultListItemHeight );

		if( m_invisibleFromTop == 0 )
		{
			if( m_rootNodes.Size() > 0 )
			{
				m_firstVisibleNode = m_rootNodes[0];
			}
			else
			{
				m_firstVisibleNode = nullptr;
			}
		}
		else
		{
			m_firstVisibleNode = GetVisibleNodeAt( m_invisibleFromTop );
		}
	}

	void CRedGuiTreeView::UpdateSelection( CRedGuiTreeNode* node )
	{
		if( m_selectedNode != nullptr )
		{
			m_selectedNode->SetSelected( false );
		}
		if( node != nullptr )
		{
			node->SetSelected( true );
			m_selectedNode = node;
		}
	}

	void CRedGuiTreeView::NotifyHeaderClicked( RedGui::CRedGuiEventPackage& eventPackage )
	{
		/* intentionally empty */
		// In the future maybe create some kind of sorting
	}

	void CRedGuiTreeView::NotifyScrollChangePosition( RedGui::CRedGuiEventPackage& eventPackage, Uint32 value )
	{
		RedGui::CRedGuiControl* sender = eventPackage.GetEventSender();

		if( sender == m_verticalBar )
		{
			Vector2 point = m_firstItemPosition;
			point.Y = -(Float)value;
			m_firstItemPosition = point;
		}
		else if( sender == m_horizontalBar )
		{
			Vector2 point = m_firstItemPosition;
			point.X = -(Float)value;
			m_firstItemPosition = point;
		}

		UpdateView();
	}

	void CRedGuiTreeView::NotifyMouseMove( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition )
	{
		RED_UNUSED( eventPackage );

		CRedGuiTreeNode* nodeUnderMouse = CheckPoint( mousePosition );

		if( m_highlightNode != nodeUnderMouse )
		{
			m_highlightNode = nodeUnderMouse;
			GRedGui::GetInstance().GetToolTipManager()->HideToolTip( m_croppClient );
		}
	}

	void CRedGuiTreeView::NotifyMouseWheel( RedGui::CRedGuiEventPackage& eventPackage, Int32 delta )
	{
		RED_UNUSED( eventPackage );

		if( m_verticalRange != 0 )
		{
			Vector2 position = m_firstItemPosition;
			Int32 offset = -(Int32)position.Y;

			if( delta < 0 )
			{
				offset += GScrollWheelValue;
			}
			else
			{
				offset -= GScrollWheelValue;
			}

			if( offset < 0 )
			{
				offset = 0;
			}
			else if( offset > (Int32)m_verticalRange )
			{
				offset = m_verticalRange;
			}

			if( offset != position.Y )
			{
				position.Y = -(Float)offset;
				if( m_verticalBar != nullptr )
				{
					m_verticalBar->SetScrollPosition( offset );
				}
				m_firstItemPosition = position;
			}
		}
		else if( m_horizontalRange != 0 )
		{
			Vector2 position = m_firstItemPosition;
			Int32 offset = -(Int32)position.X;

			if( delta < 0 )
			{
				offset += GScrollWheelValue;
			}
			else
			{
				offset -= GScrollWheelValue;
			}

			if( offset < 0 )
			{
				offset = 0;
			}
			else if( offset > (Int32)m_horizontalRange )
			{
				offset = m_horizontalRange;
			}

			if( offset != position.X )
			{
				position.X = -(Float)offset;
				if( m_horizontalBar != nullptr )
				{
					m_horizontalBar->SetScrollPosition( offset );
				}
				m_firstItemPosition = position;
			}
		}

		UpdateView();
	}

	void CRedGuiTreeView::NotifyMouseButtonDoubleClick( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button )
	{
		RED_UNUSED( eventPackage );

		OnDoubleClickOnRow();

	}

	void CRedGuiTreeView::NotifyClientMouseLostFocus( RedGui::CRedGuiEventPackage& eventPackage, CRedGuiControl* newFocusedCtrl )
	{
		RED_UNUSED( eventPackage );

		m_highlightNode = nullptr;
	}

	void CRedGuiTreeView::NotifyClientSizeChanged( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& oldSize, const Vector2& newSize )
	{
		RED_UNUSED( eventPackage );

		m_maxVerticalItemCount = (Uint32)( m_croppClient->GetHeight() / GDefaultListItemHeight );
		RecaculateTreeView();
	}

	void CRedGuiTreeView::NotifyClientMouseButtonPressed( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button )
	{
		RED_UNUSED( eventPackage );

		m_pushedNode = m_highlightNode;

		if( m_pushedNode != nullptr )
		{
			UpdateSelection( m_pushedNode );
			EventSelectedNode( this, m_pushedNode );
		}	
	}

	void CRedGuiTreeView::NotifyClientMouseButtonReleased( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button )
	{
		RED_UNUSED( eventPackage );

		m_pushedNode = nullptr;
	}

	void CRedGuiTreeView::SetHeaderVisible( Bool value )
	{
		m_header->SetVisible( value );
	}

	Bool CRedGuiTreeView::GetHeaderVisible() const
	{
		return m_header->GetVisible();
	}

	void CRedGuiTreeView::SetHeaderCaption( const String& caption )
	{
		m_header->SetText( caption );
	}

	String CRedGuiTreeView::GetHeaderCaption() const
	{
		return m_header->GetText();
	}

	CRedGuiTreeNode* CRedGuiTreeView::AddRootNode( const String& name )
	{
		CRedGuiTreeNode* newNode = new CRedGuiTreeNode( name );
		newNode->SetLevel( 0 );
		newNode->SetTreeView( this );
		newNode->SetIndex( m_rootNodes.Size() );
		m_rootNodes.PushBack( newNode );
		return m_rootNodes[ m_rootNodes.Size() -1 ];
	}

	CRedGuiTreeNode* CRedGuiTreeView::AddRootNode( CRedGuiTreeNode* rootNode )
	{
		rootNode->SetLevel( 0 );
		rootNode->SetTreeView( this );
		rootNode->SetIndex( m_rootNodes.Size() );
		m_rootNodes.PushBack( rootNode );
		return rootNode;
	}

	TreeNodeCollection& CRedGuiTreeView::GetRootNodes()
	{
		return m_rootNodes;
	}

	Uint32 CRedGuiTreeView::GetIndent() const
	{
		return m_indentSize;
	}

	void CRedGuiTreeView::SetIndent( Uint32 value )
	{
		m_indentSize = value;
	}

	Char CRedGuiTreeView::GetPathSeparator() const
	{
		return m_pathSeparator;
	}

	void CRedGuiTreeView::SetPathSeparator( Char separator /*= '\\'*/ )
	{
		m_pathSeparator = separator;
	}

	Bool CRedGuiTreeView::GetShowPlusMinus() const
	{
		return m_showPlusMinus;
	}

	void CRedGuiTreeView::SetShowPlusMinus( Bool value )
	{
		m_showPlusMinus = value;
	}

	CRedGuiTreeNode* CRedGuiTreeView::GetSelectedNode() const
	{
		return m_selectedNode;
	}

	Bool CRedGuiTreeView::GetEnableTabbedColumns() const
	{
		return m_enableTabbedColumns;
	}

	void CRedGuiTreeView::EnableTabbedColumns( Bool value )
	{
		m_enableTabbedColumns = value;
	}

	void CRedGuiTreeView::RemoveColumns()
	{
		m_columns.ClearFast();
	}

	void CRedGuiTreeView::AddColumn( const Uint32 width, const String& title )
	{
		ColumnInfo info;
		info.m_caption = title;
		info.m_width = width;
		m_columns.PushBack( info );
	}

	void CRedGuiTreeView::SetColumnTitle( const Uint32 columnIndex, const String& title )
	{
		if ( columnIndex < m_columns.Size() )
			m_columns[ columnIndex ].m_caption = title;
	}

	void CRedGuiTreeView::SetColumnWidth( const Uint32 columnIndex, const Uint32 width )
	{
		if ( columnIndex < m_columns.Size() )
			m_columns[ columnIndex ].m_width = width;
	}

	const String& CRedGuiTreeView::GetColumnTitle( const Uint32 columnIndex ) const
	{
		if ( columnIndex < m_columns.Size() )
			return m_columns[ columnIndex ].m_caption;

		return String::EMPTY;
	}

	const Uint32 CRedGuiTreeView::GetColumnWidth( const Uint32 columnIndex ) const
	{
		if ( columnIndex < m_columns.Size() )
			return m_columns[ columnIndex ].m_width;

		return 0;
	}

	void CRedGuiTreeView::SetSelectedNode( CRedGuiTreeNode* node )
	{
		if( m_selectedNode != nullptr )
		{
			m_selectedNode->SetSelected( false );
		}
		m_selectedNode = node;
		if( m_selectedNode != nullptr )
		{
			m_selectedNode->SetSelected( true );
		}
	}

	Bool CRedGuiTreeView::FindNodeByName( const String& nodeName, TreeNodeCollection& nodes ) const
	{
		const Uint32 nodeCount = m_rootNodes.Size();
		for( Uint32 i=0; i<nodeCount; ++i )
		{
			if( m_rootNodes[i]->GetText().ContainsSubstring( nodeName ) == true )
			{
				nodes.PushBack( m_rootNodes[i] );
			}
			m_rootNodes[i]->FindNodeByName( nodeName, nodes );
		}

		return ( nodes.Size() > 0 ) ? true : false;
	}

	Uint32 CRedGuiTreeView::GetNodeCount( Bool includeSubTrees /*= false*/ ) const
	{
		Uint32 nodeCount = 0;

		nodeCount = m_rootNodes.Size();

		if( includeSubTrees == true )
		{
			const Uint32 childNodeCount = m_rootNodes.Size();
			for( Uint32 i=0; i<childNodeCount; ++i )
			{
				nodeCount += m_rootNodes[i]->GetNodeCount( true );
			}
		}

		return nodeCount;
	}

	void CRedGuiTreeView::CollapseAll()
	{
		const Uint32 nodeCount = m_rootNodes.Size();
		for( Uint32 i=0; i<nodeCount; ++i )
		{
			m_rootNodes[i]->Collapse( false );
		}
	}

	void CRedGuiTreeView::ExpandAll()
	{
		const Uint32 nodeCount = m_rootNodes.Size();
		for( Uint32 i=0; i<nodeCount; ++i )
		{
			m_rootNodes[i]->Expand( false );
		}
	}

	Uint32 CRedGuiTreeView::GetRootNodeCount()
	{
		return m_rootNodes.Size();
	}

	void CRedGuiTreeView::RecaculateTreeView()
	{
		m_visibleNodeCount = 0;
		m_maxItemWidth = (Uint32)( m_croppClient->GetSize().X );

		const Uint32 nodeCount = m_rootNodes.Size();
		for( Uint32 i=0; i<nodeCount; ++i )
		{
			RecaculateTreeViewReqursive( m_rootNodes[i] );
		}

		UpdateView();
	}

	void CRedGuiTreeView::RecaculateTreeViewReqursive( CRedGuiTreeNode* node )
	{
		m_maxItemWidth = Max< Uint32 >( (Uint32)GetTextSize( node->GetText() ).X, m_maxItemWidth );
		++m_visibleNodeCount;

		if( node->GetExpanded() == true )
		{
			const Uint32 nodeCount = node->GetNodeCount();
			TDynArray< CRedGuiTreeNode* > nodes = node->GetChildrenNodes();
			for( Uint32 i=0; i<nodeCount; ++i )
			{
				RecaculateTreeViewReqursive( nodes[i] );
			}
		}
	}

	CRedGuiTreeNode* CRedGuiTreeView::GetVisibleNodeAt( Uint32 index )
	{
		Uint32 visibleNodeIndex = 0;

		const Uint32 nodeCount = m_rootNodes.Size();
		for( Uint32 i=0; i<nodeCount && visibleNodeIndex <= index; ++i )
		{
			if( visibleNodeIndex == index )
			{
				return m_rootNodes[i];
			}

			++visibleNodeIndex;

			if( m_rootNodes[i]->GetExpanded() == true )
			{
				CRedGuiTreeNode* tempNode = GetVisibleNodeAtReqursive( m_rootNodes[i], visibleNodeIndex, index );
				if( tempNode != nullptr )
				{
					return tempNode;
				}
			}
		}

		return nullptr;
	}

	CRedGuiTreeNode* CRedGuiTreeView::GetVisibleNodeAtReqursive( CRedGuiTreeNode* node, Uint32& visibleNodeIndex, Uint32 index )
	{
		const Uint32 nodeCount = node->GetNodeCount();
		TDynArray< CRedGuiTreeNode* > nodes = node->GetChildrenNodes();
		for( Uint32 i=0; i<nodeCount; ++i )
		{
			if( visibleNodeIndex == index )
			{
				return nodes[i];
			}

			++visibleNodeIndex;

			if( nodes[i]->GetExpanded() == true )
			{
				CRedGuiTreeNode* tempNode = GetVisibleNodeAtReqursive( nodes[i], visibleNodeIndex, index );
								if( tempNode != nullptr )
				{
					return tempNode;
				}
			}
		}
		return nullptr;
	}

	void CRedGuiTreeView::ExpandNode( CRedGuiTreeNode* node )
	{
		node->Expand();
		RecaculateTreeView();

		EventNodeExpanded( this, node );
	}

	void CRedGuiTreeView::CollapseNode( CRedGuiTreeNode* node )
	{
		node->Collapse( true );
		RecaculateTreeView();

		EventNodeCollapsed( this, node );
	}

	void CRedGuiTreeView::Refresh()
	{
		RecaculateTreeView();
	}

	Uint32 CRedGuiTreeView::GetVisibleNodeCount() const
	{
		return m_visibleNodeCount;
	}

	void CRedGuiTreeView::NotifyMouseButtonClick( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button )
	{
		RED_UNUSED( eventPackage );

		if( m_highlightNode != nullptr )
		{
			Uint32 xPos = ( m_highlightNode->GetLevel() * GetIndent() ) + m_croppClient->GetAbsoluteLeft();
			if( mousePosition.X < ( xPos + GDefaultListItemHeight ) && mousePosition.X > xPos )
			{
				OnClickOnRow();

			}
		}
	}

	void CRedGuiTreeView::GetAllNodes( TreeNodeCollection& nodes )
	{
		const Uint32 nodeCount = m_rootNodes.Size();
		for( Uint32 i=0; i<nodeCount; ++i )
		{
			nodes.PushBack( m_rootNodes[i] );
			m_rootNodes[i]->GetAllNodes( nodes );
		}
	}

	void CRedGuiTreeView::SetFirstNode( CRedGuiTreeNode* node )
	{
		Int32 visibleNodeIndex = FindNodeIndex( node );
		if( visibleNodeIndex != -1 )
		{
			if( (Int32)m_visibleNodeCount <= (Int32)m_maxVerticalItemCount || visibleNodeIndex < 2 )
			{
				m_firstItemPosition.Y = 0.0f;
			}
			else if ( visibleNodeIndex >= (Int32)m_visibleNodeCount - 2)
			{
				m_firstItemPosition.Y = - (Float)( ( m_visibleNodeCount - m_maxVerticalItemCount ) * GDefaultListItemHeight );
			}
			else
			{
				if( visibleNodeIndex <= (Int32)m_invisibleFromTop )
				{
					m_firstItemPosition.Y = - (Float)( ( visibleNodeIndex - 2 ) * GDefaultListItemHeight );
				}
				else if ( visibleNodeIndex > (Int32)( m_invisibleFromTop + m_maxVerticalItemCount ) )
				{
					m_firstItemPosition.Y = - (Float)( ( visibleNodeIndex - m_maxVerticalItemCount + 2 ) *GDefaultListItemHeight );
				}
				else if( visibleNodeIndex < (Int32)m_invisibleFromTop + 2 )
				{
					// We're going up
					m_firstItemPosition.Y += GDefaultListItemHeight;
				}
				else if( visibleNodeIndex > (Int32)( m_invisibleFromTop + m_maxVerticalItemCount ) - 2 )
				{
					// We're going down
					m_firstItemPosition.Y -= GDefaultListItemHeight;
				}
			}
		}
		UpdateView();
	}

	Int32 CRedGuiTreeView::FindNodeIndex( CRedGuiTreeNode* node )
	{
		Int32 visibleNodeIndex = -1;

		// expand tree to node
		for( CRedGuiTreeNode* parentNode = node->GetParentNode(); parentNode != nullptr; parentNode = parentNode->GetParentNode() )
		{
			parentNode->Expand();
		}

		RecaculateTreeView();

		// searching visible index
		const Uint32 nodeCount = m_rootNodes.Size();
		for( Uint32 i = 0; i < nodeCount; ++i )
		{
			if( m_rootNodes[i]->SearchVisibleNodeIndex( node, visibleNodeIndex ) == true )
			{
				break;
			}
		}
		return visibleNodeIndex;
	}

	void CRedGuiTreeView::RemoveAllNodes()
	{
		m_selectedNode = nullptr;
		m_highlightNode = nullptr;
		m_firstVisibleNode = nullptr;
		m_firstItemPosition.Y = 0.0f;

		m_rootNodes.ClearPtr();
		RecaculateTreeView();
	}

	Bool CRedGuiTreeView::OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data )
	{
		if( CRedGuiControl::OnInternalInputEvent( event, data ) == true )
		{
			return true;
		}

		if( event == RGIE_Down )
		{
			if( m_highlightNode == nullptr )
			{
				UpdateHighlightedRow( GetVisibleNodeAt( 0 ) );
			}
			else
			{
				CRedGuiTreeNode* node = m_highlightNode->GetNextVisibleNode();
				if( node == nullptr )
				{
					UpdateHighlightedRow( GetVisibleNodeAt( 0 ) );
				}
				else
				{
					UpdateHighlightedRow( m_highlightNode->GetNextVisibleNode() );
				}
			}
			return true;
		}
		else if( event == RGIE_Up )
		{
			if( m_highlightNode == nullptr )
			{
				UpdateHighlightedRow( GetVisibleNodeAt( GetVisibleNodeCount() - 1 ) );
			}
			else
			{
				CRedGuiTreeNode* node = m_highlightNode->GetPreviousVisibleNode();
				if( node == nullptr )
				{
					UpdateHighlightedRow( GetVisibleNodeAt( GetVisibleNodeCount() - 1 ) );
				}
				else
				{
					UpdateHighlightedRow( m_highlightNode->GetPreviousVisibleNode() );
				}
			}
			return true;
		}
		else if( event == RGIE_Select )
		{
			OnClickOnRow();
			return true;
		}
		else if( event == RGIE_Execute )
		{
			OnDoubleClickOnRow();
			return true;
		}
		else if( event == RGIE_Move )
		{
			m_horizontalBar->SetScrollPosition( (Uint32)( m_horizontalBar->GetScrollPosition() + data.X ) );
			m_verticalBar->SetScrollPosition( (Uint32)( m_verticalBar->GetScrollPosition() + data.Y ) );
			return true;
		}

		return false;
	}

	void CRedGuiTreeView::OnDoubleClickOnRow()
	{
		if( m_highlightNode != nullptr )
		{
			m_pushedNode = m_highlightNode;

			if( m_pushedNode != nullptr )
			{
				if( m_pushedNode->GetExpanded() == true )
				{
					CollapseNode( m_pushedNode );
				}
				else
				{
					ExpandNode( m_pushedNode );
				}
			}

			EventDoubleClickedNode( this, m_pushedNode );
		}
	}

	void CRedGuiTreeView::OnClickOnRow()
	{
		if( m_highlightNode != nullptr )
		{
			m_pushedNode = m_highlightNode;

			if( m_pushedNode != nullptr )
			{
				if( m_pushedNode->GetExpanded() == true )
				{
					CollapseNode( m_pushedNode );
				}
				else
				{
					ExpandNode( m_pushedNode );
				}
			}
		}
	}

	void CRedGuiTreeView::UpdateHighlightedRow( CRedGuiTreeNode* newHighlightedRow )
	{
		m_highlightNode = newHighlightedRow;
		if( m_highlightNode != nullptr )
		{
			SetFirstNode( m_highlightNode );
		}
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
