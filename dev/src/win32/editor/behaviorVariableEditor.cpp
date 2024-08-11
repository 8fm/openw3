/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/behaviorGraphInstance.h"
#include "../../common/core/dependencyLoader.h"
#include "../../common/core/dependencySaver.h"

#include "behaviorEditor.h"
#include "behaviorPreviewPanel.h"
#include "behaviorVariableEditor.h"
#include "gridEditor.h"

BEGIN_EVENT_TABLE( CEdBehaviorVariableEditor, CEdBehaviorEditorSimplePanel )
	EVT_LISTBOX( XRCID( "variablesList" ), CEdBehaviorVariableEditor::OnVarListBoxSelChange )
	EVT_LISTBOX( XRCID( "vectorVariablesList" ), CEdBehaviorVariableEditor::OnVectorVarListBoxSelChange )
	EVT_LISTBOX( XRCID( "eventsList" ), CEdBehaviorVariableEditor::OnEventsListBoxSelChange )
	EVT_LISTBOX_DCLICK( XRCID( "eventsList" ), CEdBehaviorVariableEditor::OnEventsListRaiseEvent )
	EVT_LISTBOX( XRCID( "internalVariablesList" ), CEdBehaviorVariableEditor::OnInternalVarListBoxSelChange )
	EVT_LISTBOX( XRCID( "internalVectorVariablesList" ), CEdBehaviorVariableEditor::OnInternalVectorVarListBoxSelChange )
	EVT_MENU( XRCID( "addVar"), CEdBehaviorVariableEditor::OnAddVariable )
	EVT_MENU( XRCID( "removeVar"), CEdBehaviorVariableEditor::OnRemoveVariable )
	EVT_MENU( XRCID( "copyVars"), CEdBehaviorVariableEditor::OnCopyVariables )
	EVT_MENU( XRCID( "pasteVars"), CEdBehaviorVariableEditor::OnPasteVariables )
	EVT_MENU( XRCID( "addVectorVar"), CEdBehaviorVariableEditor::OnAddVectorVariable )
	EVT_MENU( XRCID( "removeVectorVar"), CEdBehaviorVariableEditor::OnRemoveVectorVariable )
	EVT_MENU( XRCID( "copyVectorVars"), CEdBehaviorVariableEditor::OnCopyVectorVariables )
	EVT_MENU( XRCID( "pasteVectorVars"), CEdBehaviorVariableEditor::OnPasteVectorVariables )
	EVT_MENU( XRCID( "addInternalVar"), CEdBehaviorVariableEditor::OnAddInternalVariable )
	EVT_MENU( XRCID( "removeInternalVar"), CEdBehaviorVariableEditor::OnRemoveInternalVariable )
	EVT_MENU( XRCID( "copyInternalVars"), CEdBehaviorVariableEditor::OnCopyInternalVariables )
	EVT_MENU( XRCID( "pasteInternalVars"), CEdBehaviorVariableEditor::OnPasteInternalVariables )
	EVT_MENU( XRCID( "addInternalVectorVar"), CEdBehaviorVariableEditor::OnAddInternalVectorVariable )
	EVT_MENU( XRCID( "removeInternalVectorVar"), CEdBehaviorVariableEditor::OnRemoveInternalVectorVariable )
	EVT_MENU( XRCID( "copyInternalVectorVars"), CEdBehaviorVariableEditor::OnCopyInternalVectorVariables )
	EVT_MENU( XRCID( "pasteInternalVectorVars"), CEdBehaviorVariableEditor::OnPasteInternalVectorVariables )
	EVT_MENU( XRCID( "addEvent"), CEdBehaviorVariableEditor::OnAddEvent )
	EVT_MENU( XRCID( "removeEvent"), CEdBehaviorVariableEditor::OnRemoveEvent )
	EVT_MENU( XRCID( "copyEvents"), CEdBehaviorVariableEditor::OnCopyEvents )
	EVT_MENU( XRCID( "pasteEvents"), CEdBehaviorVariableEditor::OnPasteEvents )
	EVT_MENU( XRCID( "raiseEvent"), CEdBehaviorVariableEditor::OnRaiseEvent )
	EVT_MENU( XRCID( "raiseForceEvent"), CEdBehaviorVariableEditor::OnRaiseForceEvent )
	EVT_COMMAND_SCROLL( XRCID( "valueSlider" ), CEdBehaviorVariableEditor::OnValueSlider )
	EVT_COMMAND_SCROLL( XRCID( "valueSliderX" ), CEdBehaviorVariableEditor::OnVectorValueSlider )
	EVT_COMMAND_SCROLL( XRCID( "valueSliderY" ), CEdBehaviorVariableEditor::OnVectorValueSlider )
	EVT_COMMAND_SCROLL( XRCID( "valueSliderZ" ), CEdBehaviorVariableEditor::OnVectorValueSlider )
	EVT_COMMAND_SCROLL( XRCID( "internalValueSlider" ), CEdBehaviorVariableEditor::OnInternalValueSlider )
	EVT_COMMAND_SCROLL( XRCID( "internalValueSliderX" ), CEdBehaviorVariableEditor::OnInternalVectorValueSlider )
	EVT_COMMAND_SCROLL( XRCID( "internalValueSliderY" ), CEdBehaviorVariableEditor::OnInternalVectorValueSlider )
	EVT_COMMAND_SCROLL( XRCID( "internalValueSliderZ" ), CEdBehaviorVariableEditor::OnInternalVectorValueSlider )
	EVT_MENU( XRCID( "varInput" ), CEdBehaviorVariableEditor::OnShowInputs )
	EVT_MENU( XRCID( "showInPreview" ), CEdBehaviorVariableEditor::OnShowVectorVariableInPreview )
	EVT_MENU( XRCID( "eventInput" ), CEdBehaviorVariableEditor::OnShowInputs )
	EVT_MENU( XRCID( "internalVarInput" ), CEdBehaviorVariableEditor::OnShowInputs )
	EVT_MENU( XRCID( "internalShowInPreview" ), CEdBehaviorVariableEditor::OnShowInternalVectorVariableInPreview )
	EVT_CHOICE( XRCID( "inputChoiceVar" ), CEdBehaviorVariableEditor::OnSelectInput )
	EVT_CHOICE( XRCID( "inputChoiceEvent" ), CEdBehaviorVariableEditor::OnSelectInput )
	EVT_BUTTON( XRCID( "motionChoiceRefresh" ), CEdBehaviorVariableEditor::OnMotionChoice )
	EVT_BUTTON( XRCID( "motionSetStartHeading" ), CEdBehaviorVariableEditor::OnMotionSetHeading )
	EVT_MENU( XRCID( "motionPlay"), CEdBehaviorVariableEditor::OnMotionStart )
	EVT_MENU( XRCID( "motionStop"), CEdBehaviorVariableEditor::OnMotionStop )
	EVT_TOGGLEBUTTON( XRCID( "buttVecVar1"), CEdBehaviorVariableEditor::OnVectorSliderScale1 )
	EVT_TOGGLEBUTTON( XRCID( "buttVecVar10"), CEdBehaviorVariableEditor::OnVectorSliderScale10 )
	EVT_TOGGLEBUTTON( XRCID( "buttVecVar100"), CEdBehaviorVariableEditor::OnVectorSliderScale100 )
	EVT_TOGGLEBUTTON( XRCID( "buttInternalVecVar1"), CEdBehaviorVariableEditor::OnInternalVectorSliderScale1 )
	EVT_TOGGLEBUTTON( XRCID( "buttInternalVecVar10"), CEdBehaviorVariableEditor::OnInternalVectorSliderScale10 )
	EVT_TOGGLEBUTTON( XRCID( "buttInternalVecVar100"), CEdBehaviorVariableEditor::OnInternalVectorSliderScale100 )
END_EVENT_TABLE()

//RED_DEFINE_STATIC_NAME( BehaviorEventGenerated )

Float CEdBehaviorVariableEditor::SLIDER_VALUE_SCALE = 100.0f;
Float CEdBehaviorVariableEditor::MAX_SLIDER_VALUE_RANGE = 200.0f;

Float CEdBehaviorVariableEditor::SLIDER_VECTOR_VALUE_SCALE = 100.0f;
Float CEdBehaviorVariableEditor::MAX_SLIDER_VECTOR_VALUE_RANGE = 400.0f;

CEdBehaviorVariableEditor::CEdBehaviorVariableEditor( CEdBehaviorEditor* editor, wxWindow* parent ) 
	: CEdBehaviorEditorSimplePanel( editor, parent )
	, m_vecSliderScale( VSS_Var )
{
	// Load designed frame from resource
	wxPanel* innerPanel = wxXmlResource::Get()->LoadPanel( this, wxT("BehaviorVarEditor2") );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( innerPanel, 1, wxEXPAND|wxALL, 0 );
	SetSizer( sizer );

	//wxIcon iconSmall;
	//iconSmall.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( _T("IMG_TOOL") ) );
	//SetIcon( iconSmall );
	
	{ 
		wxPanel* rp = XRCCTRL( *this, "propertiesPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		m_variableProperties = new CEdPropertiesBrowserWithStatusbar( rp, settings, nullptr );
		m_variableProperties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdBehaviorVariableEditor::OnPropertiesChanged ), NULL, this );
		m_variableProperties->Get().Connect( wxEVT_GRID_VALUE_CHANGED, wxCommandEventHandler( CEdBehaviorVariableEditor::OnGridValueChanged ), NULL, this );

		sizer1->Add( m_variableProperties, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	{
		wxNotebook* n = XRCCTRL( *this, "notebook", wxNotebook );
		n->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxCommandEventHandler( CEdBehaviorVariableEditor::OnPageChange ), NULL, this );
	}

	Layout();
	Show();
}

wxAuiPaneInfo CEdBehaviorVariableEditor::GetPaneInfo() const
{
	wxAuiPaneInfo info = CEdBehaviorEditorPanel::GetPaneInfo();

	info.CloseButton( false ).Right().RightDockable( true ).MinSize(100,300).BestSize( 300, 600 );

	return info;
}

void CEdBehaviorVariableEditor::OnReset()
{ 
	RefreshAll(); 
}

