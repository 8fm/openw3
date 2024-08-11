/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "worldEnvironmentEditor.h"
#include "../../common/engine/weatherManager.h"
#include "../../common/engine/mesh.h"
#include "../../common/core/depot.h"
#include "../../common/core/memoryFileWriter.h"
#include "../../common/core/memoryFileReader.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/environmentComponentArea.h"
#include "../../common/engine/environmentManager.h"
#include "../../common/engine/environmentDefinition.h"
#include "../../common/engine/gameTimeManager.h"
#include "../../common/engine/worldIterators.h"

RED_DEFINE_STATIC_NAME( WeatherConditionsArrayChanged );

#define ID_DEFTREEMENU_NEWDEFINITION	1001
#define ID_DEFTREEMENU_NEWFOLDER		1002
#define ID_DEFTREEMENU_RENAME			1003
#define ID_DEFTREEMENU_DELETE			1004
//#define ID_DEFTREEMENU_DERIVE			1005

#define ID_ACTENVMENU_EDIT				1101
#define ID_ACTENVMENU_SELECT			1102
#define ID_ACTENVMENU_FOCUS				1103

#define WORLDENV_INSTANT_ADAPTATION_CHECKBOX_NAME	"instantAdaptationEnable"
#define WORLDENV_GLOBAL_TRAJECTORY_CHECKBOX_NAME	"trajectoryDisplayEnable"
#define WORLDENV_INSTANT_ENVPROBE_UPDATE_CHECKBOX_NAME			"envProbeInstantUpdate"
#define WORLDENV_FAKE_DAY_CYCLE_SLIDER_NAME			"fakeDayCycleProgressSlider"
#define WORLDENV_FAKE_DAY_CYCLE_CHECKBOX_NAME		"fakeDayCycleEnable"
#define WORLDENV_FAKE_DAY_CYCLE_TIME_TEXT_NAME		"fakeDayCycleTime"
#define WORLDENV_allowEnvProbeUpdate_NAME			"allowEnvProbeUpdate"
#define WORLDENV_allowBloom_NAME					"bloomAllow"
#define WORLDENV_allowColorMod_NAME					"allowColorMod"
#define WORLDENV_allowAntialiasing_NAME				"allowAntialiasing"
#define WORLDENV_allowGlobalFog_NAME				"allowGlobalFog"
#define WORLDENV_allowDOF_NAME						"allowDepthOfField"
#define WORLDENV_allowSSAO_NAME						"allowSSAO"
#define WORLDENV_allowCloudsShadow_NAME				"allowCloudsShadow"
#define WORLDENV_allowVignette_NAME					"allowVignette"
#define WORLDENV_forceCutsceneDofMode_NAME			"forceCutsceneDofMode"
#define WORLDENV_allowWaterShader_NAME				"allowWaterShader"
#define WORLDENV_debugTonemapping_NAME				"debugTonemapping"
#define WORLDENV_STABILIZE_AREAENV_BUTTON_NAME		"buttonStabilizeAreaEnv"
#define WORLDENV_OPEN_WEATHER_TEMPLATE_BUTTON_NAME	"m_openWTRes"
#define WORLDENV_ENV_LIST_NAME						"activeEnvs"
#define WORLDENV_EDIT_PROPERTIES_BUTTON_NAME		"openPropertiesEditor"
#define WORLDENV_ADVANCED_TOOLS						"m_showAdvancedTools"

#define WORLDENV_CURRENT_WEATHER_CONDITION			"m_currentWCondition"
#define WORLDENV_CURRENT_WEATHER_STATUS				"m_currentWStatus"

#define WORLDENV_LEFT_SELECT						"m_leftSelect"
#define WORLDENV_RIGHT_SELECT						"m_rightSelect"
#define WORLDENV_LEFT_SELECTION_VALUE				"m_leftSelectValue"
#define WORLDENV_RIGHT_SELECTION_VALUE				"m_rightSelectValue"
#define WORLDENV_AUTO_NEW_KEY						"m_autoNewKey"
#define WORLDENV_COPY_SELECTION						"m_copySelection"
#define WORLDENV_PASTE_SELECTION					"m_pasteSelection"
#define WORLDENV_REMOVE_POINTS						"m_removePoints"
#define WORLDENV_CLEAR_SELECTION					"m_clearSelection"
#define WORLDENV_SELECTED_TAB						"m_selectedTab"
#define WORLDENV_NEW_KEY							"m_newKey"
#define WORLDENV_NEW_KEY_TOLERANCE_TIME				"m_newKeyToleranceTime"

BEGIN_EVENT_TABLE( CEdWorldEnvironmentToolPanel, CEdDraggablePanel )
	EVT_BUTTON(			XRCID(WORLDENV_EDIT_PROPERTIES_BUTTON_NAME),			CEdWorldEnvironmentToolPanel::OnEditWorldEnvironment )
	EVT_BUTTON(			XRCID(WORLDENV_STABILIZE_AREAENV_BUTTON_NAME),			CEdWorldEnvironmentToolPanel::OnStabilizeAreaEnvironments )
	EVT_BUTTON(			XRCID(WORLDENV_OPEN_WEATHER_TEMPLATE_BUTTON_NAME),		CEdWorldEnvironmentToolPanel::OnOpenWeatherTemplate )	
	EVT_CHECKBOX(		XRCID(WORLDENV_INSTANT_ADAPTATION_CHECKBOX_NAME),		CEdWorldEnvironmentToolPanel::OnEdition )
	EVT_CHECKBOX(		XRCID(WORLDENV_GLOBAL_TRAJECTORY_CHECKBOX_NAME),		CEdWorldEnvironmentToolPanel::OnEdition )
	EVT_CHECKBOX(		XRCID(WORLDENV_INSTANT_ENVPROBE_UPDATE_CHECKBOX_NAME),			CEdWorldEnvironmentToolPanel::OnEdition )
	EVT_CHECKBOX(		XRCID(WORLDENV_FAKE_DAY_CYCLE_CHECKBOX_NAME),			CEdWorldEnvironmentToolPanel::OnEdition )
	EVT_SLIDER(			XRCID(WORLDENV_FAKE_DAY_CYCLE_SLIDER_NAME),				CEdWorldEnvironmentToolPanel::OnChangeFakeDayCycleSlider )
	EVT_CHECKBOX(		XRCID(WORLDENV_allowEnvProbeUpdate_NAME),				CEdWorldEnvironmentToolPanel::OnEdition )
	EVT_CHECKBOX(		XRCID(WORLDENV_allowBloom_NAME),						CEdWorldEnvironmentToolPanel::OnEdition )
	EVT_CHECKBOX(		XRCID(WORLDENV_allowColorMod_NAME),						CEdWorldEnvironmentToolPanel::OnEdition )
	EVT_CHECKBOX(		XRCID(WORLDENV_allowAntialiasing_NAME),					CEdWorldEnvironmentToolPanel::OnEdition )
	EVT_CHECKBOX(		XRCID(WORLDENV_allowGlobalFog_NAME),					CEdWorldEnvironmentToolPanel::OnEdition )
	EVT_CHECKBOX(		XRCID(WORLDENV_allowDOF_NAME),							CEdWorldEnvironmentToolPanel::OnEdition )
	EVT_CHECKBOX(		XRCID(WORLDENV_allowSSAO_NAME),							CEdWorldEnvironmentToolPanel::OnEdition )
	EVT_CHECKBOX(		XRCID(WORLDENV_allowWaterShader_NAME),					CEdWorldEnvironmentToolPanel::OnEdition )	
	EVT_CHECKBOX(		XRCID(WORLDENV_allowCloudsShadow_NAME),					CEdWorldEnvironmentToolPanel::OnEdition )
	EVT_CHECKBOX(		XRCID(WORLDENV_allowVignette_NAME),						CEdWorldEnvironmentToolPanel::OnEdition )
	EVT_CHECKBOX(		XRCID(WORLDENV_forceCutsceneDofMode_NAME),				CEdWorldEnvironmentToolPanel::OnEdition )
	EVT_CHECKBOX(		XRCID(WORLDENV_debugTonemapping_NAME),					CEdWorldEnvironmentToolPanel::OnEdition )
	EVT_LISTBOX_DCLICK(	XRCID(WORLDENV_ENV_LIST_NAME),							CEdWorldEnvironmentToolPanel::OnEditWorldEnvironment )
	EVT_TOGGLEBUTTON(	XRCID( WORLDENV_ADVANCED_TOOLS ),							CEdWorldEnvironmentToolPanel::ShowHideAdvancedTools )

	EVT_SLIDER(			XRCID(WORLDENV_LEFT_SELECT),							CEdWorldEnvironmentToolPanel::OnUpdateSelection )
	EVT_SLIDER(			XRCID(WORLDENV_RIGHT_SELECT),							CEdWorldEnvironmentToolPanel::OnUpdateSelection )
	EVT_BUTTON(			XRCID(WORLDENV_COPY_SELECTION),							CEdWorldEnvironmentToolPanel::OnCopySelection )
	EVT_BUTTON(			XRCID(WORLDENV_PASTE_SELECTION),						CEdWorldEnvironmentToolPanel::OnPasteSelection )
	EVT_BUTTON(			XRCID(WORLDENV_REMOVE_POINTS),							CEdWorldEnvironmentToolPanel::OnRemovePoints )
	EVT_BUTTON(			XRCID(WORLDENV_CLEAR_SELECTION),						CEdWorldEnvironmentToolPanel::OnClearSelection )
	EVT_BUTTON(			XRCID(WORLDENV_NEW_KEY),								CEdWorldEnvironmentToolPanel::OnAddNewKey )
END_EVENT_TABLE()

namespace
{
	const Float GAllSeconds = ( 23.0f * 3600.0f ) + ( 59.0f * 60.0f ) + 59.0f;	// because hour 24:00:00 doesn't exist
}

class DefinitionItemData : public wxTreeItemData
{
public:
	union {
		IDepotObject* obj;
		CDirectory* dir;
		CDiskFile* file;
	};

	DefinitionItemData();
	DefinitionItemData( IDepotObject* a_obj ) : obj( a_obj ) {}
};

void GetAllAreaEnvironments( CWorld *world, TDynArray< CAreaEnvironmentComponent* > &outList ) // TODO: kill this function with fire, write an iterator instead
{	
	if ( !world )
	{
		return;
	}

	outList.Clear();
	for ( WorldAttachedComponentsIterator it( world ); it; ++it )
	{
		CComponent *component = *it;
		if ( !component || !component->IsA<CAreaEnvironmentComponent>() )
		{
			continue;
		}

		CAreaEnvironmentComponent *areaEnvComponent = Cast<CAreaEnvironmentComponent>( component );

		outList.PushBack( areaEnvComponent );
	}
}

Int32 FindItemWithLabel( wxCheckListBox *checkListBox, const String &label )
{
	Int32 index = -1;
	if ( checkListBox )
	{
		Uint32 count = checkListBox->GetCount();
		for ( Uint32 i=0; i<count; ++i )
		{
			if ( checkListBox->GetString(i) == label.AsChar() )
			{
				index = (Int32)i;
				break;
			}
		}
	}
	return index;
}

