#include "build.h"
#include "r4DLCVideoMounter.h"
#include "../../common/core/depot.h"

IMPLEMENT_ENGINE_CLASS( CR4VideoDLCMounter );

#ifndef NO_EDITOR
namespace DLCVideoMounterHelpers
{
	static void ExtractFiles( CDirectory* mainDir, const Char* fileExtensionFilter, const TDynArray< String >* excludedFileExtensions, const Bool recursive,  CAnalyzerOutputList& outputList )
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
				DLCVideoMounterHelpers::ExtractFiles( subDir, fileExtensionFilter, excludedFileExtensions, recursive, outputList );
			}
		}
	}
}

Bool CR4VideoDLCMounter::DoAnalyze( CAnalyzerOutputList& outputList )
{
	CDirectory* videoDirectory = GDepot->FindPath( m_videoDirectoryPath.AsChar() );
	if( videoDirectory != NULL )
	{
		DLCVideoMounterHelpers::ExtractFiles( videoDirectory, TXT("usm"), NULL, true, outputList );
		DLCVideoMounterHelpers::ExtractFiles( videoDirectory, TXT("subs"), NULL, true, outputList );
	}
	return true;
}
#endif
