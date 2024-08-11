/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "curveEditDrawer.h"
#include "../../common/core/xmlWriter.h"
#include "../../common/core/xmlReader.h"


#define DEFAULT_COLORGRAPH_COLOR	( Color::CYAN )


namespace
{
	RED_INLINE bool IsPointInsideCircle( const wxPoint &center, Float radius, const wxPoint &testPoint )
	{
		wxPoint diff = center - testPoint;
		return diff.x*diff.x + diff.y*diff.y <= radius * radius;
	}

	RED_INLINE bool IsPointInsideTriangle( const wxPoint &topVertex, Int32 width, Int32 height, const wxPoint &testPoint )
	{
		// We'll actually just check the column around the triangle. This allows for dragging the control point by dragging in the color
		// section as well as the triangle itself.
		wxPoint diff = testPoint - topVertex;
		return ( width > 0 && Abs(diff.x) < width / 2 );
		//return width > 0 && height > 0 && diff.y >= 0 && diff.y <= height && Abs(diff.x/(Float)diff.y) <= 0.5f*width/(Float)height;
	}

	RED_INLINE Float NormalizeRange( Float value, Float min, Float max )
	{
		return ( value - min ) / ( max - min );
	}


	RED_INLINE bool TransformRanges( Float value, Float srcRangeMin, Float srcRangeMax, Float dstRangeMin, Float dstRangeMax, Float &outValue, Bool allowOutOfRange = false )
	{
		if ( srcRangeMax <= srcRangeMin )
			return false;
		if ( dstRangeMax <= dstRangeMin )
			return false;

		if ( !allowOutOfRange && ( value < srcRangeMin || value > srcRangeMax ) )
			return false;

		value = NormalizeRange( value, srcRangeMin, srcRangeMax ) * (dstRangeMax - dstRangeMin) + dstRangeMin;

		if ( !allowOutOfRange && ( value < dstRangeMin || value > dstRangeMax ) )
			return false;

		outValue = value;
		return true;
	}

	RED_INLINE bool TransformRanges( Float value, Float srcRangeMin, Float srcRangeMax, Float dstRangeMin, Float dstRangeMax, Int32 &outValue, Bool allowOutOfRange = false )
	{
		Float outValueFloat = 0;
		if ( !TransformRanges( value, srcRangeMin, srcRangeMax, dstRangeMin, dstRangeMax, outValueFloat, allowOutOfRange ) )
			return false;
		outValue = (Int32)outValueFloat;
		return true;
	}

	RED_INLINE wxColour LerpWxColor( Float frac, const wxColour &a, const wxColour &b )
	{
		frac = Clamp( frac, 0.f, 1.f );
		return wxColour (
			(wxColour::ChannelType) Lerp<Float>( frac, a.Red(),		b.Red()	),
			(wxColour::ChannelType) Lerp<Float>( frac, a.Green(),	b.Green() ),
			(wxColour::ChannelType) Lerp<Float>( frac, a.Blue(),	b.Blue() ),
			(wxColour::ChannelType) Lerp<Float>( frac, a.Alpha(),	b.Alpha() ) );
	}

	RED_INLINE void BuildTimeString( Float time, Int32 seconds, wxString &outString )
	{
		Int32 textHour   = (seconds / (60 * 60))	% 24;
		Int32 textMinute = (seconds / (60))		% 60;
		Int32 textSecond = (seconds / (1))		% 60;
		if ( 0 == seconds % (24*60*60) && time > 0.5f )
		{
			textHour = 24;
		}
		if ( 0 != textSecond )
		{
			outString.Printf(TXT("%02i:%02i:%02i"), (Int32)textHour, (Int32)textMinute, (Int32)textSecond);
		}
		else
		{
			outString.Printf(TXT("%02i:%02i"), (Int32)textHour, (Int32)textMinute);
		}	
	}

	RED_INLINE void BuildTimeString( Float time, wxString &outString )
	{
		time = time - floorf(time);
		BuildTimeString( time, 24 * 60 * 60 * time, outString );
	}
}



const Float CCurveEditDrawer::TIME_MAXIMUM = ( 1.0f - 0.00000001f );



CCurveEditDrawer::CCurveEditDrawer()
{
	m_displayAreaTimeMin	= 0.f;
	m_displayAreaTimeMax	= TIME_MAXIMUM;
	m_displayAreaValueMin	= 0.f;
	m_displayAreaValueMax	= 1.f;
}


CCurveEditDrawer::~CCurveEditDrawer()
{
}


void CCurveEditDrawer::SetFullRect( const wxRect& fullRect )
{
	m_fullRect = fullRect;
	m_graphRect = GetGraphRect( m_fullRect );
}
void CCurveEditDrawer::SetDC( wxDC& dc )
{
	m_dc = &dc;
}


void CCurveEditDrawer::Reset( const SCurveEditDrawerSettings& settings )
{
	m_settings = settings;
}


void CCurveEditDrawer::SetCurve( SCurveBase* curve )
{
	m_curve = curve;
}

Bool CCurveEditDrawer::CopySelection() const
{
	// Serialize the point to XML
	CXMLWriter writer;
	writer.BeginNode( TXT("pointCollection") );
	writer.Attribute( TXT("count"), ToString( m_selectedPoints.Size() ) );

	// If we have a current selection, copy that directly.
	if ( m_currentSelection.m_type == CPT_Value )
	{
		Vector copyValue;
		ESimpleCurveType copyType;
		Uint32 activeNode = 0;

		const Uint32 pointCount = m_selectedPoints.Size();
		for( Uint32 i=0; i<pointCount; ++i )
		{
			const SCurveEditControlPoint& pointRef = m_selectedPoints[i];

			if( pointRef.m_index == m_currentSelection.m_index )
			{
				activeNode = i;
			}

			writer.BeginNode( TXT("point") );
			{
				String timeStr = ::ToString< Float >( m_curve->GetCurveData().GetTimeAtIndex( pointRef.m_index ) );
				writer.Attribute( TXT("time"), timeStr );
				copyValue = pointRef.m_curve->GetValueAtIndex( pointRef.m_index );
				String value = ::ToString< Vector >( copyValue );
				writer.Attribute( TXT("value"), value );
				copyType = pointRef.m_curve->GetSimpleCurveType();
				String type = ::ToString< Int32 >( (Int32)copyType );
				writer.Attribute( TXT("type"), type );

				// Since we're copying an existing control point, we can save out a bunch of extra information,
				// beyond the value and curve type.
				SCurveTangents tangents = pointRef.m_curve->GetTangent( pointRef.m_index );

				String tanVal = ::ToString< Vector >( tangents.m_tangents );
				writer.Attribute( TXT("tanVal"), tanVal );

				String tanType;
				tanType = ::ToString< Uint32 >( tangents.m_segmentTypes[0] );
				writer.Attribute( TXT("tanType0"), tanType );
				tanType = ::ToString< Uint32 >( tangents.m_segmentTypes[1] );
				writer.Attribute( TXT("tanType1"), tanType );
			}
			writer.EndNode();
		}
		writer.Attribute( TXT("activeIndex"), ToString( activeNode ) );
	}
	// A value control point is not selected, so we create and copy a new control point based on the latest position.
	else
	{
		// Copy at current time. We have the last click position in the currentSelection (even though there is no selection)
		Float time, temp;
		if ( !TransformScalarPointInverse( m_currentSelection.m_position, time, temp ) )
			return false;

		Vector copyValue = m_curve->GetValue( time );
		ESimpleCurveType copyType = m_curve->GetSimpleCurveType();

		writer.BeginNode( TXT("point") );
		{
			String value = ::ToString< Vector >( copyValue );
			writer.Attribute( TXT("value"), value );

			String type = ::ToString< Int32 >( (Int32)copyType );
			writer.Attribute( TXT("type"), type );
		}
		writer.EndNode();
	}

	writer.EndNode();
	writer.Flush();

	// Save it in the clipboard
	if ( wxTheClipboard->Open() )
	{
		wxTheClipboard->SetData( new wxTextDataObject( writer.GetContent().AsChar() ) );
		wxTheClipboard->Close();
		wxTheClipboard->Flush();

		return true;
	}

	return false;
}

