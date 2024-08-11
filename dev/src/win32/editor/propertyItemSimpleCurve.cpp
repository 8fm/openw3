/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "floatValueEditor.h"
#include "timeValueEditor.h"
#include "curveEditDrawer.h"
#include "../../common/core/xmlWriter.h"
#include "../../common/core/xmlReader.h"
#include "../../common/engine/curve.h"

#define wxID_COPY_CURVE				3001
#define wxID_PASTE_CURVE			3002
#define wxID_COPY_POINT				3003
#define wxID_PASTE_POINT			3004
#define wxID_SET_TANGENT_CONSTANT	3005
#define wxID_SET_TANGENT_INTERP		3006
#define wxID_SET_TANGENT_BEZ		3007
#define wxID_SET_TANGENT_BSMOOTH	3008
#define wxID_SET_TANGENT_BSMOOTH_L	3009
#define wxID_SET_TANGENT_BSMOOTH_S	3010
#define wxID_SET_LINEAR				3011
#define wxID_SET_SMOOTH				3012
#define wxID_SET_SEGMENTED			3013
#define wxID_SET_VALUE				3014
#define wxID_SET_TIME				3015
#define wxID_INVERT					3016

///////////////////////////////////////////////////////////////////////////////

struct SGraphEnv
{
	// -----------------------------------
	enum { GRAPHS_BORDER_SIZE	= 3		};
	enum { COLOR_GRAPH_HEIGHT	= 60	};
	enum { SCALAR_GRAPH_HEIGHT	= 180	};
	// -----------------------------------

	SGraphEnv ( CPropertyItemSimpleCurve &propertyItem )
		: graph (NULL)
	{
		Init( propertyItem );
	}

	~SGraphEnv()
	{
		ClearDrawers();
	}

	static Int32 GetBestCollapsedGraphEnvAreaHeight()
	{
		return 18;
	}

	static Int32 GetBestEditModeGraphEnvAreaHeight( const SCurveBase &graph )
	{
		Int32 height = GetBestCollapsedGraphEnvAreaHeight();
		switch ( graph.GetSimpleCurveType() )
		{
		case SCT_Float:			height = GRAPHS_BORDER_SIZE + SCALAR_GRAPH_HEIGHT + GRAPHS_BORDER_SIZE; break;
		case SCT_Color:			height = GRAPHS_BORDER_SIZE + COLOR_GRAPH_HEIGHT + GRAPHS_BORDER_SIZE; break;
		case SCT_ColorScaled:	height = GRAPHS_BORDER_SIZE + COLOR_GRAPH_HEIGHT + GRAPHS_BORDER_SIZE + SCALAR_GRAPH_HEIGHT + GRAPHS_BORDER_SIZE; break;
		default:		ASSERT(!"invalid curve type");
		}
		return height;
	}

	void ClearDrawers()
	{
		for ( Uint32 i = 0; i < graphDrawers.Size(); ++i )
		{
			delete graphDrawers[i];
		}
		graphDrawers.Clear();
	}

	bool IsInit() const
	{
		return NULL!=graph;
	}
	
	bool Init( CPropertyItemSimpleCurve &propertyItem )
	{
		const PropertiesPageDayCycleSettings &editSettings = propertyItem.GetPage()->GetDayCycleEditSettings();
		const bool itemInEditMode = propertyItem.IsInEditionMode();

		Bool editModeChange = ( !IsInit() || isEditMode != itemInEditMode );
		isEditMode = itemInEditMode;

		SCurveBase *oldCurve = graph;
		
		graph = propertyItem.GetCurve();
		if ( NULL == graph )
		{
			ClearDrawers();
			return false;
		}

		Bool isNewCurve = ( oldCurve != graph );

		wxRect fullRect = propertyItem.CalcValueRect();

		if ( isNewCurve || editModeChange )
		{
			ClearDrawers();
		}
		
		// Float and Color just require a single drawer.
		if ( graph->GetValueType() == CVT_Float || graph->GetSimpleCurveType() == SCT_Color )
		{
			// Add scalar graph
			if ( isNewCurve || editModeChange )
			{
				if ( graph->GetValueType() == CVT_Float )
					graphDrawers.PushBack( new CCurveEditDrawerScalar() );
				else
					graphDrawers.PushBack( new CCurveEditDrawerColor() );
			}

			wxRect rect = fullRect;
			// Adapt to edit mode state
			if ( itemInEditMode )
			{
				rect.Inflate( 0, -2 * GRAPHS_BORDER_SIZE );
			}

			graphDrawers[0]->SetFullRect( rect );

			ASSERT( graphDrawers.Size() == 1 );
		}
		// Scaled-Color needs both color and scalar graphs.
		else if ( graph->GetSimpleCurveType() == SCT_ColorScaled )
		{
			// Add color graph
			if ( isNewCurve || editModeChange )
			{
				graphDrawers.PushBack( new CCurveEditDrawerColor() );
			}

			wxRect colorRect = fullRect;

			// Adapt to edit mode state
			if ( itemInEditMode )
			{
				if ( fullRect.height > GRAPHS_BORDER_SIZE + COLOR_GRAPH_HEIGHT  + GRAPHS_BORDER_SIZE )
				{
					// Add scalar graph
					if ( isNewCurve || editModeChange )
					{
						graphDrawers.PushBack( new CCurveEditDrawerScalar() );
					}

					wxRect scalarRect = fullRect;

					// Fit graphs in value area
					colorRect.y			= fullRect.y + GRAPHS_BORDER_SIZE;
					colorRect.height	= COLOR_GRAPH_HEIGHT;
					scalarRect.y		= fullRect.y + GRAPHS_BORDER_SIZE + COLOR_GRAPH_HEIGHT + GRAPHS_BORDER_SIZE;
					scalarRect.height	= (fullRect.y + fullRect.height) - GRAPHS_BORDER_SIZE - scalarRect.y;

					graphDrawers[0]->SetFullRect( colorRect );
					graphDrawers[1]->SetFullRect( scalarRect );

					ASSERT( graphDrawers.Size() == 2 );
				}
				else
				{
					colorRect.Inflate( 0, -2 * GRAPHS_BORDER_SIZE );

					ASSERT( graphDrawers.Size() == 1 );
				}
			}

			graphDrawers[0]->SetFullRect( colorRect );
		}


		SCurveEditDrawerSettings settings;
		settings.m_valueRangeMin = propertyItem.GetRangeMin();
		settings.m_valueRangeMax = propertyItem.GetRangeMax();

		if ( itemInEditMode )
		{
			settings.m_enableCurve				= true;
			settings.m_enableGrid				= true;
			settings.m_enableGridMetrics		= true;
			settings.m_enableCurrentTimePreview	= true;
			settings.m_enableKeypoints			= true;
		}
		else
		{
			settings.m_enableCurve				= false;
			settings.m_enableGrid				= true;
			settings.m_enableGridMetrics		= false;
			settings.m_enableCurrentTimePreview	= false;
			settings.m_enableKeypoints			= false;
		}

		for ( Uint32 i = 0; i < graphDrawers.Size(); ++i )
		{
			graphDrawers[i]->SetCurve( graph );
			InitCurveDrawer( graphDrawers[i], itemInEditMode, settings, editSettings );
		}

		return true;
	}


