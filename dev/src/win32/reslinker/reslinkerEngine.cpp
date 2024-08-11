
#include "build.h"

#define ADD_TO_EMAIL( x ) if ( !email.Empty() ) AddToEmail( x )


void CResLinkerEngine::AddToEmail( const Char *text )
{
	m_emailText += text;
	m_emailText += TXT("\r\n");
}

CWorld* CResLinkerEngine::LoadWorldSafely(String* path)
{
	__try
	{
		WorldLoadingContext worldContext;
		CWorld* world = CWorld::LoadWorld( *path, worldContext );

		LayerGroupLoadingContext layerGroupContext;
		world->LoadAutomaticLayers( layerGroupContext );
		world->UpdateLoadingState();
		world->DelayedActions();

		LOG(TXT("Reslinker: World loaded: %s"),*path);

		return world;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		ERR(TXT("Reslinker: Couldn't load invalid world: %s"),*path);
		return NULL;
	}
}

Bool CResLinkerEngine::SaveWorldSafely(CWorld* world)
{
	// The method has been deprecated on (12.01.2010)

	//__try
	//{
	//	// Save world object
	//	if ( world->Save() )
	//	{
	//		// Save all layers
	//		if ( !(world->SaveLayers()) )
	//		{
	//			WARN( TXT("Reslinker: Couldn't save layers, world %s") ,world->GetFile()->GetAbsolutePath());
	//		}

	//		LOG(TXT("Reslinker: World saved: %s"),world->GetFile()->GetAbsolutePath());
	//	}

	//	return true;
	//}
	//__except(EXCEPTION_EXECUTE_HANDLER)
	//{
	//	ERR(TXT("Reslinker: Couldn't save invalid world: %s"),world->GetFile()->GetAbsolutePath());
	//	return false;
	//}
	return false;
}

Bool CResLinkerEngine::UnloadWorldSafely(CWorld* world)
{
	// The method has been deprecated on (12.01.2010)

	/*__try
	{
		CWorld::UnloadWorld(world);
		LOG(TXT("Reslinker: Unload world: %s"), world->GetFile()->GetAbsolutePath());
		return true;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
			ERR(TXT("Reslinker: Couldn't unload invalid world."));
		return false;
	}*/

	return false;
}

CResource* CResLinkerEngine::LoadResourceSafely(String* path)
{
	// The method has been deprecated on (12.01.2010)

	/*__try
	{
		return GDepot->LoadResource(*path);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		ERR(TXT("Reslinker: Couldn't load invalid resource: %s"),*path);
		return NULL;
	}*/

	return false;
}

Bool CResLinkerEngine::SaveResourceSafely(CResource *resource)
{
	// The method has been deprecated on (12.01.2010)

	//__try
	//{
	//	// Save world object
	//	resource->Save();
	//	return true;
	//}
	//__except(EXCEPTION_EXECUTE_HANDLER)
	//{
	//	ERR(TXT("Reslinker: Couldn't save invalid resource: %s"),resource->GetFile()->GetAbsolutePath());
	//	return false;
	//}

	return false;
}

CResLinkerEngine::CResLinkerEngine() 
	: CBaseEngine()
{
	// Get output handles
	m_stdOut = GetStdHandle( STD_OUTPUT_HANDLE );
}

CResLinkerEngine::~CResLinkerEngine()
{
}

void CResLinkerEngine::Write( const CName& type, const Char* str )
{
	// Warning
	if ( type == *SLog::GetInstance().TYPE_WARN )
	{
		SetConsoleTextAttribute( m_stdOut, 14 );		// Yellow
	}
	else if ( type == *SLog::GetInstance().TYPE_ERR )
	{
		SetConsoleTextAttribute( m_stdOut, 12 );		// Red
	}
	else
	{
		SetConsoleTextAttribute( m_stdOut, 7 );			// Normal gray
	}

	// Print on screen
	wprintf( TXT("%s\n"), str );
	fflush(stdout);
}

