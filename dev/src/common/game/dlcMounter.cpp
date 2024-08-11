#include "build.h"
#include "dlcMounter.h"
#include "../core/depot.h"

IMPLEMENT_ENGINE_CLASS( IGameplayDLCMounter );
IMPLEMENT_ENGINE_CLASS( CGameplayDLCMounterScripted );

RED_DEFINE_NAME( OnGameEnding );
RED_DEFINE_NAME( OnCheckContentUsage );

//----------------------------------

IGameplayDLCMounter::IGameplayDLCMounter()
{
}

#ifndef NO_EDITOR
Bool IGameplayDLCMounter::DoAnalyzeCSV( CAnalyzerOutputList& outputList, const String& csvFilePath )
{
	Bool status = true;

	String absoluteFilePath = GFileManager->GetDataDirectory();
	absoluteFilePath += csvFilePath.AsChar();

	// load the CSV
	THandle<C2dArray> ar( C2dArray::CreateFromString( absoluteFilePath ) );
	if (ar)
	{
		Uint32 numDeps = 0;

		// look for the resource paths (naive but works 100% of cases right now)
		const Uint32 numCols = ar->GetNumberOfColumns();
		const Uint32 numRows = ar->GetNumberOfRows();
		for (Uint32 i=0; i<numRows; ++i)
		{
			for (Uint32 j=0; j<numCols; ++j)
			{
				const String& cellText = ar->GetValueRef(j,i);
				if ( !cellText.Empty() && cellText.GetLength() < 1000 ) // TODO: magic number ?
				{
					// is a depot file path ?
					// we use the links here for safety
					CDiskFile* file = GDepot->FindFileUseLinks( cellText, 0 );
					if (file)
					{
						if ( file->GetDepotPath() != cellText )
						{
							ERR_GAME( TXT("!!! LINK FILE IN CSV '%ls' !!! - CSV references file '%ls' that was moved to '%ls'. Please update the CSV (we don't have links on cook and this will not work)."),
								csvFilePath.AsChar(), cellText.AsChar(), file->GetDepotPath().AsChar() );
							status = false;
						}

						StringAnsi filePathAnsi = UNICODE_TO_ANSI( cellText.AsChar() );
						outputList.AddFile( filePathAnsi );
						numDeps += 1;
					}
				}
			}
		}

		// list dependencies of the CSV files
		if (numDeps)
		{
			LOG_GAME( TXT("Found %d dependencies in CSV '%ls'"), 
				numDeps, csvFilePath.AsChar() );
		}
	}
	return status;
}
#endif

//----------------------------------

CGameplayDLCMounterScripted::CGameplayDLCMounterScripted()
{
}

void CGameplayDLCMounterScripted::OnGameStarting()
{
	// pass to scripts
	CallEvent( CNAME( OnGameStarting ) );
}

void CGameplayDLCMounterScripted::OnGameEnding()
{
	// pass to scripts
	CallEvent( CNAME( OnGameEnding ) );
}

bool CGameplayDLCMounterScripted::OnCheckContentUsage()
{
	// pass to scripts
	const auto ret = CallEvent( CNAME( OnCheckContentUsage ) );
	return (ret ==  CR_EventSucceeded);
}

#ifndef NO_EDITOR

void CGameplayDLCMounterScripted::OnEditorStarted()
{}

void CGameplayDLCMounterScripted::OnEditorStopped()
{}

Bool CGameplayDLCMounterScripted::DoAnalyze( CAnalyzerOutputList& outputList )
{
	return true;
}

#endif // !NO_EDITOR
//----------------------------------
