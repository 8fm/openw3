#pragma once

#include "math.h"		// for Vector, etc.
#include "longBitField.h"

// TODO this should be moved to the math.h and fully implemented
RED_ALIGNED_STRUCT( VectorI, 16 )
{
public:
	union
	{
		struct  
		{
			Int32 X, Y, Z, W;
		};
		Int32 A[4];
	};

	VectorI()
		: X(0)
		, Y(0)
		, Z(0)
		, W(0)
	{
	}

	VectorI( Int32 iX, Int32 iY = 0, Int32 iZ = 0, Int32 iW = 0 )
		: X(iX)
		, Y(iY)
		, Z(iZ)
		, W(iW)
	{}

	VectorI( const Vector& floatVector )
		: X((Int32)floatVector.X)
		, Y((Int32)floatVector.Y)
		, Z((Int32)floatVector.Z)
		, W((Int32)floatVector.W)
	{}

	RED_INLINE Bool operator==( const VectorI& a ) const
	{
		return X == a.X && Y == a.Y && Z == a.Z && W == a.W;
	}
};

// for fast dimension masking
static Vector dimensionMasks[4] = {
	Vector(1,0,0,0),
	Vector(1,1,0,0),
	Vector(1,1,1,0),
	Vector(1,1,1,1)
};

// one element in the grid
template < typename T >
class TGridElement
{
public:
	Vector	m_Position;
	T		m_Data;

	Bool operator ==(const TGridElement& other)
	{
		return (m_Data == other.m_Data);
	}

	Bool operator ==(const TGridElement& other) const
	{
		return (m_Data == other.m_Data);
	}
};

// one cell of the grid
template < typename T, Int32 DIMENSIONS >
class TGridCell
{
private:
	//Vector m_Extents; //in case we want to shrink the bounding to the objects we will need this too but right now it would be a complication
	TDynArray< TGridElement< T > > m_Elements;

public:
	void Insert( const T& data, const Vector& position )
	{
		m_Elements.Grow();
		m_Elements[m_Elements.Size()-1].m_Position = position;
		m_Elements[m_Elements.Size()-1].m_Data = data;
	}

	const TDynArray<TGridElement<T>>& GetElements() const
	{
		return m_Elements;
	}

	void GetElements( TDynArray<TGridElement<T>>& elements ) const
	{
		if (m_Elements.Size() > 0)
		{
			for( Uint32 i = 0; i < m_Elements.Size(); ++i )
			{
				elements.PushBack(m_Elements[i]);
			}
		}
	}

	void GetElements( TDynArray<TGridElement<T>>& elements, const Vector& center, Float radius, Float cellSize ) const
	{
		if (m_Elements.Size() > 0)
		{
			Float radius2 = radius * radius;
			for( Int32 i = m_Elements.Size(); i > 0; --i )
			{
				Float axisDiff = 0.0f;
				Float dist = 0.0f;
				Vector cellPosition = m_Elements[i - 1].m_Position;
				for( Uint32 p = 0; p < DIMENSIONS; ++p )
				{
					if( center.A[p] < cellPosition.A[p] - cellSize )
					{
						axisDiff = center.A[p] - (cellPosition.A[p] - cellSize);
						dist += axisDiff * axisDiff;
					}
					else if( center.A[p] > (cellPosition.A[p] + cellSize) )
					{
						axisDiff = center.A[p] - (cellPosition.A[p] + cellSize);
						dist += axisDiff * axisDiff;
					}
				}
				if ( dist < radius2 )
				{
					elements.PushBack(m_Elements[i-1]);
				}
			}
		}
	}