void CEdBehaviorVariableEditor::OnDebug( Bool flag )
{
	XRCCTRL( *this, "varToolbar", wxToolBar )->Enable( !flag );
	XRCCTRL( *this, "vecVarToolbar", wxToolBar )->Enable( !flag );
	XRCCTRL( *this, "internalVarToolbar", wxToolBar )->Enable( !flag );
	XRCCTRL( *this, "internalVecVarToolbar", wxToolBar )->Enable( !flag );

	wxToolBar* eventTools = XRCCTRL( *this, "eventToolbar", wxToolBar );
	eventTools->EnableTool( XRCID( "addEvent"), !flag );
	eventTools->EnableTool( XRCID( "removeEvent"), !flag );
	eventTools->EnableTool( XRCID( "eventInput"), !flag );

	//XRCCTRL( *this, "valueSlider", wxSlider )->Enable( !flag );
	//XRCCTRL( *this, "valueSliderX", wxSlider )->Enable( !flag );
	//XRCCTRL( *this, "valueSliderY", wxSlider )->Enable( !flag );
	//XRCCTRL( *this, "valueSliderZ", wxSlider )->Enable( !flag );
	//XRCCTRL( *this, "internalValueSlider", wxSlider )->Enable( !flag );
	//XRCCTRL( *this, "internalValueSliderX", wxSlider )->Enable( !flag );
	//XRCCTRL( *this, "internalValueSliderY", wxSlider )->Enable( !flag );
	//XRCCTRL( *this, "internalValueSliderZ", wxSlider )->Enable( !flag );

	m_variableProperties->Enable( !flag );
}

void CEdBehaviorVariableEditor::RefreshAll()
{
	RefreshVariablesList();

	RefreshVectorVariablesList();

	RefreshEventsList();

	RefreshInternalVariablesList();

	RefreshInternalVectorVariablesList();

	RefreshControlList();

	RefreshMotionList();

	ShowVectorInPreview();
}

CEdBehaviorVariableEditor::~CEdBehaviorVariableEditor()
{
	m_floatInputs.Clear();
	m_eventInputs.Clear();
	m_internalFloatInputs.Clear();
}

void CEdBehaviorVariableEditor::AddToInputVarMap( const CName& input, const CName& var )
{
	if ( GetBehaviorGraphInstance()->HasFloatValue( var ) )
	{
		VERIFY( m_floatInputs.Insert( input ) );
	}
}

void CEdBehaviorVariableEditor::AddToInputEventMap( const CName& input, const String& e )
{
	CName eventName(e);
	Int32 eId = GetBehaviorGraphInstance()->GetEventId( eventName );
	if ( eId != CBehaviorEventsList::NO_EVENT )
	{
		m_eventInputs.Set( input, eventName );
	}
}

void CEdBehaviorVariableEditor::AddToInputInternalVarMap( const CName& input, const CName& var )
{
	if ( GetBehaviorGraphInstance()->HasInternalFloatValue( var ) )
	{
		VERIFY( m_internalFloatInputs.Insert( input ) );
	}
}

void CEdBehaviorVariableEditor::RefreshVariablesListWorker(Bool forInternalVariables)
{	
	wxListBox* lb = XRCCTRL( *this, forInternalVariables? "internalVariablesList" : "variablesList", wxListBox );

	Int32 selection = lb->GetSelection();

	lb->Clear();

	const CBehaviorVariablesList& graphVariables = forInternalVariables? GetBehaviorGraph()->GetInternalVariables() : GetBehaviorGraph()->GetVariables();

	auto vars = graphVariables.GetVariables();
	for ( auto it = vars.Begin(), end = vars.End(); it != end; ++it )
	{
		CBehaviorVariable* currVar = it->m_second;
		lb->Append( currVar->m_name.AsChar(), currVar );
	}

	if ( selection < (int)lb->GetCount() )
	{
		lb->SetSelection( selection );
	}

	ShowSelectedVariable();
}

void CEdBehaviorVariableEditor::RefreshVariablesList()
{
	RefreshVariablesListWorker(false);
}

void CEdBehaviorVariableEditor::RefreshInternalVariablesList()
{
	RefreshVariablesListWorker(true);
}

void CEdBehaviorVariableEditor::RefreshVectorVariablesListWorker(Bool forInternalVariables)
{	
	wxListBox* lb = XRCCTRL( *this, forInternalVariables? "internalVectorVariablesList" : "vectorVariablesList", wxListBox );

	Int32 selection = lb->GetSelection();

	lb->Clear();

	const CBehaviorVectorVariablesList& graphVariables = forInternalVariables? GetBehaviorGraph()->GetInternalVectorVariables() : GetBehaviorGraph()->GetVectorVariables();

	auto vars = graphVariables.GetVariables();
	for ( auto it = vars.Begin(), end = vars.End(); it != end; ++it )
	{
		CBehaviorVectorVariable* currVar = it->m_second;
		lb->Append( currVar->m_name.AsChar(), currVar );
	}

	if ( selection < (int)lb->GetCount() )
	{
		lb->SetSelection( selection );
	}

	ShowSelectedVectorVariable();
}

void CEdBehaviorVariableEditor::RefreshVectorVariablesList()
{
	RefreshVectorVariablesListWorker(false);
}

void CEdBehaviorVariableEditor::RefreshInternalVectorVariablesList()
{
	RefreshVectorVariablesListWorker(true);
}

void CEdBehaviorVariableEditor::RefreshEventsList()
{	
	wxListBox* lb = XRCCTRL( *this, "eventsList", wxListBox );

	Int32 selection = lb->GetSelection();

	lb->Clear();
	for( Uint32 i=0; i<GetBehaviorGraph()->GetEvents().GetNumEvents(); ++i )
	{
		CBehaviorEventDescription *event = GetBehaviorGraph()->GetEvents().GetEvent( i );
		const CName& currEvent = event->GetEventName();
		lb->Append( currEvent.AsString().AsChar(), event );
	}

	if ( selection < (int)lb->GetCount() )
	{
		lb->SetSelection( selection );
	}

	ShowSelectedEvent();
}

void CEdBehaviorVariableEditor::ShowSelectedVariableWorker(Bool forInternalVariable)
{
	wxListBox* lb = XRCCTRL( *this, forInternalVariable? "internalVariablesList" : "variablesList", wxListBox );
	wxNotebook* notebook = XRCCTRL( *this, "notebook", wxNotebook );
	wxPanel* varsPage = XRCCTRL( *this, forInternalVariable? "internalVariablesPanel" : "variablesPanel", wxPanel );

	if ( notebook->GetCurrentPage() != varsPage )
		return;

	Int32 selection = lb->GetSelection();
	if ( selection != wxNOT_FOUND )
	{
		const CBehaviorVariablesList& variables = forInternalVariable? GetBehaviorGraph()->GetInternalVariables() : GetBehaviorGraph()->GetVariables();
		CBehaviorVariable* variable = variables.GetVariable( CName( lb->GetStringSelection().wc_str() ) );

		SetObjectProperty( variable );
	}
	else
	{
		SetObjectProperty( NULL );
	}

	UpdateValueSliderWorker(forInternalVariable);

	UpdateControls();
}

void CEdBehaviorVariableEditor::ShowSelectedVariable()
{
	ShowSelectedVariableWorker(false);
}

void CEdBehaviorVariableEditor::ShowSelectedInternalVariable()
{
	ShowSelectedVariableWorker(true);
}

void CEdBehaviorVariableEditor::SelectItemWorker(const String& varName, wxListBox* inListBox)
{
	wxString item = varName.AsChar();

	wxArrayString& arr = inListBox->GetStrings();
	for (size_t i=0; i<arr.GetCount(); i++)
	{
		if (arr[i] == item)
		{
			inListBox->Select(i);
		}
	}
}

void CEdBehaviorVariableEditor::SelectVariable(const String& varName)
{
	SelectItemWorker( varName, XRCCTRL( *this, "variablesList", wxListBox ) );
}

void CEdBehaviorVariableEditor::SelectVectorVariable(const String& varName)
{
	SelectItemWorker( varName, XRCCTRL( *this, "vectorVariablesList", wxListBox ) );
}

void CEdBehaviorVariableEditor::SelectEvent( const CName& eventName )
{
	SelectItemWorker( eventName.AsString(), XRCCTRL( *this, "eventsList", wxListBox ) );
}

void CEdBehaviorVariableEditor::SelectInternalVariable(const String& varName)
{
	SelectItemWorker( varName, XRCCTRL( *this, "internalVariablesList", wxListBox ) );
}

void CEdBehaviorVariableEditor::SelectInternalVectorVariable(const String& varName)
{
	SelectItemWorker( varName, XRCCTRL( *this, "internalVectorVariablesList", wxListBox ) );
}

void CEdBehaviorVariableEditor::ShowSelectedVectorVariableWorker(Bool forInternalVariable)
{
	wxListBox* lb = XRCCTRL( *this, forInternalVariable? "internalVectorVariablesList" : "vectorVariablesList", wxListBox );
	wxNotebook* notebook = XRCCTRL( *this, "notebook", wxNotebook );
	wxPanel* varsPage = XRCCTRL( *this, forInternalVariable? "internalVectorVariablesPanel" : "vectorVariablesPanel", wxPanel );

	if ( notebook->GetCurrentPage() != varsPage )
		return;

	Int32 selection = lb->GetSelection();
	if ( selection != wxNOT_FOUND )
	{
		const CBehaviorVectorVariablesList& variables = forInternalVariable? GetBehaviorGraph()->GetInternalVectorVariables() : GetBehaviorGraph()->GetVectorVariables();
		CBehaviorVectorVariable* variable = variables.GetVariable( CName( lb->GetStringSelection().wc_str() ) );
		
		SetObjectProperty( variable );
	}
	else
	{
		SetObjectProperty( NULL );
	}

	UpdateVectorValueSliderWorker(forInternalVariable);

	UpdateControls();
}

void CEdBehaviorVariableEditor::ShowSelectedVectorVariable()
{
	ShowSelectedVectorVariableWorker(false);
}

void CEdBehaviorVariableEditor::ShowSelectedInternalVectorVariable()
{
	ShowSelectedVectorVariableWorker(true);
}

