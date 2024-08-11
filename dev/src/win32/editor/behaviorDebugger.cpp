/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorDebugger.h"
#include "behaviorEditor.h"
#include "../../common/engine/tagManager.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/game/itemIterator.h"


BEGIN_EVENT_TABLE( CEdBehaviorDebugger, CEdBehaviorEditorSimplePanel )
	EVT_BUTTON( XRCID( "histPrev" ), CEdBehaviorDebugger::OnHistPrev )
	EVT_BUTTON( XRCID( "histNext" ), CEdBehaviorDebugger::OnHistNext )
	EVT_BUTTON( XRCID( "histLast" ), CEdBehaviorDebugger::OnHistLast )
	EVT_BUTTON( XRCID( "btnUnlock" ), CEdBehaviorDebugger::OnUnsafeMode )
	EVT_TOGGLEBUTTON( XRCID( "btnForceTimeDelta" ), CEdBehaviorDebugger::OnForcedTimeDelta )
END_EVENT_TABLE()

CEdBehaviorDebugger::CEdBehaviorDebugger( CEdBehaviorEditor* editor )
	: CEdBehaviorEditorSimplePanel( editor )
	, m_connected( false )
	, m_selectedSnapshot( 0 )
	, m_selectedHistPage( HP_All )
	, m_listener( false )
	, m_toRelink( false )
	, m_toggledMilti( false )
	, m_forcedTimeDelta( 1.f / 30.f )
	, m_forcedTimeDeltaSet( false )
{
	// Load designed frame from resource
	wxPanel* innerPanel = wxXmlResource::Get()->LoadPanel( this, wxT("BehaviorEditorDebuggerPanel") );
	SetMinSize( innerPanel->GetSize() );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( innerPanel, 1, wxEXPAND|wxALL, 0 );

	m_conState = XRCCTRL( *this, "conState", wxStaticText );

	m_histList = XRCCTRL( *this, "histList", wxListBox );
	m_histEntityDesc = XRCCTRL( *this, "conEntDesc", wxStaticText );

	m_forcedTimeDeltaText = XRCCTRL( *this, "forcedTimeDeltaText", wxTextCtrl );

	SetSizer( sizer );	
	Layout();
}

CEdBehaviorDebugger::~CEdBehaviorDebugger()
{
	if ( m_toggledMilti )
	{
		GGame->GetGameplayConfig().m_animationMultiUpdate = true;
		m_toggledMilti = false;
	}

	if ( m_connected && GetBehaviorGraphInstance() && GetEditor()->IsDebugMode() && m_listener )
	{
		GetBehaviorGraphInstance()->RemoveEditorListener();
		m_listener = false;
	}
}

wxAuiPaneInfo CEdBehaviorDebugger::GetPaneInfo() const
{
	wxAuiPaneInfo info = CEdBehaviorEditorPanel::GetPaneInfo();

	info.Dockable( false ).Floatable( true ).Float().MinSize( GetMinSize() ).Dockable( false );

	return info;
}

void CEdBehaviorDebugger::OnReset()
{
	ClearHistList();

	m_debugTimerDuration = 0.f;
	m_debugTimer = 0.f;
}

void CEdBehaviorDebugger::OnDebug( Bool flag )
{
	Connect( flag );
}

void CEdBehaviorDebugger::OnTick( Float dt )
{
	if ( m_connected )
	{
		UpdateHistList();
	}
}

void CEdBehaviorDebugger::OnClose()
{
	if ( m_connected && GetBehaviorGraphInstance() && GetEditor()->IsDebugMode() && m_listener )
	{
		GetBehaviorGraphInstance()->RemoveEditorListener();
		m_listener = false;
	}
}

void CEdBehaviorDebugger::OnLoadEntity()
{
	if ( m_connected && GetEditor()->IsDebugMode() )
	{
		GetBehaviorGraphInstance()->SetEditorListener( this );
		m_listener = true;
	}
}

void CEdBehaviorDebugger::OnUnloadEntity()
{
	if ( m_connected && GetBehaviorGraphInstance() && GetEditor()->IsDebugMode() && m_listener )
	{
		GetBehaviorGraphInstance()->RemoveEditorListener();
		m_listener = false;
	}
}

