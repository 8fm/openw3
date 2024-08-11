
#pragma once

class CEdAutoScriptWindow : public wxSmartLayoutPanel, public IEdEventListener
{
	wxDECLARE_EVENT_TABLE();
	wxDECLARE_CLASS( CEdAutoScriptWindow );

private:
	wxTextCtrl*		m_textCtrl;

private:
	Bool			m_running;
	Float			m_timer;

	TDynArray< TPair< Float, TDynArray< String > > > m_commands;
	TDynArray< Bool > m_processed;

public:
	CEdAutoScriptWindow( wxWindow* parent );
	virtual ~CEdAutoScriptWindow();

	void Tick( Float dt );

	void OnGameStarted();
	void OnGameEnded();

public: // wxSmartLayoutFrame
	virtual void SaveSession( CConfigurationManager &config );
	virtual void RestoreSession( CConfigurationManager &config );

public: // IEdEventListener
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

private:
	void ParseScriptText();
};
