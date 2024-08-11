/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "draggableFloatingPointControl.h"
#include "customControlList.h"
#include "controlVertexHandler.h"

#include "../../common/engine/foliageCell.h"
#include "../../common/engine/foliageInstance.h"

class CEdCustomControlListPanel;
class CWXThumbnailImage;
class CEdAutosizeListCtrl;
struct SFoliageInstanceStatistics;
struct SFoliagePickShape;


class CEdBrushEntry : public CEdCustomControl
{
public:
	CEdBrushEntry( wxWindow* parent, CVegetationBrushEntry*	entry );

	void SetInstanceCount( Uint32 instanceCount );

private:
	void OnPaintThumbnail( wxPaintEvent& event );
	void UpdateThumbnail();
	virtual void DoSetup() override;

	CVegetationBrushEntry*	m_entry;
	wxStaticBitmap*			m_thumbnail;
	CWXThumbnailImage*		m_thumbnailImage;
	Uint32					m_instanceCount;

	CEdDraggableFloatValueEditor	m_radiusScaleEditor;
	CEdDraggableFloatValueEditor	m_sizeEditor;
	CEdDraggableFloatValueEditor	m_sizeVarEditor;
};

class CEdBrush : public CEdCustomControl
{
public:
	CEdBrush( wxWindow* parent, CVegetationBrush* brush );
	~CEdBrush();

	// Change current lock state.
	void SwitchLock();
	Bool Islocked() const { return m_isLocked; }

private:
	virtual void DoSetup() override;

	CVegetationBrush*		m_brush;
	wxStaticBitmap*			m_lockBitmap;
	Bool					m_isLocked;
};

class CEdBrushEntriesPanel : public CEdCustomControlListPanel
{

public:
	CEdBrushEntriesPanel( wxWindow* parent, CVegetationBrush* brush, CEdVegetationEditTool* tool );

	void SetBrush( CVegetationBrush* brush );

	void SelectItems( const TDynArray< CSRTBaseTree* >& baseTrees );

	void RefreshStats( const TDynArray< SFoliageInstanceStatistics >& stats );

private:
	// What objects does this list contain?
	virtual CClass* GetExpectedObjectClass() const override;

	// Create an instance of the custom control
	virtual CEdCustomControl* CreateCustomControl( CObject* object ) override;

	// Item was just added to the list
	virtual void OnAddItem( CObject* object, CEdCustomControl* customControl ) override;

	// Item is about to be removed
	virtual Bool OnItemRemove( CObject* object, CEdCustomControl* customControl ) override;

	// Validate object for addition
	virtual Bool CanAddItem( CObject* object ) const override;

	// An item has been selected.
	virtual void PostItemSelection( CObject* object, CEdCustomControl* customControl ) override;

	// Mouse cursor entered the item area
	virtual void OnItemHovered( CEdCustomControl* control ) override;

	void OnMouseEnter( wxMouseEvent& event );

	CVegetationBrush*		m_theBrush;
	CEdVegetationEditTool*	m_tool;
};

class CEdVegetationEditTool;

class CEdBrushesPanel : public CEdCustomControlListPanel
{
public:
	CEdBrushesPanel( wxWindow* parent, CEdVegetationEditTool* tool );

	// Loads and adds a brush. Returns added brush on success, nullptr on failure.
	CVegetationBrush* AddBrush( const String& depotPath );

	// Switch lock status on currently selected item (meaning remove lock from any previously locked item)
	void PerformBrushLock();

	// If there is a locked brush on a list, get it.
	CVegetationBrush* GetLockedBrush();

	// Get the list of currently displayed brushes
	const TDynArray< THandle< CVegetationBrush > >& GetCurrentBrushes() const;

	// Rebuilds the panel based on stored brushes paths (call it if a brush resource was reloaded)
	void Rebuild();

private:
	// What objects does this list contain?
	virtual CClass* GetExpectedObjectClass() const override;

	// Create an instance of the custom control
	virtual CEdCustomControl* CreateCustomControl( CObject* object ) override;

	// Item was just added to the list
	virtual void OnAddItem( CObject* object, CEdCustomControl* customControl ) override;

	// Item is about to be removed
	virtual Bool OnItemRemove( CObject* object, CEdCustomControl* customControl ) override;

	// Validate object for addition
	virtual Bool CanAddItem( CObject* object ) const override;

	// An item has been selected.
	virtual void PostItemSelection( CObject* object, CEdCustomControl* customControl ) override;

