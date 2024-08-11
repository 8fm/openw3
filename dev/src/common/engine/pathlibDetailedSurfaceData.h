/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/intsBitField.h"

#include "pathlibConst.h"

namespace PathLib
{

class CDetailedSurfaceCollection;
class CTerrainInfo;

// Generation only extended data used for surface processing
class CDetailedSurfaceData
{
public:

	RED_ALIGNED_CLASS( FieldData, 1 )
	{
		friend class CDetailedSurfaceData;
	public:
		enum ESurfaceState : Uint8
		{
			ESURFACE_Ok,
			ESURFACE_Slope,
			ESURFACE_Underwater
		};

		static FieldData	FromInt( Uint32 i )											{ FieldData f = reinterpret_cast< FieldData& >( i ); return f; }
		Uint32				ToInt() const												{ union { Uint32 i; FieldData d; } u; u.i = 0; u.d = *this; return u.i; }

		ESurfaceState		m_surface													: 2;
		Bool				m_isMarkedByInstance										: 1;
		Bool				m_isInstanceConnection										: 1;
		Bool				m_isSmoothed												: 1;
	};
private:
	typedef TDynArray< FieldData > TData;

	TData					m_data;
	Uint32					m_resolution;

	Uint32					GetCelIndex( Int32 x, Int32 y ) const						{ return y*m_resolution + x; }
public:

	CDetailedSurfaceData();

	void					Initialize( Uint32 resolution );

	FieldData				GetField( Int32 x, Int32 y ) const							{ return m_data[ GetCelIndex( x, y ) ]; };
	void					SetField( Int32 x, Int32 y, FieldData f )					{ m_data[ GetCelIndex( x, y ) ] = f; }

	void					ClearProcessingFlag();

	static void				GlobalSmoothOutProcess( CPathLibWorld& pathlib, CDetailedSurfaceCollection* cookerData );
};

class CDetailedSurfaceCollection
{
protected:
	TDynArray< CDetailedSurfaceData >	m_surfaceData;
public:
	CDetailedSurfaceData*	GetSurface( AreaId areaId );

	void					Initialize( CPathLibWorld& pathlib );
	void					Clear();
};


};			// namespace PathLib