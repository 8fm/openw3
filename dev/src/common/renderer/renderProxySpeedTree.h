/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderCollector.h" //for the shadow cascades

#include "renderFramePrefetch.h"
#include "speedTreeRenderInterface.h"
#include "../engine/foliageRenderSettings.h"
#include "../engine/foliageInstance.h"
#include "../core/uniquePtr.h"

#ifdef USE_SPEED_TREE

#define MAX_DYNAMIC_FOLIAGE_CASCADES 2

class CRenderSpeedTreeResource;
class CRenderProxy_Terrain;
class CSpeedTreeDensityDebugger;

// when changed change also in dynamicCollisionsCollector.h
//#define DYNAMIC_COLLIDERS_MAX_SIZE 16

//#define PARALLELIZE_TREES_FADING

//////////////////////////////////////////////////////////////////////////
// Constant buffer structure for grass features (pigment map and collision ATM)
//////////////////////////////////////////////////////////////////////////

struct SGrassConstantBuffer;


typedef TDynArray< SpeedTree::CTree*, MC_SpeedTreeContainer > TreeContainer;
typedef TDynArray< const SpeedTree::CTree*, MC_SpeedTreeContainer > ConstTreeContainer;
typedef TDynArray< SpeedTree::CTreeRender*, MC_SpeedTreeContainer > TreeRenderContainer;
typedef TDynArray< const SpeedTree::CTreeRender*, MC_SpeedTreeContainer > ConstTreeRenderContainer;

#ifdef USE_ANSEL
extern Bool isAnselSessionActive;
#endif // USE_ANSEL

template < class CELL, class INST >
class TSpeedTreeGrid
{
public:
	TSpeedTreeGrid()
		: m_cellSize( 0.0f )
		, m_numCols( 0 )
		, m_numRows( 0 )
		, m_firstCol( 0 )
		, m_firstRow( 0 )
		, m_lastCol( 0 )
		, m_lastRow( 0 )
	{}

	TSpeedTreeGrid( Float cellSize )
		: m_cellSize( cellSize )
		, m_numCols( 0 )
		, m_numRows( 0 )
		, m_firstCol( 0 )
		, m_firstRow( 0 )
		, m_lastCol( -1 )
		, m_lastRow( -1 )
	{}

	~TSpeedTreeGrid()
	{
		for (auto iter=m_perCellData.Begin(); iter!=m_perCellData.End(); ++iter)
		{
			CELL* cell = (*iter).m_second;
			delete cell;
		}
	}

	//! Initialize with specific cell size
	void Init( Float cellSize );

	//! Get cell by key
	CELL* GetCellByKey( const SpeedTree::SCellKey& key );
	const CELL* GetCellByKey( const SpeedTree::SCellKey& key ) const;

	//! Get or create a cell by key
	CELL* GetOrCreateCellByKey( const SpeedTree::SCellKey& key );

	//! Add instances, providing the sum of their extents
	void AddInstances( const TDynArray<INST, MC_SpeedTreeContainer>& instances, const SpeedTree::CExtents& sumExtents, const SpeedTree::CExtents& baseExtents, TDynArray< SpeedTree::SCellKey >& outUpdatedCells );

	//! Add instances, providing the sum of their extents - - doesn't return rows and cols changed
	void AddInstances( const TDynArray<INST, MC_SpeedTreeContainer>& instances, const SpeedTree::CExtents& sumExtents, const SpeedTree::CExtents& baseExtents );

	//! Remove instances on the circular area
	Uint32 RemoveInstances( const SpeedTree::CTreeRender* baseTree, const Vector& center, Float radius, Int32 outRowCol[4] );

	//! Remove instances on the circular area - doesn't return rows and cols changed
	Uint32 RemoveInstances( const SpeedTree::CTreeRender* baseTree, const Vector& center, Float radius );

	//! Remove instances on the rectangular area
	Uint32 RemoveInstances( const SpeedTree::CTreeRender* baseTree, const Box& rect, Int32 outRowCol[4] );

	//! Remove instances on the rectangular area - doesn't return rows and cols changed
	Uint32 RemoveInstances( const SpeedTree::CTreeRender* baseTree, const Box& rect );

	//! Getters
	Float GetCellSize() const { return m_cellSize; };

private:
	void EraseCellByKey( const SpeedTree::SCellKey& key );

	typedef TSortedMap< SpeedTree::SRowCol, CELL*, DefaultCompareFunc< SpeedTree::SRowCol >, MC_SpeedTreeContainer > CellContainer;

	CellContainer				m_perCellData;	//!< Cells of data
	SpeedTree::CExtents			m_extents;		//!< Extents of the whole gride content
	Int32						m_firstCol;		//!< Global index of the first cell columnt
	Int32						m_lastCol;		//!< Global index of the last cell columnt
	Int32						m_numCols;		//!< Number of columns
	Int32						m_firstRow;		//!< Global index of the first cell row
	Int32						m_lastRow;		//!< Global index of the last cell row
	Int32						m_numRows;		//!< Number of rows
	Float						m_cellSize;		//!< Width of the cell
};

class CGrassLayer
{
public:
	typedef TDynArray< SpeedTree::SGrassInstance, MC_SpeedTreeContainer > GrassInstances;

