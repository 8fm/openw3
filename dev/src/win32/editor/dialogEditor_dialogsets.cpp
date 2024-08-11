
#include "build.h"
#include "dialogEditor.h"

#include "dialogEditorActions.h"

#include "dialogEditorPage.h"
#include "gridEditor.h"
#include "gridCustomTypes.h"
#include "gridCustomColumns.h"
#include "gridPropertyWrapper.h"
#include "dialogTimeline.h"

#include "../../common/engine/gameResource.h"
#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneInput.h"
#include "../../common/game/storySceneGraphBlock.h"
#include "../../common/game/actor.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/game/storySceneLine.h"
#include "../../common/game/storyScenePlayer.h"
#include "../../common/game/storySceneDebugger.h"
#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"

#include "storyScenePreviewPlayer.h"
#include "dialogUtilityHandlers.h"
#include "dialogPreview.h"
#include "dialogEditorDialogsetPanel.h"
#include "voice.h"
#include "lipsyncDataSceneExporter.h"
#include "animFriend.h"
#include "dialogAnimationParameterEditor.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

CStorySceneDialogsetInstance* CEdSceneEditor::CreateNewDialogset()
{
	wxString dialogsetNameString = wxGetTextFromUser( 
		wxT( "Please specify name of new dialogset" ), 
		wxT( "New dialogset" ), 
		wxT( "New dialogset" ), 
		GetScreenplayPanel()->GetSceneEditor() );

	CName dialogsetName = CName( dialogsetNameString.c_str() );

	if ( GetStoryScene()->GetDialogsetByName( dialogsetName ) != NULL )
	{
		// Dialogset already created
		return NULL;
	}
	CStorySceneDialogsetInstance* dialogsetInstance = ::CreateObject< CStorySceneDialogsetInstance >( GetStoryScene() );
	dialogsetInstance->SetName( dialogsetName );

	GetStoryScene()->AddDialogsetInstance( dialogsetInstance );
	return dialogsetInstance;
}

CStorySceneDialogsetInstance* CEdSceneEditor::ReloadDialogsetFromFile( const CName& dialogsetName )
{
	CStorySceneDialogsetInstance * dialogset = GetStoryScene()->GetDialogsetByName(dialogsetName);
	if(dialogset == NULL)
	{
		return NULL;
	}
	String dialogsetResourcePath = dialogset->GetPath();
	CStorySceneDialogset* dialogsetResource = Cast< CStorySceneDialogset >( GDepot->LoadResource( dialogsetResourcePath ) );

	if ( dialogsetResource == NULL )
	{
		String message(TXT( "Cant load file from specifed filepath (do file still exists?) path:\n" ) + dialogsetResourcePath + TXT("\nDo you want to load active resource?") );
		if(GFeedback->AskYesNo(message.AsChar()))
		{
			GetActiveResource( dialogsetResourcePath );
			dialogsetResource = Cast< CStorySceneDialogset >( GDepot->LoadResource( dialogsetResourcePath ) );

			if ( dialogsetResource == NULL )
			{
				GFeedback->ShowError(TXT( "Cannot import file" ));
				return NULL;
			}			
		}
		else
		{
			return NULL;
		}
	}

	TagList placementTags = dialogset->GetPlacementTag();			// Store instance specific data (placement tags)

	GetStoryScene()->RemoveDialogsetInstance( dialogsetName );
	CStorySceneDialogsetInstance* dialogsetInstance = CStorySceneDialogsetInstance::CreateFromResource( dialogsetResource, GetStoryScene() );

	const TDynArray<CStorySceneDialogsetSlot*> targetSlots = dialogsetInstance->GetSlots();
	const TDynArray<CStorySceneDialogsetSlot*> sourceSlots = dialogset->GetSlots();

	for(unsigned int i = 0; i < targetSlots.Size() && i < sourceSlots.Size() ; i++)
	{
		targetSlots[i]->SetActorName(sourceSlots[i]->GetActorName());
	}

	dialogsetInstance->SetName( dialogsetName );
	dialogsetInstance->SetPlacementTags( placementTags );

	TDynArray< StorySceneCameraDefinition > dialogsetCameras	= dialogsetResource->GetCameraDefinitions();

	for ( Uint32 i = 0; i < dialogsetCameras.Size(); ++i )
	{
		dialogsetCameras[i].ParseCameraParams();
		GetStoryScene()->AddCameraDefinition(dialogsetCameras[i]);
	}

	GetStoryScene()->AddDialogsetInstance( dialogsetInstance );
	RefreshCamerasList();
	return dialogsetInstance;
}

