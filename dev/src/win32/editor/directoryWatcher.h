/**
  * Copyright © 2013 CD Projekt Red. All Rights Reserved.
  */

#pragma once

enum EFileActionFlag
{
	FAF_Added       = FLAG(0),
	FAF_Removed     = FLAG(1),
	FAF_Modified    = FLAG(2),
	FAF_Renamed     = FLAG(3),
	FAF_OnlyUnderVC = FLAG(4)
};
	
struct ChangedFileData
{
	ChangedFileData() {}

	ChangedFileData( const String& path )
		: m_actions( 0 ), m_path( path )
		{}

	Uint32 m_actions;
	String m_path;
};

class IDirectoryChangeListener
{
public:
	virtual void OnDirectoryChange( const TDynArray< ChangedFileData >& changes ) =0;
};


class CDirectoryWatcher 
	: public wxEvtHandler
	, public IEdEventListener
{
public:

	CDirectoryWatcher( 
		const String& pathToWatch,
		IDirectoryChangeListener* listener,
		Uint32 actionsToWatch // EFileActionFlag
		);

	~CDirectoryWatcher();

	void AddExcludedDirectoy( const String& path );

private:
	class WaitThread : public Red::Threads::CThread
	{
	public:
		WaitThread( CDirectoryWatcher* watcher );
		~WaitThread();

		void Stop();

	private:
		virtual void ThreadFunc() override;
		CDirectoryWatcher* m_watcher;
		::OVERLAPPED m_ovl;
	};

	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data ) override;

	void OnWatchThreadNewChange( wxEvent& evt );
	void OnTimer( wxTimerEvent& evt );

	String						m_pathToWatch;
	TDynArray< String >			m_excludedDirs;
	Uint32						m_actionsToWatch;
	Bool						m_listenerCalled;
	::HANDLE					m_directory;
	IDirectoryChangeListener*	m_listener;
	WaitThread*					m_waitThread;
	wxTimer						m_timer;

	TSet< String >				 m_ourFiles;
	TDynArray< ChangedFileData > m_changes;
	Red::Threads::CMutex		 m_mutex;
};