	template < typename F >
	void EnumElements( const Vector& center, Float radius, Float cellSize, F& fn ) const
	{
		if ( m_Elements.Size() > 0 )
		{
			Float radius2 = radius * radius;
			for( Int32 i = m_Elements.Size(); i > 0; --i )
			{
				Float axisDiff = 0.0f;
				Float dist = 0.0f;
				Vector cellPosition = m_Elements[i - 1].m_Position;
				for( Uint32 p = 0; p < DIMENSIONS; ++p )
				{
					if( center.A[p] < cellPosition.A[p] - cellSize )
					{
						axisDiff = center.A[p] - (cellPosition.A[p] - cellSize);
						dist += axisDiff * axisDiff;
					}
					else if( center.A[p] > (cellPosition.A[p] + cellSize) )
					{
						axisDiff = center.A[p] - (cellPosition.A[p] + cellSize);
						dist += axisDiff * axisDiff;
					}
				}
				if ( dist < radius2 )
				{
					fn( m_Elements[i - 1].m_Data );
				}
			}
		}
	}

	Bool Remove( const T& data )
	{
		if (m_Elements.Size() > 0)
		{
			TGridElement<T> element;
			element.m_Data = data;
			return m_Elements.Remove(element);
		}
		else
		{
			return false;
		}
	}

	Bool Remove( const Vector& center, Float radius, Float cellSize )
	{
		if (m_Elements.Size() > 0)
		{
			Bool removed = false;
			Float radius2 = radius * radius;
			for( Int32 i = m_Elements.Size(); i > 0; --i )
			{
				Float axisDiff = 0.0f;
				Float dist = 0.0f;
				Vector cellPosition = m_Elements[i - 1].m_Position;
				for( Uint32 p = 0; p < DIMENSIONS; ++p )
				{
					if( center.A[p] < cellPosition.A[p] - cellSize )
					{
						axisDiff = center.A[p] - (cellPosition.A[p] - cellSize);
						dist += axisDiff * axisDiff;
					}
					else if( center.A[p] > (cellPosition.A[p] + cellSize) )
					{
						axisDiff = center.A[p] - (cellPosition.A[p] + cellSize);
						dist += axisDiff * axisDiff;
					}
				}
				if ( dist < radius2 )
				{
					m_Elements.RemoveAt(i-1);
					removed = true;
				}
			}
			return removed;
		}
		else
		{
			return false;
		}
	}

	void Clear()
	{
		m_Elements.Clear();
	}

	Bool Move( const T& data, const Vector& position )
	{
		if (m_Elements.Size() > 0)
		{
			for( Uint32 i = 0; i < m_Elements.Size(); ++i )
			{
				if (m_Elements[i].m_Data == data)
				{
					m_Elements[i].m_Position = position;
					return true;
				}
			}
		}
		return false;
	}

	template< typename F >
	void ForEach( F& fn )
	{
		for ( auto it=m_Elements.Begin(); it != m_Elements.End(); ++it )
		{
			TGridElement<T>& element = *it;
			fn( element.m_Data );
		}
	}
};

template < typename T, Int32 DIMENSIONS >
class TGrid
{
private:
	// D dimensional Array of grid cells allocated in one batch
	TGridCell< T, DIMENSIONS >* m_Cells;
	Uint32 m_CellCount;

	Vector m_Offsets;
	VectorI m_CellCounts;
	Vector m_CellSizes;
	Vector m_CellSizesInv; // we need these in many cases so it's better to store than divide every time
	Vector m_Multipliers;  // these are precalculated so the 1D index calculation is fast

	RED_INLINE Uint32 Calc1DIndexFromPosition( const Vector& position ) const
	{
		Vector gridPosition = position - m_Offsets;
		for (Uint32 i = 0; i < DIMENSIONS; ++i)
		{
			RED_ASSERT( gridPosition.A[i] < m_CellCounts.A[i] * m_CellSizes.A[i] );
		}

		// calc the offset of the dimension
		// this should be 4 operations
		// 1, get the cellindex by dividing position with cellsizes
		// 2, truncate the float values
		// 3, dot with multipliers vector
		// 4, convert to uint
		// here the dot is not used because this way we can do the rounding, this will be optimized
		Vector cellIndex = gridPosition * m_CellSizesInv;
		return (Uint32)( ( (Uint32)cellIndex.X * m_Multipliers.X ) + 
						 ( (Uint32)cellIndex.Y * m_Multipliers.Y ) + 
						 ( (Uint32)cellIndex.Z * m_Multipliers.Z ) + 
						 ( (Uint32)cellIndex.W * m_Multipliers.W ) );
	}

