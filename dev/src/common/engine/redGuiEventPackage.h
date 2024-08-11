/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

namespace RedGui
{
	class CRedGuiControl;

	class CRedGuiEventPackage
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiInternals );
	public:
		CRedGuiEventPackage( CRedGuiControl* eventSender );

		CRedGuiControl* GetEventSender() const;

		void StopProcessing();
		Bool ProcessingIsStopped() const;

		void SetAsProcessed();
		Bool IsProcessed() const;

	private:
		CRedGuiEventPackage(){}

	private:
		CRedGuiControl*		m_eventSender;
		Bool				m_stopProcessing;
		Bool				m_processed;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