Bool CResLinkerEngine::Main( Int argCount, const char* argV[] )
{
	GIsReslinker = true;

	LOG(TXT("\n-------------------------------"));
	LOG(TXT("Resource linker engine start..."));

	String setting;
	CConfigurationManager &config = SUserConfigurationManager::GetInstance();
	config.SetPath(TXT("/VersionControl"));

	String user, password, workspace, tag, temp, dataDirectory, targetDirectory, email, linkContents;
	Bool deleteLinkFiles = true;
	Bool usePerforce = false;
	Bool isSubmitFiles = true;
	Bool isForceLoad = false;
	Bool silentMode = false;
	Bool skipResources = false;
	
	tag = TXT("-u");
	if ( !ParseCommandLine(tag, user, argCount, argV) ) config.Read( TXT("User"), &user ); 

	tag = TXT("-p");
	if ( !ParseCommandLine(tag, password, argCount, argV) ) config.Read( TXT("Password"), &password ); 

	tag = TXT("-w");
	if ( !ParseCommandLine(tag, workspace, argCount, argV) ) config.Read( TXT("Workspace"), &workspace ); 

	tag = TXT("-s");
	if ( ParseCommandLine(tag, temp, argCount, argV) )
	{
		silentMode = true;
	}

	tag = TXT("-vcp4");
	if ( ParseCommandLine(tag, temp, argCount, argV) )
	{
		usePerforce = true;

		delete GVersionControl;

		CVersionControlInterface *iface = new CVersionControlInterface;
		iface->m_silentMode = silentMode;
		GVersionControl = new CSourceControlP4( iface );
	}

	tag = TXT("-d");
	if ( !ParseCommandLine(tag, dataDirectory, argCount, argV) )
	{
		dataDirectory = GFileManager->GetDataDirectory();
	}

	tag = TXT("-t");
	if ( !ParseCommandLine(tag, targetDirectory, argCount, argV) )
	{
		targetDirectory = String::EMPTY;
	}

	tag = TXT("-link");
	if ( ParseCommandLine(tag, temp, argCount, argV) )
	{
		deleteLinkFiles = false;
	}

	tag = TXT("-ns");
	if ( ParseCommandLine(tag, temp, argCount, argV) )
	{
		isSubmitFiles = false;
	}

	tag = TXT("-fl");
	if ( ParseCommandLine(tag, temp, argCount, argV) )
	{
		isForceLoad = true;
	}

	tag = TXT("-m");
	if ( ParseCommandLine(tag, email, argCount, argV) )
	{
		if ( !email.EndsWith( TXT("@cdprojektred.com" ) ) )
		{
			LOG( TXT("Invalid e-mail address: '%s' - not in correct domain (cdprojektred.com)."), email.AsChar()  ); 
			email.Clear();
		}
	}

	tag = TXT("-rs");
	if ( ParseCommandLine(tag, temp, argCount, argV) )
	{
		skipResources = true;
	}
	
	GVersionControl->SetUser( user );
	GVersionControl->SetPassword( password );
	GVersionControl->SetClient( workspace );
	

	// Display data
	LOG(TXT("User: %s"), user);
	LOG(TXT("Password: %s"), password);
	LOG(TXT("Workspace: %s"), workspace);
	LOG(TXT("Data directory: %s"), dataDirectory);
	LOG(TXT("GDepot abs path: %s"), GDepot->GetAbsolutePath());
	LOG(TXT("Target directory: %s"), dataDirectory+targetDirectory);
	LOG(TXT("Base directory: %s"), GFileManager->GetBaseDirectory());
	LOG(TXT("Use perforce: %s"), usePerforce ? TXT("Yes") : TXT("No") );
	LOG(TXT("Delete link files: %s"), deleteLinkFiles ? TXT("Yes") : TXT("No") );
	LOG(TXT("Submit files: %s"), isSubmitFiles ? TXT("Yes") : TXT("No") );

	GFileManager->FindFiles( GDepot->GetAbsolutePath(), TXT("*.link"), m_links, true );
	
 	// Check if any file is found
 	if (m_links.Empty() && !isForceLoad)
 	{
 		LOG( TXT("No link files.\n"));
 		return true;
 	}

	LOG(TXT("Found links files: %d\n"), m_links.Size());
	ADD_TO_EMAIL( TXT("--------------------------------------") );
	ADD_TO_EMAIL( TXT("Link problems:") );

	// Prepare link paths
	for (Uint i=0; i<m_links.Size(); i++)
	{
		IFile* reader = GFileManager->CreateFileReader( m_links[i], FOF_AbsolutePath );
		if ( reader )
		{
			// Load file to string
			Uint size = reader->GetSize();
			AnsiChar* chars = new AnsiChar[ size+1 ];
			reader->Serialize( chars, size );
			chars[ size ] = 0;

			// Convert to normal string
			linkContents = ANSI_TO_UNICODE( chars );

			if (	GFileManager->GetFileSize( GFileManager->GetDataDirectory() + TXT("/") + linkContents ) < Uint64(1) &&
					GFileManager->GetFileSize( GFileManager->GetDataDirectory() + TXT("/") + linkContents + TXT(".link") ) < Uint64(1))
			{
				ADD_TO_EMAIL( String::Printf( TXT("Broken link file: '%s', destination (%s) doesn't exist."), m_links[i].AsChar(), linkContents.AsChar() ).AsChar() );
			}

			// Close file with a success
			delete reader;
			delete chars;

		}

		// Remove .link extension
		m_links[i] = m_links[i].StringBefore(TXT(".link"),true);

		// Not abs path
		GDepot->ConvertToLocalPath(m_links[i], m_links[i]);
	}
	
	// All files to check
	TDynArray< String > files;

	// Invalid files
	TDynArray< String > invalidFiles;
	TDynArray< String > unloadFiles;
	
 	// Get all resource classes
 	TDynArray< CClass* > resourceClasses;
 	SRTTI::GetInstance().EnumClasses( ClassID< CResource >(), resourceClasses );
 
	String layerPattern = String::Printf( TXT("*.%s"), ResourceExtension< CLayer >() );
	String worldPattern = String::Printf( TXT("*.%s"), ResourceExtension< CWorld >() );

     for ( Uint i=0; i<resourceClasses.Size(); i++ )
     {
     	// Get resource type extension
     	CResource* defaultResource = resourceClasses[i]->GetDefaultObject<CResource>();
     	String extension = String( defaultResource->GetExtension() ).ToLower();
     	String pattern = TXT("*.")+extension;
     
 		if ( pattern!=layerPattern && pattern!=worldPattern )
 		{
 			GFileManager->FindFiles( GDepot->GetAbsolutePath()+targetDirectory, pattern, files, true);
 		}
     }

	TDynArray< String > filesToSubmit;
	String path, absPath;
	Bool needUpdate;

	// For progress report
	Uint progStep = files.Size()/10;
	Uint currFiles = 0;

	if ( !skipResources )
	{
		LOG(TXT("--------------------------------------"));
		LOG(TXT("Files to check: %d"), files.Size());

		while (!files.Empty())
		{
			// Show progress
			LOG(TXT("Progress: %ld files.\n\n"), currFiles);

			currFiles++;
			absPath = files.PopBack();
			GDepot->ConvertToLocalPath(absPath, path);

			if ( !isForceLoad )
				needUpdate = ProcessFile( absPath );

			if ( needUpdate || isForceLoad )
			{
				// Load resource, proper files are linked in this function
				LOG( TXT("-> Reslinker: Load file %s") ,path.AsChar());

				CResource* res = LoadResourceSafely( &path );

				if (!res)
				{
					invalidFiles.PushBack( path );
					SGarbageCollector::GetInstance().Collect( GC_CollectModified );
					continue;
				}

				ISourceControl::EOpenStatus os = GVersionControl->GetOpenStatus( absPath );
				if ( !silentMode || os == ISourceControl::OS_OpenedOnlyByMe || os == ISourceControl::OS_NotOpen )
				{
					// Check out
					GVersionControl->CheckOut( *(res->GetFile()) );

					// Save resource with new data
					if(!SaveResourceSafely(res))
					{
						invalidFiles.PushBack( path );
						SGarbageCollector::GetInstance().Collect( GC_CollectModified );
						continue;
					}

					// Add to submit
					String filePath;
					GDepot->ConvertToLocalPath(res->GetFile()->GetAbsolutePath(), filePath);
					filesToSubmit.PushBack( filePath );
				}
				else
				{
					LOG( TXT("File '%s' was opened for edit by someone else, skipping."), absPath.AsChar() );
				}

				SGarbageCollector::GetInstance().Collect( GC_CollectModified );
			}
		}
	}

	// Layers and worlds
	files.Clear();

	GFileManager->FindFiles( GDepot->GetAbsolutePath()+targetDirectory, worldPattern, files, true);

	CWorld* world;

	LOG(TXT("--------------------------------------"));
	LOG(TXT("World files to check: %d"), files.Size());

	currFiles = 0;

	while (!files.Empty())
	{
		currFiles++;

		// Show progress
		LOG( TXT("Progress: %ld files"), currFiles );

		absPath = files.PopBack();
		GDepot->ConvertToLocalPath(absPath, path);

		LOG( TXT("-> Reslinker: Load world %s") ,path.AsChar());

		ISourceControl::EOpenStatus os = GVersionControl->GetOpenStatus( absPath );
		if ( !silentMode || os == ISourceControl::OS_OpenedOnlyByMe || os == ISourceControl::OS_NotOpen )
		{
			world = LoadWorldSafely(&path);

			if (!world) 
			{
				String msg = TXT("In world: ")+path;
				invalidFiles.PushBack( msg );
				continue;
			}

			TDynArray< CLayer* > layers;
			world->GetAttachedLayers( layers );

			LOG( TXT("Layers to save: %d") ,layers.Size());

			Bool success = GVersionControl->CheckOut( *( world->GetFile() ) );
			for ( Uint i=0; i<layers.Size(); ++i )
			{
				if ( layers[i]->GetFile() )
					success &= GVersionControl->CheckOut( *( layers[i]->GetFile() ) );
				else
					success = false;

				if ( !success )
				{
					LOG( TXT("File '%s' couldn't be checked out, skipping whole world."), absPath.AsChar() );
					break;
				}
			}

			if ( !success )
			{
				UnloadWorldSafely( world );
				SGarbageCollector::GetInstance().Collect( GC_CollectModified );
				continue;
			}

			if (!SaveWorldSafely(world)) 
			{
				String msg = TXT("In world: ")+path;
				invalidFiles.PushBack( msg );
				continue;
			}

			for (Uint i=0; i<layers.Size(); i++)
			{
				if (layers[i]->GetFile())
				{
					String filePath;
					GDepot->ConvertToLocalPath(layers[i]->GetFile()->GetAbsolutePath(), filePath);
					filesToSubmit.PushBack( filePath );
				}
			}

			String filePath;
			GDepot->ConvertToLocalPath(world->GetFile()->GetAbsolutePath(), filePath);
			filesToSubmit.PushBack( filePath );
		}
		else
		{
			LOG( TXT("File '%s' was opened for edit by someone else, skipping."), absPath.AsChar() );
		}

		if (!UnloadWorldSafely(world))
		{
			String msg = TXT("In world: ")+path;
			invalidFiles.PushBack( msg );
		}

		SGarbageCollector::GetInstance().Collect( GC_CollectModified );
	}


	LOG(TXT(""));
	ADD_TO_EMAIL( TXT("--------------------------------------") );

	const CDepot::TLinkFilesMap &map = GDepot->GetLinkFilesUsed();
	for ( CDepot::TLinkFilesMap::const_iterator it = map.Begin(); it != map.End(); it++ )
	{
		LOG( TXT("Link file '%s' were used %ld times."), it->m_first, it->m_second );
		ADD_TO_EMAIL( String::Printf( TXT("Link file '%s' were used %ld times."), it->m_first, it->m_second ).AsChar() );
	}

	if ( deleteLinkFiles )
	{
		// Delete link files & add to submit
		for ( Uint i=0; i<m_links.Size(); i++ )
		{
			String currentLink = m_links[ i ] + TXT(".link");
			const Uint *count = map.FindPtr( currentLink );

			if ( count == 0 || *count == 0 )
			{
				CDiskFile* linkFile = GDepot->FindFile( currentLink );

				if (linkFile)
				{
					String filePath;
					if (linkFile->IsAdded())
						linkFile->Revert();
					linkFile->GetStatus();
					Bool deleted = linkFile->Delete();
					if ( deleted )
					{
						GDepot->ConvertToLocalPath( linkFile->GetAbsolutePath(), filePath );
						filesToSubmit.PushBack( filePath );
						ADD_TO_EMAIL( String::Printf( TXT("Link file '%s' has been deleted."), filePath.AsChar() ).AsChar() );
					}
				}
				else
				{
					LOG( TXT("Reslinker: Couldn't find and delete link file %s"), currentLink.AsChar() );
				}
			}
		}
	}

	// Submit all files
	if ( isSubmitFiles && !filesToSubmit.Empty() )
	{
		TDynArray< CDiskFile* > tempFiles;
		for ( Uint i=0; i<filesToSubmit.Size(); i++ )
		{
			CDiskFile* tempFile = GDepot->FindFile( filesToSubmit[ i ] );
			if ( tempFile )
			{
				tempFile->GetStatus();
				if ( !tempFile->IsLocal() )
					tempFiles.PushBack( tempFile );
			}
			else
			{
				ERR( TXT("Reslinker: Couldn't find file to submit: %s"), filesToSubmit[i].AsChar() );
			}
		}
		if ( tempFiles.Size() )
		{
			if ( GVersionControl->Submit( tempFiles ) == false )
			{
				GVersionControl->Revert( tempFiles );
				ERR( TXT("SUBMIT FAILED!!!!!!! All files reverted!") );
				ADD_TO_EMAIL( TXT("SUBMIT FAILED!!!!!!! All files reverted!") );
			}

		}
	}


	LOG( TXT("") );

	// Info about unload files
	if ( !unloadFiles.Empty() )
	{
		LOG( TXT("--------------------------------------"));
		LOG( TXT("Couldn't load files because files don't exist: "));
		for (Uint i=0; i<unloadFiles.Size(); i++)
		{
			if (!GDepot->FindFile(unloadFiles[i]))
			{
				LOG( TXT("%s") ,unloadFiles[i].AsChar());
			}
		}

		LOG( TXT("") );

		LOG( TXT("--------------------------------------"));
		LOG( TXT("Couldn't load files:"));
		for (Uint i=0; i<unloadFiles.Size(); i++)
		{
			if (GDepot->FindFile(unloadFiles[i]))
			{
				LOG( TXT("%s") ,unloadFiles[i].AsChar());
			}
		}
	}

	LOG( TXT("") );

	ADD_TO_EMAIL( TXT("--------------------------------------") );
	ADD_TO_EMAIL( TXT("Invalid files:") );

	// Info about invalid files
	if (!invalidFiles.Empty())
	{
		LOG( TXT("--------------------------------------"));
		LOG( TXT("Invalid files !!!"));
		for (Uint i=0; i<invalidFiles.Size(); i++)
		{
			LOG( TXT("%d %s") ,i ,invalidFiles[i].AsChar());
			ADD_TO_EMAIL( invalidFiles[i].AsChar() );
		}
	}
	
	// Finished
	ADD_TO_EMAIL( TXT("--------------------------------------") );
	LOG( TXT("\nFinished.\n\n"));

	if ( !email.Empty() )
		SendEmail( email );

	return true;
}

