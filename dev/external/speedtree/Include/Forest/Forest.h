///////////////////////////////////////////////////////////////////////  
//  Forest.h
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
//
//  *** Release version 7.0.0 ***

// LAVA++
// In order to ensure that the heap reservation for instance buffers does NOT happen (it uses too much memory on 
// heavy scenes with 1000s of cells), we disable the default values in CellHeapMgr::Checkout
// In order to avoid heap allocations, we must be careful to only resize the instance buffers once, when populating them after the fine cull
#define DISABLE_VISIBLE_INSTANCE_HEAP_RESERVATION_DEFAULTS
// LAVA--

///////////////////////////////////////////////////////////////////////  
//  Preprocessor

#pragma once
#include "Core/ExportBegin.h"
#include "Core/Core.h"
#include "Core/Map.h"
#include "Core/Set.h"
#include "Core/String.h"
#include "Core/Wind.h"
#include "Core/HeapAllocCheck.h"
#include "Core/ScopeTrace.h"
#include "Utilities/Utility.h"


///////////////////////////////////////////////////////////////////////  
//  Packing

#ifdef ST_SETS_PACKING_INTERNALLY
	#pragma pack(push, 4)
#endif


///////////////////////////////////////////////////////////////////////  
//  SDK Compile-time configuration options

