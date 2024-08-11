/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "curve.h"
#include "../core/scriptStackFrame.h"
#include "../core/events.h"
#include "../core/xmlFile.h"

IMPLEMENT_ENGINE_CLASS( CCurve );

CCurve::CCurve( const Color& color )
	: SCurveBase( CVT_Float, false )
	, m_color( color )
{
	SetUseDayTime( false );

	SetCurveType( CT_Segmented );
}

CCurve::CCurve( const Float value, const Color& color )
	: SCurveBase( CVT_Float, false )
	, m_color( color )
{
	SetUseDayTime( false );

	SetCurveType( CT_Segmented );
	AddPoint( 0.0f, value, CST_Constant );
}

void CCurve::OnSerialize( IFile& file )
{
 	TBaseClass::OnSerialize( file );

	// Control points are serialized manually. The rest can be done with RTTI properties.
	if ( file.IsWriter() )
	{
		Uint32 numControlPoints = m_data.Size();
		file << numControlPoints;
		for( Uint32 i = 0; i < numControlPoints; ++i )
		{
			file << m_data.m_curveValues[i].time;
			file << m_data.m_curveValues[i].value;
			Vector c0 =  m_data.GetTangentValue( i, 0 );
			file << c0;
			Vector c1 =  m_data.GetTangentValue( i, 1 );
			file << c1;
			Int32 curveType = m_data.m_curveValues[i].curveTypeL; 
			file << curveType;
			curveType = m_data.m_curveValues[i].curveTypeR;
			file << curveType;
		}
	}
	else if ( file.IsReader() )
	{
		Clear();
		Uint32 numControlPoints;
		file << numControlPoints;

		m_data.m_curveValues.Resize( numControlPoints );
		
		for( Uint32 i = 0; i < numControlPoints; ++i )
		{
			file << m_data.m_curveValues[i].time;
			file << m_data.m_curveValues[i].value;
			Vector c0;
			file << c0;
			m_data.SetTangentValue( i, 0, c0 );
			Vector c1;
			file << c1;
			m_data.SetTangentValue( i, 1, c1 );
			Int32 t;
			file << t;
			m_data.m_curveValues[i].curveTypeL = (ECurveSegmentType)t;
			file << t;
			m_data.m_curveValues[i].curveTypeR = (ECurveSegmentType)t;
		}
	}
}

void CCurve::OnSerializeXML( IXMLFile& file )
{
    file.BeginNode( TXT("curve") );

    if ( file.IsReader() )
    {
		SetCurveType( CT_Segmented );

        String colorString;
        String pointsCount;
		String curveTypeString;

		if ( file.Attribute( TXT("baseType"), curveTypeString ) )
		{
			Int32 baseType;
			if ( ::FromString< Int32 >( curveTypeString, baseType ) )
			{
				SetCurveType( (ECurveBaseType)baseType );
			}
		}

        file.Attribute( TXT("color"), colorString );
        file.Attribute( TXT("count"), pointsCount );

        ::FromString< Color >( colorString, m_color );
        Uint32 numberOfPoints = 0;
        ::FromString< Uint32 >( pointsCount, numberOfPoints );

        for ( Uint32 i = 0; i < numberOfPoints; ++i )
        {
            String timeString, valueString, type0String, value0String, type1String, value1String;
            file.BeginNode( TXT("point") );
            file.Attribute( TXT("time"), timeString );
            file.Attribute( TXT("value"), valueString );
            file.BeginNode( TXT("tangent0") );
            file.Attribute( TXT("type"), type0String );
            file.Attribute( TXT("value"), value0String );
            file.EndNode();
            file.BeginNode( TXT("tangent1") );
            file.Attribute( TXT("type"), type1String );
            file.Attribute( TXT("value"), value1String );
            file.EndNode();
            file.EndNode();

            Float time, value;
            ECurveSegmentType type0 = CST_Interpolate, type1 = CST_Interpolate;
            Vector value0, value1;
            ::FromString< Float >( timeString, time );
            ::FromString< Float >( valueString, value );
            ::FromString< Int32 >( type0String, *((Int32 *)&type0) );
            ::FromString< Vector >( value0String, value0 );
            ::FromString< Int32 >( type1String, *((Int32 *)&type1) );
            ::FromString< Vector >( value1String, value1 );

            Uint32 index = AddPoint( time, value, type0 );
			SetTangentType( index, 0, type0 );
			SetTangentValue( index, 0, value0 );
			SetTangentType( index, 1, type1 );
			SetTangentValue( index, 1, value1 );
        }
    }
    else
    {
		String curveTypeString = ::ToString< Int32 >( (Int32)GetCurveType() );
        String colorString = ::ToString< Color >( m_color );
        String pointsCount = ::ToString< Uint32 >( m_data.Size() );

		file.Attribute( TXT("baseType"), curveTypeString );
        file.Attribute( TXT("color"), colorString );
        file.Attribute( TXT("count"), pointsCount );

        for ( Uint32 i = 0; i < m_data.Size(); ++i )
        {
            String time = ::ToString< Float >( m_data.m_curveValues[i].time );
            String value = ::ToString< Float >( m_data.m_curveValues[i].value );
            String type0 = ::ToString< Int32 >( (Int32)m_data.m_curveValues[i].curveTypeL );
            String value0 = ::ToString< Vector >( m_data.GetTangentValue( i, 0 ) );
            String type1 = ::ToString< Int32 >( (Int32)m_data.m_curveValues[i].curveTypeR );
            String value1 = ::ToString< Vector >( m_data.GetTangentValue( i, 1 ) );

            file.BeginNode( TXT("point") );
            file.Attribute( TXT("time"), time );
            file.Attribute( TXT("value"), value );
            file.BeginNode( TXT("tangent0") );
            file.Attribute( TXT("type"), type0 );
            file.Attribute( TXT("value"), value0 );
            file.EndNode();
            file.BeginNode( TXT("tangent1") );
            file.Attribute( TXT("type"), type1 );
            file.Attribute( TXT("value"), value1 );
            file.EndNode();
            file.EndNode();
        }
    }

    file.EndNode();
}

