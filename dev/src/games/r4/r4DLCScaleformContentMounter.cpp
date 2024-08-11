#include "build.h"
#include "r4DLCScaleformContentMounter.h"
#include "../../common/core/depot.h"
#include "../../common/engine/scaleformSystem.h"

IMPLEMENT_ENGINE_CLASS( CR4ScaleformContentDLCMounter );

CR4ScaleformContentDLCMounter::CR4ScaleformContentDLCMounter()
{

}

bool CR4ScaleformContentDLCMounter::OnCheckContentUsage()
{
	return false;
}

void CR4ScaleformContentDLCMounter::OnGameStarting()
{	
	Activate();
}

void CR4ScaleformContentDLCMounter::OnGameEnding()
{
	Deactivate();
}

#ifndef NO_EDITOR

void CR4ScaleformContentDLCMounter::OnEditorStarted()
{
	Activate();
}

void CR4ScaleformContentDLCMounter::OnEditorStopped()
{
	Deactivate();
}

void ExtractFiles( CDirectory* mainDir, const Char* fileExtensionFilter, const TDynArray< String >* excludedFileExtensions, const Bool recursive,  CAnalyzerOutputList& outputList )
{
	if ( !mainDir )
		return;

	// filter files
	const TFiles& files = mainDir->GetFiles();
	for ( CDiskFile* file : files )
	{
		String temp;
		const String conformedPath = CFilePath::ConformPath( file->GetDepotPath(), temp );
		const String ext = StringHelpers::GetFileExtension( conformedPath );

		// filter out files with invalid extension
		if (fileExtensionFilter)
		{
			if ( ext != fileExtensionFilter )
				continue;
		}

		// excluded
		if (excludedFileExtensions)
		{
			if ( excludedFileExtensions->Exist(ext) )
				continue;
		}

		// add to list
		outputList.AddFile( UNICODE_TO_ANSI( conformedPath.AsChar() ) );
	}

	// recurse to sub directories
	if ( recursive )
	{
		for ( CDirectory* subDir : mainDir->GetDirectories() )
		{
			ExtractFiles( subDir, fileExtensionFilter, excludedFileExtensions, recursive, outputList );
		}
	}
}

Bool CR4ScaleformContentDLCMounter::DoAnalyze( CAnalyzerOutputList& outputList )
{
	CDirectory* guiDirectory = GDepot->FindPath( m_scaleformDirectoryPath.AsChar() );
	if( guiDirectory != NULL )
	{
		ExtractFiles( guiDirectory, TXT("redswf"), NULL, true, outputList );

		// Extract all loose textures
		ExtractFiles( guiDirectory, TXT("dds"), NULL, true, outputList );
		ExtractFiles( guiDirectory, TXT("png"), NULL, true, outputList );

		// Extract all minimap resources
		ExtractFiles( guiDirectory->FindLocalDirectory( TXT("minimaps") ), NULL, NULL, true, outputList );
		ExtractFiles( guiDirectory->FindLocalDirectory( TXT("maps") ), NULL, NULL, true, outputList );
	
	}
	return true;
}
#endif // !NO_EDITOR

void CR4ScaleformContentDLCMounter::Activate()
{
	if( !m_scaleformDirectoryPath.Empty() )
	{
		#ifdef USE_SCALEFORM
		if ( CScaleformSystem::StaticInstance() )
		{
			CScaleformLoader* scaleformLoader = CScaleformSystem::StaticInstance()->GetLoader().GetPtr();
			if( scaleformLoader )
			{
				scaleformLoader->AddAdditionalContentDirectory( m_scaleformDirectoryPath );
			}
		}
		#endif // USE_SCALEFORM
	}
}

void CR4ScaleformContentDLCMounter::Deactivate()
{
	#ifdef USE_SCALEFORM
	if ( CScaleformSystem::StaticInstance() )
	{
		CScaleformLoader* scaleformLoader = CScaleformSystem::StaticInstance()->GetLoader().GetPtr();
		if( scaleformLoader )
		{
			scaleformLoader->RemoveAdditionalContentDirectory( m_scaleformDirectoryPath );
		}
	}
	#endif // USE_SCALEFORM
}
