#include "build.h"
#include "clientUsers.h"
#include "../../common/core/versionControl.h"
#include "../../common/core/depot.h"
#include "../../common/core/events.h"

#include <psapi.h>
#include <strsafe.h>

// P4V Libs
#if _MSC_VER == 1700
	#ifdef _WIN64
		#ifdef _DEBUG
		#define P4V_LIB_PATH "..\\..\\..\\external\\P4V\\x64\\lib\\VS2012\\debug\\"
		#else
		#define P4V_LIB_PATH "..\\..\\..\\external\\P4V\\x64\\lib\\VS2012\\release\\"
		#endif
	#else
		#ifdef _DEBUG
		#define P4V_LIB_PATH "..\\..\\..\\external\\P4V\\lib\\VS2012\\debug\\"
		#else
		#define P4V_LIB_PATH "..\\..\\..\\external\\P4V\\lib\\VS2012\\release\\"
		#endif
	#endif
#elif _MSC_VER == 1600
	#ifdef _WIN64
		#ifdef _DEBUG
		#define P4V_LIB_PATH "..\\..\\..\\external\\P4V\\x64\\lib\\debug\\"
		#else
		#define P4V_LIB_PATH "..\\..\\..\\external\\P4V\\x64\\lib\\release\\"
		#endif
	#else
		#ifdef _DEBUG
		#define P4V_LIB_PATH "..\\..\\..\\external\\P4V\\lib\\debug\\"
		#else
		#define P4V_LIB_PATH "..\\..\\..\\external\\P4V\\lib\\release\\"
		#endif
	#endif
#else
#error Unsupported compiler
#endif

#pragma comment (lib, P4V_LIB_PATH "libclient.lib")
#pragma comment (lib, P4V_LIB_PATH "libp4sslstub.lib")
#pragma comment (lib, P4V_LIB_PATH "librpc.lib")
#pragma comment (lib, P4V_LIB_PATH "libsupp.lib")

#define MAIN_THREAD_CHECK() ASSERT( SIsMainThread(), TXT("Perforce operation should be done only from the main thread") )

CSourceControlP4::CSourceControlP4( IVersionControlEditorInterface *iface, const SSourceControlSettings& settings )
	: m_interface( iface )
	, m_operational( false )
	, m_open( false )
	, m_warned( false )
	, automaticChangelists( true )
{
	// Create client API
	m_client = new ClientApi;

	// Set P4 settings
	SetSettings( settings );

	// Create initialization thread
	m_thread = new CVersionControlThread( this );
	m_thread->m_active.SetValue( true );
	m_thread->InitThread();
}

CSourceControlP4::~CSourceControlP4()
{
	Close();
	m_thread->m_active.SetValue( false );
	m_thread->JoinThread();
	delete m_client;
	delete m_thread;
}

Bool CSourceControlP4::TestConnection()
{
	CKeepAlive alive;
	CWatcher watcher;
	m_client->SetBreak(&alive);
	Run(TXT("info"), watcher, true);
	return alive.WasBroken();	
}

void CSourceControlP4::SetTagged()
{
	// setting protocol for tagged data; must be done before Init
	m_client->SetProtocol( "tag", "" );
	m_tagged = true;
}

Bool CSourceControlP4::Init(Bool tagged)
{
	// check if init isn't really required
	if ( m_open )
	{
		if ( ( m_tagged && tagged ) || ( !m_tagged && !tagged ) )
		{
			return true;
		}
	}

	// init is required
	Error error;
	// finish connection if it is working
	Close();

	if ( tagged )
	{
		SetTagged();
	}

	m_client->Init( &error );
	if ( error.Test() )
	{
		if ( !m_warned )
		{
			StrBuf buf;
			buf.Alloc( 2048 );
			error.Fmt( &buf );
			RED_LOG_WARNING( VersinControl, TXT("CSourceControlP4: '%s'"), ANSI_TO_UNICODE( buf.Text() ) );
			m_warned = true;
		}
		Close();
		return false;
	}
	m_open = true;
	return true;
}

void CSourceControlP4::Close()
{
	if ( m_open )
	{
		Error error;
		m_client->Final( &error );
		m_open = false;
	}
}

Bool CSourceControlP4::Run(const TDynArray< String > &arguments, const String &command, CWatcher &watcher, Bool tagged)
{
	if ( !Init( tagged ) )
	{
		return false;
	}

	m_client->SetVar("enableStreams");

	String args;
	char **argv = new char*[arguments.Size()];
	for ( Uint32 i = 0; i < arguments.Size(); i++ )
	{
		args += String::Printf( TXT(" (%d)='%s'"), i + 1, arguments[i].AsChar() );
		argv[i] = new char[arguments[i].Size()];
		Red::System::StringCopy( argv[i], UNICODE_TO_ANSI( arguments[i].AsChar() ), arguments[i].Size() );
	}
	m_client->SetArgv(arguments.Size(), argv);

	RED_LOG( VersinControl, TXT("P4 command='%s' arguments(%d) %s"), command.AsChar(), arguments.SizeInt(), args.AsChar() );

	m_client->Run(UNICODE_TO_ANSI(command.AsChar()), &watcher);

	for ( Uint32 i = 0; i < arguments.Size(); i++ )
	{
		delete argv[i];
	}
	delete argv;
	return true;
}

Bool CSourceControlP4::Run(const String &command, CWatcher &watcher, Bool tagged)
{
	TDynArray< String > arguments;
	return Run(arguments, command, watcher, tagged);
}

void CSourceControlP4::SetOperational( Bool value )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
	m_operational = value;
};

Bool CSourceControlP4::IsOperational()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
	Bool result = m_operational;
	return result;
}

Int32 CSourceControlP4::FStat(const String &unsafeFile, struct StatFields &stats)
{
	if ( !IsOperational() )
		return SC_ERROR;

	const String file = GetProperPath( unsafeFile );

	TDynArray< String > arguments;
	CFStatWatcher fStatWatcher(SC_OK, String::EMPTY, &stats);
	arguments.PushBack(file);
	Run( arguments, TXT("fstat"), fStatWatcher, true );
	return fStatWatcher.GetResult();
}