static Bool HasItemWithLabel( wxCheckListBox *checkListBox, const String &label )
{
	return -1 != FindItemWithLabel( checkListBox, label );
}


CEdWorldEnvironmentTool::CEdWorldEnvironmentTool()
	: wxFrame( NULL, wxID_ANY, wxT("Environment Editor"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE )
	, m_curveDisplayTimeChanged( false )
	, m_globalEnvListUpdateNeeded( false )
{
	SEvents::GetInstance().RegisterListener( CNAME( EditorTick ), this );
	SEvents::GetInstance().RegisterListener( CNAME( GlobalAreaEnvironmentsListUpdate ), this );
	SEvents::GetInstance().RegisterListener( CNAME( WeatherConditionsArrayChanged ), this );

	SEvents::GetInstance().RegisterListener( CNAME( SimpleCurveChanged ), this );

	Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CEdWorldEnvironmentTool::OnClose ), 0, this );
}

CEdWorldEnvironmentTool::~CEdWorldEnvironmentTool()
{
	SEvents::GetInstance().UnregisterListener( this );
}

void CEdWorldEnvironmentTool::NotifyCurveDisplayTimeChanged()
{
	m_curveDisplayTimeChanged = true;
}

void CEdWorldEnvironmentTool::OnClose( wxCloseEvent& event )
{
	ISavableToConfig::SaveSession();
	event.Veto();
	Hide();
}

void CEdWorldEnvironmentTool::SaveSession( CConfigurationManager &config )
{
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/WorldEnvironmentTool") );

	config.Write( TXT("X"), GetPosition().x );
	config.Write( TXT("Y"), GetPosition().y );
	config.Write( TXT("Width"), GetSize().GetWidth() );
	config.Write( TXT("Height"), GetSize().GetHeight() );
	config.Write( TXT("StayOnTop"), m_panel && m_panel->GetStayOnTop() ? 1 : 0 );
	if ( m_panel )
	{
		m_panel->SaveSession( config );
	}
}

void CEdWorldEnvironmentTool::RestoreSession( CConfigurationManager &config )
{
	wxPoint currentPosition = GetPosition(), storedPosition;
	wxSize currentSize = GetBestFittingSize(), storedSize;

	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/WorldEnvironmentTool") );

	storedPosition.x = config.Read( TXT("X"), -1 );
	storedPosition.y = config.Read( TXT("Y"), -1 );
	storedSize.SetWidth( config.Read( TXT("Width"), -1 ) );
	storedSize.SetHeight( config.Read( TXT("Height"), -1 ) );
	if ( m_panel )
	{
		m_panel->SetStayOnTop( config.Read( TXT("StayOnTop"), 0 ) != 0 );
		m_panel->RestoreSession( config );
	}

	storedPosition.x = storedPosition.x == -1 ? currentPosition.x : storedPosition.x;
	storedPosition.y = storedPosition.y == -1 ? currentPosition.y : storedPosition.y;
	storedSize.SetWidth( storedSize.GetWidth() == -1 ? currentSize.GetWidth() : storedSize.GetWidth() );
	storedSize.SetHeight( storedSize.GetHeight() == -1 ? currentSize.GetHeight() : storedSize.GetHeight() );

	SetPosition( storedPosition );
	SetSize( storedSize );
}

Bool CEdWorldEnvironmentTool::Show( CEdRenderingPanel* viewport )
{
#if 0
	// In game, do not open this tool
	if ( GGame->IsActive() )
	{
		wxMessageBox( wxT( "Unable to open WorldEnvironment tool while in game!"), wxT("Error"), wxOK | wxICON_ERROR );
		return false;
	}
#endif

	// Create tool panel
	if ( m_panel )
	{
		m_panel->Destroy();
	}

	m_panel = new CEdWorldEnvironmentToolPanel( this, this );
	m_panel->UpdateDefinitionsTree();

	wxBoxSizer* boxSizer = new wxBoxSizer( wxVERTICAL );
	SetSizer( boxSizer );

	boxSizer->Add( m_panel, 1, wxEXPAND, 5 );

	m_panel->Fit();
	LayoutRecursively( this );
	Fit();

	if ( !IsVisible() )
	{
		wxFrame::Show();
	}
	else
	{
		SetFocus();
	}

	if ( GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetEnvironmentParameters().m_environmentDefinition )
	{
		ShowAndEdit( GGame->GetActiveWorld()->GetEnvironmentParameters().m_environmentDefinition.Get() );
	}

	ISavableToConfig::RestoreSession();

	return true;
}

void CEdWorldEnvironmentTool::ShowAndEdit( CObject* data )
{
	ASSERT(m_panel);
	m_panel->ShowAndEdit( data );
	RunLaterOnce( [ this ](){ SetFocus(); } );
}

RED_DEFINE_STATIC_NAME( lightColor )

void CEdWorldEnvironmentTool::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	// Ignore events when we're invisible since the data is most likely invalid
	if ( !IsShown() )
	{
		return;
	}

	if ( name == CNAME( EditorTick ) )
	{
		if ( m_curveDisplayTimeChanged )
		{
			SEvents::GetInstance().DispatchEvent( CNAME( SimpleCurveDisplayedTimeChanged ), NULL );
			m_curveDisplayTimeChanged = false;
		}

		if ( m_globalEnvListUpdateNeeded )
		{
			if ( m_panel )
			{
				m_panel->UpdateGlobalEnvironmentsPanel();
			}
			m_globalEnvListUpdateNeeded = false;
		}
	}
	else if ( name == CNAME(GlobalAreaEnvironmentsListUpdate) )
	{
		m_globalEnvListUpdateNeeded = true;
	}
	else if ( name == CNAME( SimpleCurveChanged ) )
	{	
		const SSimpleCurveChangeData& curveData = GetEventData< SSimpleCurveChangeData >( data );
		if ( curveData.m_propertyName == CNAME( lightColor ) )
		{
			if( CEnvironmentManager* envMgr = m_panel->GetEnvironmentManager() )
			{
				EDITOR_DISPATCH_EVENT( CNAME(SceneEditorEnvironmentsUpdate), CreateEventData( envMgr->GetWorld() ) );

			}
		}					
		if( m_panel->m_areaEnvComponent )
		{
			m_panel->m_areaEnvComponent->NotifyPropertiesImplicitChange();
		}		

		// Update if editing weather environment
		m_panel->UpdateWeatherEnvironments();
	}
	else if ( name == CNAME( WeatherConditionsArrayChanged ) )
	{
		m_panel->UpdateWeatherConditions();
	}
}

CEdWorldEnvironmentToolPanel::CEdWorldEnvironmentToolPanel( CEdWorldEnvironmentTool* tool, wxWindow* parent )
	: m_tool( tool )
	, m_areaEnvComponent( NULL )
	, m_lastData( NULL )
	, m_leftSelectPicker( nullptr )
	, m_rightSelectPicker( nullptr )
{
	// Load layout from XRC
	wxXmlResource::Get()->LoadPanel( this, parent, wxT("WorldEnvironmentTool") );

	// Create a wxBitmapToggleButton from the standin (wxFormBuilder v3.1
	// does not support wxBitmapToggleButton controls so a standin is used
	// instead - replace this code with a simple
	//
	//   m_stayOnTop = XRCCTRL( *this, "m_stayOnTop", wxBitmapToggleButton );
	//
	// once wxFormBuilder adds wxBitmapToggleButton support.     -badsector
	{
		wxBitmapButton* standin = XRCCTRL( *this, "m_stayOnTop", wxBitmapButton );
		wxBoxSizer* standinSizer = static_cast<wxBoxSizer*>( standin->GetContainingSizer() );
		m_stayOnTop = new wxBitmapToggleButton( this, wxID_ANY, standin->GetBitmap() );
		m_stayOnTop->SetToolTip( standin->GetToolTipText() );
		standinSizer->Insert( 0, m_stayOnTop, 0, wxLEFT, 5 );
		standin->Destroy();
	}

	// Active environments
	m_activeEnvs = XRCCTRL( *this, WORLDENV_ENV_LIST_NAME, wxListBox );
	m_activeEnvs->Connect( wxEVT_CONTEXT_MENU, wxContextMenuEventHandler( CEdWorldEnvironmentToolPanel::OnActiveEnvironmentsContextMenu ), NULL, this );

	// Definitions tree
	m_definitionsTree = XRCCTRL( *this, "m_definitionsTree", wxTreeCtrl );
	m_defTreeImageList = new wxImageList( 16, 16, true,  0 );
	m_defTreeImageList->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_FOLDER_SMALL") ) );
	m_defTreeImageList->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_BRICK") ) );
	m_definitionsTree->SetImageList( m_defTreeImageList );
	Bind( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, &CEdWorldEnvironmentToolPanel::OnDefinitionsTreeItemActivated, this );
	Bind( wxEVT_COMMAND_TREE_ITEM_MENU, &CEdWorldEnvironmentToolPanel::OnDefinitionsTreeItemMenu, this );

	// Set event handler
	m_stayOnTop->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( CEdWorldEnvironmentToolPanel::OnStayOnTop ), NULL, this );

	// Save button
	m_save = XRCCTRL( *this, "m_save", wxButton );
	m_save->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdWorldEnvironmentToolPanel::OnSaveClicked ), NULL, this );

	wxChoice* defEnv = XRCCTRL( *this, "m_envStdEnvChoice", wxChoice );	
	defEnv->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdWorldEnvironmentToolPanel::OnStdEnvChoiceSelected ), NULL, this );	
		
	wxChoice* weatherEnv = XRCCTRL( *this, "envWeatherChoice", wxChoice );
	weatherEnv->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdWorldEnvironmentToolPanel::OnWStdWeatherConditionsChoiceSelected ), NULL, this );	

	m_pause = XRCCTRL( *this, "m_pause", wxCheckBox );
	if ( GGame->GetActiveWorld() )
	{
		m_pause->SetValue( !GGame->GetActiveWorld()->GetEnvironmentManager()->GetWeatherManager()->IsRunning() );
	}
	else
	{
		m_pause->SetValue( false );
	}
	m_pause->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdWorldEnvironmentToolPanel::OnPauseChecked ), NULL, this );

	// add time pickers for copy/paste tools
	wxPanel* fakeDayCycleTimePanel = XRCCTRL( *this, "fakeDayCycleTimePanel", wxPanel );
	if( fakeDayCycleTimePanel != nullptr )
	{
		wxSizer* sizer = fakeDayCycleTimePanel->GetSizer();
		if( sizer != nullptr )
		{
			m_fakeDayCycleTimePicker = new wxTimePickerCtrl( fakeDayCycleTimePanel, wxID_ANY );
			sizer->Add( m_fakeDayCycleTimePicker, 0, wxALL, 0 );
			m_fakeDayCycleTimePicker->SetTime( 12, 0, 0 );
			m_fakeDayCycleTimePicker->Connect( wxEVT_TIME_CHANGED, wxDateEventHandler( CEdWorldEnvironmentToolPanel::OnChangeFakeDayCycleTimePicker ), nullptr, this );
		}
	}
	wxPanel* leftSelectValuePanel = XRCCTRL( *this, "m_leftSelectValuePanel", wxPanel );
	if( leftSelectValuePanel != nullptr )
	{
		wxSizer* sizer = leftSelectValuePanel->GetSizer();
		if( sizer != nullptr )
		{
			m_leftSelectPicker = new wxTimePickerCtrl( leftSelectValuePanel, wxID_ANY );
			sizer->Add( m_leftSelectPicker, 0, wxALL, 0 );
			m_leftSelectPicker->SetTime( 0, 0, 0 );
			m_leftSelectPicker->Connect( wxEVT_TIME_CHANGED, wxDateEventHandler( CEdWorldEnvironmentToolPanel::OnChangeLeftTimePicker ), nullptr, this );
		}
	}
	wxPanel* rightSelectValuePanel = XRCCTRL( *this, "m_rightSelectValuePanel", wxPanel );
	if( rightSelectValuePanel != nullptr )
	{
		wxSizer* sizer = rightSelectValuePanel->GetSizer();
		if( sizer != nullptr )
		{
			m_rightSelectPicker = new wxTimePickerCtrl( rightSelectValuePanel, wxID_ANY );
			sizer->Add( m_rightSelectPicker, 0, wxALL, 0 );
			m_rightSelectPicker->SetTime( 0, 0, 0 );
			m_rightSelectPicker->Connect( wxEVT_TIME_CHANGED, wxDateEventHandler( CEdWorldEnvironmentToolPanel::OnChangeRightTimePicker ), nullptr, this );
		}
	}
	
	CEnvironmentManager* envMgr = GetEnvironmentManager();
	if( envMgr )
	{
		const TDynArray<String>& envStr = envMgr->GetDefaultEnvironmentsNames();
		for( Uint32 i=0; i<envStr.Size(); i++) defEnv->Append( envStr[i].AsChar() );
		UpdateWeatherConditions();
	}

	// Menu stuff
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdWorldEnvironmentToolPanel::OnDefMenuNewDefinition, this, ID_DEFTREEMENU_NEWDEFINITION );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdWorldEnvironmentToolPanel::OnDefMenuNewFolder, this, ID_DEFTREEMENU_NEWFOLDER );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdWorldEnvironmentToolPanel::OnDefMenuRename, this, ID_DEFTREEMENU_RENAME );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdWorldEnvironmentToolPanel::OnDefMenuDelete, this, ID_DEFTREEMENU_DELETE );
	
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdWorldEnvironmentToolPanel::OnActEnvMenuEdit, this, ID_ACTENVMENU_EDIT );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdWorldEnvironmentToolPanel::OnActEnvMenuSelect, this, ID_ACTENVMENU_SELECT );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdWorldEnvironmentToolPanel::OnActEnvMenuSelect, this, ID_ACTENVMENU_FOCUS );

	//wxCheckBox	*displayWaterShader			= XRCCTRL( *this, WORLDENV_allowWaterShader_NAME,				wxCheckBox	);
	//displayWaterShader->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdWorldEnvironmentToolPanel::OnSetWaterVisibility ), NULL, this );

	m_currentWeatheConditionTextCtrl	= XRCCTRL( *this, WORLDENV_CURRENT_WEATHER_CONDITION, wxTextCtrl );
	m_currentWeatherConditionStatus		= XRCCTRL( *this, WORLDENV_CURRENT_WEATHER_STATUS, wxGauge );

	// Synchronize data
	SynchronizeData( true );
	UpdateGlobalEnvironmentsPanel();
}