	TDynArray< THandle< CVegetationBrush > > m_brushes;
	TDynArray< String >						 m_paths;
	String									 m_activePath;
	CEdVegetationEditTool*					 m_tool;
};

enum EVegetationEditorMode
{
	VTM_Painting,
	VTM_InstanceManipulation,
	VTM_Statistics
};

enum EVegetationCursorMode
{
	VCM_PaintInstances = 0,
	VCM_PaintSize,      //1
	VCM_PaintAligment,  //2
	VCM_PickInstance,   //3
	VCM_PaintGrassMask, //4
	VPM_None
};

enum EVegetatonCursorPlacement
{
	VCP_Nowhere,
	VCP_Terrain,
	VCP_Mesh
};

enum EPaintOnMode
{
	POM_All,
	POM_Terrain,
	POM_StaticMeshes
};

struct SSelectedInstanceDesc;

class CEdVegetationEditTool 
	: public IEditorTool, public IEdEventListener, public ISavableToConfig, public wxEvtHandler
	, public CEdControlVertexHandler
	, public IEditorNodeMovementHook
{
	DECLARE_ENGINE_CLASS( CEdVegetationEditTool, IEditorTool, 0 );

public:

	CEdVegetationEditTool();
	~CEdVegetationEditTool();

	// ----------------------------------------------------------------------
	// IEditorTool implementation
	// ----------------------------------------------------------------------

	// Edit mode control
	virtual String GetCaption() const override;
	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* m_panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection ) override;
	virtual void End() override;

	// Set currently active brush (display it's contents, and allow painting with it)
	void SetActiveBrush( CVegetationBrush* brush );
	CVegetationBrush* GetActiveBrush() const;
	void UpdateThumbnailSource( CDiskFile* file );

	// A brush entry was selected
	void OnBrushSelected( CVegetationBrush* entry );

	virtual void OnEditorNodeTransformStart( Int32 vertexIndex ) override;
	virtual void OnEditorNodeMoved( Int32 vertexIndex, const Vector& oldPosition, const Vector& wishedPosition, Vector& allowedPosition ) override;
	virtual void OnEditorNodeRotated( Int32 vertexIndex, const EulerAngles& oldRotation, const EulerAngles& wishedRotation, EulerAngles& allowedRotation ) override;
	virtual void OnEditorNodeScaled( Int32 vertexIndex, const Vector& oldScale, const Vector& wishedScale, Vector& allowedScale ) override;
	virtual void OnEditorNodeTransformStop( Int32 vertexIndex ) override;