	void InitCurveDrawer( CCurveEditDrawer* drawer, bool isInEditMode, const SCurveEditDrawerSettings& drawerSettings, const PropertiesPageDayCycleSettings& editSettings )
	{
		drawer->Reset( drawerSettings );
		if ( editSettings.IsValid() )
		{
			if ( drawer->m_curve->UseDayTime() )
			{
				drawer->m_displayAreaTimeMin = editSettings.m_timeMin;
				drawer->m_displayAreaTimeMax = editSettings.m_timeMax;
			}
			
			if ( isInEditMode )
			{
				const Float editScale  = Max( 0.0005f, graph->GetScalarEditScale() );
				const Float editOrigin = graph->GetScalarEditOrigin();
				Float center = editOrigin;
				Float range  = editScale * ( drawer->m_curve->UseDayTime() ? editSettings.m_valueScale : 1.0f );
				drawer->m_displayAreaValueMin = center - range;
				drawer->m_displayAreaValueMax = center + range;
			}
			else
			{
				drawer->m_displayAreaValueMin = -2;
				drawer->m_displayAreaValueMax =  2;
			}
		}
	}

	
	SCurveBase* graph;
	TDynArray< CCurveEditDrawer* > graphDrawers;
	Bool isEditMode;
};

CPropertyItemSimpleCurve::CPropertyItemSimpleCurve ( CEdPropertiesPage* page, CBasePropItem* parent, SCurveBase* curve )
 : TBaseClass( page, parent )
 , m_isInEditionMode ( false )
 , m_ctrlColorPicker ( NULL )
 , m_lastColorPickerEntryTime ( -1 )
 , m_selectedGraph ( -1 )
 , m_mouseMove( 0 )
 , m_mouseLastPos ( 0, 0 )
 , m_curve( curve )
 , m_property( NULL )
{
	m_graphEnv = new SGraphEnv( *this );
}


CPropertyItemSimpleCurve::CPropertyItemSimpleCurve( CEdPropertiesPage* page, CBasePropItem* parent, IProperty* property )
	: TBaseClass( page, parent )
	, m_isInEditionMode ( false )
	, m_ctrlColorPicker ( NULL )
	, m_lastColorPickerEntryTime ( -1 )
	, m_selectedGraph ( -1 )
	, m_mouseMove( 0 )
	, m_mouseLastPos ( 0, 0 )
	, m_curve( NULL )
	, m_property( property )
{
	// Get curve from property.
	STypedObject parentObject = GetParentObject(0);

	// If our parent is also a Curve, then we've been wrapped up in a pointer, so we need to back out a bit.
	// The reason for this: PropertyItemPointer modifies the "parent object hierarchy" to insert the "pointed"
	// type (instead of GetParentObjectType giving the pointer type). And since this has been added inside the
	// Pointer Item, our parent object type shows as our own type.
	// Then the parent's parent's object type is the pointer, so we need that's parent's type to get the thing
	// holding the pointer.
	if ( parentObject.m_class->GetName() == CNAME( CCurve ) || parentObject.m_class->GetName() == CNAME( SSimpleCurve ) )
	{
		parentObject = m_parent->GetParent()->GetParentObject(0);
	}

	if ( parentObject.m_object )
	{
		IRTTIType* type = property->GetType();
		void* propertyObject = property->GetOffsetPtr( parentObject.m_object );

		// If this property is a pointer, we need to extract the "pointed" object and type.
		if ( type->GetType() == RT_Pointer )
		{
			CRTTIPointerType* pointerType = ( CRTTIPointerType* )type;
			type = pointerType->GetPointedType();
			propertyObject = pointerType->GetPointed( propertyObject );
		}
		else if ( type->GetType() == RT_Handle )
		{
			CRTTIHandleType* pointerType = ( CRTTIHandleType* )type;
			type = pointerType->GetPointedType();
			propertyObject = pointerType->GetPointed( propertyObject );
		}

		// Finally, we can pull out the appropriate curve object. Unfortunately, SCurveBase is not RTTI
		// (and probably can't, since that would require CCurve to have two RTTI parent classes, which is
		// not currently supported), so we have to check for both curve types here.
		if ( type->GetName() == CNAME( SSimpleCurve ) )
		{
			m_curve = ((CClass*)type)->CastTo<SSimpleCurve>(propertyObject);
		}
		else if ( type->GetName() == CNAME( CCurve ) )
		{
			m_curve = ((CClass*)type)->CastTo<CCurve>(propertyObject);
		}
	}

	m_graphEnv = new SGraphEnv( *this );
}


