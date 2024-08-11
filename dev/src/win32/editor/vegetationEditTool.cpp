/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "toolsPanel.h"
#include "vegetationEditTool.h"
#include "wxThumbnailImageLoader.h"
#include "undoVegetation.h"
#include "autoSizeListCtrl.h"

#include "../../common/engine/foliageEditionController.h"
#include "../../common/engine/foliageBroker.h"
#include "../../common/engine/grassMask.h"
#include "../../common/engine/vegetationBrush.h"
#include "../../common/engine/clipMap.h"
#include "../../common/engine/renderCommands.h"
#include "../../common/core/configFileManager.h"
#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/baseTree.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/foliageCellIterator.h"

IMPLEMENT_ENGINE_CLASS( CEdVegetationEditTool );

//#define USE_TERRAIN_NORMAL

namespace
{
	const Float MIN_BRUSH_SIZE = 0.1f;
	const Float MAX_BRUSH_SIZE = 100.0f;
	const Int32 MAX_CELL_SELECTION_RADIUS = 10;
	const Float MAX_PAINTING_DISTANCE = 200.0;

	// Utility function that return global coordinates of a point defined by the on-plane coordinates (relative to center)
	Vector GetPointOnPlane( const Vector& center, const Vector& normal, Float dx, Float dy )
	{
		if ( Vector::Near3( normal, Vector( 0.f, 0.f, 1.f ) ) )
		{
			return center + Vector( dx, dy, 0.0f );
		}
		else
		{
			const Vector u = Vector( normal.Y, -normal.X, 0 ).Normalized3();
			const Vector v = Vector::Cross( u, normal );
			return center + u*dx + v*dy;
		}
	}

	template < typename T >
	T FromStringDef( const String& str, T defVal )
	{
		T res;
		if ( FromString( str, res ) )
		{
			return res;
		}
		else
		{
			return defVal;
		}
	}

	Matrix GetInstanceRotationMatrix( const SFoliageInstance& instance )
	{
		Vector quaternion = instance.GetQuaterion();
		quaternion.W = -quaternion.W;// ctremblay I'm not sure why W is inverted here. Need to investigate further. Either ToQuat, matrix.ToEulerAngles or matrixBuildFromQuaternion is/are broken 
		return Matrix().BuildFromQuaternion( quaternion );
	}

	Bool ScaleInstanceFilter( const SFoliageInstance& inst, const TOptional< TPair< Float, Float > >& scaleRange )
	{
		return scaleRange.IsInitialized() 
			? ( inst.GetScale() >= scaleRange.Get().m_first && inst.GetScale() <= scaleRange.Get().m_second )
			: true;
	}

	Bool ScaleAndDecInstanceFilter( const SFoliageInstance& inst, Float decAmount, Red::Math::Random::Generator< Red::Math::Random::StandardRand >& randGen, const TOptional< TPair< Float, Float > >& scaleRange )
	{
		return ScaleInstanceFilter( inst, scaleRange ) && randGen.Get( 0.f, 1.f ) < decAmount;
	}
		
	class CEdReplaceIntancesDialog : private wxDialog
	{
		DECLARE_EVENT_TABLE()

	public:

		CEdReplaceIntancesDialog( wxWindow* parent )
		{
			wxXmlResource::Get()->LoadDialog( this, parent, wxT("m_replaceInstancesDialog") );
			m_resetScaleCB    = XRCCTRL( *this, "m_resetScaleCB"   , wxCheckBox );
			m_resetScaleVal   = XRCCTRL( *this, "m_resetScaleVal",   wxTextCtrl );
		}

		struct Info
		{
			Info() 
				: resetScale( false )
				, resetScaleVal( 1. )
				{}

			Bool   resetScale;
			Float  resetScaleVal;
		};

		Bool Execute( Info& info )
		{
			m_resetScaleCB   ->SetValue( info.resetScale );
			m_resetScaleVal  ->SetValue( ToString( info.resetScaleVal ).AsChar() );

			if ( ShowModal() == wxID_OK )
			{
				info.resetScale    = m_resetScaleCB->GetValue();
				info.resetScaleVal = FromStringDef( String( m_resetScaleVal->GetValue().c_str() ), 1.0f );
				return true;
			}
			else
			{
				return false;
			}
		}

	private:
		void OnUpdateUI( wxUpdateUIEvent& event )
		{
			m_resetScaleVal->Enable( m_resetScaleCB->GetValue() );
		}

		wxCheckBox* m_resetScaleCB;
		wxTextCtrl* m_resetScaleVal;
	};

	BEGIN_EVENT_TABLE( CEdReplaceIntancesDialog, wxDialog )
		EVT_UPDATE_UI( wxID_ANY, CEdReplaceIntancesDialog::OnUpdateUI )
	END_EVENT_TABLE()
}

//////////////////////////////////////////////////////////////////////////

CEdBrushEntry::CEdBrushEntry( wxWindow* parent, CVegetationBrushEntry* entry )
	: CEdCustomControl( parent, "m_brushEntryPanel", true, true )
	, m_entry ( entry )
	, m_thumbnailImage( NULL )
	, m_instanceCount( 0 )
{
	m_thumbnail = static_cast< wxStaticBitmap* >( FindWindow( "m_thumbnail" ) );
	m_thumbnail->Connect( wxEVT_PAINT, wxPaintEventHandler( CEdBrushEntry::OnPaintThumbnail ), NULL, this );

	DoSetup();

	wxTextCtrl* radiusScaleTextCtrl = XRCCTRL( *this, "m_radiusScaleText", wxTextCtrl );
	wxTextCtrl* sizeTextCtrl = XRCCTRL( *this, "m_sizeText", wxTextCtrl );
	wxTextCtrl* sizeVarTextCtrl = XRCCTRL( *this, "m_sizeVarText", wxTextCtrl );

	m_radiusScaleEditor.Init( radiusScaleTextCtrl, &m_entry->m_radiusScale, m_entry );
	m_sizeEditor.Init( sizeTextCtrl, &m_entry->m_size, m_entry );
	m_sizeVarEditor.Init( sizeVarTextCtrl, &m_entry->m_sizeVar, m_entry );
}

void CEdBrushEntry::OnPaintThumbnail( wxPaintEvent& event )
{
	if ( m_thumbnailImage )
	{
		wxPaintDC dc( m_thumbnail );
		HDC hdc = (HDC)dc.GetHDC();

		Gdiplus::Graphics graphics( hdc );

		graphics.DrawImage( m_thumbnailImage->GetBitmap(), 0, 0, m_thumbnail->GetSize().GetWidth(), m_thumbnail->GetSize().GetHeight() );
	}
}

void CEdBrushEntry::UpdateThumbnail()
{
	RED_ASSERT( m_thumbnail );

	if ( m_entry->m_resource->GetFile()->LoadThumbnail() )
	{
		TDynArray< CThumbnail* > thumbnails = m_entry->m_resource->GetFile()->GetThumbnails();
		if ( !thumbnails.Empty() )
		{
			CThumbnail* thumbnail = thumbnails[0];
			m_thumbnailImage = (CWXThumbnailImage*)thumbnail->GetImage();
		}
	}
}

void CEdBrushEntry::DoSetup()
{
	wxStaticText* nameLabel = XRCCTRL( *this, "m_resourceLabel", wxStaticText );
	nameLabel->SetLabel( m_entry->m_resource->GetFile()->GetFileName().StringBefore( TXT(".") ).AsChar() );

	wxString typeStr = m_entry->m_resource->IsGrassType() ? TXT("Grass") : TXT("Tree");
	
	if ( m_instanceCount != 0 )
	{
		typeStr += wxString::Format( TXT(", %i instances"), m_instanceCount );
	}

	wxStaticText* typeLabel = XRCCTRL( *this, "m_resourceType", wxStaticText );
	typeLabel->SetLabel( typeStr );

	UpdateThumbnail();
}

void CEdBrushEntry::SetInstanceCount( Uint32 instanceCount )
{
	m_instanceCount = instanceCount;
	DoSetup();
}

//////////////////////////////////////////////////////////////////////////

CEdBrush::CEdBrush( wxWindow* parent, CVegetationBrush* brush )
	: CEdCustomControl( parent, "m_brushDescPanel", true, true )
	, m_brush( brush )
	, m_isLocked( false )
{
	m_brush->AddToRootSet();
	m_lockBitmap = XRCCTRL( *this, "m_lockBitmap", wxStaticBitmap );
	m_lockBitmap->Hide();
	DoSetup();
}

CEdBrush::~CEdBrush()
{
	if ( m_brush && m_brush->IsInRootSet() /*May be externally discarded (e.g. by reloading)*/ )
	{
		m_brush->RemoveFromRootSet();
	}
}

void CEdBrush::DoSetup()
{
	wxStaticText* nameLabel = static_cast< wxStaticText* >( FindWindow( "m_brushLabel" ) );
	RED_ASSERT( nameLabel );
	if ( m_brush->GetFile() )
	{
		nameLabel->SetLabel( m_brush->GetFile()->GetFileName().StringBefore( TXT(".") ).AsChar() );
	}
	else
	{
		nameLabel->SetLabel( TXT("Unsaved brush") );
	}
}

void CEdBrush::SwitchLock()
{
	if ( m_isLocked )
	{
		m_isLocked = false;
		m_lockBitmap->Hide();
	}
	else
	{
		m_isLocked = true;
		m_lockBitmap->Show();
	}
}

//////////////////////////////////////////////////////////////////////////

CEdBrushEntriesPanel::CEdBrushEntriesPanel( wxWindow* parent, CVegetationBrush* brush, CEdVegetationEditTool* tool )
	: CEdCustomControlListPanel( parent, true )
	, m_tool( tool )
{
}

void CEdBrushEntriesPanel::SetBrush( CVegetationBrush* brush )
{
	m_theBrush = brush;

	Freeze();

	Clear();

	if ( m_theBrush )
	{
		for ( CVegetationBrushEntry* entry : m_theBrush->m_entries )
		{
			CEdCustomControl* panel = AddItem( entry->m_resource.Get() );
			panel->Select( false );
		}
	}

	//Layout();

	Thaw();
}

CClass* CEdBrushEntriesPanel::GetExpectedObjectClass() const
{
	return CSRTBaseTree::GetStaticClass();
}

CEdCustomControl* CEdBrushEntriesPanel::CreateCustomControl( CObject* object )
{
	CSRTBaseTree* baseTree = SafeCast< CSRTBaseTree >( object );
	CVegetationBrushEntry* entry = m_theBrush->AddEntry( baseTree );
	return new CEdBrushEntry( this, entry );
}

void CEdBrushEntriesPanel::OnAddItem( CObject* object, CEdCustomControl* customControl )
{
	RED_ASSERT( object );
	RED_ASSERT( customControl );
	RED_ASSERT( m_theBrush );
}

Bool CEdBrushEntriesPanel::OnItemRemove( CObject* object, CEdCustomControl* customControl )
{
	CVegetationBrushEntry* entry = m_theBrush->FindEntry( SafeCast< CSRTBaseTree >( object ) );
	if ( m_theBrush->MarkModified() )
	{
		m_theBrush->m_entries.Remove( entry );
		return true;
	}

	// Can't modify
	return false;
}

Bool CEdBrushEntriesPanel::CanAddItem( CObject* object ) const
{
	CSRTBaseTree* baseTree = SafeCast< CSRTBaseTree >( object );
	if ( m_theBrush->FindEntry( baseTree ) )
	{
		// Already there
		return false;
	}

	if ( !m_theBrush->MarkModified() )
	{
		// Can't modify
		return false;
	}

	// Can add
	return true;
}

void CEdBrushEntriesPanel::PostItemSelection( CObject* object, CEdCustomControl* customControl )
{
	// nothing atm
}

void CEdBrushEntriesPanel::OnItemHovered( CEdCustomControl* control )
{
	CObject* object = FindObject( control );
	RED_ASSERT( object, TXT("Some faulty behavior in the custom control list. It seems that the non-registered item was hovered. Fix it ASAP.") );

	CSRTBaseTree* baseTree = SafeCast< CSRTBaseTree >( object );
	RED_ASSERT( baseTree, TXT("A base tree shouldn't be here if it was deleted or smth.") );
	
	m_tool->UpdateThumbnailSource( baseTree->GetFile() );
}

void CEdBrushEntriesPanel::OnMouseEnter( wxMouseEvent& event )
{
	// We entered the list panel, which means we left an entry panel. Hide the thumbnail then.
	m_tool->UpdateThumbnailSource( NULL );
}

void CEdBrushEntriesPanel::SelectItems( const TDynArray< CSRTBaseTree* >& baseTrees )
{
	DeselectAll();

	for ( CSRTBaseTree* tree : baseTrees )
	{
		SelectItem( tree );
	}

	Refresh();
}

