/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

namespace RedGui
{
	class IRedGuiTimelineEvent
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		IRedGuiTimelineEvent() { /* intentionally empty */ };

		virtual void SetStopTime( Float stopTime ) = 0;

		virtual Float GetStartTime() const = 0;
		virtual Float GetDuration() const = 0;
		virtual String GetName() const = 0;
		virtual Color GetColor() const = 0;

	protected:
		~IRedGuiTimelineEvent() { /* intentionally empty */ };
	};

	class CRedGuiTimelineEvent : public IRedGuiTimelineEvent
	{
	public:
		CRedGuiTimelineEvent( const String& name, Float startTime, const Color& color );
		virtual ~CRedGuiTimelineEvent();

		virtual void SetStopTime( Float stopTime );

		virtual Float GetStartTime() const;
		virtual Float GetDuration() const;
		virtual String GetName() const;
		virtual Color GetColor() const;

	protected:
		Float	m_startTime;	//!< in seconds
		Float	m_duration;		//!< in seconds
		String	m_name;
		Color	m_color;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
