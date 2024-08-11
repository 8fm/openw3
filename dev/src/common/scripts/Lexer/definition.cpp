/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "definition.h"

#define INITIAL_NUMBER_OF_KEYWORDS	64
#define INITIAL_NUMBER_OF_CHARS		32

namespace Red
{
	namespace Scripts
	{
		LexerDefinition::LexerDefinition()
		{

		}

		LexerDefinition::~LexerDefinition()
		{

		}

		void LexerDefinition::Initialize( AllocatorProxy* allocProxy )
		{
			m_keywords.Initialize( allocProxy, INITIAL_NUMBER_OF_KEYWORDS );
			m_chars.Initialize( allocProxy, INITIAL_NUMBER_OF_CHARS );
		}

		void LexerDefinition::AddKeyword( const Char* keyword, BisonId id )
		{
			Keyword newKeyword;
			newKeyword.m_keyword = keyword;
			newKeyword.m_id = id;

			m_keywords.PushBack( newKeyword );
		}

		void LexerDefinition::AddChar( Char ch )
		{
			m_chars.PushBack( ch );
		}

		void LexerDefinition::AddLiteral( Literal type, BisonId id )
		{
			m_literals[ type ] = id;
		}
	}
}