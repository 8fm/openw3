#pragma once
#include "entityEditor.h"

class CEdEntityEditor;

class CEdGameplayParamEditor : wxEvtHandler
{
public:
	CEdGameplayParamEditor( CEdEntityEditor * entityEdit );

	void RefreshPropList		();
private:
	CEdEntityEditor		*		m_entityEditor;
	CEdPropertiesPage	*		m_propPage;
	wxListBox			*		m_paramsList;

protected:

	void OnPropAdded			( wxCommandEvent& event );
	void OnPropAddedInternal	( wxCommandEvent& event );
	void OnPropRemoved			( wxCommandEvent& event );
	void OnPropModified			( wxCommandEvent& event );
	void OnListChanged			( wxCommandEvent& event );

	RED_INLINE CEntity*			GetEntity() 
	{
		return m_entityEditor->GetPreviewPanel()->GetEntity(); 
	}
	RED_INLINE CEntityTemplate*	GetTemplate() 
	{
		return GetEntity() ? GetEntity()->GetEntityTemplate() : NULL ; 
	}

public:
	static Bool IsIncluded( const CEntityTemplate *const entTemplate, const CGameplayEntityParam *const gameplayEntityParam );
	static Bool IsFromAIPreset( const CEntityTemplate *const entTemplate, const CGameplayEntityParam *const gameplayEntityParam );
};

