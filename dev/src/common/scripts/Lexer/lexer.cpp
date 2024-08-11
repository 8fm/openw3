/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "lexer.h"

#if defined(RED_PLATFORM_WINPC) || defined(RED_PLATFORM_DURANGO)
	#include <malloc.h>
#else
	#include <stdlib.h>
#endif

#include "../../redSystem/log.h"

namespace Red
{
	namespace Scripts
	{
		using namespace System;

		const Char Lexer::BRACKETS[ BracketDir_Max ][ BracketType_Max ] =
		{
			{ TXT( '{' ), TXT( '(' ), TXT( '[' ) },
			{ TXT( '}' ), TXT( ')' ), TXT( ']' ) }
		};

		Lexer::Lexer( const LexerDefinition& definition )
		:	m_definition( definition )
		,	m_allocProxy( nullptr )
		,	m_line( 0 )
		,	m_level( 0 )
		,	m_hasError( false )
		{

		}

		Lexer::~Lexer()
		{

		}

		void Lexer::Initialize( AllocatorProxy* allocProxy )
		{
			RED_ASSERT( allocProxy != nullptr, TXT( "Allocator required in order to lex scripts" ) );

			m_allocProxy = allocProxy;

			m_brackets.Initialize( allocProxy );
		}

		RED_INLINE void Lexer::AppendCharToToken( Char*& token, Uint32& tokenBufferSize, Char c ) const
		{
			if( System::StringLength( token, tokenBufferSize ) >= tokenBufferSize - 1 )
			{
				tokenBufferSize *= 2;
				token = static_cast< Char* >( m_allocProxy->Realloc( token, tokenBufferSize * sizeof( Char ) ) );
				RED_ASSERT( token != nullptr, TXT( "Failed to increase token buffer size" ) );
			}

			RED_VERIFY( StringConcatenate( token, &c, tokenBufferSize, 1 ) );
		}

