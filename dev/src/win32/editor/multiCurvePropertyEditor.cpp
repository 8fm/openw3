///////////////////////////////////////////////////////
// headers
#include "build.h"
#include "multiCurvePropertyEditor.h"
#include "curveEditorPanelHelper.h"
#include "../../common/engine/curveControlPointEntity.h"
#include "../../common/engine/curve.h"

CMultiCurvePropertyEditor::CMultiCurvePropertyEditor( CPropertyItem* propertyItem, EditMode editMode )
	: ICustomPropertyEditor( propertyItem )
	, m_ctrlText()
	, m_2DEditorDialog( NULL )
	, m_editMode( editMode )
{	
	ASSERT( m_propertyItem->GetPropertyType()->GetName() == TXT("SMultiCurve") );

	m_icon = SEdResources::GetInstance().LoadBitmap( TXT("IMG_PB_CURVE") );
	m_icon3D = SEdResources::GetInstance().LoadBitmap( TXT("IMG_PB_CURVE_3D") );
}

Bool CMultiCurvePropertyEditor::GrabValue( String& displayValue )
{
	SMultiCurve* curve = GetCurve();
	switch ( curve->GetCurveType() )
	{
	case ECurveType_Uninitialized:
		displayValue = TXT("Curve (Uninitialized)");
		return true;
	case ECurveType_Float:
		displayValue = TXT("Curve (Float)");
		return true;
	case ECurveType_EulerAngles:
		displayValue = TXT("Curve (EulerAngles)");
		return true;
	case ECurveType_EngineTransform:
		displayValue = TXT("Curve (EngineTransform)");
		return true;
	case ECurveType_Vector:
		displayValue = TXT("Curve (Vector3)");
		return true;
	}

	return false;
}

void CMultiCurvePropertyEditor::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	SMultiCurve* curve = GetCurve();

	// Calculate placement
	wxRect valueRect;
	valueRect.y = propertyRect.y + 3;
	valueRect.height = propertyRect.height - 3;
	valueRect.x = propertyRect.x + 2;
	valueRect.width = propertyRect.width - ( propertyRect.height + 1 ) * 2;

	// Create text editor
	m_ctrlText = new wxTextCtrlEx( m_propertyItem->GetPage(), wxID_ANY,	wxEmptyString, valueRect.GetTopLeft(), valueRect.GetSize(), wxNO_BORDER | wxTE_PROCESS_ENTER );
	m_ctrlText->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_ctrlText->SetFont( m_propertyItem->GetPage()->GetStyle().m_drawFont );
	m_ctrlText->SetSelection( -1, -1 );
	m_ctrlText->SetFocus();

	// Add curve editor buttons
	if ( curve->GetCurveType() != ECurveType_Uninitialized )
	{
		if ( curve->IsEditableIn3D() && m_editMode != EditMode_2D )
		{
			m_propertyItem->AddButton( m_icon3D, wxCommandEventHandler( CMultiCurvePropertyEditor::On3DEditing ), this );
		}
		
		if ( curve->IsEditableIn2D() && m_editMode != EditMode_3D )
		{
			m_propertyItem->AddButton( m_icon, wxCommandEventHandler( CMultiCurvePropertyEditor::On2DEditing ), this );
		}
	}
}

void CMultiCurvePropertyEditor::On2DEditing( wxCommandEvent &event )
{
	String title = m_propertyItem->GetName();

	// Set up proper title if this is property animation

	CPropertyAnimationSet* animationSet = NULL;
	CName propertyName, animationName;
	if ( GetParentAnimationSet( animationSet, propertyName, animationName ) )
	{
		SPropertyAnimation* animation = animationSet->GetPropertyAnimation( propertyName, animationName );
		title = animationSet->GetParent()->GetFriendlyName() + String( TXT(": ") ) + ( animation->m_propertyName.AsString() + TXT(" : ") + animation->m_animationName.AsString() );
	}

	// Shut down 3D editing if enabled to avoid getting out of sync FUTURE TODO: Make both editors work simultaneously

	const Bool was3DEditorEnabled = Delete3DEditor();

	// Show editor when property is assigned to curve

	if ( m_2DEditorDialog )
	{
		delete m_2DEditorDialog;
	}
	m_2DEditorDialog = new CMultiCurve2DEditorDialog( this, m_propertyItem->GetPage(), title, GetCurve(), was3DEditorEnabled );
	m_2DEditorDialog->Show();
}

