
#pragma once

template< Int32 D >
class FloatPointSet
{
	TDynArray< Float >	m_points;

public:
	void AddPoint( Float x );
	void AddPoint( Float x, Float y );
	void AddPoint( Float x, Float y, Float z );

public:
	RED_INLINE const Float* operator[] ( Int32 i ) const
	{	
		ASSERT( D*i < (Int32)m_points.Size() );
		return &(m_points[ D*i ]);
	}

	RED_INLINE Float* Resize( Uint32 numPoints )
	{	
		m_points.Resize( numPoints * D );
		return m_points.TypedData();
	}

	Int32 GetPointDim() const
	{
		return D;
	}

	Int32 GetPointNum() const
	{
		return (Int32)m_points.Size() / D;
	}

	void Clear()
	{
		m_points.Clear();
	}
};

/*class EntityPointSet
{
	TDynArray< CEntity >	m_entities;

public:
	RED_INLINE const Float* operator[] ( Int32 i ) const
	{	
		ASSERT( i < (Int32)m_entities.Size() );
		return &(m_entities[ i ].GetPosition());
	}

	Int32 GetPointDim() const
	{
		return 3;
	}

	Int32 GetPointNum() const
	{
		return (Int32)m_entities.Size();
	}

	void Clear()
	{
		m_entities.Clear();
	}
};*/

template<> 
RED_INLINE void FloatPointSet< 1 >::AddPoint( Float x )
{
	m_points.PushBack( x );
}

template<>
RED_INLINE void FloatPointSet< 2 >::AddPoint( Float x, Float y )
{
	m_points.PushBack( x );
	m_points.PushBack( y );
}

template<>
RED_INLINE void FloatPointSet< 3 >::AddPoint( Float x, Float y, Float z )
{
	m_points.PushBack( x );
	m_points.PushBack( y );
	m_points.PushBack( z );
}

class FloatPtrPointSet
{
	TDynArray< Float* >	m_points;

public:
	void AddPoint( Float* p )
	{
		m_points.PushBack( p );
	}

public:
	RED_INLINE const Float* operator[] ( Int32 i ) const
	{	
		ASSERT( i < (Int32)m_points.Size() );
		return m_points[ i ];
	}

	Int32 GetPointDim() const
	{
		return 2;
	}

	Int32 GetPointNum() const
	{
		return (Int32)m_points.Size();
	}

	void Clear()
	{
		m_points.Clear();
	}
};
