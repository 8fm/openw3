/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "toolsPanel.h"
#include "umbraTool.h"
#include "../../common/engine/clipMap.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/umbraScene.h"
#include "../../common/engine/umbraTile.h"

#ifdef USE_UMBRA
#ifndef NO_UMBRA_DATA_GENERATION

//////////////////////////////////////////////////////////////////////////

CTileGrid::CTileGrid( CWorld* world, ITileStatusRenderer* gridRenderer /*= nullptr*/, ITileStatusRenderer* queueRenderer /*=nullptr*/ )
	: m_gridRenderer( gridRenderer )
	, m_queueRenderer( queueRenderer )
	, m_world( world )
{
	m_tilesCount = m_world->GetUmbraScene()->GetGrid().GetCellCounts();

	m_gridRenderer->Initialize( this );
	m_queueRenderer->Initialize( this );

	for ( Int32 j = 0; j < m_tilesCount.Y; ++j )
	{
		for ( Int32 i = 0; i < m_tilesCount.X; ++i )
		{
			EUmbraTileDataStatus status = EUTDS_Unknown;
			SComputationParameters computationParams;
			computationParams.m_smallestHole = -1.0f;
			computationParams.m_smallestOccluder = -1.0f;

			VectorI cellIndex( i, j, 0, 0 );
			const TOcclusionGrid::ElementList& elements = m_world->GetUmbraScene()->GetGrid().GetElementsByCellIndex( cellIndex );
			if ( elements.Size() > 0 && elements[0].m_Data )
			{
				status = elements[0].m_Data->GetStatus();
				if ( elements[0].m_Data->HasData() )
				{
					computationParams = elements[0].m_Data->GetComputationParameters();
				}
			}

			if ( m_gridRenderer )
			{
				m_gridRenderer->OnItemStatusChanged( cellIndex, status );
				m_gridRenderer->SetTileParameters( cellIndex, computationParams );
			}
		}
	}

	SEvents::GetInstance().RegisterListener( CNAME( UmbraTileStatusChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( UmbraTomeGenerationProgress ), this );
}

CTileGrid::~CTileGrid()
{
	SEvents::GetInstance().UnregisterListener( CNAME( UmbraTileStatusChanged ), this );
	SEvents::GetInstance().UnregisterListener( CNAME( UmbraTomeGenerationProgress ), this );
}

void CTileGrid::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	struct UpdateStatusTask : public CEdRunnable
	{
		ITileStatusRenderer*		m_statusRenderer;
		CUmbraTile*					m_tile;
		VectorI						m_id;

		virtual void Run()
		{
			if ( m_statusRenderer )
			{
				m_statusRenderer->OnItemStatusChanged( m_id, m_tile->GetStatus() );
			}
		}

		UpdateStatusTask( ITileStatusRenderer* statusRenderer, const VectorI& id, CUmbraTile* tile ) : m_statusRenderer( statusRenderer ), m_id( id ), m_tile( tile ) {}
	};

	struct UpdateProgressTask : public CEdRunnable
	{
		ITileStatusRenderer*	m_statusRenderer;
		VectorI					m_id;
		Float					m_progress;

		virtual void Run()
		{
			if ( m_statusRenderer )
			{
				m_statusRenderer->OnItemProgressChanged( m_id, m_progress );
			}
		}

		UpdateProgressTask( ITileStatusRenderer* statusRenderer, const VectorI& id, Float progress ) : m_statusRenderer( statusRenderer ), m_id( id ), m_progress( progress ) {}
	};

	if ( name == CNAME( UmbraTileStatusChanged ) )
	{
		const STomeDataGenerationContext& context = GetEventData< STomeDataGenerationContext >( data );
		RunLaterEx( new UpdateStatusTask( m_gridRenderer, Move( context.tileId ), context.tile ) );
		RunLaterEx( new UpdateStatusTask( m_queueRenderer, Move( context.tileId ), context.tile ) );
	}
	else if ( name == CNAME( UmbraTomeGenerationProgress ) )
	{
		const VectorI& tileCoords = GetEventData< VectorI >( data );
		Float progress = tileCoords.W == 0 ? 0.0f : (Float)tileCoords.Z / (Float)tileCoords.W;
		RunLaterEx( new UpdateProgressTask( m_gridRenderer, tileCoords, progress ) );
		RunLaterEx( new UpdateProgressTask( m_queueRenderer, tileCoords, progress ) );
	}
}

void CTileGrid::TeleportTo( CEdRenderingPanel* viewport, const VectorI& tile )
{
	Vector position = m_world->GetUmbraScene()->GetGrid().CalcPositionFromCellIndex( tile );
	position.Z = 550.0f;
	viewport->SetCameraPosition( position );
	m_world->UpdateCameraPosition( position );
}

