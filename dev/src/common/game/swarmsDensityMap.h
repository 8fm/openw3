
#pragma once

//this class holds num elements per cell in aabb area
template <class T>
class CSwarmDensityMap
{
public:
	CSwarmDensityMap( const Vector & aabbMin, const Vector & aabbMax, Int32 w, Int32 h )
		: m_min( aabbMin.AsVector2() )
		, m_max( aabbMax.AsVector2() )
	{

		Vector2 diff = m_max - m_min;
		m_width = w;
		m_height = h;
		m_celSize.X = diff.X / Float(w);
		m_celSize.Y = diff.Y / Float(h);
		m_data = new T[m_width*m_height];
	}
	CSwarmDensityMap( const Vector2 & aabbMin, const Vector2 & aabbMax, Int32 w, Int32 h )
		: m_min( aabbMin )
		, m_max( aabbMax )
	{
		
		Vector2 diff = m_max - m_min;
		m_width = w;
		m_height = h;
		m_celSize.X = diff.X / Float(w);
		m_celSize.Y = diff.Y / Float(h);
		m_data = new T[m_width*m_height];
	}
	~CSwarmDensityMap()
	{
		delete [] m_data;
	}

	RED_INLINE Float GetMinX()
	{
		return m_min.X;
	}
	RED_INLINE Float GetMaxX()
	{
		return m_max.X;
	}
	RED_INLINE Float GetMinY()
	{
		return m_min.Y;
	}
	RED_INLINE Float GetMaxY()
	{
		return m_max.Y;
	}

	RED_INLINE void Clear()
	{
		Red::System::MemorySet(m_data,0,sizeof(T)*m_width*m_height);
	}
	RED_INLINE const T & GetChannel( Int32 x, Int32 y ) const
	{
		return m_data[ (m_width*y)+x ]; 
	}
	RED_INLINE const T* GetChannel( const Vector & Pos ) const		{ return GetChannel( Pos.AsVector2() ); }
	RED_INLINE const T* GetChannel( const Vector2 & Pos ) const
	{
		Int32 x;
		Int32 y;
		CellAdress( Pos, x, y );
		if( IsCorrectCell( x, y ))
		{
			return &GetChannel( x, y );
		}
		else
		{
			return NULL;
		}
	}
	RED_INLINE T & GetChannel( Int32 x, Int32 y )
	{
		return m_data[ (m_width*y)+x ]; 
	}
	RED_INLINE T* GetChannel( const Vector & Pos )					{ return GetChannel( Pos.AsVector2() ); }
	RED_INLINE T* GetChannel( const Vector2 & Pos )
	{
		Int32 x;
		Int32 y;
		CellAdress( Pos, x, y );
		if( IsCorrectCell( x, y ))
		{
			return &GetChannel( x, y );
		}
		else
		{
			return NULL;
		}
	}
	RED_INLINE Bool IsCorrectCell( Int32 x, Int32 y ) const
	{
		if ( x < 0 )			{return false;}
		if ( x >= m_width )		{return false;}
		if ( y < 0 )			{return false;}
		if ( y >= m_height )	{return false;}
		return true;
	}
	RED_INLINE Int32 GetWidth() const									{ return m_width; }
	RED_INLINE Int32 GetHeight() const								{ return m_height; }
	RED_INLINE const Vector2& GetCelSize() const						{ return m_celSize; }

	RED_INLINE Vector2 GetCelWorldPositionFromCoordinates( Int32 x, Int32 y )const
	{
		return
			Vector2(
				m_min.X + ((Float(x)+0.5f) * m_celSize.X),
				m_min.Y + ((Float(y)+0.5f) * m_celSize.Y)
			);
	}

	RED_INLINE void GetCellCoordinatesFromWorldPosition( const Vector2 &worldPosition, Int32 &x, Int32 &y  )const
	{
		const Vector2 localPos = worldPosition - m_min;
		x = (Int32)(localPos.X / m_celSize.X);
		y = (Int32)(localPos.Y / m_celSize.Y);
	}

	RED_INLINE void WorldToLocal( Vector2& worldPos ) const
	{
		worldPos -= m_min;
	}

	RED_INLINE Vector2 NormalAabbVector( const Vector2 & pos ) const // smaller values then 0 or larger than 1 are outside aabb)
	{
		//delta.x and delta.y should allways be != 0 since its aabb
		Vector delta = m_max-m_min;
		Vector diff = pos-m_min;
		return Vector2( diff.X/delta.X, diff.Y/delta.Y ); //z is unimportant
	}

	RED_INLINE void CellAdress( const Vector2& pos, Int32 & x, Int32 & y ) const //values might be outside of range, less than zero or bigger then dimentions
	{
		Vector2 norm = NormalAabbVector( pos ); //this is 0-1 aabb coordinates
		norm.X *= m_width;
		norm.Y *= m_height;
		x = (Int32)norm.X;
		y = (Int32)norm.Y;
	}

	
	Float ComputeZ( const Vector2& pos ) const
	{
		//const CFlyingCrittersAlgorithmData::EnviromentData* enviroment = m_algorithmData->GetEnviroment();

		Vector2 normVec = NormalAabbVector( pos );
		normVec.X		*= Float( GetWidth() );
		normVec.Y		*= Float( GetHeight() );

		Int32 coordXLow		= Int32( normVec.X - 0.5f );
		Int32 coordYLow		= Int32( normVec.Y - 0.5f );
		Int32 coordXHigh	= Int32( normVec.X + 0.5f );
		Int32 coordYHigh	= Int32( normVec.Y + 0.5f );
		coordXLow		= Max( 0, coordXLow );
		coordYLow		= Max( 0, coordYLow );
		coordXHigh		= Min( GetWidth() - 1, coordXHigh );
		coordYHigh		= Min( GetHeight() - 1, coordYHigh );

		// TODO: Check if algorithm produces smooth output on quad edges
		Float xRatio	= normVec.X - 0.5f;
		xRatio			= xRatio - floor( xRatio );

		Float yRatio	= normVec.Y - 0.5f;
		yRatio			= yRatio - floor( yRatio );

	
		const Float zComponentLL = GetChannel( coordXLow, coordYLow ).m_z;
		const Float zComponentHL = GetChannel( coordXHigh, coordYLow ).m_z;
		const Float zComponentLH = GetChannel( coordXLow, coordYHigh ).m_z;
		const Float zComponentHH = GetChannel( coordXHigh, coordYHigh ).m_z;

		const Float zComponentL = zComponentLL + ( zComponentLH - zComponentLL ) * yRatio;
		const Float zComponentH = zComponentHL + ( zComponentHH - zComponentHL ) * yRatio;

		return zComponentL + ( zComponentH - zComponentL ) * xRatio;
	}

	Bool TestLocation( const Vector2& dest ) const
	{
		Int32 fieldX, fieldY;
		CellAdress( dest, fieldX, fieldY );
		if ( !IsCorrectCell( fieldX, fieldY ) )
		{
			return false;
		}
		const auto* fieldData = &GetChannel( fieldX, fieldY );
		return !fieldData->m_isBlocked;
	}

private:

	//values
	T* m_data;
	Vector2 m_min;
	Vector2 m_max;
	Vector2 m_celSize;
	Int32 m_width;
	Int32 m_height;
};