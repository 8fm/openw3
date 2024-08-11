/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//---------------------------------------------------------------------------

// Size limited static array of enter/exit events.

//---------------------------------------------------------------------------

template< const Uint32 N >
class TTriggerEventBuffer
{
public:
	struct Event
	{
		// trigger object (area)
		class CTriggerObject* m_object;

		// trigger activator
		class CTriggerActivator* m_activator;
	};

	// Entry events
	Event m_entryEvents[N];
	Uint32 m_numEntryEvents;

	// Exit events
	Event m_exitEvents[N];
	Uint32 m_numExitEvents;

public:
	RED_INLINE TTriggerEventBuffer()
	{
		m_numEntryEvents = 0;
		m_numExitEvents = 0;
	}

	RED_INLINE void ReportEntryEvent(class CTriggerObject* object, class CTriggerActivator* activator)
	{
		if (m_numEntryEvents < N)
		{
			m_entryEvents[m_numEntryEvents].m_object = object;
			m_entryEvents[m_numEntryEvents].m_activator = activator;
			m_numEntryEvents += 1;
		}
		else
		{
			HALT("Trigger event buffer overflow!");
		}
	}

	RED_INLINE void ReportExitEvent(class CTriggerObject* object, class CTriggerActivator* activator)
	{
		if (m_numExitEvents < N)
		{
			m_exitEvents[m_numExitEvents].m_object = object;
			m_exitEvents[m_numExitEvents].m_activator = activator;
			m_numExitEvents += 1;
		}
		else
		{
			HALT("Trigger event buffer overflow!");
		}
	}
};

//---------------------------------------------------------------------------

// Default event buffer has space for 1280 enter/exit events
typedef TTriggerEventBuffer<1280> CTriggerEventBuffer;

//---------------------------------------------------------------------------
