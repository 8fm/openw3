#include "build.h"

#include "../core/gatheredResource.h"
#include "../core/clipboardBase.h"

#include "renderFragment.h"
#include "renderFrameInfo.h"
#include "viewport.h"
#include "fonts.h"
#include "scriptInvoker.h"
#include "debugConsole.h"
#include "inputKeys.h"
#include "game.h"
#include "renderFrame.h"
#include "inputBufferedInputEvent.h"

CGatheredResource resDebugConsoleFont( TXT("gameplay\\gui\\fonts\\arial.w2fnt"), RGF_Startup );

CDebugConsole* GDebugConsole = NULL;

const Float		CDebugConsole::m_consoleAnimationDuration( 0.25f );
const Float		CDebugConsole::m_caretAnimationDuration( 0.7f );
const Float		CDebugConsole::m_keyPressNormalRepeatTime( 0.8f );
const Float		CDebugConsole::m_keyPressQuickRepeatTime( 0.1f );
const Color		CDebugConsole::m_consoleColor( 10, 10, 10, 140 );
const Color		CDebugConsole::m_popupColor( 20, 20, 20, 220 );
const Color		CDebugConsole::m_selectedPopupColor( 255, 255, 0, 210 );
const Color		CDebugConsole::m_caretColor( 255, 255, 255, 255 );
const Color		CDebugConsole::m_infoColor( 255, 255, 255, 190 );
const Color		CDebugConsole::m_warningColor( 255, 220, 40, 190 );
const Color		CDebugConsole::m_errorColor( 255, 0, 0, 190 );
const Color		CDebugConsole::m_commandColor( 0, 255, 0, 190 );
const Uint32	CDebugConsole::m_maxCommandLineHistoryEntries( 20 );
const Uint32	CDebugConsole::m_maxOutputHistoryEntries( 100 );

CDebugConsole::CDebugConsole( Bool isEnabled /*=true*/ )
	: m_font( NULL )
	, m_commandLine()
	, m_lastPressedKey( IK_None )
	, m_commandHistoryIndex( 0 )
	, m_caretPos( 0 )
	, m_selectedPropertyIndex( 0 )
	, m_consoleAnimationTime( m_consoleAnimationDuration )
	, m_caretAnimationTime( 0.f )
	, m_keyPressTime( 0.f )
	, m_popupWidth( 0.f )
	, m_popupHeight( 0.f )
	, m_overrideText( false )
	, m_visible( false )
	, m_shiftPressed( false )
	, m_ctrlPressed( false )
	, m_quickRepeatTime( false )
	, m_refreshIntelliSense( false )
	, m_isEnabled( isEnabled )
	, m_isAcceptingMessages( false )
{
	CTimeCounter timer;

	// Load resources
	m_font = resDebugConsoleFont.LoadAndGet< CFont >();
	m_commandHistory.PushBack( String::EMPTY );

	LOG_ENGINE( TXT( "Debug Console loaded in %1.2fs" ), timer.GetTimePeriod() );
}

CDebugConsole::~CDebugConsole()
{
}

void CDebugConsole::SetAcceptMessages( Bool value )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > threadLock( m_mutex );

	if ( m_isEnabled )
	{
		m_isAcceptingMessages = value;
	}
}

void CDebugConsole::Tick(float timeDelta)
{
	// Time scaling should not affect console
	//timeDelta /= GGame->GetTimeScale();

	if ( !GGame->IsCheatEnabled( CHEAT_Console ) )
	{
		return;
	}

	// Update console animation time
	if (m_consoleAnimationTime < m_consoleAnimationDuration)
	{
		m_consoleAnimationTime += timeDelta;
	}

	// Update caret animation time
	m_caretAnimationTime += timeDelta;
	if (m_caretAnimationTime > 0.5f * m_caretAnimationDuration)
	{
		m_caretAnimationTime -= m_caretAnimationDuration;
	}

	if (m_lastPressedKey != IK_None)
	{
		Float repeatTime = m_quickRepeatTime ? m_keyPressQuickRepeatTime : m_keyPressNormalRepeatTime;
		if (m_keyPressTime < repeatTime)
		{
			m_keyPressTime += timeDelta;
			if (m_keyPressTime >= repeatTime)
			{
				OnKeyDown(m_lastPressedKey);
				m_keyPressTime -= repeatTime;
				m_quickRepeatTime = true;
			}
		}
	}

	if (m_refreshIntelliSense)
	{
		m_refreshIntelliSense = false;
		
		// Refresh actual properties
		CScriptInvoker::GetAvailableProperties(m_commandLine, m_caretPos, m_intelliSenseProperties);
		m_selectedPropertyIndex = 0;

		// Update popup size
		m_popupWidth = 0.f;
		m_popupHeight = 0.f;
		Int32 unusedX, unusedY;
		Uint32 width, unusedHeight;
		for (Uint32 i = 0; i < m_intelliSenseProperties.Size(); ++i)
		{
			m_font->GetTextRectangle(m_intelliSenseProperties[i]->GetName().AsString(), unusedX, unusedY, width, unusedHeight);
			m_popupWidth = Max(m_popupWidth, Float(width));
		}
		m_popupHeight = m_intelliSenseProperties.Size() * (Float)m_font->GetLineDist();
	}
}