private:

	// Internal types and typedefs

	enum ETerrainBoxOptions
	{
		TBO_DrawCross      = FLAG(0),
		TBO_BePrecise      = FLAG(1),
		TBO_SkipRightEdge  = FLAG(2),
		TBO_SkipBottomEdge = FLAG(3)
	};

	enum class StatsSortType
	{
		ByCountUp, ByCountDown, ByNameUp, ByNameDown
	};

	struct SSelectedInstanceDesc
	{
		SFoliageInstance	m_instance;
		Vector				m_position;
		CSRTBaseTree*		m_baseTree;

		SSelectedInstanceDesc();
		SSelectedInstanceDesc( CSRTBaseTree* baseTree, const SFoliageInstance & instance );

		Bool IsEmpty() const;
		void Reset();

		Bool operator ==( const SSelectedInstanceDesc& r ) const;
		Bool operator !=( const SSelectedInstanceDesc& r ) const;
	};

	struct SSelectedCellDesc
	{
		Box   m_centerBox; // the extend of the (single) cell in the center of the selection
		Int32 m_radius;

		SSelectedCellDesc();
		SSelectedCellDesc( const Box& centerBox, Int32 radius = 1 );

		Bool IsEmpty() const;
		void Reset();

		Bool operator ==( const SSelectedCellDesc& r ) const;
		Bool operator !=( const SSelectedCellDesc& r ) const;

		Box GetTotalBox() const;
	};

	// ------------------------------------------------------------------------
	// Methods for maintaining tool mode (painting, picking, showing brush, etc.)
	// ------------------------------------------------------------------------
	void UpdateCursorData( const CMousePacket& packet );
	void UpdateHighlightData();
	Color GetCursorColor() const;
	void DrawTerrainLine( const Vector& start, const Vector& end, const Color& color, Bool bePrecise, CRenderFrame* frame );
	void DrawTerrainBox( const Box& box, const Color& color, Int32 options, CRenderFrame* frame );
	void DrawPaintingCursorPart( CRenderFrame* frame, Float size );
	void DrawPaintingCursor( CRenderFrame* frame );
	void DrawCellMarker( const SSelectedCellDesc& desc, const Color& color, Bool cross, Bool bePrecise, CRenderFrame* frame );
	void DrawInstanceMarker( const SSelectedInstanceDesc& desc, const Color& color, CRenderFrame* frame );
	void DrawCursor( CRenderFrame* frame );

	void DoPaint();
	Bool DoSelectInstance();
	void DoPickBrush();
	Bool PaintVegetation( const Vector& center, const Vector& normal, Float radius, CVegetationBrushEntry* brushEntry );
	Bool PaintSize( const Vector& pos, Float radius, CVegetationBrushEntry* brushEntry, Bool isShrinking );
	Bool PaintAlignment( const Vector& pos, const Vector& normal, Float radius, CVegetationBrushEntry* brushEntry );
	Bool EraseVegetation( const Vector& pos, Float radius, CSRTBaseTree* tree );
	void PaintGrassMask( const Vector& center, Float radius, Bool erase );
	
	void UpdateSliderValues();

	EVegetationEditorMode GetToolMode() const;

	// ------------------------------------------------------------------------
	// Convenience methods for managing brushes
	// ------------------------------------------------------------------------

	// Create a new brush, not bound to any file. 
	CVegetationBrush* CreateNewBrush();

	EVegetatonCursorPlacement ClampPositionZ( Vector3& pos, EVegetatonCursorPlacement origPlacement ) const;
	EVegetatonCursorPlacement CastPoint( Vector3& position, const Vector& direction );

	// Scan folders for vegetation
	void UpdateGrassMask();

	void UpdatePickShapes();
	void UpdateSelectedInstanceInfo();
	void UpdateRealTimeStatistics( Bool removeEmptyResources = false );
	void UpdateStatistics( const SSelectedCellDesc& cells );
	TDynArray< THandle< CSRTBaseTree > > GetTreesSelectedInStats() const;
	void UpdateStatisticsList( const TDynArray< THandle< CSRTBaseTree > >& treesToSelect );
	void UpdateRenderingStatistics();

	Bool FoliageInstanceTransformed( const SFoliageInstance& newInstance );
	
	TOptional< Float > GetDecimation() const;
	TOptional< TPair< Float, Float > > GetScaleFilter() const;
	Uint32 DoReplaceTreeForStatsSelection( CSRTBaseTree* newTreeResource, const Float* resetScale );
	Uint32 DoReplaceTreeWithMeshForStatsSelection( CMesh* meshResource, CLayerInfo* layerForTreeMeshes );

private:

	// CEdControlVertexHandler implementation

	virtual void GetControlVertices( TDynArray< ControlVertex >& vertices ) override;

	// IEditorTool overrides

	virtual Bool OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame ) override;
	virtual Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y ) override;
	virtual Bool OnViewportMouseMove( const CMousePacket& packet ) override;
	virtual Bool OnViewportTrack( const CMousePacket& packet ) override;
	virtual Bool OnViewportTick( IViewport* view, Float timeDelta ) override;
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data ) override;

	// wxWidgets events

	void OnToggleModeClicked( wxCommandEvent& event );
	void OnNewBrushTool( wxCommandEvent& event );
	void OnSaveBrushTool( wxCommandEvent& event );
	void OnCopyBrushTool( wxCommandEvent& event );
	void OnLockBrushTool( wxCommandEvent& event );
	void OnPaintThumbnail( wxPaintEvent& event );
	void OnReplaceTree( wxCommandEvent& event );
	void OnRemoveTree( wxCommandEvent& event );
	void OnRemoveDuplicates( wxCommandEvent& event );
	void OnShowStatsForChanged( wxCommandEvent& event );
	void OnRefreshGenericGrass( wxCommandEvent& event );
	void OnRadiusChanged( wxScrollEvent& event );
	void OnStrengthChanged( wxScrollEvent& event );
	void OnFormatUpdate( wxCommandEvent& event );
	void OnPaintOnChoice( wxCommandEvent& event );
	void OnClose( wxCloseEvent& event );
	void OnUpdateUI( wxUpdateUIEvent& event );
	void OnLowLimitChoice( wxCommandEvent& event );
	void OnHighLimitChoice( wxCommandEvent& event );
	void OnLowLimitText( wxCommandEvent& event );
	void OnHighLimitText( wxCommandEvent& event );
	void OnPageChanged( wxCommandEvent& event );
	void OnStatisticsColumnClicked( wxListEvent& event );
	void OnStatisticsItemActivated( wxListEvent& event );
	void OnStatisticsItemRightClick( wxListEvent& event );
	void OnStatisticsSelectionChanged( wxListEvent& event );

	// IEdEventListener overrides

	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data ) override;

	// ISavableToConfig implementation

	virtual void SaveOptionsToConfig() override;
	virtual void LoadOptionsFromConfig() override;
	virtual void SaveSession( CConfigurationManager &config ) override;
	virtual void RestoreSession( CConfigurationManager &config ) override;