CEdWorldEnvironmentToolPanel::~CEdWorldEnvironmentToolPanel()
{
	delete m_defTreeImageList;
}

void CEdWorldEnvironmentToolPanel::SynchronizeData( bool localToPanel )
{
	// --------------------------------
	struct Local
	{
		static void Synchronize( Bool &attribute, wxCheckBox &checkbox, bool localToPanel )
		{
			if ( localToPanel )
			{
				checkbox.SetValue( attribute );
			}
			else
			{
				attribute = checkbox.GetValue();
			}
		}
	};
#define WORLDENV_SYNCHRONIZE( attribute, widget )	ASSERT(widget); Local::Synchronize( attribute, *widget, localToPanel );
	// --------------------------------

	CEnvironmentManager *envManager = GetEnvironmentManager();
	if ( !envManager )
	{
		return;
	}

	// Get panel widgets

	wxCheckBox	*fakeDayCycleEnabled		= XRCCTRL( *this, WORLDENV_FAKE_DAY_CYCLE_CHECKBOX_NAME,		wxCheckBox	);
	wxSlider	*fakeDayCycleProgress		= XRCCTRL( *this, WORLDENV_FAKE_DAY_CYCLE_SLIDER_NAME,			wxSlider	);	
	wxCheckBox	*instantAdaptationEnabled	= XRCCTRL( *this, WORLDENV_INSTANT_ADAPTATION_CHECKBOX_NAME,	wxCheckBox	);
	wxCheckBox	*trajectoryDisplayEnabled	= XRCCTRL( *this, WORLDENV_GLOBAL_TRAJECTORY_CHECKBOX_NAME,		wxCheckBox	);
	wxCheckBox	*instantEnvProbeUpdate		= XRCCTRL( *this, WORLDENV_INSTANT_ENVPROBE_UPDATE_CHECKBOX_NAME,			wxCheckBox	);
	wxCheckBox	*allowEnvProbeUpdate		= XRCCTRL( *this, WORLDENV_allowEnvProbeUpdate_NAME,			wxCheckBox	);
	wxCheckBox	*allowBloom					= XRCCTRL( *this, WORLDENV_allowBloom_NAME,						wxCheckBox	);
	wxCheckBox	*allowColorMod				= XRCCTRL( *this, WORLDENV_allowColorMod_NAME,					wxCheckBox	);
	wxCheckBox	*allowAntialiasing			= XRCCTRL( *this, WORLDENV_allowAntialiasing_NAME,				wxCheckBox	);
	wxCheckBox	*allowGlobalFog				= XRCCTRL( *this, WORLDENV_allowGlobalFog_NAME,					wxCheckBox	);
	wxCheckBox	*allowDOF					= XRCCTRL( *this, WORLDENV_allowDOF_NAME,						wxCheckBox	);
	wxCheckBox	*allowSSAO					= XRCCTRL( *this, WORLDENV_allowSSAO_NAME,						wxCheckBox	);
	wxCheckBox	*allowCloudsShadow			= XRCCTRL( *this, WORLDENV_allowCloudsShadow_NAME,				wxCheckBox	);
	wxCheckBox	*allowVignette				= XRCCTRL( *this, WORLDENV_allowVignette_NAME,					wxCheckBox	);
	wxCheckBox	*forceCutsceneDofMode		= XRCCTRL( *this, WORLDENV_forceCutsceneDofMode_NAME,			wxCheckBox	);
	wxCheckBox	*allowWaterShader			= XRCCTRL( *this, WORLDENV_allowWaterShader_NAME,				wxCheckBox	);

	// Get current game parameters

	CGameEnvironmentParams	gameParams	= envManager->GetGameEnvironmentParams();

	// Synchronize

	WORLDENV_SYNCHRONIZE( gameParams.m_dayCycleOverride.m_fakeDayCycleEnable,				fakeDayCycleEnabled			);
	WORLDENV_SYNCHRONIZE( gameParams.m_displaySettings.m_enableInstantAdaptation,			instantAdaptationEnabled	);
	WORLDENV_SYNCHRONIZE( gameParams.m_displaySettings.m_enableGlobalLightingTrajectory,	trajectoryDisplayEnabled	);
	WORLDENV_SYNCHRONIZE( gameParams.m_displaySettings.m_enableEnvProbeInstantUpdate,		instantEnvProbeUpdate			);
	WORLDENV_SYNCHRONIZE( gameParams.m_displaySettings.m_allowEnvProbeUpdate,				allowEnvProbeUpdate	);
	WORLDENV_SYNCHRONIZE( gameParams.m_displaySettings.m_allowBloom,						allowBloom			);
	WORLDENV_SYNCHRONIZE( gameParams.m_displaySettings.m_allowColorMod,						allowColorMod		);
	WORLDENV_SYNCHRONIZE( gameParams.m_displaySettings.m_allowAntialiasing,					allowAntialiasing	);
	WORLDENV_SYNCHRONIZE( gameParams.m_displaySettings.m_allowGlobalFog,					allowGlobalFog		);
	WORLDENV_SYNCHRONIZE( gameParams.m_displaySettings.m_allowDOF,							allowDOF			);
	WORLDENV_SYNCHRONIZE( gameParams.m_displaySettings.m_allowSSAO,							allowSSAO			);
	WORLDENV_SYNCHRONIZE( gameParams.m_displaySettings.m_allowCloudsShadow,					allowCloudsShadow	);
	WORLDENV_SYNCHRONIZE( gameParams.m_displaySettings.m_allowVignette,						allowVignette		);
	WORLDENV_SYNCHRONIZE( gameParams.m_displaySettings.m_forceCutsceneDofMode,				forceCutsceneDofMode);
	WORLDENV_SYNCHRONIZE( gameParams.m_displaySettings.m_allowWaterShader,					allowWaterShader);


	if ( localToPanel )
	{
		fakeDayCycleProgress->SetValue( (Int32)Lerp( Clamp(gameParams.m_dayCycleOverride.m_fakeDayCycleHour/24.f, 0.f, 1.f), (Float)fakeDayCycleProgress->GetMin(), (Float)fakeDayCycleProgress->GetMax()) );
	}
	else
	{
		gameParams.m_dayCycleOverride.m_fakeDayCycleHour = Lerp( (fakeDayCycleProgress->GetValue() - fakeDayCycleProgress->GetMin()) / (Float)Max(1, fakeDayCycleProgress->GetMax() - fakeDayCycleProgress->GetMin()), 0.f, 24.f );
		if( fakeDayCycleEnabled->GetValue() )
		{
			Int32 seconds = (Int32)fakeDayCycleProgress->GetValue();
			Int32 hours = seconds / ( 60 * 60 );
			seconds -= ( hours * ( 60 * 60 ) );
			Int32 minutes = seconds / 60;
			seconds -= ( minutes * 60 );
			if( GGame ){ GGame->GetTimeManager()->SetTime( GameTime( 0, hours, minutes, seconds ), false ); }
		}
	}

	// Update fake day cycle progress value

	{
		Int32 seconds = (Int32)fakeDayCycleProgress->GetValue();
		Int32 hours = seconds / ( 60 * 60 );
		seconds -= ( hours * ( 60 * 60 ) );
		Int32 minutes = seconds / 60;
		seconds -= ( minutes * 60 );
		m_fakeDayCycleTimePicker->SetTime( hours, minutes, seconds );
	}

	// Update simple curve graphs displayed time

	Bool fakeDayCycleDisplayChanged = false;
	if ( gameParams.m_dayCycleOverride.m_fakeDayCycleEnable )
	{
		Float newDisplayValue = Clamp( gameParams.m_dayCycleOverride.m_fakeDayCycleHour / 24.f, 0.f, 1.f );
		fakeDayCycleDisplayChanged = !SSimpleCurve::s_graphTimeDisplayEnable || newDisplayValue!=SSimpleCurve::s_graphTimeDisplayValue;
		SSimpleCurve::s_graphTimeDisplayEnable = true;
		SSimpleCurve::s_graphTimeDisplayValue  = newDisplayValue;
	}
	else
	{		
		fakeDayCycleDisplayChanged = SSimpleCurve::s_graphTimeDisplayEnable;
		SSimpleCurve::s_graphTimeDisplayEnable = false;
	}

	if ( fakeDayCycleDisplayChanged )
	{
		m_tool->NotifyCurveDisplayTimeChanged();
	}

	// 

	if ( !localToPanel )
	{
		envManager->SetGameEnvironmentParams( gameParams );
	}

	// Update world status
	CWorld* world = GGame->GetActiveWorld();

	if( world )
	{	
		if( world->IsWaterShaderEnabled() != gameParams.m_displaySettings.m_allowWaterShader )
		{	
			world->SetWaterVisible( gameParams.m_displaySettings.m_allowWaterShader );			
		}		
	}
}