void CTileGrid::RegenerateMultipleTiles( const VectorI& bounds )
{
	GFeedback->ShowWarn( TXT("This feature has been temporarily turned off.") );
}

void CTileGrid::RegenerateMultipleTilesSync( const VectorI& bounds )
{
	GFeedback->ShowWarn( TXT("This feature has been temporarily turned off.") );
}

void CTileGrid::RegenerateSingleTile( const VectorI& id )
{
	m_gridRenderer->SetTileParameters( id, m_context.computationParameters );
	m_context.tileId = id;
	GFeedback->ShowWarn( TXT("This feature has been temporarily turned off.") );
}

void CTileGrid::DumpSceneForTile( const VectorI& id )
{
	m_context.tileId = id;
	//m_world->DumpSceneForTile( m_context );
	GFeedback->ShowWarn( TXT("This feature has been temporarily turned off.") );
}

void CTileGrid::RegenerateSingleTileSync( const VectorI& id )
{
	GFeedback->ShowWarn( TXT("This feature has been temporarily turned off.") );
}

void CTileGrid::CancelRegeneration( const VectorI& id )
{
	GFeedback->ShowWarn( TXT("This feature has been temporarily turned off.") );
}

//////////////////////////////////////////////////////////////////////////

CTileGridViewPanel::CTileGridViewPanel( CEdRenderingPanel* viewport, wxPanel* parent, Float worldSize )
	: wxPanel( parent )
	, m_viewport( viewport )
	, m_worldSize( worldSize )
	, m_bounds( VectorI( -1, -1, -1, -1 ) )
{
}

BEGIN_EVENT_TABLE(CTileGridViewPanel, wxPanel)
	EVT_MOTION(			CTileGridViewPanel::OnMouseMoved		)
	EVT_LEFT_DOWN(		CTileGridViewPanel::OnMouseDown			)
	EVT_LEFT_UP(		CTileGridViewPanel::OnMouseUp			)
	EVT_RIGHT_DOWN(		CTileGridViewPanel::OnMouseRightClick	)

	// catch paint events
	EVT_PAINT(			CTileGridViewPanel::OnPaintEvent		)
END_EVENT_TABLE()

// Called by the system of by wxWidgets when the panel needs to be redrawn.
// Can be triggered by calling Refresh()/Update().
void CTileGridViewPanel::OnPaintEvent( wxPaintEvent & evt )
{
	wxPaintDC dc( this );
	Render( dc );
}
 
void CTileGridViewPanel::ForceRender()
{
	// depending on your system you may need to look at double-buffered dcs
	wxClientDC dc(this);
	Render( dc );
}
 
//Can work no matter what type of DC (e.g. wxPaintDC or wxClientDC) is used.
void CTileGridViewPanel::Render( wxDC& dc )
{
	static VectorI InvalidBounds( -1, -1, -1, -1 );
	if ( m_bounds == InvalidBounds )
	{
		return;
	}

	CTileGridView* sizer = static_cast< CTileGridView* >( GetSizer() );
	ASSERT( sizer );
	if ( !sizer )
	{
		return;
	}

	wxSizerItemList list = sizer->GetChildren();
	
	VectorI tilesCount = sizer->GetTilesCount();
	size_t count = list.GetCount();
	ASSERT( count > 0 && count == tilesCount.X * tilesCount.Y );

	Uint32 topLeftObjectIndex = tilesCount.Y * m_bounds.Y + m_bounds.X;
	wxSizerItem* topLeftObject = list[ topLeftObjectIndex ];
	ASSERT( topLeftObject );

	Uint32 bottomRightObjectIndex = tilesCount.Y * m_bounds.W + m_bounds.Z;
	wxSizerItem* bottomRightObject = list[ bottomRightObjectIndex ];
	ASSERT( bottomRightObject );

	wxRect tlRect = topLeftObject->GetRect();
	wxRect brRect = bottomRightObject->GetRect();

	static const wxPoint offset( 1, 1 );
	wxPoint tl = tlRect.GetLeftTop();
	tl -= offset;
	wxPoint br = brRect.GetBottomRight();
	br += offset;
	wxSize size( br.x - tl.x, br.y - tl.y );

	dc.SetBrush( *wxRED_BRUSH );
	dc.DrawRectangle( tl, size + wxSize( 2, 2 ) );
	
	/*
	static wxPen linePen( *wxRED, 3 );

	Vector position			= m_viewport->GetCameraPosition();
	EulerAngles orientation = m_viewport->GetCameraRotation();

	wxCoord xStart = ( ( position.X + m_worldSize / 2.0f ) / m_worldSize ) * GetSize().GetWidth();
	wxCoord yStart = ( ( position.Y + m_worldSize / 2.0f ) / m_worldSize ) * GetSize().GetHeight();

	Vector2 dir = EulerAngles::YawToVector2( orientation.Yaw );

	wxCoord xEnd = xStart + dir.X * GetSize().GetWidth() * 0.1f;
	wxCoord yEnd = yStart + dir.Y * GetSize().GetHeight() * 0.1f;

	dc.SetPen( linePen );
	dc.DrawLine( xStart, yStart, xEnd, yEnd );
	*/
}
 
