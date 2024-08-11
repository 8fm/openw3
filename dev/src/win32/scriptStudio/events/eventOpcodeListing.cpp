/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "eventOpcodeListing.h"

wxDEFINE_EVENT( ssEVT_OPCODE_LISTING_EVENT, CSSOpcodeListingEvent );
wxIMPLEMENT_DYNAMIC_CLASS( CSSOpcodeListingEvent, wxEvent );

CSSOpcodeListingEvent::CSSOpcodeListingEvent( wxEventType commandType, int winid )
:	wxEvent( commandType, winid )
{

}

CSSOpcodeListingEvent::CSSOpcodeListingEvent( const vector< SOpcodeFunction >& functions )
:	wxEvent( wxID_ANY, ssEVT_OPCODE_LISTING_EVENT )
,	m_functions( functions )
{

}

CSSOpcodeListingEvent::~CSSOpcodeListingEvent()
{
}