Bool CCurveEditDrawer::PasteControlPoints( const wxPoint& pos )
{
	m_selectedPoints.ClearFast();

	// Find time at mouse cursor
	Float mouseTime, temp;
	if ( !TransformScalarPointInverse( pos, mouseTime, temp ) )
		return false;

	// Get text from the clipboard
	String clipboardContent = String::EMPTY;
	if ( wxTheClipboard->Open())
	{
		if ( wxTheClipboard->IsSupported( wxDF_TEXT ) )
		{
			wxTextDataObject data;
			wxTheClipboard->GetData( data );
			clipboardContent = data.GetText().wc_str();
			if ( clipboardContent.Empty() )
			{
				return false;
			}
		}  
		wxTheClipboard->Close();
	}

	// Deserialize the content of the clipboard
	CXMLReader reader( clipboardContent );
	if ( !reader.BeginNode( TXT("pointCollection") ) )
	{
		ERR_EDITOR( TXT("Improper data in the clipboard.") );
		return false;
	}

	//
	String pointCountStr, activePointIndexStr;
	if ( !reader.Attribute( TXT("count"), pointCountStr ) || !reader.Attribute( TXT("activeIndex"), activePointIndexStr ) )
	{
		ERR_EDITOR( TXT("Improper data in the clipboard.") );
		return false;
	}

	// parse values
	Uint32 pointCount = 0;
	if ( !::FromString< Uint32 >( pointCountStr, pointCount ) )
	{
		ERR_EDITOR( TXT("Unable to parse the value %s into the Uint32."), pointCountStr.AsChar() );
		return false;
	}

	Uint32 activePointIndex = 0;
	if ( !::FromString< Uint32 >( activePointIndexStr, activePointIndex ) )
	{
		ERR_EDITOR( TXT("Unable to parse the value %s into the Uint32."), activePointIndexStr.AsChar() );
		return false;
	}

	Float firstPointTime = 0.0f;
	for( Uint32 i=0; i<pointCount; ++i )
	{
		if ( !reader.BeginNode( TXT("point") ) )
		{
			ERR_EDITOR( TXT("Improper data in the clipboard.") );
			return false;
		}

		String timeStr, valueStr, typeStr, tanValStr, tanType0Str, tanType1Str;
		if ( !reader.Attribute( TXT("time"), timeStr ) || !reader.Attribute( TXT("value"), valueStr ) || !reader.Attribute( TXT("type"), typeStr ) )
		{
			ERR_EDITOR( TXT("Improper data in the clipboard.") );
			return false;
		}
		reader.Attribute( TXT("tanVal"), tanValStr );
		reader.Attribute( TXT("tanType0"), tanType0Str );
		reader.Attribute( TXT("tanType1"), tanType1Str );

		reader.EndNode();

		// prepare correct time 
		Float properTime = mouseTime;
		if( i==0 )
		{
			::FromString< Float >( timeStr, firstPointTime );
		}
		else
		{
			Float nodeTime;
			::FromString< Float >( timeStr, nodeTime );
			properTime = mouseTime + ( nodeTime - firstPointTime );
		}


		// Parse the value
		Vector value;
		if ( !::FromString< Vector >( valueStr, value ) )
		{
			ERR_EDITOR( TXT("Unable to parse the value %s into the vector."), valueStr.AsChar() );
			return false;
		}

		// Parse the curve's type
		ESimpleCurveType srcType = SCT_Vector;
		::FromString< Int32 >( typeStr, *(Int32 *)(&srcType) );

		if ( !m_curve->CanImportFromType( srcType ) )
		{
			ERR_EDITOR( TXT("Cannot convert curve types.") );
			return false;
		}
		m_curve->ConvertValueFromType( properTime, value, srcType );

		// Paste new point
		Int32 newIndex = m_curve->AddPoint( properTime, value );

		// Set any tangent info we have. We start from the existing tangent data, so we only modify those that we have.
		SCurveTangents tangents = m_curve->GetTangent( newIndex );
		if ( !tanValStr.Empty() && !::FromString< Vector >( tanValStr, tangents.m_tangents ) )
		{
			ERR_EDITOR( TXT("Unable to parse the tangent value %s."), tanValStr.AsChar() );
			return false;
		}
		if ( !tanType0Str.Empty() && !::FromString< Uint32 >( tanType0Str, tangents.m_segmentTypes[0] ) )
		{
			ERR_EDITOR( TXT("Unable to parse the tangent type %s."), tanType0Str.AsChar() );
			return false;
		}
		if ( !tanType1Str.Empty() && !::FromString< Uint32 >( tanType1Str, tangents.m_segmentTypes[1] ) )
		{
			ERR_EDITOR( TXT("Unable to parse the tangent type %s."), tanType1Str.AsChar() );
			return false;
		}
		m_curve->SetTangent( newIndex, tangents );

		// TODO
		SCurveEditControlPoint ctrlPt;
		ctrlPt.m_index = newIndex;
		ctrlPt.m_curve = m_curve;
		ctrlPt.m_type =  CPT_Value;
		FillControlPointPosition( ctrlPt );
		m_selectedPoints.PushBackUnique( ctrlPt );
	}

	reader.EndNode();

	// Select the new point.
	m_currentSelection = m_selectedPoints[activePointIndex];

	m_selectionActive = false;

	return true;
}


Bool CCurveEditDrawer::RemoveControlPoints()
{
	TDynArray< Float > pointsToRemove;

	const Uint32 pointCount = m_selectedPoints.Size();
	for( Uint32 i=0; i<pointCount; ++i )
	{
		pointsToRemove. PushBack( m_curve->GetCurveData().GetTimeAtIndex( m_selectedPoints[i].m_index ) );
	}

	//remove
	const Uint32 pointToRemoveCount = pointsToRemove.Size();
	for( Uint32 i=0; i<pointToRemoveCount; ++i )
	{
		m_curve->RemovePointAtTime( pointsToRemove[i] );
	}

	ClearSelection();

	return true;
}


void CCurveEditDrawer::PanView( const wxPoint& mouseDelta, Bool constrainX, Bool constrainY, PropertiesPageDayCycleSettings *dayCycleSettings /*= NULL*/ )
{
	Float deltaTime = (Float)-mouseDelta.x * (m_displayAreaTimeMax  - m_displayAreaTimeMin)  / Max( 1.f, (Float)m_graphRect.GetWidth() );
	Float deltaValue= (Float) mouseDelta.y * (m_displayAreaValueMax - m_displayAreaValueMin) / Max( 1.f, (Float)m_graphRect.GetHeight() );

	if ( constrainX )
	{
		deltaTime = 0;
	}

	if ( constrainY )
	{
		deltaValue = 0;
	}

	if ( 0.f != deltaValue )
	{
		Float newEditOrigin = m_curve->GetScalarEditOrigin() + deltaValue;
		m_curve->SetScalarEditOrigin( newEditOrigin );
	}

	if ( 0.f != deltaTime )
	{
		// If this curve loops or uses day-time, constrain it to range [0,1)
		if ( m_curve->UseDayTime() || m_curve->IsLoop() )
		{
			deltaTime = Max(deltaTime, -Max(0.f, m_displayAreaTimeMin));
			deltaTime = Min(deltaTime,  Max(0.f, TIME_MAXIMUM - m_displayAreaTimeMax));
		}

		if ( m_curve->UseDayTime() && dayCycleSettings )
		{
			dayCycleSettings->m_timeMin += deltaTime;
			dayCycleSettings->m_timeMax += deltaTime;
		}
		else
		{
			m_displayAreaTimeMin += deltaTime;
			m_displayAreaTimeMax += deltaTime;
		}
	}
}

void CCurveEditDrawer::ZoomView( const wxPoint& center, Float scale, Bool constrainX, Bool constrainY, PropertiesPageDayCycleSettings *dayCycleSettings /*= NULL*/ )
{
	Float refX, refY;
	TransformScalarPointInverse( center, refX, refY, true );

	if ( m_curve->UseDayTime() && dayCycleSettings )
	{
		const Float scaleThreshold = TIME_MAXIMUM / ( dayCycleSettings->m_timeMax - dayCycleSettings->m_timeMin );

		if ( !constrainX )
		{
			dayCycleSettings->m_timeMin = (dayCycleSettings->m_timeMin - refX) * scale + refX;
			dayCycleSettings->m_timeMax = (dayCycleSettings->m_timeMax - refX) * scale + refX;
			{
				Float delta = Max(0.f, (0 - dayCycleSettings->m_timeMin)) + Min(0.f, TIME_MAXIMUM - dayCycleSettings->m_timeMax);
				dayCycleSettings->m_timeMin += delta;
				dayCycleSettings->m_timeMax += delta;
			}
			if ( scale > scaleThreshold )
			{
				Float delta = 0.5f * TIME_MAXIMUM - 0.5f * (dayCycleSettings->m_timeMin + dayCycleSettings->m_timeMax);
				dayCycleSettings->m_timeMin += delta;
				dayCycleSettings->m_timeMax += delta;
			}
		}

		if ( !constrainY )
		{
			dayCycleSettings->m_valueScale	= dayCycleSettings->m_valueScale * scale;
		}
	}
	else
	{
		if ( !constrainX )
		{
			m_displayAreaTimeMin = ( m_displayAreaTimeMin - refX ) * scale + refX;
			m_displayAreaTimeMax = ( m_displayAreaTimeMax - refX ) * scale + refX;

			if ( m_curve->IsLoop() )
			{
				const Float scaleThreshold = TIME_MAXIMUM / ( m_displayAreaTimeMax - m_displayAreaTimeMin );
				{
					Float delta = Max(0.f, (0 - m_displayAreaTimeMin)) + Min(0.f, TIME_MAXIMUM - m_displayAreaTimeMax);
					m_displayAreaTimeMin += delta;
					m_displayAreaTimeMax += delta;
				}
				if ( scale > scaleThreshold )
				{
					Float delta = 0.5f * TIME_MAXIMUM - 0.5f * (m_displayAreaTimeMin + m_displayAreaTimeMax);
					m_displayAreaTimeMin += delta;
					m_displayAreaTimeMax += delta;
				}
			}
		}

		if ( !constrainY )
		{
			m_curve->SetScalarEditOrigin( ( m_curve->GetScalarEditOrigin() - refY ) * scale + refY );
			m_curve->SetScalarEditScale( m_curve->GetScalarEditScale() * scale );
		}
	}
}


void CCurveEditDrawer::MouseOverControlPoint( const wxPoint& pos )
{
	Bool isTangent;
	Int32 bestEntryIndex = FindNearestEntry( pos, NULL, &isTangent );

	if ( bestEntryIndex != -1 )
	{
		SCurveEditControlPoint ctrlPt;
		ctrlPt.m_index = bestEntryIndex;
		ctrlPt.m_curve = m_curve;
		ctrlPt.m_type = ( isTangent ? CPT_Tangent : CPT_Value );
		FillControlPointPosition( ctrlPt );

		if ( IsPosOnControlPoint( pos, ctrlPt ) )
		{
			m_currentMouseOver = ctrlPt;
			return;
		}
	}
	m_currentMouseOver.m_type = CPT_None;
	m_currentMouseOver.m_position = pos;
}

