/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifdef USE_SPEED_TREE

#include <Core/Core.h>

// Max. 64k instances of each type per cell tracked
class CSpeedTreeDensityTrackingData
{
public:
	CSpeedTreeDensityTrackingData();
	~CSpeedTreeDensityTrackingData();

	void IncrementGrassInstanceCount( Uint16 instanceCount = 1 );
	void IncrementTreeInstanceCount( Uint16 instanceCount = 1 );
	void IncrementGrassLayerCount( Uint16 layerCount = 1 );

	Uint16 GetGrassInstanceCount() const;
	Uint16 GetTreeInstanceCount() const;
	Uint16 GetGrassLayerCount() const;

private:
	Uint16 m_grassInstances;
	Uint16 m_treeInstances;
	Uint16 m_grassLayerCount;
};

// This keeps a bitmap of the entire world, and tracks speedtree usage 
// The bitmap is copied to a texture to be used in debug visualisation
class CSpeedTreeDensityDebugger
{
public:
	enum FoliageVisualisationMode
	{
		EMODE_None,
		EMODE_GrassInstances,
		EMODE_GrassLayers,
		EMODE_TreeInstances
	};

	CSpeedTreeDensityDebugger();
	~CSpeedTreeDensityDebugger();

	void Initialise( Float worldExtents, Float cellExtents, Uint32 debugTextureResolution );
	void AddGrassInstance( SpeedTree::Vec3 position );
	void AddTreeInstance( SpeedTree::Vec3 position );
	void AddGrassLayer( SpeedTree::Vec3 extents, Int32 layerId );
	void ResetAll();
	void UpdateTexture( const Vector& cameraPosition, const Vector& cameraForward );

	GpuApi::TextureRef	GetGrassDensityTexture() const { return m_renderTexture; }
	Float				GetCellExtents() const { return m_cellExtents; }
	Vector				GetWorldSpaceRect() const { return m_currentArea; }

	// Layer toucher used to ensure layers are only added to cells once
	void ClearLayerToucher();			
	void TouchLayer( SpeedTree::Vec3 position );

	// Customisation (budget, visualisation)
	void SetMode( FoliageVisualisationMode m );
	void SetGrassDensityBudget( Float instancesPerSqMeter );
	void SetTreeDensityBudget( Float instancesPerSqMeter );
	void SetGrassLayerDensityBudget( Float layersPerSqMeter );
	RED_INLINE FoliageVisualisationMode GetVisualisationMode() const { return m_visualisation; }

private:

	void CreateRenderResources();
	Int32 CalculateCellIndex( SpeedTree::Vec3 position );
	Int32 CalculateCellIndex( Int32 x, Int32 y );
	Uint32 MakeRGBA( Uint8 r, Uint8 g, Uint8 b, Uint8 a ) const;

	// To add a new visualisation, specialise the functions below
	template< FoliageVisualisationMode >
	Uint32 CalculateColour( const CSpeedTreeDensityTrackingData& data ) const;

	template< FoliageVisualisationMode >
	void RedrawBitmap( Uint32* lockedData, Uint32 texturePitch, Int32 cameraOffsetX, Int32 cameraOffsetY );

	typedef THashMap< Int32, CSpeedTreeDensityTrackingData, DefaultHashFunc< Int32 >, DefaultEqualFunc< Int32 >, MC_SpeedTree > DensityDataMap;
	typedef THashMap< Int32, Bool > LayerTouchMap, MC_SpeedTree;
	Float m_cellExtents;										// Resolution of the data in world-space 
	Float m_worldExtents;										// Extents of the world
	Int32 m_cellsPerSide;										// Cells per world axis
	Uint32 m_totalCells;										// Num. cells allocated
	DensityDataMap m_data;										// Map of hashed cell position -> tracking data
	LayerTouchMap m_layerMap;									// Map of bools to see if we have incremented layer count for a cell
	Int32 m_debugTextureResolution;								// Texture resolution (one side)
	GpuApi::TextureRef m_renderTexture;							// Texture used to visualise the data.
	FoliageVisualisationMode m_visualisation;					// What type of visualiser
	Vector m_currentArea;										// Currently covered worldspace rectangle

	// Params for visualisation
	Float m_grassDensityPerCellBudget;							// How many grass instances / cell
	Float m_treeDensityPerCellBudget;							// How many tree instances / cell
	Float m_grassLayerDensityPerCellBudget;						// How many grass layers / cell
};

#include "speedTreeDensityDebugger.inl"

#endif