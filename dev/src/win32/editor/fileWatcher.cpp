#include "build.h"
#include "fileWatcher.h"

DEFINE_EVENT_TYPE(wxEVT_FILE_WATCHER_NOTIFY)

CFileWatcher::CFileWatcher(String path, Bool watchSubtree, DWORD notifyFilter)
: wxEvtHandler()
{
	// Create file change notification handle
	m_handle = ::FindFirstChangeNotification(path.AsChar(), (BOOL)watchSubtree, notifyFilter);
	if (m_handle == NULL || m_handle == INVALID_HANDLE_VALUE)
	{
		WARN_EDITOR(TXT("Cannot create a file watcher for the depot path %s"), path.AsChar());
	}
}
CFileWatcher::~CFileWatcher()
{
	// Close file change notification handle
	if (m_handle != NULL && m_handle != INVALID_HANDLE_VALUE)
	{
		::FindCloseChangeNotification(m_handle);
	}
}

void CFileWatcher::Update()
{
	if (m_handle != NULL && m_handle != INVALID_HANDLE_VALUE)
	{
		if (WaitForSingleObject(m_handle, 0) == WAIT_OBJECT_0)
		{
			// Prepare an event
			wxCommandEvent evt(wxEVT_FILE_WATCHER_NOTIFY);
			//e.SetClientObject(new wxStringClientData(value));
			evt.SetEventObject( this );
			evt.SetString( TXT("File watcher notification.") );

			// Send it
			ProcessEvent( evt );

			::FindNextChangeNotification(m_handle);
		}
	}
}
