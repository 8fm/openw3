#pragma once

class CIDConnectorPack;
class CEdInteractiveDialogEditor;

class CEdIDConnectorPackEditor : public wxSmartLayoutPanel, public IEdEventListener
{
	DECLARE_CLASS( CEdIDConnectorPackEditor );
	DECLARE_EVENT_TABLE();

	friend class CEdInteractiveDialogEditor;

private:
	CIDConnectorPack*					m_pack;
	CEdPropertiesPage*					m_properties;

public:
	CEdIDConnectorPackEditor( wxWindow* parent, CIDConnectorPack* dialog = NULL );
	~CEdIDConnectorPackEditor();

	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

protected:
	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();

	void OnMenuGenAudio( wxCommandEvent& event );
	void OnMenuGenLipsync( wxCommandEvent& event );
	void OnMenuSave( wxCommandEvent& event );
	void OnMenuExit( wxCommandEvent& event );
	void OnClose( wxCloseEvent &event );

	static void GenerateAudioFiles( const CIDConnectorPack* pack, const String& path, Bool cookerMode );
	static void GenerateLipsyncFiles( const CIDConnectorPack* pack, const String& path, Bool cookerMode );
};