void CEdBehaviorDebugger::Connect( Bool flag )
{
	m_connected = flag;

	if ( m_connected )
	{
		m_conState->SetLabel( wxT("State: Connected") );

		if ( GGame->GetGameplayConfig().m_animationMultiUpdate )
		{
			m_toggledMilti = true;
			GGame->GetGameplayConfig().m_animationMultiUpdate = false;
		}
	}
	else
	{
		if ( m_toggledMilti )
		{
			GGame->GetGameplayConfig().m_animationMultiUpdate = true;
			m_toggledMilti = false;
		}

		m_conState->SetLabel( wxT("State: Disconnected") );
	}

	SetEntityDesc( m_connected );
}

void CEdBehaviorDebugger::SetEntityDesc( Bool flag )
{
	if ( flag )
	{
		String str = String::Printf( TXT("Entity '%s', component '%s'"), GetEntity()->GetName().AsChar(), GetAnimatedComponent()->GetName().AsChar() );

		CAnimatedComponent* ac = GetAnimatedComponent();
		if ( ac->UseExtractedMotion() )
		{
			str += TXT(" - motion on");
		}
		else
		{
			str += TXT(" - motion off");
		}

		m_histEntityDesc->SetLabel( str.AsChar() );
	}
	else
	{
		m_histEntityDesc->SetLabel( wxT("Empty") );
	}
}

void CEdBehaviorDebugger::ClearHistList()
{
	m_histList->Clear();
}

void CEdBehaviorDebugger::UpdateHistList()
{
	m_histList->Freeze();
	m_histList->Clear();

	/*SSnapshot& snapshot = GetSnapshot();

	if ( m_selectedHistPage == HP_All )
	{
		Uint32 msgSize = snapshot.m_messages.Size();

		for ( Uint32 i=0; i<msgSize; ++i )
		{
			m_histList->AppendString( snapshot.m_messages[i].AsChar() );
		}
	}*/

	m_histList->Thaw();
	//Refresh();
}

Bool CEdBehaviorDebugger::CanEditVariables() const
{
	return XRCCTRL( *this, "debugEditVar", wxCheckBox )->GetValue();
}

void CEdBehaviorDebugger::OnHistPrev( wxCommandEvent& event )
{	
	//m_selectedSnapshot = Clamp< Uint32 >( m_selectedSnapshot+1, 0, GetSnaphotNum() );
	UpdateHistList();
}

void CEdBehaviorDebugger::OnHistNext( wxCommandEvent& event )
{
	//m_selectedSnapshot = Clamp< Uint32 >( m_selectedSnapshot-1, 0, GetSnaphotNum() );
	UpdateHistList();
}

void CEdBehaviorDebugger::OnHistLast( wxCommandEvent& event )
{
	//m_selectedSnapshot = GetSnaphotNum();
	UpdateHistList();
}

void CEdBehaviorDebugger::OnUnsafeMode( wxCommandEvent& event )
{
	GetEditor()->ToggleDebuggerUnsafeMode();
}

void CEdBehaviorDebugger::OnForcedTimeDelta( wxCommandEvent& event )
{
	m_forcedTimeDeltaSet = event.IsChecked();
}

void CEdBehaviorDebugger::InternalUpdate( Float dt )
{
	m_debugTimer += dt;

	if ( m_debugTimer > m_debugTimerDuration )
	{
		m_debugTimer = 0.f;

		GetEditor()->Tick( dt );
		GetEditor()->Refresh();
	}
}

void CEdBehaviorDebugger::OnPostUpdateInstance( Float dt )
{
	//if ( !GGame->GetGameplayConfig().m_animationMultiUpdate )
	{
		InternalUpdate( dt );
	}
}

void CEdBehaviorDebugger::OnPreUpdateInstance( Float& dt )
{
	if ( m_forcedTimeDeltaSet )
	{
		String str = m_forcedTimeDeltaText->GetLabelText().wc_str();
		Float newDt = 1.f / 30.f;

		FromString( str, newDt );

		m_forcedTimeDelta = newDt;

		dt = m_forcedTimeDelta;
	}
}

