
#include "build.h"
#include "dialogEditor.h"

#include "dialogEditorActions.h"

#include "dialogEditorPage.h"
#include "gridEditor.h"
#include "gridCustomTypes.h"
#include "gridCustomColumns.h"
#include "gridPropertyWrapper.h"
#include "dialogTimeline.h"
#include "propertiesPageComponent.h"

#include "../../common/core/mathUtils.h"
#include "../../common/engine/localizationManager.h"
#include "../../common/engine/tagManager.h"
#include "../../common/engine/mimicComponent.h"

#include "../../common/game/storySceneLine.h"
#include "../../common/game/storySceneCutscene.h"
#include "../../common/game/storySceneScriptLine.h"
#include "../../common/game/storyScenePauseElement.h"
#include "../../common/game/storySceneBlockingElement.h"
#include "../../common/game/storySceneEvent.h"
#include "../../common/game/storySceneEventDuration.h"
#include "../../common/game/storySceneEventAnimation.h"
#include "../../common/game/storySceneEventEnterActor.h"
#include "../../common/game/storySceneEventExitActor.h"
#include "../../common/game/storySceneEventLookat.h"
#include "../../common/game/storySceneEventDespawn.h"
#include "../../common/game/storySceneEventFade.h"
#include "../../common/game/storySceneEventMimics.h"
#include "../../common/game/storySceneEventMimicsAnim.h"
#include "../../common/game/storySceneEventSound.h"
#include "../../common/game/storySceneEventRotate.h"
#include "../../common/game/storySceneEventCustomCamera.h"
#include "../../common/game/storySceneEventCustomCameraInstance.h"
#include "../../common/game/storySceneEventCameraBlend.h"
#include "../../common/game/storySceneEventChangePose.h"
#include "../../common/game/storySceneEventEquipItem.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/game/storySceneComment.h"
#include "../../common/game/storyScene.h"
#include "../../common/game/actor.h"
#include "../../common/game/storySceneCutsceneSection.h"
#include "../../common/game/storySceneEventMimicPose.h"
#include "../../common/game/storySceneEventMimicFilter.h"
#include "../../common/game/storySceneEventOverridePlacement.h"
#include "../../common/game/storySceneEventGroup.h"
#include "../../common/game/storySceneInput.h"
#include "../../common/game/storySceneChoice.h"
#include "../../common/game/storySceneEventScenePropPlacement.h"
#include "../../common/game/storySceneEventWorldPropPlacement.h"
#include "../../common/engine/gameResource.h"
#include "../../common/game/storySceneUtils.h"
#include "../../common/engine/curveEntity.h"
#include "../../common/engine/curveControlPointEntity.h"
#include "../../common/engine/curveTangentControlPointEntity.h"
#include "../../common/core/feedback.h"
#include "../../common/game/itemIterator.h"

#include "../importer/REAAnimImporter.h"

#include "storyScenePreviewPlayer.h"
#include "dialogUtilityHandlers.h"
#include "dialogPreview.h"
#include "dialogEditorDialogsetPanel.h"
#include "voice.h"
#include "lipsyncDataSceneExporter.h"
#include "animFriend.h"
#include "dialogAnimationParameterEditor.h"
#include "dialogEventGeneratorSetupDialog.h"
#include "dialogEventGenerator.h"
#include "dialogEditorTempLipsyncDlg.h"
#include "dialogTimeline.h"
#include "dialogTimeline_items.h"
#include "sceneValidator.h"

