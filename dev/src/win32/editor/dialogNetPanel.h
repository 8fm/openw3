
#pragma once

#include "netPanel.h"

class CEdSceneEditor;

class CEdDialogNetPanel : public CEdAANetPanel
{
	DECLARE_EVENT_TABLE()

	CEdSceneEditor* m_mediator;

public:
	CEdDialogNetPanel( wxWindow* parent, CEdSceneEditor* ed );

protected: 
	virtual void ProcessPacket( aaInt32 msgType, aaServer& server ) override;

private:
	void ProcessPacket_Camera( aaServer& server );
	void ProcessPacket_Test( aaServer& server );

protected:
	void OnSendData( wxCommandEvent& event );
};