// for very dense forests (50,000+ trees in the frustum), consider #defining
// SPEEDTREE_FAST_BILLBOARD_STREAMING. It will make streaming billboard vertex
// instance data much faster at the expense of heap allocations during run-time
#define SPEEDTREE_FAST_BILLBOARD_STREAMING


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

	///////////////////////////////////////////////////////////////////////  
	//  Constants

	const st_int32 c_nMaxNumShadowMaps = 4;
	const st_int32 c_nInvalidRowColIndex = -999999;
	const st_int32 c_nNumFrustumPoints = 8;


	///////////////////////////////////////////////////////////////////////  
	//  Structure SHeapReserves

	struct ST_DLL_LINK SHeapReserves
	{
					SHeapReserves( );

		st_int32	m_nMaxBaseTrees;						// the max number base trees expected to be used in a given forest (not many more than 20-30 is recommended)

		st_int32	m_nMaxVisibleTreeCells;					// the max number of tree cells visible at any one time in the camera frustum
		st_int32	m_nMaxVisibleGrassCells;				// the max number of grass cells visible at any one time in the camera frustum
		st_int32	m_nMaxVisibleTerrainCells;				// the max number of terrain cells visible at any one time in the camera frustum

		st_int32	m_nMaxTreeInstancesInAnyCell;			// across all base trees, the max number of instances in any tree cell
		st_int32	m_nMaxPerBaseGrassInstancesInAnyCell;	// for any single base grass, the max number of instances in any grass cell

		st_int32	m_nNumShadowMaps;						// # of expected shadow maps; isn't dynamic and should never exceed 4
	};


	///////////////////////////////////////////////////////////////////////
	//  Structure S3dTreeInstanceVertex

	struct S3dTreeInstanceVertex
	{
		// first float4
		Vec3		m_vPos;
		st_float32	m_fScalar;

		// second float4
		Vec3		m_vUpVector;
		st_float32	m_fLodTransition;

		// third float4
		Vec3		m_vRightVector;
		st_float32	m_fLodValue;
	};


	///////////////////////////////////////////////////////////////////////
	//  Structure SBillboardInstanceVertex

	struct SBillboardInstanceVertex
	{
		// first slot
		Vec3		m_vPos;
		st_float32	m_fScalar;

		// second slot
		Vec3		m_vUpVector;
		float		m_fPad0;

		// third
		Vec3		m_vRightVector;
		float		m_fPad1;
	};


	///////////////////////////////////////////////////////////////////////  
	//  Class CInstance

	// #define SPEEDTREE_COMPRESS_INSTANCE_VECTORS
	//
	// SPEEDTREE_COMPRESS_INSTANCE_VECTORS offers a trade-off:
	//
	//	- smaller CInstance objects, but slower Get*() queries (used to populate instance buffers), or
	//  - larger CInstance objects, but faster Get*() queries (default state)

	class ST_DLL_LINK CInstance
	{
	public:
			friend class CCullEngine;

								CInstance( );
								~CInstance( );

			// parameter settings
			void				SetPos(const Vec3& vPos);
			void				SetScalar(st_float32 fScalar);
			void				SetOrientation(const Vec3& vUp, const Vec3& vRight);
			void				SetInstanceOf(const CTree* pBaseTree);

			// parameter queries
			const CTree*		InstanceOf(void) const;
			const Vec3&			GetPos(void) const;
			st_float32			GetScalar(void) const;
			Vec3				GetUpVector(void) const;
			Vec3				GetRightVector(void) const;

			// convenience functions for quick 4fv shader constant uploads
			const st_float32*	GetUpVectorAndLodTransition(void) const;
			const st_float32*	GetRightVectorAndLodValue(void) const;

	protected:
			const CTree*		m_pInstanceOf;
			Vec3				m_vPos;					// default to (0.0f, 0.0f, 0.0f)
			st_float32			m_fScalar;				// 1.0 = no scale

			#ifdef SPEEDTREE_COMPRESS_INSTANCE_VECTORS
				//st_uint8		m_auiUp[3]; // ctremblay +- We don't need up vector. 
				st_uint8		m_auiRight[3];
			#else
				//Vec3			m_vUp;		// ctremblay +- We don't need up vector.
				Vec3			m_vRight;
			#endif
	};


	///////////////////////////////////////////////////////////////////////  
	//  Class CTreeInstance

	class ST_DLL_LINK CTreeInstance : public CInstance
	{
	public:
			friend class CCullEngine;

										CTreeInstance( );
										~CTreeInstance( );

			// culling
			void						ComputeCullParameters(void);
			const Vec3&					GetGeometricCenter(void) const;
			st_float32					GetCullingRadius(void) const;
			st_bool						IsCulled(void) const;

			// user data
			void*						GetUserData(void) const;
			void						SetUserData(void* pUserData);

	private:
			Vec3						m_vGeometricCenter;		// includes position offset
			st_float32					m_fCullingRadius;
			void*						m_pUserData;
	};


	///////////////////////////////////////////////////////////////////////  
	//  Structure SGrassInstance

	struct ST_DLL_LINK SGrassInstance
	{
								SGrassInstance( ) :
									m_fLodTransition(1.0f),
									m_fLodValue(1.0f)
								{
								}

			// LAVA++: Let's add setters and getters
			// parameter settings
			void				SetPos(const Vec3& vPos)								{ m_vPos = vPos; }
			void				SetScalar(st_float32 fScalar)							{ m_fScalar = fScalar; }
			void				SetOrientation(const Vec3& vUp, const Vec3& vRight)		{ m_vRight = vRight; m_vUp = vUp; }

			// parameter queries
			const Vec3&			GetPos(void) const			{ return m_vPos; }
			st_float32			GetScalar(void) const		{ return m_fScalar; }
			Vec3				GetUpVector(void) const		{ return Vec3(0.0f, 0.0f, 1.0f); }
			Vec3				GetRightVector(void) const	{ return m_vRight; }
			// LAVA--

			Vec3				m_vPos;
			st_float32			m_fScalar;

			Vec3				m_vUp;		// LAVA: Up is always 0,0,1
			st_float32			m_fLodTransition;

			Vec3				m_vRight;
			st_float32			m_fLodValue;
	};
	typedef SGrassInstance SGrassInstanceVertex;



	///////////////////////////////////////////////////////////////////////  
	//  Structure SCellKey

	struct SCellKey
	{
							SCellKey(st_int32 nRow, st_int32 nCol) :
								m_nRow(nRow),
								m_nCol(nCol)
							{
							}

			st_bool			operator<(const SCellKey& sIn) const	{ return (m_nRow == sIn.m_nRow) ? (m_nCol < sIn.m_nCol) : (m_nRow < sIn.m_nRow); }
			st_bool			operator!=(const SCellKey& sIn) const	{ return (m_nRow != sIn.m_nRow || m_nCol != sIn.m_nCol); }

			st_int32		m_nRow;
			st_int32		m_nCol;
	};


	///////////////////////////////////////////////////////////////////////  
	//  Type definitions

	typedef CArray<CTreeInstance>			TTreeInstArray;
	typedef CArray<SGrassInstance>			TGrassInstArray;
	typedef CArray<const CTreeInstance*>	TTreeInstConstPtrArray;


	///////////////////////////////////////////////////////////////////////  
	//	Enumeration ECullStatus

	enum ECullStatus
	{
		CS_FULLY_INSIDE_FRUSTUM,		// object is fully inside the frustum
		CS_INTERSECTS_FRUSTUM,			// object is partly in the frustum and partly out
		CS_FULLY_OUTSIDE_FRUSTUM		// object is fully outside of the frustum
	};

	///////////////////////////////////////////////////////////////////////  
	//  Structure SRowCol

	struct SRowCol
	{
		// LAVA++
		SRowCol() :
			 m_nRow( 0 ),
			 m_nCol( 0 )
		{
		}
		// LAVA--

		SRowCol(st_int32 nRow, st_int32 nCol) :
			m_nRow(nRow),
			m_nCol(nCol)
		{
		}

		st_bool							operator<(const SRowCol& sRight) const
		{
			if (m_nRow == sRight.m_nRow)
				return m_nCol < sRight.m_nCol;
			else
				return m_nRow < sRight.m_nRow;
		}

		st_bool							operator!=(const SRowCol& sRight) const
		{
			return (m_nRow != sRight.m_nRow || m_nCol != sRight.m_nCol);
		}

		st_int32						m_nRow;
		st_int32						m_nCol;
	};


	///////////////////////////////////////////////////////////////////////  
	//  Class CCell

	class CCell
	{
	public:
			friend class CVisibleInstances;
			template<typename T> friend class CCellHeapMgr;

											CCell( );
											~CCell( );

			// cell's grid placement
			void							SetRowCol(st_int32 nRow, st_int32 nCol);
			st_int32						Row(void) const;
			st_int32						Col(void) const;
			st_int32						UniqueRandomSeed(void) const;
			void							SetExtents(const CExtents& cExtents);
			const CExtents&					GetExtents(void) const;

			// culling & LOD values
			ECullStatus						GetCullStatus(void) const;
			st_float32						GetLodDistanceSquared(void) const;
			st_float32						GetLongestBaseTreeLodDistanceSquared(void) const;

            // adds a series of instances to this cell; base trees are passed as a series of CTree 
            // pointers; instances are passed an as array of CInstance pointers; instances should
            // be sorted by base tree affiliation and should be in the same order as the pBaseTrees
			void							AppendTreeInstances(const CTree** pBaseTrees, st_int32 nNumBaseTrees, const CTreeInstance** pInstances, st_int32 nNumInstances);

            // returns an array of pointers to tree instances in this cell; the instances may
            // be of more than one base tree; they'll be sorted by base tree affiliation and the
            // base tree order will be what was passed to AppendTreeInstances()
           const TTreeInstConstPtrArray&	GetTreeInstances(void) const;

			// grass instance management
			const TGrassInstArray&			GetGrassInstances(void) const;
			void							SetGrassInstances(const SGrassInstance* pInstances, st_int32 nNumInstances);

			// operator overloads
			st_bool							operator<(const CCell& cRight) const;
			st_bool							operator!=(const CCell& cRight) const;

			#ifdef SPEEDTREE_FAST_BILLBOARD_STREAMING
				CMap<const CTree*, CArray<SBillboardInstanceVertex>>	m_mBaseTreesToBillboardVboStreamsMap; // [base tree #][bb instance #]
			#endif

	private:
			// grid placement
			st_int32						m_nRow;
			st_int32						m_nCol;
			CExtents						m_cExtents;

			// culling
			st_int32						m_nFrameIndex;
			ECullStatus						m_eCullStatus;

			// LOD
			st_float32						m_fLodDistanceSquared;
			st_float32						m_fLongestBaseTreeLodDistanceSquared;

			// instances in cell
			TTreeInstConstPtrArray			m_aTreeInstances; // stores pointers to tree instances
			TGrassInstArray					m_aGrassInstances; // stores grass instances
	};


	///////////////////////////////////////////////////////////////////////  
	//  Structure S3dTreeInstanceLod

	struct ST_DLL_LINK S3dTreeInstanceLod
	{
			const CTreeInstance*		m_pInstance;					// the instance this object is storing LOD info for
			st_float32					m_fDistanceFromCameraSquared;	// distance from camera (or LOD ref point)
			st_float32					m_fLod;							// [-1.0 to 1.0] value indicating LOD state
            st_int32					m_nLodLevel;					// which discrete LOD level is active
			st_float32					m_fLodTransition;				// LOD hints to the shader system for smooth transitions
	};


	///////////////////////////////////////////////////////////////////////  
	//  Structure SDetailedCullData

	struct ST_DLL_LINK SDetailedCullData
	{
		SDetailedCullData( );

		const CTree*				m_pBaseTree;
		st_float32					m_fClosest3dTreeDistanceSquared;		// FLT_MAX if none appear
		st_float32					m_fClosestBillboardCellDistanceSquared;	// FLT_MAX if none appear
		CArray<S3dTreeInstanceLod>	m_a3dInstanceLods;
	};


	///////////////////////////////////////////////////////////////////////  
	//  Enumerations EPopulationType

	enum EPopulationType
	{
		POPULATION_TREES,
		POPULATION_GRASS
	};


	///////////////////////////////////////////////////////////////////////  
	//  Type definitions, cont

	typedef CArray<const CTreeInstance*>	TTreeInstConstPtrArray;
	typedef CArray<CTree*>					TTreePtrArray;
	typedef CArray<S3dTreeInstanceLod>		T3dTreeInstanceLodArray;
	typedef CArray<const CTree*>			TTreeConstPtrArray;
	typedef CArray<CCell>					TCellArray;
	typedef CArray<CCell*>					TCellPtrArray;
	typedef CMap<SRowCol, CCell*>			TRowColCellPtrMap;
	typedef CArray<TTreeInstArray>			TTreeInstArray2D;
	typedef	CArray<SDetailedCullData>		TDetailedCullDataArray;


	///////////////////////////////////////////////////////////////////////  
	//  Class CCellHeapMgr
	//
	//	Internal use

	template <typename T>
	class ST_DLL_LINK CCellHeapMgr
	{
	public:
									CCellHeapMgr(EPopulationType ePopulationType);
									~CCellHeapMgr( );

			void					Init(st_int32 nMaxNumCells, st_int32 nMaxNumInstancesPerCell);

			T*						CheckOut(void);
			void					CheckIn(T* pCell);

	private:
			void					Grow(void);
			void					InitCell(T& tCell) const;
			st_bool					IsInitialized(void) const;

			EPopulationType			m_ePopulationType;

			struct ST_DLL_LINK SHeapBlock
			{
									SHeapBlock( )
									{
										m_aGrassInstBuffer.SetHeapDescription("CCellHeapMgr::m_aGrassInstBuffer");
										m_aCellBuffer.SetHeapDescription("CCellHeapMgr::m_aCellBuffer");
									}

				TGrassInstArray		m_aGrassInstBuffer;
				TCellArray			m_aCellBuffer;
			};

			CArray<SHeapBlock*>		m_aHeapBlocks;
			CArray<T*>				m_aAvailableCells;

			st_int32				m_nMaxNumCells;
			st_int32				m_nMaxNumInstancesPerCell;
	};


	///////////////////////////////////////////////////////////////////////  
	//  Enumeration EFrustumPlanes

	enum EFrustumPlanes
	{
		NEAR_PLANE, FAR_PLANE, RIGHT_PLANE, LEFT_PLANE, BOTTOM_PLANE, TOP_PLANE, NUM_PLANES
	};


	///////////////////////////////////////////////////////////////////////  
	//  Class CView

	class ST_DLL_LINK CView
	{
	public:

										CView( );

			// returns true if the values passed in our different from the internal values, false otherwise
			st_bool						Set(const Vec3& vCameraPos,
											const Mat4x4& mProjection,
											const Mat4x4& mModelview,
											st_float32 fNearClip,
											st_float32 fFarClip,
											st_bool bPreventLeafCardFlip = false);
			void						SetLodRefPoint(const Vec3& vLodRefPoint);

			// get parameters set directly
			const Vec3&					GetCameraPos(void) const;
			const Vec3&					GetLodRefPoint(void) const;
			const Mat4x4&				GetProjection(void) const;
			const Mat4x4&				GetModelview(void) const;
			const Mat4x4&				GetModelviewNoTranslate(void) const;
			st_float32					GetNearClip(void) const;
			st_float32					GetFarClip(void) const;

			// get derived parameters
			const Vec3&					GetCameraDir(void) const;
			const Mat4x4&				GetComposite(void) const;
			const Mat4x4&				GetCompositeNoTranslate(void) const;
			const Mat4x4&				GetProjectionInverse(void) const;
			st_float32					GetCameraAzimuth(void) const;
			st_float32					GetCameraPitch(void) const;
			const Vec3*					GetFrustumPoints(void) const;
			const Vec4*					GetFrustumPlanes(void) const;
			const CExtents&				GetFrustumExtents(void) const;

			// get derived-by-request parameters
			const Mat4x4&				GetCameraFacingMatrix(void) const;

			// horizontal billboard support
			void						SetHorzBillboardFadeAngles(st_float32 fStart, st_float32 fEnd); // in radians
			void						GetHorzBillboardFadeAngles(st_float32& fStart, st_float32& fEnd) const; // in radians
			st_float32					GetHorzBillboardFadeValue(void) const; // 0.0 = horz bbs are transparent, 1.0 = horz bb's opaque

	private:
			void						ComputeCameraFacingMatrix(st_bool bPreventLeafCardFlip);
			void						ComputeFrustumValues(void);
			void						ExtractFrustumPlanes(void);

			// parameters are set directly
			Vec3						m_vCameraPos;
			Vec3						m_vLodRefPoint;
			Mat4x4						m_mProjection;
			Mat4x4						m_mProjectionInverse;
			Mat4x4						m_mModelview;
			st_float32					m_fNearClip;
			st_float32					m_fFarClip;

			// derived
			Vec3						m_vCameraDir;
			Mat4x4						m_mComposite;
			Mat4x4						m_mModelviewNoTranslate;
			Mat4x4						m_mCompositeNoTranslate;
			st_float32					m_fCameraAzimuth;
			st_float32					m_fCameraPitch;
			Vec3						m_avFrustumPoints[c_nNumFrustumPoints];
			Vec4						m_avFrustumPlanes[NUM_PLANES];
			CExtents					m_cFrustumExtents;

			// derived on request
			Mat4x4						m_mCameraFacingMatrix;

			// horizontal billboards
			st_float32					m_fHorzFadeStartAngle;
			st_float32					m_fHorzFadeEndAngle;
			st_float32					m_fHorzFadeValue;
    };

	// LAVA++
	class VisibilityHelper
	{
	public:
		/** Possible return values for occlusion testing */
		enum VisibilityTestResult
		{
			/** \brief Target volume is completely occluded */
			OCCLUDED      = 0x0,
			/** \brief Target volume may be at least partially visible */
			VISIBLE       = 0x1,
			/** \brief Target volume is probably fully visible */
			FULLY_VISIBLE = 0x3
		};

		st_bool		m_useLegacyVisibilityTest;

	private:
		st_bool		m_isShadowPass;
		st_bool		m_needsPerTreeVisibilityTest;
		Vec3		m_shadowReferenceCamera;
		st_float32	m_shadowDistanceMultiplier;
		st_float32	m_shadowDistanceBillboardMultiplier;
		st_float32	m_shadowFadeRange;

	public:
		VisibilityHelper( st_bool isShadowPass, st_bool needsPerTreeVisTest, const Vec3 &shadowReferenceCamera, st_float32 shadowDistanceMultiplier, st_float32 shadowDistanceBillboardMultiplier, st_float32 shadowFadeRange  )
			: m_isShadowPass( isShadowPass )
			, m_needsPerTreeVisibilityTest( needsPerTreeVisTest )
			, m_shadowReferenceCamera( shadowReferenceCamera )
			, m_shadowDistanceMultiplier( shadowDistanceMultiplier )
			, m_shadowDistanceBillboardMultiplier( st_max( 0.0f, shadowDistanceBillboardMultiplier )  )
			, m_shadowFadeRange( shadowFadeRange )
		{ 
			m_useLegacyVisibilityTest = true; 
		}

		virtual VisibilityTestResult IsVisible( const CExtents& extents ) const = 0;
		virtual bool IsTreeVisible( const Vec3& c_vCullingCenter, const st_float32 c_cullingRadius ) const = 0;

		inline bool NeedsPerTreeVisTest() const
		{
			return m_needsPerTreeVisibilityTest;
		}

		bool CalculateLodRatio( const Vec3& c_vCullingCenter, const st_float32 instanceHeight, const CTree &tree, const st_float32 distanceSquared, st_float32 &outLodRatio ) const
		{
			if ( m_isShadowPass )
			{
				const SpeedTree::SLodProfile &lodProfile = tree.GetLodProfile();
				
				const st_float32 minInstanceHeight = 1.0f;
				const st_float32 maxInstanceHeight = 10.5f;
				const st_float32 instanceHeightClamped = Clamp( instanceHeight, minInstanceHeight, maxInstanceHeight );			
				
				//st_float32 xX = (c_vCullingCenter.x - m_shadowReferenceCamera.x);
				//st_float32 yY = (c_vCullingCenter.y - m_shadowReferenceCamera.y);	
				//st_float32 zZ = (c_vCullingCenter.z - m_shadowReferenceCamera.z);
				//const st_float32 distToCamera2D = sqrt( xX*xX + yY*yY + zZ * zZ );

				const st_float32 distToCamera2D = sqrt( distanceSquared ); //< use the same distance as the one being used for lod's calculation (faster, and shadows fade better matches the billboard transition)
				
				st_float32 fadeRange = m_shadowFadeRange;
				st_float32 shadowMaxRange = m_shadowDistanceMultiplier*instanceHeightClamped;

				if ( lodProfile.m_bLodIsPresent )
				{
					fadeRange = st_min( m_shadowDistanceBillboardMultiplier * lodProfile.m_fBillboardFinalDistance, fadeRange );
					shadowMaxRange = st_min( lodProfile.m_fBillboardFinalDistance - fadeRange, shadowMaxRange );
				}
				
				outLodRatio = -Clamp( (distToCamera2D - shadowMaxRange)/ fadeRange, 0.0f, 1.0f );
			}		
			else
			{
				outLodRatio = tree.GetLodProfileSquared().m_bLodIsPresent ? tree.ComputeLodByDistanceSquared( distanceSquared ) : 1;				
			}

			return outLodRatio > -1;
		}
	};
	// LAVA--


	///////////////////////////////////////////////////////////////////////  
	//  Class CVisibleInstances

	class ST_DLL_LINK CVisibleInstances
	{
	public:
			friend class CCullingEngine;

											CVisibleInstances(EPopulationType ePopulationType = POPULATION_TREES, st_bool bTrackNearestInsts = false);
	virtual									~CVisibleInstances( );

			// init
	virtual	void							SetHeapReserves(const SHeapReserves& sHeapReserves);
			void							SetCellSize(st_float32 fCellSize); // should be invoked during init and not invoked again
			void							Clear(void);

            st_bool                         RoughCullCells(const CView& cView, st_int32 nFrameIndex, st_float32 fLargestCellOverhang);
            void                            FineCullTreeCells(const CView& cView, st_int32 nFrameIndex);
			void                            FineCullGrassCells(const CView& cView, st_int32 nFrameIndex, st_float32 fLargestCellOverhang);
			
			// LAVA++
			void							FineCullTreeCells( const CView& cView, st_int32 nFrameIndex, VisibilityHelper* visibilityHelper );
			void							Update3dTreeLists(const CView& cView, VisibilityHelper* visibilityHelper );
			// LAVA--

	virtual void							Update3dTreeLists(const CView& cView );

	static	st_bool    ST_CALL_CONV 		InstanceIsVisible(const CView& cView, const CTreeInstance& cInstance);
	
			// cell queries
			TCellArray&						RoughCells(void);
			const TRowColCellPtrMap&		VisibleCells(void) const;
			TCellPtrArray&					NewlyVisibleCells(void);
			
			// other
			void							GetExtentsAsRowCols(st_int32& nStartRow, st_int32& nStartCol, st_int32& nEndRow, st_int32& nEndCol) const;
	virtual	void							NotifyOfFrustumReset(void);
	virtual	void							NotifyOfPopulationChange(void);
	virtual	void							NotifyOfPopulationRemoval( st_int32 rowCol[4] );
	virtual	void							NotifyOfPopulationAddition( st_int32 row, st_int32 col );
			const TDetailedCullDataArray&	Get3dInstanceLods(void) const;
			SDetailedCullData*				GetInstaceLodArrayByBase(const CTree* pBaseTree);

	private:
			T3dTreeInstanceLodArray*		FindInstanceLodArray(const CTree* pBaseTree); // todo: remove

			// CVisibleInstances usage hints
			EPopulationType					m_ePopulationType;		// does this object hold trees or grass
			st_bool							m_bTrackNearestInsts;	// track nearest 3d inst and bb cell for each instance (slower)

			// cell storage/management
			CCellHeapMgr<CCell>				m_cCellHeapMgr;
			TCellArray						m_aRoughCullCells;
			TRowColCellPtrMap				m_mVisibleCells;
			TCellPtrArray					m_aNewlyVisibleCells;

			// instances
			TDetailedCullDataArray			m_aPerBase3dInstances;

			// limit data
			st_float32						m_fCellSize;

			// optimization, prevent redundant computation
			st_int32						m_anLastFrustumPointCells[c_nNumFrustumPoints][2];
			st_int32						m_nFrustumExtentsStartRow;
			st_int32						m_nFrustumExtentsStartCol;
			st_int32						m_nFrustumExtentsEndRow;
			st_int32						m_nFrustumExtentsEndCol;
	};


	///////////////////////////////////////////////////////////////////////  
	//  Class CForest

	class ST_DLL_LINK CForest
	{
	public:
										CForest( );
	virtual								~CForest( );

	public:

			// collision
			st_bool						CollisionAdjust(Vec3& vPoint, const CVisibleInstances& cVisibleInstances, st_int32 nMaxNumTestTrees);

			// wind
			void						WindEnable(st_bool bEnabled);
			st_bool						WindIsEnabled(void) const;
			void						WindEnableGusting(const TTreePtrArray& aBaseTrees, st_bool bEnabled);
			st_bool						WindIsGustingEnabled(void) const;
			void						WindAdvance(const TTreePtrArray& aBaseTrees, st_float32 fWallTimeInSecs);
			void						WindPreroll(const TTreePtrArray& aBaseTrees, st_float32 fWallTimeInSecs);
			void						WindSetStrength(const TTreePtrArray& aBaseTrees, st_float32 fTreeStrength);
			void						WindSetInitDirection(const TTreePtrArray& aBaseTrees, const Vec3& vWindDir);
			void						WindSetDirection(const TTreePtrArray& aBaseTrees, const Vec3& vWindDir);

			st_float32					WindGetGlobalTime(void) const;

			// render support
			void						FrameEnd(void);
			st_int32					GetFrameIndex(void) const;

			// shadows & lighting support
			void						SetLightDir(const Vec3& vLightDir);
			const Vec3&					GetLightDir(void) const;
			st_bool						LightDirChanged(void) const;

	static	st_bool		   ST_CALL_CONV	ComputeLightView(const Vec3& vLightDir, 
														 const Vec3 avMainFrustum[c_nNumFrustumPoints], 
														 st_float32 fMapStartDistance,
														 st_float32 fMapEndDistance,
														 CView& sLightView,
														 st_float32 fRearExtension);
			void						SetCascadedShadowMapDistances(const st_float32 afSplits[c_nMaxNumShadowMaps], st_float32 fFarClip); // each entry marks the end distance of its respective map
			const st_float32*			GetCascadedShadowMapDistances(void) const;
			void						SetShadowFadePercentage(st_float32 fFade);
			st_float32					GetShadowFadePercentage(void) const;

	protected:
			// instance storage & culling
			st_int32					m_nFrameIndex;

			// wind
			st_bool						m_bWindEnabled;
			st_bool						m_bWindGustingEnabled;
			st_float32					m_fWindTime;
			Vec3						m_vWindDir;

			// shadows & lighting support
			Vec3						m_vLightDir;
			st_bool						m_bLightDirChanged;
			st_float32					m_afCascadedShadowMapSplits[c_nMaxNumShadowMaps + 1];
			st_float32					m_fShadowFadePercentage;
	};


	// include inline functions
	#include "Forest_inl.h"
	#include "Culling_inl.h"
	#include "Instance_inl.h"
	#include "CellHeapMgr_inl.h"

} // end namespace SpeedTree

#ifdef ST_SETS_PACKING_INTERNALLY
	#pragma pack(pop)
#endif

#include "Core/ExportEnd.h"