	RED_INLINE Uint32 Calc1DIndexFromCellIndex( const VectorI& cellIndex ) const
	{
		for( Uint32 i = 0; i < DIMENSIONS; ++i )
		{
			RED_ASSERT( cellIndex.A[i] < m_CellCounts.A[i] );
		}

		return (Uint32)( ( (Uint32)cellIndex.X * m_Multipliers.X ) + 
			             ( (Uint32)cellIndex.Y * m_Multipliers.Y ) + 
						 ( (Uint32)cellIndex.Z * m_Multipliers.Z ) + 
						 ( (Uint32)cellIndex.W * m_Multipliers.W ) );
	}

public:
	RED_FORCE_INLINE const VectorI& GetCellCounts()		const {	return m_CellCounts;	}
	RED_FORCE_INLINE const Vector&	GetCellSizes()		const { return m_CellSizes;		}
	RED_FORCE_INLINE const Vector&	GetCellSizesInv()	const { return m_CellSizesInv;	}
	RED_FORCE_INLINE const Vector&	GetOffsets()		const { return m_Offsets;		}
	RED_FORCE_INLINE const Vector&	GetMultipliers()	const { return m_Multipliers;	}

	RED_INLINE Vector CalcPositionFromCellIndex( const VectorI& cellIndex ) const
	{
		for( Uint32 i = 0; i < DIMENSIONS; ++i )
		{
			RED_ASSERT( cellIndex.A[i] < m_CellCounts.A[i] );
		}

		return Vector( (cellIndex.X + 0.5f) * m_CellSizes.X + m_Offsets.X, 
					   (cellIndex.Y + 0.5f) * m_CellSizes.Y + m_Offsets.Y, 
					   (cellIndex.Y + 0.5f) * m_CellSizes.Z + m_Offsets.Z, 
					   (cellIndex.W + 0.5f) * m_CellSizes.W + m_Offsets.W );
	}

	RED_INLINE VectorI CalcCellIndexFromPosition( const Vector& position ) const
	{
		Vector gridPosition = position - m_Offsets;
		for (Uint32 d = 0; d<DIMENSIONS; d++)
		{
			RED_ASSERT( gridPosition.A[d] < m_CellCounts.A[d] * m_CellSizes.A[d] );
		}
		return VectorI( (Int32) (gridPosition.A[0] * m_CellSizesInv.A[0]), 
					    (Int32) (gridPosition.A[1] * m_CellSizesInv.A[1]),
					    (Int32) (gridPosition.A[2] * m_CellSizesInv.A[2]),
					    (Int32) (gridPosition.A[3] * m_CellSizesInv.A[3]) );
	}

	RED_INLINE Bool CellIndexInBounds( const VectorI& cellIndex ) const
	{
		for (Uint32 i = 0; i < DIMENSIONS; ++i)
		{
			if ( cellIndex.A[i] < 0 || cellIndex.A[i] >= m_CellCounts.A[i] )
			{
				return false;
			}
		}
		return true;
	}

	RED_INLINE Uint16 GetUniqueTileID( const VectorI& cellIndex ) const
	{
		return static_cast<Uint16>( Calc1DIndexFromCellIndex( cellIndex ) );
	}

	RED_INLINE Bool PositionInGrid( const Vector& position ) const
	{
		Vector gridPosition = position - m_Offsets;
		for ( Uint32 i = 0; i < DIMENSIONS; ++i )
		{
			if ( gridPosition.A[i] < 0 || gridPosition.A[i] >= m_CellCounts.A[i] * m_CellSizes.A[i] )
			{
				return false;
			}
		}
		return true;
	}

