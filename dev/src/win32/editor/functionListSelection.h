/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "selectionEditor.h"

// Function list 
class CFunctionListSelection : public wxEvtHandler, public ICustomPropertyEditor
{
public:
	enum FunctionType
	{
		FT_SCENE,
		FT_QUEST,
		FT_REWARD
	};

private:
	FunctionType		m_type;
	CName				m_functionName;

public:
	CFunctionListSelection( CPropertyItem* item, FunctionType type );
	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual Bool SaveValue() override;

protected:
	void OnClearFunction( wxCommandEvent &event );
	void OnUseSelected( wxCommandEvent &event );

private:
	bool doesFunctionTypeMatch( CFunction* function ) const;
};
