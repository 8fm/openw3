/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
#ifndef NO_MARKER_SYSTEMS

#include "../core/clipboardBase.h"
#include "../../common/engine/reviewSystem.h"
#include "freeCamera.h"
#include "redGuiSeparator.h"
#include "redGuiList.h"
#include "redGuiLabel.h"
#include "redGuiPanel.h"
#include "redGuiButton.h"
#include "redGuiCheckBox.h"
#include "redGuiComboBox.h"
#include "redGuiImage.h"
#include "redGuiTextBox.h"
#include "redGuiProgressBox.h"
#include "redGuiSpin.h"
#include "redGuiOpenFileDialog.h"
#include "redGuiManager.h"
#include "redGuiGridLayout.h"
#include "debugWindowsManager.h"
#include "debugWindowReviewMarkersShowMarker.h"
#include "debugWindowReviewMarkers.h"
#include "game.h"
#include "bitmapTexture.h"
#include "entity.h"

namespace DebugWindows
{
	CDebugWindowReviewMarkersShowMarker::CDebugWindowReviewMarkersShowMarker() 
		: RedGui::CRedGuiWindow( 200, 200, 400, 500 )
	{
		SetCaption( TXT("Review markers system - Show marker") );

		// create all controls contain by the review marker debug window
		CreateControls();
	}

	CDebugWindowReviewMarkersShowMarker::~CDebugWindowReviewMarkersShowMarker()
	{
		/* intentionally empty */
	}

