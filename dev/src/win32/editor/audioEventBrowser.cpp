/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "audioEventBrowser.h"

CEdAudioEventSelectorDialog::CEdAudioEventSelectorDialog( wxWindow* parent, TDynArray< String >* events, const String& selectedEvent )
	: CEdItemSelectorDialog( parent, TXT( "/Frames/AudioEventSelectorDialog" ), TXT( "Audio Events" ), true )
	, m_events( events )
	, m_selectedEvent( selectedEvent )
{
	/* intentionally empty */
}

CEdAudioEventSelectorDialog::~CEdAudioEventSelectorDialog()
{
	/* intentionally empty */
}

void CEdAudioEventSelectorDialog::Populate()
{
	CTimeCounter counter2;
	for( Uint32 i=0; i < m_events->Size(); ++i )
	{
		const String& eventName = (*m_events)[i];
		Bool isSelected = ( m_selectedEvent == eventName );
		AddItem( eventName, &(*m_events)[i], true, -1, isSelected );
	}
	RED_LOG( RED_LOG_CHANNEL( TIME ), TXT("Populate tree time: %f"), counter2.GetTimePeriod() );
}