Bool CCurveEditDrawer::BeginSelectControlPoint( const wxPoint& pos, Bool multiselect )
{
	UpdateSelectedPointsPosition();

	Bool isTangent;
	Int32 bestEntryIndex = FindNearestEntry( pos, NULL, &isTangent );

	if ( bestEntryIndex != -1 )
	{
		SCurveEditControlPoint ctrlPt;
		ctrlPt.m_index = bestEntryIndex;
		ctrlPt.m_curve = m_curve;
		ctrlPt.m_type = ( isTangent ? CPT_Tangent : CPT_Value );
		FillControlPointPosition( ctrlPt );

		if ( IsPosOnControlPoint( pos, ctrlPt ) )
		{
			if( multiselect == false && IsPointSelected( bestEntryIndex ) == false )
			{
				m_selectedPoints.ClearFast();
			}

			const Uint32 pointCount = m_selectedPoints.Size();
			if( multiselect == true && m_selectedPoints.Size() > 1 )
			{
				for( Uint32 i=0; i<pointCount; ++i )
				{
					if( ctrlPt.m_index == m_selectedPoints[i].m_index )
					{
						m_selectedPoints.RemoveAt( i );
						m_selectionActive = true;
						return true;
					}
				}
			}

			m_currentSelection = ctrlPt;
			m_selectedPoints.PushBackUnique( m_currentSelection );

			// sort
			struct LocalSorter
			{
				static Bool Sort( const SCurveEditControlPoint& p1, const SCurveEditControlPoint& p2 )
				{
					return p2.m_index > p1.m_index;
				}
			};
			Sort( m_selectedPoints.Begin(), m_selectedPoints.End(), LocalSorter::Sort );

			m_selectionActive = true;
			return true;
		}
	}
	m_selectedPoints.ClearFast();
	m_currentSelection.m_type = CPT_None;
	m_currentSelection.m_curve = NULL;
	m_currentSelection.m_position = pos;

	m_isSelectionAreaActive = true;
	m_selectionAreaStartPoint = m_selectionAreaEndPoint = pos;

	return false;
}

void CCurveEditDrawer::EndSelectControlPoint( const wxPoint& pos )
{
	if ( m_selectionActive )
	{
		m_selectionActive = false;
	}

	if( m_isSelectionAreaActive == true )
	{
		// clear selection
		m_selectedPoints.ClearFast();
		m_currentSelection.m_type = CPT_None;
		m_currentSelection.m_curve = NULL;
		m_currentSelection.m_position = pos;

		m_selectionAreaEndPoint = pos;

		wxRect selectionArea( m_selectionAreaStartPoint, m_selectionAreaEndPoint );

		const Uint32 controlPointCount = m_curve->GetNumPoints();
		for( Uint32 i=0; i<controlPointCount; ++i )
		{
			Float curveTime = m_curve->GetCurveData().GetTimeAtIndex(i);
			Float curveValue = m_curve->GetFloatValueAtIndex(i);

			wxPoint point;
			if ( !TransformScalarPoint( curveTime, curveValue, point ) )
				continue;

			if( selectionArea.Contains( point ) == true )
			{
				SCurveEditControlPoint ctrlPt;
				ctrlPt.m_index = i;
				ctrlPt.m_curve = m_curve;
				ctrlPt.m_type = CPT_Value;
				FillControlPointPosition( ctrlPt );

				m_currentSelection = ctrlPt;
				m_selectedPoints.PushBackUnique( m_currentSelection );
			}
		}

		// sort
		struct LocalSorter
		{
			static Bool Sort( const SCurveEditControlPoint& p1, const SCurveEditControlPoint& p2 )
			{
				return p2.m_index > p1.m_index;
			}
		};
		Sort( m_selectedPoints.Begin(), m_selectedPoints.End(), LocalSorter::Sort );
	}
	m_isSelectionAreaActive = false;
}

void CCurveEditDrawer::ClearSelection()
{
	m_selectedPoints.ClearFast();
	m_currentSelection.m_type = CPT_None;
	m_currentSelection.m_curve = NULL;
}

void CCurveEditDrawer::SetSelection( Uint32 index )
{
	if ( index < m_curve->GetNumPoints() )
	{
		m_currentSelection.m_index = index;
		m_currentSelection.m_curve = m_curve;
		m_currentSelection.m_type = CPT_Value;
		FillControlPointPosition( m_currentSelection );
	}
}


void CCurveEditDrawer::DeleteCurrentSelectedPoint()
{
	if ( m_currentSelection.m_type == CPT_Value )
	{
		m_currentSelection.m_curve->RemovePointAtIndex( m_currentSelection.m_index );
		ClearSelection();
	}
}


Bool CCurveEditDrawer::TransformScalarPoint( Float x, Float y, wxPoint &outPoint, Bool allowOutOfRange /*= false*/ ) const
{
	return
		TransformRanges( x, m_displayAreaTimeMin, m_displayAreaTimeMax, m_graphRect.GetLeft(), m_graphRect.GetRight(), outPoint.x, allowOutOfRange ) &&
		TransformRanges( m_displayAreaValueMax - (y - m_displayAreaValueMin), m_displayAreaValueMin, m_displayAreaValueMax, m_graphRect.GetTop(), m_graphRect.GetBottom(), outPoint.y, allowOutOfRange );
}

Bool CCurveEditDrawer::TransformScalarPointInverse( const wxPoint &point, Float& outX, Float& outY, Bool allowOutOfRange /*= false*/ ) const
{
	return
		TransformRanges( point.x, m_graphRect.GetLeft(), m_graphRect.GetRight(), m_displayAreaTimeMin, m_displayAreaTimeMax, outX, allowOutOfRange ) &&
		TransformRanges( m_graphRect.GetBottom() - (point.y - m_graphRect.GetTop()), m_graphRect.GetTop(), m_graphRect.GetBottom(), m_displayAreaValueMin, m_displayAreaValueMax, outY, allowOutOfRange );
}


void CCurveEditDrawer::DrawLine( const wxPoint& pt1, const wxPoint& pt2, const wxColour& colour, Int32 width ) const
{
	m_dc->SetPen( wxPen( colour, width ) );
	m_dc->DrawLine( pt1, pt2 );
}

void CCurveEditDrawer::DrawFilledRectangle( const wxRect& rect, const wxColour& colour ) const
{
	m_dc->SetBrush( wxBrush( colour ) );
	m_dc->SetPen( *wxTRANSPARENT_PEN );
	m_dc->DrawRectangle( rect );
}

void CCurveEditDrawer::DrawRectangle( const wxRect& rect, const wxColour& colour, Int32 lineWidth ) const
{
	m_dc->SetBrush( *wxTRANSPARENT_BRUSH );
	m_dc->SetPen( wxPen( colour, lineWidth ) );
	m_dc->DrawRectangle( rect );
}

void CCurveEditDrawer::DrawFrameRectangle( const wxRect& rect, const wxColour& fillColour, const wxColour& frameColour, Int32 lineWidth ) const
{
	m_dc->SetBrush( wxBrush( fillColour ) );
	m_dc->SetPen( wxPen( frameColour, lineWidth ) );
	m_dc->DrawRectangle( rect );
}


void CCurveEditDrawer::DrawFilledCircle( const wxPoint& c, Int32 radius, const wxColour& colour ) const
{
	m_dc->SetBrush( wxBrush( colour ) );
	m_dc->SetPen( *wxTRANSPARENT_PEN );
	m_dc->DrawCircle( c, radius );
}

void CCurveEditDrawer::DrawCircle( const wxPoint& c, Int32 radius, const wxColour& colour, Int32 lineWidth ) const
{
	m_dc->SetBrush( *wxTRANSPARENT_BRUSH );
	m_dc->SetPen( wxPen( colour, lineWidth ) );
	m_dc->DrawCircle( c, radius );
}

void CCurveEditDrawer::DrawFrameCircle( const wxPoint& c, Int32 radius, const wxColour& fillColour, const wxColour& frameColour, Int32 lineWidth ) const
{
	m_dc->SetBrush( wxBrush( fillColour ) );
	m_dc->SetPen( wxPen( frameColour, lineWidth ) );
	m_dc->DrawCircle( c, radius );
}



void CCurveEditDrawer::DrawTimePreview( Float time ) const
{
	Int32 x = 0;
	if ( TransformRanges( time, m_displayAreaTimeMin, m_displayAreaTimeMax, m_graphRect.GetLeft(), m_graphRect.GetRight(), x ) )
	{
		wxColour colorInside( 222, 60, 240 );
		wxColour colorBorder(   0,  0,   0 );
		DrawFrameRectangle( wxRect( x-1, m_graphRect.GetTop(), 4, m_graphRect.GetHeight() ), colorInside, colorBorder, 1 );
	}
}

void CCurveEditDrawer::DrawTimeSelection( Float minSelection, Float maxSelection ) const
{
	Int32 minX = 0;
	Int32 maxX = 0;

	if( minSelection > maxSelection )
	{
		return;
	}

	TransformRanges( minSelection, m_displayAreaTimeMin, m_displayAreaTimeMax, m_graphRect.GetLeft(), m_graphRect.GetRight(), minX, true );
	TransformRanges( maxSelection, m_displayAreaTimeMin, m_displayAreaTimeMax, m_graphRect.GetLeft(), m_graphRect.GetRight(), maxX, true );

	// Draw
	wxColour colorInside( 0, 162, 232 );
	if( minSelection != maxSelection )
	{
		DrawFilledRectangle( wxRect( minX, m_graphRect.GetTop(), maxX - minX, m_graphRect.GetHeight() ), colorInside );
	}
	else
	{
		DrawFilledRectangle( wxRect( minX, m_graphRect.GetTop(), 1, m_graphRect.GetHeight() ), colorInside );
	}
}

wxRect CCurveEditDrawer::DrawGrid( bool enableMetricsBar, const wxColour &colorTransparent ) const
{
	if ( m_curve->UseDayTime() )
	{
		return DrawGridTime( enableMetricsBar, colorTransparent );
	}
	else
	{
		return DrawGridNum( enableMetricsBar, colorTransparent,  m_curve->IsLoop() );
	}
}