class CPropertyAnimationCurveAccessor : public ICurveAccessor
{
public:
	CPropertyAnimationCurveAccessor( CPropertyAnimationSet* animationSet, CName propertyName, CName animationName ) :
		m_animationSet( animationSet ),
		m_propertyName( propertyName ),
		m_animationName( animationName )
	{}

	virtual SMultiCurve* Get() override
	{
		SPropertyAnimation* animation = m_animationSet->GetPropertyAnimation( m_propertyName, m_animationName );
		return animation ? &animation->m_curve : NULL;
	}

private:
	CPropertyAnimationSet* m_animationSet;
	CName m_propertyName;
	CName m_animationName;
};

Bool CMultiCurvePropertyEditor::GetParentAnimationSet( CPropertyAnimationSet*& animationSet, CName& propertyName, CName& animationName )
{
	// Figure out the curve owner

	CNode* curveParent = GetCurveParent();
	if ( !curveParent )
	{
		return false;
	}

	CGameplayEntity* curveOwner = Cast< CGameplayEntity >( curveParent );
	if ( !curveOwner )
	{
		if ( CComponent* curveParentAsComponent = Cast< CComponent >( curveParent ) )
		{
			curveOwner = Cast< CGameplayEntity >( curveParentAsComponent->GetEntity() );
		}

		if ( !curveOwner )
		{
			return false;
		}
	}

	// Get animation set
	animationSet = curveOwner->GetPropertyAnimationSet();
	if ( !animationSet )
	{
		return false;
	}
		
	// Get property and animation name

	SMultiCurve* curve = GetCurve();

	for ( Uint32 i = 0; i < animationSet->GetPropertyAnimationsCount(); i++ )
	{
		SPropertyAnimation* animation = animationSet->GetPropertyAnimationByIndex( i );
		if ( curve == &animation->m_curve && animation->HasAssignedProperty() && curve->IsEditableIn3D() )
		{
			animationName = animation->m_animationName;
			propertyName = animation->m_propertyName;
			return true;
		}
	}

	return false;
}

Bool CMultiCurvePropertyEditor::Delete3DEditor()
{
	if( GetCurveParent() && GetCurveParent()->GetLayer() )
	{
		return CCurveEntity::DeleteEditor( GetCurveParent()->GetLayer()->GetWorld(), GetCurve() );
	}

	return false;
}

SMultiCurve* CMultiCurvePropertyEditor::GetCurve()
{
	return m_propertyItem->GetParentObject( 0 ).As< SMultiCurve >();
}

CNode* CMultiCurvePropertyEditor::GetCurveParent()
{
	if ( CNode* parent = GetCurve()->GetParent() )
	{
		return parent;
	}

	// Find nearest CNode parent (we need something we can get world transform from)

	CBasePropItem* parent = m_propertyItem->GetParent();
	while ( parent )
	{
		if ( parent->IsClassGroupItem() )
		{
			CClassGroupItem* classGroupItem = static_cast<CClassGroupItem*>( parent );
			if ( CNode* node = classGroupItem->GetParentObject( 0 ).As< CNode >() ) 
			{
				return node;
			}
		}
		parent = parent->GetParent();
	}

	return nullptr;
}

void CMultiCurvePropertyEditor::On3DEditing( wxCommandEvent& event )
{
	// Close down 2D curve editor if active

	if ( m_2DEditorDialog )
	{
		delete m_2DEditorDialog;
		m_2DEditorDialog = NULL;
	}
	
	// Hide the curve editor if visible

	if ( Delete3DEditor() )
	{
		return;
	}

	if( GetCurveParent() == nullptr )
	{
		wxMessageBox( TXT("Can not edit trajectory curve from outside Entity Editor!!!\nPlease edit curve in Entity Editor and copy it with 'Copy via RTTI' option."), wxT( "Failed" ), wxOK );
		return;
	}
	// Otherwise create curve editor

	Create3DEditor();
}