	struct SPerCellData
	{
		DECLARE_CLASS_MEMORY_POOL_ALIGNED( MemoryPool_SmallObjects, MC_SpeedTreeGridCell, __alignof( SPerCellData ) )
	public:

		SpeedTree::CExtents	 m_extents;
		GrassInstances m_outGrassInstances;

		SPerCellData() {}

		SPerCellData( SPerCellData&& other )
			:	m_extents( std::move( other.m_extents ) ),
				m_outGrassInstances( std::move( other.m_outGrassInstances ) )
		{}

		Bool HasGrassInstances() const 
		{
			return !m_outGrassInstances.Empty();
		}

		//! Add instance 
		void AddInstance( const SpeedTree::SGrassInstance& grassInstance );
		void AddInstances( const GrassInstances& instances, const SpeedTree::CExtents& extents );

		//! Remove instances in given area, recompiling extents at the same time
		template< typename TESTFUNC >
		Uint32 RemoveInstances( const SpeedTree::CTreeRender* baseTree, const TESTFUNC& testFunctor );

		//! Recompile cell data
		void Recompile( const SpeedTree::CExtents* baseExtents = nullptr );

		void GetUsedBaseTrees( TreeContainer & baseTrees ) {} //Not necessary for grass cells

		Bool IsEmpty() const { return !HasGrassInstances(); }

		SPerCellData& operator=( SPerCellData&& other )
		{
			SPerCellData( std::move( other ) ).Swap( *this );
			return *this;
		}

		void Swap( SPerCellData & other )
		{
			SpeedTree::CExtents temp = m_extents;
			m_extents = other.m_extents;
			other.m_extents = temp;
			m_outGrassInstances.SwapWith( other.m_outGrassInstances );
		}
	};	

private:
	TSpeedTreeGrid< SPerCellData, SpeedTree::SGrassInstance >		m_cellData;

	SpeedTree::CVisibleInstancesRender								m_visibleGrass;
	SpeedTree::CView												m_currentView;
	CRenderSpeedTreeResource*										m_renderTree;

	Float															m_grassCullRadius;	//!< Used as maximum overhang of the cell
	Uint32															m_numInstances;
	Int32															m_numMaxInstances;
	Bool															m_isAutopopulated;
	Bool															m_grassPopulationUpdateScheduled;
	Bool															m_needInstanceBufferUpdate;

	// We keep an array of instances around which we use to populate generated grass cells without having to constantly reallocate memory
	GrassInstances													m_grassGenerationInstanceBuffer;

public:
	CGrassLayer();
	~CGrassLayer();

	// Release speed tree stuff
	void										ReleaseGfxResources();

	//! Setters, getters
	void										SetBaseGrass( CRenderSpeedTreeResource* renderTree );
	CRenderSpeedTreeResource*					GetBaseGrass() const { return m_renderTree; }
	SpeedTree::CVisibleInstancesRender&			GetVisible() { return m_visibleGrass; }
	SpeedTree::CView&							GetView() { return m_currentView; }
	void										SetAutoPopulatedFlag( Bool flag ) { m_isAutopopulated = flag; }
	Bool										IsAutopopulated() const { return m_isAutopopulated; }
	void										SetNeedUpdateInstanceBuffer(Bool flag) { m_needInstanceBufferUpdate = flag; }
	Bool										NeedUpdateInstanceBuffer() { return m_needInstanceBufferUpdate; }
	Int32										GetNumMaxInstances() const { return m_numMaxInstances; }
	void										SetNumMaxInstances(Int32 val) { m_numMaxInstances = val; }

	//! Add instances
	void										AddInstances( const GrassInstances& instances, const SpeedTree::CExtents& extents );

	//! Remove instances on the certain area
	void										RemoveInstances( const Vector& center, Float radius );
	void										RemoveInstances( const Box& rect );

	//! Get cell from the grid
	SPerCellData*								GetCellByKey( const SpeedTree::SCellKey& key );

	Float										GetGrassCullRadius() const { return m_grassCullRadius; }
	Uint32										GetNumInstances() const { return m_numInstances; }
	Float										GetCellSize() const { return m_cellData.GetCellSize(); }
	GrassInstances	& GetGrassGenerationInstanceBuffer() { return m_grassGenerationInstanceBuffer; }

	void										ScheduleGrassPopulationUpdate() { m_grassPopulationUpdateScheduled = true; }
	Bool										IsGrassPopulationUpdateScheduled() const { return m_grassPopulationUpdateScheduled; }
	void										UpdatePopulation();
};

//////////////////////////////////////////////////////////////////////////
// Visibility helper for umbra and speedtree integration
//////////////////////////////////////////////////////////////////////////
class RedVisibilityHelper : public SpeedTree::VisibilityHelper
{
public:
#ifdef USE_UMBRA
	const CRenderOcclusionData* m_occlusionData;
#endif // USE_UMBRA
	CRenderFrame*				m_frame;
	CFrustum					m_frustum;
	Bool						m_showBBoxes;

	RedVisibilityHelper()
		: SpeedTree::VisibilityHelper( false, true, SpeedTree::Vec3 (0,0,0), 1.f, 0.3f, 0.f )
#ifdef USE_UMBRA
		, m_occlusionData( nullptr )
#endif // USE_UMBRA
		, m_frame( nullptr )
		, m_showBBoxes( nullptr )
	{}

