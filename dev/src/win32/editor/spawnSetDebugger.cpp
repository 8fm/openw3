/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "spawnSetDebugger.h"

#if 0

// Event table
BEGIN_EVENT_TABLE( CSpawnSetDebugger, wxSmartLayoutPanel )
	// EVT_CLOSE( CSpawnSetDebugger::OnClose )
END_EVENT_TABLE()

CSpawnSetDebugger::CSpawnSetDebugger( wxWindow* parent )
	: wxSmartLayoutPanel( parent, TXT("SpawnSetsDebugger"), true )
	, m_idStartTool( XRCID("startTool") )
	, m_idStopTool( XRCID("stopTool") )
{
	// Get GUI elements
	m_toolBar                 = XRCCTRL( *this, "toolBar", wxToolBar );
	m_spawnSetListBox         = XRCCTRL( *this, "num1ListBox", wxListBox );
	m_spawnSetEntryListBox    = XRCCTRL( *this, "num2ListBox", wxListBox );
	m_actorsListBox           = XRCCTRL( *this, "num3ListBox", wxListBox );
	m_timersListBox           = XRCCTRL( *this, "num4ListBox", wxListBox );
	m_spawnInfoListBox        = XRCCTRL( *this, "num5ListBox", wxListBox );
	m_infoTextCtrl            = XRCCTRL( *this, "infoTextControl", wxTextCtrl );
	m_label1StaticText        = XRCCTRL( *this, "label1staticText", wxStaticText );
	m_label2StaticText        = XRCCTRL( *this, "label2staticText", wxStaticText );
	m_label3StaticText        = XRCCTRL( *this, "label3staticText", wxStaticText );
	m_label4StaticText        = XRCCTRL( *this, "label4staticText", wxStaticText );
	m_label5StaticText        = XRCCTRL( *this, "label5staticText", wxStaticText );
	m_label6StaticText        = XRCCTRL( *this, "label6staticText", wxStaticText );
	
	m_toolBar->Connect( wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CSpawnSetDebugger::OnToolBar ), 0, this );
	m_spawnSetListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED , wxCommandEventHandler( CSpawnSetDebugger::OnSpawnSetListBoxItemSelected ), NULL, this );
	m_spawnSetListBox->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( CSpawnSetDebugger::OnSpawnSetListBoxItemDoubleClicked ), NULL, this );
	m_spawnSetEntryListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED , wxCommandEventHandler( CSpawnSetDebugger::OnSpawnSetEntryListBoxItemSelected ), NULL, this );
	m_spawnSetEntryListBox->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( CSpawnSetDebugger::OnSpawnSetEntryListBoxItemDoubleClicked ), NULL, this );
	m_actorsListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED , wxCommandEventHandler( CSpawnSetDebugger::OnActorsListBoxItemSelected ), NULL, this );
	m_actorsListBox->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( CSpawnSetDebugger::OnActorsListBoxItemDoubleClicked ), NULL, this );

	m_label1StaticText->SetLabel( TXT("Spawnsets") );
	m_label2StaticText->SetLabel( TXT("Spawnsets entries") );
	m_label3StaticText->SetLabel( TXT("Actors") );
	m_label4StaticText->SetLabel( TXT("Timers") );
	m_label5StaticText->SetLabel( TXT("Spawn info") );
	m_label6StaticText->SetLabel( TXT("Actor and spawnset properties") );

	m_toolBar->EnableTool( m_idStartTool, true );
	m_toolBar->EnableTool( m_idStopTool, false );

	// Set timer
	m_timer = new CEdTimer();
	m_timer->Connect( wxEVT_TIMER, wxCommandEventHandler( CSpawnSetDebugger::OnTimer ), NULL, this );
	m_timer->Stop();

	// Update and finalize layout
	Layout();
	Show();
	LoadOptionsFromConfig();
}