CPropertyItemSimpleCurve::~CPropertyItemSimpleCurve()
{
	delete m_graphEnv;
	m_graphEnv = NULL;
}


Int32 CPropertyItemSimpleCurve::GetHeight() const
{
	return IsInEditionMode() && GetCurve() ? SGraphEnv::GetBestEditModeGraphEnvAreaHeight(*GetCurve()) : SGraphEnv::GetBestCollapsedGraphEnvAreaHeight();
}

String CPropertyItemSimpleCurve::GetCaption() const
{
	return m_property ? m_property->GetName().AsString().AsChar() : TXT("Unnamed");
}

void CPropertyItemSimpleCurve::DrawLayout( wxDC& dc )
{	
	TBaseClass::DrawLayout( dc );

	// Calculate the left rect area and clip drawing to it
	wxRect leftRect = m_rect;
	leftRect.width = m_page->GetSplitterPos() - leftRect.x;
	dc.DestroyClippingRegion();
	dc.SetClippingRegion( leftRect );

	// Draw property caption
	String caption = GetCaption();
	if ( !caption.Empty() )
	{
		// Calculate placement
		wxSize extents = dc.GetTextExtent( caption.AsChar() );
		const INT yCenter = ( Min(m_rect.height, SGraphEnv::GetBestCollapsedGraphEnvAreaHeight()) - extents.y ) / 2;

		// Draw caption text
		dc.SetTextForeground( wxColour (0, 0, 0) );
		dc.DrawText( caption.AsChar(), m_rect.x + m_parent->GetLocalIndent() + 15, m_rect.y + yCenter );
	}

	// Done drawing, remove clipping rect
	dc.DestroyClippingRegion();

	// Draw value
	if ( m_graphEnv->IsInit() )
	{
		wxRect  valueRect		= CalcValueRect();
		wxRect	pageRect		= GetPage()->GetRect();
		wxPoint	scrollOffset	= GetPage()->CalcUnscrolledPosition( wxPoint (0, 0) );

		const Int32 clipBorder = 0;
		bool isClipped = valueRect.GetBottom() - scrollOffset.y < clipBorder || valueRect.GetTop() - scrollOffset.y > pageRect.GetHeight() - clipBorder;
		if ( !isClipped )
		{
			// Save font
			wxFont oldFont = dc.GetFont();
		
			dc.SetClippingRegion( valueRect );
			for ( Uint32 graph_i=0; graph_i<m_graphEnv->graphDrawers.Size(); ++graph_i )
			{
				CCurveEditDrawer *drawer = m_graphEnv->graphDrawers[graph_i];
				drawer->SetDC( dc );
				drawer->Draw();
			}
			dc.DestroyClippingRegion();

			// Restore original font
			dc.SetFont(oldFont);
		}
	}
}

wxPoint CPropertyItemSimpleCurve::GetLocalMousePos() const
{
	wxPoint mousePos = wxGetMousePosition();
	if ( GetPage() )
	{
		mousePos = GetPage()->ScreenToClient( mousePos );
		mousePos = GetPage()->CalcUnscrolledPosition( mousePos );
	}
	return mousePos;
}

void CPropertyItemSimpleCurve::Expand()
{
	SetEditionMode(true);
	TBaseClass::Expand();
}

void CPropertyItemSimpleCurve::Collapse()
{
	SetEditionMode(false);
	TBaseClass::Collapse();
}

Bool CPropertyItemSimpleCurve::SerializeXML( IXMLFile& file )
{
    if ( !file.BeginNode( TXT("property") ) )
    {
        return false;
    }

    String name = GetCaption();
    if ( !file.Attribute( TXT("name"), name ) )
    {
        return false;
    }

    if ( name != GetCaption() )
    {
        return false;
    }

    if ( m_curve )
    {
        m_curve->SerializeXML( file );
    }

    file.EndNode();
    return true;
}


void CPropertyItemSimpleCurve::OnCopyCurve( wxCommandEvent& event )
{
    if ( m_curve )
    {
        CXMLWriter writer;
        if ( !m_curve->SerializeXML( writer ) )
        {
            ERR_EDITOR( TXT("Unable to serialize %s simpleCurve object."), GetCaption().AsChar() );
        }
        writer.Flush();

        // Open clipboard
        if ( wxTheClipboard->Open() )
        {
            wxTheClipboard->SetData( new wxTextDataObject( writer.GetContent().AsChar() ) );
            wxTheClipboard->Close();
			wxTheClipboard->Flush();
        }
    }
}

void CPropertyItemSimpleCurve::OnPasteCurve( wxCommandEvent& event )
{
    if ( m_curve )
    {
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
                    return;
                }
            }  
            wxTheClipboard->Close();
        }

        CXMLReader reader( clipboardContent );

        // Seek for the type of the curve
        if ( !reader.BeginNode( TXT("curve") ) )
        {
            return;
        }

        String type;
        if ( !reader.Attribute( TXT("type"), type ) )
        {
            return;
        }
        // Move back to the original node
        reader.EndNode( false );

        Bool mergeCurves = false;
        ESimpleCurveType srcType = SCT_Vector;
        ::FromString< Int32 >( type, *(Int32 *)(&srcType) );
        if ( srcType != m_curve->GetSimpleCurveType() )
        {
            if ( m_curve->CanImportFromType( srcType ) )
            {
                String info = TXT("You are trying to paste the curve of the type that does not match the target type.\n\n");
                String merge = TXT("Press YES if you want to merge source and target curves.\n");
                String replace = TXT("Press NO if you want to replace the target curve with the source one.\n");
                String cancel = TXT("Press Cancel if you are not sure.\n");
                String message = info + merge + replace + cancel;
                String caption = TXT("Curve type mismatch");
                Int32 retVal = wxMessageBox( message.AsChar(), caption.AsChar(), wxYES_NO | wxCANCEL, GetPage() );
                if ( retVal == wxCANCEL )
                {
                    return;
                }

                mergeCurves = ( retVal == wxYES );
            }
            else
            {
                String message = String::Printf( TXT("You are trying to paste the curve of the type that does not match the target type.\n\nConversion is not possible.\n") );
                String caption = TXT("Curve type mismatch");
                wxMessageBox( message.AsChar(), caption.AsChar(), wxOK, GetPage() );
                return;
            }
        }

        if ( !m_curve->SerializeXML( reader, mergeCurves ) )
        {
            ERR_EDITOR( TXT("Unable to deserialize %s simpleCurve object."), GetCaption().AsChar() );
        }

        // Refresh
        GetPage()->Refresh( false );
    }
}