void CEdBehaviorVariableEditor::UpdateValueSliderWorker(Bool forInternalVariable)
{
	wxListBox* lb = XRCCTRL( *this, forInternalVariable? "internalVariablesList" : "variablesList", wxListBox );
	Int32 selection = lb->GetSelection();
	wxSlider* slider = XRCCTRL( *this, forInternalVariable? "internalValueSlider" : "valueSlider", wxSlider );

	if ( selection == wxNOT_FOUND )
	{
		slider->Disable();
		return;
	}

	CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();

	if( !instance )
	{
		ASSERT( false && "There is no CBehaviorGraphInstance set" );
		return;
	}

	const CName varName = CName( lb->GetStringSelection().wc_str() );

	const Float minValue = forInternalVariable? instance->GetInternalFloatValueMin( varName ) : instance->GetFloatValueMin( varName );
	const Float maxValue = forInternalVariable? instance->GetInternalFloatValueMax( varName ) : instance->GetFloatValueMax( varName );
	const Float value = forInternalVariable? instance->GetInternalFloatValue( varName ) : instance->GetFloatValue( varName );

	// disable slider if range is too large
	if ( fabsf( maxValue - minValue ) > MAX_SLIDER_VALUE_RANGE )
	{
		slider->Disable();
		return;
	}

	slider->Enable();	
	slider->SetRange( (Int32)(minValue * SLIDER_VALUE_SCALE), (Int32)(maxValue * SLIDER_VALUE_SCALE) );
	slider->SetValue( (Int32)(value * SLIDER_VALUE_SCALE) );
}

void CEdBehaviorVariableEditor::UpdateValueSlider()
{
	UpdateValueSliderWorker(false);
}

void CEdBehaviorVariableEditor::UpdateInternalValueSlider()
{
	UpdateValueSliderWorker(true);
}

void CEdBehaviorVariableEditor::UpdateVectorValueSliderWorker(Bool forInternalVariable)
{
	wxListBox* lb = XRCCTRL( *this, forInternalVariable? "internalVectorVariablesList" : "vectorVariablesList", wxListBox );
	Int32 selection = lb->GetSelection();
	wxSlider* sliderX = XRCCTRL( *this, forInternalVariable? "internalValueSliderX" : "valueSliderX", wxSlider );
	wxSlider* sliderY = XRCCTRL( *this, forInternalVariable? "internalValueSliderY" : "valueSliderY", wxSlider );
	wxSlider* sliderZ = XRCCTRL( *this, forInternalVariable? "internalValueSliderZ" : "valueSliderZ", wxSlider );

	if ( selection == wxNOT_FOUND )
	{
		sliderX->Disable();
		sliderY->Disable();
		sliderZ->Disable();
		return;
	}

	Vector minValue;
	Vector maxValue;
	Vector value;

	GetVectorSliderRange( CName( lb->GetStringSelection().wc_str() ), minValue, maxValue, value, forInternalVariable );

	// disable slider if range is too large
	if ( fabsf( maxValue.X - minValue.X ) > MAX_SLIDER_VECTOR_VALUE_RANGE )
	{
		sliderX->Disable();
	}
	else
	{
		sliderX->Enable();	
		sliderX->SetRange( (Int32)(minValue.X * SLIDER_VECTOR_VALUE_SCALE), (Int32)(maxValue.X * SLIDER_VECTOR_VALUE_SCALE) );
		sliderX->SetValue( (Int32)(value.X * SLIDER_VECTOR_VALUE_SCALE) );
	}

	if ( fabsf( maxValue.Y - minValue.Y ) > MAX_SLIDER_VECTOR_VALUE_RANGE )
	{
		sliderY->Disable();
	}
	else
	{
		sliderY->Enable();	
		sliderY->SetRange( (Int32)(minValue.Y * SLIDER_VECTOR_VALUE_SCALE), (Int32)(maxValue.Y * SLIDER_VECTOR_VALUE_SCALE) );
		sliderY->SetValue( (Int32)(value.Y * SLIDER_VECTOR_VALUE_SCALE) );
	}

	if ( fabsf( maxValue.Z - minValue.Z ) > MAX_SLIDER_VECTOR_VALUE_RANGE )
	{
		sliderZ->Disable();
	}
	else
	{
		sliderZ->Enable();	
		sliderZ->SetRange( (Int32)(minValue.Z * SLIDER_VECTOR_VALUE_SCALE), (Int32)(maxValue.Z * SLIDER_VECTOR_VALUE_SCALE) );
		sliderZ->SetValue( (Int32)(value.Z * SLIDER_VECTOR_VALUE_SCALE) );
	}
}

void CEdBehaviorVariableEditor::UpdateVectorValueSlider()
{
	UpdateVectorValueSliderWorker(false);
}

void CEdBehaviorVariableEditor::UpdateInternalVectorValueSlider()
{
	UpdateVectorValueSliderWorker(true);
}

void CEdBehaviorVariableEditor::UpdateVectorValueSliderScaleWorker(Bool forInternalVariable)
{
	wxToggleButton* butt1 = XRCCTRL( *this, forInternalVariable? "buttInternalVecVar1" : "buttVecVar1", wxToggleButton );
	wxToggleButton* butt10 = XRCCTRL( *this, forInternalVariable? "buttInternalVecVar10" : "buttVecVar10", wxToggleButton );
	wxToggleButton* butt100 = XRCCTRL( *this, forInternalVariable? "buttInternalVecVar100" : "buttVecVar100", wxToggleButton );

	butt1->SetValue( false );
	butt10->SetValue( false );
	butt100->SetValue( false );

	// share this variable between normal and internal vectors
	if ( m_vecSliderScale == VSS_1 )
	{
		butt1->SetValue( true );
	}
	else if ( m_vecSliderScale == VSS_10 )
	{
		butt10->SetValue( true );
	}
	else if ( m_vecSliderScale == VSS_100 )
	{
		butt100->SetValue( true );
	}

	if (forInternalVariable)
	{
		UpdateInternalVectorValueSlider();
	}
	else
	{
		UpdateVectorValueSlider();
	}
}

void CEdBehaviorVariableEditor::UpdateVectorValueSliderScale()
{
	UpdateVectorValueSliderScaleWorker(false);
}

void CEdBehaviorVariableEditor::UpdateInternalVectorValueSliderScale()
{
	UpdateVectorValueSliderScaleWorker(true);
}

void CEdBehaviorVariableEditor::GetVectorSliderRange( const CName var, Vector& min, Vector& max, Vector& value, Bool forInternalVariable ) const
{
	CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();

	if( !instance )
	{
		ASSERT( false && "There is no CBehaviorGraphInstance set" );
		return;
	}

	// share this variable between normal and internal vectors
	if ( m_vecSliderScale == VSS_Var )
	{
		min = forInternalVariable? instance->GetInternalVectorValueMin( var ) : instance->GetVectorValueMin( var );
		max = forInternalVariable? instance->GetInternalVectorValueMax( var ) : instance->GetVectorValueMax( var );
	}
	else if ( m_vecSliderScale == VSS_1 )
	{
		max = Vector( 1.f, 1.f, 1.f );
		min = -max;
	}
	else if ( m_vecSliderScale == VSS_10 )
	{
		max = Vector( 10.f, 10.f, 10.f );
		min = -max;
	}
	else if ( m_vecSliderScale == VSS_100 )
	{
		max = Vector( 100.f, 100.f, 100.f );
		min = -max;
	}

	value = forInternalVariable? instance->GetInternalVectorValue( var ) : instance->GetVectorValue( var );

	value.X = Clamp( value.X, min.X, max.X );
	value.Y = Clamp( value.Y, min.Y, max.Y );
	value.Z = Clamp( value.Z, min.Z, max.Z );
}

void CEdBehaviorVariableEditor::OnVectorSliderScale1( wxCommandEvent& event )
{
	// share this variable between normal and internal vectors
	m_vecSliderScale = event.IsChecked() ? VSS_1 : VSS_Var;
	UpdateVectorValueSliderScale();
	UpdateInternalVectorValueSliderScale();
}

void CEdBehaviorVariableEditor::OnVectorSliderScale10( wxCommandEvent& event )
{
	// share this variable between normal and internal vectors
	m_vecSliderScale = event.IsChecked() ? VSS_10 : VSS_Var;
	UpdateVectorValueSliderScale();
	UpdateInternalVectorValueSliderScale();
}

void CEdBehaviorVariableEditor::OnVectorSliderScale100( wxCommandEvent& event )
{
	// share this variable between normal and internal vectors
	m_vecSliderScale = event.IsChecked() ? VSS_100 : VSS_Var;
	UpdateVectorValueSliderScale();
	UpdateInternalVectorValueSliderScale();
}

void CEdBehaviorVariableEditor::OnInternalVectorSliderScale1( wxCommandEvent& event )
{
	// share this variable between normal and internal vectors
	m_vecSliderScale = event.IsChecked() ? VSS_1 : VSS_Var;
	UpdateVectorValueSliderScale();
	UpdateInternalVectorValueSliderScale();
}

void CEdBehaviorVariableEditor::OnInternalVectorSliderScale10( wxCommandEvent& event )
{
	// share this variable between normal and internal vectors
	m_vecSliderScale = event.IsChecked() ? VSS_10 : VSS_Var;
	UpdateVectorValueSliderScale();
	UpdateInternalVectorValueSliderScale();
}

void CEdBehaviorVariableEditor::OnInternalVectorSliderScale100( wxCommandEvent& event )
{
	// share this variable between normal and internal vectors
	m_vecSliderScale = event.IsChecked() ? VSS_100 : VSS_Var;
	UpdateVectorValueSliderScale();
	UpdateInternalVectorValueSliderScale();
}

void CEdBehaviorVariableEditor::OnValueSliderWorker( wxScrollEvent &event, Bool forInternalVariable )
{
	wxListBox* lb = XRCCTRL( *this, forInternalVariable? "internalVariablesList" : "variablesList", wxListBox );
	Int32 selection = lb->GetSelection();
	wxSlider* slider = XRCCTRL( *this, forInternalVariable? "internalValueSlider" : "valueSlider", wxSlider );

	if ( selection == wxNOT_FOUND )
		return;

	Int32 newValue = slider->GetValue();

	const CName varName = CName( lb->GetStringSelection().wc_str() );
	Float valToSet = (Float)newValue / SLIDER_VALUE_SCALE;

	if ( m_undoManager && event.GetEventType() != wxEVT_SCROLL_CHANGED )
	{
		if (forInternalVariable)
		{
			CUndoBehaviourGraphVariableChange::PrepareInternalScalarStep( *m_undoManager, GetEditor()->GetGraphEditor(), this, varName, valToSet );
		}
		else
		{
			CUndoBehaviourGraphVariableChange::PrepareScalarStep( *m_undoManager, GetEditor()->GetGraphEditor(), this, varName, valToSet );
		}
	}

	if (forInternalVariable)
	{
		SetInternalVariable( varName, valToSet );
	}
	else
	{
		SetVariable( varName, valToSet );
	}

	m_variableProperties->Get().RefreshValues();

	RefreshControlList();

	if ( m_undoManager && event.GetEventType() == wxEVT_SCROLL_THUMBRELEASE )
	{
		CUndoBehaviourGraphVariableChange::FinalizeStep( *m_undoManager );
	}
}

