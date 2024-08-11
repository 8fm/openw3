#pragma once

class CEdPathlib;

#include "../../common/engine/pathlibGenerationManager.h"
#include "../../common/engine/pathlibWorld.h"

#include "detachablePanel.h"

namespace PathLib
{
	class CSearchData;
	class CNavmeshRenderer;
	class CNavmesh;
};			// namespace PathLib

wxDECLARE_EVENT( wxEVT_VIEWTOOL_TOGGLED, wxCommandEvent );
wxDECLARE_EVENT( wxEVT_EXCLUSIVETOOL_TOGGLED, wxCommandEvent );

struct CEdViewToolData : public wxClientData
{
	Int32 id;
};

class CEdPathlibPanel : public CEdDraggablePanel
{
	wxDECLARE_EVENT_TABLE();
	wxDECLARE_DYNAMIC_CLASS( CEdPathlibPanel );

public:
	class CPathlibListener : public PathLib::CGenerationManager::CStatusListener
	{
	protected:
		CEdPathlibPanel*			m_owner;
	public:
		CPathlibListener( CWorld* world, CEdPathlibPanel* panel )
			: PathLib::CGenerationManager::CStatusListener( world )
			, m_owner( panel ) {}
		virtual void WorldDestroyedLocked();
		virtual void TaskStatusChangedLocked( PathLib::AreaId taskId, ETaskStatus status, const String& taskDescription );
	};

	typedef long StatusId;

public:
	CEdPathlibPanel( CEdRenderingPanel* viewport, CEdPathlib* tool, wxWindow* parent, CWorld* world );
	~CEdPathlibPanel();

	void SetStatus( PathLib::AreaId id, const String& message, PathLib::CGenerationManager::CStatusListener::ETaskStatus status );

	void Log( const Char* message, ... );
	void Update();

	template < typename Class, typename EventHandler >
	void AddTool( const String& text, const Char* image, void (Class::*method)( wxCommandEvent& ), EventHandler* handler )
	{
		wxBitmap img = SEdResources::GetInstance().LoadBitmap( image );
		const Char* toolText = text.AsChar();

		m_optionBar->AddTool( m_wxIds, toolText, img, toolText );
		m_optionBar->Bind( wxEVT_COMMAND_TOOL_CLICKED, method, handler, m_wxIds );

		m_optionBar->Realize();

		++m_wxIds;
	}

	template < typename Class, typename EventHandler >
	wxToolBarToolBase* AddExclusiveToggleTool( const String& text, const Char* image, void (Class::*method)( wxCommandEvent& ), EventHandler* handler )
	{
		wxBitmap img = SEdResources::GetInstance().LoadBitmap( image );
		const Char* toolText = text.AsChar();

		wxToolBarToolBase* tool = m_exclusiveToggleBar->AddCheckTool( m_wxIds, toolText, img, wxNullBitmap, toolText );
		m_exclusiveToggleBar->Bind( wxEVT_COMMAND_TOOL_CLICKED, &CEdPathlibPanel::OnExclusiveToolToggled, this, m_wxIds );

		m_exclusiveToggleBar->Realize();

		Bind( wxEVT_EXCLUSIVETOOL_TOGGLED, method, handler, m_wxIds );

		++m_wxIds;

		return tool;
	}

	template < typename Class, typename EventHandler >
	wxToolBarToolBase* AddToggleTool( const String& text, const Char* image, void (Class::*method)( wxCommandEvent& ), EventHandler* handler, Bool toggled = false )
	{
		wxBitmap img = SEdResources::GetInstance().LoadBitmap( image );
		const Char* toolText = text.AsChar();

		wxToolBarToolBase* tool = m_optionBar->AddCheckTool( m_wxIds, toolText, img, wxNullBitmap, toolText );
		tool->Toggle( toggled );
		m_optionBar->Bind( wxEVT_COMMAND_TOOL_CLICKED, method, handler, m_wxIds );

		m_optionBar->Realize();

		++m_wxIds;

		return tool;
	}

	template < typename Class, typename EventHandler >
	int AddView( const String& text, void (Class::*method)( wxCommandEvent& ), EventHandler* handler, Bool checked = false )
	{
		CEdViewToolData* data = new CEdViewToolData();
		data->id = m_wxIds++;

		int row = m_viewList->Append( text.AsChar(), data );
		Bind( wxEVT_VIEWTOOL_TOGGLED, method, handler, data->id );
		if ( checked )
		{
			m_viewList->Check( row, true );
		}
		return row;
	}