void CEdSceneEditor::MovePreviewWindow( bool previewInEditor )
{
	if( previewInEditor )
	{
		// store the 'good-olde' layout
		m_oldPerspective = m_auiManager.SavePerspective();

		// move main tabs to control tabs
		while( m_mainTabs->GetPageCount() > 0 )
		{
			wxWindow* wnd = m_mainTabs->GetPage( 0 );
			m_movedWindows.PushBack( wnd );

			// get page text
			wxString text = m_mainTabs->GetPageText( 0 );
			m_mainTabs->RemovePage( 0 );

			// exception of the actual preview window (no longer needed)
			if( wnd == m_previewPanel )
			{
				continue;
			}

			// add page to control tabs
			m_controlTabs->AddPage( wnd, text );
		}

		m_auiManager.DetachPane( m_mainTabs );
		m_mainTabs->Hide();
		m_auiManager.Update();
	}
	else
	{
		// restore moved windows
		while( m_movedWindows.Size() > 0 )
		{
			wxWindow* wnd = m_movedWindows[ 0 ];
			m_movedWindows.RemoveAt( 0 );

			// get page text
			int iIndex = m_controlTabs->GetPageIndex( wnd );
			if( iIndex >= 0 )
			{
				wxString text = m_controlTabs->GetPageText( iIndex );
				m_controlTabs->RemovePage( iIndex );

				// add page to main tabs
				m_mainTabs->AddPage( wnd, text, wnd == m_previewPanel );
			}

			// exception of the actual preview window (no longer needed)
			if( wnd == m_previewPanel )
			{
				m_mainTabs->AddPage( wnd, wxT( "Preview" ), true );
			}
		}

		m_controlTabs->SetSelection( 1 );
		m_mainTabs->Show();
		m_auiManager.AddPane( m_mainTabs, wxTOP );
		m_auiManager.GetPane( m_mainTabs ).CloseButton( false ).Name( wxT( "Main" ) ).CaptionVisible( true ).BestSize( 400, 400 );
		m_auiManager.Update();	
		m_auiManager.LoadPerspective( m_oldPerspective, true );
	}

	// find reset layout button, and disable it if we are previewing in the editor window
	wxMenuItem* resetLayoutItem = GetMenuBar()->FindItem( XRCID( "ResetLayout" ) );
	if( resetLayoutItem )
	{
		resetLayoutItem->Enable( !previewInEditor );
	}
}

void CEdSceneEditor::MovePreviewToolbarToTimeline( bool previewInEditor )
{
	wxToolBar* tbPreview = m_preview->GetToolbar();
	wxToolBar* tbTimeline = m_timeline->GetToolbar();

	if( !tbPreview || !tbTimeline )
	{
		// Ouch...
		return;
	}

	if( !previewInEditor )
	{
		for( Uint32 i=0; i<tbPreview->GetToolsCount(); ++i )
		{
			const wxToolBarToolBase* toolSource = tbPreview->GetToolByPos( i );
			if( !toolSource )
			{
				continue;
			}

			Int32 id = toolSource->GetId();

			// move tools away from toolbar
			m_previewTools.PushBackUnique( tbTimeline->FindById( id ) );
			tbTimeline->RemoveTool( id );
		}
	}
	else
	{
		while( !m_previewTools.Empty() )
		{
			// add tools
			wxToolBarToolBase* tool = m_previewTools[ 0 ];
			m_previewTools.RemoveAt( 0 );
			tbTimeline->AddTool( tool );
		}
	}

	tbTimeline->Realize();
}

void CEdSceneEditor::SelectDetailPage( const wxString& pageName )
{
	for ( Uint32 i = 0; i < m_detailsTabs->GetPageCount(); ++i )
	{
		if ( m_detailsTabs->GetPageText( i ) == pageName )
		{
			// remember which control has focus as wxAuiNotebook::SetSelection() changes it
			wxWindow* focusCtrl = FindFocus();

			m_detailsTabs->SetSelection( i );

			// restore focus
			if ( focusCtrl )
			{
				focusCtrl->SetFocus();
			}

			break;
		}
	}
}

void CEdSceneEditor::SelectDetailPage( Uint32 pageId )
{
	SelectDetailPage( m_detailsTabs->GetPageText( pageId )  );
}

Bool CEdSceneEditor::IsDetailPageOpened( Uint32 pageId ) const
{
	return m_detailsTabs->GetSelection() == pageId;
}