	RED_INLINE Box GetBoundingBoxOfTile( const VectorI& cellIndex ) const
	{
		for( Uint32 i = 0; i < DIMENSIONS; ++i )
		{
			RED_ASSERT( cellIndex.A[i] < m_CellCounts.A[i] );
		}

		Vector minV( cellIndex.X * m_CellSizes.X + m_Offsets.X, 
			cellIndex.Y * m_CellSizes.Y + m_Offsets.Y, 
			cellIndex.Y * m_CellSizes.Z + m_Offsets.Z, 
			cellIndex.W * m_CellSizes.W + m_Offsets.W );

		Vector maxV( (cellIndex.X + 1) * m_CellSizes.X + m_Offsets.X, 
			(cellIndex.Y + 1) * m_CellSizes.Y + m_Offsets.Y, 
			(cellIndex.Y + 1) * m_CellSizes.Z + m_Offsets.Z, 
			(cellIndex.W + 1) * m_CellSizes.W + m_Offsets.W );

		return Box( minV, maxV );
	}

	typedef TDynArray<TGridElement<T>> ElementList;

	TGrid()
		: m_Cells( NULL )
		, m_CellCount( 0 )
	{
	}

	TGrid( const VectorI& cellCounts, const Vector& cellSizes )
		: m_CellCounts( cellCounts )
	{
		COMPILE_ASSERT(DIMENSIONS > 0 && DIMENSIONS < 4);

		InitCells( cellCounts, cellSizes, Vector(0.f, 0.f, 0.f, 0.f) );
	}

	TGrid( const VectorI& cellCounts, const Vector& cellSizes, const Vector& offsets )
		: m_CellCounts( cellCounts )
	{
		COMPILE_ASSERT(DIMENSIONS > 0 && DIMENSIONS < 4);

		InitCells( cellCounts, cellSizes, offsets );
	}

	~TGrid()
	{
		if ( m_Cells )
		{
			delete [] m_Cells;
		}
	}

	void InitCells( const VectorI& cellCounts, const Vector& cellSizes, const Vector& offsets )
	{
		if ( m_Cells )
		{
			delete [] m_Cells;
		}

		m_CellCounts = cellCounts;

		m_Offsets = offsets;
		m_CellSizes = Vector::ZEROS;
		m_CellSizesInv = Vector::ONES;

		m_Multipliers.SetZeros();

		m_CellCount = cellCounts.A[0];
		m_Multipliers.A[0] = 1;
		m_CellSizes.A[0] = cellSizes.A[0];
		m_CellSizesInv.A[0] /= cellSizes.A[0];
		for ( Int32 d = 1; d < DIMENSIONS; d++ )
		{
			m_Multipliers.A[d] = (Float)m_CellCount;
			m_CellCount *= cellCounts.A[d];
			m_CellSizes.A[d] = cellSizes.A[d];
			m_CellSizesInv.A[d] /= cellSizes.A[d];
		}

		RED_ASSERT(m_CellCount>0);

		m_Cells = new TGridCell< T, DIMENSIONS >[ m_CellCount ];
	}

	// use float position
	void Insert( const T& element, const Vector& position )
	{
		Uint32 index = Calc1DIndexFromPosition(position);
		RED_ASSERT( index < m_CellCount );
		m_Cells[index].Insert(element, position);
	}

	// use cell index
	void Insert( const T& element, const VectorI& cellIndex )
	{
		Uint32 index = Calc1DIndexFromCellIndex(cellIndex);
		Vector pos = CalcPositionFromCellIndex(cellIndex);
		m_Cells[index].Insert(element, pos);
	}

	// this is slow, it has to search the whole grid
	Bool Remove( const T& element )
	{
		for (Uint32 i=0; i<m_CellCount; i++)
		{
			if (m_Cells[i].Remove(element))
			{
				return true;
			}
		}
		return false;
	}

	Bool Remove( const T& element, const VectorI& cellIndex )
	{
		Uint32 index = Calc1DIndexFromCellIndex(cellIndex);
		return m_Cells[index].Remove(element);
	}

