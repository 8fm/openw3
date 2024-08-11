
#include "build.h"
#include "../../common/engine/guiManager.h"

#pragma warning(disable:4311) // 'reinterpret_cast' : pointer truncation from 'LPVOID' to 'Uint32'
#pragma warning(disable:4312) // 'reinterpret_cast' : conversion from 'Uint32' to 'void *' of greater size
#pragma warning(disable:4244) // '+=' : conversion from 'SIZE_T' to 'Uint32', possible loss of data

namespace 
{
	int FilterException( unsigned int code, struct _EXCEPTION_POINTERS *ep) 
	{
		// 0 comes from our implementation of assert and error
		if (code != 0)
			RESV_ERR(TXT("Unhandled exception (code: 0x%08X) at address: 0x%08X"), code, ep->ExceptionRecord->ExceptionAddress)
		return EXCEPTION_EXECUTE_HANDLER;
	}
}

Uint CResVerifierEngine::m_processedFiles = 0;
Uint CResVerifierEngine::m_totalFiles = 0;

CWorld* CResVerifierEngine::SaftyLoadWorld(String* path)
{
	__try
	{
		WorldLoadingContext worldContext;
		CWorld* world = CWorld::LoadWorld( *path, worldContext );

		LayerGroupLoadingContext layerGroupContext;
		world->LoadAutomaticLayers( layerGroupContext );
		world->UpdateLoadingState();
		world->DelayedActions();

		return world;
	}
	__except(FilterException(GetExceptionCode(), GetExceptionInformation()))
	{
		return NULL;
	}
}

Bool CResVerifierEngine::SaftyUnloadWorld(CWorld* world)
{
	__try
	{
		CWorld::UnloadWorld(world);
		return true;
	}
	__except(FilterException(GetExceptionCode(), GetExceptionInformation()))
	{
		return false;
	}
}


CResource* CResVerifierEngine::SafetyLoadResource(String* path)
{
	__try
	{
		return GDepot->LoadResource(*path);
	}
	__except(FilterException(GetExceptionCode(), GetExceptionInformation()))
	{
		return NULL;
	}
}

void CResVerifierEngine::SafetyUnloadResource(CResource* resource)
{
	ASSERT(resource);
	__try
	{
		CDiskFile* file = resource->GetFile();
		file->Bind(NULL);
		//resource->Discard();
	}
	__except(FilterException(GetExceptionCode(), GetExceptionInformation()))
	{
		return;
	}
}

CResVerifierEngine::CResVerifierEngine() : CBaseEngine()
{
	m_viewport = NULL;
	m_processedFiles = 0;
	m_totalFiles = 0;
}

CResVerifierEngine::~CResVerifierEngine()
{
}

void CResVerifierEngine::ParseResource( String resourcePath, Bool topLevel )
{
	String localPath;
	


	GDepot->ConvertToLocalPath( resourcePath, localPath );	
	m_resPaths.PushBack( localPath );
	RESV_RES( TXT("%s"), localPath.AsChar());

	if (resourcePath.FindSubstring(TXT("\\junk")) != -1)
	{
		RESV_ERR( TXT("Reference to junk resource."));
		RESV_REPORT_JUNK ();
	}

	Bool existsInJunk = false;
	Bool isLink = false;
	String validPath;
	IFile* file = GFileManager->CreateFileReader( resourcePath, FOF_Buffered|FOF_AbsolutePath );
	if (!file)
	{
		validPath = resourcePath + TXT(".link");
		file = GFileManager->CreateFileReader( validPath, FOF_Buffered|FOF_AbsolutePath );
		if (file)
			isLink = true;

		if (!file)
		{
			validPath = GDepot->GetAbsolutePath() + TXT("\\junk\\") + localPath;
			file = GFileManager->CreateFileReader( validPath, FOF_Buffered|FOF_AbsolutePath );
			if (file)
				existsInJunk = true;
			else
			{
				validPath = GDepot->GetAbsolutePath() + TXT("\\junk\\") + localPath + TXT(".link");
				file = GFileManager->CreateFileReader( validPath, FOF_Buffered|FOF_AbsolutePath );
				if (file)
				{
					existsInJunk = true;
					isLink = true;
				}
			}
			delete file;
			file = NULL;
		}
	}

	if ( file )
	{
		CResVerifierDependencyLoader loader( *file );

		TDynArray< String > imports;

		if ( loader.GetImports(imports) )
		{
			for (Uint i=0; i < imports.Size(); i++)
			{
				ParseResource( GDepot->GetAbsolutePath() + imports[i] );
			}
		}

		delete file;
		// Try to load resource
		if ( !LoadResource( m_resPaths.Back() ) )
		{
			RESV_ERR( TXT("LoadResource() failed: couldn't load resource.") );
		}
	}
	else
	{
		String errMessage = String::Printf(TXT("File doesn't exist%s")
			, ((existsInJunk && isLink) ? TXT(", but its link is located in junk folder.") 
			: ((existsInJunk && !isLink) ? TXT(", but it's located in junk folder.") : TXT("."))
			));
		RESV_ERR( errMessage.AsChar());
	}

	m_resPaths.PopBack();
}