void CEdSceneEditor::ValidateSceneAfterOpen( IFeedbackSystem* f )
{
	String msgSummary;

	Uint32 numFixedBlendEvents = 0;
	Uint32 numRemovedBlendEvents = 0;
	Uint32 numStrayInterpolationKeys = 0;
	Uint32 numStrayBlendKeys = 0;
	Uint32 numBadLinks = 0;

	// detect and fix issues
	for( Uint32 iSection = 0, numSections = m_storyScene->GetNumberOfSections(); iSection < numSections; ++iSection )
	{
		Uint32 numFixedBlendEventsInSection = 0;
		Uint32 numRemovedBlendEventsInSection = 0;
		m_storyScene->GetSection( iSection )->FixInvalidBlendEvents( numFixedBlendEventsInSection, numRemovedBlendEventsInSection );
		numFixedBlendEvents += numFixedBlendEventsInSection;
		numRemovedBlendEvents += numRemovedBlendEventsInSection;

		numStrayInterpolationKeys += m_storyScene->GetSection( iSection )->FixStrayInterpolationKeys();
		numStrayBlendKeys += m_storyScene->GetSection( iSection )->FixStrayBlendKeys();
		numBadLinks += m_storyScene->GetSection( iSection )->RemoveBadLinksBetweenEvents();
	}

	if( numFixedBlendEvents > 0 )
	{
		msgSummary += String::Printf( TXT("Found and fixed %u invalid blend event(s).\n"), numFixedBlendEvents );
	}

	if( numRemovedBlendEvents > 0 )
	{
		msgSummary += String::Printf( TXT("Found and removed %u invalid blend event(s) that couldn't be fixed.\n"), numRemovedBlendEvents );
	}

	if( numStrayInterpolationKeys > 0 )
	{
		msgSummary += String::Printf( TXT("Found and fixed %u stray interpolation key event(s).\n"), numStrayInterpolationKeys );
	}

	if( numStrayBlendKeys > 0 )
	{
		msgSummary += String::Printf( TXT("Found and fixed %u stray blend key event(s).\n"), numStrayBlendKeys );
	}

	if( numBadLinks > 0 )
	{
		msgSummary += String::Printf( TXT("Found and removed %u bad link(s) between events.\n"), numBadLinks );
	}

	// display summary (if any issues were found and fixed)
	if( !msgSummary.Empty() )
	{
		msgSummary += TXT( "Please save and submit scene file." );
		f->ShowMsg( TXT("Scene Editor"), msgSummary.AsChar() );
	}
}

/*
Converts old style camera blends (CStorySceneEventCameraBlend) into new style camera blends (CStorySceneCameraBlendEvent).
*/
void CEdSceneEditor::OnToolConvertCameraBlends( wxCommandEvent& event )
{
	if( !TimelineEditingEnabled() )
	{
		GFeedback->ShowMsg( TXT("Camera blend conversion"), TXT("Camera conversion - operation not allowed as timeline editing is disabled.") );
		return;
	}

	Bool allBlendsConverted = m_timeline->ConvertOldCameraBlendsToInterpolationEvents();
	
	if( !allBlendsConverted )
	{
		GFeedback->ShowMsg( TXT( "Camera blend conversion" ), TXT( "Not all camera blends were successfully converted (note that wrapped camera blends cannot be automatically converted - please convert them manually)." ) );
	}
}

void CEdSceneEditor::OnToolFixCameraBlendsEndTime( wxCommandEvent& event )
{
	GFeedback->ShowMsg( TXT("Tool"), TXT("TODO") );
}

void CEdSceneEditor::OnToolRemoveScale( wxCommandEvent& event )
{
	GFeedback->ShowMsg( TXT("Tool"), TXT("TODO") );
}

void CEdSceneEditor::OnToolScreenshotCtl( wxCommandEvent& event )
{
	if ( !m_screenshotCtrl.IsBindedToWindow() )
	{
		wxPanel* containerPanel = XRCCTRL( *this, "SmartContainerPanel", wxPanel );
		wxPanel* panel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT( "DialogScreenshotPanel" ) );
		m_mainTabs->AddPage( panel, wxT( "Screenshots" ), false );

		m_screenshotCtrl.BindToWindow( this, panel );
	}

	const Bool wasSCMode = m_screenshotCtrl.IsEnabled();
	m_screenshotCtrl.Enable( event.IsChecked() );

	if ( m_screenshotCtrl.IsEnabled() )
	{
		//m_worldCtrl.SetGameplayMode();
		//m_camera.SetGameplayMode();
	}
	else if ( wasSCMode )
	{
		//m_camera.ResetGameplayMode();
	}
}

