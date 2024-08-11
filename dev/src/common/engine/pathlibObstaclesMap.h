/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibObstacle.h"
#include "pathlibObstaclesGroup.h"

namespace PathLib
{

class CObstaclesDetourInfo;

////////////////////////////////////////////////////////////////////////////
// Obstacles list and spatial queries structure.
class CObstaclesMap : public CNavModyficationMap, public CAreaRes
{
	typedef CNavModyficationMap Super;
	friend class CVisualizer;	

	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_PathLib );

protected:
	static const Float IGNORE_SHORT_MESHES_Z;

	typedef TDynArray< CObstacle*, MC_PathLib > tCollisionCell;
	typedef TDynArray< tCollisionCell, MC_PathLib > tCollisionMap;
	typedef TArrayMap< CObstacle::Id, CObstacle* > tObstaclesList;

	typedef Red::Threads::CAtomic< Int32 > AtomicInt;
	typedef Red::Threads::CAtomic< Bool > AtomicBool;
	typedef Red::Threads::CMutex Mutex;

	tObstaclesList				m_obstacles;
	tCollisionMap				m_obstaclesMap;
	Uint16						m_cellsX;
	Uint16						m_cellsY;
	Vector3						m_areaLocalBBox[2];
	Float						m_celSizeInverted;
	Uint32						m_nextObstacleId;
	CObstacleGroupsCollection	m_groups;

private:
	AtomicBool					m_isModified;
	mutable AtomicInt			m_readersCount;
	mutable Mutex				m_modificationMutex;

protected:
	// Pair of reader - writer locks protecting obstacles map cel's as they can be modified without full area lock
	struct SmartReaderLock
	{
		enum EType
		{
			LOCKLESS_READ,
			FULL_LOCK
		};
		const CObstaclesMap*		m_map;
		EType 						m_type;

		RED_INLINE SmartReaderLock( const CObstaclesMap* map );
		RED_INLINE  ~SmartReaderLock();
	};

	struct SmartWriterLock
	{
		enum EType
		{
			FULL_LOCK,
			NO_LOCK
		};
		CObstaclesMap*				m_map;
		EType 						m_type;

		RED_INLINE SmartWriterLock( CObstaclesMap* map );
		RED_INLINE  ~SmartWriterLock();
	};


	RED_INLINE void			TranslateWorld2Cel( const Vector2& v, Uint16& xOut , Uint16& yOut ) const
	{
		// x
		Float fX = v.X - m_areaLocalBBox[0].X;
		if ( fX < 0.f )
			fX = 0.f;
		xOut = Uint16( fX * m_celSizeInverted);
		if ( xOut >= m_cellsX )
			xOut = m_cellsX - 1;
		// y
		Float fY = v.Y - m_areaLocalBBox[0].Y;
		if ( fY < 0.f )
			fY = 0.f;
		yOut = Uint16( fY * m_celSizeInverted );
		if ( yOut >= m_cellsY )
			yOut = m_cellsY - 1;
	}
	RED_INLINE Uint32			GetCelIndex( Uint16 x, Uint16 y ) const											{ ASSERT( x < m_cellsX && y < m_cellsY ); return y * m_cellsX + x; }

public:

	struct ObstaclesIterator
	{
	protected:
		CObstaclesMap*					m_map;
		Uint16							m_minX;
		Uint16							m_minY;
		Uint16							m_maxX;
		Uint16							m_maxY;
		Uint16							m_x;
		Uint16							m_y;
		Bool							m_inProgress;
		tCollisionCell::const_iterator	m_itCel;
		tCollisionCell::const_iterator	m_endCel;

		void					CelEntered();
		void					Progress();
	public:
		ObstaclesIterator( CObstaclesMap* map, const Vector2& bboxMin, const Vector2& bboxMax );

		operator Bool() const																					{ return m_inProgress; }
		void					operator++()																	{ ++m_itCel; Progress(); }
		CObstacle*				operator*()	const																{ return *m_itCel; }
	};


	static const Uint16 RES_VERSION = 9;

	CObstaclesMap();
	~CObstaclesMap();