void CEdBehaviorDebugger::OnPostSampleInstance( const SBehaviorGraphOutput& pose )
{
	//if ( !GGame->GetGameplayConfig().m_animationMultiUpdate )
	{
		GetEditor()->OnDebuggerPostSamplePose( pose );
	}
}

void CEdBehaviorDebugger::OnUnbind()
{
	if ( CBehaviorGraphInstance* instance = GetBehaviorGraphInstance() )
	{
		instance->RemoveEditorListener();
	}

	if ( !m_toRelink )
	{
		m_listener = false;
		GetEditor()->Destroy(); // Destroy instead of Close, to force it to be immediate
	}
}

Bool CEdBehaviorDebugger::CanBeRelinked() const
{
	ASSERT( !m_toRelink );
	return !m_toRelink;
}

void CEdBehaviorDebugger::RequestToRelink()
{
	m_toRelink = true;
}

void CEdBehaviorDebugger::Relink( CBehaviorGraphInstance* newInstance )
{
	GetEditor()->RelinkToInstance( newInstance );
	m_toRelink = false;
}

//////////////////////////////////////////////////////////////////////////

#define ID_DEBUG_TIMER 23456

BEGIN_EVENT_TABLE( CEdBehaviorsListener, wxFrame )
	EVT_MENU( XRCID( "buttFind" ), CEdBehaviorsListener::OnButtFind )
	EVT_MENU( XRCID( "buttShow" ), CEdBehaviorsListener::OnButtShow )
	EVT_MENU( XRCID( "buttConnect" ), CEdBehaviorsListener::OnButtConnect )
	EVT_MENU( XRCID( "buttUpdate" ), CEdBehaviorsListener::OnManualUpdateLog )
	EVT_TIMER( ID_DEBUG_TIMER , CEdBehaviorsListener::OnUpdateLog )
	EVT_TEXT_ENTER( XRCID( "editDuration" ), CEdBehaviorsListener::OnEditDuration )
	EVT_MENU( XRCID( "buttSkeleton" ), CEdBehaviorsListener::OnShowSkeleton )
END_EVENT_TABLE()

CEdBehaviorsListener::CEdBehaviorsListener( wxWindow* parent )
	: m_updateTimer( this, ID_DEBUG_TIMER )
{
	wxXmlResource::Get()->LoadFrame( this, parent, TXT("BehaviorGroupDebugger") );

	SetTitle( wxT("Behaviors Listener") );

	m_entityList = XRCCTRL( *this, "entityList", wxListBox );

	// Notebook
	{
		wxPanel* panel = XRCCTRL( *this, "notebookPanel", wxPanel );
		wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

		m_notebook = new wxAuiNotebook( panel, -1 , wxDefaultPosition, wxDefaultSize, wxAUI_NB_SCROLL_BUTTONS|wxAUI_NB_TAB_MOVE|wxAUI_NB_TAB_SPLIT );
		sizer->Add( m_notebook, 1, wxALL|wxEXPAND, 5 );

		panel->SetSizer( sizer );
	}

	Layout();
}

CEdBehaviorsListener::~CEdBehaviorsListener()
{
	m_updateTimer.Stop();
	ClearAll();
}

void CEdBehaviorsListener::ClearAll()
{
	ClearLoggers();

	// Clear list
	m_entityList->Clear();
}

void CEdBehaviorsListener::ClearLoggers()
{
	for ( Int32 i=(Int32)m_loggers.Size()-1; i>=0; --i )
	{
		CBehaviorGraphInstance* instance = m_loggers[i].m_instance.Get();

		if ( instance )
		{
			ASSERT( instance->GetEditorListener() == m_loggers[i].m_listener );
			instance->RemoveEditorListener();
		}

		delete m_loggers[i].m_listener;
	}
	m_loggers.Clear();

	RemovePages();
}

void CEdBehaviorsListener::AddLogger( CBehaviorGraphInstance* instance, const String& name )
{
	SDebugLogger logger;
	logger.m_instance = instance;
	logger.m_listener = new CBehaviorGraphSimpleLogger( instance );

	logger.m_instance.Get()->SetEditorListener( logger.m_listener );

	logger.m_page = AddPage( name, logger.m_grid );

	m_loggers.PushBack( logger );
}