Int32 CSourceControlP4::DoCheckOut( const String &unsafePath, const SChangelist& changelist, Bool exclusive )
{
	MAIN_THREAD_CHECK();

	const String path = GetProperPath( unsafePath );

	CWatcher watcher(SC_CHECKED_OUT);
	TDynArray< String > arguments;
	if ( !IsDefaultChangelist( changelist ) )
	{
		arguments.PushBack( String::Printf( TXT("-c%d"), GetChangelistID( changelist ) ) );
	}
	arguments.PushBack( path );

	SEvents::GetInstance().DispatchEvent( CNAME( FileOperationStarted ), CreateEventData( path ) );
	Run( arguments, TXT("edit"), watcher, false);

	if( watcher.GetResult() && exclusive )
	{
		Run( arguments, TXT("lock"), watcher, false );
	}

	return watcher.GetResult();
}

EOpenStatus CSourceControlP4::GetOpenStatus( const String &unsafeFile )  
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return OS_Unknown;

	struct StatFields stats;
	const String file = GetProperPath( unsafeFile );
	Int32 result = FStat(file, stats);
	if ( result == SC_OK )
	{
		if ( stats.otherOpen.Size() )
			return OS_OpenedBySomeoneElse;
		else if ( stats.action == TXT("edit") || stats.action == TXT("add") )
			return OS_OpenedOnlyByMe;
	}
	return OS_NotOpen;
}

Bool CSourceControlP4::GetStatus( CDiskFile &file, TDynArray< String > *users )
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;

	struct StatFields stats;

	const String path = GetProperPath( file.GetAbsolutePath() );
	Int32 result = FStat(path, stats);
	if ( users )
	{
		for ( Uint32 i = 0; i < stats.otherOpen.Size(); i++ )
		{
			users->PushBack(stats.otherOpen[i]);
		}
	}
	if ( result != SC_OK )
	{
		switch ( result )
		{
		case SC_NOT_IN_DEPOT:
		case SC_DELETED:
			file.SetLocal();
			return false;
		default:
			file.SetCheckedIn();
			return false;
		}
	}
	else
	{
		if ( ( stats.haveRev != stats.headRev ) && ( stats.haveRev != -1 ) )
		{
			file.SetNotSynced();
			return false;
		}
		else if ( stats.action == TXT("edit") || stats.action == TXT("branch") || stats.action == TXT("integrate") )
		{
			file.SetCheckedOut();
		}
		else if ( stats.action == TXT("add") )
		{
			file.SetAdded();
		}
		else if ( stats.action == TXT("delete") )
		{
			file.SetDeleted();
		}
		else if ( stats.headAction == TXT("delete") )
		{
			file.SetLocal();
		}
		else
		{
			file.SetCheckedIn();
			return false;
		}
		return true;
	}
}

Bool CSourceControlP4::SilentCheckOut( CDiskFile &resource, Bool exclusive )
{
	TDynArray< String > users;
	GetStatus( resource, &users );
	if ( ( !IsOperational() ) || 
		( resource.IsCheckedOut() ) ||
		( resource.IsNotSynced() ) ||
		( !users.Empty() && exclusive ) )
	{
		// fail silently if something is wrong
		return false;
	}
	// perform check out
	const String path = GetProperPath( resource.GetAbsolutePath() );
	switch ( DoCheckOut( path, resource.GetChangelist(), false ) ){
		// finish without prompting, even if check  out failed
		case SC_CHECKED_OUT:
			resource.SetCheckedOut();
			SEvents::GetInstance().QueueEvent( CNAME( VersionControlStatusChanged ), NULL );
			return true;
		case SC_NOT_CHECKED_OUT:
			return false;
		case SC_NOT_IN_DEPOT:
			resource.SetLocal();
			return false;
		default:
			return false;
	}
}
Bool CSourceControlP4::EnsureCheckedOut( const TDynArray< CDiskFile* >& fileList, Bool exclusive )
{
	if ( !IsOperational() )
		return false;

	TDynArray< String > users;
	TDynArray< CDiskFile* > operationList;
	operationList.Reserve( fileList.Size() );
	for ( Uint32 i = 0, n = fileList.Size(); i != n; ++i )
	{
		CDiskFile* file = fileList[ i ];
		GetStatus( *file, &users );

		if ( file->IsNotSynced() )
		{
			m_interface->OnSyncRequired();
			return false;
		}
		if ( file->IsCheckedOut() )
		{
			continue;
		}
		operationList.PushBack( file );
	}
	
	if ( operationList.Empty() )
	{
		// NOTICE: we got little interface inconsistency with checkout function for function usability reasons
		//m_interface->OnDoubleCheckOut();
		//return false;
		return true;
	}

	if ( !users.Empty() )
	{
		Sort( users.Begin(), users.End() );
		for ( Uint32 i = 1; i < users.Size(); )
		{
			if ( users[ i ] == users[ i-1 ] )
			{
				users.RemoveAt( i );
			}
			else
			{
				++i;
			}
		}
		if ( m_interface->OnParallelUsers( operationList, users, exclusive ) == SC_CANCEL )
		{
			return false;
		}
	}
	
	Bool hasLocalFiles = false;
	for ( Uint32 i = 0, n = operationList.Size(); i != n; ++i )
	{
		CDiskFile* file = operationList[ i ];
		const String path = GetProperPath( file->GetAbsolutePath() );

		switch( DoCheckOut( path, file->GetChangelist(), false ) )
		{
		case SC_CHECKED_OUT:
			file->SetCheckedOut();
			break;
		case SC_NOT_CHECKED_OUT:
			// try to revert checked out files
			{
				for ( Uint32 j = 0; j < i; ++j )
				{
					CDiskFile* processedFile = operationList[ j ];
					Revert( *processedFile, true );
				}
			}
			m_interface->OnFailedCheckOut();
			return false;
		case SC_NOT_IN_DEPOT:
			hasLocalFiles = true;
			file->SetLocal();
			break;
		default:
			break;
		}
	}
	if ( hasLocalFiles )
	{
		m_interface->OnLocalFile();
	}

	SEvents::GetInstance().QueueEvent( CNAME( VersionControlStatusChanged ), NULL );
	return true;
}
Bool CSourceControlP4::CheckOut( CDiskFile &resource, Bool exclusive )
{
	if ( !IsOperational() )
		return false;
	TDynArray< String > users;
	GetStatus( resource, &users );
	if ( resource.IsResourceMergeable() == false )
	{
		if ( resource.IsCheckedOut() )
		{
			m_interface->OnDoubleCheckOut();
			return false;
		}	
		if ( resource.IsNotSynced() )
		{
			m_interface->OnSyncRequired();
			return false;
		}
		if ( ( users.Size() > 0 ) && ( m_interface->OnParallelUsers( resource, users, exclusive ) == SC_CANCEL ) )
		{
			return false;
		}
	}

	const String path = GetProperPath( resource.GetAbsolutePath() );

	switch ( DoCheckOut( path, resource.GetChangelist(), false ) )
	{
		case SC_CHECKED_OUT:
			resource.SetCheckedOut();
			SEvents::GetInstance().QueueEvent( CNAME( VersionControlStatusChanged ), NULL );
			return true;
		case SC_NOT_CHECKED_OUT:
			m_interface->OnFailedCheckOut();
			return false;
		case SC_NOT_IN_DEPOT:
			m_interface->OnLocalFile();
			resource.SetLocal();
			return false;
		default:
			return false;
	}
}
Bool CSourceControlP4::CheckOut( const String &unsafeAbsoluteFilePath, const SChangelist &changeList, Bool exclusive )
{
	if ( !IsOperational() )
		return false;

	const String absoluteFilePath = GetProperPath( unsafeAbsoluteFilePath );

	switch ( DoCheckOut( absoluteFilePath, changeList, exclusive ) )
	{
		case SC_CHECKED_OUT:
			SEvents::GetInstance().QueueEvent( CNAME( VersionControlStatusChanged ), NULL );
			return true;
		case SC_NOT_CHECKED_OUT:
			m_interface->OnFailedCheckOut();
			return false;
		case SC_NOT_IN_DEPOT:
			m_interface->OnLocalFile();
			return false;
		default:
			return false;
	}
}
Bool CSourceControlP4::Submit(CDiskFile &resource)
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
	{
		return false;
	}
	GetStatus( resource );
	if ( !resource.IsEdited() )
	{
		m_interface->OnNotEdited( resource );
		return false;
	}
	String description;
	if (m_interface->OnSubmit( description, resource ) == SC_CANCEL)
	{
		return false;
	}

	const String path = GetProperPath( resource.GetAbsolutePath() );

	CWatcher watcher( SC_SUBMITTED );
	TDynArray< String > arguments;
	arguments.PushBack(TXT("-d"));
	arguments.PushBack(description);
	arguments.PushBack(path);

	SEvents::GetInstance().DispatchEvent( CNAME( FileOperationStarted ), CreateEventData( path ) );
	Run( arguments, TXT("submit"), watcher, false);

	const Int32 submitResult = watcher.GetResult();
	if ( submitResult == SC_SUBMITTED || submitResult == SC_FILES_IDENTICAL )
	{
		resource.SetCheckedIn();
		resource.VerifyExistence();
		resource.Unmodify();
	}
	return submitResult == SC_SUBMITTED || submitResult == SC_FILES_IDENTICAL;
}