	RedVisibilityHelper( 
#ifdef USE_UMBRA
		const CRenderOcclusionData* occlusionData, 
#endif // USE_UMBRA
		CRenderFrame* frame, Bool showBBoxes ) 
		: SpeedTree::VisibilityHelper( false, true, SpeedTree::Vec3 (0,0,0), 1.f, 0.3f, 0.f )
#ifdef USE_UMBRA
		, m_occlusionData( occlusionData )
#endif // USE_UMBRA
		, m_frame( frame )
		, m_showBBoxes( showBBoxes )
	{
		if ( m_frame )
		{
			m_frustum.InitFromCamera( m_frame->GetFrameInfo().m_occlusionCamera.GetWorldToScreen() );
		}
	}

	virtual Bool IsTreeVisible( const SpeedTree::Vec3& center, const SpeedTree::st_float32 radius ) const override
	{
#ifdef USE_ANSEL
		if ( isAnselSessionActive )
		{
			return true;
		}
#endif // USE_ANSEL
		Bool visible = false;
		const Box b( Vector( center.x, center.y, center.z ), radius );
#ifdef USE_UMBRA
		if ( !m_occlusionData )
		{
			// better show too much than crash
			visible = m_frustum.TestBox( b ) != FR_Outside;
		}
		else
		{
			const Box b( Vector( center.x, center.y, center.z ), radius );
			visible = m_occlusionData->IsDynamicObjectVisible( b );
		}		
#else // USE_UMBRA
		Bool visible = m_frustum.TestBox( b ) != FR_Outside;
#endif // USE_UMBRA

#ifndef RED_FINAL_BUILD
		if ( m_showBBoxes )
		{
			Color c = visible ? Color::LIGHT_GREEN : Color::LIGHT_RED;
			c.A = 10;
			m_frame->AddDebugSolidBox( b, Matrix::IDENTITY, c );
		}
#endif // RED_FINAL_BUILD

		return visible;
	}

	virtual VisibilityTestResult IsVisible( const SpeedTree::CExtents& extents ) const override
	{
#ifdef USE_ANSEL
		if ( isAnselSessionActive )
		{
			return VisibilityHelper::VISIBLE;
		}
#endif // USE_ANSEL
		VisibilityTestResult visible = VisibilityHelper::OCCLUDED;
		const Box b( Vector( extents.Min().x, extents.Min().y, extents.Min().z ), Vector( extents.Max().x, extents.Max().y, extents.Max().z ) );
#ifdef USE_UMBRA
		if ( !m_occlusionData )
		{
			visible = m_frustum.TestBox( b ) != FR_Outside ? VisibilityHelper::VISIBLE : VisibilityHelper::OCCLUDED;	
		}
		else 
		{
			visible = (VisibilityTestResult)m_occlusionData->IsDynamicObjectVisibleFullTest( b );
		}
#else // USE_UMBRA
		visible = m_frustum.TestBox( b ) != FR_Outside ? VisibilityHelper::VISIBLE : VisibilityHelper::OCCLUDED;
#endif // USE_UMBRA

#ifndef RED_FINAL_BUILD
		if ( m_showBBoxes )
		{
			Color c = Color::LIGHT_GREEN;
			switch(visible)
			{
			case VisibilityHelper::OCCLUDED:
				c = Color::LIGHT_RED;
				break;
			case VisibilityHelper::VISIBLE:
				c = Color::LIGHT_YELLOW;
				break;
			}
			c.A = 10;
			m_frame->AddDebugSolidBox( b, Matrix::IDENTITY, c );
		}
#endif // RED_FINAL_BUILD

		return visible;
	}		
};

class RedShadowVisibilityHelper : public SpeedTree::VisibilityHelper
{
public:
#ifdef USE_UMBRA
	const CRenderOcclusionData*		m_occlusionData;
#endif // USE_UMBRA
	CFrustum						m_frustum;
	Int32							m_cascadeNum;		
	
	RedShadowVisibilityHelper( 
#ifdef USE_UMBRA
		const CRenderOcclusionData* occlusionData,
#endif // USE_UMBRA
		const CFrustum& frustum, Int32 cascadeNum, SpeedTree::Vec3	referenceCamera, SpeedTree::st_float32 foliageShadowDistanceScale, SpeedTree::st_float32 foliageShadowDistanceBillboardScale, SpeedTree::st_float32 foliageShadowFadeRange ) 
		: SpeedTree::VisibilityHelper( true, false, referenceCamera, foliageShadowDistanceScale, foliageShadowDistanceBillboardScale, foliageShadowFadeRange )
#ifdef USE_UMBRA
		, m_occlusionData( occlusionData )
#endif // USE_UMBRA
		, m_frustum( frustum )
		, m_cascadeNum( cascadeNum )		
	{	
	}

	virtual Bool IsTreeVisible( const SpeedTree::Vec3& center, const SpeedTree::st_float32 radius )	const override
	{
		RED_FATAL( "This shouldn't be called" );
		return true;
	}
	