	Bool RemoveAllAroundPosition( const Vector& position, Float radius )
	{
		Bool removed = false;

		Vector gridPosition = position - m_Offsets;

		VectorI minimum( 
						(Int32)Max( (gridPosition.X - radius) * m_CellSizesInv.X * dimensionMasks[DIMENSIONS-1].X, 0.f),
						(Int32)Max( (gridPosition.Y - radius) * m_CellSizesInv.Y * dimensionMasks[DIMENSIONS-1].Y, 0.f), 
						(Int32)Max( (gridPosition.Z - radius) * m_CellSizesInv.Z * dimensionMasks[DIMENSIONS-1].Z, 0.f), 
						(Int32)Max( (gridPosition.W - radius) * m_CellSizesInv.W * dimensionMasks[DIMENSIONS-1].W, 0.f));

		VectorI maximum(
						(Int32)( dimensionMasks[DIMENSIONS-1].X * Min( ((gridPosition.X + radius) * m_CellSizesInv.X ), m_CellCounts.X - 1.f)),
						(Int32)( dimensionMasks[DIMENSIONS-1].Y * Min( ((gridPosition.Y + radius) * m_CellSizesInv.Y ), m_CellCounts.Y - 1.f)),
						(Int32)( dimensionMasks[DIMENSIONS-1].Z * Min( ((gridPosition.Z + radius) * m_CellSizesInv.Z ), m_CellCounts.Z - 1.f)),
						(Int32)( dimensionMasks[DIMENSIONS-1].W * Min( ((gridPosition.W + radius) * m_CellSizesInv.W ), m_CellCounts.W - 1.f))
						);

		for (Int32 x = minimum.X; x <= maximum.X; ++x)
		{
			for (Int32 y = minimum.Y; y <= maximum.Y; ++y)
			{
				for (Int32 z = minimum.Z; z <= maximum.Z; ++z)
				{
					for (Int32 w = minimum.W; w <= maximum.W; ++w)
					{
						Uint32 loopIndex = Calc1DIndexFromCellIndex(VectorI(x,y,z,w));
						removed |= m_Cells[loopIndex].Remove( position, radius, m_CellSizes.X );
					}
				}
			}
		}

		return removed;
	}

	void GetBitMaskAroundPosition( const Vector& position, Float radius, LongBitField& bitField ) const
	{
		RED_ASSERT( bitField.Size() * ( sizeof( Uint32 ) * 8 )  >= m_CellCount, TXT("Bit array isn't large enough to fit a bit to each cell position. This Grid requires a bitfield of size: %d"), m_CellCount );
		Vector gridPosition = position - m_Offsets;
		
		VectorI minimum( 
			(Int32)Max( (gridPosition.X - radius) * m_CellSizesInv.X * dimensionMasks[DIMENSIONS-1].X, 0.f),
			(Int32)Max( (gridPosition.Y - radius) * m_CellSizesInv.Y * dimensionMasks[DIMENSIONS-1].Y, 0.f), 
			(Int32)Max( (gridPosition.Z - radius) * m_CellSizesInv.Z * dimensionMasks[DIMENSIONS-1].Z, 0.f), 
			(Int32)Max( (gridPosition.W - radius) * m_CellSizesInv.W * dimensionMasks[DIMENSIONS-1].W, 0.f));

		VectorI maximum(
			(Int32)( dimensionMasks[DIMENSIONS-1].X * Min( ((gridPosition.X + radius) * m_CellSizesInv.X ), m_CellCounts.X - 1.f)),
			(Int32)( dimensionMasks[DIMENSIONS-1].Y * Min( ((gridPosition.Y + radius) * m_CellSizesInv.Y ), m_CellCounts.Y - 1.f)),
			(Int32)( dimensionMasks[DIMENSIONS-1].Z * Min( ((gridPosition.Z + radius) * m_CellSizesInv.Z ), m_CellCounts.Z - 1.f)),
			(Int32)( dimensionMasks[DIMENSIONS-1].W * Min( ((gridPosition.W + radius) * m_CellSizesInv.W ), m_CellCounts.W - 1.f))
			);
		for (Int32 w = minimum.W; w <= maximum.W; ++w)
		{
			for (Int32 z = minimum.Z; z <= maximum.Z; ++z)
			{
				for (Int32 y = minimum.Y; y <= maximum.Y; ++y)
				{
					for (Int32 x = minimum.X; x <= maximum.X; ++x)
					{
						Uint32 loopIndex = Calc1DIndexFromCellIndex( VectorI( x, y, z, w ) );
						Float radius2 = radius * radius;
						Float axisDiff = 0.0f;
						Float dist = 0.0f;
						Vector cellPosition = CalcPositionFromCellIndex( VectorI( x, y, z, w ) );
						for( Uint32 p = 0; p < DIMENSIONS; ++p )
						{
							if( position.A[p] < cellPosition.A[p] - m_CellCounts.X )
							{
								axisDiff = position.A[p] - (cellPosition.A[p] - m_CellCounts.X);
								dist += axisDiff * axisDiff;
							}
							else if( position.A[p] > (cellPosition.A[p] + m_CellCounts.X) )
							{
								axisDiff = position.A[p] - (cellPosition.A[p] + m_CellCounts.X);
								dist += axisDiff * axisDiff;
							}
						}
						if ( dist < radius2 )
						{
							bitField.SetBit( loopIndex, true );
						}
					}
				}
			}
		}
	}