void CPropertyItemSimpleCurve::OnCopyPoints( wxCommandEvent& event )
{
	if ( m_selectedGraph == -1 )
		return;

	CCurveEditDrawer* drawer = m_graphEnv->graphDrawers[m_selectedGraph];
	drawer->CopySelection();
}

void CPropertyItemSimpleCurve::OnPastePoints( wxCommandEvent& event )
{
	if ( m_selectedGraph == -1 )
		return;

	CCurveEditDrawer* drawer = m_graphEnv->graphDrawers[m_selectedGraph];
	drawer->PasteControlPoints( drawer->GetCurrentSelection().m_position );
}


void CPropertyItemSimpleCurve::OnRemovePoints( wxCommandEvent& event )
{
	if( m_selectedGraph == -1 )
	{
		return;
	}
	GraphPreChange( true );
	CCurveEditDrawer* drawer = m_graphEnv->graphDrawers[m_selectedGraph];
	drawer->RemoveControlPoints();
	GraphPostChange( true );
}


void CPropertyItemSimpleCurve::OnSetPointTimeExplicit( wxCommandEvent& event )
{
	if ( m_selectedGraph == -1 )
		return;

	CCurveEditDrawer* drawer = m_graphEnv->graphDrawers[m_selectedGraph];
	if ( !drawer->HasSelection() )
		return;

	const SCurveEditControlPoint& ctrlPt = drawer->GetCurrentSelection();

	if ( ctrlPt.m_index < 0 || ctrlPt.m_index >= (Int32)m_graphEnv->graph->GetNumPoints() )
		return;

	Float controlPtTime = m_graphEnv->graph->GetCurveData().GetTimeAtIndex(ctrlPt.m_index);
	CEdTimeValueEditor editor ( GetPage(), controlPtTime );
	if ( editor.ShowModal() == wxID_OK )
	{
		Float newTime = editor.GetValue();

		if ( m_graphEnv->graph->IsLoop() || m_graphEnv->graph->UseDayTime() )
		{
			newTime = Clamp( newTime, 0.0f, CCurveEditDrawer::TIME_MAXIMUM );
		}

		if ( newTime != controlPtTime )
		{
			drawer->ClearSelection();

			GraphPreChange( true );
		
			Uint32 newIndex = m_graphEnv->graph->SetPointTime( ctrlPt.m_index, newTime );
			drawer->SetSelection( newIndex );
		
			GraphPostChange( true );
			GetPage()->Refresh( false );
		}
	}
}

void CPropertyItemSimpleCurve::OnSetPointValueExplicit( wxCommandEvent& event )
{
	if ( m_selectedGraph == -1 )
		return;

	CCurveEditDrawer* drawer = m_graphEnv->graphDrawers[m_selectedGraph];
	if ( !drawer->HasSelection() )
		return;

	const SCurveEditControlPoint& ctrlPt = drawer->GetCurrentSelection();

	if ( ctrlPt.m_index < 0 || ctrlPt.m_index >= (Int32)m_graphEnv->graph->GetNumPoints() )
		return;


	Vector controlPtValue = m_graphEnv->graph->GetValueAtIndex( ctrlPt.m_index );
	CEdFloatValueEditor editor ( GetPage(), controlPtValue.W );
	if ( editor.ShowModal() == wxID_OK && editor.GetValue() != controlPtValue.W )
	{
		controlPtValue.W = editor.GetValue();

		GraphPreChange( true );
		m_graphEnv->graph->SetValue( ctrlPt.m_index, controlPtValue );
		GraphPostChange( true );
		GetPage()->Refresh( false );
	}
}


void CPropertyItemSimpleCurve::OnSetTangentType( wxCommandEvent& event )
{
	if ( m_selectedGraph == -1 )
		return;

	CCurveEditDrawer* drawer = m_graphEnv->graphDrawers[m_selectedGraph];
	if ( !drawer->HasSelection() )
		return;

	ECurveSegmentType newType;
	switch ( event.GetId() )
	{
	case wxID_SET_TANGENT_CONSTANT:
		newType =  CST_Constant;
		break;
	case wxID_SET_TANGENT_INTERP:
		newType =  CST_Interpolate;
		break;
	case wxID_SET_TANGENT_BEZ:
		newType = CST_Bezier;
		break;
	case wxID_SET_TANGENT_BSMOOTH:
		newType = CST_BezierSmooth;
		break;
	case wxID_SET_TANGENT_BSMOOTH_L:
		newType = CST_BezierSmoothLyzwiaz;
		break;
	case wxID_SET_TANGENT_BSMOOTH_S:
		newType = CST_BezierSmoothSymertric;
		break;
	default:
		return;
	}

	GraphPreChange( false );

	const SCurveEditControlPoint& ctrlPt = drawer->GetCurrentSelection();

	// If selected a tangent, set only that one tangent type.
	if ( ctrlPt.m_type == CPT_Tangent )
	{
		Uint32 pointIndex = ctrlPt.m_index / 2;
		Uint32 tangentIndex = ctrlPt.m_index % 2;
		if ( pointIndex < (Int32)ctrlPt.m_curve->GetNumPoints() )
		{
			ctrlPt.m_curve->SetTangentType( pointIndex, tangentIndex, newType );
		}
	}
	// If selected the control point itself, set both tangents.
	else
	{
		if ( ctrlPt.m_index < (Int32)m_graphEnv->graph->GetNumPoints() )
		{
			ctrlPt.m_curve->SetTangentType( ctrlPt.m_index, 0, newType );
			ctrlPt.m_curve->SetTangentType( ctrlPt.m_index, 1, newType );
		}
	}

	GraphPostChange( true );
	GetPage()->Refresh( false );
}