void CMultiCurvePropertyEditor::Create3DEditor()
{
	// Create curve accessor

	ICurveAccessor* curveAccessor = NULL;

	CPropertyAnimationSet* animationSet = NULL;
	CName propertyName, animationName;
	if ( GetParentAnimationSet( animationSet, propertyName, animationName ) )
	{
		curveAccessor = new CPropertyAnimationCurveAccessor( animationSet, propertyName, animationName );
	}
	else
	{
		curveAccessor = new CDefaultCurveAccessor( GetCurve() );
	}

	// Show curve editor

	GetCurve()->EnableConsistentNumberOfControlPoints( true );

	if ( CCurveEntity* curveEditorEntity = CCurveEntity::CreateEditor( GetCurveParent()->GetLayer()->GetWorld(), curveAccessor, true ) )
	{
		if ( animationSet )
		{
			curveEditorEntity->SetDrawOnTop( true );
		}
	}
}

void CMultiCurvePropertyEditor::CloseControls()
{
	if ( m_ctrlText )
	{
		delete m_ctrlText;
		m_ctrlText = NULL;
	}
}

// ------------------------------------------------------

BEGIN_EVENT_TABLE( CMultiCurve2DEditorDialog, wxDialog )
	EVT_BUTTON( XRCID("buttOk"), CMultiCurve2DEditorDialog::OnOk )
END_EVENT_TABLE()

CMultiCurve2DEditorDialog::CMultiCurve2DEditorDialog( CMultiCurvePropertyEditor* curvePropertyEditor, wxWindow* parent, const String& title, SMultiCurve* curve, Bool show3DEditorOnClose )
	: m_curveEditor( NULL )
	, m_curvePropertyEditor( curvePropertyEditor )
	, m_show3DEditorOnClose( show3DEditorOnClose )
{
	// Load designed frame from resource
	wxXmlResource::Get()->LoadDialog( this, NULL, TXT("PropertyCurveDialog") );
	wxDialog::SetLabel( title.AsChar() );

	// Create Curve Editor and place it in CurvePanel
	wxPanel* rp = XRCCTRL( *this, "CurvePanel", wxPanel );
	wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );		
	m_curveEditor = new CEdCurveEditor( rp );
	sizer1->Add( m_curveEditor, 1, wxEXPAND, 0 );
	rp->SetSizer( sizer1 );		
	rp->Layout();
	Layout();

	// init
	TDynArray<SCurveData>& curves = curve->GetDataCurves();
	const Uint32 curvesCount = curves.Size();

	// names/colors
	static String namesValue[] = { TXT("Value") };
	static String namesVector[] = { TXT("X"), TXT("Y"), TXT("Z") };
	static String namesEulerAngles[] = { TXT("Rot X (Roll)"), TXT("Rot Y (Pitch)"), TXT("Rot Z (Yaw)") };
	static String namesEngineTransform[] = { TXT("Pos X"), TXT("Pos Y"), TXT("Pos Z"), TXT("Rot X"), TXT("Rot Y"), TXT("Rot Z"), TXT("Scale X"), TXT("Scale Y"), TXT("Scale Z") };
	static const Color colors[] = { Color( 255, 0, 0 ), Color( 0, 255, 0 ), Color( 0, 0, 255 ), Color( 127, 0, 0 ), Color( 0, 127, 0 ), Color( 0, 0, 127 ), Color( 255, 127, 127 ), Color( 127, 255, 127 ), Color( 127, 127, 255 ), Color( 0, 0, 0 ) };

	// select names array
	String* namesArray = NULL;
	switch ( curve->GetCurveType() )
	{
		case ECurveType_EngineTransform: namesArray = namesEngineTransform; break;
		case ECurveType_EulerAngles: namesArray = namesEulerAngles; break;
		case ECurveType_Vector: namesArray = namesVector; break;
		case ECurveType_Float: namesArray = namesValue; break;
	}

	// add curves
	for ( Uint32 i = 0; i < curvesCount; ++i )
	{
		SCurveData& curveData = curves[i];
		m_curveEditor->AddCurve( &curveData, namesArray[i], true, Vector( 0, 1.0f, 0 ), NULL, colors[i] );
	}

	// compute active region min/max
	Float min, max;
	for ( Uint32 i = 0; i < curvesCount; ++i )
	{
		SCurveData& curveData = curves[i];
		const Uint32 timesCount = curveData.Size();
		if ( timesCount )
		{
			min = Min( min, curveData.GetMinTime() );
			max = Max( max, curveData.GetMaxTime() );
		}
	}

	// setup view
	SetActiveRegion( min, max );
	Show();
}