void CEdBrushEntriesPanel::RefreshStats( const TDynArray< SFoliageInstanceStatistics >& stats )
{
	if ( !m_theBrush )
	{
		return;
	}

	TDynArray< CVegetationBrushEntry* > entries;
	m_theBrush->GetEntries( entries );

	for ( CVegetationBrushEntry* entry : entries )
	{
		CSRTBaseTree* tree = entry->GetBaseTree();

		CEdBrushEntry* panel = static_cast< CEdBrushEntry* >( FindPanel( tree ) );
		ASSERT( panel );

		auto statIt = FindIf( stats.Begin(), stats.End(), [ tree ]( const SFoliageInstanceStatistics& s ) { return s.m_baseTree.Get() == tree; } );

		if ( statIt != stats.End() )
		{
			panel->SetInstanceCount( statIt->m_instanceCount );
		}
		else
		{
			panel->SetInstanceCount( 0 );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CEdBrushesPanel::CEdBrushesPanel( wxWindow* parent, CEdVegetationEditTool* tool )
	: CEdCustomControlListPanel( parent, false )
	, m_tool( tool )
{
}

CClass* CEdBrushesPanel::GetExpectedObjectClass() const
{
	return CVegetationBrush::GetStaticClass();
}

CEdCustomControl* CEdBrushesPanel::CreateCustomControl( CObject* object )
{
	return new CEdBrush( this, SafeCast< CVegetationBrush >( object ) );
}

void CEdBrushesPanel::OnAddItem( CObject* object, CEdCustomControl* customControl )
{
	RED_ASSERT( object );
	RED_ASSERT( customControl );

	CVegetationBrush* brush = SafeCast< CVegetationBrush >( object );
	m_brushes.PushBack( brush );
	m_paths.PushBack( brush->GetFile() ? brush->GetFile()->GetDepotPath() : String::EMPTY );
}

Bool CEdBrushesPanel::OnItemRemove( CObject* object, CEdCustomControl* customControl )
{
	if ( customControl->IsSelected() )
	{
		// Deselect it if it was selected
		m_tool->SetActiveBrush( NULL );
	}

	// Remove from our internal list
	ptrdiff_t idx = m_brushes.GetIndex( SafeCast< CVegetationBrush >( object ) );
	ASSERT ( idx != -1 );
	m_brushes.RemoveAt( idx );
	m_paths.RemoveAt( idx );

	return true;
}

CVegetationBrush* CEdBrushesPanel::AddBrush( const String& depotPath )
{
	CDiskFile* diskFile = GDepot->FindFile( depotPath );
	if ( diskFile && diskFile->Load( ResourceLoadingContext() ) )
	{
		CResource* resource = diskFile->GetResource();
		if ( CVegetationBrush* brush = SafeCast< CVegetationBrush >( resource ) )
		{
			AddItem( brush );
			return brush;
		}
	}

	return nullptr;
}

void CEdBrushesPanel::Rebuild()
{
	String prevActivePath = m_activePath;

	Freeze();

	m_tool->SetActiveBrush( nullptr );
	Clear();

	TDynArray< String > pathsToRestore = m_paths;
	m_brushes.Clear();
	m_paths.Clear();

	for ( const String& depotPath : pathsToRestore )
	{
		CVegetationBrush* brush = AddBrush( depotPath );
		if ( depotPath == prevActivePath )
		{
			SelectItem( brush ); // try to keep selection
		}
	}

	//Layout();
	Thaw();
}

Bool CEdBrushesPanel::CanAddItem( CObject* object ) const
{
	CVegetationBrush* brush = SafeCast< CVegetationBrush >( object );
	for ( Uint32 i=0; i<m_brushes.Size(); ++i )
	{
		if ( m_brushes[i] == brush )
		{
			// Already there
			return false;
		}
	}

	// Good to go
	return true;
}

void CEdBrushesPanel::PostItemSelection( CObject* object, CEdCustomControl* customControl )
{
	if ( object )
	{
		CVegetationBrush* brush = SafeCast< CVegetationBrush >( object );
		m_activePath = brush->GetDepotPath();
		m_tool->SetActiveBrush( brush );
	}
	else
	{
		m_activePath = String::EMPTY;
		m_tool->SetActiveBrush( nullptr );
	}
}

void CEdBrushesPanel::PerformBrushLock()
{
	for ( THandle< CVegetationBrush > brush : m_brushes )
	{
		if ( CEdBrush* panel = static_cast< CEdBrush* >( FindPanel( brush.Get() ) ) )
		{
			if ( panel->IsSelected() || panel->Islocked() ) //Switch if selected, or if not selected but locked
			{
				panel->SwitchLock();
			}
		}
	}

	Layout();
	Refresh();
}

CVegetationBrush* CEdBrushesPanel::GetLockedBrush()
{
	for ( THandle< CVegetationBrush > brush : m_brushes )
	{
		if ( CEdBrush* panel = static_cast< CEdBrush* >( FindPanel( brush.Get() ) ) )
		{
			if ( panel->Islocked() )
			{
				return brush.Get();
			}
		}
	}

	// No locked brush
	return nullptr;
}

const TDynArray< THandle< CVegetationBrush > >& CEdBrushesPanel::GetCurrentBrushes() const
{
	return m_brushes;
}

//////////////////////////////////////////////////////////////////////////

CEdVegetationEditTool::SSelectedInstanceDesc::SSelectedInstanceDesc()
	: m_baseTree( nullptr )
{}

CEdVegetationEditTool::SSelectedInstanceDesc::SSelectedInstanceDesc( CSRTBaseTree* baseTree, const SFoliageInstance & instance )
	: m_baseTree( baseTree )
	, m_instance( instance )
{
	RED_ASSERT( baseTree );
	m_position = instance.GetPosition(); // store separately to have a diff after a transform
}

Bool CEdVegetationEditTool::SSelectedInstanceDesc::IsEmpty() const 
{
	return m_baseTree == nullptr; 
}

void CEdVegetationEditTool::SSelectedInstanceDesc::Reset() 
{ 
	*this = SSelectedInstanceDesc(); 
}

Bool CEdVegetationEditTool::SSelectedInstanceDesc::operator ==( const SSelectedInstanceDesc& r ) const
{
	return m_baseTree == r.m_baseTree && m_instance == r.m_instance;
}

Bool CEdVegetationEditTool::SSelectedInstanceDesc::operator !=( const SSelectedInstanceDesc& r ) const
{
	return !operator==( r ); 
}

//////////////////////////////////////////////////////////////////////////

CEdVegetationEditTool::SSelectedCellDesc::SSelectedCellDesc()
	: m_centerBox( Box::EMPTY )
	, m_radius( 0 )
{
}

CEdVegetationEditTool::SSelectedCellDesc::SSelectedCellDesc( const Box& centerBox, Int32 radius )
	: m_centerBox( centerBox )
	, m_radius( radius )
{
}

Bool CEdVegetationEditTool::SSelectedCellDesc::IsEmpty() const
{
	return m_radius == 0;
}

void CEdVegetationEditTool::SSelectedCellDesc::Reset()
{
	*this = SSelectedCellDesc();
}

Bool CEdVegetationEditTool::SSelectedCellDesc::operator ==( const SSelectedCellDesc& r ) const 
{ 
	return m_centerBox == r.m_centerBox && m_radius == r.m_radius; 
}

Bool CEdVegetationEditTool::SSelectedCellDesc::operator !=( const SSelectedCellDesc& r ) const 
{ 
	return !operator==( r ); 
}


Box CEdVegetationEditTool::SSelectedCellDesc::GetTotalBox() const
{
	if ( m_radius == 0 )
	{
		return Box::EMPTY;
	}
	else
	{
		Vector size = m_centerBox.CalcSize();
		Vector min = m_centerBox.Min - size * (m_radius - 1);
		Vector max = m_centerBox.Max + size * (m_radius - 1) - 0.01f; // subtract some epsilon not to touch next cells
		min.Z = max.Z = 0; // shut up, assert!
		return Box( min, max );
	}
}

//////////////////////////////////////////////////////////////////////////

CEdVegetationEditTool::CEdVegetationEditTool() 
	: CEdControlVertexHandler( true, true )
	, m_world( nullptr )
	, m_viewport( nullptr )
	, m_dialog( nullptr )
	, m_isStarted( false )
	, m_activeBrush( nullptr )
	, m_brushesListPanel( nullptr )
	, m_brushEntriesPanel( nullptr )
	, m_cursorMode( VPM_None )
	, m_isPaintingNow( false )
	, m_paintBrushSize( 1.0f )
	, m_paintBrushStrength( 1.0f )
	, m_paintTimer( 0.0f )
	, m_rendStatsTimer( 0.5f )
	, m_thumbnailBitmap( nullptr )
	, m_thumbnailSource( nullptr )
	, m_cursorPlacement( VCP_Nowhere )
	, m_paintOnMode( POM_Terrain )
	, m_cursorQuickPickMode( false )
	, m_cellStatisticCursorRadius( 2 )
	, m_statsSorting( StatsSortType::ByCountDown )
{
}

CEdVegetationEditTool::~CEdVegetationEditTool()
{
}

String CEdVegetationEditTool::GetCaption() const
{
	return TXT("Vegetation Edit Tools");
}

EVegetationEditorMode CEdVegetationEditTool::GetToolMode() const
{
	int pageIdx = XRCCTRL( *m_dialog, "m_notebook", wxNotebook )->GetSelection();

	if ( pageIdx == 0 )
	{
		if ( m_cursorMode == VCM_PickInstance )
		{
			return VTM_InstanceManipulation;
		}
		else
		{
			return VTM_Painting;
		}
	}
	else if ( pageIdx == 1 )
	{
		return VTM_Statistics;
	}

	return VTM_Painting; // shouldn't get here
}

void CEdVegetationEditTool::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == RED_NAME( ActiveWorldSaving ) )
	{
		if ( m_viewport && m_world )
		{
			// Redo queue needs to be cleared before saving to not include the undone entities in the save.
			m_world->GetFoliageEditionController().FlushTransaction();
			// After this the redo won't work properly anyway, so disable it.
			m_viewport->GetUndoManager()->ClearHistory( true );
		}
	}
 	else if ( name == CNAME( ReloadBaseTree ) )
	{
		const CReloadBaseTreeInfo& reloadInfo = GetEventData< CReloadBaseTreeInfo >( data );

		if ( m_brushesListPanel )
		{
			if ( reloadInfo.m_resourceToReload->IsA< CVegetationBrush >() )
			{
				m_brushesListPanel->Rebuild();
			}
		}

		if ( CFoliageResource* resource = Cast< CFoliageResource >( reloadInfo.m_resourceToReload ) )
		{
			if ( m_isStarted )
			{
				// TODO: need to update all resource references in the engine
				m_highlightedCell.Reset();
				m_selectedCell.Reset();
				UpdateStatistics( m_selectedCell );
				UpdateRealTimeStatistics();
				UpdatePickShapes();
			}
		}
	}
	else if ( name == CNAME( EditorPostUndoStep ) )
	{
		RunLaterOnce( [ this ]() 
		{ 
			if ( m_isStarted )
			{
				UpdateRealTimeStatistics( true ); 
				UpdatePickShapes();
			}
		} );
	}
	else if ( name == CNAME( ActiveWorldChanged ) )
	{
		if ( m_isStarted )
		{
			m_highlightedCell.Reset();
			m_selectedCell.Reset();
			UpdateStatistics( m_selectedCell );
			UpdateRealTimeStatistics();
			UpdatePickShapes();
		}
	}
}


Bool CEdVegetationEditTool::Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
{
	// No world
	if ( !world )
	{
		return false;
	}

	m_brushSaveDialog.SetMultiselection( false );
	m_brushSaveDialog.AddFormat( ResourceExtension< CVegetationBrush >(), TXT("RED Vegetation Brush") );

	String depotDir;
	GDepot->GetAbsolutePath( depotDir );
	m_brushSaveDialog.SetDirectory( depotDir );

	SEvents::GetInstance().RegisterListener( RED_NAME( ActiveWorldSaving ), this );
	SEvents::GetInstance().RegisterListener( RED_NAME( ReloadBaseTree ), this );
	SEvents::GetInstance().RegisterListener( RED_NAME( EditorPostUndoStep ), this );
	SEvents::GetInstance().RegisterListener( RED_NAME( ActiveWorldChanged ), this );

	// Remember what we are editing
	m_world = world;

	// Remember render panel containing terrain view
	m_viewport = viewport;

	viewport->GetSelectionManager()->SetGranularity( CSelectionManager::SG_Entities );

	// Load layout from XRC
	//m_dialog = wxXmlResource::Get()->LoadDialog( this, "m_vegetationToolPanel" );	
	m_dialog = wxXmlResource::Get()->LoadFrame( wxTheFrame, "m_vegetationToolFrame" );	

	m_dialog->Bind( wxEVT_CLOSE_WINDOW, &CEdVegetationEditTool::OnClose, this );
	m_dialog->Bind( wxEVT_UPDATE_UI, &CEdVegetationEditTool::OnUpdateUI, this );

	// Create brushes custom control list (TM Drey) panels
	{
		wxPanel* brushPanelParent = XRCCTRL( *m_dialog, "m_brushesParentPanel", wxPanel );
		m_brushesListPanel = new CEdBrushesPanel( brushPanelParent, this );
		brushPanelParent->GetSizer()->Add( m_brushesListPanel, 1, wxEXPAND | wxALL, 5 );
	}

	{
		wxPanel* brushEntriesPanelParent = XRCCTRL( *m_dialog, "m_brushEntriesParentPanel", wxPanel );
		m_brushEntriesPanel = new CEdBrushEntriesPanel( brushEntriesPanelParent, m_activeBrush.Get(), this );
		brushEntriesPanelParent->GetSizer()->Add( m_brushEntriesPanel, 1, wxEXPAND | wxALL, 5 );
	}

	m_statisticsLabel = XRCCTRL( *m_dialog, "m_realTimeStatistics", wxStaticText );

	wxNotebook* noteBook = XRCCTRL( *m_dialog, "m_notebook", wxNotebook );
	noteBook->Bind( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, &CEdVegetationEditTool::OnPageChanged, this );
	noteBook->SetSelection( 0 ); // in case it was changed in FormBuilder by mistake
	
	// Grab paint mode toggle buttons
	{
		m_modeButtons[ VCM_PaintInstances ] = XRCCTRL( *m_dialog, "m_paintVegetationToggle", wxToggleButton );
		m_modeButtons[ VCM_PaintInstances ]->Bind( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, &CEdVegetationEditTool::OnToggleModeClicked, this );

		m_modeButtons[ VCM_PaintSize ] = XRCCTRL( *m_dialog, "m_paintSizeToggle", wxToggleButton );
		m_modeButtons[ VCM_PaintSize ]->Bind( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, &CEdVegetationEditTool::OnToggleModeClicked, this );

		m_modeButtons[ VCM_PaintAligment ] = XRCCTRL( *m_dialog, "m_paintAligment", wxToggleButton );
		m_modeButtons[ VCM_PaintAligment ]->Bind( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, &CEdVegetationEditTool::OnToggleModeClicked, this );

		m_modeButtons[ VCM_PickInstance ] = XRCCTRL( *m_dialog, "m_pickInstanceToggle", wxToggleButton );
		m_modeButtons[ VCM_PickInstance ]->Bind( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, &CEdVegetationEditTool::OnToggleModeClicked, this );

		m_modeButtons[ VCM_PaintGrassMask ] = XRCCTRL( *m_dialog, "m_paintGenericGrassMaskToggle", wxToggleButton );
		m_modeButtons[ VCM_PaintGrassMask ]->Bind( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, &CEdVegetationEditTool::OnToggleModeClicked, this );
	}

	// Connect tool buttons
	{
		wxToolBar* toolBar = XRCCTRL( *m_dialog, "m_brushesToolBar", wxToolBar );
		toolBar->Connect( XRCID("m_newBrushTool"), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( CEdVegetationEditTool::OnNewBrushTool ), NULL, this );
		toolBar->Connect( XRCID("m_saveBrushTool"), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( CEdVegetationEditTool::OnSaveBrushTool ), NULL, this );
		toolBar->Connect( XRCID("m_copyBrushTool"), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( CEdVegetationEditTool::OnCopyBrushTool ), NULL, this );
		toolBar->Connect( XRCID("m_lockBrushTool"), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( CEdVegetationEditTool::OnLockBrushTool ), NULL, this );
	}

	// Sliders
	{
		m_radiusSlider = XRCCTRL( *m_dialog, "m_radiusSlider", wxSlider );
		m_radiusSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( CEdVegetationEditTool::OnRadiusChanged ), NULL, this );
		m_radiusSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( CEdVegetationEditTool::OnRadiusChanged ), NULL, this );

		m_strengthSlider = XRCCTRL( *m_dialog, "m_strengthSlider", wxSlider );
		m_strengthSlider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( CEdVegetationEditTool::OnStrengthChanged ), NULL, this );
		m_strengthSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( CEdVegetationEditTool::OnStrengthChanged ), NULL, this );
	}

	// stats page
	{
		m_statsList = XRCCTRL( *m_dialog, "m_statistics", CEdAutosizeListCtrl );
		m_statsList->AppendColumn( TXT("Instances"), wxLIST_FORMAT_RIGHT, 80 );
		m_statsList->AppendColumn( TXT("Path"),      wxLIST_FORMAT_LEFT,  -1 );
		m_statsList->Bind( wxEVT_COMMAND_LIST_COL_CLICK,        &CEdVegetationEditTool::OnStatisticsColumnClicked, this );
		m_statsList->Bind( wxEVT_COMMAND_LIST_ITEM_ACTIVATED,   &CEdVegetationEditTool::OnStatisticsItemActivated, this );
		m_statsList->Bind( wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, &CEdVegetationEditTool::OnStatisticsItemRightClick, this );
		m_statsList->Bind( wxEVT_COMMAND_LIST_ITEM_SELECTED,    &CEdVegetationEditTool::OnStatisticsSelectionChanged, this );
		m_statsList->Bind( wxEVT_COMMAND_LIST_ITEM_DESELECTED,	&CEdVegetationEditTool::OnStatisticsSelectionChanged, this );

		m_renderingStatsLabel = XRCCTRL( *m_dialog, "m_renderingStatsLabel", wxStaticText );
	}
	
	XRCCTRL( *m_dialog, "m_showStatsForChoice", wxChoice )->Bind( wxEVT_COMMAND_CHOICE_SELECTED, &CEdVegetationEditTool::OnShowStatsForChanged, this );

	XRCCTRL( *m_dialog, "m_refreshGrassButton", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdVegetationEditTool::OnRefreshGenericGrass ), NULL, this );

	XRCCTRL( *m_dialog, "m_replaceSelected", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdVegetationEditTool::OnReplaceTree ), NULL, this );
	XRCCTRL( *m_dialog, "m_removeSelected", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdVegetationEditTool::OnRemoveTree ), NULL, this );
	XRCCTRL( *m_dialog, "m_removeDuplicates", wxButton )->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdVegetationEditTool::OnRemoveDuplicates ), NULL, this );

	// Update data format button (locked for advanced users)
	{
		Bool showAdvanced = false;
		SUserConfigurationManager::GetInstance().ReadParam( TXT("User"), TXT("Editor"), TXT("VegetationEditorShowAdvanced"), showAdvanced );

		m_performUpdateButton = XRCCTRL( *m_dialog, "m_performUpdateButton", wxButton );
		if ( showAdvanced )
		{
			m_performUpdateButton->Show();
			m_performUpdateButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdVegetationEditTool::OnFormatUpdate ), NULL, this );
		}
		else
		{
			m_performUpdateButton->Hide();
		}
	}

	{
		m_paintOnChoice = XRCCTRL( *m_dialog, "m_paintOnChoice", wxChoice );
		m_paintOnChoice->Bind( wxEVT_COMMAND_CHOICE_SELECTED, &CEdVegetationEditTool::OnPaintOnChoice, this );
		m_paintOnMode = static_cast< EPaintOnMode >( XRCCTRL( *m_dialog, "m_paintOnChoice", wxChoice )->GetSelection() );

		XRCCTRL( *m_dialog, "m_lowLimitCheck" , wxChoice )->Bind( wxEVT_COMMAND_CHECKBOX_CLICKED, &CEdVegetationEditTool::OnLowLimitChoice, this );
		XRCCTRL( *m_dialog, "m_highLimitCheck", wxChoice )->Bind( wxEVT_COMMAND_CHECKBOX_CLICKED, &CEdVegetationEditTool::OnHighLimitChoice, this );

		XRCCTRL( *m_dialog, "m_lowLimitEdit" , wxTextCtrl )->Bind( wxEVT_COMMAND_TEXT_UPDATED, &CEdVegetationEditTool::OnLowLimitText, this );
		XRCCTRL( *m_dialog, "m_highLimitEdit", wxTextCtrl )->Bind( wxEVT_COMMAND_TEXT_UPDATED, &CEdVegetationEditTool::OnHighLimitText, this );
	}

	m_thumbnailBitmap = XRCCTRL( *m_dialog, "m_thumbnailBitmap", wxStaticBitmap );
	m_thumbnailBitmap->Connect( wxEVT_PAINT, wxPaintEventHandler( CEdVegetationEditTool::OnPaintThumbnail ), NULL, this );

	m_dialog->Fit();

	LoadOptionsFromConfig();

	LayoutRecursively( m_dialog );

	UpdateGrassMask();
	UpdateSliderValues();
	UpdatePickShapes();
	UpdateSelectedInstanceInfo();

	m_dialog->Show();
	m_isStarted = true;

	// Start tool
	return true;
}