void CEdBehaviorVariableEditor::OnValueSlider( wxScrollEvent &event )
{
	OnValueSliderWorker( event, false );
}

void CEdBehaviorVariableEditor::OnInternalValueSlider( wxScrollEvent &event )
{
	OnValueSliderWorker( event, true );
}

void CEdBehaviorVariableEditor::OnVectorValueSliderWorker( wxScrollEvent &event, Bool forInternalVariable )
{
	wxListBox* lb = XRCCTRL( *this, forInternalVariable? "internalVectorVariablesList" : "vectorVariablesList", wxListBox );
	Int32 selection = lb->GetSelection();
	wxSlider* sliderX = XRCCTRL( *this, forInternalVariable? "internalValueSliderX" : "valueSliderX", wxSlider );
	wxSlider* sliderY = XRCCTRL( *this, forInternalVariable? "internalValueSliderY" : "valueSliderY", wxSlider );
	wxSlider* sliderZ = XRCCTRL( *this, forInternalVariable? "internalValueSliderZ" : "valueSliderZ", wxSlider );

	if ( selection == wxNOT_FOUND )
		return;

	Int32 newValueX = sliderX->GetValue();
	Int32 newValueY = sliderY->GetValue();
	Int32 newValueZ = sliderZ->GetValue();

	Vector vecVar = Vector((Float)newValueX / SLIDER_VECTOR_VALUE_SCALE, (Float)newValueY / SLIDER_VECTOR_VALUE_SCALE , (Float)newValueZ / SLIDER_VECTOR_VALUE_SCALE);
	const CName varName = CName( lb->GetStringSelection().wc_str() );

	if ( m_undoManager && event.GetEventType() != wxEVT_SCROLL_CHANGED  )
	{
		if (forInternalVariable)
		{
			CUndoBehaviourGraphVariableChange::PrepareInternalVectorStep( *m_undoManager, GetEditor()->GetGraphEditor(), this, varName, vecVar );
		}
		else
		{
			CUndoBehaviourGraphVariableChange::PrepareVectorStep( *m_undoManager, GetEditor()->GetGraphEditor(), this, varName, vecVar );
		}
	}

	if (forInternalVariable)
	{
		SetInternalVariable( varName, vecVar );
	}
	else
	{
		SetVariable( varName, vecVar );
	}

	m_variableProperties->Get().RefreshValues();

	RefreshControlList();

	ShowVectorInPreview();

	if ( m_undoManager && event.GetEventType() == wxEVT_SCROLL_THUMBRELEASE )
	{
		CUndoBehaviourGraphVariableChange::FinalizeStep( *m_undoManager );
	}
}

void CEdBehaviorVariableEditor::OnVectorValueSlider( wxScrollEvent &event )
{
	OnVectorValueSliderWorker( event, false );
}

void CEdBehaviorVariableEditor::OnInternalVectorValueSlider( wxScrollEvent &event )
{
	OnVectorValueSliderWorker( event, true );
}

void CEdBehaviorVariableEditor::ShowSelectedEvent()
{
	wxListBox* lb = XRCCTRL( *this, "eventsList", wxListBox );
	wxNotebook* notebook = XRCCTRL( *this, "notebook", wxNotebook );
	wxPanel* eventsPage = XRCCTRL( *this, "eventsPanel", wxPanel );

	if ( notebook->GetCurrentPage() != eventsPage )
		return;


	Int32 selection = lb->GetSelection();
	if ( selection != wxNOT_FOUND )
	{
		SetObjectProperty( (CBehaviorEventDescription*)( lb->GetClientData(selection) ) );
	}
	else
	{
		SetObjectProperty( NULL );
	}

	UpdateControls();
}

void CEdBehaviorVariableEditor::OnVarListBoxSelChange( wxCommandEvent &event )
{
	ShowSelectedVariable();
}

void CEdBehaviorVariableEditor::OnVectorVarListBoxSelChange( wxCommandEvent &event )
{
	ShowSelectedVectorVariable();

	ShowVectorInPreview();
}

void CEdBehaviorVariableEditor::OnInternalVarListBoxSelChange( wxCommandEvent &event )
{
	ShowSelectedInternalVariable();
}

void CEdBehaviorVariableEditor::OnInternalVectorVarListBoxSelChange( wxCommandEvent &event )
{
	ShowSelectedInternalVectorVariable();

	ShowVectorInPreview();
}

void CEdBehaviorVariableEditor::OnEventsListBoxSelChange( wxCommandEvent &event )
{
	ShowSelectedEvent();
}

void CEdBehaviorVariableEditor::OnEventsListRaiseEvent( wxCommandEvent &event )
{
	OnRaiseEvent( event );
}

void CEdBehaviorVariableEditor::OnPropertiesChanged( wxCommandEvent &event )
{
	CEdPropertiesPage::SPropertyEventData* eventData = static_cast<CEdPropertiesPage::SPropertyEventData*>( event.GetClientData() );

	if ( eventData->m_propertyName != TXT("value") )
	{
		// Following call must be delayed as it rebuilds the page from which the event is sent.
		RunLaterOnce( [this](){ GetEditor()->BehaviorGraphModified(); } );
		EventDuplicationTest();
		return;
	}

	if ( m_variableSelectedObject->IsA< CBehaviorVariable >() )
	{
		CBehaviorVariable* var = Cast< CBehaviorVariable >( m_variableSelectedObject );
		if (GetBehaviorGraph()->GetInternalVariables().DoesContain(var))
		{
			GetBehaviorGraphInstance()->SetInternalFloatValue( var->m_name, var->GetValue() );
		}
		else
		{
			GetBehaviorGraphInstance()->SetFloatValue( var->m_name, var->GetValue() );
		}
	}
	else if ( m_variableSelectedObject->IsA< CBehaviorVectorVariable >() )
	{
		CBehaviorVectorVariable* var = Cast< CBehaviorVectorVariable >( m_variableSelectedObject );
		if (GetBehaviorGraph()->GetInternalVectorVariables().DoesContain(var))
		{
			GetBehaviorGraphInstance()->SetInternalVectorValue( var->m_name, var->GetValue() );
		}
		else
		{
			GetBehaviorGraphInstance()->SetVectorValue( var->m_name, var->GetValue() );
		}
	}

 	RefreshVariablesList();
 
 	UpdateValueSlider();
 
 	RefreshVectorVariablesList();
 
 	UpdateVectorValueSlider();
 
 	RefreshEventsList();
 
	RefreshInternalVariablesList();

	UpdateInternalValueSlider();

	RefreshInternalVectorVariablesList();

	UpdateInternalVectorValueSlider();

	UpdateControls();
 
 	RefreshControlList();
 
 	ShowVectorInPreview();
}

void CEdBehaviorVariableEditor::OnAddVariableWorker( wxCommandEvent &event, Bool forInternalVariable )
{
	Uint32 newVarIndex = 0;
	
	String newVarName = String::Printf( TXT("newVar%02d"), newVarIndex );
	CBehaviorVariablesList& variables = forInternalVariable? GetBehaviorGraph()->GetInternalVariables() : GetBehaviorGraph()->GetVariables();
	while( variables.GetVariable( CName( newVarName.AsChar() ) ) )
	{
		++newVarIndex;
		newVarName = String::Printf( TXT("newVar%02d"), newVarIndex );
	}

	if ( m_undoManager )
	{
		CUndoBehaviorGraphVariableExistance::CreateAddingStep( *m_undoManager, GetEditor()->GetGraphEditor(), this, CName( newVarName ), forInternalVariable? CUndoBehaviorGraphVariableExistance::INTERNALSCALAR : CUndoBehaviorGraphVariableExistance::SCALAR );
	}
	variables.AddVariable( CName( newVarName ) );

	GetEditor()->BehaviorGraphModified();

	if (forInternalVariable)
	{
		RefreshInternalVariablesList();
		SelectInternalVariable(newVarName);
		ShowSelectedInternalVariable();
		RefreshInternalVariablesList();
	}
	else
	{
		RefreshVariablesList();
		SelectVariable(newVarName);
		ShowSelectedVariable();
		RefreshVariablesList();
	}
}

void CEdBehaviorVariableEditor::OnAddVariable( wxCommandEvent &event )
{
	OnAddVariableWorker(event, false);
}

void CEdBehaviorVariableEditor::OnAddInternalVariable( wxCommandEvent &event )
{
	OnAddVariableWorker(event, true);
}

void CEdBehaviorVariableEditor::OnCopyVariablesWorker( wxCommandEvent &event, Bool forInternalVariable )
{
	CBehaviorVariablesList& variables = forInternalVariable? GetBehaviorGraph()->GetInternalVariables() : GetBehaviorGraph()->GetVariables();

	TDynArray< CBehaviorVariable* > allToCopy;
	variables.GetVariables(allToCopy);

	// Serialize to memory package
	TDynArray< Uint8 > buffer;
	CMemoryFileWriter writer( buffer );
	CDependencySaver saver( writer, NULL );

	// Save object
	DependencySavingContext context( (const DependencyMappingContext::TObjectsToMap&) allToCopy );
	if ( !saver.SaveObjects( context ) )
	{
		WARN_EDITOR( TXT("Unable to copy selected objects to clipboard") );
		return;
	}

	// Open clipboard
	if ( wxTheClipboard->Open() )
	{
		wxTheClipboard->SetData( new CClipboardData( forInternalVariable? TXT("BehaviorInternalVariables") : TXT("BehaviorVariables"), buffer ) );
		wxTheClipboard->Close();
	}
}

void CEdBehaviorVariableEditor::OnCopyVariables( wxCommandEvent &event )
{
	OnCopyVariablesWorker(event, false);
}

void CEdBehaviorVariableEditor::OnCopyInternalVariables( wxCommandEvent &event )
{
	OnCopyVariablesWorker(event, true);
}