Int32 CEdBehaviorsListener::AddPage( const String& name, wxGrid*& gridOut )
{
	wxPanel* panel = new wxPanel( m_notebook, -1 );
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

	gridOut = new wxGrid( panel, -1 );
	SetupGrid( gridOut );
	sizer->Add( gridOut, 1, wxALL|wxEXPAND, 5 );

	panel->SetSizer( sizer );

	Int32 page = m_notebook->AddPage( panel, name.AsChar() );
	ASSERT( page >= 0 );

	return page;
}

void CEdBehaviorsListener::RemovePages()
{
	Int32 size = m_notebook->GetPageCount();

	for ( Int32 i=size-1; i>=0; --i )
	{
		m_notebook->DeletePage( i );
	}

	ASSERT( m_notebook->GetPageCount() == 0 );
}

void CEdBehaviorsListener::OnButtFind( wxCommandEvent& event )
{
	// Very slow...

	if ( !GGame || !GGame->GetPlayerEntity() )
	{
		wxMessageBox( wxT("Game is not active"), wxT("Info"), 4|wxCENTRE, this );
		return;
	}

	Vector playerPos = GGame->GetPlayerEntity()->GetWorldPosition();
	Float radius = GetRadius();
	Float distMax = radius * radius;

	TDynArray< CEntity* > entities;

	for ( ObjectIterator<CAnimatedComponent> it; it; ++it )
	{
		CAnimatedComponent* ac = (*it);
		Vector pos = ac->GetWorldPosition();

		if ( ac->IsAttached() && ac->GetEntity() && pos.DistanceSquaredTo( playerPos ) < distMax )
		{
			entities.PushBackUnique( ac->GetEntity() );
		}
	}

	ClearAll();
	FillList( entities );
}

void CEdBehaviorsListener::FillList( const TDynArray< CEntity* >& entities )
{
	m_entityList->Freeze();
	m_entityList->Clear();

	for ( Uint32 i=0; i<entities.Size(); ++i )
	{
		CEntity* ent = entities[i];
		CAnimatedComponent* ac = ent->GetRootAnimatedComponent();

		if ( !ac || !ac->GetBehaviorStack() )
		{
			continue;
		}
	
		TDynArray< CName > insts;
		ac->GetBehaviorStack()->GetInstances( insts );

		for ( Uint32 j=0; j<insts.Size(); ++j )
		{
			CBehaviorGraphInstance* instance = ac->GetBehaviorStack()->GetBehaviorGraphInstance( insts[j] );
			if ( instance )
			{
				String str = entities[i]->GetName() + TXT("-") + instance->GetInstanceName().AsString();
				m_entityList->Append( str.AsChar(), new CListElemData( instance ) );
			}
		}
	}

	m_entityList->Thaw();
}

Int32 CEdBehaviorsListener::GetUpdateTimeStep() const
{
	String str = XRCCTRL( *this, "editDt", wxTextCtrl )->GetValue().wc_str();
	Float dt = 0.f;
	Bool ret = FromString( str, dt );
	
	if ( !ret )
	{
		return 1;
	}

	Int32 ms = Max( ( Int32 )( dt * 1000.f ), 1 );
	return ms;
}

void CEdBehaviorsListener::OnButtConnect( wxCommandEvent& event )
{
	if ( event.IsChecked() )
	{
		m_updateTimer.Start( GetUpdateTimeStep(), false );
	}
	else
	{
		m_updateTimer.Stop();
	}
}

void CEdBehaviorsListener::OnButtShow( wxCommandEvent& event )
{
	m_notebook->Freeze();

	ClearLoggers();

	wxArrayInt selected;
	m_entityList->GetSelections( selected );

	for ( size_t i=0; i<selected.GetCount(); ++i )
	{
		Int32 elem = selected[i];

		CListElemData* data = static_cast< CListElemData* >( m_entityList->GetClientObject( elem ) );
		CBehaviorGraphInstance* instance = data->m_instance.Get();

		if ( instance )
		{
			AddLogger( instance, m_entityList->GetString( elem ).wc_str() );
		}
	}

	OnEditDuration( wxCommandEvent() );

	m_notebook->Thaw();
	Refresh();
}