void CEdWorldEnvironmentToolPanel::OnStdEnvChoiceSelected( wxCommandEvent &event )
{	
	UpdateEnv();
}

void CEdWorldEnvironmentToolPanel::OnWStdWeatherConditionsChoiceSelected( wxCommandEvent &event )
{
	UpdateWeatherPresetsChoiceBox();
}

void CEdWorldEnvironmentToolPanel::OnEdition( wxCommandEvent &event )
{
	((ISavableToConfig*)m_tool)->SaveSession();
	SynchronizeData( false );
}

void CEdWorldEnvironmentToolPanel::SaveSession( CConfigurationManager &config )
{
	wxChoice*			defEnv					= XRCCTRL( *this, "m_envStdEnvChoice", wxChoice );
	wxChoice*			weather					= XRCCTRL( *this, "envWeatherChoice", wxChoice );
	wxToggleButton*		advancedOptions			= XRCCTRL( *this, WORLDENV_ADVANCED_TOOLS, wxToggleButton );
	wxCheckBox*			fakeDayCycleEnabled		= XRCCTRL( *this, WORLDENV_FAKE_DAY_CYCLE_CHECKBOX_NAME, wxCheckBox );
	wxSlider*			fakeDayCycleProgress	= XRCCTRL( *this, WORLDENV_FAKE_DAY_CYCLE_SLIDER_NAME, wxSlider );
	wxNotebook*			leftNotebook			= XRCCTRL( *this, "m_notebook1", wxNotebook );
	wxSplitterWindow*	splitter1				= XRCCTRL( *this, "m_splitter1", wxSplitterWindow );

	config.Write( TXT("stayOnTop"), m_stayOnTop->GetValue() ? 1 : 0 );
	config.Write( TXT("defEnv"), defEnv->GetSelection() );
	config.Write( TXT("weather"), weather->GetSelection() );
	config.Write( TXT("pause"), m_pause->GetValue() ? 1 : 0 );
	config.Write( TXT("advancedOptions"), advancedOptions->GetValue() ? 1 : 0 );
	config.Write( TXT("fakeDayCycleEnabled"), fakeDayCycleEnabled->GetValue() ? 1 : 0 );
	config.Write( TXT("fakeDayCycleProgress"), fakeDayCycleProgress->GetValue() );
	config.Write( TXT("leftNotebook"), leftNotebook->GetSelection() );
	config.Write( TXT("splitter1"), splitter1->GetSashPosition() );
}

void CEdWorldEnvironmentToolPanel::RestoreSession( CConfigurationManager &config )
{
	wxChoice*			defEnv					= XRCCTRL( *this, "m_envStdEnvChoice", wxChoice );
	wxChoice*			weather					= XRCCTRL( *this, "envWeatherChoice", wxChoice );
	wxToggleButton*		advancedOptions			= XRCCTRL( *this, WORLDENV_ADVANCED_TOOLS, wxToggleButton );
	wxCheckBox*			fakeDayCycleEnabled		= XRCCTRL( *this, WORLDENV_FAKE_DAY_CYCLE_CHECKBOX_NAME, wxCheckBox );
	wxSlider*			fakeDayCycleProgress	= XRCCTRL( *this, WORLDENV_FAKE_DAY_CYCLE_SLIDER_NAME, wxSlider );
	wxNotebook*			leftNotebook			= XRCCTRL( *this, "m_notebook1", wxNotebook );
	wxSplitterWindow*	splitter1				= XRCCTRL( *this, "m_splitter1", wxSplitterWindow );

	m_stayOnTop->SetValue( config.Read( TXT("stayOnTop"), fakeDayCycleEnabled->GetValue() ? 1 : 0 ) != 0 );
	defEnv->SetSelection( config.Read( TXT("defEnv"), 0 ) );
	weather->SetSelection( config.Read( TXT("weather"), 0 ) );
	m_pause->SetValue( config.Read( TXT("pause"), 1 ) != 0 );
	advancedOptions->SetValue( config.Read( TXT("advancedOptions"), 0 ) != 0 );
	fakeDayCycleEnabled->SetValue( config.Read( TXT("fakeDayCycleEnabled"), fakeDayCycleEnabled->GetValue() ? 1 : 0 ) != 0 );
	fakeDayCycleProgress->SetValue( config.Read( TXT("fakeDayCycleProgress"), fakeDayCycleProgress->GetValue() ) );
	leftNotebook->SetSelection( config.Read( TXT("leftNotebook"), 0 ) );
	splitter1->SetSashPosition( config.Read( TXT("splitter1"), splitter1->GetSashPosition() ) );
 
	RefreshAdvancedTools();
	SynchronizeData( false );
// 	UpdateEnv();
	UpdateWeatherPresetsChoiceBox();
	UpdateWorldStatus();
}

CEnvironmentManager* CEdWorldEnvironmentToolPanel::GetEnvironmentManager() const
{
	CWorld* world = GGame->GetActiveWorld();
	ASSERT( world, TXT("No world loaded. In a perfect universe this won't crash.") );
	return world ? world->GetEnvironmentManager() : NULL;
}

void CEdWorldEnvironmentToolPanel::OnOpenWeatherTemplate( wxCommandEvent &event )
{	
	CWorld* world = GetEnvironmentManager() ? GetEnvironmentManager()->GetWorld() : nullptr;
	if( world != nullptr )
	{
		C2dArray* wt = world->GetEnvironmentParameters().m_weatherTemplate.Get();
		if( wt != nullptr )
		{
			wxTheFrame->GetAssetBrowser()->EditAsset( wt );
		}
		else
			GFeedback->ShowMsg( TXT("Error loading weather template"), TXT("Template is not set on this world!") );
	}
	else
		GFeedback->ShowMsg( TXT("Error loading weather template"), TXT("No world loaded!") );
}

void CEdWorldEnvironmentToolPanel::OnStabilizeAreaEnvironments( wxCommandEvent &event )
{
	CEnvironmentManager *envManager = GetEnvironmentManager();
	if ( envManager )
	{
		envManager->SetInstantAdaptationTrigger( true );
		envManager->GetWeatherManager()->StabilizeCurrentWeather();
	}
}

void CEdWorldEnvironmentToolPanel::ShowAndEdit( CObject* data )
{
	if ( !data )
	{
		return;
	}

	wxScrolledWindow* dataPanel = XRCCTRL(*m_tool, "envDataPanel", wxScrolledWindow);
	ASSERT(dataPanel);

	// this is to allow listener to register SCurve modification inside area env properites
	CAreaEnvironmentComponent* areaEnvCmp = Cast<CAreaEnvironmentComponent>(data);
	if(areaEnvCmp) 
	{
		m_areaEnvComponent = areaEnvCmp;
		data = areaEnvCmp->GetEnvironmentDefinition();
		if ( !data )
		{
			return;
		}
	}

	dataPanel->DestroyChildren();
	dataPanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );

	m_browser	= new CEdPropertiesPage( dataPanel, PropertiesPageSettings(), nullptr );		
	m_lastData = data;
	if ( data->IsA< CResource >() )
	{
		m_save->Enable( true );
		m_tool->SetTitle( wxString::Format( wxT("%s - Environment Editor"), static_cast<CResource*>( data )->GetDepotPath().AsChar() ) );
	}
	else
	{
		m_save->Enable( false );
		m_tool->SetTitle( wxT("Environment Editor") );
	}
	m_browser->Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdWorldEnvironmentToolPanel::OnPropertiesModified ), NULL, this );
	m_browser->SetObject( data );

	dataPanel->GetSizer()->Add( m_browser, 1, wxEXPAND, 5 );

	FillCopyAreasControl();
}

void CEdWorldEnvironmentToolPanel::DeleteSingleFile( CDiskFile* file )
{
	// Remove reference from current area environment component (if any)
	if ( m_areaEnvComponent && m_areaEnvComponent->GetEnvironmentDefinition() == file->GetResource() )
	{
		m_areaEnvComponent->SetEnvironmentDefinition( NULL );
	}

	// Delete the file
	if ( file->IsLocal() )
	{
		file->Delete( false );
	}
	else if ( file->IsCheckedOut() )
	{
		if ( file->Revert() && file->Delete( false, false ) )
		{
			file->Submit();
		}
	}
	else
	{
		file->Delete( false, false );
	}
}

Bool CEdWorldEnvironmentToolPanel::GetActiveEnvironmentsSelection( Uint32& selection ) const
{
	// Get environment manager
	CEnvironmentManager* envMgr = GetEnvironmentManager();
	if ( !envMgr )
	{
		return false;
	}

	// Make sure the selection is in active environments bounds
	const CEnvironmentManager::TAreaEnvironmentsArray& envs = envMgr->GetActiveEnvironments();
	int intSelection = m_activeEnvs->GetSelection();
	if ( intSelection < 0 || intSelection >= (int)envs.SizeInt() )
	{
		return false;
	}

	// Return selection
	selection = (Uint32)intSelection;
	return true;
}

void CEdWorldEnvironmentToolPanel::OnEditWorldEnvironment( wxCommandEvent &event )
{
	// edit particular area env
	if( event.GetId() == XRCID(WORLDENV_ENV_LIST_NAME) )
	{
		Uint32 num = event.GetInt();
		CEnvironmentManager *envManager = GetEnvironmentManager();
		if ( !envManager )
		{
			return;
		}
		const CEnvironmentManager::TAreaEnvironmentsArray& envs = envManager->GetActiveEnvironments();
		if ( num >= envs.Size() )
		{
			return;
		}
		const SEnvManagerAreaEnvData& env = envs[num];
		if ( env.areaComponent )
		{
			ShowAndEdit( env.areaComponent->GetEnvironmentDefinition() );
		}
		else if ( GGame->GetActiveWorld() ) // hacky definition derivation from parameters
		{
			if ( env.priority == 0 && GGame->GetActiveWorld()->GetEnvironmentParameters().m_environmentDefinition ) // assume global environment
			{
				ShowAndEdit( GGame->GetActiveWorld()->GetEnvironmentParameters().m_environmentDefinition.Get() );
			}
			else if ( CEnvironmentManager::ScenesEnvironmentPriority == env.priority && GGame->GetActiveWorld()->GetEnvironmentParameters().m_scenesEnvironmentDefinition ) // assume scenes environment
			{
				ShowAndEdit( GGame->GetActiveWorld()->GetEnvironmentParameters().m_scenesEnvironmentDefinition.Get() );
			}
			// weather system environment, edit the env resource
			else
			{
				CWeatherManager* wtMgr = envManager->GetWeatherManager();
				if( wtMgr != nullptr ) ShowAndEdit( wtMgr->GetCurrentWeatherCondition().m_environmentDefinition.Get() );
			}
		}
	}
}

