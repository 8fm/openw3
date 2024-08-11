#pragma once

#define CONFIG_STYLE_DEFAULT wxT( "default" )
#define CONFIG_STYLE_COMMENT wxT( "comment" )
#define CONFIG_STYLE_NUMBER wxT( "number" )
#define CONFIG_STYLE_STRING wxT( "string" )
#define CONFIG_STYLE_CHARACTER wxT( "character" )
#define CONFIG_STYLE_WORD wxT( "word" )
#define CONFIG_STYLE_WORD2 wxT( "word2" )
#define CONFIG_STYLE_GLOBALCLASS wxT( "globalClass" )
#define CONFIG_STYLE_IDENTIFIER wxT( "identifier" )
#define CONFIG_STYLE_OPERATOR wxT( "operator" )
#define CONFIG_STYLE_BRACKETHIGHLIGHT CONFIG_OPTIONS_BRACKETHIGHLIGHTING
#define CONFIG_STYLE_CALLTIP wxT( "callTip" )
#define CONFIG_STYLE_LINENUMBERS wxT( "lineNumbers" )
#define CONFIG_STYLE_BREAKPOINTS wxT( "breakpoints" )
#define CONFIG_STYLE_OUTLINING CONFIG_OPTIONS_OUTLINING
#define CONFIG_STYLE_OPCODES wxT( "opcodes" )

#define CONFIG_STYLE_HOVERINFO_PREFIX wxT( "HoverInfo/" )

#define CONFIG_STYLE_BOLD wxT( "bold" )
#define CONFIG_STYLE_FONT wxT( "font" )
#define CONFIG_STYLE_SIZE wxT( "size" )
#define CONFIG_STYLE_FOREGROUND wxT( "fore" )
#define CONFIG_STYLE_BACKGROUND wxT( "back" )

#define STYLE_START Style_Default
#define STYLE_END ( Style_GlobalClass + 1 )

#define STYLE_HOVERINFO_START Style_HoverInfoDefault
#define STYLE_HOVERINFO_END ( Style_HoverInfoGlobalClass + 1 )
#define STYLE_HOVERINFO_OFFSET ( STYLE_HOVERINFO_END - STYLE_HOVERINFO_START )

enum EStyle
{
	Style_None = -1,

	Style_Default = 0,
	Style_Comment,
	Style_Number,
	Style_String,
	Style_Character,
	Style_Identifier,
	Style_Operator,
	Style_Word,
	Style_Word2,
	Style_GlobalClass,

	Style_HoverInfoDefault,
	Style_HoverInfoComment,
	Style_HoverInfoNumber,
	Style_HoverInfoString,
	Style_HoverInfoCharacter,
	Style_HoverInfoIdentifier,
	Style_HoverInfoOperator,
	Style_HoverInfoWord,
	Style_HoverInfoWord2,
	Style_HoverInfoGlobalClass,

	Style_BracketHighlight,
	Style_Calltip,
	Style_LineNumbers,
	Style_Opcodes,

	Style_Max
};

enum ESubStyle
{
	SubStyle_All = -1,

	SubStyle_Foreground = 0,
	SubStyle_Background,
	SubStyle_Font,

	SubStyle_Max
};

struct SBaseStyle
{
	bool isSet;

	SBaseStyle();
};

template< typename TStyleItem >
struct SStyleItem : public SBaseStyle
{
	TStyleItem item;
	EStyle fallback;

	void Set( const TStyleItem& newItem )
	{
		item = newItem;
		isSet = true;
	}
};

struct SStyle2
{
	SStyleItem< wxColour > foreground;
	SStyleItem< wxColour > background;
	SStyleItem< wxFont > font;

	inline void Unset()
	{
		foreground.isSet = false;
		background.isSet = false;
		font.isSet = false;
	}
};

class CSSStyleManager
{
public:
	CSSStyleManager();
	~CSSStyleManager();

	void ResetStyle( EStyle styleId );

	void ReadStyles();
	void WriteStyles();