void CEdVegetationEditTool::End()
{
	SEvents::GetInstance().UnregisterListener( this );

	DestroyVertexEntities();
	SaveOptionsToConfig();

	m_isStarted = false;

	m_viewport->GetViewport()->SetGrassMaskPaintMode( false );

	// Close dialog
	if ( m_dialog )
	{
		m_dialog->Destroy();
	}
}

void CEdVegetationEditTool::SaveOptionsToConfig()
{
	ISavableToConfig::SaveSession();
}

void CEdVegetationEditTool::LoadOptionsFromConfig()
{
	ISavableToConfig::RestoreSession();
}

void CEdVegetationEditTool::SaveSession( CConfigurationManager &config )
{
	if ( m_isStarted )
	{
		CConfigurationScopedPathSetter pathSetter( config, TXT("/Tools/VegetationEdit") );

		Int32 x, y, w, h;
		m_dialog->GetPosition( &x, &y );
		m_dialog->GetSize( &w, &h );
		config.Write( TXT("PosX"), x );
		config.Write( TXT("PosY"), y );
		config.Write( TXT("Width"),  w );
		config.Write( TXT("Height"), h );

		wxSplitterWindow* splitter = XRCCTRL( *m_dialog, "m_brushListSplitter", wxSplitterWindow );
		config.Write( TXT("SashPos"), splitter->GetSashPosition() );

		// Grab brushes with with files on disk
		const TDynArray< THandle< CVegetationBrush > >& brushes = m_brushesListPanel->GetCurrentBrushes();
		TDynArray< THandle< CVegetationBrush > > brushesToRemember;
		for ( Uint32 i=0; i<brushes.Size(); ++i )
		{
			if ( brushes[i] && brushes[i]->GetFile() )
			{
				brushesToRemember.PushBack( brushes[i] );
			}
		}

		// Store the amount of such brushes
		config.Write( TXT("Brushes/Amount"), (Int32)brushesToRemember.Size() );

		// Store their paths
		for ( Uint32 i=0; i<brushesToRemember.Size(); ++i )
		{
			const String& depotPath = brushesToRemember[i]->GetFile()->GetDepotPath();
			config.Write( TXT("Brushes/Brush" ) + ToString( i ), depotPath );
		}

		config.Write( TXT("CursorLowLimitSet"),  m_cursorLowLimit.IsInitialized() ? 1 : 0 );
		config.Write( TXT("CursorHighLimitSet"), m_cursorHighLimit.IsInitialized() ? 1 : 0 );

		if ( m_cursorLowLimit.IsInitialized() )
		{
			config.Write( TXT("CursorLowLimit"), m_cursorLowLimit.Get() );
		}

		if ( m_cursorHighLimit.IsInitialized() )
		{
			config.Write( TXT("CursorHighLimit"), m_cursorHighLimit.Get() );
		}


		config.Write( TXT("CursorMode"), m_cursorMode );
// 		config.Write( TXT("PaintOnMode"), m_paintOnMode );
	}
}

void CEdVegetationEditTool::RestoreSession( CConfigurationManager &config )
{
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Tools/VegetationEdit") );

	Int32 x, y, w, h;
	m_dialog->GetPosition( &x, &y );
	m_dialog->GetSize( &w, &h );
	x = config.Read( TXT("PosX"), x );
	y = config.Read( TXT("PosY"), y );
	w = config.Read( TXT("Width"), w );
	h = config.Read( TXT("Height"), h );
	m_dialog->Move( x, y );
	m_dialog->SetSize( w, h );

	wxSplitterWindow* splitter = XRCCTRL( *m_dialog, "m_brushListSplitter", wxSplitterWindow );
	Int32 sashPos = config.Read( TXT("SashPos"), 300/*some sane default*/ );
	splitter->SetSashPosition( sashPos );

	// Get the amount of such brushes
	Int32 numBrushesToRestore = 0;
	numBrushesToRestore = config.Read( TXT("Brushes/Amount"), numBrushesToRestore );

	// Restore them
	for ( Int32 i=0; i<numBrushesToRestore; ++i )
	{
		String depotPath;
		if ( config.Read( TXT("Brushes/Brush" ) + ToString( i ), &depotPath ) )
		{
			m_brushesListPanel->AddBrush( depotPath );
		}
	}

	m_cursorMode = static_cast< EVegetationCursorMode >( config.Read( TXT("CursorMode"), m_cursorMode ) );

	for ( Uint32 i=0; i<sizeof( m_modeButtons )/sizeof( wxButton* ); ++i )
	{
		m_modeButtons[ i ]->SetValue( i == m_cursorMode );
	}

	Bool lowLimitSet  = config.Read( TXT("CursorLowLimitSet"),  0 ) != 0;
	Bool highLimitSet = config.Read( TXT("CursorHighLimitSet"), 0 ) != 0;

	if ( lowLimitSet )
	{
		m_cursorLowLimit = config.Read( TXT("CursorLowLimit"), 0.f );
		XRCCTRL( *m_dialog, "m_lowLimitEdit",  wxTextCtrl )->SetValue( ToString( m_cursorLowLimit.Get() ).AsChar() );
		XRCCTRL( *m_dialog, "m_lowLimitCheck",  wxCheckBox )->SetValue( true );
	}
	else
	{
		m_cursorLowLimit = TOptional< Float >();
		XRCCTRL( *m_dialog, "m_lowLimitCheck",  wxCheckBox )->SetValue( false );
	}

	if ( highLimitSet )
	{
		m_cursorHighLimit = config.Read( TXT("CursorHighLimit"), 0.f );
		XRCCTRL( *m_dialog, "m_highLimitEdit",  wxTextCtrl )->SetValue( ToString( m_cursorHighLimit.Get() ).AsChar() );
		XRCCTRL( *m_dialog, "m_highLimitCheck",  wxCheckBox )->SetValue( true );
	}
	else
	{
		m_cursorHighLimit = TOptional< Float >();
		XRCCTRL( *m_dialog, "m_highLimitCheck",  wxCheckBox )->SetValue( false );
	}

// 	m_paintOnMode = static_cast< EPaintOnMode >( config.Read( TXT("PaintOnMode"), m_paintOnMode ) );
// 	XRCCTRL( *m_dialog, "m_paintOnChoice", wxChoice )->SetSelection( m_paintOnMode );
}

void CEdVegetationEditTool::UpdateGrassMask()
{
	CFoliageEditionController & foliageEditor = m_world->GetFoliageEditionController();
	foliageEditor.CreateGrassMask( m_world );
}

void CEdVegetationEditTool::SetActiveBrush( CVegetationBrush* brush )
{
	if ( m_activeBrush == brush )
	{
		return;
	}

	m_activeBrush = brush;

	m_brushEntriesPanel->SetBrush( m_activeBrush.Get() );

	UpdateRealTimeStatistics();
}

CVegetationBrush* CEdVegetationEditTool::GetActiveBrush() const
{
	return m_activeBrush.Get();
}

void CEdVegetationEditTool::UpdateThumbnailSource( CDiskFile* file )
{
	m_thumbnailSource = file;
	m_thumbnailBitmap->Refresh();
}

void CEdVegetationEditTool::OnBrushSelected( CVegetationBrush* entry )
{
	UpdateRealTimeStatistics();
}

//////////////////////////////////////////////////////////////////////////
// Tool Callbacks
//////////////////////////////////////////////////////////////////////////

Bool CEdVegetationEditTool::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	DrawCursor( frame );

	const String shortcutHelperTxt = TXT( "CTRL+ALT+G: Grass instance heat map. CTRL+ALT+T: Tree instance heat map. CTRL+ALT+L: Grass layer heat map. CTRL+ALT+N: No heat map." );
	frame->AddDebugScreenText( 2, frame->GetFrameOverlayInfo().m_height - 30, shortcutHelperTxt );
	const String visibilityHelperTxt = TXT( "CTRL+ALT+{: Reduce foliage visibility. CTRL+ALT+}: Increase foliage visibility." );
	frame->AddDebugScreenText( 2, frame->GetFrameOverlayInfo().m_height - 16, visibilityHelperTxt );
	const String cursorPos = String::Printf( TXT("Cursor position: %3.2f, %3.2f, %3.2f"), m_cursorPosition.X, m_cursorPosition.Y, m_cursorPosition.Z );
	frame->AddDebugScreenText( 2, frame->GetFrameOverlayInfo().m_height - 2, cursorPos );

	return false;
}