	void GetElementsAroundPosition( const Vector& position, Float radius, ElementList& elements ) const
	{
		elements.ClearFast();

		Vector gridPosition = position - m_Offsets;

		VectorI minimum( 
			(Int32)Max( (gridPosition.X - radius) * m_CellSizesInv.X * dimensionMasks[DIMENSIONS-1].X, 0.f),
			(Int32)Max( (gridPosition.Y - radius) * m_CellSizesInv.Y * dimensionMasks[DIMENSIONS-1].Y, 0.f), 
			(Int32)Max( (gridPosition.Z - radius) * m_CellSizesInv.Z * dimensionMasks[DIMENSIONS-1].Z, 0.f), 
			(Int32)Max( (gridPosition.W - radius) * m_CellSizesInv.W * dimensionMasks[DIMENSIONS-1].W, 0.f));

		VectorI maximum(
			(Int32)( dimensionMasks[DIMENSIONS-1].X * Min( ((gridPosition.X + radius) * m_CellSizesInv.X ), m_CellCounts.X - 1.f)),
			(Int32)( dimensionMasks[DIMENSIONS-1].Y * Min( ((gridPosition.Y + radius) * m_CellSizesInv.Y ), m_CellCounts.Y - 1.f)),
			(Int32)( dimensionMasks[DIMENSIONS-1].Z * Min( ((gridPosition.Z + radius) * m_CellSizesInv.Z ), m_CellCounts.Z - 1.f)),
			(Int32)( dimensionMasks[DIMENSIONS-1].W * Min( ((gridPosition.W + radius) * m_CellSizesInv.W ), m_CellCounts.W - 1.f))
			);
		for (Int32 w = minimum.W; w <= maximum.W; ++w)
		{
			for (Int32 z = minimum.Z; z <= maximum.Z; ++z)
			{
				for (Int32 y = minimum.Y; y <= maximum.Y; ++y)
				{
					for (Int32 x = minimum.X; x <= maximum.X; ++x)
					{
						Uint32 loopIndex = Calc1DIndexFromCellIndex(VectorI(x,y,z,w));
						m_Cells[loopIndex].GetElements( elements, position, radius, m_CellSizes.X );
					}
				}
			}
		}
	}

