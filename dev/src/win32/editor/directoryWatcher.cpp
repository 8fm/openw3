/**
  * Copyright © 2013 CD Projekt Red. All Rights Reserved.
  */

#include "build.h"
#include "directoryWatcher.h"

#include "../../common/core/depot.h"

DEFINE_EVENT_TYPE( wxEVT_FILE_WATCHER_NEW_CHANGE )


namespace
{
	Bool IsFileUnderVC( const String& path )
	{
		const String& rootPath = GDepot->GetRootDataPath();
		if ( path.BeginsWith( rootPath ) )
		{
			String dataPath = path.RightString( path.Size() - rootPath.Size() );
			if ( CDiskFile* file = GDepot->FindFile( dataPath ) )
			{
				return !file->IsLocal();
			}
		}

		return false;
	}
}


CDirectoryWatcher::WaitThread::WaitThread( CDirectoryWatcher* watcher )
	: Red::Threads::CThread( "WaitThread ")
	, m_watcher( watcher )	
	, m_ovl()
{
	m_ovl.hEvent = ::CreateEvent( NULL, false, false, NULL );
}

CDirectoryWatcher::WaitThread::~WaitThread()
{
	::CloseHandle( m_ovl.hEvent );
}

void CDirectoryWatcher::WaitThread::ThreadFunc()
{
	const ::DWORD maxEventsAtTime = 2; // I haven't seen more than 2 (during external rename operation) 
	const ::DWORD bufSize = maxEventsAtTime  * ( sizeof( ::FILE_NOTIFY_INFORMATION ) + MAX_PATH*sizeof( wchar_t ) );
	void* buf = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_Default, bufSize, sizeof( size_t ) );

	::FILE_NOTIFY_INFORMATION* const info = reinterpret_cast< ::FILE_NOTIFY_INFORMATION* >( buf );

	while ( true )
	{
		::DWORD bytesReturned = 0;
		::DWORD filter = FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_SIZE|FILE_NOTIFY_CHANGE_LAST_WRITE;
		::ZeroMemory( buf, bufSize );
		if ( ::ReadDirectoryChangesW( m_watcher->m_directory, buf, bufSize, true, filter, &bytesReturned, &m_ovl, NULL ) )
		{
			if ( !::GetOverlappedResult( m_watcher->m_directory, &m_ovl, &bytesReturned, true ) )
			{
				// CancelIo called, end the loop
				break;
			}
			
			::FILE_NOTIFY_INFORMATION* curInfo = info;
			while ( curInfo ) // there may be more than one info returned in the buffer
			{
				String filePath = String( curInfo->FileName, curInfo->FileNameLength / sizeof( wchar_t ) );
				String fullPath = ( m_watcher->m_pathToWatch + filePath ).ToLower();
				Uint32 action = 0;

				if ( wxDirExists( fullPath.AsChar() ) )
				{
					break; // skip notification about directories
				}

				switch ( curInfo->Action )
				{
				case FILE_ACTION_ADDED:
					action = FAF_Added;
					break;
				case FILE_ACTION_MODIFIED:
 					action = FAF_Modified;
					break;
				case FILE_ACTION_REMOVED:
					action = FAF_Removed;
					break;
				case FILE_ACTION_RENAMED_OLD_NAME:
					action = FAF_Renamed;
					break;
				}

				{
					Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_watcher->m_mutex );

					if ( m_watcher->m_actionsToWatch & action )
					{
		 				if ( !m_watcher->m_ourFiles.Exist( fullPath ) ) // if the file is being altered by us, so do not track it
		 				{
							if ( !(m_watcher->m_actionsToWatch&FAF_OnlyUnderVC) || IsFileUnderVC( fullPath ) )
							{
								Bool excluded = false;
								for ( Uint32 i=0; i < m_watcher->m_excludedDirs.Size(); ++i )
								{
									if ( fullPath.BeginsWith( m_watcher->m_excludedDirs[i] ) )
									{
										excluded = true;
										break;
									}
								}

								if ( !excluded )
								{
									auto change = // find by path
										FindIf( m_watcher->m_changes.Begin(), m_watcher->m_changes.End(),
											[&fullPath]( const ChangedFileData& data ) { return data.m_path == fullPath; }
											);
									if ( change != m_watcher->m_changes.End() )
									{
										change->m_actions |= action;
									}
									else
									{
										m_watcher->m_changes.PushBack( ChangedFileData( fullPath ) );
										m_watcher->m_changes.Back().m_actions |= action;
									}
								}
							}
		 				}
					}
				}

				// Go to the next info
				curInfo = curInfo->NextEntryOffset
					? reinterpret_cast< ::FILE_NOTIFY_INFORMATION* >( reinterpret_cast< ::BYTE* >( curInfo ) + curInfo->NextEntryOffset )
					: nullptr;
			}

			// Send the event (only once to avoid spamming the message queue in case we do lots of 
			// file operations in the main thread)
			RunLaterOnce( [ this ](){ 
				m_watcher->QueueEvent( new wxCommandEvent( wxEVT_FILE_WATCHER_NEW_CHANGE ) ); 
			} );
		}
	}

	RED_MEMORY_FREE( MemoryPool_Default, MC_Default, buf );
}