	Int32 GetBrushRadius() const { return m_brushRadius; }

private:
	Bool IsRowVisible( long row, wxListCtrl* listCtrl ) const;
	CPathLibWorld* GetPathlib();
	void SetRenderingFlag( EShowFlags flag, Bool b ) const;

	// WX Events
private:
	void OnExclusiveToolToggled( wxCommandEvent& event );
	void OnViewItemChecked( wxCommandEvent& event );
	void OnBrushRadiusChangeSpin( wxSpinEvent& event );
	void OnBrushRadiusChangeSlider( wxCommandEvent& event );
	void OnAutogenerationOnToggle( wxCommandEvent& event );
	void OnSCCOffToggle( wxCommandEvent& event );
	void OnObstaclesGenerationOffToggle( wxCommandEvent& event );

	// Test functions
public:
	void OnConvertAllNavmeshes( wxCommandEvent& event );
	void OnCleanupLocalFolder( wxCommandEvent& event );
	void OnCook( wxCommandEvent& event );
	void OnReinitializeSystem( wxCommandEvent& event );
	void OnRecalculateWaypoints( wxCommandEvent& event );
	void OnGlobalRecalculation( wxCommandEvent& event );

	void OnShowNavgraph0( wxCommandEvent& event );
	void OnShowNavgraph1( wxCommandEvent& event );
	void OnShowNavgraph2( wxCommandEvent& event );
	void OnShowNavgraph3( wxCommandEvent& event );

	void OnViewNavmesh( wxCommandEvent& event );
	void OnViewNavmeshTriangles( wxCommandEvent& event );
	void OnViewNavmeshOverlay( wxCommandEvent& event );
	void OnViewNavgraph( wxCommandEvent& event );
	void OnViewNavgraphNoOcclusion( wxCommandEvent& event );
	void OnViewNavgraphRegions( wxCommandEvent& event );
	void OnViewTerrain( wxCommandEvent& event );
	void OnViewObstacles( wxCommandEvent& event );

	void OnBrushWalkable( wxCommandEvent& event );
	void OnBrushNonWalkable( wxCommandEvent& event );

	void OnPathDebuggerToggle( wxCommandEvent& event );
	void OnRecalculatorToggle( wxCommandEvent& event );
	void OnNavmeshEditorToggle( wxCommandEvent& event );
	void OnNavmeshDeleterToggle( wxCommandEvent& event );

private:
	enum EStatusColumn
	{
		SC_Message = 0,
		SC_Status,

		SC_Max
	};

private:
	// Slapa - I don't think m_owner or m_world are needed by CEdPathlibPanel, but I'll leave them here to remove at your discretion
	CEdDetachablePanel	m_detachablePanel;

	CEdPathlib* m_owner;
	THandle< CWorld > m_world;

	wxListCtrl*	m_status;
	Int32 m_previousActiveItem;
	
	wxListCtrl* m_log;

	wxCheckListBox* m_viewList;

	wxToolBar* m_optionBar;

	wxToolBar* m_exclusiveToggleBar;
	wxToolBarToolBase* m_activeTool;

	wxToolBarToolBase*	m_showNavgraph[ 4 ];

	Int32 m_wxIds;

	wxSpinCtrl* m_brushRadiusSpin;
	wxSlider* m_brushRadiusSlider;

	Int32 m_brushRadius;

	Uint32 m_lastLogVersion;
	Bool m_updateStatus;

	CPathlibListener* m_pathlibListener;
};

class CEdNavmeshEditor
{
protected:
	Bool						m_isNavmeshEditorActive;
	Bool						m_isNavmeshEdgeSelected;
	Bool						m_isNavmeshEdgePhantom;
	EngineTime					m_nextEdgeSelectionTest;
	Uint32						m_selectedNavmeshTriangle;
	Uint32						m_selectedNavmeshTriangleEdge;
	EngineTime					m_updateFrequency;

	void RenderNavmeshEdgeSelection( IViewport* view, CRenderFrame* frame );
	void ProcessNavmeshSelection( const CMousePacket& packet );
	void ClearNavmeshSelection();

	virtual CNavmeshComponent* GetEditedNavmeshComponent() = 0;
	virtual PathLib::CNavmesh* GetEditedNavmesh() = 0;
	virtual PathLib::CNavmesh* FindNavmeshForEdition( const CMousePacket& packet ) = 0;
	virtual CWorld* GetEditedWorld() = 0;
	virtual Bool GetEditedInstanceGuid( CGUID& guid );

public:
	CEdNavmeshEditor( EngineTime updateFrequency );