void CResVerifierEngine::ParseDepot( String depotSubpathToParse /* = String::EMPTY */ )
{
	// All files to check
	TDynArray< String > files;

	// Invalid files
	TDynArray< String > invalidFiles;

	// Get all resource classes
	TDynArray< CClass* > resourceClasses;
	SRTTI::GetInstance().EnumClasses( ClassID< CResource >(), resourceClasses );

	String layerPattern = String::Printf( TXT("*.%s"), ResourceExtension< CLayer >() );
	String worldPattern = String::Printf( TXT("*.%s"), ResourceExtension< CWorld >() );

	for ( Uint i=0; i < resourceClasses.Size(); i++ )
	{
		// Get resource type extension
		CResource* defaultResource = resourceClasses[i]->GetDefaultObject<CResource>();
		String extension = String( defaultResource->GetExtension() ).ToLower();
		String pattern = TXT("*.") + extension;

		if ( pattern != layerPattern && pattern != worldPattern )
		{
			GFileManager->FindFiles( depotSubpathToParse, pattern, files, true );
		}
	}

	String path, absPath;

	// For progress report
	m_totalFiles = files.Size();

	while ( !files.Empty() )
	{

		absPath = files.PopBack();
		m_resPaths.Clear();


		// Skip junked resources
		//if (absPath.FindSubstring(TXT(\\")) != -1)
		//	continue;

		ParseResource( absPath, true );
		++m_processedFiles;

	}

	// Layers and worlds
	files.Clear();

	// Get all world files
	GFileManager->FindFiles( depotSubpathToParse, worldPattern, files, true );

	CWorld* world;

	while ( !files.Empty() )
	{
		absPath = files.PopBack();
		GDepot->ConvertToLocalPath(absPath, path);

		RESV_RES( TXT("%s"), path.AsChar());

		// Load only world (without layers)
		world = SaftyLoadWorld( &path );

		if ( world )
		{
			CLayerGroup *layerGroup = world->GetWorldLayers();
			if ( layerGroup )
			{
				TDynArray< CLayerInfo* > layersInfo = layerGroup->GetLayers();

				for ( TDynArray< CLayerInfo* >::iterator i = layersInfo.Begin(); i != layersInfo.End(); ++i )
				{
					m_resPaths.Clear();
					m_resPaths.PushBack( path );

					ParseResource( GDepot->GetAbsolutePath() + (*i)->GetDepotPath() );
				}
			}
			else
			{
				// ERROR: world doesn't have layers
			}
			SaftyUnloadWorld(world);
		}
		else
		{
			// ERROR: Cannot load world 'path'
		}
		

		++m_processedFiles;
		SGarbageCollector::GetInstance().Collect( GC_CollectModified );
	}
}