void CDebugConsole::OnGenerateFragments(CRenderFrame *frame)
{
	if (!frame || (!m_visible && m_consoleAnimationTime >= m_consoleAnimationDuration))
		return;

	if ( !m_font )
		return;

	Red::Threads::CScopedLock< Red::Threads::CMutex > threadLock( m_mutex );

	// Render console
	const Float s = Clamp<Float>(m_consoleAnimationTime / m_consoleAnimationDuration, 0.f, 1.f);
	const Float width = (Float)frame->GetFrameOverlayInfo().m_width;
	const Float height = (Float)frame->GetFrameOverlayInfo().m_height / 2;
	const Float lineDist = (Float)m_font->GetLineDist();
	const Float lineMargin = 10.f;
	const Float transformY = m_visible ? (s - 1.f) * height : -s * height;
	new ( frame ) CRenderFragmentDebugRectangle2D( frame, 0.0f, transformY, width, height, m_consoleColor );

	// Render output history
	const Float bottomLineY = transformY + height - lineDist;
	Float rowY = bottomLineY - lineDist;
	for (Int32 i = (Int32)m_outputHistory.Size() - 1; i >= 0; --i)
	{
		rowY -= lineDist;
		if (m_outputHistory[i].GetType() == DMT_Info)
			PrintText(frame, lineMargin, rowY, m_outputHistory[i].GetContent(), m_infoColor);
		else if (m_outputHistory[i].GetType() == DMT_Warning)
			PrintText(frame, lineMargin, rowY, m_outputHistory[i].GetContent(), m_warningColor);
		else if (m_outputHistory[i].GetType() == DMT_User)
			PrintText(frame, lineMargin, rowY, m_outputHistory[i].GetContent(), m_commandColor );
		else
			PrintText(frame, lineMargin, rowY, m_outputHistory[i].GetContent(), m_errorColor);
		if (rowY < 0.0f)
			break;
	}

	// Prepare command line 
	String textToShow = String::Printf( TXT("> %ls"), m_commandLine.AsChar() );

	// Render blinking caret
	if (m_caretAnimationTime >= 0.0f)
	{
		// Calculate text width
		Int32 unusedX = 0, unusedY = 0;
		Uint32 textWidth = 0, textHeight = 0;
		m_font->GetTextRectangle(textToShow.LeftString(m_caretPos + 2), unusedX, unusedY, textWidth, textHeight);

		Float caretWidth = 1.2f;
		Float caretHeight = (Float)textHeight + 4.f + 2.f;
		if (m_overrideText)
		{
			if (m_caretPos + 2 < (Int32)textToShow.GetLength())
			{
				Uint32 textWithCaretWidth = 0;
				m_font->GetTextRectangle(textToShow.LeftString(m_caretPos + 3), unusedX, unusedY, textWithCaretWidth, textHeight);
				caretWidth = Float(textWithCaretWidth - textWidth);
			}
			else
			{
				caretWidth = 6.0f;
			}
			caretWidth += 0.5f;
		}

		// Draw caret
		Matrix caretMatrix(Matrix::IDENTITY);
		const Float x = lineMargin + textWidth;
		const Float y = bottomLineY - 0.75f * lineDist;
		new ( frame ) CRenderFragmentDebugRectangle2D( frame, x, y, caretWidth, caretHeight, m_caretColor );
	}

	// Draw command line
	PrintText(frame, lineMargin, bottomLineY, textToShow, m_infoColor);

	// Draw IntelliSense popup menu
	if (m_intelliSenseProperties.Size() > 0)
	{
		// Calculate text width
		Int32 unusedX = 0, unusedY = 0;
		Uint32 textWidth = 0, unusedHeight = 0;
		m_font->GetTextRectangle(textToShow.LeftString(m_caretPos + 2), unusedX, unusedY, textWidth, unusedHeight);

		// Draw menu background
		const Float menuBorder = 5.0f;
		const Float popupX = lineMargin + textWidth;
		const Float popupY = bottomLineY + lineDist;
		const Float menuWidth = m_popupWidth + 2 * menuBorder;
		const Float menuHeight = m_popupHeight + 2 * menuBorder;
		const Float x = popupX - menuBorder;
		const Float y = popupY - menuBorder;
		new ( frame ) CRenderFragmentTexturedDebugRectangle(frame, x, y, menuWidth, menuHeight, NULL, m_popupColor);
		
		// Draw selected property item background
		const Float mx = popupX - menuBorder;
		const Float my = popupY + m_selectedPropertyIndex * lineDist;
		new ( frame ) CRenderFragmentTexturedDebugRectangle(frame, mx, my, menuWidth, lineDist, NULL, m_selectedPopupColor);

		// Draw menu items
		Float rowY = popupY + 0.75f * lineDist;
		for (Uint32 i = 0; i < m_intelliSenseProperties.Size(); ++i)
		{
			PrintText(frame, popupX, rowY + i * lineDist, m_intelliSenseProperties[i]->GetName().AsString(), m_infoColor);
		}

		// Draw menu border
		//static const Uint16 indices[8] = { 0, 1, 1, 2, 2, 3, 3, 0 };
		static DebugVertex points[4];
		points[0] = DebugVertex(Vector(-0.5f, -0.5f, 0.f), m_infoColor);
		points[1] = DebugVertex(Vector(menuWidth + 1.f, -0.5f, 0.f), m_infoColor);
		points[2] = DebugVertex(Vector(menuWidth + 1.f, menuHeight + 1.f, 0.f), m_infoColor);
		points[3] = DebugVertex(Vector(-0.5f, menuHeight + 1.f, 0.f), m_infoColor);
//		popupMatrix.SetTranslation(popupX - menuBorder, popupY - menuBorder, 0.3f);
//		new ( frame ) CRenderFragmentDebugIndexedLineList(frame, popupMatrix, points, 4, indices, 8, RSG_2D);
	}
}