Float CEdBehaviorsListener::GetRadius() const
{
	String radiusStr = XRCCTRL( *this, "editRadius", wxTextCtrl )->GetValue().wc_str();
	Float radius = 0.f;
	Bool ret = FromString( radiusStr, radius );
	return ret ? radius : 0.f;
}

void CEdBehaviorsListener::SetupGrid( wxGrid* grid )
{
	grid->Freeze();

	grid->CreateGrid( LIST_DEFAULT_SIZE, CBehaviorGraphSimpleLogger::MC_Last );

	grid->SetColLabelValue( CBehaviorGraphSimpleLogger::MC_Events, wxT("Events") );
	grid->SetColLabelValue( CBehaviorGraphSimpleLogger::MC_Variables, wxT("Variables") );
	grid->SetColLabelValue( CBehaviorGraphSimpleLogger::MC_Notifications, wxT("Notifications") );
	grid->SetColLabelValue( CBehaviorGraphSimpleLogger::MC_States, wxT("States") );
	grid->SetColLabelValue( CBehaviorGraphSimpleLogger::MC_Instance, wxT("Instance") );

	grid->EnableEditing( false );
	grid->SetRowLabelSize( 0 );

	grid->EnableDragRowSize( false );
	grid->EnableDragColMove( false );

	grid->Thaw();
}

namespace
{
	unsigned char ColorLerp( unsigned char x, Float p )
	{
		return ( unsigned char )( 255 + ( x - 255 ) * p );
	}
};

wxColour CEdBehaviorsListener::GetMsgColor( Uint32 code, Float progress ) const
{
	unsigned char r = 0;
	unsigned char g = 0;
	unsigned char b = 0;

	switch ( code )
	{
	case CBehaviorGraphSimpleLogger::COLOR_DEFAULT:
		break;
	case CBehaviorGraphSimpleLogger::COLOR_ON:
		g = 255;
		break;
	case CBehaviorGraphSimpleLogger::COLOR_OFF:
		r = 255;
		break;
	case CBehaviorGraphSimpleLogger::COLOR_EVENT_ANIM:
		b = 255;
		break;
	case CBehaviorGraphSimpleLogger::COLOR_EVENT:
		g = 255;
		break;
	case CBehaviorGraphSimpleLogger::COLOR_EVENT_FORCE:
		r = 255;
		break;
	case CBehaviorGraphSimpleLogger::COLOR_VAR_FLOAT:
		break;
	case CBehaviorGraphSimpleLogger::COLOR_VAR_VECTOR:
		b = 255;
		break;
	}

	r = ColorLerp( r, progress );
	g = ColorLerp( g, progress );
	b = ColorLerp( b, progress );

	return wxColour( r,g,b );
}
 
 void CEdBehaviorsListener::OnUpdateLog( wxTimerEvent& event )
 {
 	if ( m_loggers.Size() > 0 )
 	{
 		Int32 newInter = GetUpdateTimeStep();
 
 		if ( m_updateTimer.GetInterval() != newInter )
 		{
 			m_updateTimer.Stop();
 			m_updateTimer.Start( newInter, false );
 		}
 
 		UpdateLoggers();
 	}
 }

void CEdBehaviorsListener::OnManualUpdateLog( wxCommandEvent& event )
{
	if ( m_loggers.Size() > 0 )
	{
		UpdateLoggers();
	}
}

void CEdBehaviorsListener::OnEditDuration( wxCommandEvent& event )
{
	String durStr = XRCCTRL( *this, "editDuration", wxTextCtrl )->GetValue().wc_str();
	Float dur;
	Bool ret = FromString( durStr, dur );

	if ( ret )
	{
		for ( Uint32 i=0; i<m_loggers.Size(); ++i )
		{
			m_loggers[i].m_listener->SetMsgDuration( dur );
		}
	}
}

void CEdBehaviorsListener::OnShowSkeleton( wxCommandEvent& event )
{
	Bool disp = event.IsChecked();

	for ( Uint32 i=0; i<m_loggers.Size(); ++i )
	{
		if ( m_loggers[i].m_instance.Get() )
		{
			CAnimatedComponent* ac = m_loggers[i].m_instance.Get()->GetAnimatedComponentUnsafe();
			ac->SetDispSkeleton( ACDD_SkeletonBone ,disp );
		}
	}
}