Bool CSourceControlP4::Submit( CDiskFile &resource, const String& description )
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
	{
		return false;
	}
	GetStatus( resource );
	if ( !resource.IsEdited() )
	{
		m_interface->OnNotEdited( resource );
		return false;
	}

	const String path = GetProperPath( resource.GetAbsolutePath() );

	CWatcher watcher( SC_SUBMITTED );
	TDynArray< String > arguments;
	arguments.PushBack(TXT("-d"));
	arguments.PushBack(description);
	arguments.PushBack(path);

	SEvents::GetInstance().DispatchEvent( CNAME( FileOperationStarted ), CreateEventData( path ) );
	Run( arguments, TXT("submit"), watcher, false);

	if ( watcher.GetResult() == SC_SUBMITTED )
	{
		resource.SetCheckedIn();
		resource.VerifyExistence();
		resource.Unmodify();
	}
	return watcher.GetResult() == SC_SUBMITTED;
}

Bool CSourceControlP4::Submit( CDirectory &directory )
{
	TDynArray< CDiskFile * > opened;
	Opened( opened );
	TSet< CDiskFile * > files;

	for ( Uint32 i = 0; i < opened.Size(); i++ )
	{
		if ( opened[i]->GetAbsolutePath().ToLower().BeginsWith( directory.GetAbsolutePath().ToLower() ) )
		{
			files.Insert( opened[i] );
		}
	}
	return Submit(opened, files);
}

Bool CSourceControlP4::Submit( CDirectory &directory, const String& description )
{
	TDynArray< CDiskFile * > opened;
	Opened( opened );
	TSet< CDiskFile * > files;

	for ( Uint32 i = 0; i < opened.Size(); i++ )
	{
		if ( opened[i]->GetAbsolutePath().ToLower().BeginsWith( directory.GetAbsolutePath().ToLower() ) )
		{
			files.Insert( opened[i] );
		}
	}
	return Submit( opened, files, description );
}

Bool CSourceControlP4::Submit( TDynArray< CDiskFile * > &files )
{
	TSet< CDiskFile * > chosen;
	for (Uint32 i = 0; i < files.Size(); i++)
	{
		chosen.Insert( files[i] );
	}
	return Submit( files, chosen );
}

Bool CSourceControlP4::Submit( TDynArray< CDiskFile * > &files, const String& description )
{
	TSet< CDiskFile * > chosen;
	for (Uint32 i = 0; i < files.Size(); i++)
	{
		chosen.Insert( files[i] );
	}
	return Submit( files, chosen, description );
}