namespace 
{
	Uint32 GetFreeMemorySize()
	{
		SYSTEM_INFO sInfo;
		ZeroMemory(&sInfo, sizeof(sInfo));
		GetSystemInfo( &sInfo );

		Uint32 nPageSize = sInfo.dwPageSize;
		Uint32 nCurrAddress = reinterpret_cast<Uint32>( sInfo.lpMinimumApplicationAddress );
		Uint32 nMaxAddress = reinterpret_cast<Uint32>( sInfo.lpMaximumApplicationAddress );
		Uint32 unusedBytes = 0;

		while( nCurrAddress < nMaxAddress )
		{
			MEMORY_BASIC_INFORMATION sMemInfo;

			if ( VirtualQuery( reinterpret_cast<void*>( nCurrAddress ), &sMemInfo, sizeof( sMemInfo ) ) )
			{
				if ( sMemInfo.State & MEM_FREE )
				{
					unusedBytes += sMemInfo.RegionSize;
				}
				nCurrAddress += sMemInfo.RegionSize;
			}
			else
				nCurrAddress += nPageSize;
		}

		return unusedBytes;
	}
}
// Load resource, proper files are linked in this function
Bool CResVerifierEngine::LoadResource( String resourceLocalPath )
{
	if ( m_loadableResources.Exist( resourceLocalPath ) )
		return true;


	Uint32 freeMemSizeBeforeLoad = GetFreeMemorySize();

	CResource* res = SafetyLoadResource( &resourceLocalPath );
	if (res)
		SafetyUnloadResource(res);
	SGarbageCollector::GetInstance().Collect( GC_CollectModified );

	Uint32 freeMemSizeAfterLoad = GetFreeMemorySize();

	if (freeMemSizeAfterLoad < freeMemSizeBeforeLoad)
	{
		static Uint32 wastedMemoryKB = 0;
		wastedMemoryKB += ((freeMemSizeBeforeLoad - freeMemSizeAfterLoad) >> 10);
		String errString = String::Printf(TXT("***WARNING*** Memory lost on resource load: %d KB\n***WARNING*** Total wasted memory: %d KB\n***WARNING*** Free memory: %d KB\n"), (freeMemSizeBeforeLoad - freeMemSizeAfterLoad) >> 10, wastedMemoryKB, (freeMemSizeAfterLoad) >> 10);
		RESV_LOG( errString.AsChar() );
		GSystem.OutputDebugString( errString.AsChar() );
	}


	if ( res )
	{
		m_loadableResources.Insert( resourceLocalPath );
	}

	return res ? true : false;
}

Uint32 CResVerifierEngine::GetProcessedFilesCount()
{
	return m_processedFiles;
}

Uint32 CResVerifierEngine::GetDepth()
{
	return SResVerifierEngineInstance->m_resPaths.Size();
}

const TList<String>& CResVerifierEngine::GetLoadTrace()
{
	return SResVerifierEngineInstance->m_resPaths;
}

Bool CResVerifierEngine::Initialize()
{
	Bool success = CBaseEngine::Initialize();
	if (!success)
		return false;

	SResVerifierEngineInstance = this;


	m_viewport = GRender->CreateViewport( NULL, NULL, TXT("Lava Engine © 2008-2009 CD Projekt Red"), 800, 600, false );
	if ( !m_viewport )
	{ 
		return false;
	}

	return true;
}

void CResVerifierEngine::Shutdown()
{
	// HACK: I cannot call CBaseEngine::Shutdown(), because garbage collector crashes there for some reason.
	// HACK: As it will be refactored soon, I will just paste the shutdown code from there, without GC::Collect call
	// CBaseEngine::Shutdown();

	// Shut down game
	GGame->ShutDown();
	GGame->SetViewport( NULL );
	GGame->RemoveFromRootSet();

	GSoundSystem->ReleaseResources();

	// GC added before RTTI deinit, because some classes destruction is RTTI dependant.
	// There should be also another GC after RTTI deinit to clean stuff that was part of RTTI skeleton.
	SGarbageCollector::GetInstance().Collect();

	// Deinitialize RTTI
	GIsClosing = true;
	GAsyncLoader->WakeUp();
	GAsyncLoader->WaitForFinish();
	SRTTI::GetInstance().Deinit();

	// Final GC
	// SGarbageCollector::GetInstance().Collect(); << this call causes a crash, well I prefer leaks instead

	// Delete all resources
	GRender->ReleaseDisposedResources();

	// Shut down rendering
	delete GRender;
	GRender = NULL;

	// Shutdown GUI manager
	if( GGUIManager )
	{
		GGUIManager->ShutDown();
		delete GGUIManager;
		GGUIManager = NULL;
	}

	// AsyncLoader shutdown
	GAsyncLoader->WaitForFinish();
	delete GAsyncLoader;

	// Sound system
	GSoundSystem->Shutdown();
	delete GSoundSystem;
	GSoundSystem = NULL;

	// Shut down world tick
	GTickThread->RequestExit();
	GTickThread->WaitForFinish();
	delete GTickThread;
	GTickThread = NULL;

	// Shut dawn main havok engine
	GHavokEngine->ShutDown();
	delete GHavokEngine;
	GHavokEngine = NULL;
}

void CResVerifierEngine::ReportJunkResource()
{
	String referencedBy;
	String res = m_resPaths.Back();

	// When list size is equal to 1, that means that this resource is not referenced. 
	// It is a root on resource tree.
	if (m_resPaths.Size() > 1)
	{
		m_resPaths.PopBack();
		referencedBy = m_resPaths.Back();
		m_resPaths.PushBack(res);
	}
	m_junkResources.ReportResource( res, referencedBy );
}