void CEdBehaviorsListener::UpdateLoggers()
{
	for ( Uint32 i=0; i<m_loggers.Size(); ++i )
	{
		m_loggers[i].m_grid->Freeze();
	}

	for ( Uint32 i=0; i<m_loggers.Size(); ++i )
	{
		const Float duration = m_loggers[i].m_listener->GetMsgDuration();

		wxGrid* grid = m_loggers[i].m_grid;
		grid->ClearGrid();

		if ( !m_loggers[i].m_instance.Get() )
		{
			continue;
		}

		CBehaviorGraphSimpleLogger* log = m_loggers[i].m_listener;
		Uint32 size = CBehaviorGraphSimpleLogger::MC_Last;

		for ( Uint32 j=0; j<size; ++j )
		{
			const TDynArray< CBehaviorGraphSimpleLogger::SMessage >& messageArray = log->GetMessages( (CBehaviorGraphSimpleLogger::EMessageCategory)j );
			
			Uint32 msgSize = (Int32)Min( LIST_DEFAULT_SIZE, messageArray.Size() );
			for ( Uint32 k=0; k<msgSize; ++k )
			{
				const  CBehaviorGraphSimpleLogger::SMessage& msg = messageArray[k];
				
				wxString label = msg.m_text.AsChar();
				wxColour color = GetMsgColor( msg.m_color, msg.m_time / duration );
				
				Uint32 row = msgSize - k - 1;

				grid->SetCellValue( row, j, label );
				grid->SetCellTextColour( row, j, color );
			}
		}
	}

	for ( Uint32 i=0; i<m_loggers.Size(); ++i )
	{
		m_loggers[i].m_grid->Thaw();
	}
}

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( CEdBehaviorDebuggerInstanceSelectionDlg, wxDialog )
	EVT_BUTTON( XRCID("buttonOk"), CEdBehaviorDebuggerInstanceSelectionDlg::OnOk )
	EVT_BUTTON( XRCID("buttCancel"), CEdBehaviorDebuggerInstanceSelectionDlg::OnCancel )
	EVT_BUTTON( XRCID("buttSearch"), CEdBehaviorDebuggerInstanceSelectionDlg::OnSearch )
	EVT_CHOICE( XRCID("choiceComponent"), CEdBehaviorDebuggerInstanceSelectionDlg::OnComponentSelected )
	EVT_CHOICE( XRCID("choiceInstance"), CEdBehaviorDebuggerInstanceSelectionDlg::OnInstanceSelected )
END_EVENT_TABLE()

CEdBehaviorDebuggerInstanceSelectionDlg::CEdBehaviorDebuggerInstanceSelectionDlg( wxWindow* parent, CEntity* entity )
	: m_entity( entity )
{
	// Load designed frame from resource
	wxXmlResource::Get()->LoadDialog( this, parent, TXT("BehaviorGraphDebuggerSelection") );

	m_iconValid = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CHECKED_OUT") );
	m_iconInvalid = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_MARKED_DELETE") );

	if ( entity )
	{
		XRCCTRL( *this, "buttSearch", wxButton )->Enable( false );

		wxStaticBitmap* bitmap = XRCCTRL( *this, "imgOk", wxStaticBitmap );
		bitmap->SetBitmap( m_iconValid );
	}

	LoadConfig();

	FillDefaultValues();
}

Int32 CEdBehaviorDebuggerInstanceSelectionDlg::DoModal()
{
	return wxDialog::ShowModal();
}

void CEdBehaviorDebuggerInstanceSelectionDlg::OnOk( wxCommandEvent& event )
{
	SaveConfig();

	EndDialog( wxID_OK );
}

void CEdBehaviorDebuggerInstanceSelectionDlg::OnCancel( wxCommandEvent& event )
{
	EndDialog( wxID_CANCEL );
}