Bool CDebugConsole::OnViewportInput(IViewport* view, enum EInputKey key, enum EInputAction action, Float data)
{
	if ( !GGame->IsCheatEnabled( CHEAT_Console ) )
	{
		return false;
	}

	// mind the shitfs and ctrls
	if (key == IK_LControl || key == IK_RControl)
		m_ctrlPressed = (action == IACT_Press);

	if (key == IK_LShift || key == IK_RShift || key == IK_Shift)
		m_shiftPressed = (action == IACT_Press);

	// if user pressed tilde change state of the console
	if ((key == IK_Tilde) && action == IACT_Press && !m_shiftPressed && !m_ctrlPressed)
	{
		m_visible = !m_visible;
		if (m_consoleAnimationTime < m_consoleAnimationDuration)
			m_consoleAnimationTime = m_consoleAnimationDuration - m_consoleAnimationTime;	// In case the previous animation has not finished yet, invert animation time
		else
			m_consoleAnimationTime = 0.f;	// Reset the animation time

		// Turn off IntelliSense
		m_intelliSenseProperties.Clear();
		
		return true;
	}

	// Return if console is not visible
	if (!m_visible)
	{
		return false;
	}

	// Process input
	if (action == IACT_Press)
	{
		m_lastPressedKey = key;
		m_keyPressTime = 0.0f;
		m_quickRepeatTime = false;
		OnKeyDown(key);
	}
	else if (action == IACT_Release)
	{
		m_lastPressedKey = IK_None;
		m_keyPressTime = 0.0f;
		m_quickRepeatTime = false;
		OnKeyUp(key);
	}

	return true;
}

