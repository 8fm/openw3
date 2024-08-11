
#include "build.h"



EAssertAction AssertMessageImp( const Char *file, Uint line, const Char *message, const Char *callstack )
{
	return AA_Break;
}

void ErrorMessageImp( EErrorType type, const Char *message, const Char *callStackMessage )
{
}

CGraphBlockDescription::CGraphBlockDescription()
{}

CGraphBlockDescription::~CGraphBlockDescription()
{}

void CGraphBlockDescription::DeInit()
{}

void CGraphBlockDescription::RegisterBlockDesc(const String &blockName, const String &desc)
{}

void CGraphBlockDescription::RegisterSocketDesc(const String &blockName, const String &socketName, const String &socketDesc)
{}

void CGraphBlockDescription::RegisterSocketDesc(const String &socketName, const String &socketDesc)
{}

void CGraphBlockDescription::GenerateHtmlDoc(const String &fname, const String &baseClass)
{}

String CGraphBlockDescription::GetTooltip(CGraphBaseItem *item)
{ return String(); }

String CGraphBlockDescription::GetTooltip(CGraphBaseItem *item, TBlocksDesc &brixDescList)
{ return String(); }

bool CGraphBlockDescription::ReadDescriptionsFromFile(const String &fname)
{ return false; }

void CGraphBlockDescription::RegisterSocketDesc(const String &blockName, const String &socketName, const String &socketDesc, TBlocksDesc &brixDescList)
{}

void CGraphBlockDescription::RegisterBlockDesc(const String &blockName, const String &desc, TBlocksDesc &brixDescList)
{}

bool CGraphBlockDescription::SaveDescriptionsDefinedInCodeToFile(const String &fname)
{ return false; }

bool CGraphBlockDescription::SaveDescriptionsDefinedInCodeToFile(const String &fname, const String &baseClass)
{ return false; }


extern DWORD GMainThreadID;
extern String* GCommandLine;

Bool ParseCommandLine( String tag, String& output, Int argCount, const char* argV[])
{
	for (Int i=1; i<argCount; i++)
	{
		String temp = ANSI_TO_UNICODE(argV[i]);
		if (temp.BeginsWith(tag))
		{
			output = temp.StringAfter(tag);
			return true;
		}
	}

	return false;
}

void SpecialInitializePlatform(const Char* inputBasePath)
{
	GSystem.Init();

	// Setup error system
	GError = &GWin32ErrorSystem;

	// Get ID of main thread
	GMainThreadID = ::GetCurrentThreadId();

	// Use the path from the EXE file to setup current directory
	static Char basePath[ MAX_PATH ];
	static Char dataPath[ MAX_PATH ];

	GetModuleFileName( NULL, basePath, MAX_PATH );
	// Strip EXE name
	Char* pEnd = AStrRChr( basePath, '\\' );
	if ( pEnd ) *pEnd = '\0';

	// Set new current directory to the directory of binary file
	SetCurrentDirectory( basePath );

	// Set data path
	AStrCpy( dataPath, inputBasePath );

// 	// Fix paths with \ on the end
// 	if ( GIsCooked )
// 	{
// 		AStrCat( dataPath, TXT("\\CookedPC\\data\\") );
// 		AStrCat( basePath, TXT("\\") );
// 	}
// 	else
// 	{
// 		AStrCat( dataPath, TXT("\\data\\") );
// 		AStrCat( basePath, TXT("\\") );
// 	}

	// Grab command line
	GCommandLine = new String( TXT("") );

	// Initialize file system
	GFileManager = new CFileManager( basePath, dataPath );

	// Initialize version control
	SInitializeVersionControl();

	// Initialize depot
	GDepot = new CDepot( dataPath );
	{
		double startTime = GClock.GetSeconds();
		GDepot->Repopulate();
		LOG( TXT("GDepot->Repopulate() took : %.1f sec "), (GClock.GetSeconds() - startTime) );
	}

	// Output logs to file 
	SLog::GetInstance().AddOutput( new CFileOutput() );
}

void SInitializeVersionControl()
{
}