void CTileGridViewPanel::OnMouseDown( wxMouseEvent& event )
{
	ForceRender();
}

void CTileGridViewPanel::OnMouseUp(wxMouseEvent& event)
{
	ForceRender();
}

void CTileGridViewPanel::OnMouseMoved(wxMouseEvent& event)
{
}

void CTileGridViewPanel::OnMouseRightClick(wxMouseEvent& event)
{
}

void CTileGridViewPanel::UpdateBounds( Bool minBounds, const VectorI& id )
{
	static VectorI InvalidBounds( -1, -1, -1, -1 );

	if ( m_bounds == InvalidBounds )
	{
		m_bounds.X = id.X;
		m_bounds.Y = id.Y;
		m_bounds.Z = id.X;
		m_bounds.W = id.Y;
	}
	else
	{
		if ( minBounds )
		{
			m_bounds.X = Min< Int32 >( id.X, m_bounds.Z );
			m_bounds.Y = Min< Int32 >( id.Y, m_bounds.W );
		}
		else
		{
			m_bounds.Z = Max< Int32 >( id.X, m_bounds.X );
			m_bounds.W = Max< Int32 >( id.Y, m_bounds.Y );
		}
	}
	Refresh();
}

void CTileGridViewPanel::ClearBounds()
{
	m_bounds = VectorI( -1, -1, -1, -1 );
	Refresh();
}

//////////////////////////////////////////////////////////////////////////

CTileGridView::CTileGridView( CEdRenderingPanel* viewport, CTileGridViewPanel* parent, wxStaticText* hoveredText, const VectorI& tilesCount, Bool isSaperLayout )
	: wxGridSizer( 1, 1, 3, 3 )
	, m_viewport( viewport )
	, m_parent( parent )
	, m_hoveredText( hoveredText )
	, m_tilesCount( tilesCount )
{
	parent->SetSizer( this );
	// load icons

	if ( isSaperLayout )
	{
		m_iconValid			= SEdResources::GetInstance().LoadBitmap( TEXT("IMG_UMBRA_VALID_SAPER") );
		m_iconInvalid		= SEdResources::GetInstance().LoadBitmap( TEXT("IMG_UMBRA_INVALID_SAPER") );
		m_iconInProgress	= SEdResources::GetInstance().LoadBitmap( TEXT("IMG_UMBRA_IN_PROGRESS_SAPER") );
		m_iconUnknown		= SEdResources::GetInstance().LoadBitmap( TEXT("IMG_UMBRA_UNKNOWN_SAPER") );
		m_iconNoData		= SEdResources::GetInstance().LoadBitmap( TEXT("IMG_UMBRA_NO_DATA_SAPER") );
	}
	else
	{
		m_iconValid			= SEdResources::GetInstance().LoadBitmap( TEXT("IMG_UMBRA_VALID") );
		m_iconInvalid		= SEdResources::GetInstance().LoadBitmap( TEXT("IMG_UMBRA_INVALID") );
		m_iconInProgress	= SEdResources::GetInstance().LoadBitmap( TEXT("IMG_UMBRA_IN_PROGRESS") );
		m_iconUnknown		= SEdResources::GetInstance().LoadBitmap( TEXT("IMG_UMBRA_UNKNOWN") );
		m_iconNoData		= SEdResources::GetInstance().LoadBitmap( TEXT("IMG_UMBRA_NO_DATA") );
	}

	SetCols( tilesCount.X );
	SetRows( tilesCount.Y );

	m_items.Resize( tilesCount.X * tilesCount.Y );

	parent->Freeze();
	for ( Int32 j = 0; j < tilesCount.Y; ++j )
	{
		for ( Int32 i = 0; i < tilesCount.X; ++i )
		{
			Uint32 index = j * tilesCount.Y + i;
			wxStaticBitmap* bmp = new wxStaticBitmap( parent, wxID_ANY, m_iconUnknown );
			bmp->SetClientData( new Uint32( index ) );
			bmp->Bind( wxEVT_RIGHT_DOWN, &CTileGridView::OnContextMenu, this );
			bmp->Bind( wxEVT_ENTER_WINDOW, &CTileGridView::OnTileIconHovered, this );
			Add( bmp, 0, wxALL | wxEXPAND );
			
			STileGridViewItem& item = m_items[ index ];
			item.m_bitmap = bmp;
			item.m_index = VectorI( i, j, 0, 0 );
		}
	}

	Layout();
	parent->Thaw();
	Fit( parent );
}