CSpawnSetDebugger::~CSpawnSetDebugger()
{
	SaveOptionsToConfig();

	delete m_timer;
}

void CSpawnSetDebugger::OnToolBar( wxCommandEvent &event )
{
	const int idSelected = event.GetId();

	if ( idSelected == m_idStartTool )
	{
		const int refreshFactor = 400; // milliseconds
		m_timer->Start( refreshFactor );
		m_toolBar->EnableTool( m_idStartTool, false );
		m_toolBar->EnableTool( m_idStopTool, true );
	}
	else if ( idSelected == m_idStopTool )
	{
		m_timer->Stop();
		m_toolBar->EnableTool( m_idStartTool, true );
		m_toolBar->EnableTool( m_idStopTool, false );
	}
}

void CSpawnSetDebugger::OnTimer( wxCommandEvent& event )
{
	if ( !GGame->IsActive() ) return;

	int indexSpawnSet = m_spawnSetListBox->GetSelection();
	int indexSpawnSetEntry = m_spawnSetEntryListBox->GetSelection();
	int indexActor = m_actorsListBox->GetSelection();

	ProcessSpawnSetComponents();
	if ( indexSpawnSet != wxNOT_FOUND ) ProcessSpawnSetEntries( indexSpawnSet );
	if ( indexSpawnSet != wxNOT_FOUND && indexSpawnSetEntry != wxNOT_FOUND ) 
	{
		ProcessActors( indexSpawnSet, indexSpawnSetEntry );
		ProcessTimers( indexSpawnSet, indexSpawnSetEntry );
	}
	if ( indexActor != wxNOT_FOUND )
	{
		ProcessActor( indexSpawnSet, indexActor );
	}

	UpdateGUIData();
}

void CSpawnSetDebugger::OnSpawnSetListBoxItemDoubleClicked( wxCommandEvent &event )
{

}

void CSpawnSetDebugger::UpdateGUIData()
{
	UpdateListBox( m_spawnSetListBox, m_spawnSetListBoxData );
	UpdateListBox( m_spawnSetEntryListBox, m_spawnSetEntryListBoxData );
	UpdateListBox( m_actorsListBox, m_actorsListBoxData );
	UpdateListBox( m_timersListBox, m_timersListBoxData );
	UpdateListBox( m_spawnInfoListBox, m_spawnInfoListBoxData );
	UpdateTextCtrl( m_infoTextCtrl, m_infoTextCtrlData );
}

void CSpawnSetDebugger::UpdateListBox( wxListBox *listBox, const wxArrayString &data )
{
	const wxArrayString &dataCurrent = listBox->GetStrings();
	if ( dataCurrent != data )
	{
		int oldSelection = listBox->GetSelection(); // remember old selection and then try to restore it
		listBox->Clear();
		listBox->Append( data );
		if ( oldSelection < static_cast< int >( listBox->GetCount() ) )
		{
			listBox->SetSelection( oldSelection );
		}
	}
}

void CSpawnSetDebugger::UpdateTextCtrl( wxTextCtrl *listCtrl, const wxString &data )
{
	const wxString &textValueCurrent = listCtrl->GetValue();
	if ( textValueCurrent != data )
	{
		listCtrl->Clear();
		listCtrl->SetValue( data );
	}
}

void CSpawnSetDebugger::OnSpawnSetListBoxItemSelected( wxCommandEvent &event )
{
	int index = m_spawnSetListBox->GetSelection();
	if ( index == wxNOT_FOUND ) return;

	m_actorsListBoxData.Clear();
	m_actorsListBoxDataNPCControls.Clear();

	ProcessSpawnSetEntries( index );

	UpdateListBox( m_spawnSetEntryListBox, m_spawnSetEntryListBoxData );
	UpdateListBox( m_spawnInfoListBox, m_spawnInfoListBoxData );
	UpdateListBox( m_actorsListBox, m_actorsListBoxData );
}

