/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiControl.h"
#include "redGuiEventPackage.h"

namespace RedGui
{

	CRedGuiEventPackage::CRedGuiEventPackage( CRedGuiControl* eventSender )
		: m_eventSender( eventSender )
		, m_stopProcessing( false )
		, m_processed( false )
	{
		/* intentionally empty */
	}

	CRedGuiControl* CRedGuiEventPackage::GetEventSender() const
	{
		return m_eventSender;
	}

	Bool CRedGuiEventPackage::ProcessingIsStopped() const
	{
		return m_stopProcessing;
	}

	void CRedGuiEventPackage::StopProcessing()
	{
		m_stopProcessing = true;
	}

	void CRedGuiEventPackage::SetAsProcessed()
	{
		m_processed = true;
	}

	Bool CRedGuiEventPackage::IsProcessed() const
	{
		return m_processed;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