wxRect CCurveEditDrawer::DrawGridTime( bool enableMetricsBar, const wxColour& colorTransparent ) const
{
	// Draw grid with fixed horizontal boundaries at t=0 and t=1.

	// --------------------------------
	struct DivData
	{
		Int32			segments;
		Int32			lineWidth;
		Int32			lineStyle;
		wxColour	lineColor;
		wxColour	textColor;
		Float		lineAppearPixelsDistStart;
		Float		lineAppearPixelsDistRange;
		Float		textAppearPixelsDistStart;
		Float		textAppearPixelsDistRange;
	};
	// --------------------------------
	DivData divs[] =
	{
		{ 24 * 60 * 60,	1,	wxSOLID,		wxColour( 96, 96, 96),	wxColour(255, 255, 255),  15, 10,  60, 10 },
		{ 24 * 60,		1,	wxSOLID,		wxColour( 96, 96, 96),	wxColour(255, 255, 255),  15, 10,  60, 10 },
		{ 24 * 4,		1,	wxSOLID,		wxColour( 96, 96, 96),	wxColour(255, 255, 255),  15, 10,  60, 10 },
		{ 24,			1,	wxSOLID,		wxColour( 96, 96, 96),	wxColour(255, 255, 255),  15, 10,  40, 10 },
		{ 12,			1,	wxSOLID,		wxColour(128,128,128),	wxColour(255, 255, 255),  15, 10,  40, 10 },
		{ 4,			2,	wxSHORT_DASH,	wxColour(192,192,  0),	wxColour(255, 255, 255),  15, 10,  40, 10 },
		{ 1,			2,	wxSOLID,		wxColour(255,  0,  0),	wxColour(255, 255, 255),  15, 10,  40, 10 },
	};
	// --------------------------------

	if ( m_graphRect.IsEmpty() || m_displayAreaTimeMin >= m_displayAreaTimeMax || m_displayAreaValueMin >= m_displayAreaValueMax )
	{
		return wxRect( 0, 0, 0, 0 );
	}

	size_t divCount = sizeof( divs ) / sizeof( divs[0] );

	Int32 divStart = 0;
	for ( size_t i = 0; i < divCount; ++i )
	{
		DivData &divData = divs[i];
		Float divPixels = m_graphRect.GetWidth() / (divData.segments * (m_displayAreaTimeMax - m_displayAreaTimeMin));
		Float lineAlpha = i + 1 < divCount ? Clamp( (divPixels - divData.lineAppearPixelsDistStart) / divData.lineAppearPixelsDistRange, 0.f, 1.f ) : 1.f;
		Float textAlpha =                    Clamp( (divPixels - divData.textAppearPixelsDistStart) / divData.textAppearPixelsDistRange, 0.f, 1.f );
		divData.lineColor = LerpWxColor( lineAlpha, colorTransparent, divData.lineColor );
		divData.textColor = LerpWxColor( textAlpha, colorTransparent, divData.textColor );
		if ( 0 == lineAlpha && 0 == textAlpha && i + 1 < divCount )
		{
			divStart = (Int32)( i + 1 );
		}
	}

	// Set font

	const Int32 fontSize = 8;
	if ( enableMetricsBar )
	{
		m_dc->SetFont(wxFont(fontSize, wxMODERN, wxNORMAL, wxBOLD, 0));
		m_dc->SetTextBackground( wxColour (0, 0, 0, 0) );
	}

	// Calculate curve area
	wxRect curveArea = m_graphRect;
	if ( enableMetricsBar )
	{
		curveArea.height -= m_dc->GetTextExtent( TXT("0123456789:") ).y;
	}

	// Draw vertical lines and text
	if ( !m_graphRect.IsEmpty() )
	{
		m_dc->SetClippingRegion( m_graphRect );

		const Int32 border = 2;

		Float timeStep	= 1.f / divs[divStart].segments;
		Float timeStart	= m_displayAreaTimeMin - timeStep * border;
		Float timeEnd	= m_displayAreaTimeMax + timeStep * border;

		Float offset	= ( timeStart >= 0 ? fmod(timeStart, timeStep) : timeStep - fmod(-timeStart, timeStep) );

		timeStart -= offset;
		timeEnd   -= offset;

		Float divCounterTime = ( timeStart >= 0 ? timeStart : 1 - fmod(-timeStart, 1.0f) );
		Int32   divCounter     = (Int32)( divCounterTime * divs[divStart].segments + 0.5f );

		for ( Float time = timeStart; time <= timeEnd; time += timeStep, ++divCounter )
		{
			if ( Abs(time - 0.5f) > 0.5f + 0.01f * timeStep )
				continue;

			Float f = NormalizeRange( time, m_displayAreaTimeMin, m_displayAreaTimeMax );
			Int32   x = (Int32)Lerp<Float>(f, m_graphRect.GetLeft(), m_graphRect.GetRight());

			const Int32 boundsTolerance = 50; // tolerance introduced because of printing current hour
			if ( x < m_graphRect.GetLeft() - boundsTolerance || x > m_graphRect.GetRight() + boundsTolerance )
				continue;

			// Find div data index
			size_t divDataIndex = divStart;
			while ( divDataIndex < divCount - 1 && (divCounter % (divs[divStart].segments / divs[divDataIndex+1].segments)) == 0 )
				++divDataIndex;

			const DivData& divData = divs[divDataIndex];

			// Draw text
			wxSize textExtent( 0, 0 );
			if ( enableMetricsBar )
			{	
				// Generate text (hour, minute etc)
				wxString text;
				Int32 seconds  = (divCounter * (divs[0].segments / divs[divStart].segments)) % divs[0].segments;
				BuildTimeString( time, seconds, text );

				// Draw text
				textExtent = m_dc->GetTextExtent(text);
				if ( divData.textColor != colorTransparent )
				{
					m_dc->SetTextForeground( divData.textColor );
					wxPoint	 textCorner = wxPoint(x, m_graphRect.GetBottom()) - wxPoint(textExtent.x/2, textExtent.y);
					m_dc->DrawText(text, textCorner);
				}
			}

			// Draw line
			if ( m_graphRect.height > textExtent.y )
			{
				m_dc->SetPen(wxPen(divData.lineColor, divData.lineWidth, divData.lineStyle));
				m_dc->DrawLine(x, m_graphRect.y, x, m_graphRect.GetBottom() - textExtent.y);
			}
		}

		m_dc->DestroyClippingRegion();
	}

	// Draw horizontal lines and text

	if ( !curveArea.IsEmpty() )
	{
		const Int32 lineWidthOrigin = 2;
		const Int32 lineWidth = 1;

		wxColour lineColorOrigin= wxColour( 192, 192,   0 );
		wxColour lineColor		= wxColour(  96,  96,  96 );

		m_dc->SetClippingRegion( curveArea );

		const Int32 timeZeroX = (Int32) Lerp<Float>( NormalizeRange( 0.0f, m_displayAreaTimeMin, m_displayAreaTimeMax ), m_graphRect.GetLeft(), m_graphRect.GetRight() );
		const Int32 timeOneX  = (Int32) Lerp<Float>( NormalizeRange( 1.0f, m_displayAreaTimeMin, m_displayAreaTimeMax ), m_graphRect.GetLeft(), m_graphRect.GetRight() );

		Int32 valueDisplayX		= ::Max( m_graphRect.GetLeft(), timeZeroX ) + 2;
		Int32 valueDisplayAlign	= 1;
		if ( timeZeroX > m_graphRect.GetLeft() + m_graphRect.GetWidth() / 4 )
		{
			valueDisplayX = timeZeroX - 2;
			valueDisplayAlign = -1;
		}

		const Int32 appearPixelThreshold	= 40;
		const Int32 appearPixelRange		= 10;

		Float valuePerPixel		= (m_displayAreaValueMax - m_displayAreaValueMin) / m_graphRect.height;
		Float appearValueStep	= powf( 2.0f, ceilf( logf(appearPixelThreshold * valuePerPixel) / logf(2.0f) ) );

		Float gridValueMin = m_displayAreaValueMin - ( ( m_displayAreaValueMin >= 0 ) ? fmod(m_displayAreaValueMin,appearValueStep) : (appearValueStep - fmod(-m_displayAreaValueMin, appearValueStep)) );
		Float gridValueMax = m_displayAreaValueMax + appearValueStep;

		for ( Float gridValue = gridValueMin; gridValue <= gridValueMax; gridValue += appearValueStep )
		{
			Int32 y = (Int32) Lerp<Float>( NormalizeRange( gridValue, m_displayAreaValueMin, m_displayAreaValueMax ), m_graphRect.GetBottom(), m_graphRect.GetTop() );
			if ( y < curveArea.GetTop() - fontSize || y > curveArea.GetBottom() + fontSize )
				continue;

			// ace_todo!!! calculate alpha :)
			const Float alpha = 1.f;
			if ( alpha <= 0.f )
				continue;

			// Draw line
			const bool isOrigin = Abs(gridValue) < 0.01f * appearValueStep;
			wxColour currColor = LerpWxColor( alpha, colorTransparent, isOrigin ? lineColorOrigin : lineColor );
			Int32		 currWidth = isOrigin ? lineWidthOrigin : lineWidth;

			DrawLine( wxPoint( timeZeroX, y ), wxPoint( timeOneX, y ), currColor, currWidth );

			// Draw text
			if ( enableMetricsBar )
			{
				// Build text
				wxString text;
				text.Printf(TXT("%f"), gridValue);

				// Draw text
				wxSize  textExtent = m_dc->GetTextExtent( text );
				wxPoint textCorner = wxPoint ( valueDisplayAlign > 0 ? valueDisplayX : valueDisplayX - textExtent.x, y - textExtent.y );
				m_dc->SetTextForeground( wxColour(255, 255, 255, 255) );
				m_dc->DrawText(text, textCorner);
			}
		}

		//Draw range indicators
		Float minValue;
		Float maxValue;
		TransformRanges( m_displayAreaValueMax - (m_settings.m_valueRangeMin - m_displayAreaValueMin), m_displayAreaValueMin, m_displayAreaValueMax, m_graphRect.GetTop(), m_graphRect.GetBottom(), minValue, false );
		TransformRanges( m_displayAreaValueMax - (m_settings.m_valueRangeMax - m_displayAreaValueMin), m_displayAreaValueMin, m_displayAreaValueMax, m_graphRect.GetTop(), m_graphRect.GetBottom(), maxValue, false );
		DrawLine( wxPoint( timeZeroX, minValue ), wxPoint( timeOneX, minValue ), wxColour( 128, 0, 0, 128 ) );
		DrawLine( wxPoint( timeZeroX, maxValue ), wxPoint( timeOneX, maxValue ), wxColour( 128, 0, 0, 128 ) );

		m_dc->DestroyClippingRegion();
	}

	return curveArea;
}


