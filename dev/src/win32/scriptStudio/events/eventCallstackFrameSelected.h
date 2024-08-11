/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __EVENT_CALLSTACK_FRAME_SELECTED_H
#define __EVENT_CALLSTACK_FRAME_SELECTED_H

#include "eventGoto.h"

class CCallstackFrameSelectedEvent : public CGotoEvent
{
public:
	explicit CCallstackFrameSelectedEvent();
	explicit CCallstackFrameSelectedEvent( wxEventType commandType, int winid );
	explicit CCallstackFrameSelectedEvent( Red::System::Uint32 stackFrame, const wxString& file, Red::System::Int32 line, Red::System::Bool generateHistory = false );

	virtual ~CCallstackFrameSelectedEvent();

	inline Red::System::Uint32 GetFrame() const { return m_stackFrame; }

private:
	virtual wxEvent* Clone() const override final;

private:
	Red::System::Uint32 m_stackFrame;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CCallstackFrameSelectedEvent );
};

wxDECLARE_EVENT( ssEVT_CALLSTACK_FRAME_SELECTED_EVENT, CCallstackFrameSelectedEvent );

#endif // __EVENT_CALLSTACK_FRAME_SELECTED_H
