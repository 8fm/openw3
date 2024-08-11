/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdMergedGeometryToolPanel : public wxPanel
{
	DECLARE_EVENT_TABLE();

public:
	CEdMergedGeometryToolPanel( wxWindow* parent, CWorld* world );
	~CEdMergedGeometryToolPanel();

	void RefreshStatsForCurrentCamera( const Vector& cameraPosition, const Bool force = false );

private:
	CWorld*			m_world;
	wxStaticText*	m_stats;

	void OnRebuildAllStaticGeometry( wxCommandEvent& event );
	void OnRebuildNearStaticGeometry( wxCommandEvent& event );
	void OnCheckinChanges( wxCommandEvent& event );

	struct GridStats
	{
		Uint32		m_dataSize;
		Uint32		m_numTriangles;
		Uint32		m_numVertices;
		Uint32		m_numCells;

		RED_FORCE_INLINE GridStats()
		{
			m_dataSize = 0;
			m_numTriangles = 0;
			m_numVertices = 0;
			m_numCells = 0;
		}
	};

	struct GridCell
	{
		Vector		m_pos;
		Float		m_streamingDistanceSq;
		Int32		m_x;
		Int32		m_y;
		Uint32		m_dataSize;
		Uint32		m_numTriangles;
		Uint32		m_numVertices;

		RED_FORCE_INLINE GridCell()
		{
			m_x = -1;
			m_y = -1;
			m_streamingDistanceSq = 0;
			m_dataSize = 0;
			m_numVertices = 0;
			m_numTriangles = 0;
		}
	};

	Int32								m_gridSize;
	TDynArray< GridCell >				m_gridCells;
	TDynArray< Int32 >					m_gridMap; // m_gridSize*m_gridSize

	Uint64					m_totalSize;
	Uint32					m_totalTriangles;
	Uint32					m_totalVertices;

	GridStats				m_worstPlaceInfo;
	GridStats				m_currentPlaceInfo;
	Vector					m_currentPlaceInfoRefPos;
	Bool					m_currentPlaceInfoValid;

	void ComputeStatsForPlace( const Vector& position, GridStats& outResults ) const;

	void RefreshCells();
	void RefreshWorstPlace();
	void RefreshStats();
};

class CEdMergedGeometryTool : public IEditorTool
{
	DECLARE_ENGINE_CLASS( CEdMergedGeometryTool, IEditorTool, 0 );

public:
	CEdMergedGeometryTool();
	virtual ~CEdMergedGeometryTool();

	virtual String GetCaption() const;

	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* m_panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection );
	virtual void End();

	virtual Bool UsableInActiveWorldOnly() const { return true; }

	virtual Bool OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera );

private:
	CEdMergedGeometryToolPanel*		m_panel;
};


BEGIN_CLASS_RTTI( CEdMergedGeometryTool );
	PARENT_CLASS( IEditorTool );
END_CLASS_RTTI();