CStorySceneDialogsetInstance* CEdSceneEditor::CreateDialogsetFromFile()
{
	String dialogsetResourcePath;
	GetActiveResource( dialogsetResourcePath );
	CStorySceneDialogset* dialogsetResource = Cast< CStorySceneDialogset >( GDepot->LoadResource( dialogsetResourcePath ) );

	if ( dialogsetResource == NULL )
	{
		wxString message;
		message.Printf( wxT( "You did not select a resource of type \"%s\" (Ext. \"%s\") in the asset browser" ), CStorySceneDialogset::GetFriendlyFileDescription(), CStorySceneDialogset::GetFileExtension() );
		wxMessageBox( message, wxT( "Cannot import file" ), wxOK | wxICON_ERROR );

		return NULL;
	}

	wxString dialogsetNameString = wxGetTextFromUser
		(
		wxT( "Please specify name of new dialogset" ),
		wxT( "New dialogset" ),
		dialogsetResource->GetFile()->GetFileName().AsChar(),
		GetScreenplayPanel()->GetSceneEditor()
		);

	CName dialogsetName = CName( dialogsetNameString.c_str() );

	if ( GetStoryScene()->GetDialogsetByName( dialogsetName ) != NULL )
	{
		// Dialogset already created
		wxString message;
		message.Printf( wxT( "%s with name \"%s\" already exists" ), CStorySceneDialogset::GetFriendlyFileDescription(), dialogsetNameString );
		wxMessageBox( message, wxT( "Cannot import file" ), wxOK | wxICON_ERROR );

		return NULL;
	}

	CStorySceneDialogsetInstance* dialogsetInstance = CStorySceneDialogsetInstance::CreateFromResource( dialogsetResource, GetStoryScene() );
	dialogsetInstance->SetName( dialogsetName );

	TDynArray< StorySceneCameraDefinition > dialogsetCameras = dialogsetResource->GetCameraDefinitions();
	for ( Uint32 i = 0; i < dialogsetCameras.Size(); ++i )
	{
		if ( GetStoryScene()->GetCameraDefinition( dialogsetCameras[ i ].m_cameraName ) != NULL )
		{
			continue;
		}
		dialogsetCameras[i].ParseCameraParams();
		GetStoryScene()->AddCameraDefinition( dialogsetCameras[ i ] );
	}

	GetStoryScene()->AddDialogsetInstance( dialogsetInstance );
	RefreshCamerasList();
	return dialogsetInstance;
}

CStorySceneDialogsetInstance* CEdSceneEditor::CreateDialogsetFromPreviousSection( CStorySceneSection* currentSection )
{
	return NULL;
}

CStorySceneDialogsetInstance* CEdSceneEditor::CreateDialogsetFromCurrentSection( CStorySceneSection* currentSection )
{
	if ( currentSection == NULL )
	{
		return NULL;
	}

	const CStorySceneDialogsetInstance* dialogsetInstance = m_storyScene->GetFirstDialogsetAtSection( currentSection );
	return CreateDialogsetCopy( dialogsetInstance );
}