	virtual VisibilityTestResult IsVisible( const SpeedTree::CExtents& extents ) const override
	{
#ifdef USE_ANSEL
		if ( isAnselSessionActive )
		{
			return VisibilityTestResult::VISIBLE;
		}
#endif // USE_ANSEL
		Bool visible = false;
		Box b( Vector( extents.Min().x, extents.Min().y, extents.Min().z ), Vector( extents.Max().x, extents.Max().y, extents.Max().z ) );

#ifdef USE_UMBRA
		if ( !m_occlusionData )
		{
			visible = m_frustum.TestBox( b ) != FR_Outside;
		}
		else
		{
			visible = m_occlusionData->IsDynamicObjectShadowVisibleInCascade( b, m_cascadeNum );
		}		
		return visible ? VisibilityTestResult::VISIBLE : VisibilityTestResult::OCCLUDED;
#else
		Box b( Vector( extents.Min().x, extents.Min().y, extents.Min().z ), Vector( extents.Max().x, extents.Max().y, extents.Max().z ) );
		return m_frustum.TestBox( b ) != FR_Outside ? VisibilityTestResult::VISIBLE : VisibilityTestResult::OCCLUDED;
#endif // USE_UMBRA
	}
};

class CRenderProxy_SpeedTree;

class CUpdateTreeInstancesProcessing : public CTask
{
public:
	CUpdateTreeInstancesProcessing(CRenderProxy_SpeedTree* renderProxy, SFoliageUpdateRequest updates);
	virtual ~CUpdateTreeInstancesProcessing() override;

	void FinalizeTreeInstanceUpdates();
	virtual void Run() override final;

private:
#ifndef NO_DEBUG_PAGES
	virtual const Char* GetDebugName() const override;
	virtual Uint32 GetDebugColor() const override;
#endif // NO_DEBUG_PAGES

	CRenderProxy_SpeedTree*			m_renderProxy;
	SFoliageUpdateRequest			m_updates;
};

class CTreeInstanceProcessing : public CTask
{
public:
	CTreeInstanceProcessing(CRenderProxy_SpeedTree* renderProxy, Uint32 foliageType, const TreeRenderContainer& trees);
	virtual ~CTreeInstanceProcessing() override;

	void FinalizeTreeInstanceProcessing();
	TreeRenderContainer& GetTrees();
	virtual void Run() override final;

	SpeedTree::st_int32							m_nBillboardBlockHandle;	//<! Handle to Temporary Billboard Instance Buffer
	SpeedTree::SBillboardInstanceVertex*		m_pBillboardBufferBase;		//<! Pointer to Temporary Billboard Instance Buffer

	SpeedTree::st_int32							m_nUpdatedBillboardBlockHandle;	//<! Handle to Temporary Billboard Instance Buffer
	SpeedTree::SBillboardInstanceVertex*		m_pUpdatedBillboardBufferBase;		//<! Pointer to Temporary Billboard Instance Buffer

private:
#ifndef NO_DEBUG_PAGES
	virtual const Char* GetDebugName() const override;
	virtual Uint32 GetDebugColor() const override;
#endif // NO_DEBUG_PAGES

	CRenderProxy_SpeedTree*						m_renderProxy;
	Uint32										m_foliageType;
	TreeRenderContainer							m_trees;
};

class CGrassInstanceProcessing : public CTask
{
public:
	CGrassInstanceProcessing(CRenderProxy_SpeedTree* renderProxy);
	virtual ~CGrassInstanceProcessing() override;

	void FinalizeGrassInstanceUpdates();
	virtual void Run() override final;

private:
#ifndef NO_DEBUG_PAGES
	virtual const Char* GetDebugName() const override;
	virtual Uint32 GetDebugColor() const override;
#endif // NO_DEBUG_PAGES

	CRenderProxy_SpeedTree*						m_renderProxy;
};


/// Speed Tree proxy
class CRenderProxy_SpeedTree : public IRenderObject, public IRenderPrefetchable
{
	DECLARE_RENDER_OBJECT_MEMORYCLASS( MC_RenderObjectSpeedtree )
public:

	typedef TDynArray< const SpeedTree::CTreeInstance*, MC_SpeedTreeContainer >		CompiledInstanceContainer;
	typedef TDynArray< SpeedTree::CTreeInstance, MC_SpeedTreeContainer >			InstanceContainer;

	struct SPerCellData
	{
		DECLARE_CLASS_MEMORY_POOL_ALIGNED( MemoryPool_SmallObjects, MC_SpeedTreeGridCell, __alignof( SPerCellData ) )
	public:

		typedef TDynArray< InstanceContainer, MC_SpeedTreeContainer >			InstancePerTreeContainer;
		typedef TDynArray< CRenderSpeedTreeResource*, MC_SpeedTreeContainer >	BaseTreeResourceContainer;

		SpeedTree::CExtents			m_extents;					//!< Extents of the content in this cell
		InstancePerTreeContainer	m_instancesPerTree;			//!< Instances arrays per tree in m_baseTreesRes
		BaseTreeResourceContainer	m_baseTreesRes;				//!< Base trees
		TreeContainer				m_baseTrees;				//!< "Compiled" base trees buffer (abc)
		CompiledInstanceContainer	m_instancesOut;				//!< "Compiled" instances buffer (aaaaaabbbbbbbbbcccccccccc)