		Bool Lexer::Tokenize( const Char* code )
		{
			EState state = State_Processing;
			Int32 tempLine = 0;
			Bool backoff = false;
			Char skipChar = 0;
			ETokenType bestTokenType = TokenType_Default;
			Int32 bestTokenStart = 0;
			Bool retVal = false;

			Uint32 tokenBufferSize = DEFAULT_TOKEN_BUFFER_SIZE;
			Char* token = static_cast< Char* >( m_allocProxy->Realloc( nullptr, tokenBufferSize * sizeof( Char ) ) );
			MemoryZero( token, sizeof( Char ) * tokenBufferSize );

			Comment comment;
			MemoryZero( &comment, sizeof( Comment ) );

			Bool gotComment = false;

			// Initialize
			m_level = 0;
			m_line = 1;
			m_hasError = false;

			Int32 rememberedPosition = 0;

			// Since Scintilla equates positions with bytes, not chars, we need to keep
			// the char position in sync with the byte position by keeping count of all
			// additional space (over 1 byte) used by each char
			Int32 multibyteOffset = 0;

			if( code )
			{
				Int32 codeLength = static_cast< Int32 >( StringLength( code ) );

				if( codeLength > 0 )
				{
					for ( Int32 i = 0; i <= codeLength; ++i )
					{
						// Go back one char
						if ( backoff )
						{
							--i;
							backoff = false;	 
							RED_ASSERT( i < codeLength, TXT( "Shouldn't be able to backoff the beginning of the code" ) );
						}

						// Get char
						Char c = code[ i ];

						// Skip char mode
						if ( skipChar )
						{
							// Whitespace char?
							if ( ( c == skipChar ) || IsWhitespace( c ) )
							{
								skipChar = 0;
								state = State_Processing;	  

								// Don't allow line feed when looking for skip char
								if ( IsEOL( c ) )
								{
									EmitError( TXT( "Unexpected new line" ) );
									continue;
								}

								// Not whitespace, keep looking
								if ( !IsWhitespace( c ) )
								{
									continue;	  
								}
							}
						}

						// Parsing state machine
						switch ( state )
						{
							// Base state 
						case State_Processing:
							{
								// Reset token
								MemoryZero( token, sizeof( Char ) * tokenBufferSize );

								// Whitespace char, skip it
								if ( IsWhitespace( c ) )
								{
									break;
								}

								// Comment ?
								if ( c == TXT( '/' ) && code[ i + 1 ] == TXT( '/' ) )
								{
									state = State_SingleLineComment;

									if( !gotComment )
									{
										gotComment = true;
										comment.m_startLine = m_line;
										comment.m_startPosition = i;
									}

									++i;
								}
								else if( c == TXT( '/' ) && code[ i + 1 ] == TXT( '*' ) )
								{
									tempLine = m_line;
									state = State_MultilineComment;

									if( !gotComment )
									{
										gotComment = true;
										comment.m_startLine = m_line;
										comment.m_startPosition = i;
									}

									++i;
								}

								// String
								else if ( c == TXT( '\"' ) )
								{
									tempLine = m_line;
									state = State_String;
								}

								// Name
								else if ( c == TXT( '\'' ) )
								{
									tempLine = m_line;
									state = State_Name;
								}

								else
								{
									// Hmm, it's something bigger to parse :)
									state = State_MoreDataNeeded;
									backoff = true;
								}

								if( gotComment && !( state == State_SingleLineComment || state == State_MultilineComment ) )
								{
									// Allocate some stack memory to temporarily store comment text
									Uint32 length = comment.m_endPosition - comment.m_startPosition;
									Uint32 size = length + 1;
									void* buffer = alloca( size * sizeof( Char ) );
									Char* text = static_cast< Char* >( buffer );

									// Copy the comment into the string buffer
									RED_VERIFY( StringCopy( text, &code[ comment.m_startPosition ], size, length ) );

									// Fill out the remaining
									comment.m_firstSucceedingNonCommentLine = m_line;
									comment.m_text = text;

									// Notify listener
									EmitComment( comment );

									// Reset
									MemoryZero( &comment, sizeof( Comment ) );
									gotComment = false;
								}
							}

							break;

							// Single line comment
						case State_SingleLineComment:
							{
								// Line feed?
								if ( IsEOL( c ) )
								{
									if( gotComment )
									{
										comment.m_endLine = m_line;
										comment.m_endPosition = i + multibyteOffset;
									}

									state = State_Processing;
								}

								break;
							}

							// Multiline comment
						case State_MultilineComment:
							{
								// End?
								if ( ( c == TXT( '*' ) ) && ( code[ i + 1 ] == TXT( '/' ) ) )
								{
									if( gotComment )
									{
										comment.m_endLine = m_line;
										// + 1 for the '/' terminating character
										comment.m_endPosition = i + 1 + multibyteOffset;
									}

									state = State_Processing;
									++i;
								}

								break;
							}

							// String
						case State_String:
							{
								//Folded String ? 
								//To cover simple cases
								if( c == TXT( '\\' ) )
								{
									//Do we have left and right char ? 
									if( i - 1 >= 0 && i + 1 <= codeLength )
									{
										//Is the right char " and left is not
										if( code[ i + 1 ] == TXT( '"' ) && code[ i - 1 ] != TXT( '\\' ) )
										{
											//Add it to the string token, and go to the next folded string
											AppendCharToToken( token, tokenBufferSize, code[ i + 1 ] );
											++i;
											break;
										}
									}
								}

								if ( c == TXT( '\"' ) )
								{
									if ( m_definition.GetLiteral( Literal_String ) == 0 )
									{
										EmitError( TXT( "Unexpected string literal: %" ) RED_PRIWs, token );
									}
									else
									{
										EmitToken( m_definition.GetLiteral( Literal_String ), token );
									}

									MemoryZero( token, sizeof( Char ) * tokenBufferSize );
									state = State_Processing;
									break;
								}

								// Line feed ?
								if ( IsEOL( c ) )
								{
									EmitError( TXT("New line in constant") );
									state = State_Processing; 
									MemoryZero( token, sizeof( Char ) * tokenBufferSize );
									break;
								}

								// Add char
								AppendCharToToken( token, tokenBufferSize, c );
								break;
							}

							// Name
						case State_Name:
							{
								// End ?
								if ( c == TXT( '\'' ) )
								{
									if ( m_definition.GetLiteral( Literal_Name ) == 0 )
									{
										EmitError( TXT( "Unexpected name literal" ) );
									}
									else
									{
										EmitToken( m_definition.GetLiteral( Literal_Name ), token );
									}
									MemoryZero( token, sizeof( Char ) * tokenBufferSize );
									state = State_Processing;
									break;
								}

								// Line feed ?
								if ( IsEOL( c ) )
								{
									EmitError( TXT("New line in constant") );
									state = State_Processing;
									MemoryZero( token, sizeof( Char ) * tokenBufferSize );
									break;
								}

								// Not a keyword char ?
								if ( !IsNum(c) && !IsAlpha(c) && (c != TXT( '.' )) && (c != TXT( '_' )) && (c != TXT( ' ' )) && (c != TXT( '*' )))
								{
									EmitError( TXT("Name identifier can contain only digits and letters") );
									skipChar = TXT( '\'' );
									state = State_Processing;
									MemoryZero( token, sizeof( Char ) * tokenBufferSize );
									break;
								}

								// Add char
								AppendCharToToken( token, tokenBufferSize, c );
								break;
							}	 

						case State_MoreDataNeeded:
							{
								// Initialize best token buf if
								if ( StringLength( token ) == 0 )
								{
									bestTokenType = TokenType_Unknown;
									bestTokenStart = i;
								}

								// Can be a char
								ETokenType parsedTokenType = TokenType_Unknown;
								if ( IsBreakChar( code[ bestTokenStart ] ) && ( i == bestTokenStart ) )
								{
									parsedTokenType = TokenType_Char;
								}

								// Can be a number ?
								if ( IsInteger( &code[ bestTokenStart ], ( i - bestTokenStart ) + 1 ) )
								{
									parsedTokenType = TokenType_Integer;
								}

								// Can be a float number ?
								if ( IsFloat( &code[ bestTokenStart ], ( i - bestTokenStart ) + 1 ) )
								{
									parsedTokenType = TokenType_Float;
								}

								// Can be a keyword ?
								if ( IsKeyword( &code[ bestTokenStart ], ( i - bestTokenStart ) + 1 ) )
								{
									parsedTokenType = TokenType_Keyword;
								}

								// Well we got a whitespace char or nothing was matched
								if ( IsWhitespace( c ) || ( parsedTokenType == TokenType_Unknown ) )
								{
									// If we don't have best match it means that there's an error
									if ( bestTokenType == TokenType_Unknown )
									{
										EmitError( TXT( "Unexpected \'%c\'" ), c );
									}
									else
									{
										// Emit parsed token
										if ( bestTokenType == TokenType_Char )
										{
											// Get char to emit
											Char c = token[ 0 ];
											RED_ASSERT( c != TXT( '\0' ) );

											bool foundBracket = false;
											for( Uint32 iBracketDirection = 0; iBracketDirection < BracketDir_Max; ++iBracketDirection )
											{
												for( Uint32 iBracketType = 0; iBracketType < BracketType_Max; ++iBracketType )
												{
													if( c == BRACKETS[ iBracketDirection ][ iBracketType ] )
													{
														EmitBracket( static_cast< EBracketDirection >( iBracketDirection ), static_cast< EBracketType >( iBracketType ), static_cast< Uint32 >( rememberedPosition + multibyteOffset ) );
														foundBracket = true;
													}
												}

												if( foundBracket )
												{
													break;
												}
											}

											// Add token
											EmitToken( c, token );
										}
										else if ( bestTokenType == TokenType_Integer )
										{
											// Add integer token
											if ( m_definition.GetLiteral( Literal_Integer ) == 0 )
											{
												EmitError( TXT( "Unexpected integer constant" ) );
											}
											else if ( StringCompare( token, TXT( "0x" ) ) == 0 )
											{
												EmitError( TXT( "Unfinished integer hex constant" ) );
											}
											else
											{
												EmitToken( m_definition.GetLiteral( Literal_Integer ), token );
											}
										}
										else if ( bestTokenType == TokenType_Float )
										{
											// Add floating point token
											if ( m_definition.GetLiteral( Literal_Float ) == 0 )
											{
												EmitError( TXT("Unexpected float constant") );
											}
											else
											{
												EmitToken( m_definition.GetLiteral( Literal_Float ), token );
											}
										}
										else if ( bestTokenType == TokenType_Keyword )
										{
											// Known keyword ?
											Bool found = false;
											for ( Uint32 iKeyword = 0; iKeyword < m_definition.GetKeywords().GetSize(); ++iKeyword )
											{
												if ( StringCompare( m_definition.GetKeywords()[ iKeyword ].m_keyword, token ) == 0 )
												{
													EmitToken(  m_definition.GetKeywords()[ iKeyword ].m_id, token );
													found = true;
													break;
												}
											}

											// Not a keyword, add as general identifier
											if ( !found )
											{
												if ( m_definition.GetLiteral( Literal_Identifier ) == 0 )
												{
													EmitError( TXT( "Unexpected identifier: %" ) RED_PRIWs, token );
												}
												else
												{
													EmitToken( m_definition.GetLiteral( Literal_Identifier ), token );   
												}
											}
										}
										else
										{
											// Should not happen
											RED_HALT( "Unreachable" );
										}

										// Reset token
										MemoryZero( token, sizeof( Char ) * tokenBufferSize );

										// Unparse last char if it was not a whitespace
										if ( !IsWhitespace( c ) )
										{
											backoff = true;
										}
									}

									// Go back to the default state
									state = State_Processing;
								}
								else
								{	 
									// Add char to token
									AppendCharToToken( token, tokenBufferSize, c );

									// Remember best so far
									bestTokenType = parsedTokenType;
								}

								break;
							}

						default:
							{
								RED_HALT( "Invalid lexer state" );
								break;
							}
						}

						Int32 lineSave = m_line;

						if( IsEOL( c ) )
						{
							// Skip to the last character of the EOL sequence
							ConsumeEOL( code, codeLength, i );

							++m_line;
						}

						if ( IsEOF( c ) )
						{
							Int32 saveLine = m_line;

							// In multi line comment ?
							if ( state == State_MultilineComment )
							{
								EmitError( TXT("Unexpected end of file before comment at line '%i' was closed"), tempLine );
							}

							// In string
							else if ( state == State_String )
							{
								EmitError( TXT("Unexpected end of file before string at line '%i' was closed"), tempLine );
							}

							// In name
							else if ( state == State_Name )
							{
								EmitError( TXT("Unexpected end of file before name at line '%i' was closed"), tempLine );
							}

							// Unmatched brackets ?
							if ( m_brackets.GetSize() )
							{
								const Bracket& lastBracket = m_brackets[ m_brackets.GetSize() - 1 ];
								EmitError
								(
									TXT( "Unexpected end of file found after '%c' at line %i" ),
									BRACKETS[ lastBracket.m_dir ][ lastBracket.m_type ],
									lastBracket.m_line
								);
							}

							// Restore line numbering
							m_line = saveLine;
							break;
						}

						// Restore line numbering when backing off
						if ( backoff )
						{
							m_line = lineSave;
							rememberedPosition = i;
						}

						// This is a very naive solution that assumes that code can only ever be 1 or 2 bytes
						else if( c > 127 )
						{
							++multibyteOffset;
						}
					}
				}

				// Return parsing state, if there were errors it failed
				retVal = m_hasError == false;
			}

			m_allocProxy->Free( token );

			return retVal;
		}