void CSpawnSetDebugger::OnSpawnSetEntryListBoxItemSelected( wxCommandEvent &event )
{
	int indexSpawnSet = m_spawnSetListBox->GetSelection();
	if ( indexSpawnSet == wxNOT_FOUND ) return;

	int indexSpawnSetEntry = m_spawnSetEntryListBox->GetSelection();
	if ( indexSpawnSetEntry == wxNOT_FOUND ) return;
	
	ProcessActors( indexSpawnSet, indexSpawnSetEntry );
	UpdateListBox( m_actorsListBox, m_actorsListBoxData );

	ProcessTimers( indexSpawnSet, indexSpawnSetEntry );
	UpdateListBox( m_timersListBox, m_timersListBoxData );
}

void CSpawnSetDebugger::OnSpawnSetEntryListBoxItemDoubleClicked( wxCommandEvent &event )
{

}

void CSpawnSetDebugger::OnActorsListBoxItemSelected( wxCommandEvent &event )
{
	int indexSpawnSet = m_spawnSetListBox->GetSelection();
	if ( indexSpawnSet == wxNOT_FOUND ) return;

	int indexActor = m_actorsListBox->GetSelection();
	if ( indexActor == wxNOT_FOUND ) return;

	ProcessSpawnSetEntries( indexSpawnSet );
	ProcessActor( indexSpawnSet, indexActor );
	UpdateTextCtrl( m_infoTextCtrl, m_infoTextCtrlData );
}

void CSpawnSetDebugger::OnActorsListBoxItemDoubleClicked( wxCommandEvent &event )
{
	int indexSpawnSet = m_spawnSetListBox->GetSelection();
	if ( indexSpawnSet == wxNOT_FOUND ) return;

	int indexActor = m_actorsListBox->GetSelection();
	if ( indexActor == wxNOT_FOUND ) return;

	if ( indexSpawnSet >= (int)CNPCSpawnSetComponent::m_spawnsetRegister.Size() ) return;
	CNPCSpawnSetComponent *ss = CNPCSpawnSetComponent::m_spawnsetRegister[ indexSpawnSet ];

	if ( indexActor >= (int)ss->m_actors.Size() ) return;
	const CNPCSpawnSetComponent::NPCControl* npcControl = m_actorsListBoxDataNPCControls[ indexActor ];

	CNPCSpawnSetComponent::SDebugData ssDebugData( npcControl );
	ss->SetDebugData( ssDebugData );
}

String CSpawnSetDebugger::GetFriendlySpawnSetActorStateName( CNPCSpawnSetActorState actorState )
{
	switch ( actorState )
	{
	case NPCST_Idle:
		return TXT("Idle");
	case NPCST_MovingToActionPoint:
		return TXT("MovToAP");
	case NPCST_WorkInProgress:
		return TXT("WIP");
	case NPCST_Spawning:
		return TXT("Spawning");
	case NPCST_Despawning:
		return TXT("Despawning");
	case NPCST_Suspended:
		return TXT("Suspended");
	default:
		return TXT("N/A");
	}
}

String CSpawnSetDebugger::GetFriendlyActionCategoriesName( const TDynArray< CNPCActionCategory > &actionCategories )
{
	String result;
	for ( TDynArray< CNPCActionCategory >::const_iterator actionCategory = actionCategories.Begin();
		  actionCategory != actionCategories.End(); )
	{
		result += actionCategory->m_categoryName.AsString() + TXT(" : ") + String::Printf(TXT("%f"), actionCategory->m_weight);
		if ( ++actionCategory != actionCategories.End() )
		{
			result += TXT("\n");
		}
	}
	return result;
}

String CSpawnSetDebugger::GetFriendlyTagsArrayName( const TDynArray< TagList > &tagListArray )
{
	String result;
	for ( TDynArray< TagList >::const_iterator tagList = tagListArray.Begin();
		  tagList != tagListArray.End();
		   )
	{
		result += tagList->ToString();
		if ( ++tagList != tagListArray.End() )
		{
			result += TXT(" : ");
		}
	}
	return result;
}