Bool CSourceControlP4::Submit(TDynArray< CDiskFile * > &files, TSet< CDiskFile *> &chosen)
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;
	String description;
	String location;
	TDynArray< String > paths;
	TDynArray< String > arguments;

	// getting information about the submit from the user
	if ( m_interface->OnMultipleSubmit( description, files, chosen ) == SC_CANCEL )
		return false;

	struct StatFields stats;
	// creating list of files in depot form
	TSet< CDiskFile * > :: iterator i;
	for (i = chosen.Begin(); i != chosen.End(); i++)
	{
		// verifing if user has most recent revision
		const String path = GetProperPath( (*i)->GetAbsolutePath() );

		FStat( path, stats );
		if ( ( stats.haveRev != stats.headRev ) && ( stats.haveRev != -1 ) )
		{
			m_interface->OnSyncRequired();
			return false;
		}
		ToDepot( path, location );
		paths.PushBack(location);

		SEvents::GetInstance().DispatchEvent( CNAME( FileOperationStarted ), CreateEventData( path ) );
	}

	// inform the resources that the files are about to be added
	for (i = chosen.Begin(); i != chosen.End(); i++)
	{
		CResource* resource = (*i)->GetResource();
		if ( resource )
		{
			resource->OnBeforeAddToVersionControl();
		}
	}

	// running submit command
	CSubmitWatcher watcher( SC_SUBMITTED, ANSI_TO_UNICODE(m_client->GetUser().Text()),
		ANSI_TO_UNICODE(m_client->GetClient().Text()), description, paths);
	arguments.PushBack(TXT("-i"));
	Run( arguments, TXT("submit"), watcher, true);

	if ( watcher.GetResult() == SC_FILES_IDENTICAL )
	{
		m_interface->OnFilesIdentical();
		SEvents::GetInstance().QueueEvent( CNAME( VersionControlStatusChanged ), NULL );
		return true;
	}
	else if ( watcher.GetResult() != SC_SUBMITTED )
	{
		m_interface->OnFailedSubmit();
		SEvents::GetInstance().QueueEvent( CNAME( VersionControlStatusChanged ), NULL );
		return false;
	}

	for (i = chosen.Begin(); i != chosen.End(); i++)
	{
		(*i)->SetCheckedIn();
		(*i)->Unmodify();
		(*i)->VerifyExistence();
	}
	SEvents::GetInstance().QueueEvent( CNAME( VersionControlStatusChanged ), NULL );
	return true;
}

Bool CSourceControlP4::Submit( TDynArray< CDiskFile * > &files, TSet< CDiskFile *> &chosen, const String& description )
{
	RED_UNUSED( files );
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;
	String location;
	TDynArray< String > paths;
	TDynArray< String > arguments;

	struct StatFields stats;
	// creating list of files in depot form
	TSet< CDiskFile * > :: iterator i;
	for (i = chosen.Begin(); i != chosen.End(); i++)
	{
		// verifing if user has most recent revision
		const String path = GetProperPath( (*i)->GetAbsolutePath() );
		FStat( path, stats );

		if ( ( stats.haveRev != stats.headRev ) && ( stats.haveRev != -1 ) )
		{
			m_interface->OnSyncRequired();
			return false;
		}

		ToDepot( path, location );
		paths.PushBack(location);

		SEvents::GetInstance().DispatchEvent( CNAME( FileOperationStarted ), CreateEventData( path ) );
	}

	// inform the resources that the files are about to be added
	for (i = chosen.Begin(); i != chosen.End(); i++)
	{
		CResource* resource = (*i)->GetResource();
		if ( resource )
		{
			resource->OnBeforeAddToVersionControl();
		}
	}

	// running submit command
	CSubmitWatcher watcher( SC_SUBMITTED, ANSI_TO_UNICODE(m_client->GetUser().Text()),
		ANSI_TO_UNICODE(m_client->GetClient().Text()), description, paths);
	arguments.PushBack(TXT("-i"));
	Run( arguments, TXT("submit"), watcher, true);

	if ( watcher.GetResult() == SC_FILES_IDENTICAL )
	{
		m_interface->OnFilesIdentical();
		SEvents::GetInstance().QueueEvent( CNAME( VersionControlStatusChanged ), NULL );
		return true;
	}
	else if ( watcher.GetResult() != SC_SUBMITTED )
	{
		m_interface->OnFailedSubmit();
		SEvents::GetInstance().QueueEvent( CNAME( VersionControlStatusChanged ), NULL );
		return false;
	}

	for (i = chosen.Begin(); i != chosen.End(); i++)
	{
		(*i)->SetCheckedIn();
		(*i)->Unmodify();
		(*i)->VerifyExistence();
	}
	SEvents::GetInstance().QueueEvent( CNAME( VersionControlStatusChanged ), NULL );
	return true;
}

Bool CSourceControlP4::Submit( const SChangelist& changelist )
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() || IsDefaultChangelist( changelist ) )
	{
		return false;
	}
	
	CWatcher watcher( SC_SUBMITTED );
	TDynArray< String > arguments;
	arguments.PushBack( String::Printf( TXT("-c%u"), GetChangelistID( changelist ) ) );
	
	Run( arguments, TXT("submit"), watcher, false );

	const Int32 submitResult = watcher.GetResult();
	return submitResult == SC_SUBMITTED || submitResult == SC_FILES_IDENTICAL;
}

Bool CSourceControlP4::Revert(CDiskFile &resource, Bool silent)
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;

	if (!silent && !m_interface->OnConfirmRevert()) return true;

	if ( !resource.IsEdited() )
	{
		m_interface->OnNotEdited( resource );
		return false;
	}

	const String path = GetProperPath( resource.GetAbsolutePath() );

	// running revert command
	CWatcher watcher( SC_REVERTED );
	TDynArray< String > arguments;
	arguments.PushBack( path );

	SEvents::GetInstance().DispatchEvent( CNAME( FileOperationStarted ), CreateEventData( path ) );
	Run( arguments, TXT("revert"), watcher, false );

	if ( watcher.GetResult() == SC_REVERTED )
	{
		if ( resource.IsAdded() )
		{
			resource.SetLocal();
			resource.Unmodify();
			SEvents::GetInstance().QueueEvent( CNAME( VersionControlStatusChanged ), NULL );
		}
		else{
			resource.SetCheckedIn();
			resource.Unmodify();
			SEvents::GetInstance().QueueEvent( CNAME( VersionControlStatusChanged ), NULL );
		}
		return true;
	}
	return false;
}

Bool CSourceControlP4::Revert(TDynArray< CDiskFile * > &files )
{
	if (!m_interface->OnConfirmRevert()) return true;

	for ( Uint32 i = 0; i < files.Size(); i++ )
	{
		if ( !files[i]->IsEdited() )
		{
			m_interface->OnNotEdited( *files[i] );
			return false;
		}
	}
	for ( Uint32 i = 0; i < files.Size(); i++ )
	{
		Revert( *files[i], true );
	}
	return true;
}