Bool CEdVegetationEditTool::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	const EVegetationEditorMode mode = GetToolMode();

	m_cursorQuickPickMode = ( mode == VTM_Painting ) && RIM_IS_KEY_DOWN( IK_LControl );

	if ( key == IK_MouseZ )
	{
		Float baseMod = ( 0x0001 & GetKeyState( VK_CAPITAL ) ) ? 0.005f : 0.05f;
		if ( RIM_IS_KEY_DOWN( IK_LControl ) )
		{
		}
		else if ( RIM_IS_KEY_DOWN( IK_LShift ) )
		{			
		}
		else
		{
			if ( !RIM_IS_KEY_DOWN( IK_RightMouse ) ) // to allow the camera speed changing while pressing RMB
			{
				switch ( mode )
				{
				case VTM_Painting:
					m_paintBrushSize += data * m_paintBrushSize * baseMod;
					m_paintBrushSize = Clamp< Float >( m_paintBrushSize, MIN_BRUSH_SIZE, MAX_BRUSH_SIZE );
					UpdateSliderValues();
					break;
				case VTM_Statistics:
					if ( data )
					{
						m_cellStatisticCursorRadius += ( data > 0 ) ? 1 : -1;
						m_cellStatisticCursorRadius = Clamp< Int32 >( m_cellStatisticCursorRadius, 1, MAX_CELL_SELECTION_RADIUS );
						UpdateHighlightData();
					}
					break;
				}

				return true; // 'eat' the event
			}
		}
	}
	else if ( key == IK_Delete )
	{
		if ( !m_selectedInstance.IsEmpty() )
		{
			// TODO:
		}
	}
	else if ( key == IK_G && RIM_IS_KEY_DOWN( IK_LControl ) && RIM_IS_KEY_DOWN( IK_Alt ) )
	{
		m_world->GetFoliageEditionController().SetDebugVisualisationMode( VISUALISE_GRASSINSTANCES );
	}
	else if ( key == IK_T && RIM_IS_KEY_DOWN( IK_LControl ) && RIM_IS_KEY_DOWN( IK_Alt ) )
	{
		m_world->GetFoliageEditionController().SetDebugVisualisationMode( VISUALISE_TREEINSTANCES );
	}
	else if ( key == IK_L && RIM_IS_KEY_DOWN( IK_LControl ) && RIM_IS_KEY_DOWN( IK_Alt ) )
	{
		m_world->GetFoliageEditionController().SetDebugVisualisationMode( VISUALISE_GRASSLAYERS );
	}
	else if ( key == IK_N && RIM_IS_KEY_DOWN( IK_LControl ) && RIM_IS_KEY_DOWN( IK_Alt ) )
	{
		m_world->GetFoliageEditionController().SetDebugVisualisationMode( VISUALISE_NONE );
	}
	else if ( key == IK_LeftBracket && RIM_IS_KEY_DOWN( IK_LControl ) && RIM_IS_KEY_DOWN( IK_Alt ) )
	{
		m_world->GetFoliageEditionController().ReduceVisibilityDepth();
	}
	else if ( key == IK_RightBracket && RIM_IS_KEY_DOWN( IK_LControl ) && RIM_IS_KEY_DOWN( IK_Alt ) )
	{
		m_world->GetFoliageEditionController().IncreateVisibilityDepth();
	}

	return false;
}

Bool CEdVegetationEditTool::OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
{
	const EVegetationEditorMode mode = GetToolMode();

	if ( m_cursorQuickPickMode )
	{
		if ( button==0 && state && !m_isPaintingNow && mode == VTM_Painting )
		{
			// Pick a brush
			DoPickBrush();
			return true;
		}
	}
	else
	{
		switch ( mode )
		{
		case VTM_Painting:
			if ( button == 0 )
			{
				if ( state )
				{
					if ( !m_isPaintingNow )
					{
						m_isPaintingNow = true;
						view->SetMouseMode( MM_Clip );

						switch ( m_cursorMode )
						{
						case VCM_PaintInstances:
							CUndoVegetationExistance::CreateStep( *m_viewport->GetUndoManager(), &m_world->GetFoliageEditionController() );
							break;
						case VCM_PaintAligment:
							CUndoVegetationExistance::CreateStep( *m_viewport->GetUndoManager(), &m_world->GetFoliageEditionController(), TXT("re-align vegetation") );
							break;
						case VCM_PaintSize:
							CUndoVegetationSize::PrepareStep( *m_viewport->GetUndoManager(),&m_world->GetFoliageEditionController() );
							break;
						}

						// WARNING: may recurse down to the button release section, because of displayed messages causing a focus lost
						DoPaint(); 

						return true;
					}
				}
				else
				{
					if ( m_isPaintingNow )
					{
						view->SetMouseMode( MM_Normal, true );

						switch ( m_cursorMode )
						{
						case VCM_PaintSize:
							CUndoVegetationSize::FinalizeStep( *m_viewport->GetUndoManager() );
							break;
						}

						m_isPaintingNow = false;
						
						UpdateRealTimeStatistics( true ); 

						return true;
					}
				}
			}
			break;

		case VTM_InstanceManipulation:
			if ( button==0 )
			{
				if ( !state )
				{
					view->SetMouseMode( MM_Normal, true );
				}
				else if ( DoSelectInstance() )
				{
					return true;
				}
			}
			break;

		case VTM_Statistics:
			if ( button == 0 )
			{
				if ( !state )
				{
					view->SetMouseMode( MM_Normal, true );
				}
				else
				{
					m_selectedCell = m_highlightedCell;
					UpdateStatistics( m_selectedCell );
					return true;
				}
			}

			break;
		}
	}

	// Filter when painting
	if ( m_isPaintingNow )
	{
		return true;
	}

	// Do not filter in default cases
	return false;
}

Bool CEdVegetationEditTool::OnViewportMouseMove( const CMousePacket& packet )
{
	// Track cursor
	if ( m_isPaintingNow )
	{
		UpdateCursorData( packet );
		UpdateHighlightData();
		return true;
	}

	// Not filtered
	return false;
}

Bool CEdVegetationEditTool::OnViewportTrack( const CMousePacket& packet )
{
	// Just track the cursor
	UpdateCursorData( packet );	
	UpdateHighlightData();

	// Not filtered
	return false;
}

Bool CEdVegetationEditTool::OnViewportTick( IViewport* view, Float timeDelta )
{
	// Painting time
	const Bool canPaint = m_cursorMode != VPM_None;
	if ( canPaint && m_isPaintingNow )
	{
		m_paintTimer -= timeDelta;
		if ( m_paintTimer <= 0.0f )
		{
			DoPaint();
			return true;
		}
	}

	m_rendStatsTimer -= timeDelta;
	if ( m_rendStatsTimer <= 0.0f )
	{
		UpdateRenderingStatistics();
	}

	// Do not block
	return false;
}

void CEdVegetationEditTool::OnToggleModeClicked( wxCommandEvent& event )
{
	for ( Uint32 i = 0; i < sizeof( m_modeButtons )/sizeof( wxButton* ); ++i )
	{
		if ( event.GetId() == m_modeButtons[i]->GetId() )
		{
			m_modeButtons[i]->SetValue( true );
			m_cursorMode = static_cast< EVegetationCursorMode >( i );
		}
		else
		{
			m_modeButtons[i]->SetValue( false );
		}
	}

 	m_viewport->GetViewport()->SetGrassMaskPaintMode( m_cursorMode == VCM_PaintGrassMask );
	UpdateSliderValues();
	UpdatePickShapes();
	UpdateSelectedInstanceInfo();
}

void CEdVegetationEditTool::OnNewBrushTool( wxCommandEvent& event )
{
	CVegetationBrush* brush = CreateNewBrush();
	SetActiveBrush( brush );
}

void CEdVegetationEditTool::OnSaveBrushTool( wxCommandEvent& event )
{
	if ( m_activeBrush )
	{
		if ( m_activeBrush->GetFile() == NULL )
		{
			// Ask for new world file
			String tag = (ClassID< CVegetationBrush >())->GetDefaultObject<CVegetationBrush>()->GetFriendlyName();
			m_brushSaveDialog.SetIniTag( tag );
			if ( m_brushSaveDialog.DoSave( m_dialog->GetHandle(), NULL, true ) )
			{
				// Translate absolute path to local depot path
				String localDepotPath;
				if ( !GDepot->ConvertToLocalPath( m_brushSaveDialog.GetFile(), localDepotPath ) )
				{
					WARN_EDITOR( TXT("Unable to load world in because path is not under depot") );
				}

				// Find depot directory
				CFilePath path( localDepotPath );
				CDirectory* brushDir = GDepot->CreatePath( localDepotPath );

				m_activeBrush->SaveAs( brushDir, path.GetFileName() );

				// Update name based on the newly bound file
				m_brushesListPanel->SetupControl( m_activeBrush.Get() );
			}
		}
		else
		{
			m_activeBrush->Save();
		}
	}
}

void CEdVegetationEditTool::OnCopyBrushTool( wxCommandEvent& event )
{
	if ( m_activeBrush )
	{
		CVegetationBrush* copiedBrush = (CVegetationBrush*)m_activeBrush->Clone( NULL );
		copiedBrush->AddToRootSet();
		m_brushesListPanel->AddItem( copiedBrush );
	}
}

void CEdVegetationEditTool::OnLockBrushTool( wxCommandEvent& event )
{
	if ( m_activeBrush && m_activeBrush->MarkModified() )
	{
		m_brushesListPanel->PerformBrushLock();
	}
}

void CEdVegetationEditTool::OnPaintThumbnail( wxPaintEvent& event )
{
	if ( m_thumbnailSource )
	{
		m_thumbnailSource->LoadThumbnail();
		TDynArray< CThumbnail* > thumbnails = m_thumbnailSource->GetThumbnails();
		if ( !thumbnails.Empty() )
		{
			CThumbnail* thumbnail = thumbnails[0];
			CWXThumbnailImage* image = (CWXThumbnailImage*)thumbnail->GetImage();

			wxPaintDC dc( m_thumbnailBitmap );
			HDC hdc = (HDC)dc.GetHDC();

			Gdiplus::Graphics graphics( hdc );

			graphics.DrawImage( image->GetBitmap(), 0, 0, m_thumbnailBitmap->GetSize().GetWidth(), m_thumbnailBitmap->GetSize().GetHeight() );
		}
	}
}

TOptional< Float > CEdVegetationEditTool::GetDecimation() const
{
	if ( XRCCTRL( *m_dialog, "m_decimationCheck", wxCheckBox )->GetValue() )
	{
		String decStr = XRCCTRL( *m_dialog, "m_decimationEdit", wxTextCtrl )->GetValue();

		Float keepVal;
		if ( FromString( decStr, keepVal ) )
		{
			keepVal = Clamp( keepVal / 100.0, 0.0, 1.0 );
			return 1. - keepVal;
		}
	}

	return TOptional< Float >();
}

TOptional< TPair< Float, Float > > CEdVegetationEditTool::GetScaleFilter() const
{
	if ( XRCCTRL( *m_dialog, "m_scaleFilterCheck", wxCheckBox )->GetValue() )
	{
		String lowStr  = XRCCTRL( *m_dialog, "m_scaleFilterLow", wxTextCtrl )->GetValue();
		String highStr = XRCCTRL( *m_dialog, "m_scaleFilterHigh", wxTextCtrl )->GetValue();

		Float lowVal, highVal;
		if ( FromString( lowStr, lowVal ) && FromString( highStr, highVal ) )
		{
			if ( lowVal < 0. ) lowVal = 0.;
			if ( highVal < 0. ) highVal = 0.;
			return MakePair( lowVal, highVal );
		}
	}

	return TOptional< TPair< Float, Float > >();
}


Uint32 CEdVegetationEditTool::DoReplaceTreeForStatsSelection( CSRTBaseTree* newBaseTree, const Float* resetScale )
{
	TDynArray< Int32 > selection = m_statsList->GetSelectedItems();

	if ( selection.Empty() )
	{
		return 0;
	}

	Int32 instancesReplaced = 0;

	Box totalBox = m_selectedCell.GetTotalBox();
	TOptional< Float > dec = GetDecimation();
	TOptional< TPair< Float, Float > > scaleRange = GetScaleFilter();
	Red::Math::Random::Generator< Red::Math::Random::StandardRand > randGen;

	Float decAmount = dec.IsInitialized() ? dec.Get() : 1.f; // affect chosen fraction or all

	for ( Int32 sel : selection )
	{
		const SFoliageInstanceStatistics& stats = m_statistics[ sel ];
		CSRTBaseTree* oldBaseTree = stats.m_baseTree.Get();

		if ( newBaseTree && oldBaseTree->GetType() != newBaseTree->GetType() )
		{
			GFeedback->ShowError( TXT("SRT Tree resources to replace has to be of the same type (grass / tree)") );
			continue;
		}

		if ( newBaseTree )
		{
			instancesReplaced += m_world->GetFoliageEditionController().ReplaceTree( oldBaseTree, newBaseTree, totalBox, true, 
				[ decAmount, &randGen, &scaleRange ]( const SFoliageInstance& inst ) { return ScaleAndDecInstanceFilter( inst, decAmount, randGen, scaleRange ); }, resetScale );
		}
		else
		{
			m_world->GetFoliageEditionController().ReplaceTree( oldBaseTree, nullptr, totalBox, true, 
				[ decAmount, &randGen, &scaleRange ]( const SFoliageInstance& inst ) { return ScaleAndDecInstanceFilter( inst, decAmount, randGen, scaleRange ); } );
		}
	}

	UpdateStatistics( m_selectedCell );

	return instancesReplaced;
}