void CPropertyItemSimpleCurve::OnSetCurveType( wxCommandEvent& event )
{
	ECurveBaseType newType;
	switch ( event.GetId() )
	{
	case wxID_SET_LINEAR:
		newType =  CT_Linear;
		break;
	case wxID_SET_SMOOTH:
		newType =  CT_Smooth;
		break;
	case wxID_SET_SEGMENTED:
		newType = CT_Segmented;
		break;
	default:
		return;
	}

	GraphPreChange( false );
	m_graphEnv->graph->SetCurveType( newType );
	GraphPostChange( true );
	GetPage()->Refresh( false );
}

void CPropertyItemSimpleCurve::SetEditionMode( bool editionMode )
{
	if ( editionMode != m_isInEditionMode )
	{
		m_isInEditionMode = editionMode;
	}
}

void CPropertyItemSimpleCurve::OpenColorPicker( const Color &color )
{
	m_ctrlColorPicker = new CEdAdvancedColorPicker( m_page, color, true );
	m_ctrlColorPicker->Connect( wxEVT_COMMAND_SCROLLBAR_UPDATED, wxCommandEventHandler( CPropertyItemSimpleCurve::OnColorPicked ), NULL, this );
}

void CPropertyItemSimpleCurve::OnColorPicked( wxCommandEvent& event )
{
	if ( -1 == m_lastColorPickerEntryTime || NULL == m_ctrlColorPicker )
	{
		return;
	}

	// Get the graph
	SCurveBase *graph = GetCurve();
	if ( NULL == graph )
	{
		return;
	}

	// Get picked color
	Color pickedColor = m_ctrlColorPicker->GetColor();

	// Get control point index
	const Int32 controlPointIndex = graph->GetIndex( m_lastColorPickerEntryTime );

	// Test key index validity
	bool isColorGraph = (SCT_Color == graph->GetSimpleCurveType() || SCT_ColorScaled == graph->GetSimpleCurveType());
	if ( !isColorGraph || controlPointIndex < 0 || controlPointIndex >= (Int32)graph->GetNumPoints() )
	{
		m_lastColorPickerEntryTime = -1;
		return;
	}

	// Apply color
	GraphPreChange( true );

	// Copy new color, but keep old scalar.
	Vector newValue( pickedColor.R, pickedColor.G, pickedColor.B, graph->GetFloatValueAtIndex( controlPointIndex ) );
	graph->SetValue( controlPointIndex, newValue );

	GraphPostChange( true );

	// Refresh
	GetPage()->Refresh( false );
}