Bool CSpawnSetDebugger::IsTimeActive( const TDynArray< CSpawnSetTimetableSpawnEntry > &sstse, const GameTime &gameTime )
{
	GameTime currentGameTime = GGame->GetTimeManager()->GetTime();
	GameTime currentDayTime = currentGameTime % GameTime::DAY;

	if ( sstse.Size() == 0 )
	{
		return false;
	}

	if ( sstse.Size() == 1 )
	{
		return true;
	}

	TDynArray< CSpawnSetTimetableSpawnEntry >::const_iterator outputInterval;

	for ( TDynArray< CSpawnSetTimetableSpawnEntry >::const_iterator firstInterval = sstse.Begin(); 
		  firstInterval != sstse.End();
		  ++firstInterval )
	{
		TDynArray< CSpawnSetTimetableSpawnEntry >::const_iterator secondInterval = firstInterval + 1;
		if ( secondInterval != sstse.End() )
		{
			if ( currentDayTime >= firstInterval->m_time && currentDayTime < secondInterval->m_time )
			{
				outputInterval = firstInterval;
				break;
			}
		}
		else
		{
			outputInterval = firstInterval;
			break;
		}
	}

	if ( outputInterval->m_time == gameTime ) return true;
	else return false;
}

void CSpawnSetDebugger::ProcessSpawnSetEntries( int index )
{
	CNPCSpawnSetComponent *ss = CNPCSpawnSetComponent::m_spawnsetRegister[ index ];
	const TDynArray< CSpawnSetTimetable >* sstOrg = ss->m_spawnset->GetTimetableConstPtr();

	m_spawnSetEntryListBoxData.Clear();
	m_infoTextCtrlData.Clear();
	m_spawnInfoListBoxData.Clear();

	for ( TDynArray< CSpawnSetTimetable >::const_iterator sst = sstOrg->Begin();
		  sst != sstOrg->End();
		  ++sst )
	{
		String entryInfo = sst->m_entryTags.ToString().MidString( 2, sst->m_entryTags.ToString().GetLength() - 3 );
		if ( ss->m_currentTimetableEntries.Exist( &*sst ) ) entryInfo += TXT(" [ACTIVE]");
		m_spawnSetEntryListBoxData.push_back( entryInfo.AsChar() );


		m_spawnInfoListBoxData.push_back( entryInfo.AsChar() );

		for ( TDynArray< CSpawnSetTimetableSpawnEntry >::const_iterator sstse = sst->m_spawnArray.Begin();
			  sstse != sst->m_spawnArray.End();
			  ++sstse )
		{
			String sstseInfo = TXT("Time: ") + sstse->m_time.ToString() + TXT(" Quantity: ") +
				String::Printf( TXT("%d"), sstse->m_quantity ) + TXT(" Spawnpoint tag: ") + sstse->m_spawnpointTag.ToString();
			if ( IsTimeActive( sst->m_spawnArray, sstse->m_time ) ) sstseInfo += TXT(" [Active]");
			m_spawnInfoListBoxData.push_back( sstseInfo.AsChar() );
		}
		m_spawnInfoListBoxData.push_back( TXT("") );
	}

	// fill in spawnset component info
	String spawnsetInfo = TXT("Spawnset component info\n");
	spawnsetInfo += TXT("Included Entries Tags: ") + GetFriendlyTagsArrayName( ss->m_includedEntriesTags ) + TXT("\n");
	spawnsetInfo += TXT("Excluded Entries Tags: ") + GetFriendlyTagsArrayName( ss->m_excludedEntriesTags ) + TXT("\n");
	spawnsetInfo += TXT("\n");
	m_infoTextCtrlData = spawnsetInfo.AsChar();
}

