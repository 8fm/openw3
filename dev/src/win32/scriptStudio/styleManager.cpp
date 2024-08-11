#include "build.h"
#include "styleManager.h"
#include "frame.h"

SBaseStyle::SBaseStyle()
{
	isSet = false;
}

CSSStyleManager* CSSStyleManager::s_instance = NULL;

CSSStyleManager::CSSStyleManager()
{
	m_styles.resize( Style_Max );
}

CSSStyleManager::~CSSStyleManager()
{

}

// #define CONFIG_STYLE_BRACKETHIGHLIGHT CONFIG_OPTIONS_BRACKETHIGHLIGHTING
// #define CONFIG_STYLE_CALLTIP wxT( "callTip" )
// #define CONFIG_STYLE_BREAKPOINTS wxT( "breakpoints" )
// #define CONFIG_STYLE_OUTLINING CONFIG_OPTIONS_OUTLINING

inline const wxChar* CSSStyleManager::GetConfigString( unsigned int style )
{
	RED_ASSERT( style < Style_Max, TXT( "Invalid style index: %u/%u" ), style, Style_Max );

	const wxChar* configs[ Style_Max ] =
	{
		CONFIG_STYLE_DEFAULT,
		CONFIG_STYLE_COMMENT,
		CONFIG_STYLE_NUMBER,
		CONFIG_STYLE_STRING,
		CONFIG_STYLE_CHARACTER,
		CONFIG_STYLE_IDENTIFIER,
		CONFIG_STYLE_OPERATOR,
		CONFIG_STYLE_WORD,
		CONFIG_STYLE_WORD2,
		CONFIG_STYLE_GLOBALCLASS,

		CONFIG_STYLE_HOVERINFO_PREFIX CONFIG_STYLE_DEFAULT,
		CONFIG_STYLE_HOVERINFO_PREFIX CONFIG_STYLE_COMMENT,
		CONFIG_STYLE_HOVERINFO_PREFIX CONFIG_STYLE_NUMBER,
		CONFIG_STYLE_HOVERINFO_PREFIX CONFIG_STYLE_STRING,
		CONFIG_STYLE_HOVERINFO_PREFIX CONFIG_STYLE_CHARACTER,
		CONFIG_STYLE_HOVERINFO_PREFIX CONFIG_STYLE_IDENTIFIER,
		CONFIG_STYLE_HOVERINFO_PREFIX CONFIG_STYLE_OPERATOR,
		CONFIG_STYLE_HOVERINFO_PREFIX CONFIG_STYLE_WORD,
		CONFIG_STYLE_HOVERINFO_PREFIX CONFIG_STYLE_WORD2,
		CONFIG_STYLE_HOVERINFO_PREFIX CONFIG_STYLE_GLOBALCLASS,

		CONFIG_STYLE_BRACKETHIGHLIGHT,
		CONFIG_STYLE_CALLTIP,
		CONFIG_STYLE_LINENUMBERS,

		CONFIG_STYLE_OPCODES
	};

	return configs[ style ];
}

void CSSStyleManager::ResetStyle( EStyle styleId )
{
	SStyle2& style = m_styles[ styleId ];

	style.Unset();

	switch( styleId )
	{
	case Style_Default:
		style.foreground.Set( wxColour( 0, 0, 0 ) );
		style.background.Set( wxColour( 255, 255, 255 ) );
		style.font.Set( wxFont( 10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT( "Courier New" ) ) );
		break;

	case Style_Comment:
		style.foreground.Set( wxColour( 0, 140, 0 ) );
		break;

	case Style_Number:
		style.foreground.Set( wxColour( 140, 0, 70 ) );
		break;

	case Style_String:
		style.foreground.Set( wxColour( 240, 0, 0 ) );
		break;

	case Style_Character:
		style.foreground.Set( wxColour( 240, 120, 0 ) );
		break;

	case Style_Word:
		style.foreground.Set( wxColour( 0, 0, 227 ) );
		style.font.Set( wxFont( 10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT( "Courier New" ) ) );
		break;

	case Style_Word2:
		style.foreground.Set( wxColour( 0, 120, 227 ) );
		style.font.Set( wxFont( 10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT( "Courier New" ) ) );
		break;

	case Style_GlobalClass:
		style.foreground.Set( wxColour( 0, 0, 227 ) );
		style.font.Set( wxFont( 10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT( "Courier New" ) ) );
		break;

	case Style_HoverInfoDefault:
		style.background.Set( wxColour( 223, 233, 235 ) );
		break;

	case Style_BracketHighlight:
		style.background.Set( wxColour( 178, 231, 237 ) );
		break;

	case Style_Calltip:
		style.font.Set( wxFont( 8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT( "Tahoma" ) ) );
		break;

	case Style_Opcodes:
		style.foreground.Set( wxColour( 137, 134, 150 ) );
		style.background.Set( wxColour( 255, 255, 255 ) );
		break;

	}
}

