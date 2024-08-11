/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SS_EVENT_OPCODE_LISTING_H__
#define __SS_EVENT_OPCODE_LISTING_H__

struct SOpcodeLine
{
	Red::System::Uint32 m_line;
	wxString m_details;
};

struct SOpcodeFunction
{
	wxString m_file;
	vector< SOpcodeLine > m_lines;
};

class CSSOpcodeListingEvent: public wxEvent
{
public:
	CSSOpcodeListingEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	CSSOpcodeListingEvent( const vector< SOpcodeFunction >& functions );
	virtual ~CSSOpcodeListingEvent();

	inline const vector< SOpcodeFunction >& GetFunctions() const { return m_functions; }

private:
	virtual wxEvent* Clone() const override final { return new CSSOpcodeListingEvent( m_functions ); }

private:
	const vector< SOpcodeFunction > m_functions;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CSSOpcodeListingEvent );
};

wxDECLARE_EVENT( ssEVT_OPCODE_LISTING_EVENT, CSSOpcodeListingEvent );

#endif // __SS_EVENT_OPCODE_LISTING_H__
