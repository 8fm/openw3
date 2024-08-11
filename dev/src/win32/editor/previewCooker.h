/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CEdCookingThread;

/// Local preview cooker
class CEdPreviewCooker : public wxSmartLayoutPanel//, public IDebugChannelReciever
{
	DECLARE_EVENT_TABLE();

protected:
	struct CookerMessage
	{
		String		m_resource;
		String		m_message;
	};

protected:
	wxTextCtrl*					m_log;
	CEdCookingThread*			m_cookingThread;
	CEdTimer					m_tickTimer;
	wxStaticText*				m_statNumFiles;
	wxStaticText*				m_statNumErrors;
	wxStaticText*				m_statNumWarnings;
	wxStaticText*				m_statStatus;
	wxHtmlWindow*				m_cookReport;
	wxGauge*					m_statProgress;
	Uint32						m_numFiles;
	TDynArray< CookerMessage >	m_cookerErrors;
	TDynArray< CookerMessage >	m_cookerWarnings;
	String						m_cookerDirectory;
	String						m_cookerFile;
	String						m_cookerFileGame;
	wxString					m_imageFileError;
	wxString					m_imageFileWarning;
	wxString					m_imageFileKitten;
	wxString					m_xenonDeployDir;
	wxString					m_pcDeployDir;
	wxString					m_cookType;

public:
	CEdPreviewCooker( wxWindow* parent );
	~CEdPreviewCooker();

	void ClearLog();
	void PrintLog( const Char* txt, ... );

	static void ShowFrame();

protected:
	void OnStartCooking( wxCommandEvent& event );
	void OnKillCooking( wxCommandEvent& event );
	void OnDeployCook( wxCommandEvent& event );
	void OnRunCook( wxCommandEvent& event );
	void OnResetDevkit( wxCommandEvent& event );
	void OnCloseWindow( wxCloseEvent& event );
	void OnTaskEnded( wxTimerEvent& event );

protected:
	void ResetCookingInterface();
	void GenerateReport();
	void EnableCookingUI( Bool state );
	ECookingPlatform GetCookingPlatform();

protected:
	//virtual Bool ProcessDebugRequest( CName requestType, const CNetworkPacket& packet, CNetworkPacket& response );
};