CTileGridView::~CTileGridView()
{
	m_items.Clear();
}

void CTileGridView::OnItemProgressChanged( const VectorI& id, Float progress ) { /* intentionally empty */ }

void CTileGridView::OnItemStatusChanged( const VectorI& id,  EUmbraTileDataStatus status )
{
	size_t index = id.Y * m_tilesCount.Y + id.X;
	ASSERT( index < m_items.Size() );

	const wxBitmap* icon = GetIconForStatus( status );
	m_items[ index ].m_bitmap->SetBitmap( *icon );
}

void CTileGridView::SetTileParameters( const VectorI& id, const SComputationParameters& computationParams )
{
	size_t index = id.Y * m_tilesCount.Y + id.X;
	ASSERT( index < m_items.Size() );

	m_items[ index ].m_parameters = computationParams;
}

const wxBitmap* CTileGridView::GetIconForStatus( EUmbraTileDataStatus status ) const
{
	switch ( status )
	{
	case EUTDS_Unknown:					return &m_iconUnknown;
	case EUTDS_NoData:					return &m_iconNoData;
	case EUTDS_Invalid:					return &m_iconInvalid;
	case EUTDS_ComputationInProgress:	return &m_iconInProgress;
	case EUTDS_Valid:					return &m_iconValid;
	}

	ASSERT( false, TXT( "Invalid status!" ) );
	return nullptr;
}

void CTileGridView::OnContextMenu( wxMouseEvent& event )
{
	wxMenu contextMenu;
	m_lastContextMenuObject = event.GetEventObject();
	contextMenu.Append( wxID_UMBRAEDITOR_TELEPORT, wxT( "Teleport to tile" ) );
	contextMenu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CTileGridView::OnTeleportToTile, this, wxID_UMBRAEDITOR_TELEPORT );
	contextMenu.Append( wxID_UMBRAEDITOR_REGENERATE, wxT( "Regenerate occlusion data" ) );
	contextMenu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CTileGridView::OnRegenerateContext, this, wxID_UMBRAEDITOR_REGENERATE );
	contextMenu.Append( wxID_UMBRAEDITOR_DUMPSCENE, wxT( "Dump UmbraScene data" ) );
	contextMenu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CTileGridView::OnDumpSceneContext, this, wxID_UMBRAEDITOR_DUMPSCENE );
	
	contextMenu.AppendSeparator();

	contextMenu.Append( wxID_UMBRAEDITOR_SET_MIN_BOUNDS, wxT( "Set topleft bounds" ) );
	contextMenu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CTileGridView::OnSetMinBounds, this, wxID_UMBRAEDITOR_SET_MIN_BOUNDS );
	contextMenu.Append( wxID_UMBRAEDITOR_SET_MAX_BOUNDS, wxT( "Set bottomright bounds" ) );
	contextMenu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CTileGridView::OnSetMaxBounds, this, wxID_UMBRAEDITOR_SET_MAX_BOUNDS );
	contextMenu.Append( wxID_UMBRAEDITOR_CLEARBOUNDS, wxT( "Clear bounds" ) );
	contextMenu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CTileGridView::OnClearBounds, this, wxID_UMBRAEDITOR_CLEARBOUNDS );

	contextMenu.AppendSeparator();

	contextMenu.Append( wxID_UMBRAEDITOR_CANCEL_GENERATION, wxT( "Cancel data regeneration" ) );
	contextMenu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CTileGridView::OnCancelDataRegeneration, this, wxID_UMBRAEDITOR_CANCEL_GENERATION );
	
	wxTheFrame->PopupMenu( &contextMenu );
}

void CTileGridView::OnTileIconHovered( wxMouseEvent& event )
{
	static wxString staticLabel( TXT("[unknown]") );
	wxStaticBitmap* bmp = static_cast< wxStaticBitmap* >( event.GetEventObject() );
	if ( bmp )
	{
		Uint32* cData = static_cast< Uint32* >( bmp->GetClientData() );
		if ( cData )
		{
			Uint32 index = *cData;
			ASSERT( index >= 0 && index < m_items.Size() );
			STileGridViewItem& item = m_items[ index ];
			VectorI tileId = item.m_index;	

			String label;
			if ( item.m_parameters.m_smallestHole < 0.0f || item.m_parameters.m_smallestOccluder < 0.0f )
			{
				label = String::Printf( TXT( "[%d; %d]" ), tileId.X, tileId.Y );
			}
			else
			{
				label = String::Printf( TXT( "[%d; %d] ( Smallest hole: %1.3f; Smallest occluder: %1.3f )" ), tileId.X, tileId.Y, item.m_parameters.m_smallestHole, item.m_parameters.m_smallestOccluder );
			}
			m_hoveredText->SetLabel( label.AsChar() );
			return;
		}
	}

	m_hoveredText->SetLabel( staticLabel );
}