void CEdSceneEditor::OnToolLocCtl( wxCommandEvent& event )
{
	m_locCtrl.Enable( event.IsChecked() );
	
	if ( m_locCtrl.IsEnabled() && !m_locCtrl.IsBindedToWindow() )
	{
		wxPanel* containerPanel = XRCCTRL( *this, "SmartContainerPanel", wxPanel );
		wxPanel* panel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT( "DialogLocPanel" ) );
		m_mainTabs->AddPage( panel, wxT( "Localization" ), false );

		m_locCtrl.BindToWindow( this, panel );
	}

	if ( m_locCtrl.IsEnabled() )
	{
		m_locCtrl.ParseScene( m_storyScene, GFeedback );
		m_locCtrl.RefreshWindow();

		{
			TDynArray< String > usedLangs;
			m_locCtrl.CollectUsedLangs( usedLangs );

			wxChoice* choice = XRCCTRL( *this, "locChoice", wxChoice );

			choice->Clear();

			for ( Uint32 i=0; i<usedLangs.Size(); ++i )
			{
				choice->AppendString( usedLangs[ i ].AsChar() );
			}

			choice->SetStringSelection( SLocalizationManager::GetInstance().GetCurrentLocale().AsChar() );
		}
	}
}

void CEdSceneEditor::OnLocAnalyzer( wxCommandEvent& event )
{
	m_locAnalyzer.ParseScene( m_storyScene );

	if( !m_locAnalyzer.IsBatchedExport() )
	{
		wxPanel* containerPanel = XRCCTRL( *this, "SmartContainerPanel", wxPanel );
		if( !containerPanel )
		{
			RED_ASSERT( false );
			return;
		}

		wxPanel* panel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT( "DialogLocAnalyzer" ) );
		if( !panel )
		{
			RED_ASSERT( false );
			return;
		}

		m_mainTabs->AddPage( panel, wxT( "Loc Analyzer" ), true );
		m_locAnalyzer.BindToWindow( this, panel);
	}
}

void CEdSceneEditor::OnChangeLanguage( wxCommandEvent& event )
{
	m_locPrevLang = SLocalizationManager::GetInstance().GetCurrentLocale();

	wxTheFrame->OnChangeLanguage( event );
}

void CEdSceneEditor::OnChangeLanguageToggle( wxCommandEvent& event )
{
	wxString langToSet = m_locPrevLang.AsChar();
	m_locPrevLang = SLocalizationManager::GetInstance().GetCurrentLocale();

	wxCommandEvent fake;
	fake.SetString( langToSet );

	wxChoice* choice = XRCCTRL( *this, "locChoice", wxChoice );
	choice->SetStringSelection( langToSet );

	wxTheFrame->OnChangeLanguage( fake );
}

void CEdSceneEditor::OnToolLocLinkClicked( wxHtmlLinkEvent& event )
{
	m_locCtrl.OnLinkClicked( event );
}

void CEdSceneEditor::OnToolLocLinkHover( wxHtmlCellEvent& event )
{
	m_locCtrl.OnLinkHover( event );
}

void CEdSceneEditor::OnLocAnalyzerCommonEvt( wxCommandEvent& event )
{
	m_locAnalyzer.OnCommonEvt( event );
}

void CEdSceneEditor::OnMuteSpeech( wxCommandEvent& event )
{
	const Bool flag = event.IsChecked();

	m_controller.PlaySpeechSound( !flag );
}


Vector CEdSceneEditor::CalcBestSpawnPositionWS() const
{
	Vector rayStart( Vector::ZERO_3D_POINT );
	Vector rayDirection( Vector::EY );
	Vector bestPointWS( Vector::ZERO_3D_POINT );

	if ( m_camera.CalcCameraRay( rayStart, rayDirection ) )
	{
		Float bestDist = FLT_MAX;
		Bool found = false;

		Vector segA( rayStart + rayDirection * 3.f );
		Vector segB( rayStart + rayDirection * 100.f );

		for ( auto it = m_controller.GetActorMap().Begin(), end = m_controller.GetActorMap().End(); it != end; ++it )
		{
			const THandle< CEntity > entH = (*it).m_second;
			if ( CEntity* ent = entH.Get() )
			{
				const Vector& entityPointWS = ent->GetWorldPositionRef();

				Vector linePoint;
				const Float dist = MathUtils::GeometryUtils::DistanceSqrPointToLineSeg( entityPointWS, segA, segB, linePoint );
				if ( dist < bestDist )
				{
					bestDist = dist;
					bestPointWS = linePoint;
					found = true;
				}
			}
		}

		if ( !found )
		{
			bestPointWS = rayStart + rayDirection * 5.f;
		}
	}
	return bestPointWS;
}