void CMultiCurve2DEditorDialog::SetActiveRegion( Float start, Float end )
{
	m_curveEditor->SetActiveRegion( start, end );
}

void CMultiCurve2DEditorDialog::OnOk( wxCommandEvent& event )
{
	EndDialog(1);

	if ( m_show3DEditorOnClose )
	{
		m_curvePropertyEditor->Create3DEditor();
	}
}

// -----------------------

CCurveKeyDialog* CCurveKeyDialog::s_instance = NULL;

BEGIN_EVENT_TABLE( CCurveKeyDialog, wxDialog )
	EVT_BUTTON( XRCID("closeButton"), CCurveKeyDialog::OnCloseButtonClick )
	EVT_TEXT_ENTER( XRCID("timeText"), CCurveKeyDialog::OnTimeTextEnter )
	EVT_TEXT_ENTER( XRCID("easeInText"), CCurveKeyDialog::OnEaseInTextEnter )
	EVT_TEXT_ENTER( XRCID("xText"), CCurveKeyDialog::OnTransformTextEnter )
	EVT_TEXT_ENTER( XRCID("yText"), CCurveKeyDialog::OnTransformTextEnter )
	EVT_TEXT_ENTER( XRCID("zText"), CCurveKeyDialog::OnTransformTextEnter )
	EVT_TEXT_ENTER( XRCID("rollText"), CCurveKeyDialog::OnTransformTextEnter )
	EVT_TEXT_ENTER( XRCID("pitchText"), CCurveKeyDialog::OnTransformTextEnter )
	EVT_TEXT_ENTER( XRCID("yawText"), CCurveKeyDialog::OnTransformTextEnter )
	EVT_TEXT_ENTER( XRCID("scaleXText"), CCurveKeyDialog::OnTransformTextEnter )
	EVT_TEXT_ENTER( XRCID("scaleYText"), CCurveKeyDialog::OnTransformTextEnter )
	EVT_TEXT_ENTER( XRCID("scaleZText"), CCurveKeyDialog::OnTransformTextEnter )
	EVT_KEY_DOWN( CCurveKeyDialog::OnKeyDown )
END_EVENT_TABLE()

CCurveKeyDialog::CCurveKeyDialog()
	: m_panel( NULL )
	, m_currentControlPoint( NULL )
{
	wxXmlResource::Get()->LoadDialog( this, NULL, TEXT("CurveKeyParams") );

	m_timeText = XRCCTRL( *this, "timeText", wxTextCtrl );
	m_timeText->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );

	m_easeInText = XRCCTRL( *this, "easeInText", wxTextCtrl );
	m_easeInText->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );

	m_easeOutText = XRCCTRL( *this, "easeOutText", wxTextCtrl );
	m_easeOutText->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );

	m_posText[0] = XRCCTRL( *this, "xText", wxTextCtrl );
	m_posText[0]->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );
	m_posText[1] = XRCCTRL( *this, "yText", wxTextCtrl );
	m_posText[1]->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );
	m_posText[2] = XRCCTRL( *this, "zText", wxTextCtrl );
	m_posText[2]->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );

	m_rotText[0] = XRCCTRL( *this, "rollText", wxTextCtrl );
	m_rotText[0]->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );
	m_rotText[1] = XRCCTRL( *this, "pitchText", wxTextCtrl );
	m_rotText[1]->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );
	m_rotText[2] = XRCCTRL( *this, "yawText", wxTextCtrl );
	m_rotText[2]->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );

	m_scaleText[0] = XRCCTRL( *this, "scaleXText", wxTextCtrl );
	m_scaleText[0]->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );
	m_scaleText[1] = XRCCTRL( *this, "scaleYText", wxTextCtrl );
	m_scaleText[1]->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );
	m_scaleText[2] = XRCCTRL( *this, "scaleZText", wxTextCtrl );
	m_scaleText[2]->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );
}

