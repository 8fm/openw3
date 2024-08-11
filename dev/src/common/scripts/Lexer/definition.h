/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __RED_SCRIPTS_LEXER_DEFINITION_H__
#define __RED_SCRIPTS_LEXER_DEFINITION_H__

#include "../Memory/simpleDynArray.h"

// A Map of valid script tokens and their Bison representations

namespace Red
{
	namespace Scripts
	{
		typedef System::Int32 BisonId;
		typedef System::Char Char;

		struct Keyword
		{
			const System::Char*	m_keyword;
			BisonId				m_id;
		};

		enum Literal
		{
			Literal_String = 0,
			Literal_Name,
			Literal_Integer,
			Literal_Float,
			Literal_Identifier,

			Literal_Max
		};

		class LexerDefinition
		{
		public:
			LexerDefinition();
			~LexerDefinition();
			void Initialize( AllocatorProxy* allocProxy );

			void AddKeyword( const Char* keyword, BisonId id );
			void AddChar( Char ch );
			void AddLiteral( Literal type, BisonId id );

			RED_INLINE const SimpleDynArray< Keyword >& GetKeywords() const { return m_keywords; }
			RED_INLINE const SimpleDynArray< Char >& GetChars() const { return m_chars; }
			RED_INLINE const BisonId& GetLiteral( Literal index ) const { return m_literals[ index ]; }

		private:
			SimpleDynArray< Keyword > m_keywords;
			SimpleDynArray< Char > m_chars;
			BisonId m_literals[ Literal_Max ];
		};
	}
}

#endif // __RED_SCRIPTS_LEXER_DEFINITION_H__