void CTileGridView::OnRegenerateContext( wxCommandEvent& event )
{
	if ( m_lastContextMenuObject->IsKindOf( wxCLASSINFO( wxStaticBitmap ) ) )
	{
		wxStaticBitmap* bitmap = static_cast<wxStaticBitmap*>( m_lastContextMenuObject );
		Uint32* cData = static_cast< Uint32* >( bitmap->GetClientData() );
		if ( cData )
		{
			Uint32 index = *cData;
			ASSERT( index >= 0 && index < m_items.Size() );
			VectorI tileId = m_items[ index ].m_index;
			m_dataRegenerator->RegenerateSingleTile( tileId );
		}
	}
}

void CTileGridView::OnDumpSceneContext( wxCommandEvent& event )
{
	if ( m_lastContextMenuObject->IsKindOf( wxCLASSINFO( wxStaticBitmap ) ) )
	{
		wxStaticBitmap* bitmap = static_cast<wxStaticBitmap*>( m_lastContextMenuObject );
		Uint32* cData = static_cast< Uint32* >( bitmap->GetClientData() );
		if ( cData )
		{
			Uint32 index = *cData;
			ASSERT( index >= 0 && index < m_items.Size() );
			VectorI tileId = m_items[ index ].m_index;
			m_dataRegenerator->DumpSceneForTile( tileId );
		}
	}
}

void CTileGridView::OnTeleportToTile( wxCommandEvent& event )
{
	if ( m_lastContextMenuObject->IsKindOf( wxCLASSINFO( wxStaticBitmap ) ) )
	{
		wxStaticBitmap* bitmap = static_cast<wxStaticBitmap*>( m_lastContextMenuObject );
		Uint32* cData = static_cast< Uint32* >( bitmap->GetClientData() );
		if ( cData )
		{
			Uint32 index = *cData;
			ASSERT( index >= 0 && index < m_items.Size() );
			VectorI tileId = m_items[ index ].m_index;
			m_dataRegenerator->TeleportTo( m_viewport, tileId );
		}		
	}
}

void CTileGridView::OnCancelDataRegeneration( wxCommandEvent& event )
{
	if ( m_lastContextMenuObject->IsKindOf( wxCLASSINFO( wxStaticBitmap ) ) )
	{
		wxStaticBitmap* bitmap = static_cast<wxStaticBitmap*>( m_lastContextMenuObject );
		Uint32* cData = static_cast< Uint32* >( bitmap->GetClientData() );
		if ( cData )
		{
			Uint32 index = *cData;
			ASSERT( index >= 0 && index < m_items.Size() );
			VectorI tileId = m_items[ index ].m_index;
			m_dataRegenerator->CancelRegeneration( tileId );
		}
	}
}

void CTileGridView::OnSetMinBounds( wxCommandEvent& event )
{
	if ( m_lastContextMenuObject->IsKindOf( wxCLASSINFO( wxStaticBitmap ) ) )
	{
		wxStaticBitmap* bitmap = static_cast<wxStaticBitmap*>( m_lastContextMenuObject );
		Uint32* cData = static_cast< Uint32* >( bitmap->GetClientData() );
		if ( cData )
		{
			Uint32 index = *cData;
			ASSERT( index >= 0 && index < m_items.Size() );
			VectorI tileId = m_items[ index ].m_index;
			m_parent->UpdateBounds( true, tileId );
		}
	}
}

void CTileGridView::OnSetMaxBounds( wxCommandEvent& event )
{
	if ( m_lastContextMenuObject->IsKindOf( wxCLASSINFO( wxStaticBitmap ) ) )
	{
		wxStaticBitmap* bitmap = static_cast<wxStaticBitmap*>( m_lastContextMenuObject );
		Uint32* cData = static_cast< Uint32* >( bitmap->GetClientData() );
		if ( cData )
		{
			Uint32 index = *cData;
			ASSERT( index >= 0 && index < m_items.Size() );
			VectorI tileId = m_items[ index ].m_index;
			m_parent->UpdateBounds( false, tileId );
		}
	}
}

void CTileGridView::OnClearBounds( wxCommandEvent& event )
{
	m_parent->ClearBounds();
}

//////////////////////////////////////////////////////////////////////////

