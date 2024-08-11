/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#ifndef __DEBUGGER_TAB_LOCALS_H__
#define __DEBUGGER_TAB_LOCALS_H__

#include "variables.h"

#include "../../events/eventCallstackFrameSelected.h"

class CSSLocalsDebuggerTab : public CSSVariablesTabBase
{
	wxDECLARE_CLASS( CSSLocalsDebuggerTab );

public:
	CSSLocalsDebuggerTab( wxAuiNotebook* parent );
	virtual ~CSSLocalsDebuggerTab();

	void RequestUpdate( Red::System::Uint32 stackFrameIndex );
	virtual void Refresh() override final;

	void OnStackFrameSelected( CCallstackFrameSelectedEvent& event );

private:
	virtual Red::System::Uint32 GetItemExpansionStamp() override final;

	static const Red::System::Uint32 UPDATE_STAMP = 0x6c6f636c;	//"locl"
	static const Red::System::Uint32 STACK_STAMP = 0x7374636b;	//"stck"
};

#endif // __DEBUGGER_TAB_LOCALS_H__