		void Lexer::EmitToken( BisonId type, const Char* token )
		{
			OnEmitToken( type, token );
		}

		void Lexer::OnEmitError( const Char* )
		{

		}

		void Lexer::EmitError( const Char* error, ... )
		{
			// Mark as having an error
			m_hasError = true;

			// Format error
			Char formattedBuf[ 1024 ];
			va_list arglist;
			va_start( arglist, error );
			Red::System::VSNPrintF( formattedBuf,  ARRAY_COUNT( formattedBuf ), error, arglist ); 

			OnEmitError( formattedBuf );
		}

		void Lexer::OnEmitBracket( const Bracket&, const Bracket& )
		{

		}

		void Lexer::EmitBracket( EBracketDirection direction, EBracketType type, Uint32 position )
		{
			if ( direction == BracketDir_Open )
			{
				// Add new bracket
				Bracket bracket;
				bracket.m_line = m_line;
				bracket.m_position = position;
				bracket.m_dir = direction;
				bracket.m_type = type;
				m_brackets.PushBack( bracket );

				// Code scope bracket ?
				if ( type == BracketType_Brace )
				{
					++m_level;
				}
			}
			else
			{
				// Out of scope ?
				if ( !m_brackets.GetSize() )
				{
					EmitError( TXT( "Found unexpected '%c'" ), BRACKETS[ direction ][ type ] );
					return;
				}

				// Code scope
				if ( type == BracketType_Brace )
				{
					--m_level;
				}

				// Check if this bracket is the matching closing bracket
				const Bracket& lastBracket = m_brackets[ m_brackets.GetSize() - 1 ];
				if ( type != lastBracket.m_type )
				{
					// Throw error
					EmitError
					(
						TXT( "Unexpected bracket '%c', expected '%c' to match '%c' on line %i" ),
						BRACKETS[ direction ][ type ],
						BRACKETS[ BracketDir_Close ][ lastBracket.m_type ],
						BRACKETS[ BracketDir_Open ][ lastBracket.m_type ],
						lastBracket.m_line
					);

					return;
				}
				else
				{
					// All ok, pop bracket
					Bracket close;
					close.m_line = m_line;
					close.m_position = position;
					close.m_dir = direction;
					close.m_type = type;

					OnEmitBracket( lastBracket, close );

					m_brackets.PopBack();
				}
			}
		}