CQueueItem::CQueueItem( wxWindow* parent, const VectorI& id )
	: m_id( id )
	, m_tileIdLabel( nullptr )
	, m_progressGauge( nullptr )
	, m_progressLabel( nullptr )
	, m_cancelButton( nullptr )
{
	wxXmlResource::Get()->LoadPanel( this, parent, "m_tileProgressPanel" );

	m_parentList = static_cast< CTileRegenerationQueueView* >( parent );

	String label = String::Printf( TXT("[%d; %d]"), m_id.X, m_id.Y );
	m_tileIdLabel = XRCCTRL( *this, "m_tileIdLabel", wxStaticText );
	RED_ASSERT( m_tileIdLabel );
	if ( m_tileIdLabel )
	{
		m_tileIdLabel->SetLabel( label.AsChar() );
	}	
	
	m_progressGauge = XRCCTRL( *this, "m_progressGauge", wxGauge );
	RED_ASSERT( m_progressGauge );
	
	m_progressLabel = XRCCTRL( *this, "m_progressLabel", wxStaticText );
	RED_ASSERT( m_progressLabel );

	m_cancelButton = XRCCTRL( *this, "m_cancelButton", wxBitmapButton );
	RED_ASSERT( m_cancelButton );
	if ( m_cancelButton )
	{
		m_cancelButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CTileRegenerationQueueView::OnCancelButtonClicked ), nullptr, m_parentList );
	}	
}

CQueueItem::~CQueueItem()
{
	RED_ASSERT( m_cancelButton );
	if ( m_cancelButton )
	{
		m_cancelButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CTileRegenerationQueueView::OnCancelButtonClicked ), nullptr, m_parentList );
	}	
}

void CQueueItem::UpdateProgress( Float progress )
{
	Int32 iProgress = (Int32)( 100.0f * progress );
	if ( m_progressGauge )
	{
		m_progressGauge->SetValue( iProgress );
	}	

	String label = String::Printf( TXT("%d%%"), iProgress );
	if ( m_progressLabel )
	{
		m_progressLabel->SetLabel( label.AsChar() );
	}
}

//////////////////////////////////////////////////////////////////////////

CTileRegenerationQueueView::CTileRegenerationQueueView( wxWindow* parent )
	: wxScrolledWindow( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxScrolledWindowStyle | wxCLIP_CHILDREN | wxTAB_TRAVERSAL )
{
	m_parent = static_cast< wxPanel* >( parent );
	ASSERT( m_parent );

	SetSizer( new wxBoxSizer( wxVERTICAL ) );
	SetScrollRate( 5, 5 );
	Layout();
}

void CTileRegenerationQueueView::ItemAdded( const VectorI& id )
{
	CQueueItem* item = new CQueueItem( this, id );
	GetSizer()->Add( item, 0, wxALL|wxEXPAND, 1 );
	GetSizer()->Layout();
	FitInside();
	Layout();

	m_items.PushBack( item );
}

void CTileRegenerationQueueView::ItemDeleted( const VectorI& id )
{
	CQueueItem* item = nullptr;
	for ( Uint32 i = 0; i < m_items.Size(); ++i )
	{
		if ( m_items[ i ]->GetId() == id )
		{
			item = m_items[ i ];
			break;
		}
	}
	
	if ( item )
	{
		Freeze();

		// Destroy custom control
		m_items.Remove( item );
		GetSizer()->Detach( item );
		delete item;

		Layout();
		Thaw();
	}	
}

void CTileRegenerationQueueView::OnItemProgressChanged( const VectorI& id, Float progress )
{
	for ( Uint32 i = 0; i < m_items.Size(); ++i )
	{
		const VectorI& itemId = m_items[ i ]->GetId();
		if ( itemId.X == id.X && itemId.Y == id.Y )
		{
			m_items[ i ]->UpdateProgress( progress );
		}
	}
}

void CTileRegenerationQueueView::OnItemStatusChanged( const VectorI& id, EUmbraTileDataStatus status )
{
	switch ( status )
	{
	case EUTDS_ComputationInProgress:
		ItemAdded( id );
		break;
	case EUTDS_Invalid:
	case EUTDS_Valid:
		ItemDeleted( id );
		break;
	default:
		break;
	}
}

void CTileRegenerationQueueView::SetTileParameters( const VectorI& id, const SComputationParameters& ccomputationParams ) {}