wxRect CCurveEditDrawer::DrawGridNum( Bool enableMetricsBar, const wxColour& colorTransparent, Bool isUnit ) const
{
	// Draw grid with no boundaries.

	// Set font
	const Int32 fontSize = 8;
	if ( enableMetricsBar )
	{
		m_dc->SetFont(wxFont(fontSize, wxMODERN, wxNORMAL, wxBOLD, 0));
		m_dc->SetTextBackground( wxColour (0, 0, 0, 0) );
	}

	// Calculate curve area
	wxRect curveArea = m_graphRect;
	if ( enableMetricsBar )
	{
		curveArea.height -= m_dc->GetTextExtent( TXT("0123456789.") ).y;
	}

	// Draw vertical lines and text
	if ( !m_graphRect.IsEmpty() )
	{
		const Int32 lineWidthOrigin = 2;
		const Int32 lineWidthWhole = isUnit ? 2 : 1;
		const Int32 lineWidth = 1;

		wxColour lineColorOrigin= isUnit ? wxColour( 192, 0, 0 ) : wxColour( 192, 192,   0 );
		wxColour lineColorWhole = isUnit ? wxColour( 192, 0, 0 ) : wxColour( 192, 192, 192 );
		wxColour lineColor		= wxColour(  96,  96,  96 );


		m_dc->SetClippingRegion( m_graphRect );

		const Int32 appearPixelThreshold	= 50;
		const Int32 appearPixelRange		= 10;

		Float timePerPixel		= (m_displayAreaTimeMax - m_displayAreaTimeMin) / m_graphRect.width;
		Float appearTimeStep	= powf( 2.0f, ceilf( logf(appearPixelThreshold * timePerPixel) / logf(2.0f) ) );

		if ( isUnit )
		{
			appearTimeStep = ::Min( appearTimeStep, 1.0f );
		}

		Float gridTimeMin = m_displayAreaTimeMin - ( ( m_displayAreaTimeMin >= 0 ) ? fmod(m_displayAreaTimeMin,appearTimeStep) : (appearTimeStep - fmod(-m_displayAreaTimeMin, appearTimeStep)) );
		Float gridTimeMax = m_displayAreaTimeMax + appearTimeStep;

		for ( Float gridTime = gridTimeMin; gridTime <= gridTimeMax; gridTime += appearTimeStep )
		{
			if ( isUnit && Abs(gridTime - 0.5f) > 0.5f + 0.01f * appearTimeStep )
				continue;


			Int32 x = (Int32) Lerp<Float>( NormalizeRange( gridTime, m_displayAreaTimeMin, m_displayAreaTimeMax ), m_graphRect.GetLeft(), m_graphRect.GetRight() );
			if ( x < curveArea.GetLeft() || x > curveArea.GetRight() )
				continue;

			// ace_todo!!! calculate alpha :)
			const Float alpha = 1.f;
			if ( alpha <= 0.f )
				continue;

			// Draw line
			const bool isOrigin = Abs(gridTime) < 0.01f * appearTimeStep;
			const bool isWhole = Abs(gridTime) - floorf( Abs(gridTime) ) < 0.01f * appearTimeStep;
			wxColour currColor = LerpWxColor( alpha, colorTransparent, isOrigin ? lineColorOrigin : isWhole ? lineColorWhole : lineColor );
			Int32		 currWidth = isOrigin ? lineWidthOrigin : isWhole ? lineWidthWhole : lineWidth;

			DrawLine( wxPoint( x, m_graphRect.GetTop() ), wxPoint( x, m_graphRect.GetBottom() ), currColor, currWidth );

			// Draw text
			if ( enableMetricsBar )
			{
				// Build text
				wxString text;
				text.Printf(TXT("%.3f"), gridTime);

				// Draw text
				wxSize  textExtent = m_dc->GetTextExtent( text );
				wxPoint textCorner = wxPoint ( x - textExtent.x / 2, m_graphRect.GetBottom() - textExtent.y );
				m_dc->SetTextForeground( wxColour(255, 255, 255, 255) );
				m_dc->DrawText(text, textCorner);
			}
		}

		m_dc->DestroyClippingRegion();
	}

	// Draw horizontal lines and text

	if ( !curveArea.IsEmpty() )
	{
		const Int32 lineWidthOrigin = 2;
		const Int32 lineWidthWhole = 1;
		const Int32 lineWidth = 1;

		wxColour lineColorOrigin= wxColour( 192, 192,   0 );
		wxColour lineColorWhole = wxColour( 192, 192, 192 );
		wxColour lineColor		= wxColour(  96,  96,  96 );

		m_dc->SetClippingRegion( curveArea );
		

		Int32 lineLeft;
		Int32 lineRight;
		Int32 valueDisplayX, valueDisplayAlign;
		if ( isUnit )
		{
			const Int32 timeZeroX = (Int32) Lerp<Float>( NormalizeRange( 0.0f, m_displayAreaTimeMin, m_displayAreaTimeMax ), m_graphRect.GetLeft(), m_graphRect.GetRight() );
			const Int32 timeOneX  = (Int32) Lerp<Float>( NormalizeRange( 1.0f, m_displayAreaTimeMin, m_displayAreaTimeMax ), m_graphRect.GetLeft(), m_graphRect.GetRight() );

			lineLeft = ::Max( timeZeroX, m_graphRect.GetLeft() );
			lineRight = ::Min( timeOneX, m_graphRect.GetRight() );

			if ( timeZeroX > m_graphRect.GetLeft() + m_graphRect.GetWidth() / 4 )
			{
				valueDisplayX = timeZeroX - 2;
				valueDisplayAlign = -1;
			}
			else
			{
				valueDisplayX = ::Max( m_graphRect.GetLeft(), timeZeroX ) + 2;
				valueDisplayAlign = 1;
			}
		}
		else
		{
			lineLeft = m_graphRect.GetLeft();
			lineRight = m_graphRect.GetRight();
			valueDisplayX = m_graphRect.GetLeft();
			valueDisplayAlign = 1;
		}

		const Int32 appearPixelThreshold	= 30;
		const Int32 appearPixelRange		= 10;

		Float valuePerPixel		= (m_displayAreaValueMax - m_displayAreaValueMin) / m_graphRect.height;
		Float appearValueStep	= powf( 2.0f, ceilf( logf(appearPixelThreshold * valuePerPixel) / logf(2.0f) ) );

		Float gridValueMin = m_displayAreaValueMin - ( ( m_displayAreaValueMin >= 0 ) ? fmod(m_displayAreaValueMin,appearValueStep) : (appearValueStep - fmod(-m_displayAreaValueMin, appearValueStep)) );
		Float gridValueMax = m_displayAreaValueMax + appearValueStep;

		for ( Float gridValue = gridValueMin; gridValue <= gridValueMax; gridValue += appearValueStep )
		{
			Int32 y = (Int32) Lerp<Float>( NormalizeRange( gridValue, m_displayAreaValueMin, m_displayAreaValueMax ), m_graphRect.GetBottom(), m_graphRect.GetTop() );
			if ( y < curveArea.GetTop() - fontSize || y > curveArea.GetBottom() + fontSize )
				continue;

			// ace_todo!!! calculate alpha :)
			const Float alpha = 1.f;
			if ( alpha <= 0.f )
				continue;

			// Draw line
			const bool isOrigin = Abs(gridValue) < 0.01f * appearValueStep;
			const bool isWhole = Abs(gridValue) - floorf( Abs(gridValue) ) < 0.01f * appearValueStep;
			wxColour currColor = LerpWxColor( alpha, colorTransparent, isOrigin ? lineColorOrigin : lineColor );
			Int32		 currWidth = isOrigin ? lineWidthOrigin : isWhole ? lineWidthWhole : lineWidth;

			DrawLine( wxPoint( lineLeft, y ), wxPoint( lineRight, y ), currColor, currWidth );

			// Draw text
			if ( enableMetricsBar )
			{
				// Build text
				wxString text;
				text.Printf(TXT("%.4f"), gridValue);

				// Draw text
				wxSize  textExtent = m_dc->GetTextExtent( text );
				Int32 textX = valueDisplayAlign > 0 ? valueDisplayX : valueDisplayX - textExtent.x;

				wxPoint textCorner = wxPoint ( textX, y - textExtent.y );

				m_dc->SetTextForeground( wxColour(255, 255, 255, 255) );
				m_dc->DrawText(text, textCorner);
			}
		}

		m_dc->DestroyClippingRegion();
	}

	return curveArea;
}

Bool CCurveEditDrawer::IsPointSelected( Uint32 index ) const
{
	const Uint32 pointCount = m_selectedPoints.Size();
	for( Uint32 i=0; i<pointCount; ++i )
	{
		if( m_selectedPoints[i].m_index == index )
		{
			return true;
		}
	}
	return false;
}

void CCurveEditDrawer::UpdateSelectedPointsPosition()
{
	const Uint32 pointCount = m_selectedPoints.Size();
	for( Uint32 i=0; i<pointCount; ++i )
	{
		FillControlPointPosition( m_selectedPoints[i] );
	}
}