		void Lexer::OnEmitComment( const Comment& )
		{

		}

		void Lexer::EmitComment( const Comment& comment )
		{
			OnEmitComment( comment );
		}

		void Lexer::ConsumeEOL( const Char* code, Uint32 codeLength, Int32& index )
		{
			if( static_cast< Uint32 >( index + 1 ) < codeLength )
			{
				if( code[ index ] == TXT( '\r' ) && code[ index + 1 ] == TXT( '\n' ) )
				{
					++index;
				}
			}
		}

		Bool Lexer::IsInteger( const Char* start, Int32 count ) const
		{
			Bool isHex = count >= 2 && start[ 0 ] == TXT( '0' ) && ( start[ 1 ] == TXT( 'x' ) || start[ 1 ] == TXT( 'X' ) );

			if ( isHex )
			{
				// All chars should be HEX numbers
				for ( Int32 i = 2; i < count; ++i )
				{
					if ( !IsHex( start[ i ] ) )
					{
						return false;
					}
				}
			}
			else
			{
				// All chars should be numbers
				for ( Int32 i = 0; i < count; ++i )
				{
					if ( !IsNum( start[i] ) )
					{
						return false;
					}
				}
			}

			// It is a number
			return true; 
		}

		Bool Lexer::IsFloat( const Char* start, Int32 count ) const
		{
			enum EFloatState
			{
				State_Decimal = 0, // Part before the decimal point '.'
				State_Fraction, // Part after
				State_End,

				State_Max
			};

			EFloatState state = State_Decimal;
			Int32 nums = 0;

			// Parse rest  
			for ( Int32 i = 0; i < count; ++i )
			{
				Char c = start[ i ];

				// Parsing state machine
				if ( state == State_Decimal )
				{
					// Number, continue in current state
					if ( IsNum( c ))
					{ 
						++nums;
						continue;
					}

					// Dot, go to state 1 (parsing other half)
					if ( c == TXT( '.' ) )
					{ 
						state = State_Fraction;
						continue;
					}

					// Invalid char
					return false;
				}
				else if ( state == State_Fraction )
				{
					// Number, continue in current state
					if ( IsNum( c ) )
					{ 
						++nums;
						continue;
					}

					// 'f', go to ending state
					if ( c == TXT( 'f' ) )
					{ 
						state = State_End;
						continue;
					}

					// Invalid char
					return false;
				}
				else if ( state == State_End )
				{ 
					// Every char here is invalid because 'f' should be last char :)
					return false;
				}
			}

			// Valid float was passed if we are in state 1 or 2
			return ( state != State_Decimal ) && nums;
		}