void CEdWorldEnvironmentToolPanel::OnStayOnTop( wxCommandEvent& event )
{
	SetStayOnTop( m_stayOnTop->GetValue() );
}

void CEdWorldEnvironmentToolPanel::OnSaveClicked( wxCommandEvent& event )
{
	CEnvironmentDefinition* envdef = Cast< CEnvironmentDefinition >( m_lastData );
	if ( !envdef )
	{
		return;
	}
	if ( !envdef->Save() )
	{
		wxMessageBox( wxString::Format( wxT("Failed to save the environment definition file to '%s'"), envdef->GetDepotPath().AsChar() ), wxT("Error"), wxICON_ERROR|wxOK, this );
	}
}

void CEdWorldEnvironmentToolPanel::OnActiveEnvironmentsContextMenu( wxContextMenuEvent& event )
{
	wxMenu menu;
	Uint32 selection;

	// Get the selection (if proper)
	if ( !GetActiveEnvironmentsSelection( selection ) )
	{
		return;
	}

	// Get active environments array
	CEnvironmentManager* envMgr = GetEnvironmentManager();
	const CEnvironmentManager::TAreaEnvironmentsArray& envs = envMgr->GetActiveEnvironments();

	// Create popup menu
	menu.Append( ID_ACTENVMENU_EDIT, wxT("Edit") );
	if ( envs[selection].areaComponent )
	{
		menu.Append( ID_ACTENVMENU_SELECT, wxT("Select") );
		menu.Append( ID_ACTENVMENU_FOCUS, wxT("Focus") );
	}
	PopupMenu( &menu );
}

void CEdWorldEnvironmentToolPanel::OnDefMenuNewDefinition( wxCommandEvent& event )
{
	// Get directory
	DefinitionItemData* data = static_cast<DefinitionItemData*>( m_definitionsTree->GetItemData( m_popupSource ) );
	CDirectory* dir = dynamic_cast< CDirectory* >( data->obj );
	ASSERT( dir, TXT("Somehow OnDefMenuNewDefinition was called with either an invalid m_popupSource or a non-directory item") );

	// Get name
	String name;
	if ( !InputBoxFileName( this, TXT("Create new environment definition"), wxT("Enter the name for the environment definition"), name, TXT("env") ) )	
	{
		return;
	}

	// Make sure there is no file or folder with that name
	if ( dir->FindLocalDirectory( name.AsChar() ) || dir->FindLocalFile( name ) )
	{
		wxMessageBox( wxString::Format( wxT("Cannot use '%s' as a name - another file or subdirectory with that name already exists"), name.AsChar() ), wxT("Already there"), wxOK|wxICON_ERROR|wxCENTRE, this );
		return;
	}

	// Create the resource
	CEnvironmentDefinition* newdef = CreateObject< CEnvironmentDefinition >( CEnvironmentDefinition::GetStaticClass(), NULL, 0 );
	if ( !newdef->SaveAs( dir, name ) )
	{
		wxMessageBox( wxString::Format( wxT("Failed to save '%s'"), name.AsChar() ), wxT("Error"), wxOK|wxICON_ERROR|wxCENTRE, this );
		delete newdef;
		return;
	}

	// Update tree and select the new resource
	UpdateDefinitionsTree();
	wxTreeItemId newid;
	if ( m_defTreeMap.Find( newdef->GetFile(), newid ) )
	{
		m_definitionsTree->SelectItem( newid );
		m_definitionsTree->SetFocusedItem( newid );
	}

	// If there is an environment area, activate the item to set
	// it for the area
	if ( m_areaEnvComponent )
	{
		wxTreeEvent fakeit( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, m_definitionsTree, newid );
		OnDefinitionsTreeItemActivated( fakeit );
	}
}

void CEdWorldEnvironmentToolPanel::OnDefMenuNewFolder( wxCommandEvent& event )
{
	// Get directory
	DefinitionItemData* data = static_cast<DefinitionItemData*>( m_definitionsTree->GetItemData( m_popupSource ) );
	CDirectory* dir = dynamic_cast< CDirectory* >( data->obj );
	ASSERT( dir, TXT("Somehow OnDefMenuNewFolder was called with either an invalid m_popupSource or a non-directory item") );

	// Get name
	String name;
	if ( !InputBoxFileName( this, TXT("Create new folder in environment definitions"), wxT("Enter the name for the folder"), name, TXT("") ) )	
	{
		return;
	}

	// Remove extension, we do not care
	CFilePath path( name );
	name = path.GetFileName();

	// Make sure there is no file or folder with that name
	if ( dir->FindLocalDirectory( name.AsChar() ) || dir->FindLocalFile( name + TXT(".") ) )
	{
		wxMessageBox( wxString::Format( wxT("Cannot use '%s' as a name - another file or subdirectory with that name already exists"), name.AsChar() ), wxT("Already there"), wxOK|wxICON_ERROR|wxCENTRE, this );
		return;
	}

	// Attempt to create the directory and save it
	CDirectory* newdir = dir->CreateNewDirectory( name );
	if ( !newdir->CreateOnDisk() )
	{
		wxMessageBox( wxString::Format( wxT("Failed to create the directory '%s' on disk but it is on the internal filesystem tree - what happens next, i have no idea!"), name.AsChar() ), wxT("Pay some attention to this"), wxOK|wxICON_WARNING|wxCENTRE, this );
	}

	// Update the tree and select the new directory
	UpdateDefinitionsTree();
	wxTreeItemId newid;
	if ( m_defTreeMap.Find( newdir, newid ) )
	{
		m_definitionsTree->SelectItem( newid );
		m_definitionsTree->SetFocusedItem( newid );
	}
}

void CEdWorldEnvironmentToolPanel::OnDefMenuRename( wxCommandEvent& event )
{
	// Get directory
	DefinitionItemData* data = static_cast<DefinitionItemData*>( m_definitionsTree->GetItemData( m_popupSource ) );

	// Rename directory
	if ( dynamic_cast< CDirectory* >( data->obj ) ) 
	{
		// Check if the directory has any loaded file and warn the user
		const TFiles& files = data->dir->GetFiles();
		Bool anyLoaded = false;
		for ( auto it=files.Begin(); it != files.End(); ++it )
		{
			if ( (*it)->IsLoaded() )
			{
				anyLoaded = true;
				break;
			}
		}

		// Do the Warn
		if ( anyLoaded )
		{
			if ( wxMessageBox( wxT("The directory IS NOT EMPTY AND THERE ARE LOADED FILES!! If any of the environment definitions inside are active the engine may crash! Proceed at your own risk! Do you want to abort?"), wxT("Big fat scary warning!"), wxICON_WARNING|wxYES_NO|wxCENTRE, this ) != wxNO )
			{
				return;
			}
		}

		// Get name
		CFilePath path( data->dir->GetName() );
		String name;
		if ( !InputBoxFileName( this, TXT("Rename directory"), wxT("Enter the new name for the directory"), name, TXT("") ) )	
		{
			return;
		}

		// Form proper name
		CFilePath tempPath( name );
		path.SetFileName( tempPath.GetFileName() );
		name = path.GetFileNameWithExt();

		// Check name
		if ( name == data->dir->GetName() )
		{
			return;
		}

		// Make sure there is no file or folder with that name
		if ( data->dir->GetParent()->FindLocalDirectory( name.AsChar() ) || data->dir->GetParent()->FindLocalFile( name ) )
		{
			wxMessageBox( wxString::Format( wxT("Cannot use '%s' as a name - another file or subdirectory with that name already exists"), name.AsChar() ), wxT("Already there"), wxOK|wxICON_ERROR|wxCENTRE, this );
			return;
		}

		// Rename the directory
		if ( !data->dir->Rename( path.GetFileName() ) )
		{
			wxMessageBox( wxT("Failed to rename the directory"), wxT("Failure"), wxICON_ERROR|wxOK|wxCENTRE, this );
			return;
		}

		// Update tree and select the new resource
		UpdateDefinitionsTree();
		wxTreeItemId newid;
		if ( m_defTreeMap.Find( data->dir, newid ) )
		{
			m_definitionsTree->SelectItem( newid );
			m_definitionsTree->SetFocusedItem( newid );
		}
	}
	else // Rename file
	{
		// Get name
		CFilePath path( data->file->GetFileName() );
		String name;
		if ( !InputBoxFileName( this, TXT("Rename environment definition"), wxT("Enter the new name for the environment definition"), name, TXT("env") ) )	
		{
			return;
		}

		// Form proper name
		CFilePath tempPath( name );
		path.SetFileName( tempPath.GetFileName() );
		name = path.GetFileNameWithExt();

		// Check name
		if ( name == data->file->GetFileName() )
		{
			return;
		}

		// Make sure there is no file or folder with that name
		if ( data->file->GetDirectory()->FindLocalDirectory( name.AsChar() ) || data->file->GetDirectory()->FindLocalFile( name ) )
		{
			wxMessageBox( wxString::Format( wxT("Cannot use '%s' as a name - another file or subdirectory with that name already exists"), name.AsChar() ), wxT("Already there"), wxOK|wxICON_ERROR|wxCENTRE, this );
			return;
		}

		// Rename the file
		if ( !data->file->Rename( path.GetFileName(), path.GetExtension() ) )
		{
			wxMessageBox( wxT("Failed to rename the environment definition"), wxT("Failure"), wxICON_ERROR|wxOK|wxCENTRE, this );
		}

		// Update tree and select the new resource
		UpdateDefinitionsTree();
		wxTreeItemId newid;
		if ( m_defTreeMap.Find( data->file, newid ) )
		{
			m_definitionsTree->SelectItem( newid );
			m_definitionsTree->SetFocusedItem( newid );
		}
	}
}