void CEdBehaviorDebuggerInstanceSelectionDlg::OnSearch( wxCommandEvent& event )
{
	// Get input data
	CName tag = GetEntityTag();
	String name = GetEntityName();

	// Reset
	m_componentName = String::EMPTY;
	m_instanceName = CName::NONE;
	
	// Find entity
	if ( tag != CName::NONE && GGame && GGame->IsActive() && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetTagManager() )
	{
		TDynArray< CEntity* > entities;
		GGame->GetActiveWorld()->GetTagManager()->CollectTaggedEntities( tag, entities );

		m_entity = entities.Size() > 0 ? entities[0] : NULL;

		if ( m_entity && name.Empty() == false )
		{
			for ( Uint32 i=0; i<entities.Size(); ++i )
			{
				if ( entities[i]->GetName() == name )
				{
					m_entity = entities[i];
					break;
				}
			}
		}
	}
	else if ( name.Empty() == false && GGame && GGame->IsActive() && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetDynamicLayer() )
	{
		// Dynamic layer
		m_entity = GGame->GetActiveWorld()->GetDynamicLayer()->FindEntity( name );

		// All
		if ( m_entity == NULL )
		{
			TDynArray< CEntity* > entities;
			CollectAllEntities( GGame->GetActiveWorld(), entities );

			for ( Uint32 i=0; i<entities.Size(); ++i )
			{
				if ( entities[i]->GetName() == name )
				{
					m_entity = entities[i];
					break;
				}
			}
		}
	}

	// Fill
	FillDefaultValues();
}

namespace
{
	CAnimatedComponent* FindComponentByName( CEntity* entity, const String& name )
	{
		CAnimatedComponent* ac( nullptr );

		if ( CActor* a = Cast< CActor >( entity ) )
		{
			for ( EntityWithItemsComponentIterator< CAnimatedComponent > it( a ); it; ++it )
			{
				if ( (*it)->GetName() == name )
				{
					ac = *it;
				}
			}
		}
		else
		{
			ac = entity->FindComponent< CAnimatedComponent >( name );
		}

		return ac;
	}
}

CBehaviorGraphInstance* CEdBehaviorDebuggerInstanceSelectionDlg::GetSelectedInstance() const
{
	if ( m_entity )
	{
		String compName = GetComponent();

		CAnimatedComponent* ac = FindComponentByName( m_entity, compName );

		if ( ac && ac->GetBehaviorStack() )
		{
			return ac->GetBehaviorStack()->GetBehaviorGraphInstance( GetSelectedInstanceName() );
		}
	}

	return NULL;
}

CName CEdBehaviorDebuggerInstanceSelectionDlg::GetSelectedInstanceName() const
{
	return m_instanceName;
}

String CEdBehaviorDebuggerInstanceSelectionDlg::GetComponent() const
{
	return m_componentName;
}

CName CEdBehaviorDebuggerInstanceSelectionDlg::GetEntityTag() const
{
	wxString str = XRCCTRL( *this, "editTag", wxTextCtrl )->GetValue();
	String text = str.wc_str();
	return CName( text );
}

String CEdBehaviorDebuggerInstanceSelectionDlg::GetEntityName() const
{
	return String( XRCCTRL( *this, "editName", wxTextCtrl )->GetValue().wc_str() );
}

void CEdBehaviorDebuggerInstanceSelectionDlg::OnComponentSelected( wxCommandEvent& event )
{
	wxChoice* choiceComp = XRCCTRL( *this, "choiceComponent", wxChoice );

	m_componentName = choiceComp->GetStringSelection().wc_str();

	FillInstanceList();

	RefreshOkButton();
}

void CEdBehaviorDebuggerInstanceSelectionDlg::OnInstanceSelected( wxCommandEvent& event )
{
	wxChoice* choiceInst = XRCCTRL( *this, "choiceInstance", wxChoice );

	m_instanceName = CName( choiceInst->GetStringSelection().wc_str() );

	RefreshOkButton();
}

void CEdBehaviorDebuggerInstanceSelectionDlg::FillDefaultValues()
{
	wxChoice* choiceComp = XRCCTRL( *this, "choiceComponent", wxChoice );
	wxChoice* choiceInst = XRCCTRL( *this, "choiceInstance", wxChoice );

	Bool enabled = m_entity != NULL;

	FillComponentList();
	FillInstanceList();

	wxStaticBitmap* bitmap = XRCCTRL( *this, "imgOk", wxStaticBitmap );
	if ( enabled )
	{
		bitmap->SetBitmap( m_iconValid );
	}
	else
	{
		bitmap->SetBitmap( m_iconInvalid );
	}
	
	choiceComp->Enable( enabled );
	choiceInst->Enable( enabled );

	RefreshOkButton();
}