void CCurveKeyDialog::OnCloseButtonClick( wxCommandEvent& event )
{
	Close();
}

void CCurveKeyDialog::OnTimeTextEnter( wxCommandEvent& event )
{
	double value;
	if ( m_timeText->GetValue().ToDouble( &value ) )
	{
		SetTime( value );
	}
}

void CCurveKeyDialog::OnTransformTextEnter( wxCommandEvent& event )
{
	wxTextCtrl* obj = dynamic_cast< wxTextCtrl* >( event.GetEventObject() );
	ASSERT( obj );

	double value;
	if ( !obj->GetValue().ToDouble( &value ) )
	{
		return;
	}

	const int controlPointIndex = m_currentControlPoint->GetCurveEntity()->GetControlPointIndex( m_currentControlPoint );
	SMultiCurve* curve = m_currentControlPoint->GetCurveEntity()->GetCurve();
	EngineTransform transform;
	curve->GetControlPointTransform( controlPointIndex, transform );

	if ( obj->GetName() == "xText" ) { Vector newPos = transform.GetPosition(); newPos.X = value; transform.SetPosition( newPos ); }
	else if ( obj->GetName() == "yText" ) { Vector newPos = transform.GetPosition(); newPos.Y = value; transform.SetPosition( newPos ); }
	else if ( obj->GetName() == "zText" ) { Vector newPos = transform.GetPosition(); newPos.Z = value; transform.SetPosition( newPos ); }

	else if ( obj->GetName() == "rollText" ) { EulerAngles newRot = transform.GetRotation(); newRot.Roll = value; transform.SetRotation( newRot ); }
	else if ( obj->GetName() == "pitchText" ) { EulerAngles newRot = transform.GetRotation(); newRot.Pitch = value; transform.SetRotation( newRot ); }
	else if ( obj->GetName() == "yawText" ) { EulerAngles newRot = transform.GetRotation(); newRot.Yaw = value; transform.SetRotation( newRot ); }

	else if ( obj->GetName() == "scaleXText" ) { Vector newScale = transform.GetScale(); newScale.X = value; transform.SetScale( newScale ); }
	else if ( obj->GetName() == "scaleYText" ) { Vector newScale = transform.GetScale(); newScale.Y = value; transform.SetScale( newScale ); }
	else if ( obj->GetName() == "scaleZText" ) { Vector newScale = transform.GetScale(); newScale.Z = value; transform.SetScale( newScale ); }

	curve->SetControlPointTransform( controlPointIndex, transform );
	m_currentControlPoint->GetCurveEntity()->UpdateControlPointTransforms();
	m_currentControlPoint->GetCurveEntity()->OnCurveChanged();
}

void CCurveKeyDialog::OnEaseInTextEnter( wxCommandEvent& event )
{
	if ( SCurveEaseParam* params = GetEaseParams() )
	{
		double value;
		if ( m_easeInText->GetValue().ToDouble( &value ) )
		{
			params->m_easeIn = (Float) value;
			m_currentControlPoint->GetCurveEntity()->OnCurveChanged();
		}
	}
}

void CCurveKeyDialog::OnEaseOutTextEnter( wxCommandEvent& event )
{
	if ( SCurveEaseParam* params = GetEaseParams() )
	{
		double value;
		if ( m_easeOutText->GetValue().ToDouble( &value ) )
		{
			params->m_easeOut = (Float) value;
			m_currentControlPoint->GetCurveEntity()->OnCurveChanged();
		}
	}
}