Bool CSourceControlP4::RevertAbsolutePath( const String& unsafeAbsFileName )
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;

	if (!m_interface->OnConfirmRevert()) return true;

	const String absFileName = GetProperPath( unsafeAbsFileName );

	// running revert command
	CWatcher watcher( SC_REVERTED );
	TDynArray< String > arguments;
	arguments.PushBack(absFileName);

	SEvents::GetInstance().DispatchEvent( CNAME( FileOperationStarted ), CreateEventData( absFileName ) );
	Run( arguments, TXT("revert"), watcher, false );

	if ( watcher.GetResult() == SC_REVERTED )
	{
		String fileName;
		GDepot->ConvertToLocalPath(absFileName, fileName);
		String dirName = fileName.StringBefore(TXT("\\"),true)+TXT("\\");
		CDirectory* dir = GDepot->FindPath(dirName.AsChar());
		dir->Repopulate();
		fileName = fileName.StringAfter(TXT("\\"),true);
		CDiskFile* resource = dir->FindLocalFile(fileName);

		if ( resource->IsAdded() )
		{
			resource->SetLocal();
			resource->Unmodify();
			SEvents::GetInstance().QueueEvent( CNAME( VersionControlStatusChanged ), NULL );
		}
		else{
			resource->SetCheckedIn();
			resource->Unmodify();
			SEvents::GetInstance().QueueEvent( CNAME( VersionControlStatusChanged ), NULL );
		}
		return true;
	}
	return false;
}

Bool CSourceControlP4::Revert( const SChangelist& changelist, Bool unchangedOnly /*= false*/ )
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() || IsDefaultChangelist( changelist ) )
	{
		return false;
	}

	if ( !m_interface->OnConfirmRevert() )
	{
		return true;
	}

	// running revert command
	CWatcher watcher( SC_REVERTED );
	TDynArray< String > arguments;
	if ( unchangedOnly )
	{
		arguments.PushBack( TXT("-a") );
	}
	arguments.PushBack( String::Printf( TXT("-c%u"), GetChangelistID( changelist ) ) );

	Run( arguments, TXT("revert"), watcher, false );

	return watcher.GetResult() == SC_REVERTED;
}

Bool CSourceControlP4::Delete(CDiskFile &resource, Bool confirm /* = true */)
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;

	if (confirm)
	{
		if ( resource.IsModified() )
		{
			if (!(m_interface->OnConfirmModifiedDelete()))
				return false;			
		}
		else
		{
			if (!(m_interface->OnConfirmDelete()))
				return false;
		}
	}

	if ( resource.IsLoaded() )
	{
		resource.Unload();

		if ( resource.IsLoaded() )
		{
			m_interface->OnLoadedDelete();
			return false;
		}
	}

	const String path = GetProperPath( resource.GetAbsolutePath() );

	CWatcher watcher( SC_DELETED );
	TDynArray< String > arguments;
	arguments.PushBack( path );
	
	SEvents::GetInstance().DispatchEvent( CNAME( FileOperationStarted ), CreateEventData( path ) );	
	Run(arguments, TXT("delete"), watcher, false);

	if ( watcher.GetResult() != SC_DELETED )
	{
		RED_LOG_ERROR( VersinControl, TXT("'%s':"), resource.GetAbsolutePath().AsChar() );
		m_interface->OnFailedDelete();
	}
	resource.SetDeleted();
	return watcher.GetResult() == SC_DELETED;
}

Bool CSourceControlP4::Delete( TDynArray< CDiskFile * > &files, Bool confirm /* = true */ )
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;

	if (confirm)
	{
		// Is any file modified?
		Bool isModified = false;
		for (Uint32 i=0; i<files.Size(); i++)
		{
			if (files[i]->IsModified())
			{
				if (!(m_interface->OnConfirmModifiedDelete())) 
				{
					return false;
				}
				else
				{
					// One decision is enough 
					isModified = true;
					break;
				}
			}
		}

		// Confirm delete
		if (!isModified)
		{
			if (!(m_interface->OnConfirmDelete()))
				return false;
		}
	}

	// Is any file loaded
	for (Uint32 i=0; i<files.Size(); i++)
	{
		if (files[i]->IsLoaded())
		{
			m_interface->OnLoadedDelete();
			return false;
		}
	}

	CWatcher watcher( SC_DELETED );
	TDynArray< String > arguments;

	for (Uint32 i=0; i<files.Size(); i++) 
	{
		const String path = GetProperPath( files[i]->GetAbsolutePath() );

		arguments.PushBack( path );
		SEvents::GetInstance().DispatchEvent( CNAME( FileOperationStarted ), CreateEventData( path ) );
	}

	Run(arguments, TXT("delete"), watcher, false);
	if ( watcher.GetResult() != SC_DELETED )
	{
		for ( Uint32 i=0; i<files.Size(); ++i )
			RED_LOG_ERROR( VersinControl, TXT("'%s':"), files[i]->GetAbsolutePath().AsChar() );
		m_interface->OnFailedDelete();
	}

	for (Uint32 i=0; i<files.Size(); i++) files[i]->SetDeleted();

	return watcher.GetResult() == SC_DELETED;
}

Bool CSourceControlP4::Rename( CDiskFile &resource, const String &newAbsolutePath )
{
	MAIN_THREAD_CHECK();

	TDynArray< String > users;

	const String path = GetProperPath( resource.GetAbsolutePath() );

	if ( GetStatus( resource, &users ) || resource.IsLocal() )
	{
		TDynArray< String > arguments;

		CWatcher watcher( SC_OK );

		if ( resource.IsLocal() )
		{
			// Local rename
			if ( !MoveFileW( path.AsChar(), newAbsolutePath.AsChar() ) )
			{
				return false;
			}

			return true;
		}
		else
		{
			arguments.PushBack( resource.GetAbsolutePath() );
			arguments.PushBack( newAbsolutePath );

			SEvents::GetInstance().DispatchEvent( CNAME( FileOperationStarted ), CreateEventData( path ) );
			SEvents::GetInstance().DispatchEvent( CNAME( FileOperationStarted ), CreateEventData( newAbsolutePath ) );
			Run( arguments, TXT("move"), watcher, false );

			return watcher.GetResult() == SC_OK;
		}
	}

	return false;
}

