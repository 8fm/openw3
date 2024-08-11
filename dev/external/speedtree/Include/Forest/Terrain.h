///////////////////////////////////////////////////////////////////////  
//  Terrain.h
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2014 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      http://www.idvinc.com


///////////////////////////////////////////////////////////////////////  
//  Preprocessor

#pragma once
#include "Forest/Forest.h"
#include "Core/ExportBegin.h"


///////////////////////////////////////////////////////////////////////  
//  Packing

#ifdef ST_SETS_PACKING_INTERNALLY
	#pragma pack(push, 4)
#endif


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

	///////////////////////////////////////////////////////////////////////  
	//  Enumeration ETerrainCellTransition

	enum ETerrainCellTransition
	{
		TERRAIN_CELL_TRANSITION_NONE,
		TERRAIN_CELL_TRANSITION_LEFT,
		TERRAIN_CELL_TRANSITION_RIGHT,
		TERRAIN_CELL_TRANSITION_TOP,
		TERRAIN_CELL_TRANSITION_BOTTOM,
		TERRAIN_CELL_TRANSITION_TOP_LEFT,
		TERRAIN_CELL_TRANSITION_TOP_RIGHT,
		TERRAIN_CELL_TRANSITION_BOTTOM_LEFT,
		TERRAIN_CELL_TRANSITION_BOTTOM_RIGHT,
		TERRAIN_CELL_TRANSITION_COUNT
	};


	///////////////////////////////////////////////////////////////////////  
	//  Class CTerrainCell

	class ST_DLL_LINK CTerrainCell
	{
	public:
			friend class CTerrain;


										CTerrainCell( );
			virtual						~CTerrainCell( );

			st_bool						IsNew(void) const;

			// row/col attributes
			st_int32					Col(void) const;
			st_int32					Row(void) const;
			void						SetRowCol(st_int32 nRow, st_int32 nCol);

			// extents
			const CExtents&				GetExtents(void) const;
			void						InvalidateExtents(void);
			void						SetExtents(const CExtents& cExtents);
			st_float32					GetDistanceFromCamera(void) const;

			// culling
			const Vec3&					GetCenter(void) const;
			st_float32					GetCullingRadius(void) const;

			// frame stamp
			st_int32					GetFrameIndex(void) const;
			void						SetFrameIndex(st_int32 nFrameIndex);

			// geometry
			void						GetIndices(st_uint32& uiOffset, st_uint32& uiNumIndices, st_uint32& uiMinIndex, st_uint32& uiNumVertices) const;
			st_int32					GetLod(void) const;

			// graphics-related
			void*						GetVbo(void) const;
			void						SetVbo(void* pVbo);

			// occlusion & visibility
			void						SetIsPopulated(st_bool bVisibile);
			st_bool						IsPopulated(void) const;


	private:
			// grid data
			st_int32					m_nRow;
			st_int32					m_nCol;

			// extents & culling
			CExtents					m_cExtents;
			Vec3						m_vCenter;
			st_float32					m_fCullRadius;
			st_int32					m_nFrameIndex;

			// LOD
			st_uint32					m_uiIndicesOffset;
			st_uint32					m_uiNumIndices;
			st_uint32					m_uiMinIndex;
			st_uint32					m_uiNumVertices;
			st_float32					m_fDistanceFromCamera;
			st_int32					m_nLod;
			void*						m_pVbo;

			// visibility
			st_bool						m_bPopulated;
	};


	///////////////////////////////////////////////////////////////////////  
	//  Class CCellContainer

	template<class TCellType>
	class ST_DLL_LINK CCellContainer : public CMap<SCellKey, TCellType>
	{
	public:
			// internal type definitions
			typedef typename CCellContainer::iterator TCellIterator;
			typedef typename CCellContainer::const_iterator TCellConstIterator;

										CCellContainer( );
			virtual						~CCellContainer( );

			TCellType*					GetCellPtrByRowCol_Add(st_int32 nRow, st_int32 nCol);
			const TCellType*			GetCellPtrByRowCol(st_int32 nRow, st_int32 nCol) const;
			TCellType*					GetCellPtrByPos_Add(const Vec3& vPos);
			const TCellType*			GetCellPtrByPos(const Vec3& vPos) const;

			// internal
			TCellIterator				GetCellItrByRowCol_Add(st_int32 nRow, st_int32 nCol);
			TCellConstIterator			GetCellItrByRowCol(st_int32 nRow, st_int32 nCol) const;
			TCellIterator				GetCellItrByPos_Add(const Vec3& vPos);
			TCellConstIterator			GetCellItrByPos(const Vec3& vPos) const;
			TCellIterator				Erase(TCellIterator iCell);

			// cell size
			st_float32					GetCellSize(void) const;
			void						SetCellSize(st_float32 fCellSize);

	private:
			st_float32					m_fCellSize;
	};


	///////////////////////////////////////////////////////////////////////  
	//  Type definitions

	typedef CArray<CTerrainCell*>			TTerrainCellArray;
	typedef CCellContainer<CTerrainCell>	TTerrainCellMap;
	typedef CArray<void*>					TTerrainVboArray;


	///////////////////////////////////////////////////////////////////////  
	//  Structure STerrainCullResults

	struct ST_DLL_LINK STerrainCullResults
	{
			TTerrainCellArray			m_aCellsToUpdate;
			TTerrainVboArray			m_aFreedVbos;
			TTerrainCellArray			m_aVisibleCells;

			st_bool						SetHeapReserves(const SHeapReserves& sHeapReserves);
	};


	///////////////////////////////////////////////////////////////////////  
	//  Class CTerrain

	class ST_DLL_LINK CTerrain
	{
	public:
										CTerrain( );
	virtual								~CTerrain( );

	virtual	st_bool						InitGeometry(st_int32 nNumLods, st_int32 nMaxTileRes, st_float32 fCellSize);
			st_bool						IsEnabled(void) const;
			st_int32					GetNumLods(void) const;
			st_int32					GetMaxTileRes(void) const;
			st_float32					GetCellSize(void) const;
			void						SetHeightHints(st_float32 fGlobalLowPoint, st_float32 fGlobalHighPoint);
			void						SetHeapReserves(const SHeapReserves& sHeapReserves);

			// LOD
			void						SetLodRange(st_float32 fNear, st_float32 fFar);
			void						GetLodRange(st_float32& fNear, st_float32& fFar) const;
			const CArray<st_uint16>&	GetCompositeIndices(void) const;

	virtual	void						CullAndComputeLOD(const CView& cView, st_int32 nFrameIndex, STerrainCullResults& sCullResults);
			void						FrameEnd(st_int32 nFrameIndex, STerrainCullResults& sCullResults);

	protected:
			void						ComputeCellLods(const CView& cView, STerrainCullResults& sResults) const;
			void						RemoveInactiveCells(TTerrainVboArray& aFreedVbos, st_int32 nFrameIndex);
			void						InitLodIndexStrips(void);

			// LOD
			st_int32					m_nNumLods;					// the number of discrete LOD stages
			st_int32					m_nMaxTileRes;				// the highest LOD terrain tile will be a grid mesh of m_nMaxTileRes X m_nMaxTileRes
			st_float32					m_fNearLodDistance;			// at this distance, the highest LOD cell is used
			st_float32					m_fFarLodDistance;			// at this distance, the lowest LOD cell is used

			// cell data
			TTerrainCellMap				m_cTerrainCellMap;			// maps the (row,col) search key to a terrain cell
			st_float32					m_fGlobalLowPoint;			//
			st_float32					m_fGlobalHighPoint;			//

			// indices used to control LOD of a given terrain tile
			CArray<st_uint16>			m_aMasterIndices;			// contains all of the strips for all LOD/edge combinations

			// memory utility
			SHeapReserves				m_sHeapReserves;

			struct SIndexData
			{
				st_uint32				m_uiNumIndices;				// 3 = one triangle, 6 = two, etc...
				st_uint32				m_uiOffset;
				st_uint16				m_uiMinIndex;
				st_uint16				m_uiNumVertices;
			};
			CArray<SIndexData>			m_aPerCellIndexData;
	};

	// include inline functions
	#include "Terrain_inl.h"
	#include "CellContainer_inl.h"

} // end namespace SpeedTree

#ifdef ST_SETS_PACKING_INTERNALLY
	#pragma pack(pop)
#endif

#include "Core/ExportEnd.h"