CCurve::~CCurve()
{
	// Delete control points table
}


void CCurve::funcGetValue( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, time, 0.f );
	FINISH_PARAMETERS;

	const Float val = GetFloatValue( time );

	RETURN_FLOAT( val );
}

void CCurve::funcGetDuration( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float min, max = 0.f;
	
	GetTimesMinMax( min, max );

	RETURN_FLOAT( max );
}

//----

CurveApproximation::CurveApproximation()
{
	// TODO: should we care about clearing samples or not ?
	for ( Uint32 i=0; i<NUM_SAMPLES; ++i )
		m_samples[i] = 0.0f;
}

void CurveApproximation::BuildFrom( const CCurve* sourceData )
{
	sourceData->GetApproximationSamples( m_samples, NUM_SAMPLES );
}

void CurveApproximation::Serialize( IFile& ar )
{
	if ( ar.IsReader() )
	{
		Uint16 numSamples = 0;
		ar << numSamples;

		// load only allowed number of samples
		// TODO: we may add re sampling code if needed
		const Uint32 numSamplesToRead = Red::Math::NumericalUtils::Min< Uint32 >( numSamples, NUM_SAMPLES );
		ar.Serialize( &m_samples[0], sizeof(Sample) * numSamplesToRead );
	}
	else
	{
		Uint16 numSamples = NUM_SAMPLES;
		ar << numSamples;

		ar.Serialize( &m_samples[0], sizeof(Sample) * NUM_SAMPLES );
	}
}

//----

void CurveParameter::ClearData()
{
	for ( Uint32 i=0; i<m_counts; ++i )
	{
		delete m_approx[i];
		m_approx[i] = nullptr;
		m_curves[i] = nullptr;

	}
}

void CurveParameter::CopyData( const CurveParameter& other )
{
	if ( this != &other )
	{
		ClearData();

		m_counts = other.m_counts;
		m_isCooked = other.m_isCooked;

#ifndef RED_FINAL_BUILD
		if ( !m_isCooked )
		{
			for ( Uint32 i=0; i<m_counts; ++i )
			{
				m_curves[i] = other.m_curves[i];
			}
		}
		else
#endif
		{
			RED_FATAL_ASSERT( m_isCooked, "Non cooked curve data used in final build" );

			for ( Uint32 i=0; i<m_counts; ++i )
			{
				m_approx[i] = new CurveApproximation( *other.m_approx[i] );
			}
		}
	}
}

#ifndef RED_FINAL_BUILD

void CurveParameter::CookOnSerialize( IFile& file )
{
	RED_FATAL_ASSERT( m_isCooked, "Invalid serialization" );

	// convert the CCurve to CurveApproximation on save
	for ( Uint32 i=0; i<m_counts; ++i )
	{
		const CCurve* sourceCurve = m_curves[i];
		RED_FATAL_ASSERT( sourceCurve != nullptr, "Trying to cook curve parameters with NULL curves" );

		CurveApproximation* approximatedData = new CurveApproximation();
		approximatedData->BuildFrom( sourceCurve );
		m_approx[i] = approximatedData;

		file << *approximatedData;
	}
}

#endif

