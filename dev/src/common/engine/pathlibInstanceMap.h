#pragma once
#include "pathlib.h"

namespace PathLib
{

class CAreaDescription;
class CNavmeshAreaDescription;

class CInstanceMapCel : public Red::System::NonCopyable
{
	friend class CInstanceMap;

	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_PathLib );

protected:
	TDynArray< Uint8 >					m_data;
	TDynArray< AreaId >					m_instances;

public:
	CInstanceMapCel();
	~CInstanceMapCel();

	struct DefaultIterator : public Red::System::NonCopyable
	{
		Bool operator()( AreaId naviAreaId )											{ ASSERT( false ); return false; }

		static AreaId InvalidElement()													{ return INVALID_AREA_ID; }
	};
	template < class Functor >
	RED_INLINE AreaId			IterateAreasAt( const Vector2& v, Functor& fun ) const;
	template < class Functor >
	RED_INLINE void				IterateAreasAt( const Vector2& bbMin, const Vector2& bbMax, Functor& fun ) const;

	void						Build( CPathLibWorld& pathlib );
	void						Clear();
	
	CNavmeshAreaDescription*	GetInstanceAt( CPathLibWorld& pathlib, const Vector3& v, AreaId ignoreId ) const;
	CNavmeshAreaDescription*	GetInstanceAt( CPathLibWorld& pathlib, const Vector2& v, Float zMin, Float zMax, Float& zOut ) const;
	AreaId						GetClosestInstance( CPathLibWorld& pathlib, const Vector3& v, Float maxDistSq ) const;

	void						GetCelAt( const Vector2& v, Uint32& outAreasCount, const AreaId*& outAreas, Box2& bbox ) const;

	const TDynArray< AreaId >&	GetInstancesList() const								{ return m_instances; }
};

class CInstanceMap : public Red::System::NonCopyable
{
protected:
	CPathLibWorld&							m_pathlib;
	TDynArray< CInstanceMapCel* >			m_map;
	Bool									m_isInitialized;
	Uint32									m_cellsInRow;
	Uint32									m_cells;
	Vector2									m_cellsCorner;
	Float									m_celSize;

public:
	struct CInstanceFunctor : public Red::System::NonCopyable
	{
		// Do whatever you want with area, and than
		virtual ~CInstanceFunctor(){}
		virtual Bool Handle( CNavmeshAreaDescription* naviArea ) = 0;
	};

	CInstanceMap( CPathLibWorld& pathlib );
	~CInstanceMap();

	void						Initialize();
	void						Shutdown();
	Bool						IsInitialized() const									{ return m_isInitialized; }

	void						AddInstance( CNavmeshAreaDescription* area );
	void						RemoveInstance( CNavmeshAreaDescription* area );
	void						UpdateInstance( CNavmeshAreaDescription* area );

	const CInstanceMapCel&		GetMapCelAt( Int32 x, Int32 y ) const					{ return *m_map[ GetCelIndexFromCoords( Int16(x), Int16(y) ) ]; }

	CNavmeshAreaDescription*	GetInstanceAt( const Vector3& v, AreaId ignoreId = INVALID_AREA_ID ) const;
	CNavmeshAreaDescription*	GetInstanceAt( const Vector2& v, Float zMin, Float zMax, Float& zOut ) const;
	AreaId						GetClosestIntance( const Vector3& v, Float maxDist ) const;
	Bool						IterateAreasAt( const Vector3& v, CInstanceFunctor* it ) const;
	void						IterateAreasAt( Int32 x, Int32 y, const Box& bb, CInstanceFunctor* it ) const;
	void						IterateAreasAt( const Box& bb, CInstanceFunctor* it ) const;
	Bool						GetCelAt( const Vector2& v, Uint32& outAreasCount, const AreaId*& outAreas, Box2& bbox ) const;

	void						GetCelCoordsAtPosition( const Vector2& pos, Int16& outX, Int16& outY ) const;
	Bool						GetAndTestCelCoordsAtPosition( const Vector2& pos, Int16& outX, Int16& outY ) const;
	Uint32						GetCelIndexAtPosition( const Vector2& pos ) const		{ Int16 x, y; GetCelCoordsAtPosition( pos, x, y ); return GetCelIndexFromCoords( x, y ); }
	Uint32						GetCelIndexFromCoords( Int16 x, Int16 y ) const;
	void						GetCelCoordsFromIndex( Uint32 id, Int16& x, Int16& y ) const;

	Uint32						GetCelsInRow() const									{ return m_cellsInRow; }
};

};			// namespace PathLib