void CEdBehaviorVariableEditor::OnAddVectorVariableWorker( wxCommandEvent &event, Bool forInternalVariable )
{
	Uint32 newVarIndex = 0;

	String newVarName = String::Printf( TXT("newVar%02d"), newVarIndex );
	CBehaviorVectorVariablesList& variables = forInternalVariable? GetBehaviorGraph()->GetInternalVectorVariables() : GetBehaviorGraph()->GetVectorVariables();
	while( variables.GetVariable( CName( newVarName ) ) )
	{
		++newVarIndex;
		newVarName = String::Printf( TXT("newVar%02d"), newVarIndex );
	}

	if ( m_undoManager )
	{
		CUndoBehaviorGraphVariableExistance::CreateAddingStep( *m_undoManager, GetEditor()->GetGraphEditor(), this, CName( newVarName ), forInternalVariable? CUndoBehaviorGraphVariableExistance::INTERNALVECTOR : CUndoBehaviorGraphVariableExistance::VECTOR );
	}

	variables.AddVariable( CName( newVarName ) );

	GetEditor()->BehaviorGraphModified();

	if (forInternalVariable)
	{
		RefreshInternalVectorVariablesList();
		SelectInternalVectorVariable(newVarName);
		ShowSelectedInternalVectorVariable();
		RefreshInternalVariablesList();
	}
	else
	{
		RefreshVectorVariablesList();
		SelectVectorVariable(newVarName);
		ShowSelectedVectorVariable();
		RefreshVariablesList();
	}
}

void CEdBehaviorVariableEditor::OnAddVectorVariable( wxCommandEvent &event )
{
	OnAddVectorVariableWorker(event, false);
}

void CEdBehaviorVariableEditor::OnAddInternalVectorVariable( wxCommandEvent &event )
{
	OnAddVectorVariableWorker(event, true);
}

void CEdBehaviorVariableEditor::OnCopyVectorVariablesWorker( wxCommandEvent &event, Bool forInternalVariable )
{
	CBehaviorVectorVariablesList& variables = forInternalVariable? GetBehaviorGraph()->GetInternalVectorVariables() : GetBehaviorGraph()->GetVectorVariables();

	TDynArray< CBehaviorVectorVariable* > allToCopy;
	variables.GetVariables(allToCopy);

	// Serialize to memory package
	TDynArray< Uint8 > buffer;
	CMemoryFileWriter writer( buffer );
	CDependencySaver saver( writer, NULL );

	// Save object
	DependencySavingContext context( (const DependencyMappingContext::TObjectsToMap&) allToCopy );
	if ( !saver.SaveObjects( context ) )
	{
		WARN_EDITOR( TXT("Unable to copy selected objects to clipboard") );
		return;
	}

	// Open clipboard
	if ( wxTheClipboard->Open() )
	{
		wxTheClipboard->SetData( new CClipboardData( forInternalVariable? TXT("BehaviorInternalVectorVariables") : TXT("BehaviorVectorVariables"), buffer ) );
		wxTheClipboard->Close();
	}
}

void CEdBehaviorVariableEditor::OnCopyVectorVariables( wxCommandEvent &event )
{
	OnCopyVectorVariablesWorker(event, false);
}

void CEdBehaviorVariableEditor::OnCopyInternalVectorVariables( wxCommandEvent &event )
{
	OnCopyVectorVariablesWorker(event, true);
}

void CEdBehaviorVariableEditor::OnRemoveVariableWorker( wxCommandEvent &event, bool forInternalVariable )
{
	wxListBox* lb = XRCCTRL( *this, forInternalVariable? "internalVariablesList" : "variablesList", wxListBox );

	Int32 selection = lb->GetSelection();
	if ( selection != wxNOT_FOUND )
	{
		const CName varName = CName( lb->GetString( selection ).wc_str() );

		if ( m_undoManager )
		{
			CUndoBehaviorGraphVariableExistance::CreateRemovingStep( *m_undoManager, GetEditor()->GetGraphEditor(), this, varName, forInternalVariable? CUndoBehaviorGraphVariableExistance::INTERNALSCALAR : CUndoBehaviorGraphVariableExistance::SCALAR );
		}

		(forInternalVariable? GetBehaviorGraph()->GetInternalVariables() : GetBehaviorGraph()->GetVariables()).RemoveVariable( varName );

		GetEditor()->BehaviorGraphModified();

		lb->SetSelection( wxNOT_FOUND );

		if (forInternalVariable)
		{
			ShowSelectedInternalVariable();
			RefreshInternalVariablesList();
		}
		else
		{
			ShowSelectedVariable();
			RefreshVariablesList();
		}
		RefreshControlList();
	}
}

void CEdBehaviorVariableEditor::OnRemoveVariable( wxCommandEvent &event )
{
	OnRemoveVariableWorker(event, false);
}

void CEdBehaviorVariableEditor::OnRemoveInternalVariable( wxCommandEvent &event )
{
	OnRemoveVariableWorker(event, true);
}

void CEdBehaviorVariableEditor::OnPasteVariablesWorker( wxCommandEvent &event, Bool forInternalVariable )
{
	CBehaviorVariablesList& variables = forInternalVariable? GetBehaviorGraph()->GetInternalVariables() : GetBehaviorGraph()->GetVariables();

	if ( wxTheClipboard->Open())
	{
		CClipboardData data( forInternalVariable? TXT("BehaviorInternalVariables") : TXT("BehaviorVariables") );
		if ( wxTheClipboard->IsSupported( data.GetDataFormat() ) )
		{
			// Extract data from the clipboard
			if ( wxTheClipboard->GetData( data ) )
			{
				const TDynArray< Uint8 >& objData = data.GetData();

				// Deserialize
				CMemoryFileReader reader( objData, 0 );
				CDependencyLoader loader( reader, NULL );
				DependencyLoadingContext loadingContext;

				loadingContext.m_parent = Cast< CBehaviorGraph >( GetEditor()->GetBehaviorGraph() );
				if ( loader.LoadObjects( loadingContext ) )
				{
					// Call post load of spawned objects
					loader.PostLoad();

					for ( auto iObject = loadingContext.m_loadedRootObjects.Begin(); iObject != loadingContext.m_loadedRootObjects.End(); ++ iObject )
					{
						if ( CBehaviorVariable * variable = Cast<CBehaviorVariable>(*iObject) )
						{
							CBehaviorVariable * existing = variables.GetVariable( variable->GetName() );
							if ( ! existing )
							{
								variables.AddVariable( variable->GetName() );
								existing = variables.GetVariable( variable->GetName() );
							}
							existing->Set( variable );
						}
					}

					GetEditor()->BehaviorGraphModified();
				}
			}
		}

		wxTheClipboard->Close();
	}

	if (forInternalVariable)
	{
		RefreshInternalVariablesList();
	}
	else
	{
		RefreshVariablesList();
	}
	RefreshControlList();
}

void CEdBehaviorVariableEditor::OnPasteVariables( wxCommandEvent &event )
{
	OnPasteVariablesWorker(event, false);
}

void CEdBehaviorVariableEditor::OnPasteInternalVariables( wxCommandEvent &event )
{
	OnPasteVariablesWorker(event, true);
}

void CEdBehaviorVariableEditor::OnRemoveVectorVariableWorker( wxCommandEvent &event, Bool forInternalVariable )
{
	wxListBox* lb = XRCCTRL( *this, forInternalVariable? "internalVectorVariablesList" : "vectorVariablesList", wxListBox );

	Int32 selection = lb->GetSelection();
	if ( selection != wxNOT_FOUND )
	{
		CBehaviorVectorVariable* variable = (CBehaviorVectorVariable*)( lb->GetClientData(selection) );
		//m_behaviorEditor->GetPreviewPanel()->RemovePreviewHelperFor( variable );

		const CName varName = CName( lb->GetString( selection ).wc_str() );

		if ( m_undoManager )
		{
			CUndoBehaviorGraphVariableExistance::CreateRemovingStep( *m_undoManager, GetEditor()->GetGraphEditor(), this, varName, forInternalVariable? CUndoBehaviorGraphVariableExistance::INTERNALVECTOR : CUndoBehaviorGraphVariableExistance::VECTOR );
		}

		(forInternalVariable? GetBehaviorGraph()->GetInternalVectorVariables() : GetBehaviorGraph()->GetVectorVariables()).RemoveVariable( varName );

		GetEditor()->BehaviorGraphModified();

		lb->SetSelection( wxNOT_FOUND );

		if (forInternalVariable)
		{
			ShowSelectedInternalVectorVariable();
			RefreshInternalVectorVariablesList();
		}
		else
		{
			ShowSelectedVectorVariable();
			RefreshVectorVariablesList();
		}
		RefreshControlList();
		ShowVectorInPreview();
	}
}

void CEdBehaviorVariableEditor::OnRemoveVectorVariable( wxCommandEvent &event )
{
	OnRemoveVectorVariableWorker(event, false);
}

void CEdBehaviorVariableEditor::OnRemoveInternalVectorVariable( wxCommandEvent &event )
{
	OnRemoveVectorVariableWorker(event, true);
}

void CEdBehaviorVariableEditor::OnPasteVectorVariablesWorker( wxCommandEvent &event, Bool forInternalVariable )
{
	CBehaviorVectorVariablesList& variables = forInternalVariable? GetBehaviorGraph()->GetInternalVectorVariables() : GetBehaviorGraph()->GetVectorVariables();

	if ( wxTheClipboard->Open())
	{
		CClipboardData data( forInternalVariable? TXT("BehaviorInternalVectorVariables") : TXT("BehaviorVectorVariables") );
		if ( wxTheClipboard->IsSupported( data.GetDataFormat() ) )
		{
			// Extract data from the clipboard
			if ( wxTheClipboard->GetData( data ) )
			{
				const TDynArray< Uint8 >& objData = data.GetData();

				// Deserialize
				CMemoryFileReader reader( objData, 0 );
				CDependencyLoader loader( reader, NULL );
				DependencyLoadingContext loadingContext;

				loadingContext.m_parent = Cast< CBehaviorGraph >( GetEditor()->GetBehaviorGraph() );
				if ( loader.LoadObjects( loadingContext ) )
				{
					// Call post load of spawned objects
					loader.PostLoad();

					for ( auto iObject = loadingContext.m_loadedRootObjects.Begin(); iObject != loadingContext.m_loadedRootObjects.End(); ++ iObject )
					{
						if ( CBehaviorVectorVariable * variable = Cast<CBehaviorVectorVariable>(*iObject) )
						{
							CBehaviorVectorVariable * existing = variables.GetVariable( variable->GetName() );
							if ( ! existing )
							{
								variables.AddVariable( variable->GetName() );
								existing = variables.GetVariable( variable->GetName() );
							}
							existing->Set( variable );
						}
					}

					GetEditor()->BehaviorGraphModified();
				}
			}
		}

		wxTheClipboard->Close();
	}

	if (forInternalVariable)
	{
		RefreshInternalVectorVariablesList();
	}
	else
	{
		RefreshVectorVariablesList();
	}
	RefreshControlList();
}