	// shape creation (possibly they can exist independently from obstacles)
	CObstacleShape*				NewShape( TDynArray< Vector2 >&& convexHull, const Vector3& bbmin, const Vector3& bbmax ) const;
	CObstacleShape*				NewCylinderShape( const Vector2& cylinderPos, Float radius, const Vector3& bbmin, const Vector3& bbmax ) const;
	CObstacleShape*				ComputeShape( CObstacleSpawnContext& data );

	// obstacles managment
	CObstacle*					CreateObstacle( const CObstacleSpawnData& data );								// creates new obstacle based on engine static mesh
	Bool						UpdateObstacleShape( const CObstacleSpawnData& data, CObstacle* obstacle );		// try to update obstacle shape so its matches possibly new collision mesh
	Bool						ObstacleTypeChanged( EPathLibCollision obstacleType, CObstacle* obstacle );
	Bool						SimplifyObstacles();															// simplifies obstacles on area - destroy hanging ones
	Bool						SimplifyShape( CObstacleShape*& shape );

	// pathlib internal interface
	//
	CObstacle*					GetObstacle( CObstacle::Id id ) const;
	CObstacle::Id				AddObstacle( CObstacle* obstacle, const CObstacleSpawnData& spawnData );
	Bool						RemoveObstacle( CObstacle::Id id );

	CObstacleGroupsCollection&			GetObstacleGroups()														{ return m_groups; }
	const CObstacleGroupsCollection&	GetObstacleGroups()	const												{ return m_groups; }

	Bool						ShowObstacle( CObstacle::Id id, CComponentRuntimeProcessingContext& context );
	Bool						HideObstacle( CObstacle::Id id, CComponentRuntimeProcessingContext& context );

	void						OnGraphClearance( CNavGraph* navgraph );
	void						PostGraphGeneration( CNavGraph* navgraph );
	void						GenerateDetourInfo( CObstaclesDetourInfo& detourInfo, Float personalSpace, Bool computeZ );

	void						DepopulateArea( Bool runNotyfications );										// special functions needed to remove all occluders from collision map, keeping occluders intact
	void						DepopulateArea( Bool runNotyfications, Bool persistant );
	void						RepopulateArea( Bool runNotyfications );										// repopulates collision map with occluders
	void						RepopulateArea( Bool runNotyfications, Bool persistant );

	Bool						Add2CollisionMap( CObstacle* obstacle );
	void						RemoveFromCollisionMap( CObstacle* obstacle );

	void						Initialize();
	void						Shutdown();

	Bool						MarkInstance( CNavmeshAreaDescription* area );
	Bool						UpdateObstaclesAreaMarking( const Vector2& localMin, const Vector2& localMax );

	Bool						TestLocation( const Vector3& v ) const;

	template < class TQuery >
	Bool						TSpatialQuery( TQuery& query ) const;

	void						SerializeToDataBuffer( TDynArray< Int8 >& buffer ) const;
	static CObstaclesMap*		SertializeFromDataBuffer( const TDynArray< Int8 >& buffer, CAreaDescription* area );

	Bool						ReadFromBuffer( CSimpleBufferReader& reader );
	void						WriteToBuffer( CSimpleBufferWriter& writer ) const;

	void						OnPreLoad( CAreaDescription* area );
	void						OnPostLoad( CAreaDescription* area );
	void						OnPreUnload( CAreaDescription* area );
	Bool						Save( const String& depotPath ) const;
	Bool						Load( const String& depotPath );

	// CAreaRes interface
	Bool						VHasChanged() const override;
	Bool						VSave( const String& depotPath ) const override;
	void						VOnPreLoad( CAreaDescription* area ) override;
	Bool						VLoad( const String& depotPath, CAreaDescription* area ) override;
	void						VOnPostLoad( CAreaDescription* area ) override;
	void						VOnPreUnload( CAreaDescription* area ) override;
	const Char*					VGetFileExtension() const override;
	ENavResType					VGetResType() const override;

	static const Char*			GetFileExtension()																{ return TXT("naviobstacles"); }
	static ENavResType			GetResType()																	{ return NavRes_Obstacles; }
};


};			// namespace PathLib