Bool CSourceControlP4::Save( CDiskFile &resource )
{
	MAIN_THREAD_CHECK();

	const String path = GetProperPath( resource.GetAbsolutePath() );

	TDynArray< String > users;
	if ( GetStatus( resource, &users ) || resource.IsLocal() )
	{
		resource.Save( path );
		return true;
	}

	// file is locked or not synced - if it's a mergable file then get the latest revision and merge the file automatically
	if( resource.IsResourceMergeable() )
	{
		// checkout the file anyway
		if ( SoftSync( resource ) )
		{
			if ( resource.Save( resource.GetAbsolutePath() ) )
			{
				// auto merge file
				{
					CWatcher watcher( SC_OK );
					TDynArray< String > arguments;
					arguments.PushBack( TXT("-am") ); // automatic merge			 
					arguments.PushBack( path );
	
					SEvents::GetInstance().DispatchEvent( CNAME( FileOperationStarted ), CreateEventData( path ) );
					Run(arguments, TXT("resolve"), watcher, false);
				}

				return true;
			}
		}
	}

	return m_interface->OnSaveFailure( resource );
}

Bool CSourceControlP4::Edit( CDiskFile &resource )
{	
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;	

	const String path = GetProperPath( resource.GetAbsolutePath() );

	TDynArray< String > users;
	if ( GetStatus(resource, &users) || resource.IsLocal() )
	{
		return true;
	}
	if ( m_interface->OnCheckOutRequired(path, users) == SC_OK )
	{
		return CheckOut( resource );
	}
	else
	{
		return false;
	}
}

Bool CSourceControlP4::SoftSync( CDiskFile &resource )
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;

	const String path = GetProperPath( resource.GetAbsolutePath() );

	// run sync command
	{
		CWatcher watcher( SC_OK );
		TDynArray< String > arguments;
		arguments.PushBack( path );
	
		SEvents::GetInstance().DispatchEvent( CNAME( FileOperationStarted ), CreateEventData( path ) );
		Run(arguments, TXT("sync"), watcher, false);
	}

	// checkout file
	DoCheckOut( path, resource.GetChangelist(), false );
	return true;
}

Bool CSourceControlP4::GetLatest( CDiskFile &resource )
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;

	const String path = GetProperPath( resource.GetAbsolutePath() );

	// running sync command
	CWatcher watcher( SC_OK );
	TDynArray< String > arguments;
	arguments.PushBack( path );
	
	SEvents::GetInstance().DispatchEvent( CNAME( FileOperationStarted ), CreateEventData( path ) );
	Run(arguments, TXT("sync"), watcher, false);

	if( resource.IsResourceMergeable() )
	{
		// auto resolve any changes (file could been already checked out)
		CWatcher watcher( SC_OK );
		TDynArray< String > arguments;
		arguments.PushBack( TXT("-am") ); // automatic merge			 
		arguments.PushBack( path );
	
		SEvents::GetInstance().DispatchEvent( CNAME( FileOperationStarted ), CreateEventData( path ) );
		Run(arguments, TXT("resolve"), watcher, false);
		
		// assume everything went OK
		return true;
	}
	else
	{
		if ( watcher.GetResult() == SC_WRITABLE )
		{
			if ( m_interface->OnSyncFailed() == SC_FORCE )
			{
				arguments.PopBack();
				arguments.PushBack( TXT("-f") );
				arguments.PushBack( path );

				SEvents::GetInstance().DispatchEvent( CNAME( FileOperationStarted ), CreateEventData( path ) );
				Run( arguments, TXT("sync"), watcher, false );
			}
		}
		if ( watcher.GetResult() == SC_OK )
		{
			resource.Unmodify();
			resource.SetCheckedIn();
		}
		return watcher.GetResult() == SC_OK;
	}
}

Bool CSourceControlP4::GetLatest( CDirectory &directory, Bool force)
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;
	// running sync command
	CWatcher watcher( SC_OK );
	TDynArray< String > arguments;
	arguments.PushBack(directory.GetAbsolutePath() + TXT("...#head"));
	if ( force )
	{
		arguments.PushBack(TXT("-f"));
	}

	Run( arguments, TXT("sync"), watcher, false );

	return watcher.GetResult() == SC_OK;
}

// This one is so that we can check perforce in general on a path to a file
Bool CSourceControlP4::GetLatest( const String &unsafeAbsoluteFilePath, Bool force)
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;

	const String absoluteFilePath = GetProperPath( unsafeAbsoluteFilePath );

	// running sync command
	CWatcher watcher( SC_OK );
	TDynArray< String > arguments;
	arguments.PushBack(absoluteFilePath);
	if ( force )
	{
		arguments.PushBack(TXT("-f"));
	}

	Run( arguments, TXT("sync"), watcher, false );

	return watcher.GetResult() == SC_OK;
}

Bool CSourceControlP4::Opened( TDynArray< CDiskFile* > &files )
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
	{
		return false;
	}

	// Get depot path
	String path;
	GDepot->GetAbsolutePath( path );

	String location;

	// running opened command
	TDynArray< String > names;
	COpenedWatcher watcher(SC_OK, names);
	TDynArray< String > arguments;
	String workSpaceRootPath = path.LeftString( path.GetLength()-1 );
	workSpaceRootPath += TXT("/...");
	arguments.PushBack( workSpaceRootPath );
	Run( arguments, TXT("opened"), watcher, true );

	// gathering and creating disk files
	Int32 index = path.GetLength();
	for ( Uint32 i = 0; i < names.Size(); i++ )
	{
		if ( ToLocal( names[i], location ) )
		{
			location = location.RightString( location.Size() - index - 1 );
			location.MakeLower();

			CDiskFile *file = GDepot->FindFile( location );
			if ( !file )
			{	
				file = GDepot->CreateNewFile( location.AsChar() );
				if ( file )
				{
					file->SetDeleted();
				}
			}

			if ( file )
			{
				files.PushBack( file );
			}
		}
	}
	return watcher.GetResult() == SC_OK;
}

Bool CSourceControlP4::GetListOfFiles( const String& folderPath, TDynArray < String > &files)
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;

	TDynArray< String > names;
	CFilesWatcher watcher(SC_OK, names);
	TDynArray< String > arguments;
	arguments.PushBack(folderPath);

	Run( arguments, TXT("fstat"), watcher, false );

	for ( Uint32 i = 0; i < names.Size(); i++ )
	{
		files.PushBack( names[i] );
	}

	return watcher.GetResult() == SC_OK;
}