Bool CResLinkerEngine::ProcessFile( const String &absPath )
{
	IFile* file = GFileManager->CreateFileReader( absPath, FOF_Buffered | FOF_AbsolutePath );
	CReslinkerDependencyLoader loader( *file );
	TDynArray< String > imports;
	Bool needUpdate = file->GetVersion() < VER_CURRENT;

	if ( !needUpdate && loader.GetImports( imports ) )
	{
		for ( Uint i=0; i<imports.Size(); i++ )
		{
			for ( Uint j=0; j<m_links.Size(); j++ )
			{
				if ( imports[i] == m_links[j].ToLower() )
				{
					// Save link file path, cannot be deleted here because of saving file process
					needUpdate = true;
				}
			}
		}
	}

	delete file;
	return needUpdate;
}

void CResLinkerEngine::SendEmail( const String &address )
{
	String cmd = GFileManager->GetBaseDirectory(); 
	cmd += TXT("\\tools\\blat\\blat.exe");

	String commandLine = TXT("- -body \"");
	commandLine += m_emailText;
	commandLine += TXT("\" -server wiedzmin.pl.cdprojekt.com -try 72000 -subject \"Reslinker report\" -to \"");
	commandLine += address;
	commandLine += TXT("\" -charset windows-1250 -f \"The Automatic Reslinker Information Service <dont_respond@cdprojekt.com>\"");

	// Initialize process startup info
	STARTUPINFO startupInfo;
	ZeroMemory( &startupInfo, sizeof( STARTUPINFO ) );
	startupInfo.cb = sizeof( STARTUPINFO );

	// Process information
	PROCESS_INFORMATION processInformation;    
	ZeroMemory( &processInformation, sizeof( PROCESS_INFORMATION ) );

	Char* commandLineBuf = commandLine.TypedData();
	CreateProcess( cmd.AsChar(), commandLineBuf, NULL, NULL, false, DETACHED_PROCESS, NULL, NULL, &startupInfo, &processInformation ); 

	GFileManager->SaveStringToFile( GFileManager->GetBaseDirectory() + TXT("\\ReslinkerEmailText.txt"), m_emailText );
}