		Float m_maxFade;					//!< Maximum fade value for trees in this cell
		Float m_minFade;					//!< Minimum fade value for trees in this cell

		SPerCellData()
			: m_maxFade( 1.0f )
			, m_minFade( 0.0f )
		{}

		SPerCellData( SPerCellData&& other )
			:	m_extents( std::move( other.m_extents ) ),
				m_instancesPerTree( std::move( other.m_instancesPerTree ) ),
				m_baseTreesRes( std::move( other.m_baseTreesRes ) ),
				m_baseTrees( std::move( other.m_baseTrees ) ),
				m_instancesOut( std::move( other.m_instancesOut ) ),
				m_maxFade( std::move( other.m_maxFade ) ),
				m_minFade( std::move( other.m_minFade ) )
		{}

		//! Add instance 
		void AddInstance( const SpeedTree::CTreeInstance& treeInstance );
		void AddInstances( const InstanceContainer & instances, const SpeedTree::CExtents& extents );

		//! Remove instances in given area, recompiling extents at the same time
		template< typename TESTFUNC >
		Uint32 RemoveInstances( const SpeedTree::CTreeRender* baseTree, const TESTFUNC& testFunctor );

		//! Replace tree object with a new one, and rebuild it's instances
		//void ReplaceTree( SpeedTree::CTreeRender* prevTree, SpeedTree::CTreeRender* newTree );

		//! Recompile cell data
		void Recompile( const SpeedTree::CExtents* baseExtents = nullptr );

		Bool IsEmpty() const { return m_instancesPerTree.Empty(); }

		void GetUsedBaseTrees( TreeContainer & baseTrees );
		void GetUsedBaseTrees( ConstTreeContainer& baseTrees ) const;
		//use this lukasz
		void GetUsedBaseTrees( TreeRenderContainer & baseTrees );
		void GetUsedBaseTrees( ConstTreeRenderContainer& baseTrees ) const;

		SPerCellData& operator=( SPerCellData&& other )
		{
			SPerCellData( std::move( other ) ).Swap( *this );
			return *this;
		}

		void Swap( SPerCellData & swapWith )
		{
			SpeedTree::CExtents temp = m_extents;
			m_extents = swapWith.m_extents;
			swapWith.m_extents = temp;

			m_instancesPerTree.SwapWith( swapWith.m_instancesPerTree );			
			m_baseTreesRes.SwapWith( swapWith.m_baseTreesRes );				
			m_baseTrees.SwapWith( swapWith.m_baseTrees );				
			m_instancesOut.SwapWith( swapWith.m_instancesOut );				

			::Swap( m_minFade, swapWith.m_minFade );
			::Swap( m_maxFade, swapWith.m_maxFade );
		}
	};

private:
#define STATIC_FOLIAGE_IDX  0
#define DYNAMIC_FOLIAGE_IDX 1

	SFoliageRenderParams											m_foliageRenderParams;
	THashMap< CRenderSpeedTreeResource*, Uint32 >					m_usedSpeedTreeResources;					//!< Speed tree resources in use, excluding the grass resources
	TDynArray< SpeedTree::CTreeRender* >							m_updatedTrees;								//!< Updated baseTrees during streaming
	TDynArray< SpeedTree::SCellKey >								m_updatedCellsKeys;							//!< updated cell keys
	SpeedTree::CForestRender										m_forestRender;								//!< Forest renderer
	SpeedTree::CView												m_forestView;								//!< Frustum definition for LOD
	SpeedTree::CView												m_forestViewReversedProjection;				//!< Frustum definition for rendering
	SpeedTree::CVisibleInstancesRender								m_visibleTreesFromCamera[2];				//!< Visibility manager (culling) (0-trees, 1-herbs)
	TSpeedTreeGrid< SPerCellData, SpeedTree::CTreeInstance >		m_cellData[2];								//!< Array of cells, organized in a way allowing fast speed tree cells updation (0-trees, 1-herbs)
	Float															m_longestCellOverhang;						//!< A value need to avoid instances disappearing on the edges of tiles
	TDynArray< CGrassLayer* >										m_grassLayers;								//!< Each grass instance, needs a separate set of objects
	CSpeedTreeInstanceRingBuffer									m_grassInstancesRingBuffer;					//!< A ring buffer for all grass instancing
	Bool															m_lastUpdateTreesPresent[2];				//!< Trees was present on last update
	Bool															m_lastUpdateGrassPresent;					//!< Grass was present on last update
	Bool															m_cullFoliageWithUmbra;
	Bool															m_visible;									//!< Is foliage visible (rendering on/off)
	RedVisibilityHelper												m_vHelper;									//!< Visibility Helper
	GpuApi::BufferRef												m_grassConstantBufferRef;					//!< Custom constant buffer for grass (pigment and collision settings)
	SGrassConstantBuffer*											m_grassConstantBuffer;						//!< Pigment and dynamic collision buffer - cpu updated
	SpeedTree::CVisibleInstancesRender								m_visibleTreesFromCascades[2][ MAX_CASCADES ];//!< Visibility manager for cascades (culling trees (billboards, 3d trees)) (0-trees, 1-herbs)
	SpeedTree::CView												m_renderCascadeViews[ MAX_CASCADES ];		//!< View transforms for cascades
	SpeedTree::CView												m_cascadeViews[ MAX_CASCADES ];				//!< View transforms for cascades

