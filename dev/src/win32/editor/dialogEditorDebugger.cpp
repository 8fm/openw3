
#include "build.h"
#include "dialogEditorDebugger.h"
#include "dialogEditor.h"

#include "../../common/game/storySceneInput.h"
#include "../../common/game/itemIterator.h"
#include "../../common/engine/animDangleComponent.h"
#include "../../common/engine/mimicComponent.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

CEdSceneDebugger::CEdSceneDebugger()
	: m_mediator( NULL )
	, m_isInitialized( false )
	, m_sceneMainFlowChanged( false )
	, m_loadingsChanged( false )
	, m_executorLogChanged( true )
	, m_showActorDetails( false )
	, m_isRunning( false )
{

}

wxPanel* CEdSceneDebugger::Setup( CEdSceneEditor* mediator, wxWindow* parent )
{
	m_mediator = mediator;

	wxPanel* debugTabPanel = wxXmlResource::Get()->LoadPanel( parent, wxT( "DialogDebugPanel" ) );

	m_mainPanel = XRCCTRL( *debugTabPanel, "dialogDebug_flow_html", wxHtmlWindow );
	m_eventListPanel = XRCCTRL( *debugTabPanel, "dialogDebug_eventList_html", wxHtmlWindow );
	m_actorStatesPanel = XRCCTRL( *debugTabPanel, "dialogDebug_actorState_html", wxHtmlWindow );
	m_actorStatesPanel->Bind( wxEVT_COMMAND_HTML_LINK_CLICKED, &CEdSceneDebugger::OnDebuggerLinkClicked, this );
	m_loadingPanel = XRCCTRL( *debugTabPanel, "dialogDebug_loading_html", wxHtmlWindow );
	m_executorPanel = XRCCTRL( *debugTabPanel, "dialogDebug_executor_html", wxHtmlWindow );

	return debugTabPanel;
}

void CEdSceneDebugger::EnableDebugging( Bool flag )
{
	m_isRunning = flag;
}

void CEdSceneDebugger::Init()
{
	m_isInitialized = true;

	const Double currTime = GEngine->GetRawEngineTime();
	m_loadings.PushBack( TPair< Double, String >( currTime, String( TXT("Start") ) ) );
}

Bool CEdSceneDebugger::IsThisPanelVisible() const
{
	return false; //m_mainPanel->IsShown() || m_eventListPanel->IsShown();
}

void CEdSceneDebugger::RemoveDuplicatesFromLatentInfoList()
{
	TDynArray< Uint32 > toRemove;

	for ( Int32 i=0; i<m_latentInfos.SizeInt(); ++i )
	{
		const String& itemA = m_latentInfos[ i ].m_info;
		const void* idA = m_latentInfos[ i ].m_id;

		for ( Int32 j=i+1; j<m_latentInfos.SizeInt(); ++j )
		{	
			const String& itemB = m_latentInfos[ j ].m_info;
			const void* idB = m_latentInfos[ j ].m_id;

			if ( itemA == itemB && idA == idB )
			{
				toRemove.PushBackUnique( i );
			}
		}
	}

	for ( Int32 i=toRemove.SizeInt()-1; i>=0; --i )
	{
		m_latentInfos.RemoveAtFast( toRemove[ i ] );
	}
}