void CCurveKeyDialog::ShowInstance( Bool show, CEdRenderingPanel* panel )
{
	if ( !s_instance && show )
	{
		s_instance = new CCurveKeyDialog();
		s_instance->Show();
		CCurveEntity::RegisterSelectionListener( s_instance );
		SMultiCurve::RegisterChangeListener( s_instance );
	}
	else if ( s_instance && !show )
	{
		SMultiCurve::UnregisterChangeListener( s_instance );
		CCurveEntity::UnregisterSelectionListener( s_instance );
		s_instance->Close();
		delete s_instance;
		s_instance = nullptr;
	}

	if ( s_instance )
	{
		s_instance->m_panel = panel;
		s_instance->RefreshValues();
	}
}

void CCurveKeyDialog::RefreshValues( Bool force )
{
	if ( s_instance )
	{
		s_instance->RefreshValues_Internal( force );
	}
}

CCurveControlPointEntity* CCurveKeyDialog::GetCurrentControlPoint()
{
	CWorld* world = m_panel->GetWorld();
	if ( !world )
	{
		return NULL;
	}

	SCurveSelectionInfo selection;
	if ( !CCurveEditorPanelHelper::GetSelectionInfo( m_panel, selection ) || !selection.m_singleControlPoint )
	{
		return NULL;
	}

	return selection.m_singleControlPoint;
}

SCurveEaseParam* CCurveKeyDialog::GetEaseParams()
{
	UpdateCurrentControlPoint();
	if ( !m_currentControlPoint )
	{
		return NULL;
	}

	const int controlPointIndex = m_currentControlPoint->GetCurveEntity()->GetControlPointIndex( m_currentControlPoint );
	SMultiCurve* curve = m_currentControlPoint->GetCurveEntity()->GetCurve();
	if ( !curve->HasEaseParams() )
	{
		return NULL;
	}

	return &curve->GetEaseParams( controlPointIndex );
}

void CCurveKeyDialog::GetTransform( EngineTransform& transform )
{
	if ( !m_currentControlPoint )
	{
		return;
	}

	const int controlPointIndex = m_currentControlPoint->GetCurveEntity()->GetControlPointIndex( m_currentControlPoint );
	SMultiCurve* curve = m_currentControlPoint->GetCurveEntity()->GetCurve();

	curve->GetControlPointTransform( controlPointIndex, transform );
}

Float CCurveKeyDialog::GetTime()
{
	const int controlPointIndex = m_currentControlPoint->GetCurveEntity()->GetControlPointIndex( m_currentControlPoint );
	SMultiCurve* curve = m_currentControlPoint->GetCurveEntity()->GetCurve();
	return curve->GetControlPointTime( controlPointIndex );
}

void CCurveKeyDialog::SetTime( Float time )
{
	UpdateCurrentControlPoint();
	if ( !m_currentControlPoint )
	{
		return;
	}

	const int controlPointIndex = m_currentControlPoint->GetCurveEntity()->GetControlPointIndex( m_currentControlPoint );
	m_currentControlPoint->GetCurveEntity()->SetControlPointTime( controlPointIndex, time );
	m_currentControlPoint->GetCurveEntity()->OnCurveChanged();

	if ( m_currentControlPoint )
	{
		// Get the actual value that was set
		m_timeText->SetValue( wxString::Format( "%f", GetTime() ) );
	}
}

Bool CCurveKeyDialog::UpdateCurrentControlPoint()
{
	CCurveControlPointEntity* currentControlPoint = GetCurrentControlPoint();
	if ( m_currentControlPoint != currentControlPoint )
	{
		m_currentControlPoint = currentControlPoint;
		if ( m_currentControlPoint )
		{
			const int controlPointIndex = m_currentControlPoint->GetCurveEntity()->GetControlPointIndex( m_currentControlPoint );
			SetTitle( wxString::Format( "Curve key: %d", controlPointIndex ) );
		}
		else
		{
			SetTitle( "No curve key selected" );
			m_timeText->Enable( false );
			m_easeInText->Enable( false );
			m_easeOutText->Enable( false );
			for ( Uint32 i = 0; i < 3; i++ )
			{
				m_posText[i]->Enable( false );
				m_rotText[i]->Enable( false );
				m_scaleText[i]->Enable( false );
			}
		}
		return true;
	}
	m_currentControlPoint = currentControlPoint;
	return false;
}