void CEdWorldEnvironmentToolPanel::OnDefMenuDelete( wxCommandEvent& event )
{
	DefinitionItemData* data = static_cast<DefinitionItemData*>( m_definitionsTree->GetItemData( m_popupSource ) );
	CDirectory* parent;

	// Delete directory
	if ( dynamic_cast< CDirectory* >( data->obj ) )
	{
		// Confirm deletion
		if ( wxMessageBox( wxString::Format( wxT("Are you sure that you want to delete the folder '%s'?"), data->dir->GetName().AsChar() ), wxT("Confirm This"), wxICON_QUESTION|wxYES_NO|wxCENTRE, this ) != wxYES )
		{
			return;
		}

		// A second confirmation in case there are files in there
		if ( (data->dir->GetFiles().Size() > 0) && wxMessageBox( wxString::Format( wxT("Are you *REALLY* sure? There are files in there and stuff will break if these files are referenced from somewhere!"), data->file->GetFileName().AsChar() ), wxT("Think of the files"), wxICON_QUESTION|wxYES_NO|wxCENTRE, this ) != wxYES )
		{
			return;
		}

		// Attempt to delete the directory
		parent = data->dir->GetParent();
		if ( !parent->DeleteChildDirectory( data->dir ) )
		{
			wxMessageBox( wxT("Failed to remove the directory"), wxT("Error"), wxICON_ERROR|wxCENTRE|wxOK, this );
		}
	}
	else // Delete file
	{
		// Confirm deletion
		if ( wxMessageBox( wxString::Format( wxT("Are you sure that you want to delete the file '%s'?"), data->file->GetFileName().AsChar() ), wxT("Confirm This"), wxICON_QUESTION|wxYES_NO|wxCENTRE, this ) != wxYES )
		{
			return;
		}

		// Do delete
		parent = data->file->GetDirectory();
		DeleteSingleFile( data->file );
	}

	// Update the parent directory
	parent->Repopulate();

	// Update the tree
	UpdateDefinitionsTree();

	// Select the parent
	wxTreeItemId parentid;
	if ( m_defTreeMap.Find( parent, parentid ) )
	{
		m_definitionsTree->SelectItem( parentid );
		m_definitionsTree->SetFocusedItem( parentid );
	}
}

void CEdWorldEnvironmentToolPanel::OnActEnvMenuEdit( wxCommandEvent& event )
{
	// Get selected item index
	Uint32 selection;
	if ( !GetActiveEnvironmentsSelection( selection ) )
	{
		return;
	}

	// Get active environments array
	CEnvironmentManager* envMgr = GetEnvironmentManager();
	const CEnvironmentManager::TAreaEnvironmentsArray& envs = envMgr->GetActiveEnvironments();

	// Create fake event
	wxCommandEvent fakeEvent( wxEVT_COMMAND_BUTTON_CLICKED, XRCID(WORLDENV_ENV_LIST_NAME) );
	fakeEvent.SetInt( (int)selection );

	// Call event handler for double clicking on the active environment list
	OnEditWorldEnvironment( fakeEvent );
}

void CEdWorldEnvironmentToolPanel::OnActEnvMenuSelect( wxCommandEvent& event )
{
	// Get selected item index
	Uint32 selection;
	if ( !GetActiveEnvironmentsSelection( selection ) )
	{
		return;
	}

	// Get active environments array
	CEnvironmentManager* envMgr = GetEnvironmentManager();
	const CEnvironmentManager::TAreaEnvironmentsArray& envs = envMgr->GetActiveEnvironments();
	ASSERT( envs[selection].areaComponent, TXT("The selected environment has no associated area component") );

	// Select the environment component's entity in the world
	if ( GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetSelectionManager() )
	{
		GGame->GetActiveWorld()->GetSelectionManager()->DeselectAll();
		GGame->GetActiveWorld()->GetSelectionManager()->Select( envs[selection].areaComponent->GetEntity() );

		// Focus the world editor camera to the area if the "Focus" option was selected
		if ( event.GetId() == ID_ACTENVMENU_FOCUS )
		{
			wxTheFrame->GetWorldEditPanel()->LookAtSelectedNodes();
		}
	}
}

void CEdWorldEnvironmentToolPanel::SetStayOnTop( bool stayOnTop )
{
	wxWindow* parent = GetParent();

	if ( parent )
	{
		// Find toplevel window
		while ( parent->GetParent() ) parent = parent->GetParent();

		// Make sure the flag isn't there
		parent->SetWindowStyleFlag( parent->GetWindowStyleFlag() & ~wxSTAY_ON_TOP );

		// Set the flag if needed
		if ( stayOnTop )
		{
			parent->SetWindowStyleFlag( parent->GetWindowStyleFlag() | wxSTAY_ON_TOP );
		}

		// Make sure the flags are applied
		parent->Refresh();
	}

	// Sync the toggle button and passed flag
	if ( m_stayOnTop->GetValue() != stayOnTop )
	{
		m_stayOnTop->SetValue( stayOnTop );
	}
}

bool CEdWorldEnvironmentToolPanel::GetStayOnTop() const
{
	return m_stayOnTop->GetValue();
}

void CEdWorldEnvironmentToolPanel::OnDefinitionsTreeItemActivated( wxTreeEvent& event )
{
	// Get item
	wxTreeItemId item = event.GetItem();
	DefinitionItemData* itemData = static_cast<DefinitionItemData*>( m_definitionsTree->GetItemData( item ) );
	CWorld* world = GGame->GetActiveWorld();

	// Ignore if we have no world
	if ( !world )
	{
		return;
	}
	
	// If a directory was activated, ignore it
	if ( dynamic_cast<CDirectory*>( itemData->obj ) )
	{
		return;
	}

	// Check if we have an environment area available
	if ( m_areaEnvComponent )
	{
		// Make sure we can actually modify the environment area
		if ( !m_areaEnvComponent->MarkModified() )
		{
			return;
		}
	}
	else // No environment area, edit world
	{
		// Make sure we can modify the world
		if ( !world->MarkModified() )
		{
			return;
		}
	}

	// Load/find the environment definition for the clicked item's file
	if ( !itemData->file->Load() )
	{
		wxMessageBox( wxT("Failed to load the environment definition file."), wxT("Error"), wxOK|wxICON_ERROR|wxCENTRE, this );
		return;
	}
	CEnvironmentDefinition* envdef = Cast< CEnvironmentDefinition >( itemData->file->GetResource() );
	if ( !envdef )
	{
		wxMessageBox( wxT("Invalid resource - expected an environment definition resource."), wxT("Invalid resource"), wxOK|wxICON_ERROR|wxCENTRE, this );
		return;
	}

	// Assign the environment definition to the environment area/world
	if ( m_areaEnvComponent )
	{
		m_areaEnvComponent->SetEnvironmentDefinition( envdef );
	}
	else
	{
		SWorldEnvironmentParameters params = world->GetEnvironmentParameters();
		params.m_environmentDefinition = envdef;
		world->SetEnvironmentParameters( params );
	}

	// Edit the new environment definition
	ShowAndEdit( envdef );
}

void CEdWorldEnvironmentToolPanel::OnDefinitionsTreeItemMenu( wxTreeEvent& event )
{
	wxMenu menu;
	m_popupSource = event.GetItem();
	DefinitionItemData* data = static_cast<DefinitionItemData*>( m_definitionsTree->GetItemData( m_popupSource ) );

	if ( dynamic_cast< CDirectory* >( data->obj ) )
	{
		menu.Append( ID_DEFTREEMENU_NEWDEFINITION, wxT("New definition") );
		menu.Append( ID_DEFTREEMENU_NEWFOLDER, wxT("New subfolder") );
	}
	menu.Append( ID_DEFTREEMENU_RENAME, wxT("Rename") );
	menu.Append( ID_DEFTREEMENU_DELETE, wxT("Delete") );
	PopupMenu( &menu );
}

void CEdWorldEnvironmentToolPanel::UpdateGlobalEnvironmentsPanel()
{
	CEnvironmentManager* envMgr = GetEnvironmentManager();
	if ( !envMgr )
	{
		return;
	}

	const CEnvironmentManager::TAreaEnvironmentsArray& envs = envMgr->GetActiveEnvironments();

	int itemIndex = m_activeEnvs->GetSelection();
	m_activeEnvs->Clear();
	for ( auto it=envs.Begin(); it != envs.End(); ++it )
	{
		const SEnvManagerAreaEnvData& env = *it;
		String description = String::Printf( TXT("[P%d] %d%% "), env.priority, (int)( env.appliedBlendFactor*100.0f ) );
		if ( -1 != env.appliedMostImportantFactor )
		{
			description += String::Printf( TXT(" (important %d%%)"), (int)( env.appliedMostImportantFactor*100.0f ) );
		}
		switch ( env.timeBlendState )
		{
		case ETBS_BlendIn:
			description += String::Printf( TXT(" (blend in %d%%)"), (int)( env.timeBlendFactor*100.0f ) );
			break;
		case ETBS_BlendOut:
			description += String::Printf( TXT(" (blend out %d%%)"), (int)( env.timeBlendFactor*100.0f ) );
			break;
		}
		if ( env.areaComponent )
		{
			description += String::Printf( TXT(" [%s]"), env.areaComponent->GetFriendlyName().AsChar() );
		}
		else if( !env.pathToEnvDefinition.Empty() )
		{
			String fileNameStr = env.pathToEnvDefinition.StringBefore( TXT(".env") );
			TDynArray<String> splitStr = fileNameStr.Split( TXT("\\") );

			if( !splitStr.Empty() ) description += splitStr.Back();
		}

		m_activeEnvs->AppendString( description.AsChar() );
	}

	if ( itemIndex < envs.SizeInt() )
	{
		m_activeEnvs->Select( itemIndex );
	}

	// Update current weather condition info
	CWeatherManager* wMgr = envMgr->GetWeatherManager();
	if( wMgr )
	{
		String currentConditionName = String::EMPTY;
		Float currentEvolutionStatus = 0.0f;

		wMgr->GetCurrentWeatherConditionInfo( currentConditionName, currentEvolutionStatus );

		m_currentWeatheConditionTextCtrl->SetLabelText( currentConditionName.AsChar() );
		m_currentWeatherConditionStatus->SetValue( Clamp<Float>( currentEvolutionStatus, 0.0f, 1.0f ) * 100.0f );
	}
	
}

static void UpdateDefinitionsTree_RecFunc( CDirectory* dir, wxTreeCtrl* tree, const wxTreeItemId& parent, THashMap< IDepotObject*, wxTreeItemId >& defTreeMap )
{
	// Scan subdirectories
	for ( CDirectory* sub : dir->GetDirectories() )
	{
		wxTreeItemId subitem = tree->AppendItem( parent, sub->GetName().AsChar(), 0, 0, new DefinitionItemData( sub ) );
		defTreeMap.Insert( sub, subitem );
		UpdateDefinitionsTree_RecFunc( sub, tree, subitem, defTreeMap );
	}

	// Scan all files
	const TFiles& files = dir->GetFiles();
	for ( auto it=files.Begin(); it != files.End(); ++it )
	{
		CDiskFile* file = *it;

		// Add the file to the array if it is an environment
		if ( file->GetFileName().EndsWith( wxT(".env") ) )
		{
			CFilePath path( file->GetDepotPath() );
			wxTreeItemId fileitem = tree->AppendItem( parent, path.GetFileName().AsChar(), 1, 1, new DefinitionItemData( file ) );
			defTreeMap.Insert( file, fileitem );
		}
	}
}

void CEdWorldEnvironmentToolPanel::UpdateDefinitionsTree()
{
	// Reset tree and map
	m_definitionsTree->DeleteAllItems();
	m_defTreeMap.Clear();
	
	// Add root
	wxTreeItemId root = m_definitionsTree->AddRoot( wxT("Definitions") );

	// Obtain files
	CDirectory* rootDir = GDepot->FindPath( WORLDENV_DEFINITIONS_ROOT );
	if ( rootDir )
	{
		UpdateDefinitionsTree_RecFunc( rootDir, m_definitionsTree, root, m_defTreeMap );
	}
}