CStorySceneDialogsetInstance* CEdSceneEditor::CreateDialogsetCopy( const CStorySceneDialogsetInstance* dialogsetInstance )
{
	if ( dialogsetInstance == NULL )
	{
		return NULL;
	}

	wxString newDialogsetNameString = wxGetTextFromUser
		(
		wxT( "Please specify name of new dialogset" ),
		wxT( "Copy dialogset" ),
		wxString::Format( wxT( "%s(copy)" ),
		dialogsetInstance->GetName().AsString().AsChar() ), 
		GetScreenplayPanel()->GetSceneEditor()
		);

	if( !newDialogsetNameString.IsEmpty() )
	{
		CName newDialogsetName = CName( newDialogsetNameString.c_str() );

		CStorySceneDialogsetInstance* clonedInstance = Cast< CStorySceneDialogsetInstance >( dialogsetInstance->Clone( GetStoryScene() ) );
		clonedInstance->SetName( newDialogsetName );
		clonedInstance->OnCloned();

		GetStoryScene()->AddDialogsetInstance( clonedInstance );
		return clonedInstance;
	}

	return NULL;
}

void CEdSceneEditor::SaveDialogsetToFile( const CName& dialogsetName )
{
	CStorySceneDialogsetInstance* dialogsetInstance = GetStoryScene()->GetDialogsetByName( dialogsetName );
	if ( dialogsetInstance == NULL )	
	{
		return;
	}

	String sceneFileDirectoryPath;
	GetStoryScene()->GetFile()->GetDirectory()->GetAbsolutePath( sceneFileDirectoryPath );

	wxString filePathString = wxFileSelector
		(
		wxT( "Choose dialogset file location" ),
		sceneFileDirectoryPath.AsChar(),
		dialogsetName.AsString().AsChar(),
		ResourceExtension< CStorySceneDialogset >(),
		wxString::Format
		(
		wxT( "Dialogset files (*.%s)|*.%s" ),
		ResourceExtension< CStorySceneDialogset >(),
		ResourceExtension< CStorySceneDialogset >()
		),
		wxFD_SAVE,
		this
		);

	String fileName = filePathString.AfterLast( Char( '\\' ) ).c_str();

	dialogsetInstance->SetPath(String(filePathString.c_str()));

	wxString depotDirectoryString;
	if ( filePathString.StartsWith( GDepot->GetRootDataPath().AsChar(), &depotDirectoryString ) == false )
	{
		// save location not in depot
		return;
	}

	//wxSaveFileSelector( wxT( "what what what" ), ResourceExtension< CStorySceneDialogset >(), dialogsetName.AsChar(), this );

	CStorySceneDialogset* dialogsetResource = ::CreateObject< CStorySceneDialogset >();
	dialogsetResource->SetName( dialogsetName );


	TDynArray<String>	questions(3);
	TDynArray<Bool>		answers(3);
	questions[0] = TXT("Save actor state");
	answers[0] = false;
	questions[1] = TXT("Save actor visibility");
	answers[1] = false;
	questions[2] = TXT("Save actor action");
	answers[2] = false;

	if(! MultiBoolDialog(this, TXT("Properites to save") , questions, answers))
	{
		//user pressed cancel do not save
		return;
	}

	const TDynArray< CStorySceneDialogsetSlot* > instanceSlots = dialogsetInstance->GetSlots();
	for ( Uint32 i = 0; i < instanceSlots.Size(); ++i )
	{
		CStorySceneDialogsetSlot* resourceSlot = Cast<CStorySceneDialogsetSlot>( instanceSlots[ i ]->Clone( dialogsetResource ) );
		resourceSlot->SetActorName(CName::NONE);

		//if(! answers[0] )resourceSlot->SetActorState( CNAME( Neutral ) );
		if(! answers[1] )resourceSlot->SetActorVisibility(true);
		if(! answers[2] )resourceSlot->SetNoSetupActions();

		dialogsetResource->AddSlot( resourceSlot );
	}

	const TDynArray< StorySceneCameraDefinition >& sceneCameras = GetStoryScene()->GetCameraDefinitions();
	for ( Uint32 j = 0; j < sceneCameras.Size(); ++j )
	{
		dialogsetResource->AddCamera( sceneCameras[ j ] );
	}

	dialogsetResource->SaveAs( GDepot->FindPath( depotDirectoryString.c_str() ), fileName );
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