Uint32 CEdVegetationEditTool::DoReplaceTreeWithMeshForStatsSelection( CMesh* meshResource, CLayerInfo* layerForTreeMeshes )
{
	TDynArray< Int32 > selection = m_statsList->GetSelectedItems();

	if ( selection.Empty() || !layerForTreeMeshes || !meshResource )
	{
		return 0;
	}
	Int32 instancesReplaced = 0;

	Box totalBox = m_selectedCell.GetTotalBox();
	TOptional< TPair< Float, Float > > scaleRange = GetScaleFilter();

	Red::Math::Random::Generator< Red::Math::Random::StandardRand > randGen;
	TOptional< Float > dec = GetDecimation();
	Float decAmount = dec.IsInitialized()
		? dec.Get()
		: meshResource != nullptr ? 0.f : 1.f;

	Matrix additionalRot = Matrix::IDENTITY;
	additionalRot.SetRotZ33( -M_PI_HALF );

	for ( Int32 sel : selection )
	{
		const SFoliageInstanceStatistics& stats = m_statistics[ sel ];
		CSRTBaseTree* oldBaseTree = stats.m_baseTree.Get();

		// remove some of instances according to the decimation factor
		m_world->GetFoliageEditionController().ReplaceTree( oldBaseTree, nullptr, totalBox, true, 
			[ decAmount, &randGen, &scaleRange ]( const SFoliageInstance& inst ) { return ScaleAndDecInstanceFilter( inst, decAmount, randGen, scaleRange ); } );

		// collect all instances that we can delete later, otherwise there may appear duplicates
		FoliageInstanceContainer modifiableInstances;
		for ( CFoliageCellIterator cell = m_world->GetFoliageEditionController().GetFoliageBroker()->GetCellIterator( totalBox ); cell; ++cell )
		{
			if ( cell->IsResourceValid() )
			{
				cell->Wait(); // wait for resource to load
				CFoliageResource* res = cell->GetFoliageResource();

				if ( m_world->GetFoliageEditionController().PerformSilentCheckOutOnResource( *res ) )
				{
					res->GetInstancesFromArea( oldBaseTree, totalBox, modifiableInstances );
				}
			}
		}

		// spawn meshes
		for ( const SFoliageInstance& fi : modifiableInstances )
		{
			if ( ScaleInstanceFilter( fi, scaleRange ) )
			{
				const Vector3& pos = fi.GetPosition();
				Float scale = fi.GetScale();
				Matrix rotMtx = GetInstanceRotationMatrix( fi );
				rotMtx = rotMtx * additionalRot;

				EntitySpawnInfo sinfo;
				sinfo.m_spawnPosition = Vector( pos.X, pos.Y, pos.Z );
				sinfo.m_resource = meshResource;
				sinfo.m_template = LoadResource< CEntityTemplate >( TXT("engine\\templates\\editor\\staticmesh.w2ent") );
				sinfo.m_detachTemplate = true;
				sinfo.AddHandler( new TemplateInfo::CSpawnEventHandler() );

				CEntity* newEntity = layerForTreeMeshes->GetLayer()->CreateEntitySync( sinfo );
				if ( newEntity )
				{
					newEntity->CreateStreamedComponents( SWN_NotifyWorld );
					newEntity->SetRotation( rotMtx.ToEulerAngles() );
					newEntity->SetScale( Vector( scale, scale, scale ) );
				}
			}
		}

		// remove all remaining instances of given tree
		instancesReplaced += m_world->GetFoliageEditionController().ReplaceTree( oldBaseTree, nullptr, totalBox, true, 
			[ &scaleRange ]( const SFoliageInstance& inst ){ return ScaleInstanceFilter( inst, scaleRange ); } );
	}

	UpdateStatistics( m_selectedCell );
	return instancesReplaced;
}

void CEdVegetationEditTool::OnReplaceTree( wxCommandEvent& event )
{
	String resourcePath;
	GetActiveResource( resourcePath );

	CFilePath::PathString ext = CFilePath( resourcePath ).GetExtension();
	CResource* newBaseTree = GDepot->LoadResource( resourcePath );
	if ( !( newBaseTree && ext.EqualsNC( newBaseTree->GetExtension() ) ) )
	{
		GFeedback->ShowError( TXT("Please select valid SRT Tree or mesh resource in Asset Browser first.") );
		return;
	}

	CEdReplaceIntancesDialog::Info info;
	CEdReplaceIntancesDialog dlg( m_dialog );
	if ( !dlg.Execute( info ) )
	{
		return;
	}
	
	CSelectionManager* selectionManager = m_world->GetSelectionManager();
	CLayerInfo* activeLayer = selectionManager->GetActiveLayer();

	String msg = TXT("Couldn't load the resource");
	Bool replacementPerformed = newBaseTree->IsA< CSRTBaseTree >();
	Uint32 instancesReplaced = 0;

	if ( CSRTBaseTree* baseTreeRes = Cast< CSRTBaseTree >( newBaseTree ) )
	{
		instancesReplaced = DoReplaceTreeForStatsSelection( baseTreeRes, info.resetScale ? &info.resetScaleVal : nullptr );
		replacementPerformed = true;
	}
	else if ( CMesh* meshRes = Cast< CMesh >( newBaseTree ) )
	{
		if ( !activeLayer || !activeLayer->GetLayer() || !activeLayer->IsLoaded() || !activeLayer->IsVisible() )
		{
			msg = TXT("No active layers that are loaded and visible");
		}
		else if ( !activeLayer->GetLayer()->IsModified() && activeLayer->GetLayer()->GetFile() && !activeLayer->GetLayer()->GetFile()->Edit() )
		{
			msg = TXT("Cannot edit layer");
		}
		else
		{
			instancesReplaced = DoReplaceTreeWithMeshForStatsSelection( meshRes, activeLayer );
			replacementPerformed = true;
		}
	}

	if ( replacementPerformed )
	{
		GFeedback->ShowMsg( TXT("Instances updated"), String::Printf( TXT("%i instances updated"), instancesReplaced ).AsChar() );
	}
	else
	{
		GFeedback->ShowError( msg.AsChar() );
	}
}

void CEdVegetationEditTool::OnRemoveTree( wxCommandEvent& event )
{
	if ( !GFeedback->AskYesNo( TXT("Are you sure to remove this tree? This change cannot be undone") ) )
	{
		return;
	}

	DoReplaceTreeForStatsSelection( nullptr, nullptr );
}

void CEdVegetationEditTool::OnRemoveDuplicates( wxCommandEvent& event )
{
	if ( m_statsSelectionLatch.Empty() || m_selectedCell.IsEmpty() )
	{
		return;
	}

	Uint32 instancesRemoved = 0;

	for ( BaseTreeHandle& baseTreeHandle : m_statsSelectionLatch )
	{
		FoliageInstanceContainer container;
		m_world->GetFoliageEditionController().GetInstancesFromArea( baseTreeHandle, m_selectedCell.GetTotalBox(), container );
		Uint32 instancesCountBefore = container.Size();

		for ( SFoliageInstance& foliageInstance: container )
		{
			// remove all instances of foliageInstance in it's position
			m_world->GetFoliageEditionController().RemoveInstances( baseTreeHandle, foliageInstance.GetPosition(), NumericLimits< Float >::Epsilon() );
			// add just one instance in previously emptied location
			FoliageInstanceContainer oneInstanceContainer;
			oneInstanceContainer.PushBack( foliageInstance );
			m_world->GetFoliageEditionController().AddInstances( baseTreeHandle, oneInstanceContainer );
		}

		container.Clear();
		m_world->GetFoliageEditionController().GetInstancesFromArea( baseTreeHandle, m_selectedCell.GetTotalBox(), container );
		instancesRemoved += instancesCountBefore - container.Size();
	}
	UpdateStatistics( m_selectedCell );
	GFeedback->ShowMsg( TXT("Summary"), TXT("%d instances removed."), instancesRemoved );
}

void CEdVegetationEditTool::OnShowStatsForChanged( wxCommandEvent& event )
{
	UpdateStatistics( m_selectedCell );
}

void CEdVegetationEditTool::OnRefreshGenericGrass( wxCommandEvent& event )
{
	if ( m_world->GetTerrain() )
	{
		m_world->GetTerrain()->UpdateGrassRendering();
	}
}

void CEdVegetationEditTool::OnRadiusChanged( wxScrollEvent& event )
{
	// Update radius
	Float frac = Clamp< Float >( m_radiusSlider->GetValue() / 1000.0f, 0.0f, 1.0f );
	m_paintBrushSize = MIN_BRUSH_SIZE + 100.0f * frac * frac;
}

void CEdVegetationEditTool::OnStrengthChanged( wxScrollEvent& event )
{
	// Update strength
	Float frac = Clamp< Float >( m_strengthSlider->GetValue() / 1000.0f, 0.0f, 1.0f );
	m_paintBrushStrength = frac;
}

void CEdVegetationEditTool::OnFormatUpdate( wxCommandEvent& event )
{
	// If data format of CFoliageResource change, you might have to handle this.
	// For now, move along sir.
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// CURSOR DRAWING:

const Float MIN_MESH_PITCH = 20.f;

void CEdVegetationEditTool::UpdateCursorData( const CMousePacket& packet )
{
	if ( !GGame->GetActiveWorld() )
	{
		return;
	}

	Float cursorDistance = NumericLimits< Float >::Max();

	m_cursorRayOrigin    = packet.m_rayOrigin;
	m_cursorRayDirection = packet.m_rayDirection;
	m_cursorPlacement    = VCP_Nowhere;

	EVegetationEditorMode mode = GetToolMode();

	Bool checkMesh    = ( mode != VTM_Statistics ) && ( m_paintOnMode == POM_StaticMeshes || m_paintOnMode == POM_All );
	Bool checkTerrain = ( mode == VTM_Statistics ) || ( m_paintOnMode == POM_Terrain || m_paintOnMode == POM_All );

	if ( checkMesh )
	{ // check meshes only if there is mesh-enabled mode active; otherwise allow to pain "under" the meshes by checking the terrain only
		CPhysicsWorld* pworld = nullptr;
		if ( GGame->GetActiveWorld()->GetPhysicsWorld( pworld ) )
		{
			const Float RAY_DISTANCE = 10000.f;
			Int32 included =  GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Platforms ) );

			SPhysicsContactInfo result;

			if ( pworld->RayCastWithSingleResult( packet.m_rayOrigin, packet.m_rayOrigin + packet.m_rayDirection * RAY_DISTANCE, included, 0, result ) == TRV_Hit )
			{
				cursorDistance = ( packet.m_rayOrigin - result.m_position ).Mag3();

				Float pitch = fabs( result.m_normal.ToEulerAngles().Pitch );
				if ( pitch > MIN_MESH_PITCH ) // check if the surface is not to steep (to prevent from weird cursor behavior)
				{
					m_cursorPlacement = VCP_Mesh;
					m_cursorPosition = result.m_position;
					m_cursorNormal = result.m_normal;
				}
				// else -> keep the cursor on "nowhere" for steep surfaces
			}
		}
	}

	if ( checkTerrain )
	{
		Vector terrainCursorPosition;
		if ( m_world->GetTerrain()->Intersect( packet.m_rayOrigin, packet.m_rayDirection, terrainCursorPosition ) )
		{
			Float terrainCursorDistance = ( packet.m_rayOrigin - terrainCursorPosition ).Mag3();

			// get the nearest from mesh and terrain - important when there are "basement" meshes under the terrain that may get in the way.
			if ( terrainCursorDistance < cursorDistance ) 
			{
				m_cursorPosition = terrainCursorPosition;
				m_cursorPlacement = VCP_Terrain;
			#ifdef USE_TERRAIN_NORMAL
				m_cursorNormal = m_world->GetTerrain()->GetNormalForWorldPosition( m_cursorPosition );
			#else
				m_cursorNormal = Vector( 0.f, 0.f, 1.f );
			#endif
			}
		}
	}
}

void CEdVegetationEditTool::UpdateHighlightData()
{
	const EVegetationEditorMode mode = GetToolMode();
	Box cellExtents = m_world->GetFoliageEditionController().GetCellExtend( m_cursorPosition );

	switch ( mode )
	{
	case VTM_Painting:
		{
			// ensure there is no instance highlight in painting mode
			m_highlightedInstance.Reset();
			m_selectedInstance.Reset();
			DestroyVertexEntities();

			SSelectedCellDesc oldHighlightedCell = m_highlightedCell;
			m_highlightedCell = SSelectedCellDesc( cellExtents, 1 );

			if ( m_highlightedCell != oldHighlightedCell )
			{ // crossed a cell boundary
				UpdateRealTimeStatistics();
			}
		}
		break;

	case VTM_InstanceManipulation:
		{
			SSelectedCellDesc oldHighlightedCell = m_highlightedCell;
			m_highlightedCell = SSelectedCellDesc( cellExtents, 1 );

			if ( m_highlightedCell != oldHighlightedCell )
			{ // crossed a cell boundary
				UpdateRealTimeStatistics();
				UpdatePickShapes();
			}

			m_highlightedInstance.Reset();

			Float minDist = NumericLimits< Float >::Max();
			for ( const SFoliagePickShape& pickShape : m_pickShapes )
			{
				Float distFromIntersect;
				if ( pickShape.m_shape.IntersectRay( m_cursorRayOrigin, m_cursorRayDirection, distFromIntersect ) )
				{
					SSelectedInstanceDesc instToHighlight = SSelectedInstanceDesc( pickShape.m_baseTree.Get(), pickShape.m_instance );
					Float distFromCenter = ( m_cursorRayOrigin - pickShape.m_shape.GetPosition() ).Mag3();
					if ( distFromCenter < minDist )
					{
						minDist = distFromCenter;
						if ( instToHighlight != m_selectedInstance )
						{
							m_highlightedInstance = instToHighlight;
						}
					}
				}
			}
		}
		break;

	case VTM_Statistics:
		{
			m_highlightedInstance.Reset();
			m_selectedInstance.Reset();
			DestroyVertexEntities();

			m_highlightedCell = SSelectedCellDesc( cellExtents, m_cellStatisticCursorRadius );
		}
		break;
	}
}

Bool CEdVegetationEditTool::DoSelectInstance()
{
	// Promote highlighted instance to selected instance
	if ( m_highlightedInstance != m_selectedInstance )
	{
		m_selectedInstance = m_highlightedInstance;
		RebuildVertexEntities();
		m_highlightedInstance.Reset();
		UpdateSelectedInstanceInfo();
		return true;
	}

	return false;
}