bool CPropertyItemSimpleCurve::OnBrowserMouseEventLocal( wxMouseEvent& event )
{
	const wxPoint lastMousePos	= m_mouseLastPos;
	const wxPoint mousePos		= GetLocalMousePos();
	m_mouseLastPos				= mousePos;


	if ( event.Dragging() || event.Moving() )
	{
		m_mouseMove += Abs( mousePos.x - lastMousePos.x) + Abs( mousePos.y - lastMousePos.y);
	}

	if ( event.LeftDClick() )
	{	
		wxRect captionRect = CalcCaptionRect();
		if ( mousePos.x >= captionRect.GetLeft()  && 
			 mousePos.x <= captionRect.GetRight() &&
			 mousePos.y >= captionRect.GetTop()   && 
			 mousePos.y <= captionRect.GetBottom() )
		{
			SetEditionMode( !IsInEditionMode() );
			GetPage()->Refresh( false );
			return true;
		}
	}

	if ( !IsInEditionMode() )
	{
		return false;
	}

	if ( event.LeftDown() )
	{
		// Reset curr selection
		m_selectedGraph = -1;
		m_lastColorPickerEntryTime = -1;
		m_isScalingValue = true;

		// Determine new selection
		for ( Uint32 graph_i=0; graph_i<m_graphEnv->graphDrawers.Size(); ++graph_i )
		{
			CCurveEditDrawer* drawer = m_graphEnv->graphDrawers[graph_i];
			if ( drawer->m_fullRect.Contains( mousePos ) )
			{
				m_selectedGraph = graph_i;
				if ( !event.m_shiftDown || !event.m_controlDown )
				{
					if ( drawer->BeginSelectControlPoint( mousePos, event.m_altDown ) )
					{
						GetPage()->Refresh( false );
						return false;
					}
				}
				else
				{
					m_scalingValuePt = mousePos;
					m_isScalingValue = true;
				}
			}
		}

		GetPage()->Refresh( false );
	}

	if ( event.Dragging() && (event.m_leftDown != event.m_rightDown) )
	{
		// Only the active graph processes dragging.
		if ( m_selectedGraph != -1 )
		{
			CCurveEditDrawer* drawer = m_graphEnv->graphDrawers[m_selectedGraph];

			// Shift + Ctrl + [Alt] + LeftMouse --> Adjust scale and stuff... 
			if ( event.m_shiftDown && event.m_controlDown && event.m_leftDown && m_isScalingValue )
			{
				Float scale = powf( 1.01f, (mousePos.y - lastMousePos.y));
				if ( 1.f != scale )
				{
					if ( event.m_altDown )
					{
						PropertiesPageDayCycleSettings dayCycleSettings = GetPage()->GetDayCycleEditSettings();
						drawer->ZoomView( m_scalingValuePt, scale, true, false, &dayCycleSettings );
						GetPage()->SetDayCycleEditSettings( dayCycleSettings );
					}
					else
					{
						GraphPreChange( false );
						drawer->ZoomView( m_scalingValuePt, scale, true, false, NULL );
						GraphPostChange( false );
					}
					GetPage()->Refresh( false );
				}
			}
			// LeftMouse --> Drag selected control point
			else if ( event.m_leftDown )
			{
				if ( drawer->HasActiveSelection() )
				{
					wxPoint moveToPos = mousePos;
					// If holding shift, constrain the X position.
					if ( event.m_shiftDown && drawer->HasVerticalDependency() )
					{
						moveToPos.x = drawer->GetCurrentSelection().m_position.x;
					}
					// If holding ctrl, constrain Y position.
					if ( event.m_controlDown )
					{
						moveToPos.y = drawer->GetCurrentSelection().m_position.y;
					}

					GraphPreChange( true );

					drawer->MoveSelection( moveToPos );

					GraphPostChange( true );
				}
				else if( drawer->AreaSelectionActive() == true )
				{
					drawer->UpdateSelectionArea( mousePos );
				}
			}
			// RightMouse --> Pan around
			else if ( event.m_rightDown )
			{
				wxPoint mouseDelta = mousePos - lastMousePos;
				if ( mouseDelta.x != 0 || mouseDelta.y != 0 )
				{
					GraphPreChange( false );

					PropertiesPageDayCycleSettings dayCycleSettings = GetPage()->GetDayCycleEditSettings();
					drawer->PanView( mouseDelta, event.m_shiftDown, !drawer->HasVerticalDependency() || event.m_controlDown, &dayCycleSettings );
					GetPage()->SetDayCycleEditSettings( dayCycleSettings );

					GraphPostChange( false );
					GetPage()->Refresh( false );

					return true;
				}
			}
		}
	}

	if ( event.LeftUp() || event.Entering() || event.Leaving() )
	{
		m_isScalingValue = false;
		for ( Uint32 i = 0; i < m_graphEnv->graphDrawers.Size(); ++i )
		{
			m_graphEnv->graphDrawers[i]->EndSelectControlPoint( mousePos );
		}
		GetPage()->Refresh( false );
	}

	const Int32 mouseWheelRotation = event.GetWheelRotation();
	if ( 0 != mouseWheelRotation )
	{
		if( event.AltDown() == true )
		{
			for ( Uint32 i = 0; i < m_graphEnv->graphDrawers.Size(); ++i )
			{
				CCurveEditDrawer* drawer = m_graphEnv->graphDrawers[i];
				if ( drawer->m_fullRect.Contains( mousePos ) )
				{
					m_selectedGraph = i;
					drawer->AddDistanceMultipier( mouseWheelRotation );
				}
			}
			GetPage()->Refresh( false );
		}
		else
		{
			for ( Uint32 graph_i=0; graph_i<m_graphEnv->graphDrawers.Size(); ++graph_i )
			{
				CCurveEditDrawer *drawer = m_graphEnv->graphDrawers[graph_i];
				const wxRect graphRect = drawer->m_graphRect;

				Float refX, refY;
				if ( drawer->TransformScalarPointInverse( mousePos, refX, refY ) )
				{
					const Float absoluteScaleMin = ( CCurveEditDrawer::TIME_MAXIMUM / (1024 * 64) ) / ( drawer->m_displayAreaTimeMax - drawer->m_displayAreaTimeMin );
					const Float absoluteScaleMax = ( CCurveEditDrawer::TIME_MAXIMUM * (1024) ) / ( drawer->m_displayAreaTimeMax - drawer->m_displayAreaTimeMin );
					const Float scale = Max( absoluteScaleMin, Min( absoluteScaleMax, powf(2.f, -mouseWheelRotation / 750.f) ) );

					PropertiesPageDayCycleSettings dayCycleSettings = GetPage()->GetDayCycleEditSettings();
					drawer->ZoomView( mousePos, scale, false, false, &dayCycleSettings );
					GetPage()->SetDayCycleEditSettings( dayCycleSettings );

					GetPage()->Refresh( false );
					return true;
				}
			}
		}
	}

	if ( event.LeftDClick() )
	{
		for ( Uint32 graph_i=0; graph_i<m_graphEnv->graphDrawers.Size(); ++graph_i )
		{
			CCurveEditDrawer *drawer = m_graphEnv->graphDrawers[graph_i];

			// If we've double-clicked on this drawer's area.
			if ( drawer->m_fullRect.Contains( mousePos ) )
			{
				// If we clicked in the "color pick" area (only on color drawers), edit the selected color
				if ( drawer->IsInsideColorPickArea( mousePos ) )
				{
					if ( drawer->GetCurrentSelection().m_type == CPT_Value )
					{
						const SCurveEditControlPoint& ctrlPt = drawer->GetCurrentSelection();

						m_lastColorPickerEntryTime = ctrlPt.m_curve->GetCurveData().GetTimeAtIndex(ctrlPt.m_index);

						Vector color( ctrlPt.m_curve->GetValueAtIndex( ctrlPt.m_index ) );
						OpenColorPicker( Color( color.X, color.Y, color.Z ) );
					}
					return true;
				}
				else
				{
					// Remove key if we double clicked on existing key point on graph
					if ( drawer->GetCurrentSelection().m_type == CPT_Value )
					{
						GraphPreChange( true );

						drawer->DeleteCurrentSelectedPoint();

						GraphPostChange( true );
						return true;
					}
					else
					{
						// Add new key if we double clicked on graph area
						Float newTime;
						Vector newValue;
						if ( drawer->GenerateNewControlPoint( mousePos, newTime, newValue ) )
						{
							GraphPreChange( true );
							if ( -1 != m_graphEnv->graph->AddPoint( newTime, newValue, false ) )
							{
								GraphPostChange( true );

								// Send begin/end select requests, so that this new point will be selected.
								drawer->BeginSelectControlPoint( mousePos, event.m_altDown );
								drawer->EndSelectControlPoint( mousePos );
								return true;
							}
						}
					}
				}
			}
		}
		return true;
	}

	if ( event.RightDown() )
	{
		for ( Uint32 i = 0; i < m_graphEnv->graphDrawers.Size(); ++i )
		{
			CCurveEditDrawer* drawer = m_graphEnv->graphDrawers[i];
			if ( drawer->m_fullRect.Contains( mousePos ) )
			{
				m_selectedGraph = i;
				if ( drawer->BeginSelectControlPoint( mousePos, event.m_altDown ) )
				{
					break;
				}
			}
		}
		m_mouseMove = 0;
	}

	if ( event.RightUp() && m_mouseMove < 5 )
	{
		wxRect captionRect = CalcCaptionRect();
		if ( captionRect.Contains( mousePos ) )
		{
			// Setup menu
			wxMenu menu;
			menu.Append( wxID_COPY_CURVE, wxT("Copy") );
			menu.Connect( wxID_COPY_CURVE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CPropertyItemSimpleCurve::OnCopyCurve ), NULL, this );
			menu.Append( wxID_PASTE_CURVE, wxT("Paste") );
			menu.Connect( wxID_PASTE_CURVE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CPropertyItemSimpleCurve::OnPasteCurve ), NULL, this );
			menu.AppendSeparator();


			menu.Append( wxID_SET_LINEAR, wxT("Linear"), wxEmptyString, wxITEM_CHECK )->Check( m_graphEnv->graph->GetCurveType() == CT_Linear );
			menu.Connect( wxID_SET_LINEAR, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CPropertyItemSimpleCurve::OnSetCurveType ), NULL, this );
			menu.Append( wxID_SET_SMOOTH, wxT("Smooth"), wxEmptyString, wxITEM_CHECK )->Check( m_graphEnv->graph->GetCurveType() == CT_Smooth );
			menu.Connect( wxID_SET_SMOOTH, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CPropertyItemSimpleCurve::OnSetCurveType ), NULL, this );
			// If this graph has Float values, we can also switch to segmented
			if ( m_graphEnv->graph->GetValueType() == CVT_Float )
			{
				menu.Append( wxID_SET_SEGMENTED, wxT("Segmented"), wxEmptyString, wxITEM_CHECK )->Check( m_graphEnv->graph->GetCurveType() == CT_Segmented );
				menu.Connect( wxID_SET_SEGMENTED, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CPropertyItemSimpleCurve::OnSetCurveType ), NULL, this );
			}

			// Show menu
			GetPage()->PopupMenu( &menu, event.GetPosition() );
		}
		else
		{
			for ( Uint32 i = 0; i < m_graphEnv->graphDrawers.Size(); ++i )
			{
				CCurveEditDrawer* drawer = m_graphEnv->graphDrawers[i];
				if ( drawer->m_graphRect.Contains( mousePos ) )
				{
					m_selectedGraph = i;

					// Build menu
					wxMenu menu;
					menu.Append( wxID_COPY_POINT, wxT("Copy points") );
					menu.Connect( wxID_COPY_POINT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CPropertyItemSimpleCurve::OnCopyPoints ), NULL, this );
					menu.Append( wxID_PASTE_POINT, wxT("Paste points") );
					menu.Connect( wxID_PASTE_POINT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CPropertyItemSimpleCurve::OnPastePoints ), NULL, this );

					if ( drawer->HasActiveSelection() )
					{
						drawer->EndSelectControlPoint( mousePos );

						const SCurveEditControlPoint& ctrlPt = drawer->GetCurrentSelection();
						const Uint32 selectedPointCount = drawer->GetSelectedPoints().Size();
						Bool isTangent = ctrlPt.m_type == CPT_Tangent;

						if ( !isTangent )
						{
							menu.Append( wxID_PASTE_POINT, wxT("Remove points") );
							menu.Connect( wxID_PASTE_POINT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CPropertyItemSimpleCurve::OnRemovePoints ), NULL, this );
							menu.AppendSeparator();

							if( selectedPointCount == 1 )
							{
								if ( m_graphEnv->graph->GetSimpleCurveType() != SCT_Color )
								{
									menu.Append( wxID_SET_VALUE, wxT("Set value") );
									menu.Connect( wxID_SET_VALUE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CPropertyItemSimpleCurve::OnSetPointValueExplicit ), NULL, this );
								}

								menu.Append( wxID_SET_TIME, wxT("Set time") );
								menu.Connect( wxID_SET_TIME, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CPropertyItemSimpleCurve::OnSetPointTimeExplicit ), NULL, this );
							}
							else
							{
								menu.Append( wxID_INVERT, wxT("Invert values") );
								menu.Connect( wxID_INVERT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CPropertyItemSimpleCurve::OnInvertSelectedPoints ), NULL, this );
							}
						}

						if ( m_graphEnv->graph->UsesTangents() )
						{
							ECurveSegmentType currType = (ECurveSegmentType)-1;
							if ( isTangent )
							{
								currType = m_graphEnv->graph->GetTangentType( ctrlPt.m_index / 2, ctrlPt.m_index % 2 );
							}

							menu.AppendSeparator();
							menu.Append( wxID_ANY, !isTangent ? TXT("In/Out") : ( ( ctrlPt.m_index % 2 == 0 ) ? TXT("In") : TXT("Out") ), wxEmptyString );

							menu.AppendSeparator();
							menu.Append( wxID_SET_TANGENT_CONSTANT, wxT("Constant"), wxEmptyString, wxITEM_CHECK )->Check( isTangent && currType == CST_Constant );
							menu.Connect( wxID_SET_TANGENT_CONSTANT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CPropertyItemSimpleCurve::OnSetTangentType ), NULL, this );
							menu.Append( wxID_SET_TANGENT_INTERP, wxT("Interpolate"), wxEmptyString, wxITEM_CHECK )->Check( isTangent && currType == CST_Interpolate );
							menu.Connect( wxID_SET_TANGENT_INTERP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CPropertyItemSimpleCurve::OnSetTangentType ), NULL, this );
							menu.Append( wxID_SET_TANGENT_BEZ, wxT("Bezier"), wxEmptyString, wxITEM_CHECK )->Check( isTangent && currType == CST_Bezier );
							menu.Connect( wxID_SET_TANGENT_BEZ, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CPropertyItemSimpleCurve::OnSetTangentType ), NULL, this );
							menu.Append( wxID_SET_TANGENT_BSMOOTH, wxT("Bezier Smooth"), wxEmptyString, wxITEM_CHECK )->Check( isTangent && currType == CST_BezierSmooth );
							menu.Connect( wxID_SET_TANGENT_BSMOOTH, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CPropertyItemSimpleCurve::OnSetTangentType ), NULL, this );
							menu.Append( wxID_SET_TANGENT_BSMOOTH_S, wxT("Bezier Smooth Symertric"), wxEmptyString, wxITEM_CHECK )->Check( isTangent && currType == CST_BezierSmoothSymertric );
							menu.Connect( wxID_SET_TANGENT_BSMOOTH_S, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CPropertyItemSimpleCurve::OnSetTangentType ), NULL, this );
							menu.Append( wxID_SET_TANGENT_BSMOOTH_L, wxT("Bezier Smooth Lyzwiaz"), wxEmptyString, wxITEM_CHECK )->Check( isTangent && currType == CST_BezierSmoothLyzwiaz );
							menu.Connect( wxID_SET_TANGENT_BSMOOTH_L, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CPropertyItemSimpleCurve::OnSetTangentType ), NULL, this );
						}
					}

					// Show menu
					 GetPage()->PopupMenu( &menu, event.GetPosition() );
					break;
				}
			}
		}

		return true;
	}


	return false;
}