void CEdSceneDebugger::RemoveItemsFromLatentInfoListByTime( Float time )
{
	while ( m_latentInfos.Size() > 30 )
	{
		Int32 sel = -1;
		Float selVal = -1.f;

		for ( Int32 i=0; i<m_latentInfos.SizeInt(); ++i )
		{
			const Float itemTime = m_latentInfos[ i ].m_time;
			const Float diff = MAbs( itemTime - time );

			if ( diff > selVal )
			{
				sel = i;
				selVal = diff;
			}
		}

		if ( sel != -1 )
		{
			m_latentInfos.RemoveAtFast( sel );
		}
		else
		{
			ASSERT( sel != -1 );
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

#define RET_IF_NOT_DEBUG if ( !m_isInitialized || !m_isRunning ) return;

void CEdSceneDebugger::PlayerLogicTickBegin( const CStoryScenePlayer* player )
{
	RET_IF_NOT_DEBUG;

	const Float time = player->GetCurrentSectionTime();

	// TEMP
	if ( m_context.Size() > 60 )
	{
		m_context.RemoveAtFast( 0 );
	}

	TickContext tickContext;
	tickContext.m_time = time;
	m_context.PushBack( tickContext );
}

void CEdSceneDebugger::PlayerLogicTickEnd( const CStoryScenePlayer* player )
{
	//if ( !IsThisPanelVisible() || !m_isInitialized )
	//{
	//	return;
	//}

	RET_IF_NOT_DEBUG;

	RefreshActorStatesPage();

	//RemoveDuplicatesFromLatentInfoList();
	//RemoveItemsFromLatentInfoListByTime( player->GetCurrentSectionTime() );

	//RefreshMainPage();
	//RefreshLatentListPage();
	//RefreshLoadingPage();
	//RefreshExecutorPage();
}

void CEdSceneDebugger::BeginTickElement( const IStorySceneElementInstanceData* element, Float dt )
{
	RET_IF_NOT_DEBUG;

	TickContext& tickContext = m_context[ m_context.SizeInt() - 1 ];

	ElementContext item;
	item.m_element = element->GetName();
	tickContext.m_elems.PushBack( item );

	m_latentInfos.PushBack( LatentItem( element->GetName(), tickContext.m_time, element->GetCurrentTime(), element ) );
}

void CEdSceneDebugger::EndTickElement( const IStorySceneElementInstanceData* element, Float dt )
{
	RET_IF_NOT_DEBUG;

	TickContext& tickContext = m_context[ m_context.SizeInt() - 1 ];

	m_latentInfos.PushBack( LatentItem( element->GetName(), tickContext.m_time, element->GetCurrentTime(), element ) );
}

void CEdSceneDebugger::FireAllStartEvents( const IStorySceneElementInstanceData* element )
{
	RET_IF_NOT_DEBUG;

	TickContext& tickContext = m_context[ m_context.SizeInt() - 1 ];

	ElementContext item;
	item.m_element = element->GetName();
	tickContext.m_elems.PushBack( item );

	String str = String::Printf( TXT("%s - fire all start events"), element->GetName().AsChar() );
	m_latentInfos.PushBack( LatentItem( str, tickContext.m_time, element->GetCurrentTime(), element ) );
}

void CEdSceneDebugger::FireAllStopEvents( const IStorySceneElementInstanceData* element )
{
	RET_IF_NOT_DEBUG;

	TickContext& tickContext = m_context[ m_context.SizeInt() - 1 ];

	ElementContext item;
	item.m_element = element->GetName();
	tickContext.m_elems.PushBack( item );

	String str = String::Printf( TXT("%s - fire all stop events"), element->GetName().AsChar() );
	m_latentInfos.PushBack( LatentItem( str, tickContext.m_time, element->GetCurrentTime(), element ) );
}

void CEdSceneDebugger::EventStart( const CStorySceneEvent* event )
{
	RET_IF_NOT_DEBUG;

	TickContext& tickContext = m_context[ m_context.SizeInt() - 1 ];
	ElementContext& item = tickContext.m_elems[ tickContext.m_elems.SizeInt() - 1 ];
	item.m_eventsStarted.PushBack( event->GetName() );
	m_latentInfos.PushBack( LatentItem( event->GetName(), tickContext.m_time, 0.f, event ) );
}

void CEdSceneDebugger::EventProcess( const CStorySceneEvent* event, Float eventTime, Float progress, Float timeDelta )
{
	RET_IF_NOT_DEBUG;

	TickContext& tickContext = m_context[ m_context.SizeInt() - 1 ];
	ElementContext& item = tickContext.m_elems[ tickContext.m_elems.SizeInt() - 1 ];
	String str = String::Printf( TXT("%s [%1.2f / %1.2f]"), event->GetName().AsChar(), eventTime, progress );
	item.m_eventsProcessed.PushBack( str );
}

void CEdSceneDebugger::EventEnd( const CStorySceneEvent* event )
{
	RET_IF_NOT_DEBUG;

	TickContext& tickContext = m_context[ m_context.SizeInt() - 1 ];
	ElementContext& item = tickContext.m_elems[ tickContext.m_elems.SizeInt() - 1 ];
	item.m_eventsEnded.PushBack( event->GetName() );
	m_latentInfos.PushBack( LatentItem( event->GetName(), tickContext.m_time, 0.f, event ) );
}

/*void CEdSceneDebugger::OnPreloadNextSection( const CStorySceneSection* section )
{
	RET_IF_NOT_DEBUG;

	const Double currTime = GEngine->GetRawEngineTime();
	String sectionName = section ? section->GetName() : TXT("<None>");
	String str = String::Printf( TXT("Start preloading section: [%s]"), sectionName.AsChar() );
	m_loadings.PushBack( TPair< Double, String >( currTime, str ) );
	m_loadingsChanged = true;
}

void CEdSceneDebugger::OnPreloadState( const String& stateName )
{
	RET_IF_NOT_DEBUG;

	const Double currTime = GEngine->GetRawEngineTime();
	m_loadings.PushBack( TPair< Double, String >( currTime, stateName ) );
	m_loadingsChanged = true;
}*/

void CEdSceneDebugger::OnChangeSection( const CStorySceneSection* prev, const CStorySceneSection* next )
{
	RET_IF_NOT_DEBUG;

	String prevName = prev ? prev->GetName() : TXT("<None>");
	String nextName = next ? next->GetName() : TXT("<None>");
	String str = String::Printf( TXT("Change section: [%s] -> [%s]"), prevName.AsChar(), nextName.AsChar() );
	m_sceneMainFlow.PushBack( str );
	m_sceneMainFlowChanged = true;
}

void CEdSceneDebugger::OnFinishChangingSection( const CStorySceneSection* prev, const CStorySceneSection* next )
{
	RET_IF_NOT_DEBUG;

	String prevName = prev ? prev->GetName() : TXT("<None>");
	String nextName = next ? next->GetName() : TXT("<None>");
	String str = String::Printf( TXT("Finish changing section: [%s] -> [%s]"), prevName.AsChar(), nextName.AsChar() );
	m_sceneMainFlow.PushBack( str );
	m_sceneMainFlowChanged = true;
}

void CEdSceneDebugger::OnStartFromInput( const CStorySceneInput* input )
{
	RET_IF_NOT_DEBUG;

	String inputName = input ? input->GetName() : TXT("<None>");
	String str = String::Printf( TXT("Start from input: [%s]"), inputName.AsChar() );
	m_sceneMainFlow.PushBack( str );
	m_sceneMainFlowChanged = true;
}

void CEdSceneDebugger::OnFinishedAtOutput( const CStorySceneOutput* output )
{
	RET_IF_NOT_DEBUG;

	String outputName = output ? output->GetName() : TXT("<None>");
	String str = String::Printf( TXT("Finished at output: [%s]"), outputName.AsChar() );
	m_sceneMainFlow.PushBack( str );
	m_sceneMainFlowChanged = true;
}

void CEdSceneDebugger::OnExecutor_ProcessPlacements( const CEntity* entity, const Matrix& placementWS )
{
	RET_IF_NOT_DEBUG;

	m_executorLogChanged = true;

	const String& actorId = entity->GetName();

	String posStr = ToString( placementWS.GetTranslation() );
	String rotStr = ToString( placementWS.ToEulerAngles() );

	m_executorLog.PushBack( String::Printf( TXT("[ChangeIdle] - entity: '%s', pos: '%s', rot: '%s'<br>"), actorId.AsChar(), posStr.AsChar(), rotStr.AsChar() ) );
}

void CEdSceneDebugger::OnExecutor_ChangeIdle( const CEntity* entity, const SStorySceneActorAnimationState& newState )
{
	RET_IF_NOT_DEBUG;

	m_executorLogChanged = true;

	const String& actorId = entity->GetName();

	String stateStr;
	newState.ToString( stateStr );

	m_executorLog.PushBack( String::Printf( TXT("[ChangeIdle] - entity: '%s', state: '%s'<br>"), actorId.AsChar(), stateStr.AsChar() ) );
}

#undef RET_IF_NOT_DEBUG

//////////////////////////////////////////////////////////////////////////

void CEdSceneDebugger::RefreshMainPage()
{
	if ( m_sceneMainFlowChanged )
	{
		wxString code;

		const Int32 size = m_sceneMainFlow.SizeInt();
		for ( Int32 i=size-1; i>=0; --i )
		{
			code += wxString::Format( wxT("%s<br>"), m_sceneMainFlow[ i ].AsChar() );
		}

		m_sceneMainFlowChanged = false;

		m_mainPanel->Freeze();
		m_mainPanel->SetPage( code );
		m_mainPanel->Thaw();
	}
}

//////////////////////////////////////////////////////////////////////////

void CEdSceneDebugger::RefreshLatentListPage()
{
	wxString code;

	TickContext& tickContext = m_context[ m_context.SizeInt() - 1 ];
	code += wxString::Format( wxT("Time: %1.2f<br><hr><br>"), tickContext.m_time );

	for ( Int32 i=m_latentInfos.SizeInt()-1; i>=0; --i )
	{
		const LatentItem& item = m_latentInfos[ i ];

		code += wxString::Format( wxT("[%1.2f / %1.2f] %s<br>"), item.m_time, item.m_localTime, item.m_info.AsChar() );
	}

	m_eventListPanel->Freeze();
	m_eventListPanel->SetPage( code );
	m_eventListPanel->Thaw();
}

//////////////////////////////////////////////////////////////////////////

void CEdSceneDebugger::WriteActorState( const CName& name, const CEntity* entity, wxString& code )
{
	const CActor* actor = Cast< const CActor >( entity );
	CName actorVoiceTag = actor ? actor->GetVoiceTag() : name;

	String empty( TXT("<None>") );

	if ( name != actorVoiceTag )
	{
		code += wxString::Format( wxT("<font color=\"red\">Voicetag (Scene): %s<br></font>"), name.AsString().AsChar() );
		code += wxString::Format( wxT("<font color=\"red\">Voicetag (Actor): %s<br></font>"), actorVoiceTag.AsString().AsChar() );
	}
	else
	{
		code += wxString::Format( wxT("Voicetag (s): %s<br>"), name.AsString().AsChar() );
		code += wxString::Format( wxT("Voicetag (a): %s<br>"), actorVoiceTag.AsString().AsChar() );
	}

	code += wxString::Format( wxT("IsSpawned:    %d<br>"), entity != NULL ? 1 : 0 );
	code += wxString::Format( wxT("HideInGame:   %d<br>"), entity->IsHiddenInGame() ? 1 : 0 );
	code += wxString::Format( wxT("App:          %s<br>"), actor ? actor->GetAppearance().AsString().AsChar() : empty.AsChar() );
	code += wxString::Format( wxT("Mimics:       %d<br>"), actor && actor->HasMimic() ? 1 : 0 );

	SStorySceneActorAnimationState state;
	const Bool hasState = m_mediator->OnDebugger_GetActorAnimationState( name, state );

	code += wxString::Format( wxT("Status:       %s<br>"), hasState ? state.m_status.AsString().AsChar() : empty.AsChar() );
	code += wxString::Format( wxT("EmoState:     %s<br>"), hasState ? state.m_emotionalState.AsString().AsChar() : empty.AsChar() );
	code += wxString::Format( wxT("Pose:         %s<br>"), hasState ? state.m_poseType.AsString().AsChar() : empty.AsChar() );
	code += wxString::Format( wxT("MimicsEmoSt:  %s<br>"), hasState ? state.m_mimicsEmotionalState.AsString().AsChar() : empty.AsChar() );
	code += wxString::Format( wxT("ML Eyes:      %s<br>"), hasState ? state.m_mimicsLayerEyes.AsString().AsChar() : empty.AsChar() );
	code += wxString::Format( wxT("ML Pose:      %s<br>"), hasState ? state.m_mimicsLayerPose.AsString().AsChar() : empty.AsChar() );
	code += wxString::Format( wxT("ML Animation: %s<br>"), hasState ? state.m_mimicsLayerAnimation.AsString().AsChar() : empty.AsChar() );

	if ( m_showActorDetails )
	{
		TDynArray< CName > data;
		m_mediator->OnDebugger_GetActorAnimationNames( name, data );

		code += wxString::Format( wxT("<br>Body Prev:         %s<br>"), data[0].AsString().AsChar() );
		code += wxString::Format( wxT("Body Next:         %s<br>"), data[1].AsString().AsChar() );
		code += wxString::Format( wxT("Body Weight:       %s<br><br>"), data[2].AsString().AsChar() );

		code += wxString::Format( wxT("Mimics Eyes Prev:  %s<br>"), data[3].AsString().AsChar() );
		code += wxString::Format( wxT("Mimics Eyes Next:  %s<br>"), data[4].AsString().AsChar() );
		code += wxString::Format( wxT("Mimics Pose Prev:  %s<br>"), data[5].AsString().AsChar() );
		code += wxString::Format( wxT("Mimics Pose Next:  %s<br>"), data[6].AsString().AsChar() );
		code += wxString::Format( wxT("Mimics Anim Prev:  %s<br>"), data[7].AsString().AsChar() );
		code += wxString::Format( wxT("Mimics Anim Next:  %s<br>"), data[8].AsString().AsChar() );
		code += wxString::Format( wxT("Mimics PoseW Prev: %s<br>"), data[9].AsString().AsChar() );
		code += wxString::Format( wxT("Mimics PoseW Next: %s<br>"), data[10].AsString().AsChar() );
		code += wxString::Format( wxT("Mimics Weight:     %s<br><br>"), data[11].AsString().AsChar() );

		code += wxString::Format( wxT("LookAt Body Prev:  %s<br>"), data[12].AsString().AsChar() );
		code += wxString::Format( wxT("LookAt Head Prev:  %s<br>"), data[13].AsString().AsChar() );
		code += wxString::Format( wxT("LookAt Body Next:  %s<br>"), data[14].AsString().AsChar() );
		code += wxString::Format( wxT("LookAt Head Next:  %s<br><br>"), data[15].AsString().AsChar() );
	}

	const Matrix sPos = m_mediator->OnDebugger_GetActorPosition( name );
	const Matrix aPos = entity->GetLocalToWorld();

	const Bool positionsAreEqual = Vector::Near3( aPos.GetTranslation(), sPos.GetTranslation() ) && aPos.ToEulerAngles().AlmostEquals( sPos.ToEulerAngles() );
	if ( positionsAreEqual )
	{
		code += wxString::Format( wxT("Position (s): %s | %s<br>"), ToString( sPos.GetTranslation() ).AsChar(), ToString( sPos.ToEulerAngles() ).AsChar() );
		code += wxString::Format( wxT("Position (a): %s | %s<br>"), ToString( aPos.GetTranslation() ).AsChar(), ToString( aPos.ToEulerAngles() ).AsChar() );
	}
	else
	{
		code += wxString::Format( wxT("<font color=\"red\">Position (s): %s | %s<br></font>"), ToString( sPos.GetTranslation() ).AsChar(), ToString( sPos.ToEulerAngles() ).AsChar() );
		code += wxString::Format( wxT("<font color=\"red\">Position (a): %s | %s<br></font>"), ToString( aPos.GetTranslation() ).AsChar(), ToString( aPos.ToEulerAngles() ).AsChar() );
	}

	CAnimatedComponent* comp = m_mediator->OnDebugger_GetAnimatedComponentForActor( name );
	CAnimatedComponent* head = comp && comp->GetEntity()->QueryActorInterface() ? comp->GetEntity()->QueryActorInterface()->GetMimicComponent() : nullptr;

	if ( comp && comp->IsDispSkeleton( ACDD_SkeletonBone ) )
	{
		code += wxString::Format( wxT("<a href=\"HideSkeleton_%s\">Hide body skeleton</a><br>"), name.AsString().AsChar() );
	}
	else
	{
		code += wxString::Format( wxT("<a href=\"ShowSkeleton_%s\">Show body skeleton</a><br>"), name.AsString().AsChar() );
	}

	if ( head && head->IsDispSkeleton( ACDD_SkeletonBone ) )
	{
		code += wxString::Format( wxT("<a href=\"HideMimicSkeleton_%s\">Hide mimic skeleton</a><br>"), name.AsString().AsChar() );
	}
	else
	{
		code += wxString::Format( wxT("<a href=\"ShowMimicSkeleton_%s\">Show mimic skeleton</a><br>"), name.AsString().AsChar() );
	}

	if ( m_selectedActor == name )
	{
		code += wxString::Format( wxT("<a><i>Selected actor</i></a><br>"), name.AsString().AsChar() );
	}
	else
	{
		code += wxString::Format( wxT("<a href=\"SelectActor_%s\">Select actor</a><br>"), name.AsString().AsChar() );
	}

	code += wxString::Format( wxT("<a href=\"ShowDangles_%s\">Show dangles</a><br>"), name.AsString().AsChar() );
	code += wxString::Format( wxT("<a href=\"HideDangles_%s\">Hide dangles</a>"), name.AsString().AsChar() );

	code += wxT("<hr>");
}

void CEdSceneDebugger::RefreshActorStatesPage()
{
	wxString code;

	{
		const Matrix scenePose = m_mediator->OnDebugger_GetDialogPosition();

		code += wxString::Format( wxT("Scene position: %s | %s<br><br>"), ToString( scenePose.GetTranslation() ).AsChar(), ToString( scenePose.ToEulerAngles() ).AsChar() );

		if ( !m_showActorDetails )
			{
				code += wxString::Format( wxT("<a href=\"ShowDetails\">Show details</a><br>") );
			}
			else
			{
				code += wxString::Format( wxT("<a href=\"HideDetails\">Hide details</a><br>") );
			}

		code += wxT("<hr>");
	}

	const THashMap< CName, THandle< CEntity > >& actorMap = m_mediator->OnDebugger_GetActorMap();
	THashMap< CName, THandle< CEntity > >::const_iterator end = actorMap.End();

	for ( THashMap< CName, THandle< CEntity > >::const_iterator it = actorMap.Begin(); it != end; ++it )
	{
		CName name = it->m_first;

		if ( m_selectedActor == name )
		{
			const CEntity* entity = it->m_second.Get();

			WriteActorState( name, entity, code );
		}
	}

	for ( THashMap< CName, THandle< CEntity > >::const_iterator it = actorMap.Begin(); it != end; ++it )
	{
		CName name = it->m_first;

		if ( m_selectedActor != name )
		{
			const CEntity* entity = it->m_second.Get();

			WriteActorState( name, entity, code );
		}
	}

	m_actorStatesPanel->Freeze();
	m_actorStatesPanel->SetPage( code );
	m_actorStatesPanel->Thaw();
}

//////////////////////////////////////////////////////////////////////////

void CEdSceneDebugger::RefreshLoadingPage()
{
	if ( m_loadingsChanged )
	{
		wxString code;

		const Int32 size = m_loadings.SizeInt();
		for ( Int32 i=size-1; i>=1; --i )
		{
			const TPair< Double, String >& curr = m_loadings[ i ];
			const TPair< Double, String >& prev = m_loadings[ i-1 ];

			const Double timeDiff = curr.m_first - prev.m_first;

			code += wxString::Format( wxT("Time: %1.2f, Time diff: %1.2f - State: %s<br>"), (Float)curr.m_first, (Float)timeDiff, curr.m_second.AsChar() );
		}

		m_loadingPanel->Freeze();
		m_loadingPanel->SetPage( code );
		m_loadingPanel->Thaw();

		m_loadingsChanged = false;
	}
}

//////////////////////////////////////////////////////////////////////////

void CEdSceneDebugger::RefreshExecutorPage()
{
	if ( m_executorLogChanged )
	{
		wxString code;

		const Int32 size = m_executorLog.SizeInt();
		for ( Int32 i=size-1; i>=0; --i )
		{
			code += m_executorLog[ i ].AsChar();
		}

		m_executorPanel->Freeze();
		m_executorPanel->SetPage( code );
		m_executorPanel->Thaw();

		m_executorLogChanged = false;
	}
}

//////////////////////////////////////////////////////////////////////////

void CEdSceneDebugger::OnDebuggerLinkClicked( wxHtmlLinkEvent& event )
{
	const wxHtmlLinkInfo& linkInfo = event.GetLinkInfo();
	String str( linkInfo.GetHref() );

	if ( str == TXT("ShowDetails") )
	{
		m_showActorDetails = true;
	}
	else if ( str == TXT("HideDetails") )
	{
		m_showActorDetails = false;
	}
	else if ( str.BeginsWith( TXT("ShowSkeleton") ) )
	{
		String nameStr = str.StringAfter( TXT("_") );

		CAnimatedComponent* comp = m_mediator->OnDebugger_GetAnimatedComponentForActor( CName( nameStr ) );
		if ( comp )
		{
			comp->SetDispSkeleton( ACDD_SkeletonBone, true );
		}
	}
	else if ( str.BeginsWith( TXT("HideSkeleton") ) )
	{
		String nameStr = str.StringAfter( TXT("_") );

		CAnimatedComponent* comp = m_mediator->OnDebugger_GetAnimatedComponentForActor( CName( nameStr ) );
		if ( comp )
		{
			comp->SetDispSkeleton( ACDD_SkeletonBone, false );
		}
	}
	else if ( str.BeginsWith( TXT("ShowMimicSkeleton") ) )
	{
		String nameStr = str.StringAfter( TXT("_") );

		CAnimatedComponent* comp = m_mediator->OnDebugger_GetAnimatedComponentForActor( CName( nameStr ) );
		CAnimatedComponent* head = comp->GetEntity()->QueryActorInterface() ? comp->GetEntity()->QueryActorInterface()->GetMimicComponent() : nullptr;
		if ( head )
		{
			head->SetDispSkeleton( ACDD_SkeletonBone, true );
		}
	}
	else if ( str.BeginsWith( TXT("HideMimicSkeleton") ) )
	{
		String nameStr = str.StringAfter( TXT("_") );

		CAnimatedComponent* comp = m_mediator->OnDebugger_GetAnimatedComponentForActor( CName( nameStr ) );
		CAnimatedComponent* head = comp->GetEntity()->QueryActorInterface() ? comp->GetEntity()->QueryActorInterface()->GetMimicComponent() : nullptr;
		if ( head )
		{
			head->SetDispSkeleton( ACDD_SkeletonBone, false );
		}
	}
	else if ( str.BeginsWith( TXT("ShowDangles") ) )
	{
		String nameStr = str.StringAfter( TXT("_") );

		CAnimatedComponent* comp = m_mediator->OnDebugger_GetAnimatedComponentForActor( CName( nameStr ) );
		if ( CActor* a = Cast< CActor >( comp->GetEntity() ) )
		{
			for ( EntityWithItemsComponentIterator< CAnimDangleComponent > it( a ); it; ++it )
			{
				CAnimDangleComponent* c = *it;
				c->ShowDebugRender( true );
			}
		}
	}
	else if ( str.BeginsWith( TXT("HideDangles") ) )
	{
		String nameStr = str.StringAfter( TXT("_") );

		CAnimatedComponent* comp = m_mediator->OnDebugger_GetAnimatedComponentForActor( CName( nameStr ) );
		if ( CActor* a = Cast< CActor >( comp->GetEntity() ) )
		{
			for ( EntityWithItemsComponentIterator< CAnimDangleComponent > it( a ); it; ++it )
			{
				CAnimDangleComponent* c = *it;
				c->ShowDebugRender( false );
			}
		}
	}
	else if ( str.BeginsWith( TXT("SelectActor") ) )
	{
		String nameStr = str.StringAfter( TXT("_") );
		m_selectedActor = CName( nameStr );
	}

	RefreshActorStatesPage();
}

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( CEdDialogFCurveEditor, CEdCurveEditorCanvas )
END_EVENT_TABLE()

CEdDialogFCurveEditor::CEdDialogFCurveEditor( wxWindow *parentWin )
	: CEdCurveEditorCanvas( parentWin )
{

}

void CEdDialogFCurveEditor::SetDataToPlot( const TDynArray< String >& tracks, const TDynArray< TDynArray< TPair< Vector2, Vector > > >& data )
{
	RemoveAllCurves();

	const Uint32 numCurves = Min< Uint32 >( tracks.Size(), data.Size() );
	m_plotData.Resize( numCurves );

	const Color colors[3] = { Color::RED, Color::GREEN, Color::BLUE };

	Float min = FLT_MAX;
	Float max = -FLT_MAX;
	Bool set = false;

	for ( Uint32 k=0; k<numCurves; ++k )
	{
		SCurveData& c = m_plotData[k];
		c.Clear();

		const TDynArray< TPair< Vector2, Vector > >& currData = data[ k ];
		for ( Uint32 i=0; i<currData.Size(); ++i )
		{
			const TPair< Vector2, Vector >& elem = currData[ i ];

			if ( Vector::Equal4( elem.m_second, Vector::ZEROS ) )
			{
				c.AddPoint( elem.m_first.X, elem.m_first.Y );
			}
			else
			{
				c.AddPoint( elem.m_first.X, elem.m_first.Y, elem.m_second, CST_Bezier2D );
			}

			min = Min( min, elem.m_first.X );
			max = Max( max, elem.m_first.X );
			set = true;
		}

		AddCurve( &c, tracks[ k ] )->SetColor( colors[ k % 3 ] );
	}

	if ( set )
	{
		SetActiveRegion( min, max );
	}

	SetZoomedRegionToFit();
	Repaint();
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