void CTileRegenerationQueueView::OnCancelButtonClicked( wxCommandEvent& event )
{
	CQueueItem* control = (CQueueItem*)( (wxBitmapButton*)event.GetEventObject() )->GetParent();
	if ( control && m_dataRegenerator )
	{
		m_dataRegenerator->CancelRegeneration( control->GetId() );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CEdUmbraTool );

CEdUmbraTool::CEdUmbraTool()
	: m_isStarted( false )
{
}

CEdUmbraTool::~CEdUmbraTool()
{
}

Bool CEdUmbraTool::Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
{
	m_world    = world;
	m_viewport = viewport;

	// figure out dialog size
	m_cellCounts = m_world->GetUmbraScene()->GetGrid().GetCellCounts();
	static const Uint32 ICON_SIZE = 16;
	static const Uint32 X_OFFSET = 100;
	static const Uint32 Y_OFFSET = 200;
	static const Uint32 MIN_SIZE_X = 675;
	static const Uint32 MIN_SIZE_Y = 550;
	wxSize size( Max< Uint32>( m_cellCounts.X * ICON_SIZE + X_OFFSET, MIN_SIZE_X ), Max< Uint32 >( m_cellCounts.Y * ICON_SIZE + Y_OFFSET, MIN_SIZE_Y ) );

	// load and prepare dialog
	m_dialog = wxXmlResource::Get()->LoadDialog( this, "UmbraDialog" );	
	m_dialog->SetSize( size );
	m_dialog->Show();

	// Add event handler for closing the window
	m_dialog->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CEdUmbraTool::OnClose ), nullptr, this );

	InitializeControls();
	InitializeTileStatusGrid();
	
	LoadOptionsFromConfig();
	OnContextChanged(wxCommandEvent(wxEVT_NULL));

	m_isStarted = true;

	return true;
}

void CEdUmbraTool::End()
{
	/*
	if ( m_tileGridView )
	{
		delete m_tileGridView;
		m_tileGridView = nullptr;
	}	

	if ( m_statusPanel )
	{
		delete m_statusPanel;
		m_statusPanel = nullptr;
	}
	*/

	if ( m_tileGrid )
	{
		delete m_tileGrid;
		m_tileGrid = nullptr;
	}

	if ( m_dialog )
	{
		m_dialog->Destroy();
	}

	SaveOptionsToConfig();
	m_isStarted = false;
}

//////////////////////////////////////////////////////////////////////////
// ISavableToConfig begin
void CEdUmbraTool::SaveOptionsToConfig()
{
	ISavableToConfig::SaveSession();
}

void CEdUmbraTool::LoadOptionsFromConfig()
{
	ISavableToConfig::RestoreSession();
}

void CEdUmbraTool::SaveSession( CConfigurationManager &config )
{
	if ( m_isStarted )
	{
		wxString SO = m_smallestOccluderCtrl->GetStringSelection();
		wxString SH = m_smallestHoleCtrl->GetStringSelection();
		wxString TS = m_umbraTileSizeCtrl->GetStringSelection();
		
		config.Write( TXT("Tools/Umbra/SmallestOccluder"),	SO.wc_str() );
		config.Write( TXT("Tools/Umbra/SmallestHole"),		SH.wc_str() );
		config.Write( TXT("Tools/Umbra/TileSize"),			TS.wc_str() );
	}	
}

void CEdUmbraTool::RestoreSession( CConfigurationManager &config )
{
	String SO = config.Read( TXT("Tools/Umbra/SmallestOccluder"),	TXT("") );
	String SH = config.Read( TXT("Tools/Umbra/SmallestHole"),		TXT("") );
	String TS = config.Read( TXT("Tools/Umbra/TileSize"),			TXT("") );

	m_smallestOccluderCtrl->SetStringSelection( SO.AsChar() );
	m_smallestHoleCtrl->SetStringSelection( SH.AsChar() );
	m_umbraTileSizeCtrl->SetStringSelection( TS.AsChar() );
}
// ISavableToConfig end
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// IEdEventListener begin

// IEdEventListener end
//////////////////////////////////////////////////////////////////////////

void CEdUmbraTool::OnClose( wxCloseEvent& event )
{
	RunLaterOnce( [](){ wxTheFrame->GetToolsPanel()->CancelTool(); } );
	event.Veto();
}

void CEdUmbraTool::OnRegenerateSelected( wxCommandEvent& event )
{
	m_tileGrid->RegenerateMultipleTilesSync( m_statusPanel->GetBounds() );
}

void CEdUmbraTool::OnRegenerateAll( wxCommandEvent& event )
{
#if 0
	TDynArray< VectorI > tiles;

	Uint32 startI, startJ, endI, endJ;
	ParseTileRanges( startI, startJ, endI, endJ );
	GenerateTileList( startI, startJ, endI, endJ, tiles, false );

	m_tileGrid->Regenerate( tiles );
#else
	VectorI bounds( -1, -1, -1, -1 );
	m_tileGrid->RegenerateMultipleTilesSync( bounds );
#endif
}

