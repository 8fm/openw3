/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "../redIO/redIO.h"
#include "../core/feedback.h"
#include "../core/depot.h"

#include "redGuiList.h"
#include "redGuiPanel.h"
#include "redGuiButton.h"
#include "redGuiSaveFileDialog.h"
#include "redGuiManager.h"
#include "debugWindowPerformanceLog.h"

namespace DebugWindows
{
	CDebugWindowPerformanceLog::CDebugWindowPerformanceLog() 
		: RedGui::CRedGuiWindow(200,200,750,400)
		, m_prevLogListener(nullptr)
		, m_messagesList(nullptr)
		, m_pauseListen(nullptr)
		, m_clearList(nullptr)
		, m_saveToFile(nullptr)
		, m_saveFileDialog(nullptr)
		, m_pauseDisplay(false)
	{
		SetCaption(TXT("Performance log"));

		RedGui::CRedGuiPanel* bottomPanel = new RedGui::CRedGuiPanel(0, 350, GetWidth(), 25);
		bottomPanel->SetMargin(Box2(5, 5, 5, 5));
		AddChild(bottomPanel);
		bottomPanel->SetDock(RedGui::DOCK_Bottom);

		m_pauseListen = new RedGui::CRedGuiButton(0, 0, 100, 20);
		m_pauseListen->SetText(TXT("Pause listen"));
		m_pauseListen->SetToggleMode(true);
		m_pauseListen->EventCheckedChanged.Bind(this, &CDebugWindowPerformanceLog::NotifyEventCheckedChanged);
		bottomPanel->AddChild(m_pauseListen);
		m_pauseListen->SetDock(RedGui::DOCK_Left);

		m_clearList = new RedGui::CRedGuiButton(0, 0, 100, 20);
		m_clearList->SetText(TXT("Clear list"));
		m_clearList->EventButtonClicked.Bind(this, &CDebugWindowPerformanceLog::NotifyEventButtonClicked);
		bottomPanel->AddChild(m_clearList);
		m_clearList->SetDock(RedGui::DOCK_Left);

		m_saveToFile = new RedGui::CRedGuiButton(0, 0, 100, 20);
		m_saveToFile->SetText(TXT("Save to file"));
		m_saveToFile->EventButtonClicked.Bind(this, &CDebugWindowPerformanceLog::NotifyEventButtonClicked);
		bottomPanel->AddChild(m_saveToFile);
		m_saveToFile->SetDock(RedGui::DOCK_Left);

		m_messagesList = new RedGui::CRedGuiList(0,0, 600, 300);
		m_messagesList->AppendColumn( TXT("Messages"), 100 );
		m_messagesList->SetSelectionMode( RedGui::SM_None );
		m_messagesList->SetMargin(Box2(5, 5, 5, 5));
		AddChild(m_messagesList);
		m_messagesList->SetDock(RedGui::DOCK_Fill);

		// save file dialog
		m_saveFileDialog = new RedGui::CRedGuiSaveFileDialog();
		m_saveFileDialog->AddFilter(TXT("Text file"), TXT("txt"));
		m_saveFileDialog->EventFileOK.Bind( this, &CDebugWindowPerformanceLog::NotifyEventFileOK );
		m_saveFileDialog->EventWindowClosed.Bind( this, &CDebugWindowPerformanceLog::NotifyCloseSaveDialog );
	}

	CDebugWindowPerformanceLog::~CDebugWindowPerformanceLog()
	{
		
	}

	void CDebugWindowPerformanceLog::OnPendingDestruction()
	{
		m_pauseListen->EventCheckedChanged.Unbind(this, &CDebugWindowPerformanceLog::NotifyEventCheckedChanged);
		m_clearList->EventButtonClicked.Unbind(this, &CDebugWindowPerformanceLog::NotifyEventButtonClicked);
		m_saveToFile->EventButtonClicked.Unbind(this, &CDebugWindowPerformanceLog::NotifyEventButtonClicked);
	}

	void CDebugWindowPerformanceLog::SetVisible( Bool value )
	{
		CRedGuiWindow::SetVisible(value);

		if(value == true)
		{
			// Install this window as the on screen log
			m_prevLogListener = GScreenLog;
			GScreenLog = this;
		}
		else
		{
			if(m_prevLogListener != nullptr)
			{
				// Restore previous log listener
				GScreenLog = m_prevLogListener;
			}
		}
	}

	void CDebugWindowPerformanceLog::ClearPerfWarnings()
	{
		m_messagesList->RemoveAllItems();
	}