void CEdVegetationEditTool::DoPickBrush()
{
	// Get all base trees that have instances in the cursor area
	TDynArray< SFoliageInstanceCollection > collections;
	m_world->GetFoliageEditionController().GetInstancesFromArea( m_cursorPosition, m_paintBrushSize, collections );

	if ( !collections.Empty() )
	{
		// See if we have a brush locked for picking.
		CVegetationBrush* brush = m_brushesListPanel->GetLockedBrush();
		if ( !brush )
		{
			// No brush locked. Create a new brush.
			brush = CreateNewBrush();
		}
		
		if( brush && brush->MarkModified() )
		{
			// Add picked trees to the brush
			TDynArray< CSRTBaseTree* > baseTrees;
			for ( Uint32 i=0; i<collections.Size(); ++i )
			{
				brush->AddEntry( collections[i].m_baseTree.Get() );
				baseTrees.PushBack( collections[i].m_baseTree.Get() );
			}

			// Select on the list
			m_brushesListPanel->SelectItem( brush );

			// Do a full reset to be safe
			SetActiveBrush( NULL );
			SetActiveBrush( brush );

			// Select picked trees in the entries list
			m_brushEntriesPanel->SelectItems( baseTrees );
		}
	}
}

EVegetatonCursorPlacement CEdVegetationEditTool::ClampPositionZ( Vector3& pos, EVegetatonCursorPlacement origPlacement ) const
{
	if ( m_cursorLowLimit.IsInitialized() && pos.Z < m_cursorLowLimit.Get() )
	{
		return VCP_Nowhere;
	}

	if ( m_cursorHighLimit.IsInitialized() && pos.Z > m_cursorHighLimit.Get() )
	{
		return VCP_Nowhere;
	}

	return origPlacement;
}

EVegetatonCursorPlacement CEdVegetationEditTool::CastPoint( Vector3& position, const Vector& direction )
{
	const Float RAY_OFFSET = 1.f;

	if ( m_paintOnMode == POM_StaticMeshes || m_paintOnMode == POM_All )
	{
		CPhysicsWorld* pworld = nullptr;
		if ( GGame->GetActiveWorld()->GetPhysicsWorld( pworld ) )
		{
			Int32 included =  GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Platforms ) );
			
			SPhysicsContactInfo res;
			if ( pworld->RayCastWithSingleResult( position - direction*RAY_OFFSET, position + direction*RAY_OFFSET, included, 0, res ) == TRV_Hit )
			{
				Float pitch = fabs( res.m_normal.ToEulerAngles().Pitch );
				if ( pitch > MIN_MESH_PITCH ) // check if the surface is not to steep (to prevent from weird cursor behavior)
				{
					if ( m_cursorPlacement == VCP_Mesh )
					{
						position = res.m_position;
						return ClampPositionZ( position, VCP_Mesh );
					}
					else
					{
						return VCP_Nowhere;
					}
				}
			}
		}
	}
		
	if ( m_paintOnMode == POM_Terrain || m_paintOnMode == POM_All ) 
	{
		if ( CClipMap* terrain = GGame->GetActiveWorld()->GetTerrain() )
		{
			#ifdef USE_TERRAIN_NORMAL
				Vector res;
				if ( terrain->Intersect( position - direction*RAY_OFFSET, direction, res ) )
				{
					if ( m_cursorPlacement == VCP_Terrain )
					{
						position = res;
						return VCP_Terrain;
					}
					else
					{
						return  VCP_Nowhere;
					}
				}
			#else
				Float height;
				terrain->GetHeightForWorldPositionSync( position, 0, height );

				if ( m_cursorPlacement == VCP_Terrain )
				{
					position = Vector( position.X, position.Y, height );
					return ClampPositionZ( position, VCP_Terrain );
				}
				else
				{
					return  VCP_Nowhere;
				}
			#endif
		}
	}

	// Not on terrain
	return VCP_Nowhere;
}

Color CEdVegetationEditTool::GetCursorColor() const
{
	if ( m_cursorQuickPickMode )
	{
		return Color::YELLOW;
	}
	else
	{
		if ( m_cursorPlacement == VCP_Mesh )
		{
			return Color::WHITE;
		}
		else 
		{
			return Color::CYAN;
		}
	}
}

void CEdVegetationEditTool::DrawTerrainLine( const Vector& start, const Vector& end, const Color& color, Bool bePrecise, CRenderFrame* frame )
{
	const Float segSize = 0.2f;
	Int32 numSteps = bePrecise ? ( end-start ).Mag2() / segSize : 1;
	
	Vector delta = ( end-start ) / numSteps;

	Vector curPoint = start;
	m_world->GetTerrain()->GetHeightForWorldPosition( curPoint, curPoint.Z );

	for ( Int32 step = 1; step <= numSteps; ++step )
	{
		Vector nextPoint = start + delta * step;
		m_world->GetTerrain()->GetHeightForWorldPosition( nextPoint, nextPoint.Z );

		frame->AddDebugLine( curPoint, nextPoint, color, true );

		curPoint = nextPoint;
	}
}

void CEdVegetationEditTool::DrawTerrainBox( const Box& box, const Color& color, Int32 options, CRenderFrame* frame )
{
	Vector min = box.Min;
	Vector max = box.Max;

	Bool bePrecise = ( options & TBO_BePrecise ) != 0;

	DrawTerrainLine( Vector( min.X, min.Y, 0 ), Vector( max.X, min.Y, 0 ), color, bePrecise, frame );
	
	if ( !( options & TBO_SkipRightEdge ) )
	{
		DrawTerrainLine( Vector( max.X, min.Y, 0 ), Vector( max.X, max.Y, 0 ), color, bePrecise, frame );
	}

	if ( !( options & TBO_SkipBottomEdge ) )
	{
		DrawTerrainLine( Vector( max.X, max.Y, 0 ), Vector( min.X, max.Y, 0 ), color, bePrecise, frame );
	}

	DrawTerrainLine( Vector( min.X, max.Y, 0 ), Vector( min.X, min.Y, 0 ), color, bePrecise, frame );

	if ( options & TBO_DrawCross )
	{
		DrawTerrainLine( Vector( min.X, min.Y, 0 ), Vector( max.X, max.Y, 0 ), color, false, frame );
		DrawTerrainLine( Vector( max.X, min.Y, 0 ), Vector( min.X, max.Y, 0 ), color, false, frame );
	}
}

void CEdVegetationEditTool::DrawCellMarker( const SSelectedCellDesc& desc, const Color& color, Bool cross, Bool bePrecise, CRenderFrame* frame )
{
	if ( desc.IsEmpty() )
	{
		return;
	}

	Vector size = desc.m_centerBox.CalcSize();

	Int32 start = -desc.m_radius+1, end = desc.m_radius-1;
	for ( Int32 yIdx = start; yIdx <= end; ++yIdx )
	{
		for ( Int32 xIdx = start; xIdx <= end; ++xIdx )
		{
			Int32 options = 0;
			if ( cross )       options |= TBO_DrawCross;
			if ( bePrecise )   options |= TBO_BePrecise;
			if ( xIdx != end ) options |= TBO_SkipRightEdge;
			if ( yIdx != end ) options |= TBO_SkipBottomEdge;

			Vector min = Vector( desc.m_centerBox.Min.X + xIdx*size.X, desc.m_centerBox.Min.Y + yIdx*size.Y, 0 );
			DrawTerrainBox( Box( min, min+size ), color, options, frame );
		}
	}
}

void CEdVegetationEditTool::DrawInstanceMarker( const SSelectedInstanceDesc& desc, const Color& color, CRenderFrame* frame )
{
	if ( !desc.IsEmpty() )
	{
		CSRTBaseTree* baseTree = desc.m_baseTree;
		const Box box = baseTree->GetBBox();
		
		Matrix additionalRot = Matrix::IDENTITY;
		additionalRot.SetRotZ33( -M_PI_HALF );
		Matrix m = GetInstanceRotationMatrix( desc.m_instance ) * additionalRot;
		m.SetScale33( desc.m_instance.GetNormalizedScale() );
		m = m * Matrix( Matrix::IDENTITY ).SetTranslation( desc.m_instance.GetPosition() );

		frame->AddDebugBox( box, m, color, true );
	}
}

void CEdVegetationEditTool::DrawPaintingCursorPart( CRenderFrame* frame, Float size )
{
	// Circle subdivision based on brush radius
	const Uint32 numPoints = Max( Int32( 2.0 * M_PI * size ), 32 );

	typedef TDynArray< Vector > Segment;

	// Generate points
	TDynArray< Segment > ringSegments;

	EVegetatonCursorPlacement prevPlacement = VCP_Nowhere;
	for ( Uint32 i = 0; i < numPoints; i ++ )
	{
		const Float localAngle = 2.0f * M_PI * ( i / (Float)numPoints );

		// Calculate vertex world space xy position
		const Float dx = MCos( localAngle ) * size;
		const Float dy = MSin( localAngle ) * size;

		Vector3 pos = GetPointOnPlane( m_cursorPosition, m_cursorNormal, dx, dy );

		// Update Z position
		EVegetatonCursorPlacement placement = CastPoint( pos, -m_cursorNormal );

		if ( ringSegments.Empty() || placement != prevPlacement ) // first seg or terrain<->mesh transition
		{
			ringSegments.PushBack( Segment() ); // being new segment
		}

		if ( placement != VCP_Nowhere )
		{
			ringSegments.Back().PushBack( pos );
		}

		prevPlacement = placement;
	}

	const Color color = GetCursorColor();

	// Draw lines
	for ( Uint32 s = 0; s<ringSegments.Size(); ++s )
	{
		const Segment& seg = ringSegments[s];
		if ( !seg.Empty() )
		{
			for ( Uint32 i = 0; i<seg.Size()-1; i ++ )
			{
				const Vector& a = seg[i];
				const Vector& b = seg[i+1];

				frame->AddDebugLine( a, b, color, true );
			}
		}
	}

	// Draw closing line (only if there is no empty "spacer" segment)
	if ( !ringSegments.Empty() && !ringSegments[0].Empty() && !ringSegments.Back().Empty() )
	{
		frame->AddDebugLine( ringSegments.Back().Back(), ringSegments[0][0], color, true );
	}
}

void CEdVegetationEditTool::DrawPaintingCursor( CRenderFrame* frame )
{
	if ( ( m_cursorMode == VCM_PaintInstances || m_cursorMode == VCM_PaintGrassMask ) && !m_cursorQuickPickMode )
	{
		DrawPaintingCursorPart( frame, m_paintBrushSize );
		DrawPaintingCursorPart( frame, MIN_BRUSH_SIZE );
	}
	else
	{
		if ( m_cursorPlacement != VCP_Nowhere )
		{
			frame->AddDebugSphere( m_cursorPosition, m_paintBrushSize, Matrix::IDENTITY, GetCursorColor() );
		}
	}
}

