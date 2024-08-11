#pragma once 

#ifdef USE_UMBRA
#ifndef NO_UMBRA_DATA_GENERATION

#include "../../common/engine/umbraJobs.h"

struct SComputationParameters;
enum EUmbraTileDataStatus : Int32;

//////////////////////////////////////////////////////////////////////////
// enums

enum
{
	wxID_UMBRAEDITOR_REGENERATE			= 9500,
	wxID_UMBRAEDITOR_TELEPORT			= 9501,
	wxID_UMBRAEDITOR_CANCEL_GENERATION	= 9502,
	wxID_UMBRAEDITOR_SET_MIN_BOUNDS		= 9503,
	wxID_UMBRAEDITOR_SET_MAX_BOUNDS		= 9504,
	wxID_UMBRAEDITOR_CLEARBOUNDS		= 9505,
	wxID_UMBRAEDITOR_DUMPSCENE			= 9506,
};

//////////////////////////////////////////////////////////////////////////
// Interfaces

class IOcclusionDataRegenerator
{
public:
	virtual void TeleportTo( CEdRenderingPanel* viewport, const VectorI& tile ) = 0;
	virtual void RegenerateMultipleTiles( const VectorI& bounds ) = 0;
	virtual void RegenerateMultipleTilesSync( const VectorI& bounds ) = 0;
	virtual void RegenerateSingleTile( const VectorI& id ) = 0;
	virtual void DumpSceneForTile( const VectorI& id ) = 0;
	virtual void RegenerateSingleTileSync( const VectorI& id ) = 0;
	virtual void CancelRegeneration( const VectorI& id ) = 0;
};

class ITileStatusRenderer
{
public:
	virtual void OnItemProgressChanged( const VectorI& id, Float progress ) = 0;
	virtual void OnItemStatusChanged( const VectorI& id, EUmbraTileDataStatus status ) = 0;

	virtual void SetTileParameters( const VectorI& id, const SComputationParameters& computationParams ) = 0;

	void Initialize( IOcclusionDataRegenerator* regenerator ) { m_dataRegenerator = regenerator; }

protected:
	IOcclusionDataRegenerator*	m_dataRegenerator;
};

//////////////////////////////////////////////////////////////////////////
// Data classes

class CTileGrid : public IEdEventListener, public IOcclusionDataRegenerator
{
public:
	CTileGrid( CWorld* world, ITileStatusRenderer* gridRenderer = nullptr, ITileStatusRenderer* queueRenderer = nullptr );
	virtual ~CTileGrid();

public:
	void SetTomeDataGenerationContext( const STomeDataGenerationContext& context ) { m_context = context; }

	// IEdEventListener implementation begin
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );
	// IEdEventListener implementation end

	// IOcclusionDataRegenerator implementation begin
	virtual void TeleportTo( CEdRenderingPanel* viewport, const VectorI& tile );
	virtual void RegenerateMultipleTiles( const VectorI& bounds );
	virtual void RegenerateMultipleTilesSync( const VectorI& bounds );
	virtual void RegenerateSingleTile( const VectorI& id );
	virtual void DumpSceneForTile( const VectorI& id );
	virtual void RegenerateSingleTileSync( const VectorI& id );
	virtual void CancelRegeneration( const VectorI& id );
	// IOcclusionDataRegenerator implementation end

private:
	CWorld*						m_world;
	ITileStatusRenderer*		m_gridRenderer;
	ITileStatusRenderer*		m_queueRenderer;

	VectorI						m_tilesCount;
	STomeDataGenerationContext	m_context;
};

//////////////////////////////////////////////////////////////////////////
// UI classes

class CTileGridViewPanel : public wxPanel
{
public:
	CTileGridViewPanel( CEdRenderingPanel* viewport, wxPanel* parent, Float worldSize );

protected:
	CEdRenderingPanel*	m_viewport;
	Float				m_worldSize;
	VectorI				m_bounds;

public:
	RED_INLINE const VectorI& GetBounds() const { return m_bounds; }

public:
	void UpdateBounds( Bool minBounds, const VectorI& id );
	void ClearBounds();

public:
	void OnPaintEvent( wxPaintEvent & evt );
	void ForceRender();
	void Render( wxDC& dc );
	void OnMouseMoved( wxMouseEvent& event );
	void OnMouseDown( wxMouseEvent& event );
	void OnMouseUp( wxMouseEvent& event );
	void OnMouseRightClick( wxMouseEvent& event );

	DECLARE_EVENT_TABLE()
};

//////////////////////////////////////////////////////////////////////////

struct STileGridViewItem
{
	wxStaticBitmap*			m_bitmap;
	VectorI					m_index;
	SComputationParameters	m_parameters;
};