	template < typename F >
	void EnumElementsAroundPosition( const Vector& position, Float radius, F& fn ) const
	{
		Vector gridPosition = position - m_Offsets;

		VectorI minimum( 
			(Int32)Max( (gridPosition.X - radius) * m_CellSizesInv.X * dimensionMasks[DIMENSIONS-1].X, 0.f),
			(Int32)Max( (gridPosition.Y - radius) * m_CellSizesInv.Y * dimensionMasks[DIMENSIONS-1].Y, 0.f), 
			(Int32)Max( (gridPosition.Z - radius) * m_CellSizesInv.Z * dimensionMasks[DIMENSIONS-1].Z, 0.f), 
			(Int32)Max( (gridPosition.W - radius) * m_CellSizesInv.W * dimensionMasks[DIMENSIONS-1].W, 0.f));

		VectorI maximum(
			(Int32)( dimensionMasks[DIMENSIONS-1].X * Min( ((gridPosition.X + radius) * m_CellSizesInv.X ), m_CellCounts.X - 1.f)),
			(Int32)( dimensionMasks[DIMENSIONS-1].Y * Min( ((gridPosition.Y + radius) * m_CellSizesInv.Y ), m_CellCounts.Y - 1.f)),
			(Int32)( dimensionMasks[DIMENSIONS-1].Z * Min( ((gridPosition.Z + radius) * m_CellSizesInv.Z ), m_CellCounts.Z - 1.f)),
			(Int32)( dimensionMasks[DIMENSIONS-1].W * Min( ((gridPosition.W + radius) * m_CellSizesInv.W ), m_CellCounts.W - 1.f))
			);
		for (Int32 w = minimum.W; w <= maximum.W; ++w)
		{
			for (Int32 z = minimum.Z; z <= maximum.Z; ++z)
			{
				for (Int32 y = minimum.Y; y <= maximum.Y; ++y)
				{
					for (Int32 x = minimum.X; x <= maximum.X; ++x)
					{
						Uint32 loopIndex = Calc1DIndexFromCellIndex(VectorI(x,y,z,w));
						m_Cells[loopIndex].template EnumElements<F>( position, radius, m_CellSizes.X, fn );
					}
				}
			}
		}
	}

	void GetElementsFromMasks( const Vector& position, const Float radius, const LongBitField& bitMask, ElementList& elements ) const
	{
		elements.Clear();

		Vector gridPosition = position - m_Offsets;

		VectorI minimum( 
			(Int32)Max( (gridPosition.X - radius) * m_CellSizesInv.X * dimensionMasks[DIMENSIONS-1].X, 0.f),
			(Int32)Max( (gridPosition.Y - radius) * m_CellSizesInv.Y * dimensionMasks[DIMENSIONS-1].Y, 0.f), 
			(Int32)Max( (gridPosition.Z - radius) * m_CellSizesInv.Z * dimensionMasks[DIMENSIONS-1].Z, 0.f), 
			(Int32)Max( (gridPosition.W - radius) * m_CellSizesInv.W * dimensionMasks[DIMENSIONS-1].W, 0.f));

		VectorI maximum(
			(Int32)( dimensionMasks[DIMENSIONS-1].X * Min( ((gridPosition.X + radius) * m_CellSizesInv.X ), m_CellCounts.X - 1.f)),
			(Int32)( dimensionMasks[DIMENSIONS-1].Y * Min( ((gridPosition.Y + radius) * m_CellSizesInv.Y ), m_CellCounts.Y - 1.f)),
			(Int32)( dimensionMasks[DIMENSIONS-1].Z * Min( ((gridPosition.Z + radius) * m_CellSizesInv.Z ), m_CellCounts.Z - 1.f)),
			(Int32)( dimensionMasks[DIMENSIONS-1].W * Min( ((gridPosition.W + radius) * m_CellSizesInv.W ), m_CellCounts.W - 1.f))
			);
		for (Int32 w = minimum.W; w <= maximum.W; ++w)
		{
			for (Int32 z = minimum.Z; z <= maximum.Z; ++z)
			{
				for (Int32 y = minimum.Y; y <= maximum.Y; ++y)
				{
					for (Int32 x = minimum.X; x <= maximum.X; ++x)
					{
						Uint32 loopIndex = Calc1DIndexFromCellIndex(VectorI(x,y,z,w));
						if( bitMask.IsBitSet( loopIndex ) )
						{	
							m_Cells[loopIndex].GetElements( elements );//, position, radius, m_CellSizes.X );
						}
					}
				}
			}
		}
	}