void CCurveKeyDialog::RefreshValues_Internal( Bool force )
{
	const Bool updateValues = UpdateCurrentControlPoint() || force;

	m_timeText->Enable( false );
	m_easeInText->Enable( false );
	m_easeOutText->Enable( false );
	for ( Uint32 i = 0; i < 3; i++ )
	{
		m_posText[i]->Enable( false );
		m_rotText[i]->Enable( false );
		m_scaleText[i]->Enable( false );
	}

	if ( m_currentControlPoint )
	{
		SMultiCurve* curve = m_currentControlPoint->GetCurveEntity()->GetCurve();
		const Bool isAutomaticTimeRecalculationEnabled = curve->IsAutomaticTimeByDistanceRecalculationEnabled() || curve->IsAutomaticTimeRecalculationEnabled();

		m_timeText->Enable( !isAutomaticTimeRecalculationEnabled );
		if ( updateValues )
		{
			m_timeText->SetValue( wxString::Format( "%f", GetTime() ) );
		}

		if ( SCurveEaseParam* params = GetEaseParams() )
		{
			m_easeInText->Enable( true );
			m_easeOutText->Enable( true );
			if ( updateValues )
			{
				m_easeInText->SetValue( wxString::Format( "%f", params->m_easeIn ) );
				m_easeOutText->SetValue( wxString::Format( "%f", params->m_easeOut ) );
			}
		}

		EngineTransform transform;
		GetTransform( transform );

		if ( curve->HasPosition() )
		{
			for ( Uint32 i = 0; i < 3; i++ )
			{
				m_posText[ i ]->Enable( true );
			}
			if ( updateValues )
			{
				m_posText[ 0 ]->SetValue( wxString::Format( "%f", transform.GetPosition().X ) );
				m_posText[ 1 ]->SetValue( wxString::Format( "%f", transform.GetPosition().Y ) );
				m_posText[ 2 ]->SetValue( wxString::Format( "%f", transform.GetPosition().Z ) );
			}
		}

		if ( curve->HasRotation() )
		{
			for ( Uint32 i = 0; i < 3; i++ )
			{
				m_rotText[ i ]->Enable( true );
			}
			if ( updateValues )
			{
				m_rotText[ 0 ]->SetValue( wxString::Format( "%f", transform.GetRotation().Roll ) );
				m_rotText[ 1 ]->SetValue( wxString::Format( "%f", transform.GetRotation().Pitch ) );
				m_rotText[ 2 ]->SetValue( wxString::Format( "%f", transform.GetRotation().Yaw ) );
			}
		}

		if ( curve->HasScale() )
		{
			for ( Uint32 i = 0; i < 3; i++ )
			{
				m_scaleText[ i ]->Enable( true );
			}
			if ( updateValues )
			{
				m_scaleText[ 0 ]->SetValue( wxString::Format( "%f", transform.GetScale().X ) );
				m_scaleText[ 1 ]->SetValue( wxString::Format( "%f", transform.GetScale().Y ) );
				m_scaleText[ 2 ]->SetValue( wxString::Format( "%f", transform.GetScale().Z ) );
			}
		}
	}
}

void CCurveKeyDialog::OnKeyDown( wxKeyEvent& event )
{
	if ( !m_currentControlPoint )
	{
		return;
	}

	CCurveEntity* curveEntity = m_currentControlPoint->GetCurveEntity();

	if ( event.GetKeyCode() == WXK_SEPARATOR )
	{
		curveEntity->SelectPreviousControlPoint( m_currentControlPoint );
	}
	else if ( event.GetKeyCode() == WXK_DECIMAL )
	{
		curveEntity->SelectNextControlPoint( m_currentControlPoint );
	}
}

void CCurveKeyDialog::OnCurveChanged( SMultiCurve* curve )
{
	RefreshValues( true );
}

void CCurveKeyDialog::OnCurveSelectionChanged( CCurveEntity* curveEntity, const TDynArray< Uint32 >& selectedControlPointIndices )
{
	RefreshValues( true );
}