void CEdWorldEnvironmentToolPanel::UpdateWeatherConditions()
{
	// Find environment manager
	CEnvironmentManager* envMgr = GetEnvironmentManager();
	if ( !envMgr )
	{
		return;
	}

	// Find weathers choice box
	wxChoice* choice = XRCCTRL( *this, "envWeatherChoice", wxChoice );

	// Refill the box with new weather conditions
	wxString previousChoice = choice->GetStringSelection();
	choice->Clear();

	const TDynArray<SWeatherCondition>& weatherConditions = envMgr->GetWeatherManager()->GetWeatherConditions();
	for ( auto it=weatherConditions.Begin(); it != weatherConditions.End(); ++it )
	{
		const SWeatherCondition& condition = *it;
		choice->Append( condition.m_name.AsString().AsChar() );
	}

	// Reselect the previous weather, if it is still there
	int previousChoiceIndex = choice->FindString( previousChoice );
	if ( previousChoiceIndex != wxNOT_FOUND )
	{
		choice->SetSelection( previousChoiceIndex );
	}
}

void CEdWorldEnvironmentToolPanel::UpdateWeatherEnvironments()
{
	// Find environment manager
	CEnvironmentManager* envMgr = GetEnvironmentManager();
	if ( !envMgr )
	{
		return;
	}

	CWeatherManager* wMgr = envMgr->GetWeatherManager();
	if( wMgr != nullptr && wMgr->GetCurrentWeatherCondition().m_environmentDefinition ) envMgr->ChangeAreaEnvironmentParameters( wMgr->GetCurrentEnvironmentId(), wMgr->GetCurrentWeatherCondition().m_environmentDefinition.Get()->GetAreaEnvironmentParams() );
}

void CEdWorldEnvironmentToolPanel::OnPropertiesModified( wxCommandEvent& event )
{
	CEnvironmentManager* manager = GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetEnvironmentManager() : NULL;
	ASSERT( manager, TXT("No environment manager in the world, what is going on?") );

	if ( m_areaEnvComponent && manager->IsAreaEnvironmentActive( m_areaEnvComponent->GetAreaEnvId() ) )
	{
		m_areaEnvComponent->Deactivate( true );
		m_areaEnvComponent->Activate( true );
	}	
}

void CEdWorldEnvironmentToolPanel::OnPauseChecked( wxCommandEvent& event )
{
	UpdateWorldStatus();
	}

void CEdWorldEnvironmentToolPanel::OnUpdateSelection( wxCommandEvent &event )
{
	CEnvironmentManager *envManager = GetEnvironmentManager();
	CGameEnvironmentParams	gameParams	= envManager->GetGameEnvironmentParams();

	wxSlider*		rightSelectSlider	= XRCCTRL( *this, WORLDENV_RIGHT_SELECT, wxSlider );
	wxSlider*		leftSelectSlider	= XRCCTRL( *this, WORLDENV_LEFT_SELECT, wxSlider );

	// update 
	{
		Float leftValue = leftSelectSlider->GetValue() / GAllSeconds;
		SSimpleCurve::s_graphMinTimeSelection = leftValue;
		Float rightValue = rightSelectSlider->GetValue() / GAllSeconds;
		SSimpleCurve::s_graphMaxTimeSelection = rightValue;

		if( SSimpleCurve::s_graphMinTimeSelection >= SSimpleCurve::s_graphMaxTimeSelection && leftSelectSlider->HasFocus() == true )
		{
			SSimpleCurve::s_graphMaxTimeSelection = SSimpleCurve::s_graphMinTimeSelection;
			wxSlider*		rightSelectSlider		= XRCCTRL( *this, WORLDENV_RIGHT_SELECT, wxSlider );
			rightSelectSlider->SetValue( leftSelectSlider->GetValue() );
		}
		if( SSimpleCurve::s_graphMaxTimeSelection <= SSimpleCurve::s_graphMinTimeSelection && rightSelectSlider->HasFocus() == true )
		{
			SSimpleCurve::s_graphMinTimeSelection = SSimpleCurve::s_graphMaxTimeSelection;
			wxSlider	*leftSelectSlider		= XRCCTRL( *this, WORLDENV_LEFT_SELECT, wxSlider );
			leftSelectSlider->SetValue( rightSelectSlider->GetValue() );
		}
	}

	// Update labels
	{
		Int32 seconds = (Int32)leftSelectSlider->GetValue();
		Int32 hours = seconds / ( 60 * 60 );
		seconds -= ( hours * ( 60 * 60 ) );
		Int32 minutes = seconds / 60;
		seconds -= ( minutes * 60 );
		m_leftSelectPicker->SetTime( hours, minutes, seconds );

		// update right
		seconds = (Int32)rightSelectSlider->GetValue();
		hours = seconds / ( 60 * 60 );
		seconds -= ( hours * ( 60 * 60 ) );
		minutes = seconds / 60;
		seconds -= ( minutes * 60 );
		m_rightSelectPicker->SetTime( hours, minutes, seconds );
	}

	SSimpleCurve::s_graphGlobalSelectionEnable = true;
	m_tool->NotifyCurveDisplayTimeChanged();
}

void CEdWorldEnvironmentToolPanel::OnClearSelection( wxCommandEvent &event )
{
	ClearAdvancedTools();
}

void CEdWorldEnvironmentToolPanel::OnChangeLeftTimePicker( wxDateEvent& event )
{
	Int32 hours, minutes, seconds;
	m_leftSelectPicker->GetTime( &hours, &minutes, &seconds );

	seconds += ( minutes * 60 ) + ( hours * 60 * 60 );
	Float value = seconds / (Float)GAllSeconds;

	wxSlider* leftSelectSlider	= XRCCTRL( *this, WORLDENV_LEFT_SELECT, wxSlider );
	leftSelectSlider->SetValue( seconds );
	leftSelectSlider->SetFocus();

	OnUpdateSelection( event );
}

void CEdWorldEnvironmentToolPanel::OnChangeRightTimePicker( wxDateEvent& event )
{
	Int32 hours, minutes, seconds;
	m_rightSelectPicker->GetTime( &hours, &minutes, &seconds );

	seconds += ( minutes * 60 ) + ( hours * 60 * 60 );
	Float value = seconds / (Float)GAllSeconds;

	wxSlider* rightSelectSlider	= XRCCTRL( *this, WORLDENV_RIGHT_SELECT, wxSlider );
	rightSelectSlider->SetValue( seconds );
	rightSelectSlider->SetFocus();

	OnUpdateSelection( event );
}

void CEdWorldEnvironmentToolPanel::OnChangeFakeDayCycleTimePicker( wxDateEvent& event )
{
	Int32 hours, minutes, seconds;
	m_fakeDayCycleTimePicker->GetTime( &hours, &minutes, &seconds );

	seconds += ( minutes * 60 ) + ( hours * 60 * 60 );
	Float value = seconds / (Float)GAllSeconds;

	wxSlider* fakeDayCycleProgress = XRCCTRL( *this, WORLDENV_FAKE_DAY_CYCLE_SLIDER_NAME, wxSlider );
	fakeDayCycleProgress->SetValue( seconds );

	SendSceneEditorRefreshNotification();
	OnEdition( event );
}

void CEdWorldEnvironmentToolPanel::OnChangeFakeDayCycleSlider( wxCommandEvent &event )
{
	SendSceneEditorRefreshNotification();
	OnEdition( event );
}

void CEdWorldEnvironmentToolPanel::SendSceneEditorRefreshNotification() const
{
	CWorld* world = GetEnvironmentManager() ? GetEnvironmentManager()->GetWorld() : nullptr;		
	EDITOR_DISPATCH_EVENT( CNAME(SceneEditorEnvironmentsUpdate), CreateEventData( world ) );
}

void CEdWorldEnvironmentToolPanel::ShowHideAdvancedTools( wxCommandEvent &event )
{
	RefreshAdvancedTools();
	}

void CEdWorldEnvironmentToolPanel::ManageCurve( SSimpleCurve* curve, const String& curvePath, ECurveActionType action, CurvePointsCollection& values )
{
	wxChoice* selectedTab = XRCCTRL( *this, WORLDENV_SELECTED_TAB, wxChoice );
	if( selectedTab == nullptr )
	{
		return;
	}
	else
	{
		String text = TXT("/CAreaEnvironmentParams/");
		text += selectedTab->GetString( selectedTab->GetSelection() );
		if( selectedTab->GetString( selectedTab->GetSelection() ) != TXT("All") && curvePath.ContainsSubstring( text ) == false )
		{
			return;
		}
	}

	// manage
	if( action == AT_Copy )
	{
		const Uint32 pointCount = curve->GetNumPoints();
		for ( Uint32 i=0; i<pointCount; ++i )
		{
			Float time = curve->GetCurveData().GetTimeAtIndex(i);
			Vector value = curve->GetValue( time );

			if( time >= SSimpleCurve::s_graphMinTimeSelection && time <= SSimpleCurve::s_graphMaxTimeSelection )
			{
				CurvePoints& points = values.GetRef( curvePath );
				points.PushBack( PointEntry( time, value ) );
			}
		}
	}
	else if( action == AT_Paste )
	{
		CurvePoints* pointsPtr = values.FindPtr( curvePath );
		if( pointsPtr != nullptr )
		{
			CurvePoints& pointsRef = *pointsPtr;

			Float firstPointTime = 0.0f;
			const Uint32 pointCount = pointsRef.Size();
			for( Uint32 i=0; i<pointCount; ++i )
			{
				PointEntry& point = pointsRef[i];
				Float time = point.m_first;

				// prepare correct time 
				if( i==0 )
				{
					firstPointTime = time;
					time = SSimpleCurve::s_graphTimeDisplayValue;
				}
				else
				{
					time = SSimpleCurve::s_graphTimeDisplayValue + ( time - firstPointTime );
				}

				Vector value = point.m_second;

				Int32 pointIndex = curve->GetIndex( time );
				if( pointIndex != -1 )
				{
					curve->SetValue( pointIndex, value );
				}
				else
				{
					Int32 newIndex = curve->AddPoint( time, value );
					if( newIndex != -1 )
					{
						SCurveTangents tangents = curve->GetTangent( newIndex );
						curve->SetTangent( newIndex, tangents );
					}
				}
				SEvents::GetInstance().DispatchEvent( CNAME( SimpleCurveChanged ), CreateEventData( SSimpleCurveChangeData( curve, CName::NONE ) ) );	
			}
		}
	}
	else if( action == AT_Remove )
	{
		// remove points
		{
			// mark to remove
			TDynArray< Float > pointsToRemove;
			const Uint32 pointCount = curve->GetNumPoints();
			for ( Uint32 i=0; i<pointCount; ++i )
			{
				Float time;
				time = curve->GetCurveData().GetTimeAtIndex(i);

				if( time >= SSimpleCurve::s_graphMinTimeSelection && time <= SSimpleCurve::s_graphMaxTimeSelection )
				{
					pointsToRemove.PushBack( time );
				}
			}
			
			//remove
			const Uint32 pointToRemoveCount = pointsToRemove.Size();
			for( Uint32 i=0; i<pointToRemoveCount; ++i )
			{
				curve->RemovePointAtTime( pointsToRemove[i] );
			}
		}
	}
	else if( action == AT_AddNewKey )
	{
		wxSpinCtrl* toleranceTimeCtrl = XRCCTRL( *this, WORLDENV_NEW_KEY_TOLERANCE_TIME, wxSpinCtrl );
		Uint32 toleranceTime = 0;
		if( toleranceTimeCtrl != nullptr )
		{
			toleranceTime = toleranceTimeCtrl->GetValue();
		}
		Float normalizedTimeTolerance = toleranceTime / ( 24.0f * 60.0f );

		// find the closest points
		Bool found = false;
		const Uint32 pointCount = curve->GetCurveData().Size();
		Float leftBorder = SSimpleCurve::s_graphTimeDisplayValue - normalizedTimeTolerance;
		Clamp( leftBorder, 0.0f, 1.0f );
		Float rightBorder = SSimpleCurve::s_graphTimeDisplayValue + normalizedTimeTolerance;
		Clamp( rightBorder, 0.0f, 1.0f );
		for( Uint32 i=0; i<pointCount; ++i )
		{
			Float time =  curve->GetCurveData().GetTimeAtIndex(i);
			if( time >= leftBorder && time <= rightBorder )
			{
				found = true;
				break;
			}
		}

		if( found == false )
		{
			Int32 pointIndex = curve->GetIndex( SSimpleCurve::s_graphTimeDisplayValue );
			if( pointIndex == -1 )
			{
				Vector newValue = curve->GetValue( SSimpleCurve::s_graphTimeDisplayValue );
				Int32 newIndex = curve->AddPoint( SSimpleCurve::s_graphTimeDisplayValue, newValue );
				if( newIndex != -1 )
				{
					SCurveTangents tangents = curve->GetTangent( newIndex );
					curve->SetTangent( newIndex, tangents );
				}
			}
		}
	}
}

