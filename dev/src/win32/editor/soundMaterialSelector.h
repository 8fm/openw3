/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "selectionEditor.h"

class CEdSoundMaterialSelector : public wxEvtHandler, public ICustomPropertyEditor
{
protected:
	CEdChoice *m_ctrlChoice;

public:
	CEdSoundMaterialSelector( CPropertyItem* item );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual void CloseControls() override;

	virtual Bool GrabValue( String& displayValue ) override;
	virtual Bool SaveValue() override;

private:
	THashMap< Uint32, StringAnsi > m_materials;

	void FillChoices();
	void OnChoiceChanged( wxCommandEvent &event );
};
