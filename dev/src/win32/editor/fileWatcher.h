#pragma once

DECLARE_EVENT_TYPE(wxEVT_FILE_WATCHER_NOTIFY, 6501)

class CFileWatcher : public wxEvtHandler
{
public:
	CFileWatcher(String path, Bool watchSubtree = true, DWORD notifyFilter = (FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME));
	~CFileWatcher();

	void Update();

private:
	HANDLE m_handle;

};