void CEdBehaviorVariableEditor::OnPasteVectorVariables( wxCommandEvent &event )
{
	OnPasteVectorVariablesWorker(event, false);
}

void CEdBehaviorVariableEditor::OnPasteInternalVectorVariables( wxCommandEvent &event )
{
	OnPasteVectorVariablesWorker(event, true);
}

void CEdBehaviorVariableEditor::EventDuplicationTest()
{
	if( !GetBehaviorGraph() )
		return;

	if( GetBehaviorGraph()->GetEvents().DuplicationTestName() )
	{
		wxMessageBox( TXT("Duplicated events found, check the log"), TXT("Warning!"), wxOK | wxICON_EXCLAMATION, this );		
	}
}

void CEdBehaviorVariableEditor::OnAddEvent( wxCommandEvent &event )
{
	Uint32 newEventIndex = 0;

	String newEventName = String::Printf( TXT("newEvent%02d"), newEventIndex );
	while( GetBehaviorGraph()->GetEvents().GetEventId( CName( newEventName ) ) != CBehaviorEventsList::NO_EVENT )
	{
		++newEventIndex;
		newEventName = String::Printf( TXT("newEvent%02d"), newEventIndex );
	}

	if ( m_undoManager )
	{
		CUndoBehaviorGraphVariableExistance::CreateAddingStep( *m_undoManager, GetEditor()->GetGraphEditor(), this, CName( newEventName ), CUndoBehaviorGraphVariableExistance::EVENT );
	}

	GetBehaviorGraph()->GetEvents().AddEvent( CName( newEventName ) );

	GetEditor()->BehaviorGraphModified();

	RefreshEventsList();

	SelectEvent( CName( newEventName ) );

	// select last 
	//wxListBox* lb = XRCCTRL( *this, "eventsList", wxListBox );
	//lb->SetSelection( m_behaviorEditor->GetBehaviorGraph()->GetEvents().GetNumEvents()-1 );

	ShowSelectedEvent();

	RefreshControlList();

	EventDuplicationTest();
}

void CEdBehaviorVariableEditor::OnRemoveEvent( wxCommandEvent &event )
{
	wxListBox* lb = XRCCTRL( *this, "eventsList", wxListBox );

	Int32 selection = lb->GetSelection();
	if ( selection != wxNOT_FOUND )
	{
		const CName eventName = CName( lb->GetString( selection ).wc_str() );

		if ( m_undoManager )
		{
			CUndoBehaviorGraphVariableExistance::CreateRemovingStep( *m_undoManager, GetEditor()->GetGraphEditor(), this, eventName, CUndoBehaviorGraphVariableExistance::EVENT );
		}

		GetBehaviorGraph()->GetEvents().RemoveEvent( eventName );

		GetEditor()->BehaviorGraphModified();

		lb->SetSelection( wxNOT_FOUND );
		ShowSelectedEvent();

		RefreshEventsList();

		RefreshControlList();
	}
}

void CEdBehaviorVariableEditor::OnCopyEvents( wxCommandEvent &event )
{
	CBehaviorEventsList & events = GetBehaviorGraph()->GetEvents();

	TDynArray< CBehaviorEventDescription* > allToCopy;
	events.GetEvents(allToCopy);

	// Serialize to memory package
	TDynArray< Uint8 > buffer;
	CMemoryFileWriter writer( buffer );
	CDependencySaver saver( writer, NULL );

	// Save object
	DependencySavingContext context( (const DependencyMappingContext::TObjectsToMap&) allToCopy );
	if ( !saver.SaveObjects( context ) )
	{
		WARN_EDITOR( TXT("Unable to copy selected objects to clipboard") );
		return;
	}

	// Open clipboard
	if ( wxTheClipboard->Open() )
	{
		wxTheClipboard->SetData( new CClipboardData( TXT("BehaviorEvents"), buffer ) );
		wxTheClipboard->Close();
	}
}

void CEdBehaviorVariableEditor::OnPasteEvents( wxCommandEvent &event )
{
	CBehaviorEventsList & events = GetBehaviorGraph()->GetEvents();

	if ( wxTheClipboard->Open())
	{
		CClipboardData data( TXT("BehaviorEvents") );
		if ( wxTheClipboard->IsSupported( data.GetDataFormat() ) )
		{
			// Extract data from the clipboard
			if ( wxTheClipboard->GetData( data ) )
			{
				const TDynArray< Uint8 >& objData = data.GetData();

				// Deserialize
				CMemoryFileReader reader( objData, 0 );
				CDependencyLoader loader( reader, NULL );
				DependencyLoadingContext loadingContext;

				loadingContext.m_parent = Cast< CBehaviorGraph >( GetEditor()->GetBehaviorGraph() );
				if ( loader.LoadObjects( loadingContext ) )
				{
					// Call post load of spawned objects
					loader.PostLoad();

					for ( auto iObject = loadingContext.m_loadedRootObjects.Begin(); iObject != loadingContext.m_loadedRootObjects.End(); ++ iObject )
					{
						if ( CBehaviorEventDescription * eventDesc = Cast<CBehaviorEventDescription>(*iObject) )
						{
							CBehaviorEventDescription * existing = events.GetEvent( eventDesc->GetEventName() );
							if ( ! existing )
							{
								events.AddEvent( eventDesc->GetEventName() );
								existing = events.GetEvent( eventDesc->GetEventName() );
							}
							existing->Set( eventDesc );
						}
					}

					GetEditor()->BehaviorGraphModified();
				}
			}
		}

		wxTheClipboard->Close();
	}

	RefreshEventsList();

	RefreshControlList();
}

void CEdBehaviorVariableEditor::OnRaiseEvent( wxCommandEvent &event )
{
	wxListBox* lb = XRCCTRL( *this, "eventsList", wxListBox );

	Int32 selection = lb->GetSelection();
	if ( selection != wxNOT_FOUND )
	{
		if ( CBehaviorGraphInstance *instance = GetBehaviorGraphInstance() )
		{
			instance->GenerateEvent( CName( lb->GetString( selection ).wc_str() ) );
		}
	}
}

void CEdBehaviorVariableEditor::OnRaiseForceEvent( wxCommandEvent &event )
{
	wxListBox* lb = XRCCTRL( *this, "eventsList", wxListBox );

	Int32 selection = lb->GetSelection();
	if ( selection != wxNOT_FOUND )
	{
		if ( CBehaviorGraphInstance *instance = GetBehaviorGraphInstance() )
		{
			instance->GenerateForceEvent( CName( lb->GetString( selection ).wc_str() ) );
		}
	}
}

void CEdBehaviorVariableEditor::OnPageChange( wxCommandEvent& event )
{	
	// above functions check if proper page is active anyway...
	ShowSelectedVariable();

	ShowSelectedVectorVariable();


	ShowSelectedEvent();

	ShowSelectedInternalVariable();

	ShowSelectedInternalVectorVariable();

	EventDuplicationTest();
}

void CEdBehaviorVariableEditor::SetVariable( const CName name, Float value )
{
	CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();
	CBehaviorGraph* graph = GetBehaviorGraph();

	if( !instance )
	{
		ASSERT( false && "There is no CBehaviorGraphInstance set" );
		return;
	}

	Bool ret = instance->SetFloatValue( name, value );
	ASSERT( ret );

	/* // modification of actual variable disabled, as it should just affect instance
	// modify actual variable, not just instance
	Int32 id = graph->GetVariables().GetVariableId( name.AsChar() );
	CBehaviorVariable* var = graph->GetVariables().GetVariableById( id );

	ASSERT( var );
	if ( var )
	{
		var->SetValue( value );
	}
	*/

	m_variableProperties->Get().RefreshValues();
}

void CEdBehaviorVariableEditor::SetVariable( const CName name, const Vector& value )
{
	CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();
	CBehaviorGraph* graph = GetBehaviorGraph();

	if( !instance )
	{
		ASSERT( false && "There is no CBehaviorGraphInstance set" );
		return;
	}

	Bool ret = instance->SetVectorValue( name, value );
	ASSERT( ret );

	/* // modification of actual variable disabled, as it should just affect instance
	// modify actual variable, not just instance
	Int32 id = graph->GetVectorVariables().GetVariableId( name.AsChar() );
	CBehaviorVectorVariable* var = graph->GetVectorVariables().GetVariableById( id );

	ASSERT( var );
	if ( var )
	{
		var->SetValue( value );
	}
	*/

	m_variableProperties->Get().RefreshValues();
}

void CEdBehaviorVariableEditor::SetInternalVariable( const CName name, Float value )
{
	CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();
	CBehaviorGraph* graph = GetBehaviorGraph();

	if( !instance )
	{
		ASSERT( false && "There is no CBehaviorGraphInstance set" );
		return;
	}

	Bool ret = instance->SetInternalFloatValue( name, value );
	ASSERT( ret );

	/* // modification of actual variable disabled, as it should just affect instance
	// modify actual variable, not just instance
	Int32 id = graph->GetInternalVariables().GetVariableId( name.AsChar() );
	CBehaviorVariable* var = graph->GetInternalVariables().GetVariableById( id );

	ASSERT( var );
	if ( var )
	{
		var->SetValue( value );
	}
	*/

	m_variableProperties->Get().RefreshValues();
}

void CEdBehaviorVariableEditor::SetInternalVariable( const CName name, const Vector& value )
{
	CBehaviorGraphInstance* instance = GetBehaviorGraphInstance();
	CBehaviorGraph* graph = GetBehaviorGraph();

	if( !instance )
	{
		ASSERT( false && "There is no CBehaviorGraphInstance set" );
		return;
	}

	Bool ret = instance->SetInternalVectorValue( name, value );
	ASSERT( ret );

	/* // modification of actual variable disabled, as it should just affect instance
	// modify actual variable, not just instance
	Int32 id = graph->GetInternalVectorVariables().GetVariableId( name.AsChar() );
	CBehaviorVectorVariable* var = graph->GetInternalVectorVariables().GetVariableById( id );

	ASSERT( var );
	if ( var )
	{
		var->SetValue( value );
	}
	*/

	m_variableProperties->Get().RefreshValues();
}

