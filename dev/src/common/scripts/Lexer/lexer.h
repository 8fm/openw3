/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef _RED_SCRIPTS_LEXER_H__
#define _RED_SCRIPTS_LEXER_H__

#include "../../redSystem/types.h"
#include "../../redSystem/utility.h"

#include "definition.h"

namespace Red
{
	namespace Scripts
	{
		using namespace System;

		class Lexer : public NonCopyable
		{
		public:
			Lexer( const LexerDefinition& definition );
			virtual ~Lexer();

			void Initialize( AllocatorProxy* allocProxy );

			Bool Tokenize( const Char* code );

			// Public interface
		public:
			enum EBracketDirection
			{
				BracketDir_Open = 0,
				BracketDir_Close,

				BracketDir_Max
			};

			enum EBracketType
			{
				BracketType_Brace = 0,
				BracketType_Parenthesis,
				BracketType_Square,

				BracketType_Max
			};

			struct Bracket
			{
				Uint32				m_line;			// Located on this line
				Uint32				m_position;		// Located at this absolute position (in terms of number of characters)
				EBracketDirection	m_dir;			// Open/Push or Close/Pop
				EBracketType		m_type;			// Brace, Parenthesis or Square Bracket
			};

			struct Comment
			{
				const Char*	m_text;
				Uint32	m_startLine;
				Uint32	m_endLine;
				Uint32	m_startPosition;
				Uint32	m_endPosition;
				Uint32	m_firstSucceedingNonCommentLine;
			};

			static const Char BRACKETS[ BracketDir_Max ][ BracketType_Max ];

		public:
			virtual void OnEmitToken( BisonId type, const Char* token ) = 0;
			virtual void OnEmitError( const Char* error );
			virtual void OnEmitBracket( const Bracket& open, const Bracket& close );
			virtual void OnEmitComment( const Comment& comment );

		private:
			void EmitError( const Char* error, ... );
			void EmitToken( BisonId type, const Char* token );
			void EmitBracket( EBracketDirection direction, EBracketType type, Uint32 position );
			void EmitComment( const Comment& comment );

		private:

			void AppendCharToToken( Char*& token, Uint32& tokenBufferSize, Char lex ) const;

			//! Moves the current processing position to the end of the current EOL sequence
			void ConsumeEOL( const Char* code, Uint32 codeLength, Int32& index );

			//! Is on a character that marks the end of the current line
			RED_INLINE Bool IsEOL( Char c ) const
			{
				return ( c == TXT( '\n' ) || c == TXT( '\r' ) ); 
			}

			//! Does this character mark the end of the file?
			RED_INLINE Bool IsEOF( Char c ) const
			{
				// End of file, end parsing
				if ( c == TXT( '\0' ) )
				{
					return true;
				}

				return false; 
			}

			//! Can this character be interpreted as a number?
			Bool IsNum( Char c ) const
			{
				return  (c >= TXT( '0' ) ) && ( c <= TXT( '9' ) ); 
			}

			//! Can this character be interpreted as a hexadecimal number?
			Bool IsHex( Char c ) const
			{
				return
				(
					( c >= TXT( '0' ) ) &&
					( c <= TXT( '9' ) )
				) ||
				(
					( c >= TXT( 'A' ) ) &&
					( c <= TXT( 'F' ) )
				) ||
				(
					( c >= TXT( 'a' ) ) &&
					( c <= TXT( 'f' ) )
				); 
			}

			//! Is this character a valid letter?
			Bool IsAlpha( Char c ) const
			{
				return
				(
					( c >= TXT( 'A' ) ) &&
					( c <= TXT( 'Z' ) )
				) ||
				(
					( c >= TXT( 'a' ) ) &&
					( c <= TXT( 'z' ) )
				);
			}

			//! Is this character considered whitespace?
			Bool IsWhitespace( Char c ) const
			{
				// Anything less than the ascii value "32" is considered whitespace
				return c <= TXT( ' ' );
			}

			//! Is this character important? ( should it break keyword or number parsing? )
			Bool IsBreakChar( Char c ) const
			{
				// Make sure we break before important chars
				for ( Uint32 i = 0; i < m_definition.GetChars().GetSize(); ++i )
				{
					if ( m_definition.GetChars()[ i ] == c )
					{
						return true;
					}
				}

				// Not found
				return false;   
			}

			//! Is this stream of characters a valid integer?
			Bool IsInteger( const Char* start, Int32 count ) const;

			//! Is this stream of characters a valid floating point number?
			Bool IsFloat( const Char* start, Int32 count ) const;

			//! Does this stream of characters match a predefined keyword?
			Bool IsKeyword( const Char* start, Int32 count ) const;

		private:
			enum EState
			{
				State_Processing = 0,
				State_SingleLineComment,
				State_MultilineComment,
				State_String,
				State_Name,
				State_MoreDataNeeded,

				State_Max
			};

			enum ETokenType
			{
				TokenType_Unknown = -1,
				TokenType_Default = 0,
				TokenType_Char,
				TokenType_Integer,
				TokenType_Float,
				TokenType_Keyword,

				TokenType_Max
			};

		private:
			static const Uint32				DEFAULT_TOKEN_BUFFER_SIZE = 128u;

		private:
			const LexerDefinition&			m_definition;
			AllocatorProxy*					m_allocProxy;

		protected:
			Uint32							m_line;
			Uint32							m_level;		// Current scope

		private:
			SimpleDynArray< Bracket >		m_brackets;
			Bool							m_hasError;
		};
	}
}

#endif // _RED_SCRIPTS_LEXER_H__