Bool CSourceControlP4::Add( CDiskFile& resource, const SChangelist& changelist  )
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;
	CWatcher watcher;
	TDynArray< String > arguments;
	if ( !IsDefaultChangelist( changelist ) )
	{
		arguments.PushBack( String::Printf( TXT("-c%d"), GetChangelistID( changelist ) ) );
	}

	if ( resource.IsResourceMergeable() )
	{
		arguments.PushBack( TXT("-ttext") );
	}
	else
	{
		arguments.PushBack( TXT("-tbinary+l") );
	}

	const String path = GetProperPath( resource.GetAbsolutePath() );
	arguments.PushBack( path );

	SEvents::GetInstance().DispatchEvent( CNAME( FileOperationStarted ), CreateEventData( path ) );
	Run( arguments, TXT("add"), watcher, false );

	if ( watcher.GetResult() == SC_OK )
	{
		resource.SetChangelist( changelist );
		resource.SetAdded();
		return true;
	}

	return false;
}

Bool CSourceControlP4::Lock( CDiskFile & resource )
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;

	const String path = GetProperPath( resource.GetAbsolutePath() );

	CWatcher watcher;
	TDynArray< String > arguments;
	arguments.PushBack( path );
	Run( arguments, TXT("lock"), watcher, false );
	if ( watcher.GetResult() == SC_OK )
	{
		return true;
	}
	return false;
}

Bool CSourceControlP4::FileLog( CDiskFile &file, TDynArray< THashMap< String, String > > &history )
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;

	const String path = GetProperPath( file.GetAbsolutePath() );

	CFileLogWatcher watcher( history );
	TDynArray< String > arguments;
	arguments.PushBack( path );
	Run( arguments, TXT("filelog"), watcher, true );

	m_interface->OnFileLog(file, history );

	return watcher.GetResult() == SC_OK;
}

Bool CSourceControlP4::FileLastEditedBy( CDiskFile &file, String& p4user )
{
	return FileLastEditedBy( file.GetAbsolutePath(), p4user );
}

Bool CSourceControlP4::FileLastEditedBy( const String& unsafeAbsoluteFilePath, String& p4user )
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;

	// If it fails, it'll default to the local safe path
	String path = GetProperPath( unsafeAbsoluteFilePath );
	ToDepot( path, path );

	TDynArray< THashMap< String, String > > history;

	// We're only requesting the latest log entry
	history.Reserve( 1 );

	CFileLogWatcher watcher( history );

	TDynArray< String > arguments;
	arguments.PushBack( TXT( "-m 1" ) );
	arguments.PushBack( path );

	Run( arguments, TXT("filelog"), watcher, true );

	if ( watcher.GetResult() == SC_OK )
	{
		p4user = history[ 0 ][ TXT( "Submitter" ) ] + TXT( "@" ) + history[ 0 ][ TXT( "Workspace" ) ];
		return true;
	}

	return false;
}

Bool CSourceControlP4::ToDepot( const String &unsafePath, String &result )
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;

	const String path = GetProperPath( unsafePath );

	CWhereWatcher watcher(SC_OK);
	TDynArray< String > arguments;
	arguments.PushBack(path);
	Run( arguments, TXT("where"), watcher, false);
	if ( watcher.GetResult() == SC_OK )
	{
		result = watcher.GetDepot();
		return true;
	}
	return false;
}

Bool CSourceControlP4::ToLocal( const String &unsafePath, String &result )
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;

	const String path = GetProperPath( unsafePath );

	CWhereWatcher watcher(SC_OK);
	TDynArray< String > arguments;
	arguments.PushBack(path);
	Run( arguments, TXT("where"), watcher, false);
	if ( watcher.GetResult() == SC_OK )
	{
		result = watcher.GetLocal();
		return true;
	}
	return false;
}

Bool CSourceControlP4::IsSourceControlDisabled() const
{
	return false;
}

void CSourceControlP4::SetSettings( const SSourceControlSettings& settings )
{
	m_client->SetUser( UNICODE_TO_ANSI( settings.m_user.AsChar() ) );
	m_client->SetClient( UNICODE_TO_ANSI( settings.m_client.AsChar() ) );
	m_client->SetHost( UNICODE_TO_ANSI( settings.m_host.AsChar() ) );
	m_client->SetPassword( UNICODE_TO_ANSI( settings.m_password.AsChar() ) );
	m_client->SetPort( UNICODE_TO_ANSI( settings.m_port.AsChar() ) );
	automaticChangelists = settings.m_automaticChangelists;
	Close();
}

Bool CSourceControlP4::AutomaticChangelistsEnabled() const
{
	return automaticChangelists;
}

Bool CSourceControlP4::CreateChangelist( const String& name, SChangelist& changelist )
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;
	CNewChangelistWatcher watcher( SC_OK, ANSI_TO_UNICODE(m_client->GetUser().Text()), ANSI_TO_UNICODE(m_client->GetClient().Text()), name );
	TDynArray< String > arguments;
	arguments.PushBack( TXT("-i") );
	Run( arguments, TXT("change"), watcher, false);
	if ( watcher.GetResult() == SC_OK && watcher.GetChangelistNumber() != 0 )
	{
		changelist = ConstructChangelistWithID( watcher.GetChangelistNumber() );
		return true;
	}
	return false;
}

Bool CSourceControlP4::CreateChangelistWithNumber( const Uint32 number, SChangelist& changelist )
{
	changelist = ConstructChangelistWithID( number );

	return true;
}

Bool CSourceControlP4::IsDefaultChangelist( const SChangelist& changelist ) const
{
	return GetChangelistID( changelist ) == 0;
}

Bool CSourceControlP4::GetAttribute(const String &unsafeAbsFileName, const String &name, String &value)
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;

	const String absFileName = GetProperPath( unsafeAbsFileName );

	struct StatFields stats;
	Int32 result = FStat(absFileName, stats);

	if ( result == SC_OK )
	{
		if (name == TXT("action"))
		{
			value = stats.action;
		}
		else if (name == TXT("haveRev"))
		{
			String s = String::Printf(TXT("%d"),stats.haveRev);
			value = s;
		}
		else if (name == TXT("headRev"))
		{
			String s = String::Printf(TXT("%d"),stats.headRev);
			value = s;
		}
		else if (name == TXT("headAction"))
		{
			value = stats.headAction;
		}
		else if (name == TXT("headChange"))
		{
			String s = String::Printf(TXT("%d"),stats.headChange);
			value = s;
		}

		return true;
	}

	return false;
}

