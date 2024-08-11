/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __BUNDLER_FEEDBACK_H__
#define __BUNDLER_FEEDBACK_H__

#include "../../common/core/core.h"

#include "../../common/redThreads/redThreadsThread.h"

#define BUNDLE_WARNING( feedback, message, ... )\
if( feedback ) { feedback->MarkCompleted( Feedback::CS_Warning ); } \
	RED_LOG_WARNING( RED_LOG_CHANNEL( Bundler ), message, ##__VA_ARGS__ )

#define BUNDLE_ERROR( feedback, message, ... ) \
	if( feedback ) { feedback->MarkCompleted( Feedback::CS_Failure ); } \
	RED_LOG_ERROR( RED_LOG_CHANNEL( Bundler ), message, ##__VA_ARGS__ )

namespace Bundler
{
	class Feedback
	{
	public:
		static Bool SILENT;

		enum ECompletionState
		{
			CS_InProgress = 0,
			CS_Success,
			CS_Warning,
			CS_Failure
		};

	public:
		Feedback();
		~Feedback();

		static Bool Initialize( Uint32 numBundles );
		static void Shutdown();

		void NewProgressbar( const AnsiChar* name );
		void SetProgress( Float percentageComplete );

		void MarkCompleted( ECompletionState success );

	private:
		void Scroll();

	private:
		static Red::Threads::CAtomic< Uint32 > START_LINE;
		static Red::Threads::CAtomic< Uint32 > NEXT_LINE;

		static Red::Threads::CMutex SCROLL_LOCK;

		static HANDLE m_consoleHandle;

		static const Uint8 PIP_COMPLETE;
		static const Uint8 PIP_PENDING;
		static const Uint32 BAR_START;
		static const Uint32 BAR_LENGTH;
		static const Uint32 PROGRESS_NUMBER_START;
		static const Uint32 PROGRESS_NUMBER_LENGTH;
		static const Uint32 PROGRESS_NUMBER_END;
		static const Uint32 TIME_START;
		static const Uint32 TIME_LENGTH;
		static const Uint32 TIME_END;
		static const Uint32 FEEDBACK_END;
		static const Uint32 MAX_FILENAME_LENGTH;
		static const Uint32 BUFFER_SIZE = 128;

		ECompletionState m_state;
		Uint16 m_line;
		Uint32 m_completion;
		WORD m_attributes;
		Red::System::StopClock m_timer;
		
		//Typical Line: world_definition_bundle_0_0_0.bundle                        ---------- 100%
		//              ^ file                              ^ spare space           ^ progress bar
		CHAR_INFO m_buffer[ BUFFER_SIZE ];
	};
}

#endif // __BUNDLER_FEEDBACK_H__
