/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "feedback.h"

namespace Bundler
{
	//////////////////////////////////////////////////////////////////////////
	// Constants
	const Uint32 Feedback::MAX_FILENAME_LENGTH		= 64;
	const Uint32 Feedback::BAR_LENGTH				= 10;
	const Uint32 Feedback::BAR_START				= Feedback::MAX_FILENAME_LENGTH + 1;
	const Uint32 Feedback::PROGRESS_NUMBER_START	= Feedback::BAR_START + Feedback::BAR_LENGTH + 1;
	const Uint32 Feedback::PROGRESS_NUMBER_LENGTH	= 4;
	const Uint32 Feedback::PROGRESS_NUMBER_END		= Feedback::PROGRESS_NUMBER_START + Feedback::PROGRESS_NUMBER_LENGTH;
	const Uint32 Feedback::TIME_START				= Feedback::PROGRESS_NUMBER_END + 1;
	const Uint32 Feedback::TIME_LENGTH				= 9; // " 00:00.00"
	const Uint32 Feedback::TIME_END					= Feedback::TIME_START + Feedback::TIME_LENGTH;
	const Uint32 Feedback::FEEDBACK_END				= Feedback::TIME_END;
	const Uint8 Feedback::PIP_PENDING				= 176;
	const Uint8 Feedback::PIP_COMPLETE				= 219;

	//////////////////////////////////////////////////////////////////////////
	// Statics
	Bool Feedback::SILENT( true );

	Red::Threads::CAtomic< Uint32 > Feedback::START_LINE( 0 );
	Red::Threads::CAtomic< Uint32 > Feedback::NEXT_LINE( 0 );
	Red::Threads::CMutex Feedback::SCROLL_LOCK;
	
	HANDLE Feedback::m_consoleHandle = INVALID_HANDLE_VALUE;

	//////////////////////////////////////////////////////////////////////////
	// Methods
	Feedback::Feedback()
	:	m_attributes( FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED )
	{
	}

	//////////////////////////////////////////////////////////////////////////
	Feedback::~Feedback()
	{

	}

	//////////////////////////////////////////////////////////////////////////
	Bool Feedback::Initialize( Uint32 numBundles )
	{
		if( !SILENT )
		{
			m_consoleHandle = GetStdHandle( STD_OUTPUT_HANDLE );

			if( m_consoleHandle == INVALID_HANDLE_VALUE )
			{
				return false;
			}

			CONSOLE_SCREEN_BUFFER_INFO info;
			GetConsoleScreenBufferInfo( m_consoleHandle, &info );

			// Configure the starting line based on the current cursor position (so that subsequent runs of the program don't overwrite what came before
			START_LINE.SetValue( info.dwCursorPosition.Y + 1 );

			// Make sure the console buffer is large enough for our purposes
			COORD size;
			size.X = info.dwSize.X;
			size.Y = Max( static_cast< SHORT >( START_LINE.GetValue() + numBundles + 2 ), info.dwSize.Y );

			if( !SetConsoleScreenBufferSize( m_consoleHandle, size ) )
			{
				RED_LOG_ERROR( RED_LOG_CHANNEL( Feedback ), TXT( "Resize buffer failure: %u" ), GetLastError() );
			}

			if( !SetConsoleTitle( TXT( "Bundle Builder" ) ) )
			{
				RED_LOG_ERROR( RED_LOG_CHANNEL( Feedback ), TXT( "Could not set console title: %u" ), GetLastError() );
			}
		}
		return true;
	}