	void CDebugWindowReviewMarkersShowMarker::CreateControls()
	{
		m_editMarker = new RedGui::CRedGuiButton( 0, 0, 100, 20 );
		m_editMarker->SetMargin( Box2(5, 5, 5, 5) );
		m_editMarker->SetText( TXT("Edit marker") );
		m_editMarker->SetDock( RedGui::DOCK_Bottom );
		m_editMarker->SetTabIndex( 3 );
		m_editMarker->EventButtonClicked.Bind( this, &CDebugWindowReviewMarkersShowMarker::NotifyOnEditButtonClicked );
		AddChild( m_editMarker );

		// informations
		RedGui::CRedGuiPanel* infoPanel = new RedGui::CRedGuiPanel( 0, 0, GetWidth(), GetHeight() );
		infoPanel->SetBorderVisible( false );
		infoPanel->SetMargin( Box2(5, 5, 5, 5) );
		infoPanel->SetDock(RedGui::DOCK_Fill);
		AddChild(infoPanel);
		{
			RedGui::CRedGuiPanel* topInfoPanel = new RedGui::CRedGuiPanel( 0, 0, GetWidth(), 150 );
			topInfoPanel->SetBackgroundColor( Color::CLEAR );
			topInfoPanel->SetBorderVisible( false );
			infoPanel->AddChild( topInfoPanel );
			topInfoPanel->SetDock( RedGui::DOCK_Top );

			// panel for labels
			{
				RedGui::CRedGuiGridLayout* layout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
				layout->SetBackgroundColor( Color(20, 20, 20, 255) );
				layout->SetDock( RedGui::DOCK_Fill );
				layout->SetDimensions( 1, 7 );
				topInfoPanel->AddChild( layout );
				{
					m_showFlagSummary = new RedGui::CRedGuiLabel(0,0, 100, 15);
					m_showFlagSummary->SetMargin(Box2(10, 3, 10, 3));
					m_showFlagSummary->SetDock(RedGui::DOCK_Top);
					m_showFlagSummary->SetText(TXT("Summary: "));
					layout->AddChild(m_showFlagSummary);

					m_showFlagAuthor = new RedGui::CRedGuiLabel(0,0, 100, 15);
					m_showFlagAuthor->SetMargin(Box2(10, 3, 10, 3));
					m_showFlagAuthor->SetDock(RedGui::DOCK_Top);
					m_showFlagAuthor->SetText(TXT("Created by: "));
					layout->AddChild(m_showFlagAuthor);

					m_showFlagCreated = new RedGui::CRedGuiLabel(0,0, 100, 15);
					m_showFlagCreated->SetMargin(Box2(10, 3, 10, 3));
					m_showFlagCreated->SetDock(RedGui::DOCK_Top);
					m_showFlagCreated->SetText(TXT("Created date: "));
					layout->AddChild(m_showFlagCreated);

					m_showFlagTTPNumber = new RedGui::CRedGuiLabel(0,0, 100, 15);
					m_showFlagTTPNumber->SetMargin(Box2(10, 3, 10, 3));
					m_showFlagTTPNumber->SetDock(RedGui::DOCK_Top);
					m_showFlagTTPNumber->SetText(TXT("TTP number: "));
					layout->AddChild(m_showFlagTTPNumber);

					m_showFlagType = new RedGui::CRedGuiLabel(0,0, 100, 15);
					m_showFlagType->SetMargin(Box2(10, 3, 10, 3));
					m_showFlagType->SetDock(RedGui::DOCK_Top);
					m_showFlagType->SetText(TXT("Type: "));
					layout->AddChild(m_showFlagType);

					m_showFlagPriority = new RedGui::CRedGuiLabel(0,0, 100, 15);
					m_showFlagPriority->SetMargin(Box2(10, 3, 10, 3));
					m_showFlagPriority->SetDock(RedGui::DOCK_Top);
					m_showFlagPriority->SetText(TXT("Current Priority: "));
					layout->AddChild(m_showFlagPriority);

					m_showFlagState = new RedGui::CRedGuiLabel(0,0, 100, 15);
					m_showFlagState->SetMargin(Box2(10, 3, 10, 3));
					m_showFlagState->SetDock(RedGui::DOCK_Top);
					m_showFlagState->SetText(TXT("Current state: "));
					layout->AddChild(m_showFlagState);
				}
			}

			// add separator
			RedGui::CRedGuiSeparator* separator = new RedGui::CRedGuiSeparator( 0, 0, GetWidth(), 1 );
			infoPanel->AddChild( separator );
			separator->SetDock( RedGui::DOCK_Top );

			// information bottom panel
			RedGui::CRedGuiPanel* bottomInfoPanel = new RedGui::CRedGuiPanel( 0, 0, GetWidth(), 160 );
			bottomInfoPanel->SetBackgroundColor( Color::CLEAR );
			bottomInfoPanel->SetBorderVisible( false );
			bottomInfoPanel->SetMargin( Box2( 5, 5, 5, 5 ) );
			bottomInfoPanel->SetDock( RedGui::DOCK_Fill );
			infoPanel->AddChild( bottomInfoPanel );
			{
				m_comments = new RedGui::CRedGuiList( 0, 0, 150, 200 );
				bottomInfoPanel->AddChild( m_comments );
				m_comments->SetMargin( Box2( 3, 3, 3, 3 ) );
				m_comments->SetDock( RedGui::DOCK_Left );
				m_comments->AppendColumn( TXT("Comments"), 100 );
				m_comments->SetTabIndex( 0 );
				m_comments->EventSelectedItem.Bind( this, &CDebugWindowReviewMarkersShowMarker::NotifyOnCommentSelected );

				// info about comment
				{
					RedGui::CRedGuiPanel* topCommentInfo = new RedGui::CRedGuiPanel(0,0, GetWidth(), 75);
					topCommentInfo->SetBackgroundColor(Color(20, 20, 20, 255));
					topCommentInfo->SetMargin(Box2(5,5,5,5));
					bottomInfoPanel->AddChild(topCommentInfo);
					topCommentInfo->SetDock(RedGui::DOCK_Top);

					m_commentAuthor = new RedGui::CRedGuiLabel(0,0, 100, 15);
					topCommentInfo->AddChild(m_commentAuthor);
					m_commentAuthor->SetMargin(Box2(5, 5, 5, 3));
					m_commentAuthor->SetDock(RedGui::DOCK_Top);
					m_commentAuthor->SetText(TXT("Author: "));

					m_commentState = new RedGui::CRedGuiLabel(0,0, 100, 15);
					topCommentInfo->AddChild(m_commentState);
					m_commentState->SetMargin(Box2(5, 3, 5, 3));
					m_commentState->SetDock(RedGui::DOCK_Top);
					m_commentState->SetText(TXT("State: "));

					m_commentPriority = new RedGui::CRedGuiLabel(0,0, 100, 15);
					topCommentInfo->AddChild(m_commentPriority);
					m_commentPriority->SetMargin(Box2(5, 3, 5, 5));
					m_commentPriority->SetDock(RedGui::DOCK_Top);
					m_commentPriority->SetText(TXT("Priority: "));

#ifdef RED_PLATFORM_WINPC
					RedGui::CRedGuiButton* copyScreenPath= new RedGui::CRedGuiButton( 0, 0, 100, 15 );
					copyScreenPath->SetMargin( Box2(5, 5, 5, 5) );
					copyScreenPath->SetText( TXT("Copy screen path") );
					copyScreenPath->SetDock( RedGui::DOCK_Bottom );
					copyScreenPath->SetTabIndex( 1 );
					copyScreenPath->EventButtonClicked.Bind( this, &CDebugWindowReviewMarkersShowMarker::NotifyCopyScreenPathClicked );
					topCommentInfo->AddChild( copyScreenPath );
#endif	// RED_PLATFORM_WINPC

					m_commentDescription = new RedGui::CRedGuiTextBox(0,0, 100, 15);
					m_commentDescription->SetMargin(Box2(5, 5, 5, 5));
					m_commentDescription->SetDock(RedGui::DOCK_Fill);
					m_commentDescription->SetMultiLine(true);
					m_commentDescription->SetReadOnly(true);
					m_commentDescription->SetTabIndex( 2 );
					bottomInfoPanel->AddChild(m_commentDescription);
				}
			}
		}
	}