void CSSStyleManager::ReadStyles()
{
	// Set default values
	m_styles[ Style_Default ].foreground.fallback	= Style_None;
	m_styles[ Style_Default ].background.fallback	= Style_None;
	m_styles[ Style_Default ].font.fallback			= Style_None;

	for( unsigned int i = STYLE_START + 1; i < STYLE_END; ++i )
	{
		m_styles[ i ].foreground.fallback	= Style_Default;
		m_styles[ i ].background.fallback	= Style_Default;
		m_styles[ i ].font.fallback			= Style_Default;
	}

	m_styles[ Style_BracketHighlight ].foreground.fallback	= Style_Default;
	m_styles[ Style_BracketHighlight ].background.fallback	= Style_Default;
	m_styles[ Style_BracketHighlight ].font.fallback		= Style_Default;

	m_styles[ Style_Calltip ].foreground.fallback	= Style_Default;
	m_styles[ Style_Calltip ].background.fallback	= Style_Default;
	m_styles[ Style_Calltip ].font.fallback			= Style_Default;

	m_styles[ Style_LineNumbers ].foreground.fallback	= Style_Default;
	m_styles[ Style_LineNumbers ].background.fallback	= Style_Default;
	m_styles[ Style_LineNumbers ].font.fallback			= Style_Default;

	m_styles[ Style_Opcodes ].foreground.fallback	= Style_Default;
	m_styles[ Style_Opcodes ].background.fallback	= Style_Default;
	m_styles[ Style_Opcodes ].font.fallback			= Style_Default;

	// Hoverinfo styles
	for( unsigned int i = STYLE_HOVERINFO_START; i < STYLE_HOVERINFO_END; ++i )
	{
		m_styles[ i ].foreground.fallback	= static_cast< EStyle >( i - STYLE_HOVERINFO_OFFSET );
		m_styles[ i ].background.fallback	= Style_HoverInfoDefault;
		m_styles[ i ].font.fallback			= static_cast< EStyle >( i - STYLE_HOVERINFO_OFFSET );
	}

	// Read values in from configuration file
	for( unsigned int i = 0; i < Style_Max; ++i )
	{
		// Default values
		ResetStyle( static_cast< EStyle >( i ) );

		wxColour colour;
		if( wxTheFrame->ReadStyle2( GetConfigString( i ), CONFIG_STYLE_FOREGROUND, colour ) )
		{
			m_styles[ i ].foreground.Set( colour );
		}

		if( wxTheFrame->ReadStyle2( GetConfigString( i ), CONFIG_STYLE_BACKGROUND, colour ) )
		{
			m_styles[ i ].background.Set( colour );
		}

		wxFont font;
		if( wxTheFrame->ReadStyle2( GetConfigString( i ), CONFIG_STYLE_FONT, font ) )
		{
			m_styles[ i ].font.Set( font );
		}
	}

	ResetCaretColour();
	ResetCaretBlinkRate();
	ResetCaretThickness();
	ResetCaretHighlight();
	ResetCaretHighlightColour();
	ResetWordHighlightColour();

	wxTheFrame->ReadCaretStyle( m_caretColour, m_caretBlinkRate, m_caretThickness, m_caretLineHighlight, m_caretLineHighlightColour, m_wordHighlightColour );
}

void CSSStyleManager::WriteStyles()
{
	for( unsigned int i = 0; i < Style_Max; ++i )
	{
		if( m_styles[ i ].foreground.isSet )
		{
			wxTheFrame->WriteStyle( GetConfigString( i ), CONFIG_STYLE_FOREGROUND, m_styles[ i ].foreground.item );
		}
		else
		{
			wxTheFrame->ClearStyle( GetConfigString( i ), CONFIG_STYLE_FOREGROUND );
		}

		if( m_styles[ i ].background.isSet )
		{
			wxTheFrame->WriteStyle( GetConfigString( i ), CONFIG_STYLE_BACKGROUND, m_styles[ i ].background.item );
		}
		else
		{
			wxTheFrame->ClearStyle( GetConfigString( i ), CONFIG_STYLE_BACKGROUND );
		}

		if( m_styles[ i ].font.isSet )
		{
			wxTheFrame->WriteStyle( GetConfigString( i ), CONFIG_STYLE_FONT, m_styles[ i ].font.item );
		}
		else
		{
			wxTheFrame->ClearStyle( GetConfigString( i ), CONFIG_STYLE_FONT );
		}
	}

	wxTheFrame->WriteCaretStyle( m_caretColour, m_caretBlinkRate, m_caretThickness, m_caretLineHighlight, m_caretLineHighlightColour, m_wordHighlightColour );
}
