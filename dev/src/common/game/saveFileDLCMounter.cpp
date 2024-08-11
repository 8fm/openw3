/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "saveFileDLCMounter.h"

#include "../core/depot.h"

IMPLEMENT_ENGINE_CLASS( CSaveFileDLCMounter );

CSaveFileDLCMounter::CSaveFileDLCMounter()
{
}

void CSaveFileDLCMounter::OnGameStarting()
{
}

void CSaveFileDLCMounter::OnGameEnding()
{
}

Bool CSaveFileDLCMounter::OnCheckContentUsage()
{
	return false;
}

Bool CSaveFileDLCMounter::IsValid() const
{
	return GDepot->FileExist( m_starterSaveFilePath );
}

IFile* CSaveFileDLCMounter::CreateStarterFileReader() const
{
	if ( CDiskFile *file = GDepot->FindFile( m_starterSaveFilePath ) )
	{
		return file->CreateReader();
	}

	return nullptr;
}

#ifndef NO_EDITOR

	void CSaveFileDLCMounter::OnEditorStarted()
	{
	}

	void CSaveFileDLCMounter::OnEditorStopped()
	{
	}

	Bool CSaveFileDLCMounter::DoAnalyze( CAnalyzerOutputList& outputList )
	{
		if( !m_starterSaveFilePath.Empty() )
		{
			if ( IsValid() )
			{
				const StringAnsi actualDepotPathAnsi( UNICODE_TO_ANSI( m_starterSaveFilePath.AsChar() ) );
				outputList.AddFile( actualDepotPathAnsi );	
			}
		}
		return true;
	}

#endif // !NO_EDITOR