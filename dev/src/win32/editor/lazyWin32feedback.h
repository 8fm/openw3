
#pragma once

#include "../../common/core/feedback.h"

class CLazyWin32Feedback : public IFeedbackSystem
{
	TDynArray< String > m_msgs;
	TDynArray< String > m_warns;
	TDynArray< String > m_errors;

public:
	virtual void BeginTask( const Char* name, Bool canCancel ) { HALT( "BeginTask called from a CLazyWin32Feedback instance" ); }
	virtual void EndTask() { HALT( "EndTask called from a CLazyWin32Feedback instance" ); }
	virtual void UpdateTaskProgress( Uint32 current, Uint32 total ) { HALT( "ShowMultiBoolDialog called from a CLazyWin32Feedback instance" ); }
	virtual void UpdateTaskInfo( const Char* info, ... ) { HALT( "UpdateTaskInfo called from a CLazyWin32Feedback instance" ); }
	virtual Bool IsTaskCanceled() { HALT( "IsTaskCanceled called from a CLazyWin32Feedback instance" ); return true; }
	virtual Bool IsNullFeedback() { return false; };
	virtual Bool AskYesNo( const Char* info, ... ) { HALT( "AskYesNo called from a CLazyWin32Feedback instance" ); return false; }
	virtual EFeedbackYesNoCancelResult AskYesNoCancel( const Char* info, ... ) { ASSERT(0); return FeedbackCancel; }
	virtual Bool ShowMultiBoolDialog( const String&, const TDynArray<String>&, TDynArray<Bool>& ) { HALT( "ShowMultiBoolDialog called from a CLazyWin32Feedback instance" ); return false; }

	virtual void ShowMsg( const Char* title, const Char* msg, ... )
	{
		Char buffer[ 8192 ];

		// Print the text
		va_list arglist;
		va_start(arglist, msg);
		Red::System::VSNPrintF( buffer, ARRAY_COUNT(buffer), msg, arglist ); 

		m_msgs.PushBack( buffer );
	}

	virtual void ShowWarn( const Char* msg, ... )
	{
		Char buffer[ 8192 ];

		// Print the text
		va_list arglist;
		va_start(arglist, msg);
		Red::System::VSNPrintF( buffer, ARRAY_COUNT(buffer), msg, arglist ); 

		m_warns.PushBack( buffer );
	}

	virtual void ShowError( const Char* msg, ... )
	{
		Char buffer[ 8192 ];

		// Print the text
		va_list arglist;
		va_start(arglist, msg);
		Red::System::VSNPrintF( buffer, ARRAY_COUNT(buffer), msg, arglist ); 

		m_errors.PushBack( buffer );
	}

public:
	void ShowAll()
	{
		if ( m_msgs.Empty() && m_warns.Empty() && m_errors.Empty() )
		{
			return;
		}

		String msg;
		Uint32 total = 0;
		static Uint32 maxMsg = 20;

		if ( m_errors.Size() > 0 )
		{
			msg += TXT("Error(s):\n");

			for ( Uint32 i=0; i<m_errors.Size() && total < maxMsg; ++i )
			{
				msg += String::Printf( TXT("%s\n"), m_errors[i].AsChar() );

				ERR_EDITOR( TXT("%s"), m_errors[i].AsChar() );

				++total;
			}
		}

		if ( m_warns.Size() > 0 && total < maxMsg )
		{
			msg += TXT("Warning(s):\n");

			for ( Uint32 i=0; i<m_warns.Size() && total < maxMsg; ++i )
			{
				msg += String::Printf( TXT("%s\n"), m_warns[i].AsChar() );

				WARN_EDITOR( TXT("%s"), m_warns[i].AsChar() );

				++total;
			}
		}

		if ( m_msgs.Size() > 0 && total < maxMsg )
		{
			msg += TXT("Message(s):\n");

			for ( Uint32 i=0; i<m_msgs.Size() && total < maxMsg; ++i )
			{
				msg += String::Printf( TXT("%s\n"), m_msgs[i].AsChar() );

				LOG_EDITOR( TXT("%s"), m_msgs[i].AsChar() );

				++total;
			}
		}

		if ( total == maxMsg )
		{
			msg += TXT("(...)");
		}

		wxMessageBox( msg.AsChar(), wxT("Reimport all animations") );
	}
};