void CEdVegetationEditTool::DrawCursor( CRenderFrame* frame )
{
	switch ( GetToolMode() )
	{
	case VTM_Painting:
		DrawPaintingCursor( frame );
		DrawCellMarker( m_highlightedCell, GetCursorColor(), false, true, frame );
		break;

	case VTM_InstanceManipulation:

		DrawInstanceMarker( m_highlightedInstance, Color::GREEN, frame ); // draw the picker
		DrawInstanceMarker( m_selectedInstance, Color::YELLOW, frame ); // draw the picker
		DrawPaintingCursorPart( frame, MIN_BRUSH_SIZE ); // draw the cursor position indicator
		DrawCellMarker( m_highlightedCell, GetCursorColor(), false, true, frame );
		break;

	case VTM_Statistics:
		// Do not call this with 'bePrecise' set to true, as it can be damn slow for a larger radius
		DrawCellMarker( m_highlightedCell, Color::CYAN, true, false, frame );
		DrawCellMarker( m_selectedCell, Color::YELLOW, false, false, frame );
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CEdVegetationEditTool::DoPaint()
{
	// Paint every 1/10th of the second
	m_paintTimer = 0.1f;

	const Bool alter = RIM_IS_KEY_DOWN( IK_LShift );
	if ( m_cursorMode == VCM_PaintGrassMask )
	{
		PaintGrassMask( m_cursorPosition, m_paintBrushSize, !alter );
	}
	else
	{
		if ( !m_activeBrush )
		{
			return;
		}

		// Collect selected entries
		TDynArray< CVegetationBrushEntry* > entries, tempEntries;
		m_activeBrush->GetEntries( tempEntries );
		for ( Uint32 e=0; e<tempEntries.Size(); ++e )
		{
			if ( m_brushEntriesPanel->IsItemSelected( tempEntries[e]->GetBaseTree() ) )
			{
				entries.PushBack( tempEntries[e] );
			}
		}

		if ( !entries.Empty() )
		{
			// Paint by each type
			for ( CVegetationBrushEntry* entry : entries )
			{
				Bool success = true;
				if ( m_cursorMode == VCM_PaintInstances )
				{
					if ( alter )
					{
						success = EraseVegetation( m_cursorPosition, m_paintBrushSize, entry->GetBaseTree() );
					}
					else
					{
						success = PaintVegetation( m_cursorPosition, m_cursorNormal, m_paintBrushSize, entry );
					}
				}
				else if ( m_cursorMode == VCM_PaintSize )
				{
					success = PaintSize( m_cursorPosition, m_paintBrushSize, entry, alter );
				}
				else if ( m_cursorMode == VCM_PaintAligment )
				{
					success = PaintAlignment( m_cursorPosition, m_cursorNormal, m_paintBrushSize, entry );
				}

				if ( !success )
				{
					LOG_EDITOR( TXT("Painting canceled") );
					break;
				}
			}
		}
	}
}

Bool CEdVegetationEditTool::PaintVegetation( const Vector& center, const Vector& normal, Float radius, CVegetationBrushEntry* brushEntry )
{
	CSRTBaseTree* baseTree = brushEntry->m_resource.Get();
	if ( !baseTree )
	{
		// No mesh
		return false;
	}

	if ( !baseTree->GetRenderObject() && !baseTree->CreateRenderObject() )
	{
		// Broken object. Can't render it.
		return false;
	}

	// Check if position is not occluded
	const Vector meshExtents = baseTree->GetBBox().CalcExtents();
	const Float meshRadiusScale = brushEntry->m_radiusScale > 0.0f ? brushEntry->m_radiusScale : 1.0f;
	const Float meshRadius = Max< Float >( meshExtents.A[0], meshExtents.A[1] ) * ( 1.0f / MSqrt( 2.0f ) ) * meshRadiusScale;
	const Float maxSize = brushEntry->m_size + MAbs( brushEntry->m_sizeVar );
	const Float searchRadius = radius + ( meshRadius * maxSize * 2.0f );

	if ( ( center - m_world->GetCameraPosition() ).Mag2() > MAX_PAINTING_DISTANCE )
	{
 		// Painting too far away
 		return false;
	}

	// Infos to upload to foliage container
	FoliageInstanceContainer instanceInfos;

	// Generate base distribution
	const Float placementDensity = brushEntry->m_density;
	const Float placementStep = 2.0f * maxSize * meshRadius * placementDensity;
	Int32 intSize = ceilf( radius / placementStep );

	// Get meshes to test if we do not occlude any existing mesh
	FoliageInstanceContainer instances;
	m_world->GetFoliageEditionController().GetInstancesFromArea( baseTree, center, searchRadius, instances );

	for ( Int32 y=-intSize; y<=intSize; y++ )
	{
		for ( Int32 x=-intSize; x<=intSize; x++ )
		{
			const Float realX = ( x + GEngine->GetRandomNumberGenerator().Get< Float >( -0.3f , 0.3f ) ) * placementStep;
			const Float realY = ( y + GEngine->GetRandomNumberGenerator().Get< Float >( -0.3f , 0.3f ) ) * placementStep;
			const Float dist = realX*realX + realY*realY;
			if ( dist < radius*radius )
			{
				const Float smoothSize = Max( 0.1f, brushEntry->m_size + ( brushEntry->m_sizeVar > 0.0f ? GEngine->GetRandomNumberGenerator().Get< Float >( 0.0f, brushEntry->m_sizeVar ) : 0.0f ) );
				const Float size = (Int32)( smoothSize * 20 ) / 20.0f;
				const Float spaceNeeded = size * meshRadius;
				
				Vector3 pos = GetPointOnPlane( center, normal, realX, realY );

				// Test if we are are colliding with any of the instances, if so we cannot place a new foliage here
				Bool collisionFound = false;
				for ( Uint32 i=0; i<instances.Size(); i++ )
				{
					SFoliageInstance & instance = instances[i];
					const Float instanceRadius = instance.GetScale() * meshRadius;
					const Float dist = Vector(instance.GetPosition()).DistanceTo2D( pos );
					if ( dist < ( instanceRadius + spaceNeeded ) )
					{
						collisionFound = true;
						break;
					}
				}

				// Unable to place this mesh
				if ( collisionFound )
				{
					continue;
				}

				// Find Z
				//Float height = 0.0f;
				if ( CastPoint( pos, -normal ) != VCP_Nowhere )
				{
					// Set Z
					//const Vector insertPos( pos.X, pos.Y, height );

					// Add instance info
					new ( instanceInfos ) SFoliageInstance;
					SFoliageInstance& iInfo = instanceInfos.Back();

					// Initialize parameters
					iInfo.SetPosition( pos );
					iInfo.SetScale( size );
					
					// Calculate random rotation
					const Float randomYaw = GEngine->GetRandomNumberGenerator().Get< Float >( 360.0f );
					iInfo.SetQuaternion( EulerAngles( 0.0f, 0.0f, randomYaw ).ToQuat() );
				}
			}
		}
	}

	if ( instanceInfos.Empty() )
	{
		return true; // nothing to do
	}

	if ( instanceInfos.Size() > GMaxInstanceInsertion )
	{
		GFeedback->ShowError( TXT("The brushSize is too big for this kind of vegetation") );
		return false;
	}

	return m_world->GetFoliageEditionController().AddInstances( baseTree, instanceInfos );
}

Bool CEdVegetationEditTool::PaintSize( const Vector& pos, Float radius, CVegetationBrushEntry* brushEntry, Bool isShrinking )
{
	CSRTBaseTree* baseTree = brushEntry->m_resource.Get();
	if ( !baseTree )
	{
		// No mesh
		return false;
	}

	if ( !baseTree->GetRenderObject() && !baseTree->CreateRenderObject() )
	{
		// Broken object. Can't render it.
		return false;
	}

	CUndoVegetationSize::AddStroke( *m_viewport->GetUndoManager(), brushEntry->GetBaseTree(), pos, radius, m_paintBrushStrength, isShrinking );
	return m_world->GetFoliageEditionController().ResizeInstances( brushEntry->GetBaseTree(), pos, radius, m_paintBrushStrength, isShrinking );
}

Bool CEdVegetationEditTool::PaintAlignment( const Vector& center, const Vector& normal, Float radius, CVegetationBrushEntry* brushEntry )
{
	CSRTBaseTree* baseTree = brushEntry->m_resource.Get();
	if ( !baseTree )
	{
		// No mesh
		return false;
	}

	FoliageInstanceContainer oldInstances;
	m_world->GetFoliageEditionController().GetInstancesFromArea( baseTree, center, radius, oldInstances );

	if ( oldInstances.Empty() )
	{
		return true; // nothing to do
	}

	// push the data from instances to instanceInfos to recreate them later
	FoliageInstanceContainer newInstanceInfos;
	for ( auto instIt = oldInstances.Begin(); instIt < oldInstances.End(); ++instIt )
	{
		SFoliageInstance& inst = *instIt;
		SFoliageInstance info = inst;
		Vector3 positon = info.GetPosition();
		CastPoint( positon, -normal );
		info.SetPosition( positon );
		newInstanceInfos.PushBack( info );
	}

	if ( newInstanceInfos.Size() > GMaxInstanceInsertion )
	{
		GFeedback->ShowError( TXT("The brushSize is too big for this kind of vegetation") );
		return false;
	}

	// Realigning is done by means of re-creating the instances. This way it's possible to use CUndoVegetationExistance
	// undo step, otherwise it would be quite difficult to track all the atomic changes that each alignment stroke does.
	if ( m_world->GetFoliageEditionController().RemoveInstances( baseTree, center, radius ) )
	{
		if ( m_world->GetFoliageEditionController().AddInstances( baseTree, newInstanceInfos ) )
		{
			return true;
		}
	}

	return false;
}

Bool CEdVegetationEditTool::EraseVegetation( const Vector& pos, Float radius, CSRTBaseTree* tree )
{
	Box worldBox( pos, radius );

	// Get local instances
	FoliageInstanceContainer instances;
	m_world->GetFoliageEditionController().GetInstancesFromArea( tree, pos, radius, instances );

	if ( instances.Empty() )
	{
		return true; // nothing to do
	}

	return m_world->GetFoliageEditionController().RemoveInstances( tree, pos, radius );
}

void CEdVegetationEditTool::PaintGrassMask( const Vector& center, Float radius, Bool erase )
{
#define SETBIT(b,n) ( b[ n / 8 ] |= ( 1 << ( n % 8 ) ) )
#define UNSETBIT(b,n) ( b[ n / 8 ] &= ~( 1 << ( n % 8 ) ) )

	CFoliageEditionController & foliageSceneProxy = m_world->GetFoliageEditionController();
	CGenericGrassMask* genericGrassMask = foliageSceneProxy.GetGrassMask();
	if ( genericGrassMask )
	{
		Uint8* grassMask = genericGrassMask->GetGrassMask();

		if ( !grassMask && GFeedback->AskYesNo( TXT("The grass mask is not created. Do you want to initiate it?") ) )
		{
			if ( genericGrassMask->MarkModified() )
			{
				SClipmapParameters clipmapParams;
				m_world->GetTerrain()->GetClipmapParameters( &clipmapParams );
				genericGrassMask->InitGenericGrassMask( clipmapParams.clipmapSize );
				grassMask = genericGrassMask->GetGrassMask();
			}
			else
			{
				GFeedback->ShowError( TXT("Can't modify the vegetation library file.") );
			}
		}

		if ( grassMask )
		{
			if ( genericGrassMask->MarkModified() )
			{
				Uint32 grassMaskRes = genericGrassMask->GetGrassMaskRes();
				RED_ASSERT( IsPow2( grassMaskRes ) );

				Vector kernelMin( center.X - radius, center.Y - radius, 0.0f );
				Vector kernelMax( center.X + radius, center.Y + radius, 0.0f );

				CClipMap * terrain = m_world->GetTerrain();
				Vector2 kernelMinNorm = terrain->GetTexelSpaceNormalizedPosition( kernelMin );
				Vector2 kernelMaxNorm = terrain->GetTexelSpaceNormalizedPosition( kernelMax );

				Int32 minCol = Clamp<Int32>( Int32( kernelMinNorm.X * (Float)grassMaskRes ), 0, grassMaskRes - 1 );
				Int32 maxCol = Clamp<Int32>( Int32( kernelMaxNorm.X * (Float)grassMaskRes ), 0, grassMaskRes - 1 );
				Int32 minRow = Clamp<Int32>( Int32( kernelMinNorm.Y * (Float)grassMaskRes ), 0, grassMaskRes - 1 );
				Int32 maxRow = Clamp<Int32>( Int32( kernelMaxNorm.Y * (Float)grassMaskRes ), 0, grassMaskRes - 1 );

				for ( Int32 r = minRow; r <= maxRow; ++r )
				{
					for ( Int32 c = minCol; c <= maxCol; ++c )
					{
						Int32 bitIndex = r * grassMaskRes + c;
						erase ? UNSETBIT( grassMask, bitIndex ) : SETBIT( grassMask, bitIndex );
					}
				}

				foliageSceneProxy.UpdateGrassMask( terrain->GetTerrainProxy() );
			}
			else
			{
				GFeedback->ShowError( TXT("Can't modify the vegetation library file.") );
			}
		}
	}

#undef SETBIT
#undef UNSETBIT
}

void CEdVegetationEditTool::UpdateSliderValues()
{
	m_radiusSlider->SetValue( Clamp< Int32 >( 1000.0f * MSqrt( ( m_paintBrushSize - MIN_BRUSH_SIZE ) / 100.0f ), 0, 1000 ) );
	m_strengthSlider->SetValue( Clamp< Int32 >( m_paintBrushStrength * 1000.0f, 0, 1000 ) );
}

CVegetationBrush* CEdVegetationEditTool::CreateNewBrush()
{
	CVegetationBrush* brush = CreateObject< CVegetationBrush >();
	brush->AddToRootSet();

	m_brushesListPanel->AddItem( brush );

	return brush;
}

void CEdVegetationEditTool::OnPaintOnChoice( wxCommandEvent& event )
{
	m_paintOnMode = static_cast< EPaintOnMode >( XRCCTRL( *m_dialog, "m_paintOnChoice", wxChoice )->GetSelection() );
}

void CEdVegetationEditTool::OnClose( wxCloseEvent& event )
{
	RunLaterOnce( [](){ wxTheFrame->GetToolsPanel()->CancelTool(); } );
	event.Veto();
}

void CEdVegetationEditTool::OnUpdateUI( wxUpdateUIEvent& event )
{
	if ( !m_isStarted )
	{
		return;
	}

	Bool lowLimit  = XRCCTRL( *m_dialog, "m_lowLimitCheck",  wxCheckBox )->GetValue();
	Bool highLimit = XRCCTRL( *m_dialog, "m_highLimitCheck", wxCheckBox )->GetValue();
	XRCCTRL( *m_dialog, "m_lowLimitEdit",  wxTextCtrl )->Enable( lowLimit );
	XRCCTRL( *m_dialog, "m_highLimitEdit", wxTextCtrl )->Enable( highLimit );

	m_strengthSlider->Enable( m_cursorMode == VCM_PaintSize );

	XRCCTRL( *m_dialog, "m_replaceSelected", wxButton )->Enable( m_statsList->GetSelectedItemCount() > 0 );
	XRCCTRL( *m_dialog, "m_removeSelected", wxButton )->Enable( m_statsList->GetSelectedItemCount() > 0 );

	Bool decimationEnabled = XRCCTRL( *m_dialog, "m_decimationCheck", wxCheckBox )->GetValue() != 0;
	XRCCTRL( *m_dialog, "m_decimationEdit", wxTextCtrl )->Enable( decimationEnabled );

	Bool scaleFilterEnabled = XRCCTRL( *m_dialog, "m_scaleFilterCheck", wxCheckBox )->GetValue() != 0;
	XRCCTRL( *m_dialog, "m_scaleFilterLow", wxTextCtrl )->Enable( scaleFilterEnabled );
	XRCCTRL( *m_dialog, "m_scaleFilterHigh", wxTextCtrl )->Enable( scaleFilterEnabled );
}

void CEdVegetationEditTool::OnLowLimitChoice( wxCommandEvent& event )
{
	if ( event.GetSelection() )
	{
		OnLowLimitText( event );
	}
	else
	{
		m_cursorLowLimit = TOptional< Float >();
	}

	event.Skip();
}

void CEdVegetationEditTool::OnHighLimitChoice( wxCommandEvent& event )
{
	if ( event.GetSelection() )
	{
		OnHighLimitText( event );
	}
	else
	{
		m_cursorHighLimit = TOptional< Float >();
	}

	event.Skip();
}

void CEdVegetationEditTool::OnLowLimitText( wxCommandEvent& event )
{
	Float limit;
	if ( FromString( String( XRCCTRL( *m_dialog, "m_lowLimitEdit",  wxTextCtrl )->GetValue() ), limit ) )
	{
		m_cursorLowLimit = limit;
	}
}

void CEdVegetationEditTool::OnHighLimitText( wxCommandEvent& event )
{
	Float limit;
	if ( FromString( String( XRCCTRL( *m_dialog, "m_highLimitEdit",  wxTextCtrl )->GetValue() ), limit ) )
	{
		m_cursorHighLimit = limit;
	}

	event.Skip();
}

void CEdVegetationEditTool::OnPageChanged( wxCommandEvent& event )
{
	int pageIdx = XRCCTRL( *m_dialog, "m_notebook", wxNotebook )->GetSelection();
	
	switch ( pageIdx )
	{
	case 0:
		UpdatePickShapes();
		break;
	case 1:
		m_selectedInstance.Reset();
		m_highlightedInstance.Reset();
		UpdateStatistics( m_selectedCell );
		break;
	}

	UpdateHighlightData();
	event.Skip();
}

void CEdVegetationEditTool::UpdatePickShapes()
{
	m_pickShapes.Clear();

	if ( m_highlightedCell.IsEmpty() )
	{
		return;
	}

	if ( GetToolMode() == VTM_InstanceManipulation )
	{
		Box totalBox = m_highlightedCell.GetTotalBox();
		m_world->GetFoliageEditionController().GetPickShapesFromArea( totalBox, m_pickShapes );
	}
}

void CEdVegetationEditTool::UpdateSelectedInstanceInfo()
{
	wxStaticText* info = XRCCTRL( *m_dialog, "m_selectedInstanceInfo", wxStaticText );
	if ( GetToolMode() == VTM_InstanceManipulation )
	{
		if ( !info->IsShown() )
		{
			info->Show();
			info->GetParent()->Layout();
		}

		if ( m_selectedInstance.IsEmpty() )
		{
			info->SetLabel( TXT("No instance selected") );
		}
		else
		{
			String strTreeName = m_selectedInstance.m_baseTree->GetFile()->GetFileName();

			Float angle = GetInstanceRotationMatrix( m_selectedInstance.m_instance ).ToEulerAngles().Yaw;
			Float scale = m_selectedInstance.m_instance.GetScale();
			String infoStr = String::Printf( L"Selected: [%s], Rotation: %g, Scale: %g", strTreeName.AsChar(), angle, scale );
			info->SetLabel( infoStr.AsChar() );
		}
	}
	else
	{
		if ( info->IsShown() )
		{
			info->Hide();
			info->GetParent()->Layout();
		}
	}
}

void CEdVegetationEditTool::UpdateRealTimeStatistics( Bool removeEmptyResources )
{
	m_realTimeStatistics.Clear();

	if ( m_highlightedCell.IsEmpty() )
	{
		return;
	}

	Box totalBox = m_highlightedCell.GetTotalBox();

	CFoliageEditionController& controller = m_world->GetFoliageEditionController();

	controller.GetStatisticsFromArea( totalBox, m_realTimeStatistics, false );

	if ( removeEmptyResources )
	{	// Get rid of resources that are no longer needed. It requires a consistency between the higlightedCell area
		// and gathered statistics data, so it's safer to do it (optionally) here, than in a separate method.
		for ( const SFoliageInstanceStatistics& stats : m_realTimeStatistics )
		{
			if ( stats.m_instanceCount == 0 )
			{
				// Do not try to modify the resource if it's not modifiable, because ReplaceTree would cause a silent check-out
				// that may screw up things a little bit.
				if ( controller.CanModifyResources( totalBox ) )
				{
					m_world->GetFoliageEditionController().ReplaceTree( stats.m_baseTree, nullptr, totalBox, false );
					LOG_EDITOR( TXT("Removing empty resources") );
				}
			}
		}
	}

	m_brushEntriesPanel->RefreshStats( m_realTimeStatistics );

	Uint32 totalGrass = 0;
	Uint32 totalTrees = 0;
	for ( const SFoliageInstanceStatistics& stats : m_realTimeStatistics )
	{
		if ( stats.m_baseTree->IsGrassType() )
		{
			totalGrass += stats.m_instanceCount;
		}
		else
		{
			totalTrees += stats.m_instanceCount;
		}
	}

	m_statisticsLabel->SetLabel( wxString::Format( TXT("Instances on current tile: grass %i, trees: %i"), totalGrass, totalTrees ) );
}

TDynArray< THandle< CSRTBaseTree > > CEdVegetationEditTool::GetTreesSelectedInStats() const
{
	TDynArray< THandle< CSRTBaseTree > > selectedTrees;
	for ( Int32 sel : m_statsList->GetSelectedItems() )
	{
		if ( sel < m_statistics.SizeInt() )
		{
			selectedTrees.PushBack( m_statistics[sel].m_baseTree );
		}
	}
	return selectedTrees;
}

void CEdVegetationEditTool::UpdateStatistics( const SSelectedCellDesc& cells )
{
	if ( cells.IsEmpty() )
	{
		return;
	}

	Box totalBox = cells.GetTotalBox();

//	auto selection = GetTreesSelectedInStats();

	m_statistics.Clear();
	
	wxBeginBusyCursor();
	Uint32 cellCount = m_world->GetFoliageEditionController().GetStatisticsFromArea( totalBox, m_statistics, false );
	wxEndBusyCursor();

	Int32 showForIdx = XRCCTRL( *m_dialog, "m_showStatsForChoice", wxChoice )->GetSelection();

	m_statistics.Erase(
		RemoveIf( m_statistics.Begin(), m_statistics.End(),
			[ showForIdx ]( const SFoliageInstanceStatistics& s ) {
				switch ( showForIdx )
				{
				case 0: /*all*/		return false;
				case 1: /*grass*/	return !s.m_baseTree->IsGrassType();
				case 2: /*trees*/	return s.m_baseTree->IsGrassType();
				default:			return false;
				}
			} ),
		m_statistics.End()
		);

	UpdateStatisticsList( m_statsSelectionLatch );
}

void CEdVegetationEditTool::UpdateStatisticsList( const TDynArray< THandle< CSRTBaseTree > >& treesToSelect )
{
	m_statsList->Freeze();

	switch ( m_statsSorting )
	{
	case StatsSortType::ByCountUp:
		Sort( m_statistics.Begin(), m_statistics.End(),
			[ ]( const SFoliageInstanceStatistics& a, const SFoliageInstanceStatistics& b  ) { 
				return a.m_instanceCount < b.m_instanceCount; 
			} );
		m_statsList->SetSortArrow( 0, CEdAutosizeListCtrl::SortArrow::Up );
		m_statsList->SetSortArrow( 1, CEdAutosizeListCtrl::SortArrow::None );
		break;

	case StatsSortType::ByCountDown:
		Sort( m_statistics.Begin(), m_statistics.End(),
			[ ]( const SFoliageInstanceStatistics& a, const SFoliageInstanceStatistics& b  ) { 
				return a.m_instanceCount > b.m_instanceCount; 
			} );
		m_statsList->SetSortArrow( 0, CEdAutosizeListCtrl::SortArrow::Down );
		m_statsList->SetSortArrow( 1, CEdAutosizeListCtrl::SortArrow::None );
		break;

	case StatsSortType::ByNameUp:
		Sort( m_statistics.Begin(), m_statistics.End(),
			[ ]( const SFoliageInstanceStatistics& a, const SFoliageInstanceStatistics& b  ) { 
				return a.m_baseTree->GetDepotPath() < b.m_baseTree->GetDepotPath(); 
			} );
		m_statsList->SetSortArrow( 0, CEdAutosizeListCtrl::SortArrow::None );
		m_statsList->SetSortArrow( 1, CEdAutosizeListCtrl::SortArrow::Up );
		break;

	case StatsSortType::ByNameDown:
		Sort( m_statistics.Begin(), m_statistics.End(),
			[ ]( const SFoliageInstanceStatistics& a, const SFoliageInstanceStatistics& b  ) { 
				return a.m_baseTree->GetDepotPath() > b.m_baseTree->GetDepotPath(); 
			} );
		m_statsList->SetSortArrow( 0, CEdAutosizeListCtrl::SortArrow::None );
		m_statsList->SetSortArrow( 1, CEdAutosizeListCtrl::SortArrow::Down );
		break;
	};

	m_statsList->DeleteAllItems();

	for ( Uint32 i=0; i<m_statistics.Size(); ++i )
	{
		const SFoliageInstanceStatistics& stats = m_statistics[i];
		Uint32 count = stats.m_instanceCount;
		String name = stats.m_baseTree->GetDepotPath();

		m_statsList->InsertItem( i, ToString( count ).AsChar() );
		m_statsList->SetItem( i, 1, name.AsChar() );
	}

	// Try to restore previous selection
	for ( THandle< CSRTBaseTree > tree : treesToSelect )
	{
		auto foundIt = 
			FindIf( m_statistics.Begin(), m_statistics.End(), 
				[ tree ]( SFoliageInstanceStatistics& s ) { return s.m_baseTree == tree; } );

		if ( foundIt != m_statistics.End() )
		{
			Int32 idx = foundIt - m_statistics.Begin();
			m_statsList->Select( idx );
			m_statsList->EnsureVisible( idx );
		}
	}

	m_statsList->Thaw();
}

void CEdVegetationEditTool::OnStatisticsColumnClicked( wxListEvent& event )
{
	Int32 column = event.GetColumn();

	switch ( column )
	{
	case 0:
		m_statsSorting = ( m_statsSorting == StatsSortType::ByCountUp ) ? StatsSortType::ByCountDown : StatsSortType::ByCountUp;
		break;
	case 1:
		m_statsSorting = ( m_statsSorting == StatsSortType::ByNameUp ) ? StatsSortType::ByNameDown : StatsSortType::ByNameUp;
		break;
	default:
		RED_HALT( "Unsupported column" );
		break;
	}

	UpdateStatisticsList( GetTreesSelectedInStats() );
}

void CEdVegetationEditTool::OnStatisticsItemActivated( wxListEvent& event )
{
	Int32 sel = event.GetItem().GetId();

	if ( sel < m_statistics.SizeInt() && m_statistics[sel].m_baseTree )
	{
		wxTheFrame->GetAssetBrowser()->SelectFile( m_statistics[sel].m_baseTree->GetDepotPath() );
	}
}

void CEdVegetationEditTool::OnStatisticsItemRightClick( wxListEvent& event )
{
}

void CEdVegetationEditTool::OnStatisticsSelectionChanged( wxListEvent& event )
{
	RunLaterOnce( [this]()
	{
		m_statsSelectionLatch = GetTreesSelectedInStats();
	});
}


void CEdVegetationEditTool::UpdateRenderingStatistics()
{
	if ( !m_isStarted || !GRender )
	{
		return;
	}

	SSpeedTreeResourceMetrics speedTreeStats;
	GRender->PopulateSpeedTreeMetrics( speedTreeStats );

	wxStaticText* statsLabel = XRCCTRL( *m_dialog, "m_renderingStatsLabel", wxStaticText );

	String text = String::Printf( TXT("Grass layers: %d"), speedTreeStats.m_renderStats.m_grassLayerCount );
	statsLabel->SetLabel( text.AsChar() );

	m_rendStatsTimer = 0.5;
}

void CEdVegetationEditTool::GetControlVertices( TDynArray< ControlVertex >& vertices )
{
	if ( !m_selectedInstance.IsEmpty() )
	{
		vertices.PushBack( ControlVertex(
				this, 
				0,
				m_selectedInstance.m_instance.GetPosition(),
				GetInstanceRotationMatrix( m_selectedInstance.m_instance ).ToEulerAngles(),
				m_selectedInstance.m_instance.GetScale()
			) );
	}
}

Bool CEdVegetationEditTool::FoliageInstanceTransformed( const SFoliageInstance& newInstance )
{
	// Refresh selected instance(s)
	if ( !m_selectedInstance.IsEmpty() )
	{
		m_highlightedInstance.Reset(); // disable highlighting

		// update the cursor data to make the it nicely follow the transformation (without drawing it) - for coordinates display, drawing cell extends, etc
		m_cursorPosition = newInstance.GetPosition(); 
		m_cursorNormal = Vector( 0.f, 0.f, 1.f );
		m_highlightedCell = SSelectedCellDesc( m_world->GetFoliageEditionController().GetCellExtend( m_cursorPosition ) );
		m_cursorPlacement = VCP_Nowhere;
		if ( m_world->GetFoliageEditionController().ReplantInstance( m_selectedInstance.m_baseTree, m_selectedInstance.m_instance, newInstance ) )
		{
			m_selectedInstance.m_instance = newInstance;
			UpdateSelectedInstanceInfo();
			return true;
		}
	}

	return false;
}

void CEdVegetationEditTool::OnEditorNodeTransformStart( Int32 vertexIndex )
{
	if ( RIM_IS_KEY_DOWN( IK_LShift ) )
	{
		// duplicate instance
		CFoliageEditionController& controller = m_world->GetFoliageEditionController();
		CUndoVegetationExistance::CreateStep( *m_viewport->GetUndoManager(), &m_world->GetFoliageEditionController() );

		FoliageInstanceContainer inst;
		inst.PushBack( m_selectedInstance.m_instance );
		
		Vector3 position = inst.Back().GetPosition();
		position.X += 0.01f; // just to differentiate (there cannot be two instances in the exact same spot)
		inst.Back().SetPosition( position ); 

		controller.AddInstances( m_selectedInstance.m_baseTree, inst );
	}
}

void CEdVegetationEditTool::OnEditorNodeMoved( Int32 vertexIndex, const Vector& oldPosition, const Vector& wishedPosition, Vector& allowedPosition )
{
	SFoliageInstance newInstance = m_selectedInstance.m_instance;
	newInstance.SetPosition(  wishedPosition );
	if ( FoliageInstanceTransformed( newInstance ) )
	{
		allowedPosition = wishedPosition;
	}
	else
	{
		allowedPosition = oldPosition;
	}
}

void CEdVegetationEditTool::OnEditorNodeRotated( Int32 vertexIndex, const EulerAngles& oldRotation, const EulerAngles& wishedRotation, EulerAngles& allowedRotation )
{
	SFoliageInstance newInstance = m_selectedInstance.m_instance;
	newInstance.SetQuaternion( EulerAngles( 0.f, 0.f, wishedRotation.Yaw ).ToQuat() ); // allow only rotation around Z
	if ( FoliageInstanceTransformed( newInstance ) )
	{
		allowedRotation = wishedRotation;
	}
	else
	{
		allowedRotation = oldRotation;
	}
}

void CEdVegetationEditTool::OnEditorNodeScaled( Int32 vertexIndex, const Vector& oldScale, const Vector& wishedScale, Vector& allowedScale )
{
	SFoliageInstance newInstance = m_selectedInstance.m_instance;
	newInstance.SetScale( wishedScale.X ); // only uniform scale supported
	if ( FoliageInstanceTransformed( newInstance ) )
	{
		allowedScale = Vector( wishedScale.X, wishedScale.X, wishedScale.X );
	}
	else
	{
		allowedScale = oldScale;
	}
}

void CEdVegetationEditTool::OnEditorNodeTransformStop( Int32 vertexIndex )
{
	UpdatePickShapes();
}