void CDebugConsole::OnKeyDown(enum EInputKey key)
{
	if ( !GGame->IsCheatEnabled( CHEAT_Console ) )
	{
		return;
	}

	if (key == IK_Backspace)
	{
		RemoveAt(m_caretPos - 1);
		MoveCaretTo(m_caretPos - 1);
	}
	else if (key == IK_Tab)
	{
		// Insert selected property name into command line string
		if (m_intelliSenseProperties.Size() > 0)
		{
			Int32 dotPos = m_caretPos;
			for ( ; dotPos >= 0; --dotPos)
			{
				if (m_commandLine.AsChar()[dotPos] == TXT('.'))
					break;
			}
			String leftString = m_commandLine.LeftString(dotPos + 1);
			String rightString = m_commandLine.RightString(m_commandLine.GetLength() - m_caretPos);
			String middleString = m_intelliSenseProperties[m_selectedPropertyIndex]->GetName().AsString();
			m_commandLine = leftString + middleString + rightString;
			m_intelliSenseProperties.Clear();
			MoveCaretTo((Int32)leftString.GetLength() + (Int32)middleString.GetLength());
		}
	}
	else if (key == IK_Enter)
	{
		// Insert selected property name into command line string
		if (m_intelliSenseProperties.Size() > 0)
		{
			Int32 dotPos = m_caretPos;
			for ( ; dotPos >= 0; --dotPos)
			{
				if (m_commandLine.AsChar()[dotPos] == TXT('.'))
					break;
			}
			String leftString = m_commandLine.LeftString(dotPos + 1);
			String rightString = m_commandLine.RightString(m_commandLine.GetLength() - m_caretPos);
			String middleString = m_intelliSenseProperties[m_selectedPropertyIndex]->GetName().AsString();
			m_commandLine = leftString + middleString + rightString;
			m_intelliSenseProperties.Clear();
			MoveCaretTo((Int32)leftString.GetLength() + (Int32)middleString.GetLength());
		}
		else
		{
			m_commandLine.Trim();
			if ( !m_commandLine.Empty() )
			{
				// If current command line content is not the same as the last one then add it to the history
				if (!(m_commandHistory.Size() > 1 && m_commandLine == m_commandHistory[m_commandHistory.Size() - 2]))
				{
					m_commandHistory.Last() = m_commandLine;
					m_commandHistory.PushBack( String::EMPTY );
					if (m_commandHistory.Size() > m_maxCommandLineHistoryEntries)
					{
						m_commandHistory.Erase(m_commandHistory.Begin());
					}
				}
				m_commandHistoryIndex = (Int32)m_commandHistory.Size() - 1;

				// Taint regardless of whether successfully executed command. Otherwise we can 
				// exploit existing debug commands to unlock achievements (or other things) and *then*
				// the game becomes unkosher too late. We can "taint" a savegame, but not an online profile.
				// Ideally the setting it to unkosher would be before actually exec'ing the command.
#ifdef RED_FINAL_BUILD
				GGame->SetKosherTaintFlags( CGame::eKosherTaintFlag_Session );
#endif

				// Output the command line to the console
				OutputToConsole( CDebugMessage( m_commandLine, DMT_User ), true );

				// Parse the command line
				if ( !CScriptInvoker::Parse( m_commandLine ) )
				{
					WARN_ENGINE( TXT("Invalid command") );
				}

				// Reset
				m_commandLine.Clear();
				MoveCaretTo(0);
			}
		}
	}
	else if (key == IK_Escape)
	{
		m_intelliSenseProperties.Clear();
	}
	else if (key == IK_End)
	{
		MoveCaretTo(m_commandLine.GetLength());
	}
	else if (key == IK_Home)
	{
		MoveCaretTo(0);
	}
	else if (key == IK_Left)
	{
		if (m_ctrlPressed)
		{
			String leftString = m_commandLine.LeftString(m_caretPos);
			leftString.TrimRight();
			const Char *ch = leftString.AsChar();
			for (Int32 i = leftString.GetLength() - 1; i >= 0; --i)
			{
				if (ch[i] == TXT(' ') || ch[i] == TXT('.'))
				{
					MoveCaretTo(i);
					break;
				}
			}
		}
		else
			MoveCaretTo(m_caretPos - 1);
	}
	else if (key == IK_Right)
	{
		if (m_ctrlPressed)
		{
			String rightString = m_commandLine.RightString(m_commandLine.GetLength() - m_caretPos - 1);
			rightString.TrimLeft();
			const Char *ch = rightString.AsChar();
			Uint32 i;
			for (i = 0; i < rightString.GetLength(); ++i)
			{
				if (ch[i] == TXT(' ') || ch[i] == TXT('.'))
					break;
			}
			MoveCaretTo(m_caretPos + i + 1);
		}
		else
			MoveCaretTo(m_caretPos + 1);
	}
	else if (key == IK_Up)
	{
		if (m_intelliSenseProperties.Size() > 0)
		{
			m_selectedPropertyIndex = Max(m_selectedPropertyIndex - 1, 0);
		}
		else if (m_commandHistoryIndex - 1 >= 0 && m_commandHistoryIndex - 1 < (Int32)m_commandHistory.Size())
		{
			m_commandLine = m_commandHistory[--m_commandHistoryIndex];
			MoveCaretTo((Int32)m_commandLine.GetLength());
		}
	}
	else if (key == IK_Down)
	{
		if (m_intelliSenseProperties.Size() > 0)
		{
			m_selectedPropertyIndex = Min(m_selectedPropertyIndex + 1, (Int32)m_intelliSenseProperties.Size() - 1);
		}
		else if (m_commandHistoryIndex + 1 < (Int32)m_commandHistory.Size())
		{
			m_commandLine = m_commandHistory[++m_commandHistoryIndex];
			MoveCaretTo((Int32)m_commandLine.GetLength());
		}
	}
	else if (key == IK_Insert)
	{
		m_overrideText = !m_overrideText;
	}
	else if (key == IK_Delete)
	{
		RemoveAt(m_caretPos);
	}
	else if( key == IK_V && m_ctrlPressed )
	{
		String clipboardContents;
		if( GClipboard && GClipboard->Paste( clipboardContents ) )
		{
			InsertAt( m_caretPos, clipboardContents );
			MoveCaretTo( m_caretPos + clipboardContents.GetLength() );
		}
	}
	else 
	{
		// Process visible chars
		Char charToInsert = (Char)key;

		if (key >= IK_A && key <= IK_Z)	// letters
		{
			if (!m_shiftPressed)
				charToInsert += 0x20;
		}
		else if (key >= IK_0 && key <= IK_9)	// digits
		{
			if (m_shiftPressed)
			{
				switch (key)
				{
				case IK_0: charToInsert = ')'; break;
				case IK_1: charToInsert = '!'; break;
				case IK_2: charToInsert = '@'; break;
				case IK_3: charToInsert = '#'; break;
				case IK_4: charToInsert = '$'; break;
				case IK_5: charToInsert = '%'; break;
				case IK_6: charToInsert = '^'; break;
				case IK_7: charToInsert = '&'; break;
				case IK_8: charToInsert = '*'; break;
				case IK_9: charToInsert = '('; break;
				default: return;
				}
			}
		}
		else if (key == IK_Space)
			charToInsert = TXT(' ');
		else if (key  == IK_Period)
			charToInsert = m_shiftPressed ? TXT('>') : TXT('.');
		else if (key  == IK_Semicolon)
			charToInsert = m_shiftPressed ? TXT(':') : TXT(';');
		else if (key  == IK_Equals)
			charToInsert = m_shiftPressed ? TXT('+') : TXT('=');
		else if (key  == IK_Comma)
			charToInsert = m_shiftPressed ? TXT('<') : TXT(',');
		else if (key  == IK_Minus)
			charToInsert = m_shiftPressed ? TXT('_') : TXT('-');
		else if (key  == IK_Backslash)
			charToInsert = m_shiftPressed ? TXT('|') : TXT('\\');
		else if (key  == IK_Tilde)
			charToInsert = m_shiftPressed ? TXT('~') : TXT('`');
		else if (key  == IK_LeftBracket)
			charToInsert = m_shiftPressed ? TXT('{') : TXT('[');
		else if (key  == IK_Slash)
			charToInsert = m_shiftPressed ? TXT('?') : TXT('/');
		else if (key  == IK_RightBracket)
			charToInsert = m_shiftPressed ? TXT('}') : TXT(']');
		else if (key  == IK_SingleQuote)
			charToInsert = m_shiftPressed ? TXT('"') : TXT('\'');
		else 
		{
			// Unknown character
			return;
		}

		if (m_overrideText)
			RemoveAt(m_caretPos);

		InsertAt(m_caretPos, String::Printf(TXT("%c"), charToInsert));
		MoveCaretTo(m_caretPos + 1);
	}

	// Refresh IntelliSense
	if ((key >= IK_A && key <= IK_Z) ||
		((key >= IK_0 && key <= IK_9) && !m_shiftPressed) ||
		(key == IK_Period && !m_shiftPressed) ||
		key == IK_Backspace || key == IK_Delete)
	{
		m_refreshIntelliSense = true;
	}
	else if (key != IK_Up && key != IK_Down)
	{
		m_intelliSenseProperties.Clear();
	}
}

