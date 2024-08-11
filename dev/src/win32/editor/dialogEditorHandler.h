#pragma once

class IEdDialogEditorHandler : public wxEvtHandler
{
public:
	virtual void ConnectTo( wxWindow* window ) = 0;
	virtual void DisconnectFrom( wxWindow* window ) {};
};