		Bool Lexer::IsKeyword( const Char* start, Int32 count ) const
		{
			enum EKeywordState
			{
				State_FirstLetter = 0,
				State_OtherLetters,

				State_Max
			};

			EKeywordState state = State_FirstLetter;

			// Matches any known keywords?
			for ( Uint32 j = 0; j < m_definition.GetKeywords().GetSize(); ++j )
			{
				Uint32 uCount = static_cast< Uint32 >( count );
				if ( StringLength( m_definition.GetKeywords()[ j ].m_keyword ) == uCount )
				{
					if ( StringCompare( m_definition.GetKeywords()[ j ].m_keyword, start, uCount ) == 0 )
					{
						// Valid known keyword was matched
						return true;
					}
				}
			}

			// Parse rest  
			for ( Int32 i = 0; i < count; ++i )
			{
				Char c = start[ i ];

				// Parsing state machine
				if ( state == State_FirstLetter )
				{
					// Allow '_' char
					if ( c == TXT( '_' ) )
					{ 
						state = State_OtherLetters;
						continue;
					}

					// In state 0 (first char) only letters are allowed
					if ( IsAlpha( c ) )
					{ 
						state = State_OtherLetters;
						continue;
					}

					// Invalid char
					return false;
				}
				else if ( state == State_OtherLetters )
				{
					// Allow '_' char
					if ( c == TXT( '_' ) )
					{
						continue;
					}

					// Other char can be letters or numbers
					if ( IsNum(c) || IsAlpha(c) )
					{
						continue;
					}

					// Invalid char
					return false;
				}
			}

			// If we're here valid identifier was parsed
			return true; 
		}
	}
}