void CEdWorldEnvironmentToolPanel::RecursiveStuff( void* obj, IRTTIType* type, const String& path, ECurveActionType action, CurvePointsCollection& values )
{
	if( obj == nullptr )
	{
		return;
	}

	switch ( type->GetType() )
	{
	case RT_Class:
		{
			CClass* cls = static_cast< CClass* >( type );
			if( cls->IsA< SSimpleCurve >() == true )
			{
				SSimpleCurve* curve = cls->CastTo< SSimpleCurve >( obj );
				String curvePath = path + TXT("/") + cls->GetName().AsString();
				ManageCurve( curve, curvePath, action, values );
				return;
			}

			const auto& properties = cls->GetCachedProperties();
			for ( auto it=properties.Begin(); it != properties.End(); ++it )
			{
				CProperty* property = *it;
				RecursiveStuff( property->GetOffsetPtr( obj ), property->GetType(), path + TXT("/") + cls->GetName().AsString() + TXT("/") + property->GetName().AsString(), action, values );
			}
		}
		break;
	case RT_Array:
		{
			IRTTIBaseArrayType* arr = static_cast< IRTTIBaseArrayType* >( type );
			IRTTIType* inner = arr->ArrayGetInnerType();
			Uint32 size = arr->ArrayGetArraySize( obj );
			for ( Uint32 i=0; i < size; ++i )
			{
				void* item = arr->ArrayGetArrayElement( obj, i );
				RecursiveStuff( item, inner, path + TXT("/") + arr->GetName().AsString() + TXT("/") + ToString(i), action, values );
			}
		}
		break;
	case RT_Pointer:
		{
			CRTTIPointerType* ptr = static_cast< CRTTIPointerType* >( type );
			void* pointed = ptr->GetPointed( obj );
			if ( pointed != nullptr )
			{
				RecursiveStuff( pointed, ptr->GetPointedType(), path + TXT("/") + ptr->GetName().AsString(), action, values );
			}
		}
		break;
	}
}

void CEdWorldEnvironmentToolPanel::OnCopySelection( wxCommandEvent &event )
{
	if ( m_lastData != nullptr )
	{
		CurvePointsCollection values;
		RecursiveStuff( m_lastData, m_lastData->GetClass(), TXT(""), AT_Copy, values );

		// serialize information to clipboard
		TDynArray< Uint8 > buffer;
		CMemoryFileWriter writer( buffer );
		writer << values;

		if ( wxTheClipboard->Open() == true )
		{
			wxTheClipboard->SetData( new CClipboardData( TXT("CurveTool"), buffer, true ) );
			wxTheClipboard->Close();
		}
		m_browser->Refresh( false );
	}
}

void CEdWorldEnvironmentToolPanel::OnPasteSelection( wxCommandEvent &event )
{
	if ( m_lastData != nullptr )
	{
		CurvePointsCollection values;
		if ( wxTheClipboard->Open() == true )
		{
			CClipboardData data( TXT("CurveTool") );
			if ( wxTheClipboard->IsSupported( data.GetDataFormat() ) == true )
			{
				if ( wxTheClipboard->GetData( data ) == true )
				{
					CMemoryFileReader reader( data.GetData(), 0 );
					reader << values;
				}
			}
			wxTheClipboard->Close();
		}

		RecursiveStuff( m_lastData, m_lastData->GetClass(), TXT(""), AT_Paste, values );
		m_browser->Refresh( false );
	}
}

void CEdWorldEnvironmentToolPanel::OnRemovePoints( wxCommandEvent& event )
{
	if ( m_lastData != nullptr )
	{
		CurvePointsCollection values;
		if ( wxTheClipboard->Open() == true )
		{
			CClipboardData data( TXT("CurveTool") );
			if ( wxTheClipboard->IsSupported( data.GetDataFormat() ) == true )
			{
				if ( wxTheClipboard->GetData( data ) == true )
				{
					CMemoryFileReader reader( data.GetData(), 0 );
					reader << values;
				}
			}
			wxTheClipboard->Close();
		}

		RecursiveStuff( m_lastData, m_lastData->GetClass(), TXT(""), AT_Remove, values );
		m_browser->Refresh( false );
	}
}

void CEdWorldEnvironmentToolPanel::OnAddNewKey( wxCommandEvent& event )
{
	if ( m_lastData != nullptr )
	{
		CurvePointsCollection notUse;
		RecursiveStuff( m_lastData, m_lastData->GetClass(), TXT(""), AT_AddNewKey, notUse );
		m_browser->Refresh( false );
	}
}

void CEdWorldEnvironmentToolPanel::FillCopyAreasControl()
{
	wxChoice* selectedTab = XRCCTRL( *this, WORLDENV_SELECTED_TAB, wxChoice );

	if( selectedTab != nullptr && m_lastData != nullptr )
	{
		selectedTab->Clear();
		
		// add new options
		selectedTab->Append( TXT("All") );
		selectedTab->Select( 0 );

		//
		RecursiveFillAreas( m_lastData, m_lastData->GetClass(), selectedTab );
	}
}

void CEdWorldEnvironmentToolPanel::RecursiveFillAreas( void* obj, IRTTIType* type, wxChoice* choice )
{
	if( obj == nullptr )
	{
		return;
	}

	switch ( type->GetType() )
	{
	case RT_Class:
		{
			CClass* cls = static_cast< CClass* >( type );
			
			Bool addToChoice = false;
			if( cls->GetName() == TXT("CAreaEnvironmentParams") )
			{
				addToChoice = true;
			}

			const auto& properties = cls->GetCachedProperties();
			for ( auto it=properties.Begin(); it != properties.End(); ++it )
			{
				CProperty* property = *it;
				if( addToChoice ==  true )
				{
					choice->Append( property->GetHint().AsChar() );
				}
				else
				{
					RecursiveFillAreas( property->GetOffsetPtr( obj ), property->GetType(), choice );
				}
			}
		}
		break;
	}
}

void CEdWorldEnvironmentToolPanel::RefreshAdvancedTools()
{
	wxToggleButton*	advancedOptions	= XRCCTRL( *this, WORLDENV_ADVANCED_TOOLS,	wxToggleButton );
	wxPanel* copyPastePanel = XRCCTRL( *this, "m_copyPasteTool", wxPanel );

	Bool showTools = !advancedOptions->GetValue();
	if( showTools == true )
	{
		copyPastePanel->Hide();
		ClearAdvancedTools();
	}
	else
	{
		copyPastePanel->Show();
	}
	LayoutRecursively( this, false );
}

void CEdWorldEnvironmentToolPanel::ClearAdvancedTools()
{
	SSimpleCurve::s_graphGlobalSelectionEnable = false;
	SSimpleCurve::s_graphMinTimeSelection = 0.0f;
	SSimpleCurve::s_graphMaxTimeSelection = 0.0f;

	wxSlider* leftSelectSlider	= XRCCTRL( *this, WORLDENV_LEFT_SELECT, wxSlider );
	wxSlider* rightSelectSlider	= XRCCTRL( *this, WORLDENV_RIGHT_SELECT, wxSlider );
	rightSelectSlider->SetValue( 0 );
	leftSelectSlider->SetValue( 0 );
	m_leftSelectPicker->SetTime( 0, 0, 0 );	 
	m_rightSelectPicker->SetTime( 0, 0, 0 );

	m_tool->NotifyCurveDisplayTimeChanged();
}

void CEdWorldEnvironmentToolPanel::UpdateWorldStatus()
{
	if ( GGame->GetActiveWorld() )
	{
		std::function<bool()> func;
		func = [&]() {
			// Try to set it up now
			CWorld* world = GGame->GetActiveWorld();
			if ( world != nullptr )
			{
				CEnvironmentManager* manager = world->GetEnvironmentManager();
				if ( manager != nullptr )
				{
					CWeatherManager* weatherManager = manager->GetWeatherManager();
					if ( m_pause != nullptr )
					{
						weatherManager->SetRunning( !m_pause->GetValue() );
						return true;
					}
				}
			}

			// Something is missing, try later
			RunLaterOnce( func );
			return false;
		};
		RunLaterOnce( func );
	}
}

void CEdWorldEnvironmentToolPanel::UpdateEnv()
{
	Int32 selection = XRCCTRL(*this, "m_envStdEnvChoice", wxChoice)->GetSelection();

	CEnvironmentManager* envman = GetEnvironmentManager();
	if ( envman )
	{
		envman->SwitchToPredefinedEnv( selection );
	}
}

void CEdWorldEnvironmentToolPanel::UpdateWeatherPresetsChoiceBox()
{
	// Get selected name
	wxString selectedName = XRCCTRL(*this, "envWeatherChoice", wxChoice)->GetStringSelection();
	if ( selectedName.IsEmpty() )
	{
		return;
	}

	// Get weather manager
	CEnvironmentManager* envManager = GetEnvironmentManager();
	if ( !envManager )
	{
		return;
	}
	CWeatherManager* weatherManager = envManager->GetWeatherManager();
	ASSERT( weatherManager, TXT("No weather manager in environment manager, that is impossible!") );

	// Request a weather change
	if ( !weatherManager->RequestWeatherChangeTo( CName( selectedName.wc_str() ) ) )
	{
		wxMessageBox( wxT("Failed to change the weather condition - check if the condition names are correct!"), wxT("Weather Failure"), wxICON_ERROR | wxOK );
	}
}