Bool CPropertyItemSimpleCurve::ShouldSuppressMouseScrollEvent( const wxMouseEvent& event ) const
{
	return true;
}

void CPropertyItemSimpleCurve::OnBrowserMouseEvent( wxMouseEvent& event )
{
	if ( !OnBrowserMouseEventLocal( event ) )
	{
		TBaseClass::OnBrowserMouseEvent( event );
	}

	// Make sure to update the drawers' mouse-over status. We do this out here instead of inside OnBrowserMouseEventLocal(), to make sure
	// it's done after any other changes (in case of index changes), and because the other function has many exit points.
	for ( Uint32 graph_i=0; graph_i<m_graphEnv->graphDrawers.Size(); ++graph_i )
	{
		CCurveEditDrawer* drawer = m_graphEnv->graphDrawers[graph_i];
		drawer->MouseOverControlPoint( m_mouseLastPos );
	}
}

wxRect CPropertyItemSimpleCurve::CalcValueRect() const
{
	// Calculate value drawing rect
	wxRect valueRect = m_rect;
	valueRect.x = m_page->GetSplitterPos() + 1;
	valueRect.width = ( m_rect.x + m_rect.width ) - m_buttonsWidth - (valueRect.x+1);
	return valueRect;
}

wxRect CPropertyItemSimpleCurve::CalcCaptionRect() const
{
	wxRect captionRect = m_rect;
	captionRect.width = m_page->GetSplitterPos() - 1 - captionRect.x;
	return captionRect;
}