CResVerifierEngine* CResVerifierEngine::SResVerifierEngineInstance = NULL;

CName CResVerifierOutput::TYPE_LOG( TXT("ResVerifier.Log") );
CName CResVerifierOutput::TYPE_ERR( TXT("ResVerifier.Err") );
CName CResVerifierOutput::TYPE_RES( TXT("ResVerifier.Res") );

CResVerifierOutput::CResVerifierOutput()
{
	SLog::GetInstance().AddOutput(this);

	m_stdOut = GetStdHandle( STD_OUTPUT_HANDLE );

	char fileName[1024];
	sprintf( fileName, "resVerifier.html" );

	m_resVerifiacationLogFile = fopen( fileName, "wb" );

	const int BYTE_ORDER_MASK = 0XFEFF;
	fwrite( &BYTE_ORDER_MASK, 2, 1, m_resVerifiacationLogFile ); 

	sprintf( fileName, "resVerifierErrors.html" );
	m_resVerifiacationErrFile = fopen( fileName, "wb" );
	fwrite( &BYTE_ORDER_MASK, 2, 1, m_resVerifiacationErrFile ); 

	sprintf( fileName, "resVerifierErrors.csv" );
	m_resVerifiacationErrFileCSV = fopen( fileName, "wb" );

	const Char* htmlBegin = TXT("<html>\n<title>Resource Verification</title>\n<body style=\"font-family: 'Courier New';\">");
	fwrite( htmlBegin, AStrLen(htmlBegin), sizeof(Char), m_resVerifiacationLogFile ); 
}

CResVerifierOutput::~CResVerifierOutput()
{
	const Char* htmlEnd = TXT("</body>\n</html>");
	fwrite( htmlEnd, AStrLen(htmlEnd), sizeof(Char), m_resVerifiacationLogFile ); 
	fclose(m_resVerifiacationLogFile);
	fwrite( htmlEnd, AStrLen(htmlEnd), sizeof(Char), m_resVerifiacationErrFile ); 
	fclose(m_resVerifiacationErrFile);
	if (m_resVerifiacationErrFileCSV)
		fclose(m_resVerifiacationErrFileCSV);
	SLog::GetInstance().RemoveOutput(this);
}

