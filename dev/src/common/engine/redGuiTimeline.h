/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiTimelineEvent.h"

namespace RedGui
{

	class CRedGuiTimeline
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiTimeline( const String& name );
		~CRedGuiTimeline();

		void SetName( const String& name );
		String GetName() const;

		void StartEvent( const String& eventName, Float startTime );
		void StopEvent( const String& eventName, Float stopTime );
		void AddEvent( const String& eventName, Float startTime, Float stopTime, Color color = Color::LIGHT_BLUE );

		void GetEvents( Float time, TDynArray< const IRedGuiTimelineEvent* >& events ) const;

		Uint32 GetEventCount() const;
		IRedGuiTimelineEvent* GetEvent( Uint32 index ) const;

		void ClearData();
		void ClearData( Uint32 amountOfFirstElements );

	private:
		IRedGuiTimelineEvent* FindEvent( const String& name );

	private:
		String	m_name;
		TDynArray<  CRedGuiTimelineEvent *, MC_RedGuiControls, MemoryPool_RedGui > m_events;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