Vector CEdSceneEditor::CalcBestSpawnPositionSS() const
{
	Vector bestPointWS = CalcBestSpawnPositionWS();
	const EngineTransform scenePos = m_controller.GetCurrentScenePlacement();
	Matrix worldToScene;
	scenePos.CalcWorldToLocal( worldToScene );

	return worldToScene.TransformPoint( bestPointWS );
}

void CEdSceneEditor::CheckScenePlacement()
{
	TDynArray< String > logs;
	TDynArray< String > logs2;
	TDynArray< String > logs3;

	const TDynArray< CStorySceneDialogsetInstance* >& dialogsets = m_storyScene->GetDialogsetInstances();
	for ( auto iter = dialogsets.Begin(); iter != dialogsets.End(); ++iter )
	{
		TDynArray< CNode* > taggedNodes;
		GetWorld()->GetTagManager()->CollectTaggedNodes( (*iter)->GetPlacementTag() , taggedNodes, BCTO_MatchAll );

		if ( taggedNodes.Size() == 0 )
		{
			logs.PushBack( (*iter)->GetPlacementTag().ToString() );
		}
		else if ( taggedNodes.Size() > 1 )
		{
			logs2.PushBack( (*iter)->GetPlacementTag().ToString() );
		}

		for ( Int32 j=taggedNodes.SizeInt()-1; j>=0; --j )
		{
			if ( taggedNodes[ j ]->GetTransform().HasScale() )
			{
				logs3.PushBack( (*iter)->GetPlacementTag().ToString() );
			}
		}
	}

	if ( logs.Size() > 0 )
	{
		String msg;
		for ( Uint32 i=0; i<logs.Size(); ++i )
		{
			msg += logs[ i ];
			msg += TXT("\n");
		}
		GFeedback->ShowWarn( TXT("Cannot find\n [%s] \ntagged place(s)"), msg.AsChar() );
	}

	if ( logs2.Size() > 0 )
	{
		String msg;
		for ( Uint32 i=0; i<logs2.Size(); ++i )
		{
			msg += logs2[ i ];
			msg += TXT("\n");
		}
		GFeedback->ShowError( TXT("Editor found few nodes with the same scene tag:\n[%s]"), msg.AsChar() );
	}

	if ( logs3.Size() > 0 )
	{
		String msg;
		for ( Uint32 i=0; i<logs3.Size(); ++i )
		{
			msg += logs3[ i ];
			msg += TXT("\n");
		}
		GFeedback->ShowError( TXT("Editor found scale in transform for palces with tag:\n[%s]"), msg.AsChar() );
	}
}

void CEdSceneEditor::OnPreviewGridChanged( wxCommandEvent& event )
{
	if ( event.GetId() == XRCID( "pg_none" ) )
	{
		m_controller.SetPreviewGrid( SSPPCG_None );
	}
	else if ( event.GetId() == XRCID( "pg_33" ) )
	{
		m_controller.SetPreviewGrid( SSPPCG_33 );
	}
	else if ( event.GetId() == XRCID( "pg_f_lt" ) )
	{
		m_controller.SetPreviewGrid( SSPPCG_FIB_LT );
	}
	else if ( event.GetId() == XRCID( "pg_f_rt" ) )
	{
		m_controller.SetPreviewGrid( SSPPCG_FIB_RT );
	}
	else if ( event.GetId() == XRCID( "pg_f_lb" ) )
	{
		m_controller.SetPreviewGrid( SSPPCG_FIB_LB );
	}
	else if ( event.GetId() == XRCID( "pg_f_rb" ) )
	{
		m_controller.SetPreviewGrid( SSPPCG_FIB_RB );
	}
	else if ( event.GetId() == XRCID( "pg_fr" ) )
	{
		m_controller.SetPreviewGrid( SSPPCG_FIB_RATIO );
	}
	else if ( event.GetId() == XRCID( "pg_ds" ) )
	{
		m_controller.SetPreviewGrid( SSPPCG_DYN_SYMMETRY );
	}
}

