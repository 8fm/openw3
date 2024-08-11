#pragma once

#include "debugMessage.h"

#include "../redThreads/redThreadsThread.h"

enum EInputKey : Int32;
enum EInputAction : Int32;

class CRenderFrame;
class IViewport;

/// Debug console
class CDebugConsole : public Red::System::Log::OutputDevice
{
public:
	CDebugConsole( Bool isEnabled = true );
	~CDebugConsole();

	// Update internal state
	void Tick( float timeDelta );

	//! Generate console rendering
	void OnGenerateFragments( CRenderFrame *frame );

	//! Handle input from the user
	Bool OnViewportInput( IViewport* view, EInputKey key,  EInputAction action, Float data );

	//! Is console visible ?
	Bool IsVisible() const { return m_visible; }

	void SetAcceptMessages( Bool value );

private:
	static const Float		m_consoleAnimationDuration;
	static const Float		m_caretAnimationDuration;
	static const Float		m_keyPressNormalRepeatTime;
	static const Float		m_keyPressQuickRepeatTime;
	static const Color		m_consoleColor;
	static const Color		m_popupColor;
	static const Color		m_selectedPopupColor;
	static const Color		m_caretColor;
	static const Color		m_infoColor;
	static const Color		m_warningColor;
	static const Color		m_errorColor;
	static const Color		m_commandColor;
	static const Uint32		m_maxCommandLineHistoryEntries;
	static const Uint32		m_maxOutputHistoryEntries;

private:
	TDynArray< String >				m_commandHistory;
	TDynArray< CDebugMessage >		m_outputHistory;
	TDynArray< CProperty* >			m_intelliSenseProperties;
	CFont*							m_font;
	String							m_commandLine;
	enum EInputKey					m_lastPressedKey;
	Int32							m_commandHistoryIndex;
	Int32							m_caretPos;
	Int32							m_selectedPropertyIndex;
	Float							m_consoleAnimationTime;
	Float							m_caretAnimationTime;
	Float							m_keyPressTime;
	Float							m_popupWidth;
	Float							m_popupHeight;
	Bool 							m_overrideText;
	Bool 							m_visible;
	Bool 							m_shiftPressed;
	Bool 							m_ctrlPressed;
	Bool 							m_quickRepeatTime;
	Bool 							m_refreshIntelliSense;

	Bool							m_isEnabled;
	Bool							m_isAcceptingMessages;

	Red::Threads::CMutex			m_mutex;

private:
	// Internal methods
	void OnKeyDown( enum EInputKey key );
	void OnKeyUp( enum EInputKey key ) {}
	void PrintText( CRenderFrame *frame, Float x, Float y, const String &text, Color color );
	void OutputToConsole( const CDebugMessage &text, Bool force = false );
	void RemoveAt( Int32 pos );
	void InsertAt( Int32 pos, const String &str );
	void MoveCaretTo( Int32 newPos );

private:
	virtual void Write( const Red::System::Log::Message& message );
};

extern CDebugConsole* GDebugConsole;