private:
	Bool				 			m_isStarted;			//!< Flag the tool as initialized

	CWorld*				 			m_world;				//!< Word in the main window
	CEdRenderingPanel*	 			m_viewport;				//!< Viewport in the main window

	CEdBrushesPanel*	 			m_brushesListPanel;		//!< A panel with brushes
	CEdBrushEntriesPanel*			m_brushEntriesPanel;	//!< A panel with brush entries list
	CEdPropertiesPage*	 			m_properties;			//!< Properties of the selected brush entry
	CEdFileDialog					m_brushSaveDialog;		//!< Dialog window for saving brushes
	CDiskFile*						m_thumbnailSource;		//!< File from which we take thumbnail currently

	THandle< CVegetationBrush >	 	m_activeBrush;			//!< Currently active brush
	Float				 			m_paintBrushSize;		//!< Size of the brush
	Float				 			m_paintBrushStrength;	//!< Strength of the brush	
	Float				 			m_paintTimer;			//!< Apply brush with time interval (not every frame)

	EPaintOnMode					m_paintOnMode;			//!<
	Vector							m_cursorRayOrigin;		//!<
	Vector							m_cursorRayDirection;	//!<
	Vector				 			m_cursorPosition;		//!< Position of the cursor (brush, instances picker, etc)
	Vector							m_cursorNormal;			//!< Normal at the position of the cursor
	EVegetatonCursorPlacement		m_cursorPlacement;		//!< Is cursor on the terrain or on a mesh
	EVegetationCursorMode			m_cursorMode;			//!< Mode of the cursor (paint veg., paint size, pick, etc.)
	Bool				 			m_isPaintingNow;		//!< Are we during a brush stroke?
	Bool							m_cursorQuickPickMode;	//!< Are we during quick-picking?
	TOptional< Float >				m_cursorLowLimit;		//!<
	TOptional< Float >				m_cursorHighLimit;		//!<

	SSelectedInstanceDesc			m_highlightedInstance;	//!< Descriptor of the highlighted instance
	SSelectedInstanceDesc			m_selectedInstance;		//!< Descriptor of the selected instance
	Int32							m_cellStatisticCursorRadius;
	SSelectedCellDesc				m_highlightedCell;
	SSelectedCellDesc				m_selectedCell;

	TDynArray< SFoliagePickShape >			m_pickShapes;
	TDynArray< SFoliageInstanceStatistics > m_realTimeStatistics;	// statistics gathered from the current cell
	TDynArray< SFoliageInstanceStatistics > m_statistics;			// statistics gathered on demand
	TDynArray< THandle< CSRTBaseTree > >	m_statsSelectionLatch;
	Float									m_rendStatsTimer;

	// wx controls
	wxFrame*		m_dialog;
	wxToggleButton*	m_modeButtons[5];		//!< Toggle buttons for painting modes
	wxSlider*		m_radiusSlider;			//!< Radius size slider control
	wxSlider*		m_strengthSlider;		//!< Brush strength slider control
	wxChoice*		m_paintOnChoice;
	wxStaticBitmap*	m_thumbnailBitmap;		//!< Thumbnail previewing currently hovered SRT
	wxButton*		m_performUpdateButton;	//!< Hidden by default. Button launching full streaming data conversion.
	wxStaticText*	m_statisticsLabel;		//!<

	// stats page
	wxStaticText*		 m_renderingStatsLabel;
	CEdAutosizeListCtrl* m_statsList;
	StatsSortType		 m_statsSorting;
};

BEGIN_CLASS_RTTI( CEdVegetationEditTool );
PARENT_CLASS( IEditorTool );
END_CLASS_RTTI();