void CDebugConsole::PrintText( CRenderFrame *frame, Float x, Float y, const String &text, Color color )
{
	static Matrix textMatrix( Matrix::IDENTITY );
	textMatrix.SetTranslation( x, y, 0.1f );
	new (frame) CRenderFragmentText( frame, textMatrix, *m_font, text.AsChar(), color );
}

void CDebugConsole::OutputToConsole( const CDebugMessage &message, Bool force /*=false*/ )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > threadLock( m_mutex );

	if( m_isAcceptingMessages || force )
	{
		CDebugMessage msg;
		if ( message.GetType() == DMT_Info && message.GetContent().BeginsWith(TXT("Error:")) )
		{
			msg = CDebugMessage( message.GetContent(), DMT_Error );
		}
		else if ( message.GetType() == DMT_Info && message.GetContent().BeginsWith(TXT("Warning:")) )
		{
			msg = CDebugMessage( message.GetContent(), DMT_Warning );
		}
		else
		{
			msg = message;
		}

		const String &text = msg.GetContent();
		TDynArray<String> parts = text.Split(TXT("\n"));
		if (parts.Size() == 0)
		{
			m_outputHistory.PushBack(Move(msg));
		}
		else
		{
			for (Uint32 i = 0; i < parts.Size(); ++i)
			{
				m_outputHistory.PushBack(CDebugMessage(parts[i], msg.GetType()));
			}
		}

		if (m_outputHistory.Size() > m_maxOutputHistoryEntries)
		{
			Uint32 count = m_outputHistory.Size() - m_maxOutputHistoryEntries;
			m_outputHistory.Erase(m_outputHistory.Begin(), m_outputHistory.Begin() + count);
		}
	}
}