void CurveParameter::Serialize( IFile& file )
{
	// GC
	if ( file.IsGarbageCollector() )
	{
		for ( Uint32 i=0; i<m_counts; ++i )
		{
			file << m_curves[i];
		}

		return;
	}

	// header
	file << m_name;
	file << m_counts;
	RED_FATAL_ASSERT( m_counts <= MAX_CURVES, "Curve data in '%ls' is corrupted. Please revert the file !!", file.GetFileNameForDebug() );

#ifndef RED_FINAL_BUILD
	// cook the data when saving for cooker
	if ( file.IsCooker() && !m_isCooked )
	{
		RED_FATAL_ASSERT( file.IsWriter(), "Non-writing cooker, WTF?" );

		// change and save the cooked data flag
		m_isCooked = 1;
		file << m_isCooked;

		// convert curves
		CookOnSerialize( file );
		return;
	}
#endif

	// clean current data
	if ( file.IsReader() )
		ClearData();

	// cooked flag for curves (CCurve vs CurveApproximation)
	file << m_isCooked;

#ifndef RED_FINAL_BUILD
	if ( !m_isCooked )
	{
		for ( Uint32 i=0; i<m_counts; ++i )
		{
			file << m_curves[i];
			RED_FATAL_ASSERT( m_curves[i], "Curve data for curve %d/%d in '%ls' is corrupted. Please revert the file !!", i, m_counts, file.GetFileNameForDebug() );
		}
	}
	else
#endif
	{
		RED_FATAL_ASSERT( m_isCooked, "Non cooked curve data serialized in final build. File '%ls'.", file.GetFileNameForDebug() );

		for ( Uint32 i=0; i<m_counts; ++i )
		{
			if ( file.IsReader() && !m_approx[i] )
				m_approx[i] = new CurveApproximation();

			file << *m_approx[i];
		}
	}
}

#ifndef RED_FINAL_BUILD

void CurveParameter::SerializeLegacy( IFile& file )
{
	RED_FATAL_ASSERT( !m_isCooked, "Legacy serialization of CurveParameter does not support cooked data" );

	file << m_name;

	// old serialized count is 32 bit
	if ( file.IsReader() )
	{
		Uint32 count = 0;
		file << count;

		RED_FATAL_ASSERT( count <= MAX_CURVES, "Unsupported number of curves in the CurveParameter" );
		
		m_isCooked = 0; // legacy curves are NEVER cooked
		m_counts = (Uint8) count;
	}
	else
	{
		Uint32 count = m_counts;
		file << count; // save as 32-bit
	}

	// curve data, always CCurve in case of legacy stuff
	file << (CCurve*&)m_curves[0];
	file << (CCurve*&)m_curves[1];
	file << (CCurve*&)m_curves[2];
	file << (CCurve*&)m_curves[3];
}

#endif

void CurveParameter::CreateCurves( const CName& paramName, const Uint32 curveCount, CObject* curveParent )
{
	RED_FATAL_ASSERT( !m_isCooked, "CreateCurves is not supported on cooked data" );

	if ( m_isCooked )
		return;

	SetName( paramName );
	ChangeCurveCount( curveCount, curveParent );
}

void CurveParameter::DiscardCurves()
{
	RED_FATAL_ASSERT( !m_isCooked, "DiscardCurves is not supported on cooked data" );

	for ( Uint32 i = 0; i < m_counts; ++i )
	{
		ASSERT( m_curves[ i ] );

		m_curves[ i ]->Discard();
		m_curves[ i ] = NULL;
	}
}

void CurveParameter::ChangeCurveCount( const Uint32 curveCount, CObject* curveParent )
{
	RED_FATAL_ASSERT( !m_isCooked, "ChangeCurveCount is not supported on cooked data" );

	if ( m_isCooked )
		return;

	ASSERT( curveParent );
	ASSERT( curveCount <= 4 );
	
	CCurve** curves = (CCurve**) &m_curves[0];

	// Destroy curves
	if ( curveCount < m_counts )
	{
		for ( Uint32 i=m_counts; i<curveCount; i++ )
		{
			ASSERT( curves[ i ] );
			curves[ i ]->Discard();
			curves[ i ] = NULL;
		}
	}
	else if ( curveCount > m_counts )
	{
		// Create new curves
		for ( Uint32 i = m_counts; i < curveCount; ++i )
		{
			CCurve* curve = CreateObject< CCurve >( curveParent );
			ASSERT( curve );
			SetCurve( i, curve );
		}
	}

	// Set colors
	if ( curveCount > 1 )
	{
		if ( curveCount >= 4 )	{ curves[3]->SetColor( Color::WHITE ); }
		if ( curveCount >= 3 )	{ curves[2]->SetColor( Color::BLUE ); }
		curves[1]->SetColor( Color::GREEN );
		curves[0]->SetColor( Color::RED );
	}
	else
	{
		curves[0]->SetColor( Color::WHITE );
	}

	// Set new count
	m_counts = (Uint8) curveCount;
}