void CSpawnSetDebugger::ProcessSpawnSetComponents()
{
	m_spawnSetListBoxData.Clear();

	for ( TDynArray< CNPCSpawnSetComponent*>::iterator ss = CNPCSpawnSetComponent::m_spawnsetRegister.Begin();
		ss != CNPCSpawnSetComponent::m_spawnsetRegister.End();
		++ss )
	{
		String ssName = (*ss)->GetName();
		m_spawnSetListBoxData.push_back( ssName.AsChar() );
	}
}

void CSpawnSetDebugger::ProcessActors( int indexSpawnSet, int indexSpawnSetEntry )
{
	m_actorsListBoxData.Clear();
	m_actorsListBoxDataNPCControls.Clear();

	if ( indexSpawnSet >= (int)CNPCSpawnSetComponent::m_spawnsetRegister.Size() ) return;
	CNPCSpawnSetComponent *ss = CNPCSpawnSetComponent::m_spawnsetRegister[ indexSpawnSet ];
	//if ( indexSpawnSetEntry >= (int)ss->m_currentTimetableEntries.Size() ) return;
	//const CSpawnSetTimetable* sst = ss->m_currentTimetableEntries[ indexSpawnSetEntry ];

	if ( indexSpawnSetEntry >= (int)ss->m_spawnset->GetTimetableConstPtr()->Size() ) return;
	const CSpawnSetTimetable* sst = &(*ss->m_spawnset->GetTimetableConstPtr())[ indexSpawnSetEntry ];
//	const TDynArray< CSpawnSetTimetable >* sstOrg = ss->m_spawnset->GetTimetableConstPtr();
	
	for ( TDynArray< CNPCSpawnSetComponent::NPCControl* >::iterator npcControl = ss->m_actors.Begin();
		npcControl != ss->m_actors.End();
		++npcControl )
	{
		String actorInfo;
		const Bool actorStolen = ( (*npcControl)->m_spawnsetEntryOverride == sst );
		if ( ( (*npcControl)->m_spawnsetEntryOverride == NULL && (*npcControl)->m_spawnsetEntry == sst )
			 || actorStolen )
		{
			String actorTemplateName = (*npcControl)->m_template->GetFile()->GetFileName();
			actorInfo = (*npcControl)->m_actor->GetName() + TXT(" ") + actorTemplateName + TXT(" ") +
				TXT(" AP Layer: ") + (*npcControl)->m_actionPointLayerName.AsString() + TXT(" ") + GetFriendlySpawnSetActorStateName( (*npcControl)->m_state );
			if ( actorStolen ) actorInfo += TXT(" [Stolen]");
			if ( (*npcControl)->m_state == NPCST_Suspended ) actorInfo += TXT(" Prev State: ") + GetFriendlySpawnSetActorStateName( (*npcControl)->m_previousState );
			if ( (*npcControl)->m_originalEntryExpired ) actorInfo += TXT(" [OrgExp]");
			if ( (*npcControl)->m_overrideEntryExpired ) actorInfo += TXT(" [OverExp]");
			
			m_actorsListBoxData.push_back( actorInfo.AsChar() );
			m_actorsListBoxDataNPCControls.PushBack( *npcControl );
		}
	}
}