	void CDebugWindowReviewMarkersShowMarker::NotifyOnEditButtonClicked( RedGui::CRedGuiEventPackage& package )
	{
		CDebugWindowReviewMarkers* reviewWindow = static_cast< CDebugWindowReviewMarkers* >( m_parentWindow );
		reviewWindow->EditFlagDirectly( m_flag );
		SetVisible( false );
	}

	void CDebugWindowReviewMarkersShowMarker::NotifyCopyScreenPathClicked( RedGui::CRedGuiEventPackage& package )
	{
		const Uint32 commentCount = m_flag->m_comments.Size();
		const CReviewFlagComment& lastComment = m_flag->m_comments[commentCount-1];

		// get text from clipboard
		if( GClipboard->Open( CF_Text ) == false )
		{
			return;
		}

		GClipboard->Copy( lastComment.m_pathToScreen );
		GClipboard->Close();
	}

	void CDebugWindowReviewMarkersShowMarker::OnWindowClosed( CRedGuiControl* control )
	{
		if( m_parentWindow != nullptr )
		{
			m_parentWindow->SetVisible( true );
			m_parentWindow = nullptr;
			m_flag = nullptr;
		}

		m_attachedMarkerSystem->UnlockUpdate();
	}

	void CDebugWindowReviewMarkersShowMarker::SetParentWindow( RedGui::CRedGuiWindow* parentWindow )
	{
		m_parentWindow = parentWindow;
	}

	void CDebugWindowReviewMarkersShowMarker::NotifyOnCommentSelected( RedGui::CRedGuiEventPackage& package, Int32 selectedItemIndex )
	{
		if( m_flag != nullptr && selectedItemIndex != -1 )
		{
			if( selectedItemIndex < (Int32)m_flag->m_comments.Size() )
			{
				UpdateSelectedCommentInfo( selectedItemIndex );
			}
		}
	}

	void CDebugWindowReviewMarkersShowMarker::SetFlagToShow( CReviewSystem* reviewSystem, CReviewFlag* flag )
	{
		m_attachedMarkerSystem = reviewSystem;
		m_attachedMarkerSystem->LockUpdate();
		m_flag = flag;

		// select first row in comments list
		RedGui::CRedGuiEventPackage package( this );
		NotifyOnCommentSelected( package, 0 );
		m_comments->SetSelection( 0 );
	}