	Box																m_clipmap0worldRect;						//!< World space rectangle covered by 0'th clipmap level
	Uint32															m_clipWindowResolution;						//!< Clipmap window resolution					
	Float															m_minElevation;								//!< Lowest available terrain elevation
	Float															m_maxElevation;								//!< Highest available terrain elevation
	Float															m_terrainInterVertexSpace;					//!< World scale space between two vertices on the best detail level

	CRenderProxy_Terrain*											m_cachedTerrainProxy;						//!< Cached terrain proxy
	Bool															m_genericGrassOn;							//!< Is generic grass enabled?

	CSpeedTreeDensityDebugger*										m_densityDebugger;							//!< Density debugging helper

	Vector															m_treeFadingLeftReference;					//!< Tree fading, to hide trees that may be blocking the player, left reference point
	Vector															m_treeFadingRightReference;					//!< Tree fading, to hide trees that may be blocking the player, right reference point
	Vector															m_treeFadingCenterReference;				//!< Tree fading, to hide trees that may be blocking the player, center reference point

	static Bool														m_treeFadingEnabled;						//!< Static switch
	static Float													m_treeFadingFadeInTime;						//!< Add comment
	static Float													m_treeFadingFadeOutTime;					//!< Add comment
	static Float													m_treeFadingMinTreeHeight;					//!< Add comment
	
	static Bool														m_UseMultiThreadedTrees;
	TDynArray< CGrassCellMask >										m_grassOccurrenceMasks;						//!< A mask per automatically populated grass type, allowing to skip cells that would only waste cycles

	class CShadowCascade
	{
	public:
		Matrix							m_mtxWorldToView;
		Matrix							m_mtxViewToScreen;
		Matrix							m_mtxWorldToScreen;
		Vector							m_camForward;
		Vector							m_camPos;
		Float							m_zoom;
		Float							m_camFarPlane;
		Float							m_camNearPlane;
		Uint16							m_cascadeIndex;			//!< Index of this cascade
		Uint16							m_foliageType;			//!< 0 - static foliage, 1 - dynamic foliage
		Bool							m_cullShadows;
#ifdef USE_UMBRA
		const CRenderOcclusionData*		m_occlusionData;
#endif // USE_UMBRA
	};

	struct SSpeedTreeParallelProcessingElement
	{
		SSpeedTreeParallelProcessingElement ( CShadowCascade *cascade, Float fadeDistance )
			: m_cascade ( cascade )
			, m_fadeDistance ( fadeDistance )
		{}

		CShadowCascade *m_cascade;
		Float m_fadeDistance;
	};

	typedef CParallelForTaskSingleArray< SSpeedTreeParallelProcessingElement, CRenderProxy_SpeedTree > TParallelShadowsProcess;
	TDynArray< SSpeedTreeParallelProcessingElement >				m_parallelShadowsProcessingElements;
	TParallelShadowsProcess::SParams*								m_parallelShadowsProcessingParams;			//!< Parallel shadows processing setup
	Bool															m_hasBaseTrees[2];							//!< For trees (0) and herbs (1)
	Vector3															m_cameraPosition_cascades;
	Bool															m_cullShadowsWithUmbra;
	TDynArray< CShadowCascade* >									m_cascadeThreadData;

	SpeedTree::Mat4x4												m_cachedViewMatrix;
	SpeedTree::Vec3													m_cachedCamPos_grass;
	Float															m_cachedFov;
	Float															m_cachedAspect;
	Float															m_cachedNearPlane;
	Bool															m_staticPopulatioChangeScheduled;
	Bool															m_staticPopulatioChangeInCascadesScheduled;
	Bool															m_dynamicPopulatioChangeScheduled;
	Bool															m_dynamicPopulatioChangeInCascadesScheduled;

	Uint32															m_staticFoliageBalance;
	Uint32															m_dynamicFoliageBalance;

	struct DynamicFoliageRemoveInstance
	{
		RenderObjectHandle m_baseTree;
		Vector m_position;
		Float m_radius;
	};
	typedef TDynArray< DynamicFoliageRemoveInstance, MC_FoliageInstances > DynamicFoliageRemoveInstancesContainer;							
	FoliageAddInstancesContainer									m_dynamicsToAddQueue;
	DynamicFoliageRemoveInstancesContainer							m_dynamicsToRemoveQueue;

	friend class CTreeInstanceProcessing;
	friend class CGrassInstanceProcessing;
	friend class CUpdateTreeInstancesProcessing;
	CTreeInstanceProcessing*										m_TreeProcessing[2];
	CGrassInstanceProcessing*										m_grassProcessing;
	CUpdateTreeInstancesProcessing*									m_updateInstancesProcessing;


	TreeRenderContainer												m_cachedBaseTrees;							//!< Cached between FrameUpdate and PreRenderUpdate
	TreeRenderContainer												m_cachedBaseGrass;

