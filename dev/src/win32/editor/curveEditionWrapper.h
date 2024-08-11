/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/core/curveData.h"

#define APPROXIMATED_CURVE_EVALUATION

/************************************************************************/
/*	This class is SCurveData wrapper. It's used it make edition of SCurveData easier.
	Use control points obtained from CCurveData to edit wrapped SCurveData.
	Under no circumstances use more then one wrapper on SCurveData. */
/************************************************************************/

class CCurveEditionWrapper
{
public:
	class ControlPoint
	{
		DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

	public:

		enum ECurveDir
		{
			CD_In = 0,
			CD_Out = 1
		};

	protected:
		Uint32					m_index;
		SCurveData&				m_data;
		CCurveEditionWrapper*	m_curve;

	public:
		ControlPoint( Uint32 index, SCurveData& data, CCurveEditionWrapper* curve )
			: m_index( index )
			, m_data( data)
			, m_curve( curve )
		{}

		void		SetTangentType( const Int32& tangentIndex, ECurveSegmentType type );
		ECurveSegmentType	GetTangentType( const Int32& tangentIndex ) const;
		void		SetTangentValue( const Int32& tangentIndex, const Vector& tangentValue );
		Vector		GetTangentValue( const Int32& tangentIndex ) const { return m_data.GetTangentValue( m_index, tangentIndex ); }
		void		SetTime( const Float time );
		Float		GetTime() const { return m_data.m_curveValues[ m_index ].time; }
		void		SetValue( const Float value ){ m_data.m_curveValues[ m_index ].value = value; }
		Float		GetValue() const { return m_data.m_curveValues[ m_index ].value; }
		void		SetTimeValue( const Float time, const Float value ) { m_data.m_curveValues[ m_index ].time = time; m_data.m_curveValues[ m_index ].value = value; }
		CCurveEditionWrapper*	GetCurve() const { return m_curve; }

		friend IFile& operator<<( IFile& file, ControlPoint& c );
		friend class CCurveEditionWrapper;
	};

protected:
	Color								m_color;
	TDynArray< CCurveEditionWrapper::ControlPoint* >	m_controlPoints;
	SCurveData&							m_data;
	void*								m_userData;

	void CreateControlPoints();
public:
	CCurveEditionWrapper& operator= (const CCurveEditionWrapper& other)
	{
		if ( this != &other )
		{
			m_color = other.m_color;
			m_controlPoints = other.m_controlPoints;
			m_data = other.m_data;
			m_controlPoints = other.m_controlPoints;
		}
		return *this;
	}

	CCurveEditionWrapper( SCurveData& data, const Color& color = Color( 255, 255, 255 ) );
	CCurveEditionWrapper( SCurveData& data, const Float value, const Color& color = Color( 255, 255, 255 ) );
	virtual ~CCurveEditionWrapper();

	void SetUserData( void* userData ) { m_userData = userData; }
	void* GetUserData() { return m_userData; }

	Float					GetValue( const Float& time ) const;
	Color					GetColor() const { return m_color; }
	void					SetColor( const Color& color ) { m_color = color; }

	const TDynArray< CCurveEditionWrapper::ControlPoint* >&	GetControlPoints() { return m_controlPoints; }
	const TDynArray< CCurveEditionWrapper::ControlPoint* >&	GetControlPoints() const { return m_controlPoints; }
	CCurveEditionWrapper::ControlPoint*	AddControlPoint( const Float time, const Float value, const ECurveSegmentType& curveType = CST_Interpolate, bool addToCurve = true );
	Bool					RemoveControlPoint( CCurveEditionWrapper::ControlPoint* controlPoint, bool removeFromCurve = true );
	void					SortControlPoints();
	void					Clear();

	Int32						GetControlPointsIndex( const CCurveEditionWrapper::ControlPoint* controlPoint ) const
	{
		for( Uint32 i = 0; i < m_controlPoints.Size(); ++i )
		{
			if ( m_controlPoints[i] == controlPoint )
			{
				return i;
			}
		}
		return -1;
	}
	RED_INLINE SCurveData&	GetCurveData() { return m_data; }

	void RemapControlPoints( Float oldRangeMin, Float oldRangeMax, Float newRangeMin, Float newRangeMax );
	Bool GetValuesMinMax( Float& minValue, Float& maxValue ) const;
	Bool GetTimesMinMax( Float& minValue, Float& maxValue ) const;

	void GetApproximationSamples( TDynArray< Float >& samples ) const;
	void GetApproximationSamples( Float* samples ) const;
};


struct CCurveControlPointCompareFunc
{
	RED_INLINE Bool operator()( const CCurveEditionWrapper::ControlPoint* key1, const CCurveEditionWrapper::ControlPoint* key2 ) const
	{
		ASSERT( key1 && key2 );
		return key1->GetTime() < key2->GetTime();
	}
};
