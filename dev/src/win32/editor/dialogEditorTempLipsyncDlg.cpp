// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "dialogEditorTempLipsyncDlg.h"
#include "dialogEditor.h"
#include "dialogEditorUtils.h"
#include "../../common/engine/localizationManager.h"

wxBEGIN_EVENT_TABLE( CEdDialogEditorTempLipsyncDlg, wxDialog )
	EVT_BUTTON( XRCID("buttonDelete"), CEdDialogEditorTempLipsyncDlg::OnDeleteButton )
	EVT_BUTTON( XRCID("buttonRecreate"), CEdDialogEditorTempLipsyncDlg::OnRecreateButton )
	EVT_BUTTON( XRCID("buttonClose"), CEdDialogEditorTempLipsyncDlg::OnCloseButton )
wxEND_EVENT_TABLE()

/*
Ctor.

\param sceneEditor Scene editor. Must not be nulltpr.
*/
CEdDialogEditorTempLipsyncDlg::CEdDialogEditorTempLipsyncDlg(CEdSceneEditor* mediator)
: m_mediator( mediator )
{
	wxXmlResource::Get()->LoadDialog( this, nullptr, "DialogTemporaryLipsync" );
}

/*
Dtor.
*/
CEdDialogEditorTempLipsyncDlg::~CEdDialogEditorTempLipsyncDlg()
{}

/*
Called when "Delete" button is clicked.
*/
void CEdDialogEditorTempLipsyncDlg::OnDeleteButton( wxCommandEvent& event )
{
	TDynArray< const CStorySceneSection* > sectionsToProcess;
	GetSectionsToProcess( sectionsToProcess );
	Uint32 numSectionsToProcess = sectionsToProcess.Size();

	wxStaticText* statsSectionsProcessed = XRCCTRL( *this, "staticTextSectionsProcessedValue", wxStaticText );
	wxStaticText* statsLipsyncsRecreated = XRCCTRL( *this, "staticTextLipsyncsProcessedValue", wxStaticText );

	String languageId = SLocalizationManager::GetInstance().GetCurrentLocale();
	Uint32 numSectionsProcessed = 0;
	Uint32 numLipsyncsProcessed = 0;
	wxString stats;

	for( auto it = sectionsToProcess.Begin(), end = sectionsToProcess.End(); it != end; ++it )
	{
		const CStorySceneSection* section = *it;
		numLipsyncsProcessed += StorySceneEditorUtils::DeleteTemporaryLipsync( section, languageId );
		++numSectionsProcessed;

		// update stats
		stats.Printf( "%u / %u", numSectionsProcessed, numSectionsToProcess );
		statsSectionsProcessed->SetLabel( stats );
		stats.Printf( "%u", numLipsyncsProcessed );
		statsLipsyncsRecreated->SetLabel( stats );
	}

	m_mediator->OnTempLipsyncDlg_RefreshLipsyncs();
}

/*
Called when "Recreate" button is clicked.
*/
void CEdDialogEditorTempLipsyncDlg::OnRecreateButton( wxCommandEvent& event )
{
	TDynArray< const CStorySceneSection* > sectionsToProcess;
	GetSectionsToProcess( sectionsToProcess );
	Uint32 numSectionsToProcess = sectionsToProcess.Size();

	wxStaticText* statsSectionsProcessed = XRCCTRL( *this, "staticTextSectionsProcessedValue", wxStaticText );
	wxStaticText* statsLipsyncsRecreated = XRCCTRL( *this, "staticTextLipsyncsProcessedValue", wxStaticText );

	String languageId = SLocalizationManager::GetInstance().GetCurrentLocale();
	Uint32 numSectionsProcessed = 0;
	Uint32 numLipsyncsProcessed = 0;
	wxString stats;

	for( auto it = sectionsToProcess.Begin(), end = sectionsToProcess.End(); it != end; ++it )
	{
		const CStorySceneSection* section = *it;
		numLipsyncsProcessed += StorySceneEditorUtils::CreateTemporaryLipsync( section, languageId, true, true );
		++numSectionsProcessed;

		// update stats
		stats.Printf( "%u / %u", numSectionsProcessed, numSectionsToProcess );
		statsSectionsProcessed->SetLabel( stats );
		stats.Printf( "%u", numLipsyncsProcessed );
		statsLipsyncsRecreated->SetLabel( stats );
	}

	m_mediator->OnTempLipsyncDlg_RefreshLipsyncs();
}

/*
Called when "Close" button is clicked.
*/
void CEdDialogEditorTempLipsyncDlg::OnCloseButton( wxCommandEvent& event )
{
	EndModal( XRCID( "buttonClose" ) );
}

/*
Creates list of sections to process.
*/
void CEdDialogEditorTempLipsyncDlg::GetSectionsToProcess( TDynArray< const CStorySceneSection* >& sectionsToProcess ) const
{
	// check whether to process current section only or all sections
	Bool processCurrentSectionOnly = XRCCTRL( *this, "radioBtnCurrentSection", wxRadioButton )->GetValue();

	if( processCurrentSectionOnly )
	{
		const CStorySceneSection* currentSection = m_mediator->OnTempLipsyncDlg_GetCurrentSection();
		if( currentSection )
		{
			sectionsToProcess.PushBack( currentSection );
		}
	}
	else // process all sections
	{
		CStoryScene* scene = m_mediator->OnTempLipsyncDlg_GetScene();
		if( scene )
		{
			Uint32 numSections = scene->GetNumberOfSections();
			for( Uint32 i = 0; i < numSections; ++i )
			{
				sectionsToProcess.PushBack( scene->GetSection( i ));
			}
		}
	}
}