	void Feedback::Shutdown()
	{
		if( !SILENT )
		{
			// Reset title
			WCHAR originalConsoleTitle[ 256 ];
			if( GetConsoleOriginalTitle( originalConsoleTitle, 256 ) )
			{
				if( !SetConsoleTitle( originalConsoleTitle ) )
				{
					RED_LOG_ERROR( RED_LOG_CHANNEL( Feedback ), TXT( "Could not set console original title: %u" ), GetLastError() );
				}
			}
			else
			{
				RED_LOG_ERROR( RED_LOG_CHANNEL( Feedback ), TXT( "Could not get console original title: %u" ), GetLastError() );
			}

			// Move carat to the end of the buffer
			COORD cursorPosition;
			cursorPosition.X = 0;
			cursorPosition.Y = static_cast< SHORT >( START_LINE.GetValue() + NEXT_LINE.Increment() + 2 );

			if( !SetConsoleCursorPosition( m_consoleHandle, cursorPosition ) )
			{
				RED_LOG_ERROR( RED_LOG_CHANNEL( Feedback ), TXT( "Could not set console cursor position: %u" ), GetLastError() );
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void Feedback::NewProgressbar( const AnsiChar* name )
	{
		if( !SILENT )
		{
			m_state = CS_InProgress;
			m_attributes = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE;
			m_timer.Reset();

			Scroll();

			m_line = static_cast< SHORT >( START_LINE.GetValue() + NEXT_LINE.Increment() );

			Uint32 nameLength = static_cast< Uint32 >( Red::System::StringLength( name ) );

			Red::System::MemoryZero( m_buffer, BUFFER_SIZE * sizeof( CHAR_INFO ) );

			for( Uint32 i = 0; i < nameLength; ++i )
			{
				m_buffer[ i ].Char.AsciiChar = name[ i ];
				m_buffer[ i ].Attributes = m_attributes;
			}

			for( Uint32 i = BAR_START; i < BAR_START + BAR_LENGTH; ++i )
			{
				m_buffer[ i ].Char.AsciiChar = PIP_PENDING;
				m_buffer[ i ].Attributes = m_attributes;
			}

			m_buffer[ PROGRESS_NUMBER_START ].Char.AsciiChar = '0';
			m_buffer[ PROGRESS_NUMBER_START ].Attributes = m_attributes;

			m_buffer[ PROGRESS_NUMBER_START + 1 ].Char.AsciiChar = '%';
			m_buffer[ PROGRESS_NUMBER_START + 1 ].Attributes = m_attributes;

			const AnsiChar* time = "00:00.00";
			RED_ASSERT( Red::System::StringLength( time ) <= TIME_LENGTH );

			for( Uint32 i = 0; i < TIME_LENGTH; ++i )
			{
				Uint32 position = i + TIME_START;

				m_buffer[ position ].Char.AsciiChar = time[ i ];
				m_buffer[ position ].Attributes = m_attributes;
			}

			COORD bufferSizeCoord;
			bufferSizeCoord.X = FEEDBACK_END;
			bufferSizeCoord.Y = 1;

			COORD writePosition;
			writePosition.X = 0;
			writePosition.Y = 0;

			CONSOLE_SCREEN_BUFFER_INFO info;
			GetConsoleScreenBufferInfo( m_consoleHandle, &info );

			info.srWindow.Top		= m_line;
			info.srWindow.Bottom	= m_line;

			BOOL success = WriteConsoleOutputA( m_consoleHandle, m_buffer, bufferSizeCoord, writePosition, &info.srWindow );

			if( !success )
			{
				RED_LOG_ERROR( RED_LOG_CHANNEL( Feedback ), TXT( "New Progressbar failure: %u" ), GetLastError() );
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void Feedback::SetProgress( Float percentageComplete )
	{
		if( !SILENT )
		{
			// We start over running buffers if the progress bar is longer than expected
			// So make sure it can't be more than 100% complete
			if( percentageComplete > 1.0f )
			{
				percentageComplete = 1.0f;
			}

			Uint32 numFullBars = static_cast< Uint32 >( BAR_LENGTH * percentageComplete );

			for( Uint32 i = 0; i < numFullBars; ++i )
			{
				m_buffer[ BAR_START + i ].Char.AsciiChar = PIP_COMPLETE;
				m_buffer[ BAR_START + i ].Attributes = m_attributes;
			}

			Uint32 progress = static_cast< Uint32 >( 100 * percentageComplete );

			AnsiChar progressNumber[ PROGRESS_NUMBER_LENGTH ];
			Int32 charsPrinted = Red::System::SNPrintF( progressNumber, PROGRESS_NUMBER_LENGTH, "%u", progress );

			m_buffer[ PROGRESS_NUMBER_START ].Char.AsciiChar = static_cast< CHAR >( progress );

			for( Int32 i = 0; i < charsPrinted; ++i )
			{
				m_buffer[ PROGRESS_NUMBER_START + i ].Char.AsciiChar = progressNumber[ i ];
				m_buffer[ PROGRESS_NUMBER_START + i ].Attributes = m_attributes;
			}

			m_buffer[ PROGRESS_NUMBER_START + charsPrinted ].Char.AsciiChar = '%';
			m_buffer[ PROGRESS_NUMBER_START + charsPrinted ].Attributes = m_attributes;

			for( Int32 i = PROGRESS_NUMBER_START + charsPrinted; i < TIME_START; ++i )
			{
				m_buffer[ i ].Char.AsciiChar = ' ';
				m_buffer[ i ].Attributes = 0;
			}

			Double timeTakenSoFar = m_timer.GetDelta();

			AnsiChar time[ TIME_LENGTH + 1 ];
			Double minutes = timeTakenSoFar / 60.0;
			Double minutesAsSeconds = static_cast< Int32 >( minutes ) * 60.0;
			Double seconds = timeTakenSoFar - minutesAsSeconds;
			Double hundreds = timeTakenSoFar - ( minutesAsSeconds + static_cast< Int32 >( seconds ) );
			Red::System::SNPrintF( time, TIME_LENGTH, "%02.Lf:%02.Lf.%02.Lf", minutes, seconds, hundreds * 100.0 );

			for( Uint32 i = 0; i < TIME_LENGTH; ++i )
			{
				Uint32 position = i + TIME_START;

				m_buffer[ position ].Char.AsciiChar = time[ i ];
				m_buffer[ position ].Attributes = m_attributes;
			}

			COORD bufferSizeCoord;
			bufferSizeCoord.X = TIME_END;
			bufferSizeCoord.Y = 1;

			COORD writePosition;
			writePosition.X = BAR_START;
			writePosition.Y = 0;

			CONSOLE_SCREEN_BUFFER_INFO info;
			GetConsoleScreenBufferInfo( m_consoleHandle, &info );

			info.srWindow.Top		= m_line;
			info.srWindow.Bottom	= m_line;
			info.srWindow.Left		= BAR_START;

			BOOL success = WriteConsoleOutputA( m_consoleHandle, m_buffer, bufferSizeCoord, writePosition, &info.srWindow );

			if( !success )
			{
				RED_LOG_ERROR( RED_LOG_CHANNEL( Feedback ), TXT( "Set Progress Write failure: %u" ), GetLastError() );
			}
		}
	}

	void Feedback::Scroll()
	{
		CONSOLE_SCREEN_BUFFER_INFO info;
		GetConsoleScreenBufferInfo( m_consoleHandle, &info );

		if( NEXT_LINE.GetValue() >= static_cast< Uint32 >( info.srWindow.Bottom ) - START_LINE.GetValue() )
		{
			info.srWindow.Top += 1;
			info.srWindow.Bottom += 1;

			if( !SetConsoleWindowInfo( m_consoleHandle, TRUE, &info.srWindow ) )
			{
				printf( "Scroll window failure: %u", GetLastError() );
			}
		}
	}

	void Feedback::MarkCompleted( ECompletionState success )
	{
		if( !SILENT )
		{
			if( m_state < success )
			{
				switch( success )
				{
				case CS_InProgress:
					m_attributes = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE;
					break;

				case CS_Success:
					m_attributes = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
					break;

				case CS_Warning:
					m_attributes = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY;
					break;

				case CS_Failure:
					m_attributes = FOREGROUND_RED | FOREGROUND_INTENSITY;
				}
			}

			m_state = success;

			WORD attributes[ FEEDBACK_END ];

			for( Uint32 i = 0; i < FEEDBACK_END; ++i )
			{
				attributes[ i ] = m_attributes;
			}

			COORD writePos;
			writePos.X = 0;
			writePos.Y = m_line;

			DWORD charsWritten = 0;

			if( !WriteConsoleOutputAttribute( m_consoleHandle, attributes, FEEDBACK_END, writePos, &charsWritten ) )
			{
				RED_LOG_ERROR( RED_LOG_CHANNEL( Feedback ), TXT( "WriteConsoleOutputAttribute() failed, error code: %u" ), GetLastError() );
			}
		}
	}
} // namespace Bundler