void CDebugConsole::RemoveAt(Int32 pos)
{
	if (pos >= 0 && pos < (Int32)m_commandLine.GetLength())
	{
		m_commandLine = m_commandLine.LeftString(pos) + m_commandLine.RightString(m_commandLine.GetLength() - pos - 1);
	}
}

void CDebugConsole::InsertAt(int pos, const String &str)
{
	// Limit commandLine length
	if (m_commandLine.GetLength() >= 256)
	{
		return;
	}

	ASSERT(pos >= 0 && pos <= (Int32)m_commandLine.GetLength());

	if (pos < (Int32)m_commandLine.GetLength())
	{
		m_commandLine = m_commandLine.LeftString(pos) + str + m_commandLine.RightString(m_commandLine.GetLength() - pos);
	}
	else
	{
		m_commandLine += str;
	}
}

void CDebugConsole::MoveCaretTo(Int32 newPos)
{
	m_caretPos = Clamp<Int32>(newPos, 0, (Int32)m_commandLine.GetLength());
	m_caretAnimationTime = 0.0f;
}

void CDebugConsole::Write( const Red::System::Log::Message& consoleMessage )
{
	// Ignore possible spam from other threads when accepting messages, not so much to do with thread safety
	if ( ! ::SIsMainThread() )
	{
		return;
	}

	// Shouldn't even really need to be multithreaded, but leaving this here for now since the log devices can be accessed from any thread
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > threadLock( m_mutex );
		if ( ! m_isAcceptingMessages )
		{
			return;
		}
	}

	if ( consoleMessage.channelHash == CNAME( Warning ).GetSerializationHash() )
	{
		// Log
		OutputToConsole( CDebugMessage( consoleMessage.text, DMT_Warning ) );
	}
	else if ( consoleMessage.channelHash == CNAME( Error ).GetSerializationHash() )
	{
		// Log
		OutputToConsole( CDebugMessage( consoleMessage.text, DMT_Error ) );
	}
	else if ( consoleMessage.channelHash == CNAME( Script ).GetSerializationHash() )
	{
		// Log
		OutputToConsole( CDebugMessage( consoleMessage.text, DMT_Info ) );
	}
	else
	{
		// Log
		OutputToConsole( CDebugMessage( consoleMessage.text, DMT_Info ) );
	}
}