	void CDebugWindowPerformanceLog::PerfWarning( Float timeTook, const String& group, const Char* info, ... )
	{
		if(GetVisible() == true)
		{
			// Format text
			va_list arglist;
			va_start( arglist, info );
			Char formattedMessage[2048];
			Red::System::VSNPrintF( formattedMessage, ARRAY_COUNT(formattedMessage), info, arglist );

			// Assign color
			Color color = AssignCorrectColor( timeTook );

			// Performance warnings stays on the screen for 5s
			String msg = String::Printf( TXT("%1.2fms"), timeTook * 1000.0f );
			msg += TXT("         ");
			msg += group;
			msg += TXT("         ");
			msg += formattedMessage;

			if(m_pauseDisplay == false)
			{
				// Add to list
				m_messagesList->AddItem( msg, color );
				m_messagesList->ScrollToBottom();
			}
			else
			{
				// Add to container with paused messages
				m_pausedMessages.PushBack( SInternalMessageData( msg, color ) );
			}
		}
	}

	void CDebugWindowPerformanceLog::NotifyEventCheckedChanged( RedGui::CRedGuiEventPackage& eventPackage, Bool value )
	{
		m_pauseDisplay = value;

		if(m_pauseDisplay == false)
		{
			for(Uint32 i=0; i<m_pausedMessages.Size(); ++i)
			{
				const SInternalMessageData& msg = m_pausedMessages[i];
				m_messagesList->AddItem( msg.m_message, msg.m_color );
			}

			m_pausedMessages.Clear();
		}
	}

	void CDebugWindowPerformanceLog::NotifyEventButtonClicked( RedGui::CRedGuiEventPackage& eventPackage )
	{
		CRedGuiControl* sender = eventPackage.GetEventSender();

		if(sender == m_clearList)
		{
			m_messagesList->RemoveAllItems();
		}
		else if(sender == m_saveToFile)
		{
			m_pauseListen->SetToggleValue(true);
			m_pauseDisplay = true;

			m_saveFileDialog->SetVisible(true);
		}
	}

	Color CDebugWindowPerformanceLog::AssignCorrectColor( Float timeTook )
	{
		Color color = Color( 200, 255, 100 );

		if ( timeTook > 0.005f ) 
		{
			color = Color( 255, 255, 100 );
		}
		else if ( timeTook > 0.030f )
		{
			color = Color( 255, 100, 100 );
		}
		else if ( timeTook > 0.100f )
		{
			color = Color( 255, 100, 255 );
		}

		return color;
	}

	void CDebugWindowPerformanceLog::NotifyEventFileOK( RedGui::CRedGuiEventPackage& eventPackage )
	{
		String depotPath = String::EMPTY;
		GDepot->GetAbsolutePath(depotPath);
		depotPath += m_saveFileDialog->GetFileName();

		Uint32 helperSizer = 0;
		Red::IO::CNativeFileHandle dumpFile;
		if( dumpFile.Open( depotPath.AsChar(), Red::IO::eOpenFlag_WriteNew ) == false )
		{
			RED_LOG_ERROR( DebugWindows, TXT("Cannot open file: %s"), depotPath.AsChar() );
			GRedGui::GetInstance().MessageBox( TXT("File could not be opened for writing."), TXT("Error"), RedGui::MESSAGEBOX_Error );
			return;
		}

		for( Uint32 i=0; i<m_messagesList->GetItemCount(); ++i)
		{
			String itemName = m_messagesList->GetItemText( i ) + TXT("\n");
			dumpFile.Write( itemName.AsChar(), itemName.GetLength() * sizeof( Char ), helperSizer );
			if( helperSizer < itemName.Size() )
			{
				RED_LOG_WARNING( DebugWindows, TXT("Cannot write all data to file: %s"), depotPath.AsChar() );
			}
		}

		if( dumpFile.Flush() == false )
		{
			RED_LOG_ERROR( DebugWindows, TXT("Cannot flush file: %s"), depotPath.AsChar() );
		}

		if( dumpFile.Close() == false )
		{
			RED_LOG_ERROR( DebugWindows, TXT("Cannot close file: %s"), depotPath.AsChar() );
		}

		GRedGui::GetInstance().MessageBox( TXT("File has been saved correctly."), TXT("Success"), RedGui::MESSAGEBOX_Info );
	}

	void CDebugWindowPerformanceLog::NotifyCloseSaveDialog( RedGui::CRedGuiEventPackage& eventPackage )
	{
		m_pauseListen->SetToggleValue(false);
		m_pauseDisplay = false;
	}

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