	Float m_currentFoliageDistanceScaleParam;	// keep current value to check if there's a need to update render params;
	Float m_currentGrassDistanceScaleParam;		// keep current value to check if there's a need to update render params;

public:
	CRenderProxy_SpeedTree();
	virtual ~CRenderProxy_SpeedTree();

	virtual void Prefetch( CRenderFramePrefetch* prefetch ) const override;

	void AttachToScene();
	void DetachFromScene();

	RED_INLINE virtual void SetVisible( Bool visible ) { m_visible = visible; }
	RED_INLINE Bool			IsVisible(){ return m_visible; }

	// Visualisation stuff
	void SetGrassDensityBudget( Float instancesPerSquareMeter );
	void SetTreeDensityBudget( Float instancesPerSquareMeter );
	void SetGrassLayerDensityBudget( Float layersPerSquareMeter );
	void DisableVisualisation();
	void EnableGrassInstanceVisualisation();
	void EnableGrassLayerVisualisation();
	void EnableTreeInstanceVisualisation();

	void MarkBaseTreeAsInteractive( RenderObjectHandle baseTree );

	void ProcessAllUpdateRequest( SFoliageUpdateRequest updates );

	void AddStaticInstances( IRenderObject* baseTree, const FoliageInstanceContainer & instancesData, const Box & box );
	void QueueAddDynamicInstances( RenderObjectHandle baseTreeHandle, const FoliageInstanceContainer & instancesData, const Box & box );
	void AddDynamicInstances( IRenderObject* baseTree, const FoliageInstanceContainer & instancesData, const Box & box );

	void RemoveStaticInstances( IRenderObject* baseTree, const Vector& position, Float radius );
	void QueueRemoveDynamicInstances( RenderObjectHandle baseTreeHandle, const Vector& position, Float radius );
	void RemoveDynamicInstances( IRenderObject* baseTree, const Vector& position, Float radius );
	void RemoveStaticInstances( IRenderObject* baseTree, const Box& rect );

	void ProcessQueuedInstances();

	void FinalizeParallelInstanceUpdates();
	void FinalizeParallelGrassProcessing();
	void FinalizeParallelCascadesProcessing();

	void CollectAllBaseTreesUsed( TreeContainer & baseTrees );
	void CollectAllBaseTreesUsed( TreeRenderContainer & baseTrees );
	void CollectAllBaseTreesUsed( ConstTreeContainer & baseTrees );

	void RefreshGenericGrass();

	// Finalize processing begun earlier in a frame ( culling etc. ). Do render foliage.
	void Render( const RenderingContext& context, const CRenderFrameInfo& frameInfo );
	// Finalize processing begun earlier in a frame ( culling etc. ). Don't render.
	void NoRender_FinalizeProcessing();

	//dex++: shadow integration
	void PrepareCascadeShadows( const CRenderCollector& collector );
	void PrepareSpecificCascadeShadows(CShadowCascade* const cascade, Float fadeDistance );
	void PrepareSpecificCascadeShadowsElement( SSpeedTreeParallelProcessingElement &element );
	void RenderCascadeShadows( const RenderingContext& context, const CRenderFrameInfo& frameInfo, const SShadowCascade* cascade );
	//dex--

	void FrameUpdate( const CRenderFrameInfo& frameInfo, CRenderSceneEx* scene );

	void PreRenderUpdate( const CRenderFrameInfo& frameInfo, CRenderSceneEx* scene
#ifdef USE_UMBRA		
		, const CRenderOcclusionData* occlusionData
#endif // USE_UMBRA
		, CRenderFrame* frame
		);

	// The terrain proxy had grass setup updated
	void OnGrassSetupUpdated( const TDynArray< IRenderObject* >& baseObjectsInUse );

	// Update dynamic grass collisions
	void UpdateDynamicGrassColisions( const TDynArray< SDynamicCollider >& collisions );

	// Apply a cell mask for each automatically populated grass type. A cell mask allow skipping empty cells.
	void SetOccurrenceMasks( TDynArray< CGrassCellMask >& occurrenceMasks );

public:
	void UpdateFoliageRenderParams( const SFoliageRenderParams &params );


	void UpdateCellFading( const SpeedTree::CCell* const& cell );

	static void SetupTreeFading( Bool enable );

	RED_INLINE void SetTreeFadingReferencePoints( const Vector& left, const Vector& right, const Vector& center )
	{
		m_treeFadingLeftReference = left;
		m_treeFadingRightReference = right;
		m_treeFadingCenterReference = center;
	}

private:
	void ProcessTreesInstances( const TreeRenderContainer & trees
#ifdef USE_UMBRA
		, Bool cullWithUmbra, const CRenderOcclusionData* occlusionData
#endif // USE_UMBRA
		, CRenderFrame* frame
		);

	void ProcessGrassInstances( const CRenderCamera &renderCamera
#ifdef USE_UMBRA
		, Bool cullWithUmbra, const CRenderOcclusionData* occlusionData
#endif // USE_UMBRA
		);

	void ProcessTreeBuffers(TreeRenderContainer& trees, Uint32 foliageType);
	void FinishProcessTreeBuffers();
	void ProcessGrassLayer( CGrassLayer*& grassLayer );
	void ProcessAllGrassLayers();
	void UpdateGrassLayerInstanceBuffer( CGrassLayer*& grassLayer );

