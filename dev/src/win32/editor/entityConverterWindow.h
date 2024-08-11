
#pragma once

class CEdEntityConverterWindow : public wxSmartLayoutPanel
{
	DECLARE_EVENT_TABLE()

	THandle< CEntityTemplate >	m_templ;
	THandle< CEntity >			m_entity;
	TDynArray< CClass* >		m_classes;

public:
	CEdEntityConverterWindow( wxWindow* parent, CEntityTemplate* templ, CEntity* entity );
	~CEdEntityConverterWindow();

protected:
	void FillChoice( const CEntityTemplate* templ );

protected:
	void OnCancelWin( wxCommandEvent& event );
	void OnExportEntity( wxCommandEvent& event );
};