	void GetElementsAroundPositionRect( const Vector& position, Float radius, ElementList& elements ) const
	{
		elements.Clear();

		Vector gridPosition = position - m_Offsets;

		VectorI minimum( 
			(Int32)Max( (gridPosition.X - radius) * m_CellSizesInv.X * dimensionMasks[DIMENSIONS-1].X, 0.f),
			(Int32)Max( (gridPosition.Y - radius) * m_CellSizesInv.Y * dimensionMasks[DIMENSIONS-1].Y, 0.f), 
			(Int32)Max( (gridPosition.Z - radius) * m_CellSizesInv.Z * dimensionMasks[DIMENSIONS-1].Z, 0.f), 
			(Int32)Max( (gridPosition.W - radius) * m_CellSizesInv.W * dimensionMasks[DIMENSIONS-1].W, 0.f));

		VectorI maximum(
			(Int32)( dimensionMasks[DIMENSIONS-1].X * Min( ((gridPosition.X + radius) * m_CellSizesInv.X ), m_CellCounts.X - 1.f)),
			(Int32)( dimensionMasks[DIMENSIONS-1].Y * Min( ((gridPosition.Y + radius) * m_CellSizesInv.Y ), m_CellCounts.Y - 1.f)),
			(Int32)( dimensionMasks[DIMENSIONS-1].Z * Min( ((gridPosition.Z + radius) * m_CellSizesInv.Z ), m_CellCounts.Z - 1.f)),
			(Int32)( dimensionMasks[DIMENSIONS-1].W * Min( ((gridPosition.W + radius) * m_CellSizesInv.W ), m_CellCounts.W - 1.f))
			);

		for (Int32 x = minimum.X; x <= maximum.X; ++x)
		{
			for (Int32 y = minimum.Y; y <= maximum.Y; ++y)
			{
				for (Int32 z = minimum.Z; z <= maximum.Z; ++z)
				{
					for (Int32 w = minimum.W; w <= maximum.W; ++w)
					{
						Uint32 loopIndex = Calc1DIndexFromCellIndex(VectorI(x,y,z,w));
						m_Cells[loopIndex].GetElements( elements );
					}
				}
			}
		}
	}

	const ElementList& GetElementsByPosition( const Vector& position ) const
	{
		Uint32 index = Calc1DIndexFromPosition(position);
		RED_FATAL_ASSERT( index < m_CellCount, "Cell index out of range, crashing to avoid memory stomp" );
		return m_Cells[index].GetElements();
	}

	const ElementList& GetElementsByCellIndex( const VectorI& cellIndex ) const
	{
		Uint32 index = Calc1DIndexFromCellIndex(cellIndex);
		RED_FATAL_ASSERT( index < m_CellCount, "Cell index out of range, crashing to avoid memory stomp" );
		return m_Cells[index].GetElements();
	}

	void GetAllElements( ElementList& elements ) const
	{
		for (Uint32 i = 0; i < m_CellCount; ++i)
		{
			m_Cells[i].GetElements(elements);
		}
	}

	void Clear()
	{
		for (Uint32 i = 0; i < m_CellCount; ++i)
		{
			m_Cells[i].Clear();
		}
	}

	void ClearCell( const VectorI& cellIndex )
	{
		Uint32 index = Calc1DIndexFromCellIndex( cellIndex );
		RED_ASSERT( index < m_CellCount );
		m_Cells[ index ].Clear();
	}

	Bool Move( const T& element, const Vector& position )
	{
		//Try if it's the same cell and move inside the cell
		Uint32 index = Calc1DIndexFromPosition(position);
		if ( !m_Cells[index].Move(element, position) )
		{
			//If it didn't work than we have to remove and add
			Remove(element);
			m_Cells[index].Insert(element, position);
		}
		return true;
	}

	Bool Move( const T& element, const VectorI& cellIndex )
	{
		//Try if it's the same cell and move inside the cell
		Uint32 index = Calc1DIndexFromCellIndex(cellIndex);
		Vector position = CalcPositionFromCellIndex(cellIndex);
		if ( !m_Cells[index].Move(element, position) )
		{
			//If it didn't work than we have to remove and add
			Remove(element);
			m_Cells[index].Insert(element, position);
		}
		return true;
	}

	template< typename F >
	void ForEach( F& fn )
	{
		for ( Uint32 i = 0; i < m_CellCount; ++i )
		{
			m_Cells[i].template ForEach<F>( fn );
		}
	}
};