void CEdUmbraTool::OnRegenerateInvalid( wxCommandEvent& event )
{
	/*
	TDynArray< VectorI > tiles;

	Uint32 startI, startJ, endI, endJ;
	ParseTileRanges( startI, startJ, endI, endJ );
	GenerateTileList( startI, startJ, endI, endJ, tiles );

	m_tileGrid->RegenerateMultipleTiles( tiles );
	*/
}

void CEdUmbraTool::OnContextChanged( wxCommandEvent& event )
{
	Float smallestOccluder;
	FromString< Float >( String( m_smallestOccluderCtrl->GetStringSelection().wchar_str() ),	smallestOccluder );
	Float smallestHole;
	FromString< Float >( String( m_smallestHoleCtrl->GetStringSelection().wchar_str() ),		smallestHole );
	Float umbraTileSize;
	FromString< Float >( String( m_umbraTileSizeCtrl->GetStringSelection().wchar_str() ),		umbraTileSize );
	
	SComputationParameters computationParams( smallestOccluder, smallestHole, m_tileSize, umbraTileSize );
	STomeDataGenerationContext context;
	context.computationParameters = computationParams;
	m_tileGrid->SetTomeDataGenerationContext( context );
}

void CEdUmbraTool::InitializeControls()
{
	Vector tileSize = m_world->GetUmbraScene()->GetGrid().GetCellSizes();
	ASSERT( tileSize.X == tileSize.Y );
	m_tileSize = tileSize.X;

	VectorI tileCounts = m_world->GetUmbraScene()->GetGrid().GetCellCounts();

	// initialize controls
	m_smallestOccluderCtrl		= XRCCTRL( *this, "m_choiceSmallestOccluder",	wxChoice );			ASSERT( m_smallestOccluderCtrl );
	m_smallestOccluderCtrl->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdUmbraTool::OnContextChanged ), nullptr, this );
	m_smallestHoleCtrl			= XRCCTRL( *this, "m_choiceSmallestHole",		wxChoice );			ASSERT( m_smallestHoleCtrl );
	m_smallestHoleCtrl->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdUmbraTool::OnContextChanged ), nullptr, this );
	m_umbraTileSizeCtrl			= XRCCTRL( *this, "m_choiceTileSize",			wxChoice );			ASSERT( m_umbraTileSizeCtrl );
	m_umbraTileSizeCtrl->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdUmbraTool::OnContextChanged ), nullptr, this );

	m_regenerateSelectedButton	= XRCCTRL( *this, "m_regenerateSelectedButton",	wxButton );			ASSERT( m_regenerateSelectedButton );
	m_regenerateAllButton		= XRCCTRL( *this, "m_regenerateAllButton",		wxButton );			ASSERT( m_regenerateAllButton );
	m_regenerateInvalidButton	= XRCCTRL( *this, "m_regenerateInvalidButton",	wxButton );			ASSERT( m_regenerateInvalidButton );
	m_hoveredText				= XRCCTRL( *this, "m_hoveredTile",				wxStaticText );		ASSERT( m_hoveredText );

	Bool isSaper = m_randomGenerator.Get( 0, 3 ) == 1;

	wxPanel* parentPanel = XRCCTRL( *this, "m_panel7", wxPanel ); ASSERT( parentPanel );

	SClipmapParameters params;
	m_world->GetTerrain()->GetClipmapParameters( &params );
	m_statusPanel = new CTileGridViewPanel( m_viewport, parentPanel, params.terrainSize );

	m_tileGridView = new CTileGridView( m_viewport, m_statusPanel, m_hoveredText, tileCounts, isSaper );

	ASSERT( parentPanel->GetSizer() );
	parentPanel->GetSizer()->Add( m_statusPanel, 0, wxALL | wxEXPAND, 5 );
	parentPanel->GetSizer()->Layout();

	wxPanel* tileQueuePanel = XRCCTRL( *m_dialog, "m_tileQueuePanel", wxPanel ); ASSERT( tileQueuePanel );
	m_queueView = new CTileRegenerationQueueView( tileQueuePanel );
	tileQueuePanel->GetSizer()->Add( m_queueView, 1, wxEXPAND | wxALL, 5 );
	tileQueuePanel->GetSizer()->Layout();

	m_regenerateSelectedButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdUmbraTool::OnRegenerateSelected ), nullptr, this );
	m_regenerateAllButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdUmbraTool::OnRegenerateAll ), nullptr, this );
	m_regenerateInvalidButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdUmbraTool::OnRegenerateInvalid ), nullptr, this );
}

void CEdUmbraTool::InitializeTileStatusGrid()
{
	m_tileGrid = new CTileGrid( m_world, m_tileGridView, m_queueView );
}

#endif // NO_UMBRA_DATA_GENERATION
#endif // USE_UMBRA