void CCurveEditDrawer::AddDistanceMultipier( Int32 value )
{
	if( value == 0 )
	{
		return;
	}

	UpdateSelectedPointsPosition();

	const Uint32 pointCount = m_selectedPoints.Size();
	for( Uint32 i=0; i<pointCount; ++i )
	{
		if( m_currentSelection.m_index == m_selectedPoints[i].m_index )
		{
			continue;
		}

		Float delta = m_selectedPoints[i].m_position.x - m_currentSelection.m_position.x;

		SCurveEditControlPoint& pointRef = m_selectedPoints[i];
		Vector newValue = pointRef.m_curve->GetValueAtIndex( pointRef.m_index );
		Float newTime;

		//
		if( value > 0 )
		{
			TransformScalarPointInverse( m_selectedPoints[i].m_position + wxPoint( delta, 0 ), newTime, newValue.W, true );
		}
		else if( value < 0 )
		{
			TransformScalarPointInverse( m_selectedPoints[i].m_position - wxPoint( ( delta / 2 ), 0 ), newTime, newValue.W, true );
		}

		// Clamp to [0,1) for loop/day-time curves.
		if ( m_curve->IsLoop() || m_curve->UseDayTime() )
		{
			if ( newTime < 0 || newTime > TIME_MAXIMUM  )
			{
				return;
			}
		}

		// Set new time, update the selection index in case it changed.
		pointRef.m_index = pointRef.m_curve->SetPointTime( pointRef.m_index, newTime );
		pointRef.m_curve->SetValue( pointRef.m_index, newValue );
	}
}

void CCurveEditDrawer::UpdateSelectionArea( const wxPoint& pos )
{
	m_selectionAreaEndPoint = pos;
}


//////////////////////////////////////////////////////////////////////////

CCurveEditDrawerScalar::CCurveEditDrawerScalar()
{
}

void CCurveEditDrawerScalar::Reset( const SCurveEditDrawerSettings& settings )
{
	CCurveEditDrawer::Reset( settings );

	m_backgroundColorActive			= wxColour(  64,  64,  64, 255 );
	m_backgroundColorInactive		= wxColour( 128, 128, 128, 255 );
	m_backgroundTypesBorderColor	= wxColour( 255, 255,   0, 255 );
	m_lineColor						= wxColour( 255, 255, 255, 255 );
	m_keyPointsColor				= wxColour( 255, 255, 255, 255 );
	m_keyPointsColorSelected		= wxColour( 255,   0, 255, 255 );
	m_keyPointsColorLastSelected	= wxColour( 192, 192,  64, 255 );
	m_keyPointsBorder				= wxColour(   0,   0,   0, 255 );
	m_keyPointsBorderMouseOver		= wxColour( 255, 255, 255, 255 );
	m_keyPointsLastSelectedBorder	= wxColour( 127,   0,   0, 255 );
	m_keyPointsRadius				= 6.f;

	m_lineColorRangeExceeded		= wxColour( 255,   0,   0, 255 );
	m_keyPointsBorderRangeExceeded	= wxColour( 255,   0,   0, 255 );
}


void CCurveEditDrawerScalar::MoveSelection( const wxPoint& pos )
{
	if ( m_currentSelection.m_type == CPT_Value )
	{
		wxPoint delta = pos - m_currentSelection.m_position;

		Uint32 pointCount = m_selectedPoints.Size();
		for( Uint32 i=0; i<pointCount; ++i )
		{
			SCurveEditControlPoint& pointRef = m_selectedPoints[i];

			// Want to keep the same vector value (if there was one), just update the scalar component...
			Vector newValue = pointRef.m_curve->GetValueAtIndex( pointRef.m_index );
			Float newTime;
			TransformScalarPointInverse( m_selectedPoints[i].m_position + delta, newTime, newValue.W, true );

			// Clamp to [0,1) for loop/day-time curves.
			if ( m_curve->IsLoop() || m_curve->UseDayTime() )
			{
				if ( newTime < 0 ) newTime = 0;
				if ( newTime > TIME_MAXIMUM ) newTime = TIME_MAXIMUM;
			}

			// Set new time, update the selection index in case it changed.
			pointRef.m_index = pointRef.m_curve->SetPointTime( pointRef.m_index, newTime );
			pointRef.m_curve->SetValue( pointRef.m_index, newValue );
		}
	}
	else if ( m_currentSelection.m_type == CPT_Tangent )
	{
		Uint32 baseIdx = m_currentSelection.m_index / 2;
		Uint32 tanIdx = m_currentSelection.m_index % 2;

		Float baseTime = m_currentSelection.m_curve->GetCurveData().GetTimeAtIndex( baseIdx );
		Float baseValue = m_currentSelection.m_curve->GetFloatValueAtIndex( baseIdx );

		Float tanTime, tanValue;
		TransformScalarPointInverse( pos, tanTime, tanValue, true );

		m_currentSelection.m_curve->SetTangentValue( baseIdx, tanIdx, Vector( tanTime - baseTime, tanValue - baseValue, 0, 0 ) );
	}
}


void CCurveEditDrawerScalar::FillControlPointPosition( SCurveEditControlPoint& point ) const
{
	if ( point.m_type == CPT_Tangent )
	{
		Int32 i = point.m_index / 2;
		Int32 tanIdx = point.m_index % 2;

		Float time = m_curve->GetCurveData().GetTimeAtIndex(i);
		Float value = m_curve->GetFloatValueAtIndex( i );
		Vector tanVec = m_curve->GetTangentValue( i, tanIdx );

		TransformScalarPoint( time + tanVec.X, value + tanVec.Y, point.m_position, true );
	}
	else if (point.m_type == CPT_Value )
	{
		if( m_curve->GetCurveData().Size() > 0 )
		{
			Float time = m_curve->GetCurveData().GetTimeAtIndex(point.m_index);
			Float value = m_curve->GetFloatValueAtIndex( point.m_index );
			TransformScalarPoint( time, value, point.m_position, true );
		}
	}
}