void CResVerifierOutput::Write( const CName& type, const Char* str )
{
	String htmlPrefix;
	const Char* htmlSuffix = TXT("</font>");
	if (type == TYPE_LOG)
	{
		SetConsoleTextAttribute( m_stdOut, 7 );			// Normal gray
		htmlPrefix = TXT("<font color=black>");
	}
	else if (type == TYPE_ERR)
	{
		SetConsoleTextAttribute( m_stdOut, 12 );		// Red
		htmlPrefix = TXT("<font color=red>");
	}
	else if (type == TYPE_RES)
	{
		SetConsoleTextAttribute( m_stdOut, FOREGROUND_BLUE | FOREGROUND_INTENSITY );		// Red
		htmlPrefix = TXT("<font color=blue>");
	}
	else
	{
		return;
	}

	String outCome;
	const Char eol[] = TXT("\n");

	if (type != TYPE_LOG)
	{
		Uint32 count = CResVerifierEngine::GetProcessedFilesCount();
		outCome = String::Printf( TXT("[%05d] "), count);
		Int32 depth = CResVerifierEngine::GetDepth();
		for (Int32 i = 0; i < depth; i++)
		{
			outCome += TXT("|");

		}
		outCome += TXT("-");
	}

	String htmlized = str;
	while (htmlized.Replace(TXT("\n"), TXT("<br>")));
	outCome += str;
	outCome += TXT("\n");
	wprintf( TXT("%s"), outCome.AsChar() );

	while (outCome.Replace(TXT("\n"), TXT("<br>")));
	outCome = htmlPrefix + outCome + htmlSuffix;
	fwrite( outCome.AsChar(), outCome.Size(), sizeof(Char), m_resVerifiacationLogFile );
	fwrite( eol, AStrLen(eol), sizeof(Char), m_resVerifiacationLogFile ); 

	if (type == TYPE_ERR)
	{
		static Uint SNumErrors = 0;
		static String SCSVSeparator = TXT(";");
		String errorCountStr = String::Printf(TXT("<b>[Error: %04d]</b> "), ++SNumErrors);
		fwrite( htmlPrefix.AsChar(), htmlPrefix.Size(), sizeof(Char), m_resVerifiacationErrFile );
		fwrite( errorCountStr.AsChar(), errorCountStr.Size(), sizeof(Char), m_resVerifiacationErrFile );
		if (m_resVerifiacationErrFileCSV)
			fwrite( str, AStrLen(str), sizeof(Char), m_resVerifiacationErrFileCSV );
		fwrite( htmlized.AsChar(), htmlized.Size(), sizeof(Char), m_resVerifiacationErrFile );
		fwrite( htmlSuffix, AStrLen(htmlSuffix), sizeof(Char), m_resVerifiacationErrFile );
		fwrite( TXT("<BR>"), AStrLen(TXT("<BR>")), sizeof(Char), m_resVerifiacationErrFile );
		fwrite( eol, AStrLen(eol), sizeof(Char), m_resVerifiacationErrFile ); 

		Int32 depth = CResVerifierEngine::GetDepth();
		const TList<String>& loadTrace = CResVerifierEngine::GetLoadTrace();
		int j = 0;
		for (TList<String>::const_iterator i = loadTrace.Begin(); i != loadTrace.End(); i++, j++)
		{
			String s = *i;
			String s2 = SCSVSeparator + s + SCSVSeparator + eol;
			s = TXT("-> ") + String((j == depth-1) ? TXT("<B>") : String::EMPTY) + s;
			for (int k = 0; k < j; k++)
				s = TXT("&nbsp;&nbsp;&nbsp;") + s;

			s += TXT("</B><BR>");
			fwrite( s.AsChar(), s.Size(), sizeof(Char), m_resVerifiacationErrFile );
			if (m_resVerifiacationErrFileCSV)
				fwrite( s2.AsChar(), s2.Size(), sizeof(Char), m_resVerifiacationErrFileCSV );
		}

		fwrite( TXT("<BR>"), AStrLen(TXT("<BR>")), sizeof(Char), m_resVerifiacationErrFile );
		fwrite( eol, AStrLen(eol), sizeof(Char), m_resVerifiacationErrFile ); 
		if (m_resVerifiacationErrFileCSV)
			fwrite( eol, AStrLen(eol), sizeof(Char), m_resVerifiacationErrFileCSV ); 

	}
	fflush(stdout);
	fflush( m_resVerifiacationLogFile );
	fflush( m_resVerifiacationErrFile );
	if (m_resVerifiacationErrFileCSV)
		fflush( m_resVerifiacationErrFileCSV );
}

void CResVerifierCaseCategory::ReportResource( const String& resPath, const String& referencedBy )
{
	TReferences::iterator it = m_references.Find( resPath );
	if (it == m_references.End())
	{
		m_references.Insert( resPath, TSet<String>() );
		it = m_references.Find( resPath );
	}
	m_references[ resPath ].Insert (referencedBy);
}

CResVerifierCaseCategory::~CResVerifierCaseCategory()
{
	RESV_LOG(TXT("Dumping logfile %s.log"), m_fileName.AsChar());

	FILE* file = fopen( UNICODE_TO_ANSI((m_fileName + TXT(".log")).AsChar()), "wb"); 

	const int BYTE_ORDER_MASK = 0XFEFF;
	fwrite( &BYTE_ORDER_MASK, 2, 1, file ); 

	static Char* eol = TXT("\r\n");
	static Char* tab = TXT("\t->");
	Uint numErrors = 0;
	for (TReferences::iterator it = m_references.Begin(); it != m_references.End(); it++)
	{
		String* filePath = &it->m_first;
		String entry = String::Printf(TXT("%d. %s"), ++numErrors, filePath->AsChar());
		fwrite( entry.AsChar(), sizeof(Char), entry.Size(), file);
		fwrite( eol, sizeof(Char), AStrLen(eol), file);
		for (TSet<String>::iterator it2 = it->m_second.Begin(); it2 != it->m_second.End(); it2++)
		{
			filePath = &(*it2);
			fwrite( tab, sizeof(Char), AStrLen(tab), file);
			fwrite( filePath->AsChar(), sizeof(Char), filePath->Size(), file );
			fwrite( eol, sizeof(Char), AStrLen(eol), file);
		}
	}

	fclose(file);
}

CResVerifierCaseCategory::CResVerifierCaseCategory( const Char* fileName )
{
	m_fileName = fileName;
}

CResVerifierJunkCategory::CResVerifierJunkCategory() : CResVerifierCaseCategory(TXT("resVerifierJunkResources"))
{

}

