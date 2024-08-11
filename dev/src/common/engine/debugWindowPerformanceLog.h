/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiWindow.h"
#include "../core/feedback.h"

namespace DebugWindows
{
	class CDebugWindowPerformanceLog : public RedGui::CRedGuiWindow, public IOnScreenLog
	{
		struct SInternalMessageData
		{
			SInternalMessageData()
				: m_message(TXT(""))
				, m_color(Color::WHITE)
			{
				/* intentionally empty */
			}
			SInternalMessageData( const String& message, const Color& color)
				: m_message(message)
				, m_color(color)
			{
				/* intentionally empty */
			}

			String	m_message;	//!< message text
			Color	m_color;	//!< message text color
		};

	public:
		CDebugWindowPerformanceLog();
		~CDebugWindowPerformanceLog();

		// when visible was changed, class must connect or disconnect to log listener object
		void SetVisible(Bool value);

		// interface IOnScreenLog
		virtual void ClearPerfWarnings();
		virtual void PerfWarning( Float timeTook, const String& group, const Char* info, ... );

	private:
		void NotifyEventCheckedChanged( RedGui::CRedGuiEventPackage& eventPackage, Bool value);
		void NotifyEventButtonClicked( RedGui::CRedGuiEventPackage& eventPackage );

		void NotifyEventFileOK( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyCloseSaveDialog( RedGui::CRedGuiEventPackage& eventPackage );

		Color AssignCorrectColor(Float timeTook);

		virtual void OnPendingDestruction() override final;

		IOnScreenLog*						m_prevLogListener;	//!<
		RedGui::CRedGuiList*				m_messagesList;		//!<
		RedGui::CRedGuiButton*				m_pauseListen;		//!<
		RedGui::CRedGuiButton*				m_clearList;		//!<
		RedGui::CRedGuiButton*				m_saveToFile;		//!<
		RedGui::CRedGuiSaveFileDialog*		m_saveFileDialog;	//!<

		Bool								m_pauseDisplay;		//!<
		TDynArray< SInternalMessageData >	m_pausedMessages;	//!< Received message but not displayed on list
	};

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
