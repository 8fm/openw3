/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "scriptToken.h"
#include "dynarray.h"

/// Stream of tokens
class CScriptTokenStream
{
public:
	typedef TDynArray< CScriptToken, MC_ScriptCompilation, MemoryPool_ScriptCompilation > Tokens;

private:

	Tokens	m_tokens;			//!< Tokens
	Uint32	m_readPos;			//!< Read position

public:
	//! Get raw tokens
	RED_INLINE const Tokens& GetTokens() const { return m_tokens; }

	//! Get raw tokens
	RED_INLINE Tokens& GetTokens() { return m_tokens; }

	//! End of stream
	RED_INLINE Bool IsEndOfStream() const { return m_readPos >= m_tokens.Size(); }

	//! Empty token stream
	RED_INLINE Bool IsEmpty() const { return m_tokens.Empty(); }

public:
	//! Constructor 
	CScriptTokenStream();

	//! Push token (at the end of the stream)
	RED_INLINE void PushToken( const String& text, Int32 id, Int32 level, const CScriptFileContext& context );
	RED_INLINE void PushToken( const CScriptToken& token);

	//! Pop token 
	Bool PopToken( CScriptToken& token );

	RED_INLINE CScriptToken& GetReadToken();
	RED_INLINE const CScriptToken& GetReadToken() const;

	RED_INLINE void IncrementReadPosition();

public:
	Bool ExtractFunctionTokens( CScriptTokenStream &stream );
	Bool ExtractInitCode( CScriptTokenStream &stream );
};

RED_INLINE void CScriptTokenStream::PushToken( const String& text, Int32 id, Int32 level, const CScriptFileContext& context )
{
	m_tokens.PushBack( std::move( CScriptToken( text, id, level, context ) ) );
}

RED_INLINE void CScriptTokenStream::PushToken( const CScriptToken& token )
{
	m_tokens.PushBack( token );
}

RED_INLINE CScriptToken& CScriptTokenStream::GetReadToken()
{
	RED_FATAL_ASSERT( !IsEmpty(), "No more tokens to read" );
	RED_FATAL_ASSERT( !IsEndOfStream(), "No more tokens to read" );

	return m_tokens[ m_readPos ];
}

RED_INLINE const CScriptToken& CScriptTokenStream::GetReadToken() const
{
	RED_FATAL_ASSERT( !IsEmpty(), "No more tokens to read" );
	RED_FATAL_ASSERT( !IsEndOfStream(), "No more tokens to read" );

	return m_tokens[ m_readPos ];
}

RED_INLINE void CScriptTokenStream::IncrementReadPosition()
{
	++m_readPos;
}