	void SetNavmeshEditorActive( Bool b )											{ m_isNavmeshEditorActive = b; if ( !b ) { m_isNavmeshEdgeSelected = false; m_nextEdgeSelectionTest = 0.f; } }

	Bool HandleViewportMouseMove( const CMousePacket& packet );
	Bool HandleViewportTrack( const CMousePacket& packet );
	Bool HandleViewportGenerateFragments( IViewport *view, CRenderFrame *frame );
};

class CEdPathlib : public IEditorTool, protected CEdNavmeshEditor
{
	DECLARE_ENGINE_CLASS( CEdPathlib, IEditorTool, 0 );
protected:
	static const Float RECALCULATOR_DELAY_FOR_DOUBLECLICK;

	CEdPathlibPanel*			m_panel;
	THandle< CWorld >			m_world;

	Bool						m_isBrushActive;
	Bool						m_isBrushDrawing;
	Bool						m_brushDrawBlockers;
	Bool						m_isRecalculatorActive;
	Bool						m_recalculatorConfirmRecalculation;
	Bool						m_isPathDebuggerActive;
	Bool						m_isPathDebuggerStartingPointSet;
	Bool						m_isNavmeshDeleterActive;
	PathLib::AreaId				m_selectedNavmeshArea;
	PathLib::AreaId				m_recalculatorAreaSelected;
	EngineTime					m_recalculatorClickTime;
	Uint32						m_brushRadius;
	Vector						m_cursorPosition;
	Vector						m_pathDebuggerStartingPoint;
	PathLib::CSearchData*	m_searchData;

	

	TDynArray< PathLib::AreaId >	m_generationScheduledAreas;
	
	void UpdateCursor( const CMousePacket& packet );

	PathLib::CNavmesh* GetEditedNavmesh() override;
	PathLib::CNavmesh* FindNavmeshForEdition( const CMousePacket& packet ) override;
	CWorld* GetEditedWorld() override;
	CNavmeshComponent* GetEditedNavmeshComponent() override;
	Bool GetEditedInstanceGuid( CGUID& guid ) override;

	void RenderAreaFrame( PathLib::CAreaDescription* area, Color boxColor, CRenderFrame* frame );

	void RenderPathDebugger( IViewport* view, CRenderFrame* frame );
	void RenderBrush( IViewport* view, CRenderFrame* frame );
	void RenderRecalculator( IViewport* view, CRenderFrame* frame );
	void RenderGenerationScheduler( IViewport* view, CRenderFrame* frame );

	void ProcessBrush();
	void ProcessRecalculator( const Vector& cameraPosition, const Vector& dir );
	void ProcessGenerationScheduler( const Vector& cameraPosition, const Vector& dir );

	void DoRecalculate();

public:
	CEdPathlib();
	~CEdPathlib();

	String GetCaption() const override;

	Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection ) override;
	void End() override;

	void SetNavmeshEditorActive( Bool b )								{ CEdNavmeshEditor::SetNavmeshEditorActive( b ); }
	void SetBrushActive( Bool b )										{ m_isBrushActive = b; if ( !b ) m_isBrushDrawing = false; }
	void SetBrushRadius( Uint32 r )										{ m_brushRadius = r; }
	Uint32 GetBrushRadius() const										{ return m_brushRadius; }
	void SetBrushDrawBlockers( Bool b )									{ m_brushDrawBlockers = b; }
	Bool GetBrushDrawBlockers() const									{ return m_brushDrawBlockers; }
	void SetRecalculatorActive( Bool b )								{ m_isRecalculatorActive = b; m_recalculatorAreaSelected = PathLib::INVALID_AREA_ID; m_recalculatorConfirmRecalculation = false; }
	void SetNavmeshDeleterActive( Bool b )								{ m_isNavmeshDeleterActive = b; }

	void SetPathDebuggerMode( Bool b )									{ m_isPathDebuggerActive = b; m_isPathDebuggerStartingPointSet = false; }

	Bool OnViewportTick( IViewport* view, Float timeDelta ) override;
	Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y ) override;
	Bool OnViewportMouseMove( const CMousePacket& packet ) override;
	Bool OnViewportTrack( const CMousePacket& packet ) override;
	Bool OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame ) override;
};


BEGIN_CLASS_RTTI( CEdPathlib );
	PARENT_CLASS( IEditorTool );
END_CLASS_RTTI();