	void UpdateGrassInstanceMetrics( const SpeedTree::CVisibleInstancesRender& visibleGrassFromCamera );
	void BuildGrassInstanceSpeedtreeView( SpeedTree::CTreeRender* baseTree, SpeedTree::CView& targetView );
	void UpdateGrassCellExtents( CGrassLayer* grassLayer, SpeedTree::CVisibleInstancesRender& visibleGrassFromCamera, Bool autoPopulate, const Vector2& areaSize );
	void PopulateVisibleGrassCells( CGrassLayer* grassLayer, SpeedTree::CVisibleInstancesRender& visibleGrassFromCamera, Bool autoPopulate );

	CGrassLayer* GetOrCreateGrassLayer( CRenderSpeedTreeResource* renderTree );
	CGrassLayer* GetGrassLayer( CRenderSpeedTreeResource* renderTree );

	void RemoveGrassInstances( CGrassLayer* layer, const Vector& position, Float radius );
	void RemoveGrassInstances( CGrassLayer* layer, const Box& rect );

	void RemoveTreeStaticInstances( CRenderSpeedTreeResource* renderTreeResource, const Vector& position, Float radius );
	void RemoveTreeDynamicInstances( CRenderSpeedTreeResource* renderTreeResource, const Vector& position, Float radius );
	void RemoveTreeStaticInstances( CRenderSpeedTreeResource* renderTreeResource, const Box& rect );

	void PreGrassLayerChange( CGrassLayer* layer );
	void PreTreeLayerChange();

	// Can destroy layer
	void PostGrassInstancesRemoval( CGrassLayer* layer );

	// Can decrement refcount
	void PostTreeInstancesRemoval( CRenderSpeedTreeResource* renderTreeResource );

	RED_INLINE void ScheduleStaticTreesPopulationChange() { m_staticPopulatioChangeScheduled = true; m_staticPopulatioChangeInCascadesScheduled = true; }
	RED_INLINE void ScheduleDynamicTreesPopulationChange() { m_dynamicPopulatioChangeScheduled = true; m_dynamicPopulatioChangeInCascadesScheduled = true; }
	RED_INLINE Bool IsStaticTreesPopulationChangedScheduled()				const { return m_staticPopulatioChangeScheduled; }
	RED_INLINE Bool IsStaticTreesPopulationChangedInCascadesScheduled()	const { return m_staticPopulatioChangeInCascadesScheduled; }
	RED_INLINE Bool IsDynamicTreesPopulationChangedScheduled()				const { return m_dynamicPopulatioChangeScheduled; }
	RED_INLINE Bool IsDynamicTreesPopulationChangedInCascadesScheduled()	const { return m_dynamicPopulatioChangeInCascadesScheduled; }
	void NotifyStaticTreesPopulationChanged();
	void NotifyStaticTreesPopulationChangedInCascades();
	void NotifyDynamicTreesPopulationChanged();
	void NotifyDynamicTreesPopulationChangedInCascades();
	
	void NotifyTreesPopulationRemoval( Uint16 foliageType, Int32 rowColRange[4], const SpeedTree::CTree* tree );
	void NotifyTreesPopulationAddition( Uint16 foliageType, TDynArray< SpeedTree::SCellKey >& updatedCellsKeys, const SpeedTree::CTree* tree );

	Bool InitializeBaseTree( CRenderSpeedTreeResource* baseTreeRenderResource, Uint32 foliageType = STATIC_FOLIAGE_IDX );

	Uint32 GenerateGrass( SpeedTree::CCell* targetCell, SpeedTree::TGrassInstArray& outInstances, CGrassLayer* grassLayer );

	void UpdateGrassConstants( const CRenderFrameInfo& frameInfo );

	void AddTreeStaticInstances( CRenderSpeedTreeResource* baseTreeRenderResource, const FoliageInstanceContainer & renderFoliageInstancesBuffer, SpeedTree::CTreeRender* treeRender, const Box & box );
	void AddTreeDynamicInstances( CRenderSpeedTreeResource* baseTreeRenderResource, const FoliageInstanceContainer & renderFoliageInstancesBuffer, SpeedTree::CTreeRender* treeRender, const Box & box );
	void AddGrassInstances( CRenderSpeedTreeResource* baseTreeRenderResource, const FoliageInstanceContainer & renderFoliageInstancesBuffer, SpeedTree::CTreeRender* treeRender, const Box & box );

	const CGrassCellMask* GetGrassCellMaskForLayer( const CGrassLayer* grassLayer ) const;

	void PrefetchHelper( CRenderFramePrefetch* prefetch, const TSpeedTreeGrid< SPerCellData, SpeedTree::CTreeInstance >& grid, const SpeedTree::Vec3& refPointVec ) const;
	void PrefetchHelper( CRenderFramePrefetch* prefetch, const ConstTreeRenderContainer& trees, Float squareDistance ) const;
	void PrefetchHelperAddTextureBinds( CRenderFramePrefetch* prefetch, const SpeedTree::CRenderState& renderState, Float squareDistance ) const;

	void CheckUpdateFoliageRenderParams();
	void ReinitializeRingBuffer();
};

#endif
