#include "build.h"
#include "../../common/core/commandlet.h"
#include "../../common/engine/worldSceneDependencyInfo.h"
#include "../../common/core/depot.h"
#include "../../common/core/objectGC.h"

class CWorldSceneDependencyCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CWorldSceneDependencyCommandlet, ICommandlet, 0 );
private:
	TDynArray< CWorld* >		m_levelsToLoad;

public:
	CWorldSceneDependencyCommandlet();

	// Executes commandlet command
	virtual bool Execute( const CommandletOptions& options );

	// Returns commandlet one-liner
	virtual const Char* GetOneLiner() const;

	// Prints commandlet help
	virtual void PrintHelp() const;
};

BEGIN_CLASS_RTTI( CWorldSceneDependencyCommandlet )
	PARENT_CLASS( ICommandlet)
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS(CWorldSceneDependencyCommandlet);

CWorldSceneDependencyCommandlet::CWorldSceneDependencyCommandlet()
{
	m_commandletName = CName( TXT("WorldSceneDependencyInfoFiles" ) );
}

bool CWorldSceneDependencyCommandlet::Execute( const CommandletOptions& options )
{
	LOG_WCC( TXT("Performing WorldSceneDependencyInfoFiles execute...") );
	//String outputDir;
	auto arguments = options.GetFreeArguments();
	/*
	if ( !options.GetSingleOptionValue( TXT("-path"), outputDir ) )
	{
		LOG_WCC( TXT("Need a path, specify it with -path") );
		return false;
	}*/
	String outputDir = arguments[ 0 ];

	CDirectory* levels = GDepot->FindLocalDirectory( TXT("levels" ) );

	TDynArray< CDiskFile* > worldFiles;
	levels->CollectFiles( worldFiles, ResourceExtension< CWorld >(), true, false );
	CTimeCounter timer;
	for( CDiskFile* file : worldFiles )
	{
		WorldLoadingContext wc;
		CWorld* lvl = CWorld::LoadWorld( file->GetDepotPath(), wc );
		if ( lvl )
		{
			LOG_WCC( TXT("Creating .dep file for world '%ls'"), lvl->GetFriendlyName().AsChar() );
			WorldSceneDependencyInfo::Start( lvl , outputDir);

			GObjectGC->CollectNow();
			CWorld::UnloadWorld( lvl );
		}
	}
	LOG_WCC( TXT(".dep files created in %1.2fs"), timer.GetTimePeriod() );

	// Done
	return true;
}

const Char* CWorldSceneDependencyCommandlet::GetOneLiner() const
{
	return TXT("Creates the .dep files");
}

void CWorldSceneDependencyCommandlet::PrintHelp() const
{
	LOG_WCC( TXT("Creates the .dep files needed for the database creation used by the Database Viewer.") );
}

