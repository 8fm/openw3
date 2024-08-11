#pragma once

#include "itemSelectionDialogs/basicSelectorDialogPropertyEditor.h"

class CEntityVoiceTagEditor : public CEdBasicSelectorDialogPropertyEditor
{
public:
	CEntityVoiceTagEditor( CPropertyItem* item );
	~CEntityVoiceTagEditor();

protected:
	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual Bool GrabValue( String& displayValue ) override;

	virtual void OnSelectDialog( wxCommandEvent& event ) override;
	void OnClearDialog( wxCommandEvent& event );
};