void CSpawnSetDebugger::ProcessTimers( int indexSpawnSet, int indexSpawnSetEntry )
{
	m_timersListBoxData.Clear();

	if ( indexSpawnSet >= (int)CNPCSpawnSetComponent::m_spawnsetRegister.Size() ) return;
	CNPCSpawnSetComponent *ss = CNPCSpawnSetComponent::m_spawnsetRegister[ indexSpawnSet ];
	if ( indexSpawnSetEntry >= (int)ss->m_currentTimetableEntries.Size() ) return;
	const CSpawnSetTimetable* sst = ss->m_currentTimetableEntries[ indexSpawnSetEntry ];
	
	for ( TDynArray< CNPCSpawnSetComponent::NPCControlTimer >::iterator npcCtrlTimer = ss->m_timersQueue.Begin();
		npcCtrlTimer != ss->m_timersQueue.End();
		++npcCtrlTimer )
	{
		String timerInfo;
		const Bool actorStolen = ( npcCtrlTimer->m_control->m_spawnsetEntryOverride == sst );
		if ( ( npcCtrlTimer->m_control->m_spawnsetEntryOverride == NULL && npcCtrlTimer->m_control->m_spawnsetEntry == sst )
			|| actorStolen )
		{
			if ( npcCtrlTimer->m_control && npcCtrlTimer->m_control->m_actor )
			{
				timerInfo = npcCtrlTimer->m_control->m_actor->GetName() + TXT(" : ") + npcCtrlTimer->m_timerName.AsString() +  + TXT(" [") + String::Printf( TXT("%f"), npcCtrlTimer->m_timeLeft ) + TXT("]");
			}
			else
			{
				timerInfo = TXT("N/A : ") + npcCtrlTimer->m_timerName.AsString() +  + TXT(" [") + String::Printf( TXT("%f"), npcCtrlTimer->m_timeLeft ) + TXT("]");
			}
			m_timersListBoxData.push_back( timerInfo.AsChar() );
		}
	}
}

void CSpawnSetDebugger::ProcessActor( int indexSpawnSet, int indexActor )
{
	if ( indexSpawnSet >= (int)CNPCSpawnSetComponent::m_spawnsetRegister.Size() ) return;
	CNPCSpawnSetComponent *ss = CNPCSpawnSetComponent::m_spawnsetRegister[ indexSpawnSet ];
	
	if ( indexActor >= (int)ss->m_actors.Size() ) return;
	const CNPCSpawnSetComponent::NPCControl* npcControl = m_actorsListBoxDataNPCControls[ indexActor ];
	
	String actorInfo = TXT("Actor info\n");
	actorInfo += TXT("Action categories\n") + GetFriendlyActionCategoriesName( npcControl->m_actionCategories ) + TXT("\n");
	if ( npcControl->m_actionPoint && npcControl->m_actionPointComponent )
	{
		actorInfo += TXT("Action point\nAP component name: ") + npcControl->m_actionPointComponent->GetName() +
			TXT("\nAP layer name: ") + npcControl->m_actionPointLayerName.AsString() + TXT("\n");
	}

	if ( npcControl->m_actor )
	{
		actorInfo += TXT("Animations\n");
		String preAnimName, postAnimName, animName;
		npcControl->m_actor->GetCustomAnimation( TXT("StartLoopedWorkAnimation"), preAnimName );
		npcControl->m_actor->GetCustomAnimation( TXT("StopLoopedWorkAnimation"), postAnimName );
		if ( preAnimName != String::EMPTY || postAnimName != String::EMPTY )
		{
			npcControl->m_actor->GetCustomAnimation( TXT("LoopedWorkAnimation"), animName );
			actorInfo += TXT("Pre anim name: ") + preAnimName + TXT("\n");
			actorInfo += TXT("Anim name: ") + animName + TXT("\n");
			actorInfo += TXT("Post anim name: ") + postAnimName + TXT("\n");
		}
		else
		{
			npcControl->m_actor->GetCustomAnimation( TXT("WorkAnimation"), animName );
			if ( animName != String::EMPTY )
			{
				actorInfo += TXT("Anim name: ") + animName + TXT("\n");
			}
			else
			{
				actorInfo += TXT("N/A\n");
			}
		}
	}

	m_infoTextCtrlData += actorInfo.AsChar();
}

void CSpawnSetDebugger::SaveOptionsToConfig()
{
	SaveLayout( TXT("/Frames/SpawnsetDebugger") );
}

void CSpawnSetDebugger::LoadOptionsFromConfig()
{
	LoadLayout( TXT("/Frames/SpawnsetDebugger") );
}


#endif