void CEdBehaviorVariableEditor::SetObjectProperty( CObject* obj )
{
	m_variableSelectedObject = obj;
	m_variableProperties->Get().SetObject( m_variableSelectedObject );
}

/*static*/ 
String CEdBehaviorVariableEditor::GetHelperNameForSelectedItem(wxListBox* inListBox)
{
	return inListBox->GetStringSelection().wc_str();
}

void CEdBehaviorVariableEditor::OnShowVectorVariableInPreviewWorker( wxCommandEvent& event, Bool forInternalVariable )
{
	wxListBox* lb = XRCCTRL( *this, forInternalVariable? "internalVectorVariablesList" : "vectorVariablesList", wxListBox );
	wxListBox* ilb = XRCCTRL( *this, "internalVectorVariablesList", wxListBox );

	if (lb->GetSelection() != wxNOT_FOUND)
	{
		String HelperName = GetHelperNameForSelectedItem(lb);
		if (! HelperName.Empty())
		{
			GetEditor()->GetPreviewPanel()->ToggleHelper( HelperName, forInternalVariable );
		}
	}

	ShowVectorInPreview();
}

void CEdBehaviorVariableEditor::OnShowVectorVariableInPreview( wxCommandEvent& event )
{
	OnShowVectorVariableInPreviewWorker( event, false );
}

void CEdBehaviorVariableEditor::OnShowInternalVectorVariableInPreview( wxCommandEvent& event )
{
	OnShowVectorVariableInPreviewWorker( event, true );
}

void CEdBehaviorVariableEditor::UpdateShowVectorInPreviewTools()
{
	wxToolBar* tb = XRCCTRL( *this, "vecVarToolbar", wxToolBar );
	wxToolBar* itb = XRCCTRL( *this, "internalVecVarToolbar", wxToolBar );
	wxListBox* lb = XRCCTRL( *this, "vectorVariablesList", wxListBox );
	wxListBox* ilb = XRCCTRL( *this, "internalVectorVariablesList", wxListBox );

	Bool tbActive = lb->GetSelection() != wxNOT_FOUND && GetEditor()->GetPreviewPanel()->HasHelper( GetHelperNameForSelectedItem(lb), false );
	Bool itbActive = ilb->GetSelection() != wxNOT_FOUND && GetEditor()->GetPreviewPanel()->HasHelper( GetHelperNameForSelectedItem(ilb), true );

	tb->ToggleTool( XRCID("showInPreview"), tbActive );
	itb->ToggleTool( XRCID("internalShowInPreview"), itbActive );
}

void CEdBehaviorVariableEditor::ShowVectorInPreview()
{
	GetEditor()->GetPreviewPanel()->RefreshItems();
	UpdateShowVectorInPreviewTools();
}

void CEdBehaviorVariableEditor::OnGridValueChanged( wxCommandEvent& event )
{
	ShowVectorInPreview();
}

void CEdBehaviorVariableEditor::OnSelectInput( wxCommandEvent &event )
{
	CName deselectFloat = CName::NONE;
	CName deselectEvent = CName::NONE;
	CName deselectInternalFloat = CName::NONE;

	if ( event.GetId() == XRCID( "inputChoiceVar" ) )
	{
		wxChoice* choice = XRCCTRL( *this, "inputChoiceVar", wxChoice );
		Int32 choiceSel = choice->GetSelection();
		if ( choiceSel != wxNOT_FOUND )
		{
			wxListBox* lb = XRCCTRL( *this, "variablesList", wxListBox );
			Int32 varSel = lb->GetSelection();
			if ( varSel != wxNOT_FOUND )
			{
				// Select
				String temp = choice->GetString( choiceSel ).wc_str();
				CName input( temp );
				m_floatInputs.Insert( input );

				// Mark to deselect
				deselectEvent = input;
				deselectInternalFloat = input;
			}
		}
	}
	else if ( event.GetId() == XRCID( "inputChoiceEvent" ) )
	{
		wxChoice* choice = XRCCTRL( *this, "inputChoiceEvent", wxChoice );
		Int32 choiceSel = choice->GetSelection();
		if ( choiceSel != wxNOT_FOUND )
		{
			wxListBox* lb = XRCCTRL( *this, "eventsList", wxListBox );
			Int32 varSel = lb->GetSelection();
			if ( varSel != wxNOT_FOUND )
			{
				// Select
				CBehaviorEventDescription* e = (CBehaviorEventDescription*)( lb->GetClientData(varSel) );
				String temp = choice->GetString( choiceSel ).wc_str();
				CName input( temp );
				m_eventInputs.Insert( input, e->GetEventName() );

				// Mark to deselect
				deselectFloat = input;
				deselectInternalFloat = input;
			}
		}
	}
	else if ( event.GetId() == XRCID( "inputChoiceInternalVar" ) )
	{
		wxChoice* choice = XRCCTRL( *this, "inputChoiceInternalVar", wxChoice );
		Int32 choiceSel = choice->GetSelection();
		if ( choiceSel != wxNOT_FOUND )
		{
			wxListBox* lb = XRCCTRL( *this, "internalVariablesList", wxListBox );
			Int32 varSel = lb->GetSelection();
			if ( varSel != wxNOT_FOUND )
			{
				// Select
				String temp = choice->GetString( choiceSel ).wc_str();
				CName input( temp );
				m_internalFloatInputs.Insert( input );

				// Mark to deselect
				deselectFloat = input;
				deselectEvent = input;
			}
		}
	}

	// Deselect if needed
	if ( ! deselectFloat.Empty() && m_floatInputs.Find( deselectFloat ) != m_floatInputs.End() )
	{
		m_floatInputs.Erase( deselectFloat );
	}
	if ( ! deselectEvent.Empty() && m_eventInputs.Find( deselectEvent ) != m_eventInputs.End() )
	{
		m_eventInputs.Erase( deselectEvent );
	}
	if ( ! deselectInternalFloat.Empty() && m_internalFloatInputs.Find( deselectInternalFloat ) != m_internalFloatInputs.End() )
	{
		m_internalFloatInputs.Erase( deselectInternalFloat );
	}
}

void CEdBehaviorVariableEditor::OnShowInputs( wxCommandEvent& event )
{
	Bool select = event.IsChecked();

	if ( event.GetId() == XRCID( "varInput" ) )
	{
		XRCCTRL( *this, "inputChoiceVar", wxChoice )->Show( select );

		if ( !select )
		{
			wxChoice* choice = XRCCTRL( *this, "inputChoiceVar", wxChoice );
			Int32 choiceSel = choice->GetSelection();
			if ( choiceSel != wxNOT_FOUND )
			{
				String temp = choice->GetString( choiceSel ).wc_str();
				CName input( temp );
				VERIFY( m_floatInputs.Insert( input ) );
			}
		}
	}
	else if ( event.GetId() == XRCID( "eventInput" ) )
	{
		XRCCTRL( *this, "inputChoiceEvent", wxChoice )->Show( select );

		if ( !select )
		{
			wxChoice* choice = XRCCTRL( *this, "inputChoiceEvent", wxChoice );
			Int32 choiceSel = choice->GetSelection();
			if ( choiceSel != wxNOT_FOUND )
			{
				CName input( choice->GetString( choiceSel ).wc_str() );
				m_eventInputs.Set( input, CName::NONE );
			}
		}
	}
	else if ( event.GetId() == XRCID( "internalVarInput" ) )
	{
		XRCCTRL( *this, "inputChoiceInternalVar", wxChoice )->Show( select );

		if ( !select )
		{
			wxChoice* choice = XRCCTRL( *this, "inputChoiceInternalVar", wxChoice );
			Int32 choiceSel = choice->GetSelection();
			if ( choiceSel != wxNOT_FOUND )
			{
				String temp = choice->GetString( choiceSel ).wc_str();
				CName input( temp );
				VERIFY( m_internalFloatInputs.Insert( input ) );
			}
		}
	}
}

void CEdBehaviorVariableEditor::OnMotionChoice( wxCommandEvent& event )
{
	RefreshMotionList();
}

void CEdBehaviorVariableEditor::OnMotionSetHeading( wxCommandEvent& event )
{
	/*wxTextCtrl* hStart = XRCCTRL( *this, "motionHeadingStart", wxTextCtrl );
	Float headingStart;

	FromString( hStart->GetValue().wc_str(), headingStart );

	CEntity* ent = m_behaviorEditor->GetBehaviorGraph()->GetAnimatedComponent()->GetEntity();
	ent->SetRotation( EulerAngles( 0.f, 0.f, headingStart ) );*/
}

void CEdBehaviorVariableEditor::OnMotionStop( wxCommandEvent& event )
{
	/*CBehaviorGraph* graph = m_behaviorEditor->GetBehaviorGraph();
	CAnimatedComponent* ac = graph->GetAnimatedComponent();

	if ( ac->IsKeyframedMoving() )
	{
		ac->CancelKeyframedMotion();
	}*/
}

