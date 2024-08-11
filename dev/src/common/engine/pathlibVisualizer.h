#pragma once

#include "../core/engineTime.h"

#include "pathlib.h"
#include "renderVertices.h"

class CPathLibWorld;
class IRenderResource;
class CRenderFrame;

namespace PathLib
{

class CNavGraph;
class CNavmesh;
class CPathLink;
class CPathNode;
class CTerrainMap;
class CAreaDescription;


class CNavmeshRenderer
{
protected:
	PathLib::AreaId										m_id;
	Bool												m_isPrepeared;
	IRenderResource*									m_navmeshPreview;
	IRenderResource*									m_navmeshPreviewWireframe;
	TDynArray< DebugVertex >							m_borders;
	Matrix												m_transformation;

	CNavmesh* GetNavmesh() const;
public:
	CNavmeshRenderer( PathLib::AreaId id = PathLib::INVALID_AREA_ID );
	~CNavmeshRenderer();

	Bool PrepeareRender();
	void GenerateEditorFragments( CRenderFrame* frame );

	void InvalidatePreview();
	Bool UpdateTransformation( Matrix& m )									{ m_transformation = m; return false; }
};

class CVisualizer
{
protected:
	CPathLibWorld*				m_pathlib;
	////////////////////////////////////////////////////////////////////////
	// Navmeshes
	TArrayMap< AreaId, CNavmeshRenderer* >		m_navmeshRenderers;
	TDynArray< AreaId >							m_renderedNavmeshes;
	////////////////////////////////////////////////////////////////////////
	// Navgraphs
	Uint32						m_debugNavgraph;
	Bool						m_colorNavgraphsByRegions;
	struct CNavgraphInfo
	{
		CNavgraphInfo()	
			: m_obstacles( NULL )											{ Clear(); }
		~CNavgraphInfo()													{ Clear(); }
		void Clear();

		AreaId						m_areaId;
		Uint32						m_lastGraphVersion;
		TDynArray< DebugVertex >	m_links;
		EngineTime					m_lastUsage;
		Uint32						m_lastObstaclesVersion;
		IRenderResource*			m_obstacles;
		//TDynArray< Uint8 >		m_colors;
		//Bool						m_useColors;
	};
	TStaticArray< CNavgraphInfo, 8 > m_navgraphs;
	////////////////////////////////////////////////////////////////////////
	TSortedArray< Uint32 >			m_debuggedTiles;
	////////////////////////////////////////////////////////////////////////
	AreaId							m_terrainHeightDebugArea;
	IRenderResource*				m_terrainHeightMesh;

	void DrawNavgraph( CRenderFrame* frame, CNavgraphInfo* data );
	void DrawObstacles( CRenderFrame* frame, CNavgraphInfo* data );
	void DrawTerrainHeight( CRenderFrame* frame );

	CNavgraphInfo* GetNavgraph( AreaId areaId, Bool precomputeNavgraph, Bool precomputeOccluders );
	void ProcessNavgraph( CNavgraphInfo* data, CNavGraph* graph );
	void ProcessObstacles( CNavgraphInfo* data, CAreaDescription* graph );
	void ProcessLink( CNavgraphInfo* data, CNavGraph* graph, const CPathNode* node1, const CPathNode* node2, const CPathLink* link );

	void ProcessTerrainTile( const CTerrainMap& terrainMap, CTerrainTile* terrainTile );

public:
	CVisualizer( CPathLibWorld* pathlib );
	~CVisualizer();

	void GenerateEditorFragments( CRenderFrame* frame );

	void CollectAndDrawNavgraphs( CRenderFrame* frame, Bool navgraphs, Bool obstacles );
	void CollectAndDrawObstacles( CRenderFrame* frame );
	void CollectAndDrawNavmeshes( CRenderFrame* frame );

	void InstanceUpdated( AreaId areaId );
	void InstanceRemoved( AreaId areaId );
	
	void DebugNavgraph( Uint32 i );
	Uint32 GetDebuggedNavgraph() const									{ return m_debugNavgraph; }
	const TDynArray< AreaId >& GetNavmeshesBeingRendered() const		{ return m_renderedNavmeshes; }
	void ClearNavgraphs();

	CPathLibWorld* GetPathlib() const									{ return m_pathlib; }
	CNavmeshRenderer* GetNavmeshRender( AreaId areaId );
};


};			// namespace PathLib