Bool CSourceControlP4::ChangelistLog( Uint32 number, ChangelistDescription& out )
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;

	CChangelistWatcher watcher(SC_OK,out);
	TDynArray< String > arguments;
	arguments.PushBack( ToString( number ) );
	Run( arguments, TXT("describe"), watcher, true );

	//m_interface->OnChangelistLog( number, out );

	return watcher.GetResult() == SC_OK;
}
// Returns a list of change descriptions containing everything needed, like user, time, description etc
Bool CSourceControlP4::GetChangelists( String unsafePath, Uint32 fromChangelist, Uint32 toChangelist, TDynArray < ChangeDescription > &out )
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;

	const String path = GetProperPath( unsafePath );

	TDynArray< ChangeDescription > list;
	CChangesWatcher watcher(SC_OK,list);
	TDynArray< String > arguments;
	arguments.PushBack( TXT("-L") );
	arguments.PushBack( TXT("-s") );
	arguments.PushBack( TXT("submitted") );
	String newArg = path + TXT("@") + ToString( fromChangelist ) + TXT(",") + ( toChangelist == 0 ? TXT("now") : ToString( toChangelist ) );
	arguments.PushBack( newArg.AsChar() );

	Run( arguments, TXT("changes"), watcher, true );

	if ( watcher.GetResult() == SC_OK && list.Size() > 0 )
	{
		for ( Uint32 i=0; i <list.Size(); ++i )
		{
			const ChangeDescription& desc = list[i];
			out.PushBack( desc );
		}
	}

	return watcher.GetResult() == SC_OK;
}

Bool CSourceControlP4::GetLastChangelist( Uint32& number )
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;

	TDynArray< ChangeDescription > list;

	CChangesWatcher watcher(SC_OK,list);
	TDynArray< String > arguments;
	arguments.PushBack( TXT("-m") );
	arguments.PushBack( TXT("1") );
	Run( arguments, TXT("changes"), watcher, true );

	//m_interface->OnGetLastChangelist( number );

	if ( watcher.GetResult() == SC_OK && list.Size() > 0 )
	{
		for ( Uint32 i=0; i <list.Size(); ++i )
		{
			const ChangeDescription& desc = list[0];
			if ( FromString( desc.m_change, number ) )
			{
				break;
			}
		}
	}

	return watcher.GetResult() == SC_OK;
}

Bool CSourceControlP4::GetLastLocalChangelistNumber( Uint32& number )
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;

	TDynArray< ChangeDescription > list;

	CChangesWatcher watcher(SC_OK,list);
	TDynArray< String > arguments;

	String workSpace = GetWorkspace();

	arguments.PushBack( TXT("-m") );
	arguments.PushBack( TXT("1") );
	arguments.PushBack( String::Printf( TXT("//%s/...#have"), workSpace.AsChar() ) );
	Run( arguments, TXT("changes"), watcher, true );

	if ( watcher.GetResult() == SC_OK && list.Size() > 0 )
	{
		for ( Uint32 i=0; i <list.Size(); ++i )
		{
			const ChangeDescription& desc = list[0];
			if ( FromString( desc.m_change, number ) )
			{
				break;
			}
		}
	}

	return watcher.GetResult() == SC_OK;
}

// This one is so that we can check perforce in general on a path to a file and not only CFile or CDirectory
Bool CSourceControlP4::DoesFileExist( const String &unsafeAbsoluteFilePath )
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;
	if ( unsafeAbsoluteFilePath.Empty() )
		return false;

	const String absoluteFilePath = GetProperPath( unsafeAbsoluteFilePath );

	// running sync command
	CWatcher watcher( SC_OK );
	TDynArray< String > arguments;
	arguments.PushBack(absoluteFilePath);

	Run( arguments, TXT("files"), watcher, false );

	return watcher.GetResult() == SC_OK;
}

// This one is so that we can check perforce in general on a path to a file and not only CFile or CDirectory
Bool CSourceControlP4::DoesFolderExist( const String& absolutePath )
{
	MAIN_THREAD_CHECK();

	if ( !IsOperational() )
		return false;

	struct StatFields stats;
	Int32 result = FStat(absolutePath, stats);

	if ( result == SC_OK )
	{
		return true;
	}
	else
	{
		return false;
	}
}

String CSourceControlP4::GetUser() const
{
	return ANSI_TO_UNICODE( m_client->GetUser().Text() );
}

String CSourceControlP4::GetWorkspace() const
{
	return ANSI_TO_UNICODE( m_client->GetClient().Text() );
}

struct ThreadSafeLookup
{
public:
	Bool Get( const String& path, String& outPath )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );
		return m_data.Find( path, outPath );
	}

	void Set( const String& path, const String& newPath )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );
		m_data.Set( path, newPath );
	}

private:
	THashMap< String, String >		m_data;
	Red::Threads::CMutex			m_lock;
};

ThreadSafeLookup GFileMapping;

String CSourceControlP4::GetProperPath( const String& caseAgnosticPath )
{
	String retPath = caseAgnosticPath;

	if ( GFileMapping.Get( caseAgnosticPath, retPath ) )
		return retPath;

	HANDLE hFile = CreateFileW(caseAgnosticPath.AsChar(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if ( hFile != INVALID_HANDLE_VALUE )
	{
		WCHAR szPath[ MAX_PATH+1 ];
		ZeroMemory( szPath, sizeof(szPath) );

		const DWORD len = GetFinalPathNameByHandleW( hFile, szPath, MAX_PATH, FILE_NAME_OPENED );
		if ( len > 0 )
		{
			szPath[len] = 0;

			// get depot directory name
			const CFilePath depotPath( GDepot->GetRootDataPath() );
			if ( depotPath.GetNumberOfDirectories() > 1 )
			{
				const String depotDirName = depotPath.GetDirectories().Back();
				if ( !depotDirName.Empty() )
				{
					// get the part after "r4data"
					WCHAR* path2 = wcsstr( szPath, depotDirName.AsChar() );

					// override the base path with the actual depot root
					if ( path2 != nullptr )
					{
						retPath = GDepot->GetRootDataPath();
						retPath += (path2 + depotDirName.GetLength() + 1); // +1 because of path separator
					}
				}
			}
		}

		CloseHandle( hFile );
	}

	GFileMapping.Set( caseAgnosticPath, retPath );

	return retPath;
}
