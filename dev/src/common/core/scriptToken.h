/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "string.h"
#include "scriptFileContext.h"

/// Parsed file token
class CScriptToken
{
public:
	String				m_text;			//!< Token text
	Int32					m_token;		//!< Token ID
	Int32					m_level;		//!< Code level ( bracket level )
	CScriptFileContext	m_context;		//!< Token context

public: 
	//! Default constructor
	RED_INLINE CScriptToken() 
		: m_token( 0 )
		, m_level( -1 )
	{};

	//! Constructor
	RED_INLINE CScriptToken( const String &text, Int32 id, Int32 level, const CScriptFileContext &context )
		: m_text( text )
		, m_token( id )
		, m_level( level )
		, m_context( context )
	{};

	//! Describe
	RED_INLINE String ToString() const
	{
		return String::Printf( TXT("Token %i \"%ls\", #%i at %ls"), m_token, m_text.AsChar(), m_level, m_context.ToString().AsChar() );
	}
};