	void CDebugWindowReviewMarkersShowMarker::FillInformation()
	{
		if( m_flag != nullptr )
		{
			m_showFlagSummary->SetText(TXT("Summary: ") + m_flag->m_summary);

			Uint32 newestCommentIndex = m_flag->m_comments.Size()-1;
			m_showFlagPriority->SetText(TXT("Current priority: ") + ToString(m_flag->m_comments[newestCommentIndex].m_priority));

			TDynArray<String> states;
			m_attachedMarkerSystem->GetStateList( states );
			Int32 stateIndex = m_flag->m_comments[newestCommentIndex].m_state;
			if( stateIndex < 0 || stateIndex > Int32 ( states.Size() - 1 ) )
			{
				m_showFlagState->SetText( TXT("Incorrect state, please fix it.") );
			}
			else
			{
				m_showFlagState->SetText( TXT("Current state: ") + states[stateIndex] );
			}

			TDynArray<String> types;
			m_attachedMarkerSystem->GetBugTypeList( types );
			Int32 typeIndex = m_flag->m_type;
			if( typeIndex < 0 || typeIndex > (Int32)(types.Size() - 1 ) )
			{
				m_showFlagType->SetText( TXT("Incorrect type, please fix it.") );
			}
			else
			{
				m_showFlagType->SetText( TXT("Type: ") + types[typeIndex] );
			}

			m_showFlagAuthor->SetText(TXT("Created by: ") + m_flag->m_comments[0].m_author);
			m_showFlagTTPNumber->SetText(TXT("TTP number: ") + ToString(m_flag->m_testTrackNumber));

			tm* localTime = &m_flag->m_comments[0].m_creationDate;
			String dateTime = ToString(localTime->tm_year)+TXT("-")+ToString(localTime->tm_mon)+TXT("-")+ToString(localTime->tm_mday)+TXT("_");
			dateTime += ToString(localTime->tm_hour)+TXT(":")+ToString(localTime->tm_min)+TXT(":")+ToString(localTime->tm_sec);
			dateTime.ReplaceAll(TXT("_"), TXT(" "), true);
			m_showFlagCreated->SetText(TXT("Created date: ") + dateTime);

			// add comments
			m_comments->RemoveAllItems();
			Uint32 commentCount = m_flag->m_comments.Size();
			for (Uint32 i=0; i<commentCount; ++i)
			{
				tm* localTime = &m_flag->m_comments[i].m_creationDate;
				String dateTime = ToString(localTime->tm_year)+TXT("-")+ToString(localTime->tm_mon)+TXT("-")+ToString(localTime->tm_mday)+TXT("_");
				dateTime += ToString(localTime->tm_hour)+TXT(":")+ToString(localTime->tm_min)+TXT(":")+ToString(localTime->tm_sec);
				dateTime.ReplaceAll(TXT("_"), TXT(" "), true);
				m_comments->AddItem(dateTime);
			}

			m_comments->SetSelection( commentCount-1 );
			UpdateSelectedCommentInfo( commentCount-1 );
		}
	}

	void CDebugWindowReviewMarkersShowMarker::OnWindowOpened( CRedGuiControl* control )
	{
		ClearControls();
		FillInformation();
	}

	void CDebugWindowReviewMarkersShowMarker::ClearControls()
	{
		m_comments->RemoveAllItems();
		m_showFlagSummary->ClearText();
		m_showFlagAuthor->ClearText();
		m_showFlagPriority->ClearText();
		m_showFlagState->ClearText();
		m_showFlagType->ClearText();
		m_showFlagCreated->ClearText();
		m_showFlagTTPNumber->ClearText();
		m_commentAuthor->ClearText();
		m_commentState->ClearText();
		m_commentPriority->ClearText();
		m_commentDescription->SetText( TXT("") );
	}

	void CDebugWindowReviewMarkersShowMarker::UpdateSelectedCommentInfo( Int32 selectedItemIndex )
	{
		CReviewFlagComment* selectedComment = &m_flag->m_comments[selectedItemIndex];
		m_commentAuthor->SetText(TXT("Author: ") + selectedComment->m_author);
		TDynArray<String> states;
		m_attachedMarkerSystem->GetStateList( states );
		m_commentState->SetText(TXT("State: ") + states[selectedComment->m_state-1]);
		m_commentPriority->SetText(TXT("Priority: ") + ToString(selectedComment->m_priority));
		m_commentDescription->SetText(selectedComment->m_description);

		// change flag entity position
		m_flag->m_flagEntity->SetPosition( selectedComment->m_flagPosition );

		// set camera position and rotation
		EulerAngles eulerAngles(selectedComment->m_cameraOrientation.X,selectedComment->m_cameraOrientation.Y,selectedComment->m_cameraOrientation.Z);
		GGame->EnableFreeCamera(true);
		CGameFreeCamera& freeCamera = const_cast<CGameFreeCamera&>(GGame->GetFreeCamera());
		freeCamera.MoveTo(selectedComment->m_cameraPosition, eulerAngles);
	}

}	// namespace DebugWindows

#endif	// NO_MARKER_SYSTEMS
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