void CDirectoryWatcher::WaitThread::Stop()
{
	::CancelIoEx( m_watcher->m_directory, &m_ovl );
}

CDirectoryWatcher::CDirectoryWatcher( 
	const String& pathToWatch,
	IDirectoryChangeListener* listener,
	Uint32 actionsToWatch
	)
	: m_pathToWatch( pathToWatch )
	, m_listener( listener )
	, m_listenerCalled( false )
	, m_actionsToWatch( actionsToWatch )
	, m_timer( this )
{
	m_directory = ::CreateFile( 
		pathToWatch.AsChar(),
		FILE_LIST_DIRECTORY,
		FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
		NULL, // security descriptor
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED, // file attributes
		NULL
	);

	if ( m_directory != nullptr && m_directory != INVALID_HANDLE_VALUE )
	{
		SEvents::GetInstance().RegisterListener( RED_NAME( FileOperationStarted ), this );

		Bind( wxEVT_FILE_WATCHER_NEW_CHANGE, &CDirectoryWatcher::OnWatchThreadNewChange, this );
		Bind( wxEVT_TIMER, &CDirectoryWatcher::OnTimer, this );
 		m_waitThread = new WaitThread( this );
 		m_waitThread->InitThread();
	}
	else
	{
		WARN_EDITOR( TXT("Cannot create a file watcher for path %s"), pathToWatch.AsChar() );
	}
}

CDirectoryWatcher::~CDirectoryWatcher()
{
	if ( m_directory != nullptr && m_directory != INVALID_HANDLE_VALUE )
	{
		m_waitThread->Stop();
		m_waitThread->JoinThread();
		delete m_waitThread;
		m_waitThread = NULL;

		::CloseHandle( m_directory );
		m_directory = NULL;

		SEvents::GetInstance().UnregisterListener( this );
	}
}

void CDirectoryWatcher::AddExcludedDirectoy( const String& path )
{
	m_excludedDirs.PushBack( path.ToLower() );

	if ( !m_excludedDirs.Back().EndsWith( TXT("\\") ) )
		m_excludedDirs.Back() += TXT("\\");
}

void CDirectoryWatcher::OnWatchThreadNewChange( wxEvent& event )
{
	m_timer.Start( 500, true ); // on every change restart the delay timer
}

void CDirectoryWatcher::OnTimer( wxTimerEvent& evt )
{
	TDynArray< ChangedFileData > changes;
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
		changes = m_changes;
		m_changes.Clear();
		m_ourFiles.Clear();
	}

	if ( m_listener && !m_listenerCalled && !changes.Empty() )
	{
		Red::System::ScopedFlag< Bool > listenerLock( m_listenerCalled = true, false );
		m_listener->OnDirectoryChange( changes );
	}
}

void CDirectoryWatcher::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == RED_NAME( FileOperationStarted ) )
	{
		String path = GetEventData< String >( data );
		//LOG_EDITOR( TXT("FileOperationStarted: %s"), path );

		{
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
			m_ourFiles.Insert( path );
		}
	}
}