void CCurveEditDrawerScalar::Draw() const
{
	if ( m_fullRect.IsEmpty() ) 
	{
		return;
	}

	// Clip to draw rect.
	m_dc->SetClippingRegion( m_graphRect );

	// Day-Time curves are only active in a certain range.
	if ( m_curve->UseDayTime() )
	{
		// Fill background
		DrawFilledRectangle( m_graphRect, m_backgroundColorInactive );

		// Draw active background
		Int32 activeX1, activeX2;
		TransformRanges( 0, m_displayAreaTimeMin, m_displayAreaTimeMax, m_graphRect.GetLeft(), m_graphRect.GetRight(), activeX1, true );
		TransformRanges( TIME_MAXIMUM, m_displayAreaTimeMin, m_displayAreaTimeMax, m_graphRect.GetLeft(), m_graphRect.GetRight(), activeX2, true );
		wxRect activeRect( activeX1, m_graphRect.GetTop(), activeX2 - activeX1, m_graphRect.GetHeight() );
		DrawFilledRectangle( activeRect, m_backgroundColorActive );
	}
	// Other curves are active everywhere.
	else
	{
		DrawFilledRectangle( m_graphRect, m_backgroundColorActive );
	}

	// Draw selection range
	if( SSimpleCurve::s_graphGlobalSelectionEnable == true )
	{
		DrawTimeSelection( SSimpleCurve::s_graphMinTimeSelection, SSimpleCurve::s_graphMaxTimeSelection );
	}
	
	//
	if( m_isSelectionAreaActive == true )
	{
		wxRect backRect( m_selectionAreaStartPoint, m_selectionAreaEndPoint );
		const wxColour colorBorder( 0, 0, 255);
		const wxColour colorInside( 0, 162, 232 );
		DrawFilledRectangle( backRect, colorInside );
		DrawRectangle( backRect, colorBorder );
	}

	// Draw grid
	wxRect curveRect = m_graphRect;
	if ( m_settings.m_enableGrid )
	{
		curveRect = DrawGrid( m_settings.m_enableGridMetrics, m_backgroundColorActive );
	}

	// Set curve clipping region
	if ( !curveRect.IsEmpty() )
	{
		m_dc->SetClippingRegion( curveRect );
	}

	// Draw current time preview line
	if ( m_curve->UseDayTime() && m_settings.m_enableCurrentTimePreview && SSimpleCurve::s_graphTimeDisplayEnable )
	{
		DrawTimePreview( SSimpleCurve::s_graphTimeDisplayValue );
	}


	// Draw curve
	if ( m_settings.m_enableCurve && m_lineColor.Alpha() > 0 && !curveRect.IsEmpty() && m_curve->GetNumPoints()>0 )
	{
		wxPoint lastPt;
		Bool haveLastPt = false;
		for ( Int32 column_i=0; column_i<m_graphRect.GetWidth(); ++column_i )
		{
			Int32 x = m_graphRect.GetLeft() + column_i;

			Float time = 0;
			if ( !TransformRanges( x, m_graphRect.GetLeft(), m_graphRect.GetRight(), m_displayAreaTimeMin, m_displayAreaTimeMax, time ) )
			{
				haveLastPt = false;
				continue;
			}

			// If this curve has limited range, skip anything out of range.
			if ( ( m_curve->IsLoop() || m_curve->UseDayTime() ) && ( time < 0 || time > TIME_MAXIMUM ) )
			{
				haveLastPt = false;
				continue;
			}

			wxPoint thisPt;
			Float currentValue = m_curve->GetFloatValue( time );
			TransformScalarPoint( time, currentValue, thisPt, true );

			if ( haveLastPt )
			{
				if ( currentValue > m_settings.m_valueRangeMax || currentValue < m_settings.m_valueRangeMin )
				{
					DrawLine( lastPt, thisPt, m_lineColorRangeExceeded, 2 );
				}
				else
				{
					DrawLine( lastPt, thisPt, m_lineColor, 2 );
				}
			}

			lastPt = thisPt;
			haveLastPt = true;
		}
	}

	// Draw key points
	if ( m_settings.m_enableKeypoints && !curveRect.IsEmpty() )
	{	
		for ( Uint32 entry_i=0; entry_i<m_curve->GetNumPoints(); ++entry_i )
		{
			Float time, value;
			time = m_curve->GetCurveData().GetTimeAtIndex(entry_i);
			value = m_curve->GetFloatValueAtIndex( entry_i );
			
			wxPoint point;
			if ( !TransformScalarPoint( time, value, point ) )
				continue;

			wxColour currColor, borderColor;

			GetControlPointColour( entry_i, currColor, borderColor );

			if ( value > m_settings.m_valueRangeMax || value < m_settings.m_valueRangeMin )
			{
				borderColor = m_keyPointsBorderRangeExceeded;
			}

			DrawControlPoint( point, borderColor, currColor );
		}
	}

	// if the curve is using tangents, draw those.
	if ( m_curve->UsesTangents() && m_settings.m_enableCurve && !curveRect.IsEmpty() && m_curve->GetNumPoints()>0 )
	{
		for ( Uint32 i = 0; i < m_curve->GetNumPoints(); ++i )
		{
			Float time = m_curve->GetCurveData().GetTimeAtIndex(i);
			Float value = m_curve->GetFloatValueAtIndex( i );

			wxPoint ctrlPt;
			if ( !TransformScalarPoint( time, value, ctrlPt ) )
			{
				continue;
			}

			for ( Uint32 tanIdx = 0; tanIdx < 2; ++tanIdx )
			{
				if ( ShouldDrawTangent( i, tanIdx ) )
				{
					Vector tanVec = m_curve->GetTangentValue( i, tanIdx );

					wxPoint tanPt;
					TransformScalarPoint( time + tanVec.X, value + tanVec.Y, tanPt, true );

					DrawLine( ctrlPt, tanPt, m_lineColor, 1 );

					wxColour currColor, borderColor;
					GetTangentPointColour( tanIdx + i * 2, currColor, borderColor );

					DrawTangentPoint( tanPt, borderColor, currColor );
				}
			}
		}
	}


	// Draw selected key point value
	if ( m_currentSelection.m_type == CPT_Value && m_currentSelection.m_index >= 0 && m_currentSelection.m_index < (Int32)m_curve->GetNumPoints() )
	{
		Float time = m_curve->GetCurveData().GetTimeAtIndex( m_currentSelection.m_index );
		Float value = m_curve->GetFloatValueAtIndex( m_currentSelection.m_index );

		const Int32      border		= 2;
		const wxPoint  displace		= wxPoint (3, 3);
		const wxColour colorBack	= wxColour (  0,  0,  0);
		const wxColour colorBorder	= wxColour (255,255,255);
		const wxColour colorText	= wxColour (255,255,  0);

		wxString textValue;
		textValue.Printf( TXT("%f"), value );

		wxString textTime;
		if ( m_curve->UseDayTime() )
		{
			BuildTimeString( time, textTime );
		}
		else
		{
			textTime.Printf( "%f", time );
		}

		m_dc->SetFont(wxFont(8, wxMODERN, wxNORMAL, wxBOLD, 0));
		m_dc->SetTextForeground( colorText );
		m_dc->SetTextBackground( colorBack );

		wxSize  extentValue = m_dc->GetTextExtent( textValue );
		wxSize  extentTime  = m_dc->GetTextExtent( textTime );
		wxSize  extent		= wxSize ( 2 * border + Max(extentValue.x, extentTime.x), 2 * border + extentValue.y + extentTime.y );
		wxRect  backRect    = wxRect ( curveRect.x + 1, curveRect.y + 1, extent.x, extent.y );

		DrawFilledRectangle( backRect, colorBack );
		DrawRectangle( backRect, colorBorder );

		m_dc->DrawText( textValue, backRect.GetTopLeft() + wxPoint(border, border));
		m_dc->DrawText( textTime,  backRect.GetTopLeft() + wxPoint(border, border+extentValue.y) );
	}

	// Destroy clipping region
	if ( !curveRect.IsEmpty() )
	{
		m_dc->DestroyClippingRegion();
	}
}


void CCurveEditDrawerScalar::GetControlPointColour( Uint32 idx, wxColour& base, wxColour& border ) const
{
	if ( m_currentSelection.m_type == CPT_Value && IsPointSelected( idx ) == true )
	{
		if ( m_selectionActive )
			base = m_keyPointsColorSelected;
		else
			base = m_keyPointsColorLastSelected;
	}
	else
		base = m_keyPointsColor;

	if ( m_currentMouseOver.m_type == CPT_Value && (int)idx == m_currentMouseOver.m_index )
		border = m_keyPointsBorderMouseOver;
	else if( m_currentSelection.m_index == idx && m_selectedPoints.Size() > 0 )
		border = m_keyPointsLastSelectedBorder;
	else
		border = m_keyPointsBorder;
}

void CCurveEditDrawerScalar::GetTangentPointColour( Uint32 idx, wxColour& base, wxColour& border ) const
{
	if ( m_currentSelection.m_type == CPT_Tangent && (Int32)idx == m_currentSelection.m_index )
	{
		if ( m_selectionActive )
			base = m_keyPointsColorSelected;
		else
			base = m_keyPointsColorLastSelected;
	}
	else
		base = m_keyPointsColor;

	if ( m_currentMouseOver.m_type == CPT_Tangent && (int)idx == m_currentMouseOver.m_index )
		border = m_keyPointsBorderMouseOver;
	else
		border = m_keyPointsBorder;
}



void CCurveEditDrawerScalar::DrawControlPoint( const wxPoint& point, const wxColour& borderColor, const wxColour &baseColor ) const
{
	const Int32 size = GetKeyPointsRadius();
	wxColour shadowColor( baseColor.Red() * 3 / 4, baseColor.Green() * 3 / 4, baseColor.Blue() * 3 / 4 );
	DrawFrameCircle( point, size - 1, baseColor, shadowColor, 2 );
	DrawCircle( point, size, borderColor, 2 );
}

void CCurveEditDrawerScalar::DrawTangentPoint( const wxPoint& point, const wxColour& borderColor, const wxColour &baseColor ) const
{
	const Int32 size = Max( GetKeyPointsRadius() - 2, 0 );
	wxColour shadowColor( baseColor.Red() * 3 / 4, baseColor.Green() * 3 / 4, baseColor.Blue() * 3 / 4 );
	DrawFrameCircle( point, size - 1, baseColor, shadowColor, 2 );
	DrawCircle( point, size, borderColor, 1 );
}

bool CCurveEditDrawerScalar::IsPosOnControlPoint( const wxPoint &pos, const SCurveEditControlPoint &controlPoint ) const
{
	Int32 radius = GetKeyPointsRadius();
	return radius > 0 && IsPointInsideCircle(controlPoint.m_position, radius, pos);
}


bool CCurveEditDrawerScalar::GenerateNewControlPoint( const wxPoint& pos, Float& outTime, Vector& outValue ) const
{
	Float ptX, ptY;
	if ( !TransformScalarPointInverse(pos, ptX, ptY) )
		return false;
	if ( ptX != m_curve->AdaptTime(ptX) || m_curve->GetIndex(ptX) != -1 )
		return false;

	outTime = ptX;
	// Keep interpolated Color value, just replace the scalar (W).
	outValue.Set3( m_curve->GetValue(ptX) );
	outValue.W = ptY;

	return true;
}

Int32 CCurveEditDrawerScalar::FindNearestEntry( const wxPoint& pt, Int32 *outDist, Bool *outIsTangent )
{
	Bool bestTangent = false;
	Int32 bestEntryIndex = -1;
	Float bestEntryDist  = ::NumericLimits< Float >().Max();
	for ( Int32 i=0; i<(Int32)m_curve->GetNumPoints(); ++i )
	{
		Float curveTime = m_curve->GetCurveData().GetTimeAtIndex(i);
		Float curveValue = m_curve->GetFloatValueAtIndex(i);

		for ( Int32 tanIdx = 0; tanIdx < 2; ++tanIdx )
		{
			// Can only select drawn tangent points.
			if ( ShouldDrawTangent( i, tanIdx ) )
			{
				Vector tanValue = m_curve->GetTangentValue(i, tanIdx);

				wxPoint tanPt;
				if ( !TransformScalarPoint( curveTime + tanValue.X, curveValue + tanValue.Y, tanPt ) )
					continue;

				wxPoint diff = tanPt - pt;
				// Work with Float, to make overflow less likely (not very likely anyways, but let's be sure). Perfect precision at great distances is not important.
				Float currDist = (Float)diff.x*diff.x + (Float)diff.y*diff.y;
				if (currDist < bestEntryDist)
				{	
					bestEntryIndex = i * 2 + tanIdx;
					bestEntryDist  = currDist;
					bestTangent = true;
				}
			}
		}

		wxPoint ctlPt;
		if ( !TransformScalarPoint( curveTime, curveValue, ctlPt ) )
			continue;

		wxPoint diff = ctlPt - pt;
		// Work with Float, to make overflow less likely (not very likely anyways, but let's be sure). Perfect precision at great distances is not important.
		Float currDist = (Float)diff.x*diff.x + (Float)diff.y*diff.y;
		if (currDist < bestEntryDist)
		{	
			bestEntryIndex = i;
			bestEntryDist  = currDist;
			bestTangent = false;
		}
	}
	if ( -1 != bestEntryIndex )
	{
		if ( outDist )
			*outDist = (Int32)(sqrt(bestEntryDist));
		if ( outIsTangent )
			*outIsTangent = bestTangent;
	}
	return bestEntryIndex;
}

Int32 CCurveEditDrawerScalar::GetKeyPointsRadius() const
{
	if ( m_fullRect.IsEmpty() || m_keyPointsRadius < 1 )
		return 0;
	Float pixelsPerHour = m_fullRect.width / (24 * (m_displayAreaTimeMax - m_displayAreaTimeMin));
	return (Int32) (m_keyPointsRadius * Clamp( (pixelsPerHour - 5.f) / 5.f, 0.f, 1.f ));
}

