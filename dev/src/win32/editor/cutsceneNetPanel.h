
#pragma once

#include "netPanel.h"

class CEdCutsceneEditor;

class CEdCutsceneNetPanel : public CEdAANetPanel
{
	DECLARE_EVENT_TABLE()

	CEdCutsceneEditor* m_editor;

public:
	CEdCutsceneNetPanel( wxWindow* parent, CEdCutsceneEditor* ed );

protected: 
	virtual void ProcessPacket( aaInt32 msgType, aaServer& server ) override;

	virtual void OnConnectionAccepted() override;
	virtual void OnConnectionLost() override;

private:
	void ProcessPacket_Camera( aaServer& server );
	void ProcessPacket_Main( aaServer& server );
	void ProcessPacket_Skel( aaServer& server );
	void ProcessPacket_UnknownMessage( aaServer& server );
};