void CEdBehaviorDebuggerInstanceSelectionDlg::RefreshOkButton()
{
	wxButton* buttOk = XRCCTRL( *this, "buttonOk", wxButton );

	if ( m_entity && GetSelectedInstanceName() != CName::NONE && GetComponent() != String::EMPTY )
	{
		buttOk->Enable( true );
	}
	else
	{
		buttOk->Enable( false );
	}
}

void CEdBehaviorDebuggerInstanceSelectionDlg::FillComponentList()
{
	wxChoice* choiceComp = XRCCTRL( *this, "choiceComponent", wxChoice );
	
	choiceComp->Freeze();
	choiceComp->Clear();

	if ( m_entity )
	{
		TDynArray< CAnimatedComponent* > comps;

		if ( CActor* a = Cast< CActor >( m_entity ) )
		{
			for ( EntityWithItemsComponentIterator< CAnimatedComponent > it( a ); it; ++it )
			{
				comps.PushBack( *it );
			}
		}
		else
		{
			CollectEntityComponents( m_entity, comps );
		}

		for ( Uint32 i=0; i<comps.Size(); ++i )
		{
			choiceComp->AppendString( comps[i]->GetName().AsChar() );
		}

		if ( m_entity->GetRootAnimatedComponent() )
		{
			choiceComp->SetStringSelection( m_entity->GetRootAnimatedComponent()->GetName().AsChar() );
			m_componentName = choiceComp->GetStringSelection().wc_str();
		}
	}

	choiceComp->Thaw();
}

void CEdBehaviorDebuggerInstanceSelectionDlg::FillInstanceList()
{
	wxChoice* choiceInst = XRCCTRL( *this, "choiceInstance", wxChoice );

	choiceInst->Freeze();
	choiceInst->Clear();

	if ( m_entity )
	{
		String selCompName = GetComponent();

		if ( selCompName.Empty() == false )
		{
			CAnimatedComponent* ac = FindComponentByName( m_entity, selCompName );
			if ( ac )
			{
				if ( ac->GetBehaviorStack() )
				{
					TDynArray< CName > instances;
					ac->GetBehaviorStack()->GetInstances( instances );

					for ( Uint32 i=0; i<instances.Size(); ++i )
					{
						choiceInst->AppendString( instances[i].AsString().AsChar() );
					}

					if ( m_instanceName != CName::NONE && choiceInst->FindString( m_instanceName.AsString().AsChar() ) != -1 )
					{
						choiceInst->SetStringSelection( m_instanceName.AsString().AsChar() );
					}
					else
					{
						CName currInst = ac->GetBehaviorStack()->GetActiveBottomInstance();
						choiceInst->SetStringSelection( currInst.AsString().AsChar() );
					}

					m_instanceName = CName( choiceInst->GetStringSelection().wc_str() );
				}
			}
			else
			{
				ASSERT( ac );
			}
		}
		else
		{
			ASSERT( selCompName.Empty() == false );
		}
	}

	choiceInst->Thaw();
}

void CEdBehaviorDebuggerInstanceSelectionDlg::LoadConfig()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/BehaviorDebuggerInstanceSelection") );

	String tag = config.Read( TXT("Tag"), String::EMPTY );
	String name = config.Read( TXT("Name"), String::EMPTY );

	if ( tag.Empty() == false )
	{
		XRCCTRL( *this, "editTag", wxTextCtrl )->SetLabel( tag.AsChar() );
	}

	if ( name.Empty() == false )
	{
		XRCCTRL( *this, "editName", wxTextCtrl )->SetLabel( name.AsChar() );
	}
}

void CEdBehaviorDebuggerInstanceSelectionDlg::SaveConfig()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/BehaviorDebuggerInstanceSelection") );

	config.Write( TXT("Tag"), GetEntityTag().AsString() );
	config.Write( TXT("Name"), GetEntityName() );
}
