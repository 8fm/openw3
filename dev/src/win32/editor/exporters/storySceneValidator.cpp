#include "build.h"
#include "storySceneValidator.h"

#ifndef NO_EDITOR

#include "../../../common/game/storyScene.h"
#include "../../../common/game/storySceneGraph.h"

#include "../../../common/core/depot.h"
#include "../../../common/core/feedback.h"
#include "../../../common/core/objectGC.h"
#include "../sceneValidator.h"
#include "../utils.h"
#include "wx/webview.h"
#include "wx/filedlg.h"
#include "wx/xrc/xmlres.h"
#include "wx/textfile.h"


RED_DEFINE_STATIC_NAME( SceneValidator )

CStorySceneValidator::CStorySceneValidator()
{
}

namespace 
{
	String IndexToFile( Uint32 index )
	{
		return TXT( "validationData\\" ) + ToString( index ) + TXT(".html");
	}

	Int32 FileToindex( const String& file )
	{
		String temp, token;
		file.Split( TXT("\\"), nullptr, &temp, true );
		temp.Split( TXT("."), &token, nullptr );
		Int32 index;
		if( FromString<Int32>( token, index ) )
		{
			return index;
		}
		return -1;	
	}
}

void CStorySceneValidator::Initialize()
{
	GFeedback->BeginTask( TXT( "Scanning depot" ), true );	
	BatchExportGroup	rootExportGroup;
	FillRootExportGroup( rootExportGroup );
	for (Uint32 i = 0; i < rootExportGroup.m_subGroups.Size(); i++ )
	{
		if( rootExportGroup.m_subGroups[ i ].m_groupName == TXT("quests") )
		{
			FillSceneFiles( rootExportGroup.m_subGroups[ i ], m_sceneFiles );
		}		
	}

    CEdSceneValidator validator;
	TDynArray<CEdSceneValidator::SValidationOutput> outData;
	outData.Resize( m_sceneFiles.Size() );

	for ( Uint32 i = 0; i < m_sceneFiles.Size(); ++i )
	{	
		const CStoryScene* scene = SafeCast<CStoryScene>( GDepot->LoadResource( m_sceneFiles[i] ) );		
		outData[i] = validator.Process( scene );
		if ( i % 25 == 0 )
		{
			String message = String::Printf( TXT("Processing scene files %d/%d"), i, m_sceneFiles.Size() );
			GFeedback->UpdateTaskInfo( message.AsChar() );
			GFeedback->UpdateTaskProgress( i, m_sceneFiles.Size() );
			if( GFeedback->IsTaskCanceled() )
			{
				break;
			}
			if( i % 1000 == 999 )
			{
				GFeedback->UpdateTaskInfo( TXT("Freeing memory") );
				GObjectGC->CollectNow();
			}
		}	
	}

	TDynArray<String> links;
	TDynArray< CEdSceneValidator::SValidationOutputMessage >  messages;

	for ( Uint32 i = 0; i < outData.Size(); i++ )
	{
		CEdSceneValidator::SValidationOutput& msg = outData[i];
		if ( msg.m_messages.Size() > 0 )
		{
			CEdSceneValidator::MessageType msgType = msg.NumOfErrors() > 0 ? CEdSceneValidator::Error : msg.NumOfWarnings() > 0  ? CEdSceneValidator::Warning : CEdSceneValidator::Info;
			messages.PushBack( CEdSceneValidator::SValidationOutputMessage( msgType, m_sceneFiles[i] ) );
			links.PushBack( IndexToFile( i ) );
		}			
	}

	String htmlReport = CEdSceneValidator::GenerateHTMLReport( messages, &links );

	GFeedback->EndTask();

	class SceneReportsHandler : public SimpleWebViewHandler
	{
	public:
		TDynArray<CEdSceneValidator::SValidationOutput>& data;
		SceneReportsHandler( TDynArray<CEdSceneValidator::SValidationOutput>& _data ) : data( _data )
		{}

		virtual void HandlePage( const String& uri )
		{

			Int32 index = FileToindex( uri );
			if( index >= 0 )
			{
				HtmlBox( nullptr, TXT("Validation result"), CEdSceneValidator::GenerateHTMLReport( data[index].m_messages ) );
			}
		}
	} handler( outData );

	HtmlBox( nullptr, TXT("Validation result"), htmlReport, false, &handler );

	wxFileDialog saveFileDialog( nullptr, TXT("Save result file"), "", "index","html files (*.html)|*.html", wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
	if ( saveFileDialog.ShowModal() == wxID_CANCEL )
		return;

	wxString path = saveFileDialog.GetPath();
	if( path != wxEmptyString )
	{
		String htmlReport = CEdSceneValidator::GenerateHTMLReport( messages, &links, true );
		wxString dir, filename, ext;
		wxFileName::SplitPath( path, &dir, &filename, &ext );
		dir += TXT("\\");

		wxTextFile file( path );
		if( !file.Exists() )
		{
			file.Create();
		}
		file.Open();
		file.Clear();
		file.AddLine( htmlReport.AsChar() );
		file.Write();
		file.Close();

		
		wxString dataDir = dir + TXT("validationData\\");
		if( !wxDirExists( dataDir ) )
		{
			wxMkDir( dataDir );
		}
		for ( Uint32 i = 0; i < outData.Size(); i++ )
		{
			CEdSceneValidator::SValidationOutput& msg = outData[i];
			if ( msg.m_messages.Size() > 0 )
			{
				wxTextFile file( dir + IndexToFile( i ).AsChar() );
				if( !file.Exists() )
				{
					file.Create();
				}
				file.Open();
				file.Clear();
				file.AddLine( CEdSceneValidator::GenerateHTMLReport( outData[i].m_messages, nullptr, true ).AsChar() );
				file.Write();
				file.Close(); 
			}			
		}
	}
}

void CStorySceneValidator::FillSceneFiles( const BatchExportGroup& group, TDynArray< String >& files )
{
	for (Uint32 i = 0; i < group.m_groupEntries.Size(); i++ )
	{
		files.PushBack( group.m_groupEntries[ i ] );
	}
	for (Uint32 i = 0; i < group.m_subGroups.Size(); i++ )
	{
		const String& name = group.m_subGroups[i].m_groupName;

		if( name != TXT("dialogues_junk") && name != TXT("demo_files") && name != TXT("voicesets") )
		{
			FillSceneFiles( group.m_subGroups[ i ], files );
		}		
	}
}

void CStorySceneValidator::FillRootExportGroup( BatchExportGroup& exportGroup )
{
	AddDirectoryToBatchGroup( GDepot, exportGroup );
}

void CStorySceneValidator::AddDirectoryToBatchGroup( CDirectory* directory, BatchExportGroup& batchGroup )
{
	if ( directory == NULL )
	{
		return;
	}

	batchGroup.m_groupName = directory->GetName();

	String filename;
	String depotPath;

	const TFiles & directoryFiles = directory->GetFiles();
	for ( TFiles::const_iterator fileIter = directoryFiles.Begin();
		fileIter != directoryFiles.End(); ++fileIter )
	{
		CDiskFile* file = (*fileIter);


		//if ( filename.EndsWith( GetResourceExtension() ) == false )
		if ( CanExportFile( file ) == false )
		{
			continue;
		}

		filename = file->GetFileName();
		depotPath = file->GetDepotPath();
		batchGroup.m_groupEntries.PushBack( depotPath );
	}

	for( CDirectory* dir : directory->GetDirectories() )
	{
		BatchExportGroup subGroup;
		batchGroup.m_subGroups.PushBack( subGroup );

		AddDirectoryToBatchGroup( dir, batchGroup.m_subGroups.Back() );		
	}
}

Bool CStorySceneValidator::CanExportFile( CDiskFile* file ) const
{
	return file != NULL && file->GetFileName().EndsWith( TXT( ".w2scene" ) );
}

#endif //NO_EDITOR