Bool CCurveEditDrawerScalar::ShouldDrawTangent( Uint32 ctlIdx, Uint32 tanIdx ) const
{
	return	m_curve->UsesTangents() &&
			m_curve->GetTangentType( ctlIdx, tanIdx ) == CST_Bezier ||
			m_curve->GetTangentType( ctlIdx, tanIdx ) == CST_BezierSmooth ||
			m_curve->GetTangentType( ctlIdx, tanIdx ) == CST_BezierSmoothSymertric ||
			m_curve->GetTangentType( ctlIdx, tanIdx ) == CST_BezierSmoothLyzwiaz ||
			m_curve->GetTangentType( ctlIdx, tanIdx ) == CST_Bezier2D;
}


//////////////////////////////////////////////////////////////////////////


CCurveEditDrawerColor::CCurveEditDrawerColor()
{
}

void CCurveEditDrawerColor::Reset( const SCurveEditDrawerSettings& settings )
{
	CCurveEditDrawer::Reset( settings );

	const Int32 keyBarSize = 10;

	m_backgroundColorActive			= wxColour ( 64,  64,  64, 255);
	m_backgroundColorInactive		= wxColour (128, 128, 128, 255);
	m_keysBarHeight					= m_settings.m_enableKeypoints ? keyBarSize : 0;
	m_keysBarBackColor				= wxColour (  0,  32,   0, 255);
	m_keysBorderColor				= wxColour (255, 255, 255, 255);
	m_keysBorderColorSelected		= wxColour (255,   0, 255, 255);
	m_keysBorderColorLastSelected	= wxColour (192, 192,  64, 255);
	m_keysBorderColorLastSelected	= wxColour (192, 192, 64,  255);
	m_keysWidth						= keyBarSize;
	m_keysHeight					= keyBarSize;
}


void CCurveEditDrawerColor::SetFullRect( const wxRect& fullRect )
{
	CCurveEditDrawer::SetFullRect( fullRect );

	m_keysBarRect = m_graphRect;
	m_keysBarRect.y += m_graphRect.height;
	m_keysBarRect.height = m_keysBarHeight;
}


void CCurveEditDrawerColor::MoveSelection( const wxPoint& pos )
{
	if ( m_currentSelection.m_type == CPT_Value )
	{
		wxPoint delta = pos - m_currentSelection.m_position;

		Uint32 pointCount = m_selectedPoints.Size();
		for( Uint32 i=0; i<pointCount; ++i )
		{
			SCurveEditControlPoint& pointRef = m_selectedPoints[i];

			// Want to keep the same vector value (if there was one), just update the scalar component...
			Vector newValue = pointRef.m_curve->GetValueAtIndex( pointRef.m_index );
			Float newTime;
			TransformScalarPointInverse( m_selectedPoints[i].m_position + delta, newTime, newValue.W, true );

			// Clamp to [0,1) for loop/day-time curves.
			if ( m_curve->IsLoop() || m_curve->UseDayTime() )
			{
				if ( newTime < 0 ) newTime = 0;
				if ( newTime > TIME_MAXIMUM ) newTime = TIME_MAXIMUM;
			}

			// Set new time, update the selection index in case it changed.
			pointRef.m_index = pointRef.m_curve->SetPointTime( pointRef.m_index, newTime );
		}
	}
}


void CCurveEditDrawerColor::FillControlPointPosition( SCurveEditControlPoint& point ) const
{
	if (point.m_type == CPT_Value )
	{
		Float time = m_curve->GetCurveData().GetTimeAtIndex(point.m_index);
		TransformScalarPoint( time, 0.0f, point.m_position, true );
		point.m_position.y = 0;
	}
}


void CCurveEditDrawerColor::Draw() const
{
	if ( m_fullRect.IsEmpty() ) 
	{
		return;
	}

	// Draw inactive background
	DrawFilledRectangle( m_graphRect, m_backgroundColorInactive );

	// Draw active area
	if ( !m_graphRect.IsEmpty() )
	{
		if ( m_curve->GetNumPoints() > 0 )
		{
			// ace_optimize: this is suboptimal
			for ( Int32 column_i=0; column_i<m_graphRect.GetWidth(); ++column_i )
			{
				Float time = 0;
				if ( !TransformRanges( m_graphRect.x + column_i, m_graphRect.GetLeft(), m_graphRect.GetRight(), m_displayAreaTimeMin, m_displayAreaTimeMax, time) )
					continue;
				if ( time < 0 || time > TIME_MAXIMUM )
					continue;

				Vector colorVec = m_curve->GetValue( time );
				DrawLine( wxPoint( m_graphRect.x + column_i, m_graphRect.GetTop() ), wxPoint( m_graphRect.x + column_i, m_graphRect.GetBottom() ), wxColour( colorVec.X, colorVec.Y, colorVec.Z ) );
			}
		}
		else
		{
			DrawFilledRectangle( m_graphRect, m_backgroundColorActive );
		}
	}

	// Draw current time preview line
	if ( m_settings.m_enableCurrentTimePreview && SSimpleCurve::s_graphTimeDisplayEnable )
	{
		DrawTimePreview( SSimpleCurve::s_graphTimeDisplayValue );
	}

	// Draw keys bar
	if ( !m_keysBarRect.IsEmpty() )
	{
		// Clear background
		DrawFilledRectangle( m_keysBarRect, m_keysBarBackColor );

		// Draw keys
		for ( Uint32 entry_i=0; entry_i<m_curve->GetNumPoints(); ++entry_i )
		{
			wxPoint points[3];
			Float time = m_curve->GetCurveData().GetTimeAtIndex(entry_i);
			if ( !CalcTriangleKeyTopVertex( time, points[0] ) )
				continue;

			Bool isSelected = m_currentSelection.m_type == CPT_Value && m_currentSelection.m_index == entry_i;

			wxColour currColor;
			if ( isSelected )
			{
				if ( m_selectionActive )
					currColor = m_keysBorderColorSelected;
				else
					currColor = m_keysBorderColorLastSelected;
			}
			else
				currColor = m_keysBorderColor;

			points[1] = points[0] + wxPoint (  m_keysWidth/2, m_keysHeight );
			points[2] = points[0] + wxPoint ( -m_keysWidth/2, m_keysHeight );
			if ( isSelected )
			{
				m_dc->SetPen( wxPen( currColor ) );
				m_dc->SetBrush( wxBrush( currColor ) );
				m_dc->DrawPolygon(3, points);
			}
			else
			{
				DrawLine( points[0], points[1], currColor );
				DrawLine( points[1], points[2], currColor );
				DrawLine( points[2], points[0], currColor );
			}
		}
	}
}

bool CCurveEditDrawerColor::IsPosOnControlPoint( const wxPoint &pos, const SCurveEditControlPoint &controlPoint ) const
{
	wxPoint triangleTopVertex = wxPoint( controlPoint.m_position.x, m_keysBarRect.y );
	return	IsPointInsideTriangle( triangleTopVertex, m_keysWidth, m_keysHeight, pos );
}

wxRect CCurveEditDrawerColor::GetGraphRect( const wxRect &fullRect ) const
{
	wxRect graphRect = fullRect;
	graphRect.height -= m_keysBarHeight;
	return graphRect;
}

bool CCurveEditDrawerColor::IsInsideColorPickArea( const wxPoint &pos ) const
{
	return m_graphRect.Contains( pos );
}

bool CCurveEditDrawerColor::GenerateNewControlPoint( const wxPoint& pos, Float& outTime, Vector& outValue ) const
{
	Float time;
	if ( pos.y < m_fullRect.GetTop() || pos.y > m_fullRect.GetBottom() )
		return false;
	if ( !TransformRanges(pos.x, m_graphRect.GetLeft(), m_graphRect.GetRight(), m_displayAreaTimeMin, m_displayAreaTimeMax, time) )
		return false;
	if ( time != m_curve->AdaptTime(time) )
		return false;
	if ( -1!=m_curve->GetIndex(time) )
		return false;

	if ( m_curve->IsEmpty() )
	{
		outTime = time;
		outValue = DEFAULT_COLORGRAPH_COLOR.ToVector();
	}
	else
	{
		outTime = time;
		outValue = m_curve->GetValue(time);
	}

	return true;
}

Int32 CCurveEditDrawerColor::FindNearestEntry( const wxPoint& pt, Int32 *outDist, Bool *outIsTangent )
{
	Int32 bestEntryIndex = -1;
	Int32 bestEntryDist  = ::NumericLimits< Int32 >().Max();
	for ( Int32 i=0; i<(Int32)m_curve->GetNumPoints(); ++i )
	{
		Int32 entryX;

		if ( !TransformRanges( m_curve->GetCurveData().GetTimeAtIndex(i), m_displayAreaTimeMin, m_displayAreaTimeMax, m_fullRect.GetLeft(), m_fullRect.GetRight(), entryX ) )
			continue;

		Int32 currDist = Abs(entryX - pt.x);
		if (currDist < bestEntryDist)
		{	
			bestEntryIndex = i;
			bestEntryDist  = currDist;
		}
	}
	if ( -1 != bestEntryIndex )
	{
		if ( outDist )
			*outDist = (Int32)bestEntryDist;
		if ( outIsTangent )
			*outIsTangent = false;
	}
	return bestEntryIndex;
}

bool CCurveEditDrawerColor::CalcTriangleKeyTopVertex( Float time, wxPoint &outPoint ) const
{
	Int32 x = 0;
	if ( !TransformRanges( time, m_displayAreaTimeMin, m_displayAreaTimeMax, m_keysBarRect.GetLeft(), m_keysBarRect.GetRight(), x ) )
		return false;
	outPoint = wxPoint ( x, m_keysBarRect.y );
	return true;
}