void CEdBehaviorVariableEditor::OnMotionStart( wxCommandEvent& event )
{
	/*wxChoice* motionChoice = XRCCTRL( *this, "motionChoice", wxChoice );

	 Int32 selection = motionChoice->GetSelection();
	 
	 if ( selection != wxNOT_FOUND )
	 {
		 // Get selected node
		 String selNode = motionChoice->GetString( selection ).wc_str();

		 // Get graph
		 CBehaviorGraph* graph = m_behaviorEditor->GetBehaviorGraph();
		 CAnimatedComponent* ac = graph->GetAnimatedComponent();

		 // Check node
		 if ( !ac->IsKeyframedMoving() && graph->IsMotionNodeAvailable( selNode ) )
		 {
			// Get target
			Vector target;

			wxTextCtrl* vecX = XRCCTRL( *this, "motionTargetX", wxTextCtrl );
			wxTextCtrl* vecY = XRCCTRL( *this, "motionTargetY", wxTextCtrl );
			wxTextCtrl* vecZ = XRCCTRL( *this, "motionTargetZ", wxTextCtrl );

			FromString( vecX->GetValue().wc_str(), target.X );
			FromString( vecY->GetValue().wc_str(), target.Y );
			FromString( vecZ->GetValue().wc_str(), target.Z );

			// Get heading
			wxTextCtrl* hEnd = XRCCTRL( *this, "motionHeadingEnd", wxTextCtrl );
			Float headingEnd;

			FromString( hEnd->GetValue().wc_str(), headingEnd );

			wxChoice* spaceChoice = XRCCTRL( *this, "spaceChoice", wxChoice );
			Int32 spaceSel = spaceChoice->GetSelection();
			if ( spaceSel == 0 )
			{
				// Model space
				CEntity* ent = m_behaviorEditor->GetBehaviorGraph()->GetAnimatedComponent()->GetEntity();
				target = ent->GetLocalToWorld().TransformVectorAsPoint( target );
				headingEnd += ent->GetWorldYaw();
				
				Int32 cycles = (Int32)headingEnd / 360;
				if ( cycles ) headingEnd -= cycles * 360.0f;
			}

			Float speed = 1.f;

			if ( ac->CanUseKeyframedMotion( selNode, target, headingEnd, speed ) )
			{
				if ( ac->SetKeyframedMotion( selNode, target, headingEnd, speed ) )
				{
					LOG_EDITOR( TXT("CEdBehaviorVariableEditor: Set keyframed motion %s"), selNode.AsChar() );
				}
				else
				{
					LOG_EDITOR( TXT("CEdBehaviorVariableEditor: Couldn't set keyframed motion %s"), selNode.AsChar() );
				}
			}
			else
			{
				LOG_EDITOR( TXT("CEdBehaviorVariableEditor: Couldn't use keyframed motion %s"), selNode.AsChar() );
			}
		 }
	 }*/
}

void CEdBehaviorVariableEditor::FillInputChoices()
{
	// Collect inputs
	TDynArray< CName > inputs;
	GetEditor()->EnumInputs( inputs );

	wxChoice* choice1 = XRCCTRL( *this, "inputChoiceVar", wxChoice );
	choice1->Clear();
	wxChoice* choice2 = XRCCTRL( *this, "inputChoiceEvent", wxChoice );
	choice2->Clear();

	for ( Uint32 i=0; i<inputs.Size(); i++ )
	{
		choice1->Append( inputs[i].AsString().AsChar() );
		choice2->Append( inputs[i].AsString().AsChar() );
	}
}

void CEdBehaviorVariableEditor::ResetInputChoices()
{
	wxToolBar* tb1 = XRCCTRL( *this, "varToolbar", wxToolBar );
	tb1->ToggleTool( XRCID("varInput"), false );

	wxToolBar* tb2 = XRCCTRL( *this, "eventToolbar", wxToolBar );
	tb2->ToggleTool( XRCID("eventInput"), false );

	wxToolBar* tb3 = XRCCTRL( *this, "internalVarToolbar", wxToolBar );
	tb3->ToggleTool( XRCID("internalVarInput"), false );

	XRCCTRL( *this, "inputChoiceVar", wxChoice )->Hide();
	XRCCTRL( *this, "inputChoiceEvent", wxChoice )->Hide();
	XRCCTRL( *this, "inputChoiceInternalVar", wxChoice )->Hide();

	// Fill
	FillInputChoices();
}

void CEdBehaviorVariableEditor::UpdateControls()
{
	// Reset input choices
	ResetInputChoices();

	CName varInput;
	if ( GetCurrFloatInput( varInput ) )
	{
		wxToolBar* tb = XRCCTRL( *this, "varToolbar", wxToolBar );
		tb->ToggleTool( XRCID("varInput"), true );

		wxChoice* choice = XRCCTRL( *this, "inputChoiceVar", wxChoice );
		choice->Show( true );
		choice->SetStringSelection( varInput.AsString().AsChar() );
	}

	CName eventInput;
	if ( GetCurrEventInput( eventInput ) )
	{
		wxToolBar* tb = XRCCTRL( *this, "eventToolbar", wxToolBar );
		tb->ToggleTool( XRCID("eventInput"), true );

		wxChoice* choice = XRCCTRL( *this, "inputChoiceEvent", wxChoice );
		choice->Show( true );
		choice->SetStringSelection( eventInput.AsString().AsChar() );
	}

	CName internalVarInput;
	if ( GetCurrInternalFloatInput( internalVarInput ) )
	{
		wxToolBar* tb = XRCCTRL( *this, "internalVarToolbar", wxToolBar );
		tb->ToggleTool( XRCID("internalVarInput"), true );

		wxChoice* choice = XRCCTRL( *this, "inputChoiceInternalVar", wxChoice );
		choice->Show( true );
		choice->SetStringSelection( varInput.AsString().AsChar() );
	}
}

Bool CEdBehaviorVariableEditor::GetCurrFloatInputWorker( CName& input, Bool forInternalVariable )
{
	wxListBox* lb = XRCCTRL( *this, forInternalVariable? "internalVariablesList" : "variablesList", wxListBox );
	Int32 selection = lb->GetSelection();

	if ( selection != wxNOT_FOUND )
	{
		const CName varName = CName( lb->GetStringSelection().wc_str() );
		if ( !GetBehaviorGraph()->GetVariables().GetVariable( varName ) )
		{
			return false;
		}

		// Find variable input
		auto inputs = forInternalVariable ? m_internalFloatInputs : m_floatInputs;
		if ( inputs.Exist( varName ) )
		{
			input = varName;
			return true;
		}
	}

	return false;
}

Bool CEdBehaviorVariableEditor::GetCurrFloatInput( CName& input )
{
	return GetCurrFloatInputWorker( input, false );
}

Bool CEdBehaviorVariableEditor::GetCurrInternalFloatInput( CName& input )
{
	return GetCurrFloatInputWorker( input, true );
}

Bool CEdBehaviorVariableEditor::GetCurrEventInput( CName& input )
{
	wxListBox* lb = XRCCTRL( *this, "eventsList", wxListBox );
	Int32 selection = lb->GetSelection();

	if ( selection != wxNOT_FOUND )
	{
		CBehaviorEventDescription* event = (CBehaviorEventDescription*)( lb->GetClientData(selection) );

		// Find variable input
		for ( THashMap< CName, CName >::iterator it = m_eventInputs.Begin(); it != m_eventInputs.End(); ++it )
		{
			if ( it->m_second == event->GetEventName() )
			{
				if ( it->m_first != CName::NONE )
				{
					input = it->m_first;
					return true;
				}
			}
		}
	}

	return false;
}

void CEdBehaviorVariableEditor::RefreshControlList()
{
	// Refresh preview
}

void CEdBehaviorVariableEditor::RefreshMotionList()
{
	/*wxChoice* motionChoice = XRCCTRL( *this, "motionChoice", wxChoice );

	motionChoice->Freeze();
	motionChoice->Clear();

	CBehaviorGraph* graph = m_behaviorEditor->GetBehaviorGraph();

	TDynArray< String > nodes;
	graph->EnumMotionNodes( nodes );

	for ( Uint32 i=0; i<nodes.Size(); i++ )
	{
		motionChoice->AppendString( nodes[i].AsChar() );
	}

	// End update
	motionChoice->Thaw();
	motionChoice->Refresh();*/
}

void CEdBehaviorVariableEditor::OnPreInstanceReload()
{
	CEdBehaviorEditorSimplePanel::OnPreInstanceReload();
	m_storedFloatValues.Clear();
	m_storedVectorValues.Clear();
	m_storedInternalFloatValues.Clear();
	m_storedInternalVectorValues.Clear();
	if ( CBehaviorGraphInstance* instance = GetBehaviorGraphInstance() )
	{
		// store current values of variables
		CBehaviorGraph* graph = GetBehaviorGraph();
		{
			Uint32 limitTo = graph->GetRuntimeFloatVariablesNum( *instance );
			auto vars = graph->GetVariables().GetVariables();
			for ( auto it = vars.Begin(), end = vars.End(); it != end; ++it )
			{
				if ( it->m_second->GetVarIndex() < limitTo )
				{
					m_storedFloatValues.Set( it->m_first, instance->GetFloatValue( it->m_first ) );
				}
			}
		}
		{
			Uint32 limitTo = graph->GetRuntimeVectorVariablesNum( *instance );
			auto vars = graph->GetVectorVariables().GetVariables();
			for ( auto it = vars.Begin(), end = vars.End(); it != end; ++it )
			{
				if ( it->m_second->GetVarIndex() < limitTo )
				{
					m_storedVectorValues.Set( it->m_first, instance->GetVectorValue( it->m_first ) );
				}
			}
		}
		{
			Uint32 limitTo = graph->GetRuntimeInternalFloatVariablesNum( *instance );
			auto vars = graph->GetInternalVariables().GetVariables();
			for ( auto it = vars.Begin(), end = vars.End(); it != end; ++it )
			{
				if ( it->m_second->GetVarIndex() < limitTo )
				{
					m_storedInternalFloatValues.Set( it->m_first, instance->GetInternalFloatValue( it->m_first ) );
				}
			}
		}
		{
			Uint32 limitTo = graph->GetRuntimeInternalVectorVariablesNum( *instance );
			auto vars = graph->GetInternalVectorVariables().GetVariables();
			for ( auto it = vars.Begin(), end = vars.End(); it != end; ++it )
			{
				if ( it->m_second->GetVarIndex() < limitTo )
				{
					m_storedInternalVectorValues.Set( it->m_first, instance->GetInternalVectorValue( it->m_first ) );
				}
			}
		}
	}
}

void CEdBehaviorVariableEditor::OnInstanceReload()
{
	CEdBehaviorEditorSimplePanel::OnInstanceReload();
	if ( CBehaviorGraphInstance* instance = GetBehaviorGraphInstance() )
	{
		// restore stored values of variables
		for ( auto it = m_storedFloatValues.Begin(); it != m_storedFloatValues.End(); ++it )
		{
			instance->SetFloatValue( it->m_first, it->m_second );
		}
		for ( auto it = m_storedVectorValues.Begin(); it != m_storedVectorValues.End(); ++it )
		{
			instance->SetVectorValue( it->m_first, it->m_second );
		}
		for ( auto it = m_storedInternalFloatValues.Begin(); it != m_storedInternalFloatValues.End(); ++it )
		{
			instance->SetInternalFloatValue( it->m_first, it->m_second );
		}
		for ( auto it = m_storedInternalVectorValues.Begin(); it != m_storedInternalVectorValues.End(); ++it )
		{
			instance->SetInternalVectorValue( it->m_first, it->m_second );
		}
	}
	RefreshAll();
}
