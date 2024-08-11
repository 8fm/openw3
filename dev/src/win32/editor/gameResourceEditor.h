#pragma once

#include "..\..\common\engine\gameResource.h"

#include "../../common/engine/flashPlayer.h"

class CGameResource;

class CGameResourceEditor : public wxFrame
{
	DECLARE_EVENT_TABLE()

private:
	THandle< CGameResource >	m_gameResource;
	CEdPropertiesPage*			m_nodeProperties;

protected:
	//! Events
	void OnPropertiesChanged( wxCommandEvent& event );
	void OnSave( wxCommandEvent& event );

public:
	CGameResourceEditor( wxWindow *parent, class CGameResource* gameResource );
	~CGameResourceEditor();
};