void CEdSceneEditor::OnPreviewTrackSun( wxCommandEvent& event )
{
	m_preview->TrackSun( event.IsChecked() );
}


void CEdSceneEditor::OnFloatingHelpersCheckbox( wxCommandEvent& event )
{
	m_helperEntitiesCtrl.EnableFloatingHelpers( event.IsChecked() );
}

void CEdSceneEditor::OnLightPresetLoad( wxCommandEvent& event )
{
	RED_LOG( RED_LOG_CHANNEL( Feedback ), TXT( "OnLightPresetLoad: %i" ), event.GetId() );

	// create list of unique preset names
	THashMap< String, Bool > uniquePresetMap;
	TDynArray< String > presetNames;
	for( Uint32 i=0; i<m_lightPresets.Size(); ++i )
	{
		// use a temporary hash map to make sure the presets only gets added once
		if( uniquePresetMap.FindPtr( m_lightPresets[ i ].m_presetName ) != nullptr )
		{
			continue;
		}

		uniquePresetMap.Insert( m_lightPresets[ i ].m_presetName, true );
		presetNames.PushBack( m_lightPresets[ i ].m_presetName );
	}

	// get preset index
	Uint32 presetIndex = event.GetId() - wxID_LIGHT_PRESET_LOAD_FIRST;
	if( presetIndex < 0 || presetIndex > uniquePresetMap.Size() )
	{
		GFeedback->ShowError( TXT( "Unknown error trying to load preset #%i." ), presetIndex );
		return;
	}

	// create new light definitions from preset
	for( Uint32 i=0; i<m_lightPresets.Size(); ++i )
	{
		if( m_lightPresets[ i ].m_presetName != presetNames[ presetIndex ] )
		{
			continue;
		}

		// create new light def
		CStorySceneLight* lightDef = CreateObject< CStorySceneLight >( m_storyScene );

		lightDef->m_id = CName( m_lightPresets[ i ].m_lightID );
		lightDef->m_type = m_lightPresets[ i ].m_type;
		lightDef->m_innerAngle = m_lightPresets[ i ].m_innerAngle;
		lightDef->m_outerAngle = m_lightPresets[ i ].m_outerAngle;
		lightDef->m_softness = m_lightPresets[ i ].m_softness;
		lightDef->m_shadowCastingMode = m_lightPresets[ i ].m_shadowCastingMode;
		lightDef->m_shadowFadeDistance = m_lightPresets[ i ].m_shadowFadeDistance;
		lightDef->m_shadowFadeRange = m_lightPresets[ i ].m_shadowFadeRange;

		m_storyScene->AddLightDefinition( lightDef );
	}

	// refresh stuff
	if( m_actorsProvider )
	{
		m_actorsProvider->Rebuild( m_storyScene );
	}

	m_controlRequest.RequestRebuild();
	m_timeline->RequestRebuild();	
}

void CEdSceneEditor::ProcessEditorState( CEdSceneValidator::SValidationOutput& ret )
{
	ProcessSpawnedEntities( ret );

	//...
}

void CEdSceneEditor::ProcessSpawnedEntities( CEdSceneValidator::SValidationOutput& ret )
{
	const TDynArray< THandle< CEntity > >& actors = m_actorsProvider->GetActorsForEditor();
	for ( Uint32 i=0; i<actors.Size(); ++i )
	{
		if ( CEntity* e = actors[ i ].Get() )
		{
			Int32 numHeads = 0;

			if ( CActor* a = Cast< CActor >( e ) )
			{
				for ( EntityWithItemsComponentIterator< CMimicComponent > it( a ); it; ++it )
				{
					numHeads++;
				}

				if ( numHeads > 1 )
				{
					const String s = String::Printf( TXT("Actor '%s' has '%d' heads"), a->GetVoiceTag().AsChar(), numHeads );
					ret.m_messages.PushBack( CEdSceneValidator::SValidationOutputMessage( CEdSceneValidator::Error, s ) );
				}
			}
		}
	}
}