void CPropertyItemSimpleCurve::GraphPreChange( bool valueDataChange )
{
	// empty
}

void CPropertyItemSimpleCurve::GraphPostChange( bool valueDataChange )
{
	if ( valueDataChange )
	{	
		for ( CBasePropItem *prop = this; NULL != prop; prop = prop->GetParent() )
		{
			if ( CObject *object = prop->GetParentObject(0).AsObject() )
			{
				object->MarkModified();
				break;
			}
		}

		if ( m_curve )
		{
			SCurveData& data = m_curve->GetCurveData();
			for ( auto it=data.m_curveValues.Begin(); it != data.m_curveValues.End(); ++it )
			{
				it->value = Clamp( it->value, GetRangeMin(), GetRangeMax() );
			}

			SEvents::GetInstance().DispatchEvent( CNAME( SimpleCurveChanged ), CreateEventData( SSimpleCurveChangeData( m_curve, m_property->GetName() ) ) );
		}		
	}
}

void CPropertyItemSimpleCurve::UpdateLayout( Int32& yOffset, Int32 x, Int32 width )
{
	TBaseClass::UpdateLayout( yOffset, x, width );

	m_graphEnv->Init( *this );
}

void CPropertyItemSimpleCurve::OnInvertSelectedPoints( wxCommandEvent& event )
{
	if( m_selectedGraph == -1 )
	{
		return;
	}

	CCurveEditDrawer* drawer = m_graphEnv->graphDrawers[m_selectedGraph];
	if( drawer == nullptr || drawer->HasSelection() == false )
	{
		return;
	}

	const TDynArray< SCurveEditControlPoint >& selectedPoints = drawer->GetSelectedPoints();
	const Uint32 pointCount = selectedPoints.Size();
	if( pointCount < 2 )
	{
		return;
	}

	TDynArray< Vector > temporaryValues;
	for( Int32 i=pointCount-1; i>=0; --i )
	{
		temporaryValues.PushBack( m_graphEnv->graph->GetValueAtIndex( selectedPoints[i].m_index ) );
	}

	GraphPreChange( true );
	for( Uint32 i=0; i<pointCount; ++i )
	{
		m_graphEnv->graph->SetValue( selectedPoints[i].m_index, temporaryValues[i] );
	}
	GraphPostChange( true );
	GetPage()->Refresh( false );
}

