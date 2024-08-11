/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __EVENT_LINE_MOVE_H__
#define __EVENT_LINE_MOVE_H__

#include "../solution/slnDeclarations.h"

class CLineMoveEvent : public wxEvent
{
public:
	CLineMoveEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	CLineMoveEvent( const SolutionFilePtr& file, Red::System::Int32 line, Red::System::Int32 added );

	virtual ~CLineMoveEvent();

	inline const SolutionFilePtr& GetFile() const { return m_file; }
	inline Red::System::Int32 GetLine() const { return m_line; }
	inline Red::System::Int32 GetAdded() const { return m_added; }

private:
	virtual wxEvent* Clone() const override final { return new CLineMoveEvent( m_file, m_line, m_added ); }

private:
	SolutionFilePtr m_file;
	Red::System::Int32 m_line;
	Red::System::Int32 m_added;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CLineMoveEvent );
};

wxDECLARE_EVENT( ssEVT_LINE_MOVE_EVENT, CLineMoveEvent );

#endif // __EVENT_LINE_MOVE_H__