	template < typename TStyleItem, SStyleItem< TStyleItem > SStyle2::* MemPtr >
	inline const TStyleItem& GetStyleItem( EStyle styleType ) const
	{
		const SStyle2& style = m_styles[ styleType ];
		const SStyleItem< TStyleItem >& styleItem = style.*MemPtr;

		if( styleItem.isSet )
		{
			return styleItem.item;
		}
		else
		{
			return GetStyleItem< TStyleItem, MemPtr >( styleItem.fallback );
		}
	}

	inline const wxColour& GetForegroundColour( EStyle style ) const
	{
		return GetStyleItem< wxColour, &SStyle2::foreground >( style );
	}

	inline const wxColour& GetBackgroundColour( EStyle style ) const
	{
		return GetStyleItem< wxColour, &SStyle2::background >( style );
	}

	inline const wxColour& GetColour( EStyle style, ESubStyle subStyle ) const
	{
		switch( subStyle )
		{
		case SubStyle_Foreground:
			return GetForegroundColour( style );

		case SubStyle_Background:
			return GetBackgroundColour( style );
		}

		RED_HALT( "Invalid Substyle specified: Not a colour (%i)", subStyle )
		return *wxBLACK;
	}

	inline void SetColour( EStyle styleType, ESubStyle subStyle, const wxColour& colour )
	{
		SStyleItem< wxColour > SStyle2::* memPtr = NULL;

		switch( subStyle )
		{
		case SubStyle_Foreground:
			memPtr = &SStyle2::foreground;
			break;

		case SubStyle_Background:
			memPtr = &SStyle2::background;
			break;
		}

		RED_ASSERT( memPtr, TXT( "Invalid Substyle specified: Not a colour (%i)" ), subStyle );

		SStyle2& style = m_styles[ styleType ];
		SStyleItem< wxColour >& styleItem = style.*memPtr;

		styleItem.Set( colour );
	}

	inline const wxFont& GetFont( EStyle style )
	{
		return GetStyleItem< wxFont, &SStyle2::font >( style );
	}

	inline void SetFont( EStyle style, const wxFont& font )
	{
		m_styles[ style ].font.Set( font );
	}

	inline void SetCaretColour( const wxColour& colour ) { m_caretColour = colour; }
	inline void SetCaretBlinkRate( int rate ) { m_caretBlinkRate = rate; }
	inline void SetCaretThickness( int thickness ) { m_caretThickness = thickness; }
	inline void SetCaretHighlight( bool enabled ) { m_caretLineHighlight = enabled; }
	inline void SetCaretHighlightColour( const wxColour& colour ) { m_caretLineHighlightColour = colour; }
	inline void SetWordHighlightColour( const wxColour& colour ) { m_wordHighlightColour = colour; }

	inline const wxColour& GetCaretColour() const { return m_caretColour; }
	inline int GetCaretBlinkRate() const { return m_caretBlinkRate; }
	inline int GetCaretThickness() const { return m_caretThickness; }
	inline bool GetCaretHighlight() const { return m_caretLineHighlight; }
	inline const wxColour& GetCaretHighlightColour() const { return m_caretLineHighlightColour; }
	inline const wxColour& GetWordHighlightColour() const { return m_wordHighlightColour; }

	inline void ResetCaretColour() { m_caretColour = *wxBLACK; }
	inline void ResetCaretBlinkRate() { m_caretBlinkRate = 500; }
	inline void ResetCaretThickness() { m_caretThickness = 1; }
	inline void ResetCaretHighlight() { m_caretLineHighlight = false; }
	inline void ResetCaretHighlightColour() { m_caretLineHighlightColour = *wxGREEN; }
	inline void ResetWordHighlightColour() { m_wordHighlightColour = *wxLIGHT_GREY; }

	static CSSStyleManager& GetInstance()
	{
		if( !s_instance )
		{
			s_instance = new CSSStyleManager();
		}

		return *s_instance;
	}

private:
	const wxChar* GetConfigString( unsigned int style );
	vector< SStyle2 > m_styles;

	wxColour m_caretColour;
	int m_caretBlinkRate;
	int m_caretThickness;
	bool m_caretLineHighlight;
	wxColour m_caretLineHighlightColour;
	
	wxColour m_wordHighlightColour;

	static CSSStyleManager* s_instance;
};