class CTileGridView : public ITileStatusRenderer, public wxGridSizer
{
public:
	CTileGridView( CEdRenderingPanel* viewport, CTileGridViewPanel* parent, wxStaticText* hoveredText, const VectorI& tilesCount, Bool isSaperLayout );
	virtual ~CTileGridView();

public:
	RED_INLINE VectorI				GetTilesCount() const { return m_tilesCount; }
	
public:
	// ITileStatusRenderer implementation begin
	virtual void OnItemProgressChanged( const VectorI& id, Float progress );
	virtual void OnItemStatusChanged( const VectorI& id, EUmbraTileDataStatus status );
	virtual void SetTileParameters( const VectorI& id, const SComputationParameters& computationParams );
	// ITileStatusRenderer implementation begin

protected:
	void							OnContextMenu( wxMouseEvent& event );
	void							OnTileIconHovered( wxMouseEvent& event );
	void							OnRegenerateContext( wxCommandEvent& event );
	void							OnDumpSceneContext( wxCommandEvent& event );
	void							OnTeleportToTile( wxCommandEvent& event );
	void							OnCancelDataRegeneration( wxCommandEvent& event );
	void							OnSetMinBounds( wxCommandEvent& event );
	void							OnSetMaxBounds( wxCommandEvent& event );
	void							OnClearBounds( wxCommandEvent& event );

private:
	const wxBitmap*					GetIconForStatus( EUmbraTileDataStatus status ) const;

private:
	VectorI							m_tilesCount;
	CEdRenderingPanel*				m_viewport;
	CTileGridViewPanel*				m_parent;
	
	// controls
	wxObject*						m_lastContextMenuObject;
	TDynArray< STileGridViewItem >	m_items;
	wxStaticText*					m_hoveredText;

	// icons
	wxBitmap						m_iconValid;
	wxBitmap						m_iconInvalid;
	wxBitmap						m_iconInProgress;
	wxBitmap						m_iconUnknown;
	wxBitmap						m_iconNoData;
};

class CTileRegenerationQueueView;

class CQueueItem : public wxPanel
{
public:
	CQueueItem( wxWindow* parent, const VectorI& id );
	~CQueueItem();

public:
	RED_INLINE const VectorI& GetId() const { return m_id; }

public:
	void UpdateProgress( Float progress );

private:
	CTileRegenerationQueueView*	m_parentList;
	wxStaticText*				m_tileIdLabel;
	wxGauge*					m_progressGauge;
	wxStaticText*				m_progressLabel;
	wxBitmapButton*				m_cancelButton;
	VectorI						m_id;
};

class CTileRegenerationQueueView : public ITileStatusRenderer, public wxScrolledWindow
{
public:
	CTileRegenerationQueueView( wxWindow* parent );

	void OnCancelButtonClicked( wxCommandEvent& event );

public:
	// ITileStatusRenderer implementation begin
	virtual void OnItemProgressChanged( const VectorI& id, Float progress );
	virtual void OnItemStatusChanged( const VectorI& id, EUmbraTileDataStatus status );
	virtual void SetTileParameters( const VectorI& id, const SComputationParameters& computationParams );
	// ITileStatusRenderer implementation end

	void ItemAdded( const VectorI& id );
	void ItemDeleted( const VectorI& id );

private:
	TDynArray< CQueueItem* >	m_items;
	wxPanel*					m_parent;
};

//////////////////////////////////////////////////////////////////////////

class CEdUmbraTool : public IEditorTool, public ISavableToConfig, public wxWindow
{
	DECLARE_ENGINE_CLASS( CEdUmbraTool, IEditorTool, 0 );

public:
	CEdUmbraTool();
	virtual ~CEdUmbraTool();

protected:
	CEdRenderingPanel*				m_viewport;
	CWorld*							m_world;
	SComputationParameters			m_computationParams;
	
protected:
	// world properties
	Float							m_tileSize;
	VectorI							m_cellCounts;
	CTileGrid*						m_tileGrid;

protected:
	// controls
	wxDialog*						m_dialog;
	wxChoice*						m_smallestOccluderCtrl;
	wxChoice*						m_smallestHoleCtrl;
	wxChoice*						m_umbraTileSizeCtrl;
	wxButton*						m_regenerateSelectedButton;
	wxButton*						m_regenerateAllButton;
	wxButton*						m_regenerateInvalidButton;
	CTileGridViewPanel*				m_statusPanel;
	CTileRegenerationQueueView*		m_queueView;
	wxStaticText*					m_hoveredText;
	CTileGridView*					m_tileGridView;
	Red::Math::Random::Generator< Red::Math::Random::StandardRand >		m_randomGenerator;

private:
	Bool							m_isStarted;

public:
	RED_INLINE CWorld*			GetWorld() const { return m_world; }

public:
	virtual String					GetCaption() const { return TXT("Umbra"); }

	virtual Bool					Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection );
	virtual void					End();

public:
	void							OnClose( wxCloseEvent& event );
	void							OnRegenerateSelected( wxCommandEvent& event );
	void							OnRegenerateAll( wxCommandEvent& event );
	void							OnRegenerateInvalid( wxCommandEvent& event );
	void							OnRegenerateContext( wxCommandEvent& event );	
	void							OnContextChanged( wxCommandEvent& event );

private:
	void							InitializeControls();
	void							InitializeTileStatusGrid();

public:
	//////////////////////////////////////////////////////////////////////////
	// ISavableToConfig
	virtual void					SaveOptionsToConfig();
	virtual void					LoadOptionsFromConfig();
	virtual void					SaveSession( CConfigurationManager &config );
	virtual void					RestoreSession( CConfigurationManager &config );
	//////////////////////////////////////////////////////////////////////////
};

BEGIN_CLASS_RTTI( CEdUmbraTool );
PARENT_CLASS( IEditorTool );
END_CLASS_RTTI();

#endif // NO_UMBRA_DATA_GENERATION
#endif // USE_UMBRA