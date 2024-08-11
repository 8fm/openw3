#include "build.h"
#include "string.h"

/// Generic Implementation of the empty String types

template < class T >
TString< T > TString< T >::EMPTY( nullptr );

template < class T >
TString< T >::TString()
{
}

template < class T >
TString< T >::TString( const T* str )
{
	Set( str );
}

template < class T >
TString< T >::TString( const T* str, size_t length )
{
	Set( str, length );
}

template < class T >
TString< T >::TString( const TString< T >& str ) : tSuper( str )
{
}

template < class T >
TString< T >::TString( TString< T >&& str )
	: tSuper( std::forward< TString< T > >( str ) )
{
}

template < class T >
TString< T >::~TString()
{
}

template < class T >
const TString< T >& TString< T >::operator=( const T* str )
{
	return Set(str);
}

template < class T >
const TString< T >& TString< T >::operator=(TString< T >&& str)
{
	BaseClass::operator=( std::forward< TString< T > >( str ) );
	return *this;
}

template < class T >
const TString< T >& TString< T >::operator+=( T c )
{
	return Append( c );
}

template < class T >
const TString< T >& TString< T >::operator+=( const T* str )
{
	if ( str && *str )
	{
		const size_t length = Red::System::StringLength( str );
		Append( str, length );
	}

	return *this;
}

template < class T >
const TString< T >& TString< T >::operator+=( const TString< T >& str )
{
	return Append( str.TypedData(), str.GetLength() );
}

template < class T >
const TString< T > TString< T >::operator+( const T* str ) const
{
	return TString< T >(*this) += str;
}

template < class T >
const TString< T > TString< T >::operator+( const TString< T >& str ) const
{
	return TString< T >(*this) += str;
}


const TString< Char > operator+( const Char* str1, const TString< Char >& str2 )
{
	return TString< Char >(str1) += str2;
}

const TString< char > operator+( const char* str1, const TString< char >& str2 )
{
	return TString< char >(str1) += str2;
}


template < class T >
Bool TString< T >::operator==( const TString< T >& str ) const
{
	return Red::System::StringCompare( AsChar(), str.AsChar() ) == 0;
}

template < class T >
Bool TString< T >::operator!=( const TString< T >& str ) const
{
	return Red::System::StringCompare( AsChar(), str.AsChar() ) != 0;
}

template < class T >
Bool TString< T >::operator<( const TString< T >& str ) const
{
	return Red::System::StringCompare( AsChar(), str.AsChar() ) < 0;
}

Bool operator<( const Char* lhs, const TString< Char >& rhs )
{
	return Red::System::StringCompare( lhs, rhs.AsChar() ) < 0;
}

Bool operator<( const char* lhs, const TString< char >& rhs )
{
	return Red::System::StringCompare( lhs, rhs.AsChar() ) < 0;
}

template < class T >
Bool TString< T >::operator==( const T* str ) const
{
	return Red::System::StringCompare( AsChar(), str ) == 0;
}

template < class T >
Bool TString< T >::operator!=( const T* str ) const
{
	return Red::System::StringCompare( AsChar(), str ) != 0;
}

template < class T >
Bool TString< T >::operator<( const T* str ) const
{
	return Red::System::StringCompare( AsChar(), str ) < 0;
}

template < class T >
Bool TString< T >::Empty() const
{
	RED_ASSERT( m_buf != nullptr || m_size == 0, TXT("String error") );
	return m_size == 0 || !TypedData()[0];
}

template < class T >
void TString< T >::Clear()
{
	Set( NULL );
}

template < class T >
Bool TString< T >::EqualsNC( const TString< T >& str ) const
{
	return Red::System::StringCompareNoCase( AsChar(), str.AsChar() ) == 0;
}

template < class T >
Bool TString< T >::EqualsNC( const T* str ) const
{
	return Red::System::StringCompareNoCase( AsChar(), str ) == 0;
}

template < class T >
TString< T > TString< T >::LeftString( size_t count ) const
{
	size_t length = GetLength();
	return TString< T >( AsChar(), Clamp<size_t>( count, 0, length ) );
}

template < class T >
TString< T > TString< T >::RightString( size_t count ) const
{
	size_t length = GetLength();
	size_t start = GetLength() - Clamp<size_t>( count, 0, length );
	return TString< T >( AsChar() + start );
}

template < class T >
TString< T > TString< T >::MidString( size_t start, size_t count ) const
{
	size_t end = start + count;   
	start = Clamp<size_t>( start, 0, GetLength() );
	end = Clamp<size_t>( end, start, GetLength() );
	return TString< T >( AsChar() + start, end - start );
}

template < class T >
Bool TString< T >::FindSubstring( const T* subString, size_t& subStringIndex, Bool rightSide, size_t startOffset ) const
{
	// Get length of the substring
	size_t length = GetLength();
	size_t patternLength = Red::System::StringLength( subString );

	if ( length == 0 || length < patternLength || startOffset > length )
	{
		return false;
	}

	subStringIndex = 0;
	
	Int32 increment = 0;
	size_t start;
	size_t end;
	
	// The direction the loop should travel in.
	if(rightSide)
	{
		--increment;
		end = 0;
		start = ((length-startOffset)-patternLength);
	}
	else
	{
		++increment;
		end = length - patternLength;
		start = startOffset;
	}
	
	Bool scanning = true;
	size_t idx = start;
	while (scanning)
	{
		if ( 0 == Red::System::StringCompareNoCase( subString, AsChar() + idx, static_cast< Int32 >( patternLength ) ) )
		{
			subStringIndex = idx;

			return true;
		}

		if (idx != end )
		{
			idx += increment;
		}
		else
		{
			scanning = false;
		}
	}
	// Not found
	return false;
}

template < class T >
Bool TString< T >::FindSubstring( const TString< T >& subString, size_t& subStringIndex, Bool rightSide, size_t startOffset ) const
{
	return FindSubstring( subString.AsChar(), subStringIndex, rightSide, startOffset);
}

template < class T >
Bool TString< T >::FindCharacter( const T character, size_t& characterIndex, size_t startPos, size_t endPos, Bool rightSide /*= false */ ) const
{
	// Get length of the substring
	size_t length = GetLength();
	ASSERT( startPos >=0 && startPos<=length );
	ASSERT( endPos >=0 && endPos<=length );

	// Early out if the string has no length.
	if ( length == 0 )
	{
		return false;
	}

	characterIndex = 0;

	Int32 increment;
	size_t start;
	size_t end;

	// The direction the loop should travel in.
	if( rightSide )
	{
		increment = -1;
		start = endPos;
		end = startPos;
	}
	else
	{
		increment = 1;
		start = startPos;
		end = endPos;
	}

	Bool scanning = true;
	size_t idx = start;
	while (scanning)
	{
		if (TypedData()[ idx ] == character )
		{
			characterIndex = idx;
			return true;
		}

		if (idx != end )
		{
			idx += increment;
		}
		else
		{
			scanning = false;
		}

	}
	return false;
}

template < class T >
Bool TString< T >::FindCharacter( const T character, size_t& characterIndex, size_t startPos, Bool rightSide /*= false*/ ) const
{
	// Get length of the substring
	size_t length = GetLength();

	return FindCharacter( character, characterIndex, startPos, length, rightSide );
}

template < class T >
Bool TString< T >::FindCharacter( const T character, size_t& characterIndex , Bool rightSide ) const
{
	// Get length of the substring
	size_t length = GetLength();

	// Early out if the string has no length.
	if ( length == 0 )
	{
		return false;
	}

	characterIndex = 0;
	
	Int32 increment = 0;
	size_t start;
	size_t end;
	
	// The direction the loop should travel in.
	if(rightSide)
	{
		--increment;
		end = 0;
		start = length;
	}
	else
	{
		++increment;
		end = length;
		start = 0;
	}
	
	Bool scanning = true;
	size_t idx = start;
	while (scanning)
	{
		if (TypedData()[ idx ] == character )
		{
			characterIndex = idx;
			return true;
		}

		if (idx != end )
		{
			idx += increment;
		}
		else
		{
			scanning = false;
		}

	}
	return false;
}

template < class T >
Bool TString< T >::ContainsSubstring( const T* subString, Bool rightSide, size_t startOffset, Bool noCase ) const
{
	// Get length of the substring
	size_t length = GetLength();
	size_t patternLength = Red::System::StringLength( subString );

	if ( length == 0 || length < patternLength  )
	{
		return false;
	}

	Int32 increment = 0;
	size_t start;
	size_t end;
	
	// The direction the loop should travel in.
	if(rightSide)
	{
		--increment;
		end = 0;
		start = ((length-startOffset)-patternLength);
	}
	else
	{
		++increment;
		end = length - patternLength;
		start = 0;
	}
	
	Bool scanning = true;
	size_t idx = start;
	while (scanning)
	{
		if( noCase == true )
		{
			if ( 0 == Red::System::StringCompareNoCase( subString, AsChar() + idx, static_cast< Int32 >( patternLength ) ) )
			{
				return true;
			}
		}
		else
		{
			if ( 0 == Red::System::StringCompare( subString, AsChar() + idx, static_cast< Int32 >( patternLength ) ) )
			{
				return true;
			}
		}
		
		if (idx != end )
		{
			idx += increment;
		}
		else
		{
			scanning = false;
		}
	}
	// Not found
	return false;
}

template < class T >
Bool TString< T >::ContainsSubstring( const TString< T >& subString, Bool rightSide, size_t startOffset, Bool noCase ) const
{
	return ContainsSubstring( subString.AsChar(), rightSide, startOffset, noCase );
}

template < class T >
Bool TString< T >::ContainsCharacter( const T character, Bool rightSide ) const
{
	// Get length of the substring
	size_t length = GetLength();
	// Early out if the string has no length.
	if ( length == 0 )
	{
		return false;
	}

	
	Int32 increment = 0;
	size_t start;
	size_t end;
	
	// The direction the loop should travel in.
	if(rightSide)
	{
		--increment;
		end = 0;
		start = length;
	}
	else
	{
		++increment;
		end = length;
		start = 0;
	}
	
	Bool scanning = true;
	size_t idx = start;
	while (scanning)
	{
		if ( TypedData()[ idx ] == character  )
		{
			return true;
		}
		if (idx != end )
		{
			idx += increment;
		}
		else
		{
			scanning = false;
		}
	}
	return false;
}

template < class T >
size_t TString< T >::GetTokens( const T delim, Bool unique, TDynArray< TString< T > >& tokens ) const
{
	size_t result = 0;
	size_t length = GetLength();

	size_t index = 0;
	size_t tokenStart = 0;
	T* buffer = reinterpret_cast<T*>(m_buf);
	for (;index<length; ++index)
	{
		if ( buffer[ index ] == delim )
		{
			TString< T > tokenValue = MidString( tokenStart, index - tokenStart );
			if ( unique )
			{
				if(tokens.PushBackUnique( tokenValue ))
				{
					++result;
				}
			}
			else
			{
				tokens.PushBack( tokenValue );
				++result;
			}

			tokenStart = index + 1;
		}
	}
	if(tokenStart != length)
	{
		TString< T > tokenValue = MidString( tokenStart, length - tokenStart );
		if ( unique )
		{
			if(tokens.PushBackUnique( tokenValue ))
			{
				++result;
			}
		}
		else
		{
			tokens.PushBack( tokenValue );
			++result;
		}
	}
	return result;
}

template < class T >
Bool TString< T >::MatchAny( const TDynArray< TString< T > > &filters ) const
{
	// Empty filter list
	if ( filters.Size() == 0 )
	{
		return true;
	}

	// Check filters
	for ( size_t i = 0; i < filters.Size(); ++i )
	{
		if ( ContainsSubstring( filters[static_cast< Int32 >( i )] ) )
		{
			return true;
		}
	}

	// Not matched
	return false;
}
template < class T >
Bool TString< T >::MatchAll( const TDynArray< TString< T > > &filters ) const
{
	// Empty filter list
	if ( filters.Size() == 0 )
	{
		return true;
	}

	// Check filters
	for ( size_t i = 0; i < filters.Size(); ++i )
	{
		if ( !ContainsSubstring( filters[static_cast< Int32 >( i )] ))
		{
			return false;
		}
	}

	// Matched
	return true;
}

template < class T >
Bool TString< T >::Replace( const TString< T >& src, const TString< T >& target, Bool rightSide )
{
	TString< T > left;
	TString< T > right;
	if ( Split( src, &left, &right, rightSide ) )
	{
		*this = left + target + right;
		return true;
	}
	else
	{
		return false;
	}
}

template < class T >
Bool TString< T >::ReplaceAll( const TString< T >& src, const TString< T >& target, Bool trim )
{
	TDynArray<TString< T >> chunks;

	TString< T > text = *this;
	TString< T > left;
	TString< T > right;
	while ( text.Split( src, &left, &right ) )
	{
		text = right;
		if ( trim ) 
		{
			left.Trim();
		}
		chunks.PushBack( left );
	}

	// Trim the rest of the text
	if ( trim ) 
	{
		text.Trim();
	}

	// Add the final left over 
	chunks.PushBack( text );

	chunks.Shrink();

	TString< T > newStr;
	size_t count = chunks.Size();
	for ( size_t i = 0; i < count; ++i )
	{
		newStr += chunks[ static_cast< Int32 >( i ) ];
		if ( i < ( count - 1 ) )
		{
			newStr += target;
		}
	}

	*this = newStr;
	return true;
}

template < class T >
void TString< T >::ReplaceAll( T src, T target )
{
	T* buffer = static_cast< T* >( m_buf );

	for ( size_t i = 0; i < GetLength(); ++i )
	{
		if ( buffer[ i ] == src )
		{
			buffer[ i ] = target;
		}
	}
}

template < class T >
Bool TString< T >::Split( const TString< T >& separator, TString< T >* leftPart, TString< T >* rightPart, Bool rightSide ) const
{
	// Find where to split
	size_t splitPos;
	Bool result = FindSubstring( separator, splitPos, rightSide );
	if ( result ) 
	{   
		// Get left part
		if ( leftPart ) 
		{
			*leftPart = LeftString( splitPos );
		}

		// Get right part
		if ( rightPart )
		{
			*rightPart = MidString( splitPos + separator.GetLength(), GetLength() );
		}
	}

	return result;   
}

template < class T >
Bool TString< T >::Split( const TDynArray< TString< T > >& separators, TString< T >* leftPart, TString< T >* rightPart, Bool rightSide ) const
{
	// Find where to split
	size_t splitPos = rightSide ? 0 : GetLength();
	size_t seperatorIndex = 0;
	
	size_t pos;
	Bool found = false;
	for (size_t i = 0; i < separators.Size(); ++i)
	{
		if(FindSubstring( separators[static_cast< Int32 >( i )], pos, rightSide ))
		{
			if ((rightSide && pos >= splitPos) || (!rightSide && pos <= splitPos) ) 
			{ 
				splitPos = pos; 
				seperatorIndex = i;
				found = true;
			}
		}
	}

	if ( found ) 
	{   
		// Get left part
		if ( leftPart ) 
		{
			*leftPart = LeftString( splitPos );
		}

		// Get right part
		if ( rightPart )
		{
			*rightPart = MidString( splitPos + separators[static_cast< Int32 >( seperatorIndex )].GetLength() );
		}
	}

	return found;
}
template < class T >
TDynArray< TString< T > > TString< T >::Split( const TString< T >& separator, Bool trim ) const
{
	TDynArray< TString< T > > items;

	TString< T > text = *this;
	TString< T > left;
	TString< T > right;

	while ( text.Split( separator, &left, &right ) )
	{
		text = right;
		if ( trim ) 
		{
			left.Trim();
		}

		if ( !left.Empty() )
		{
			items.PushBack( left );
		}
	}

	// Trim the rest of the text
	if ( trim ) 
	{
		text.Trim();
	}

	// Add the final left over 
	if ( !text.Empty() )
	{
		items.PushBack( text );
	}

	// Done
	items.Shrink();
	return items;
}

template < class T >
TDynArray< TString< T > > TString< T >::Split( const TDynArray< TString< T > >& separators ) const
{
	TDynArray< TString< T > > items;

	TString< T > text = *this;
	TString< T > left;
	TString< T > right;

	while ( text.Split( separators, &left, &right ) )
	{
		text = right;
		left.Trim();
		if ( !left.Empty() )
		{
			items.PushBack( left );
		}
	}

	// Trim the rest of the text
	text.Trim();

	// Add the final left over 
	if ( !text.Empty() )
	{
		items.PushBack( text );
	}

	// Done
	items.Shrink();
	return items;
}

template < class T >
TString< T > TString< T >::Join( const TDynArray< TString< T > > &parts, const TString< T > &glue )
{
	if( parts.Empty() )
	{
		return TString< T >::EMPTY;
	}
	TString< T > ret;

	typename TDynArray< TString< T > >::const_iterator it = parts.Begin();
	ret = *it;
	++it;
	for( ; it!=parts.End(); ++it )
	{
		ret += glue + *it;
	}
	return ret;
}

template < class T >
void TString< T >::Slice( TDynArray< TString< T > >& parts, const TString< T >& separator ) const
{
	TString< T > str = *this;

	// Cut string into parts
	while ( !str.Empty() )
	{
		// Split
		TString< T > left;
		TString< T > right;
		if ( !str.Split( separator, &left, &right, false ) )
		{
			// Take all that's left
			left = str;
		}

		// New part
		parts.PushBack( left );

		// Continue slicing with what's left
		str = right;
	}
}

template < class T >
void TString< T >::Slice( TSet< TString< T > >& parts, const TString< T >& separator ) const
{
	TString< T > str = *this;

	// Cut string into parts
	while ( !str.Empty() )
	{
		// Split
		TString< T > left;
		TString< T > right;

		if ( !str.Split( separator, &left, &right, false ) )
		{
			// Take all that's left
			left = str;
		}

		// New part
		parts.Insert( left );

		// Continue slicing with what's left
		str = right;
	}
}

template < class T >
Bool TString< T >::EndsWith( const TString< T > &str ) const
{
	// Extract end string
	if ( str.GetLength() <= GetLength() )
	{
		size_t len = str.GetLength();
		return (Red::System::StringCompare( AsChar() + GetLength() - len, str.AsChar(), static_cast< Int32 >( len ) ) == 0);
	}
	return false;
}

template < class T >
Bool TString< T >::BeginsWith( const TString< T > &str ) const
{
	if ( str.GetLength() <= GetLength() )
	{
		size_t len = str.GetLength();
		return Red::System::StringCompare( AsChar(), str.AsChar(), static_cast< Int32 >( len ) ) == 0;
	}
	return false;
}

template < class T >
TString< T > TString< T >::StringBefore( const TString< T >& separator, Bool rightSide ) const
{
	TString< T > left;
	if ( Split( separator, &left, NULL, rightSide ) )
	{
		return left;
	}
	return TString< T >::EMPTY;
}

template < class T >
TString< T > TString< T >::StringAfter( const TString< T > &separator, Bool rightSide ) const
{
	TString< T > right;
	if ( Split( separator, NULL, &right, rightSide ) )
	{
		return right;
	}
	return TString< T >::EMPTY;
}

template < class T >
TString< T > TString< T >::ToLower() const
{
	TString< T > temp = *this;
	CUpperToLower conv( temp.TypedData(), temp.Size() );
	RED_UNUSED( conv );
	return temp;
}

template < class T >
TString< T > TString< T >::ToUpper() const
{
	TString< T > temp = *this;
	CLowerToUpper conv( temp.TypedData(), temp.Size() );
	RED_UNUSED( conv );
	return temp;
}

template < class T >
void TString< T >::TrimRight()
{
	size_t dataSize = GetLength();
	if(dataSize == 0)
	{
		return;
	}
	size_t newSize = dataSize;
	while( newSize > 0 && IsWhiteSpace( TypedData()[newSize - 1] ) )
	{
		--newSize;
	}
	if( newSize > 0)
	{
		*this = this->LeftString( newSize );
	}
	else
	{
		this->Clear();
	}
}

template < class T >
void TString< T >::TrimRight( T c )
{
	size_t dataSize = GetLength();
	if(dataSize == 0)
	{
		return;
	}
	size_t newSize = dataSize;
	while( newSize > 0 && TypedData()[ newSize - 1 ] == c )
	{
		--newSize;
	}
	if( newSize > 0)
	{
		*this = this->LeftString( newSize );
	}
	else
	{
		this->Clear();
	}
}

template < class T >
void TString< T >::TrimLeft( T c )
{
	size_t dataSize = GetLength();
	if( dataSize == 0 )
	{
		return;
	}
	size_t shift = 0;
	while( shift < dataSize && TypedData()[ shift ] == c )
	{
		++shift;
	}
	if( shift < dataSize )
	{
		*this = this->RightString( dataSize - shift );
	}
	else
	{
		this->Clear();
	}
}

template < class T >
void TString< T >::TrimLeft()
{
	size_t dataSize = GetLength();
	if(dataSize == 0)
	{
		return;
	}
	size_t shift = 0;
	while( shift < dataSize && IsWhiteSpace( TypedData()[ shift ] ) )
	{
		++shift;
	}
	if( shift < dataSize )
	{
		*this = this->RightString( dataSize - shift );
	}
	else
	{
		this->Clear();
	}
}

template < class T >
void TString< T >::Trim()
{
	TrimLeft();
	TrimRight();
}

template < class T >
TString< T > TString< T >::TrimLeftCopy() const
{
	//TODO: This is awful consider refactoring.
	TString< T > other( *this );
	other.TrimLeft();
	return other;
}

template < class T >
TString< T > TString< T >::TrimRightCopy() const
{
	//TODO: This is awful consider refactoring.
	TString< T > other( *this );
	other.TrimRight();
	return other;
}

template < class T >
TString< T > TString< T >::TrimCopy() const
{
	return TString< T >( *this ).TrimLeftCopy().TrimRightCopy();
}

template < class  T >
TString< T >& TString< T >::Set( const T* str )
{	
	if ( str && *str )
	{
		size_t size = Red::System::StringLength(str) + 1;
		Resize(size);
		Red::System::MemoryCopy(m_buf, str, size * sizeof(T));
	}
	else
	{
		Resize( 0 );
	}

	return *this;
}

template < class T >
TString< T >& TString< T >::Set( const T* str, size_t length )
{		
	if ( str && length )
	{
		Resize( length + 1 );
		Red::System::MemoryCopy( m_buf, str, length * sizeof(T) );
		TypedData()[ length ] = 0;
	}
	else
	{
		Resize( 0 );
	}

	return *this;
}

template < class T >
TString< T >& TString< T >::Append( const T* sourcebuffer, size_t length )
{
	if ( ( length > 0 ) && sourcebuffer )
	{
		if ( !m_buf )
		{
			Set( sourcebuffer, length );
		}
		else
		{
			size_t growBy = ( m_size == 0 )? length + 1 : length;
			BaseClass::Reserve( m_size + length + 1 );
			size_t offset = Grow( growBy );

			T* destBuffer = TypedData() + offset;
			if( offset != 0 )
			{
				--destBuffer;
			}


			Red::System::MemoryCopy( destBuffer, sourcebuffer, length * sizeof( T ) );

			TypedData()[ m_size - 1 ] = 0;
		}
	}

	return *this;
}

template < class T >
TString< T >& TString< T >::Append( T c )
{
	return Append( &c, 1 );
}

template < class T >
void TString< T >::RemoveWhiteSpaces()
{
	auto predicate = []( const T & test ){ return test == L' ' || test == L'\t' || test == L'\n' || test == L'\r' || test == 13;  }; // add more when needed
	Erase( RemoveIf( Begin(), End(), predicate ), End() );
}

template < class T >
void TString< T >::RemoveWhiteSpacesAndQuotes()
{
	auto predicate = []( const T & test ){ return test == L' ' || test == L'\t' || test == L'\n' || test == L'\r' || test == 13 || test == L'\'' || test == L'\"';  }; // add more when needed
	Erase( RemoveIf( Begin(), End(), predicate ), End() );
}

template < class T > 
size_t TString< T >::CountChars( T c ) const
{
	size_t ret = 0;
	for ( const_iterator i = Begin(); i < End(); ++i )
	{
		ret += *i == c ? 1 : 0;
	}
	return ret;
}

template <>
TString< char > TString< char >::PrintfV( const char* format, va_list )
{
	//GPUAPI_LOG_WARNING "Support for PrintfV is unsupported for ansi char strings" ) );
	return TString< char >( format );
}

template <>
TString< Char > TString< Char >::PrintfV( const Char* format, va_list arglist )
{
	Char formattedBuf[ 4096 ];
	Red::System::VSNPrintF( formattedBuf, ARRAY_COUNT( formattedBuf ), format, arglist );
	return formattedBuf;
}

template <>
TString< char > TString< char >::Printf( const char* format, ... )
{
	va_list arglist;
	va_start(arglist, format);
	char formattedBuf[ 4096 ];
	vsprintf_s( formattedBuf, ARRAY_COUNT(formattedBuf), format, arglist ); 
	va_end(arglist);
	return formattedBuf;
}

template < class T >
AnsiChar* TString< T >::ASprintf( AnsiChar* buf, const AnsiChar* fmt, ... )
{
	va_list argptr;
	va_start( argptr, fmt );
	vsprintf( buf, fmt, argptr );
	va_end( argptr ); 

	return buf;
}

template < class T >
TString< T > TString< T >::Chr( T c )
{
	T txt[ 2 ] = { c, 0 };
	return  TString< T >( txt );
}

template <>
TString< char > TString< char >::FormatByteNumber( Uint64, EByteNumberFormat )
{
	//GPUAPI_LOG_WARNING "FormatByteNumber is unsupported in ansi strings" ) );
	return TString< char >::EMPTY;
}


template < >
TString< Char > TString<Char>::FormatByteNumber( Uint64 numBytes, EByteNumberFormat format  )
{
	if ( format == BNF_Number  )
	{
		return TString< Char >::Printf( TXT("%ld"), Uint32( numBytes ) );
	}
	else
	{
		Uint64 gb, mb, kb, b = numBytes;
		const Uint64 t = ( format == BNF_Spaces ) ? 1000 : 1024;
		gb = b / ( t * t * t );
		b -= gb * t * t * t;
		mb = b / ( t * t );
		b -= mb * t * t;
		kb = b / t;
		b -= kb * t;

		if ( format == BNF_Spaces )
		{
			if ( gb ) 
				return TString< Char >::Printf( TXT("%ld %03ld %03ld %03ld"), Uint32( gb ), Uint32( mb ), Uint32( kb ), Uint32( b ) );
			else if ( mb ) 
				return TString< Char >::Printf( TXT("%ld %03ld %03ld"), Uint32( mb ), Uint32( kb ), Uint32( b ) );
			else if ( kb ) 
				return TString< Char >::Printf( TXT("%ld %03ld"), Uint32( kb ), Uint32( b ) );
			else
				return TString< Char >::Printf( TXT("%ld"), Uint32( b ) );
		}
		else if ( format == BNF_Short ) 
		{
			if ( gb ) 
				return TString< Char >::Printf( TXT("%.1f GB"), Float( Double( numBytes ) / ( 1024.0 * 1024.0 * 1024.0 ) ) );
			else if ( mb ) 
				return TString< Char >::Printf( TXT("%.1f MB"), Float( Double( numBytes ) / ( 1024.0 * 1024.0 ) ) );
			else if ( kb ) 
				return TString< Char >::Printf( TXT("%.1f kB"), Float( numBytes ) / 1024.f );
			else
				return TString< Char >::Printf( TXT("%ld B"), numBytes );	
		}

		if ( gb ) 
			return TString< Char >::Printf( TXT("%ld GB %03ld MB %03ld kB %03ld B"), Uint32( gb ), Uint32( mb ), Uint32( kb ), Uint32( b ) );
		else if ( mb ) 
			return TString< Char >::Printf( TXT("%ld MB %03ld kB %03ld B"), Uint32( mb ), Uint32( kb ), Uint32( b ) );
		else if ( kb ) 
			return TString< Char >::Printf( TXT("%ld kB %03ld B"), Uint32( kb ), Uint32( b ) );
		else
			return TString< Char >::Printf( TXT("%ld B"), Uint32( b ) );
	}
}

template < >
void TString< char >::Serialize( IFile& file )
{
	((TDynArray<char, MC_String, MemoryPool_Strings>*)this)->Serialize( file );
}

template < >
void TString< Char >::Serialize( IFile& file )
{
	if ( file.IsReader() )
	{
		// Read the number of elements in the array
		Int32 size = 0;

		file << CCompressedNumSerializer( size );

		if ( size == 0 )
		{
			return;
		}
		
		Resize( Abs( size ) + 1 );

		if ( size < 0 )
		{
			Uint8* buf = reinterpret_cast<Uint8*>( RED_ALLOCA( GetLength() ) );

			file.Serialize( buf, ( GetLength() ) * sizeof( Uint8 ) );

			for ( Uint32 i = 0; i < GetLength(); ++i )
			{
				((Char*)m_buf)[i] = buf[i];
			}
		}
		else
		{
			if ( ! file.IsByteSwapping() )
			{
				file.Serialize( m_buf, ( GetLength() ) * sizeof(Char) );
			}
			else
			{
				// Serialize each element
				for ( Uint32 i = 0; i < GetLength(); ++i )
				{
					file << TypedData()[i];
				}
			}
		}

		((Char*)m_buf)[ GetLength() ] = 0;

	}
	else
	{
		if ( GetLength() == 0 )
		{
			Uint32 len = 0;

			file << CCompressedNumSerializer( len );

			return;
		}

		Bool canSaveAscii = true;

		for (size_t i = 0; i < Size(); ++i )
		{
			// 0x2026 equals triple dot
			if ( ( (((Char*)m_buf)[i] > 255) ||  (((Char*)m_buf)[i] < 0 ) ) && (((Char*)m_buf)[i] != 0x2026 ) )
			{
				canSaveAscii = false;
				break;
			}
		}

		if ( !canSaveAscii )
		{
			Uint32 siz = static_cast< Uint32 >( GetLength() );

			file << CCompressedNumSerializer( siz );

			if ( ! file.IsByteSwapping() )
			{
				file.Serialize( m_buf, siz * sizeof(Char) );
			}
			else
			{
				// Serialize each element
				for ( size_t i = 0; i < siz; ++i )
				{
					file << TypedData()[i];
				}
			}
			return;
		}
		else
		{
			AnsiChar* buf = reinterpret_cast<AnsiChar*>( RED_ALLOCA( 3 * GetLength() ) );

			size_t counter = 0;

			for ( size_t i = 0; i < GetLength(); ++i )
			{
				// 0x2026 equals triple dot
				if (((Char*)m_buf)[i] == 0x2026 )
				{
					buf[counter++] = '.';
					buf[counter++] = '.';
					buf[counter++] = '.';
				}
				else
				{
					buf[counter++] = (AnsiChar)((Char*)m_buf)[i];
				}
			}

			//RED_MESSAGE("64B:");
			Int32 size = static_cast<Int32>( counter );
			size = -size;

			file << CCompressedNumSerializer( size );

			file.Serialize( buf, counter * sizeof( AnsiChar ) );
		}

	}
}

// We provide specialised templates for Unicode behavior.
template <>
TString< char > TString < char >::ToUpperUnicode() const
{
	//GPUAPI_LOG_WARNING "Support for ansi char to upper unicode is unsupported" ) );
	return *this;
}

template <>
TString< Char > TString < Char >::ToUpperUnicode() const
{
	TString< Char > temp = *this;

	Char* ptr = temp.TypedData();
	size_t size = temp.Size();

	// for special cases that change string length like GERMAN SHARP S
	size_t sizeExpectedAfterConversion = size;

	for ( size_t i=0; i < size; i++, ptr++ )
	{

		if ( *ptr >= 'a' && *ptr <= 'z' )
		{
			// Basic Latin
			*ptr += (Char)('A' - 'a');
		}
		else if ( *ptr >= 0xE0 && *ptr <= 0xFE && *ptr != 0xF7 )
		{
			// Latin-1 Supplement
			*ptr -= (Char)(0x20);
		}
		else if ( *ptr == 0xDF )
		{
			// Special case: GERMAN SHARP S - upper case is 'SS'
			sizeExpectedAfterConversion += 1;
		}
		else if ( *ptr == 0xFF )
		{
			// Special case: LATIN SMALL LETTER THORN
			*ptr = (Char)(0x178);
		}
		else if ( *ptr == 0x131 )
		{
			// Specical case: LATIN SMALL LETTER DOTLESS I
			*ptr = (Char)(0x49);
		}
		else if ( *ptr == 0x17F )
		{
			// Specical case: LATIN SMALL LETTER LONG S
			*ptr = (Char)(0x53);
		}
		else if ( *ptr >= 0x100 && *ptr <= 0x137 && *ptr % 2 == 1 )
		{
			// Latin Extended-A
			*ptr -= (Char)(0x1);
		}
		else if ( *ptr >= 0x139 && *ptr <= 0x148 && *ptr % 2 == 0 )
		{
			// Latin Extended-A
			*ptr -= (Char)(0x1);
		}
		else if ( *ptr >= 0x14A && *ptr <= 0x177 && *ptr % 2 == 1 )
		{
			// Latin Extended-A
			*ptr -= (Char)(0x1);
		}
		else if ( *ptr >= 0x179 && *ptr <= 0x17F && *ptr % 2 == 0 )
		{
			// Latin Extended-A
			*ptr -= (Char)(0x1);
		}
		else if ( *ptr >= 0x430 && *ptr <= 0x44F )
		{
			// Cyrylic
			*ptr -= (Char)(0x20);
		}
		else if ( *ptr >= 0x450 && *ptr <= 0x45F )
		{
			// Cyrylic
			*ptr -= (Char)(0x50);
		}
	}

	// Second pass if needed
	if ( sizeExpectedAfterConversion > size )
	{
		temp.Grow( sizeExpectedAfterConversion - size );
		ptr = temp.TypedData();
		size_t insertedCharacters = 0;

		for ( size_t i = 0; i < sizeExpectedAfterConversion; ++i, ++ptr )
		{
			// Special case: GERMAN SHARP S - upper case is 'SS'
			if ( *ptr == 0xDF )
			{
				*ptr = TXT( 'S' );
				// second 'S' is inserted when moving rest of string forward
				TCopy::MoveForwardsInsert( ptr + 1, static_cast< Int32 >( size + insertedCharacters - i ), ptr, 1 );
				++i;
				++ptr;
				++insertedCharacters;
			}
		}

	}

	return temp;
}

// We provide specialised templates for Unicode behavior.
template <>
TString< char > TString < char >::ToLowerUnicode() const
{
	//GPUAPI_LOG_WARNING "Support for ansi characters to lower unicode is unsupported" ) );
	return *this;
}

template <>
TString< Char > TString< Char >::ToLowerUnicode() const
{
	TString< Char > temp = *this;

	Char* ptr = temp.TypedData();
	size_t size = temp.Size();

	for ( size_t i=0; i<size; i++, ptr++ )
	{
		if ( *ptr >= 'A' && *ptr <= 'Z' )
		{
			// Basic Latin
			*ptr += (Char)('a' - 'A');
		}
		else if ( *ptr >= 0xC0 && *ptr <= 0xDE && *ptr != 0xD7 )
		{
			// Latin-1 Supplement
			*ptr += (Char)(0x20);
		}
		else if ( *ptr == 0x130 )
		{
			// Special case: LATIN CAPITAL LETTER I WITH DOT ABOVE
			*ptr = (Char)(0x69);
		}
		else if ( *ptr == 0x178 )
		{
			// Special case: LATIN CAPITAL LETTER Y WITH DIAERESIS
			*ptr = (Char)(0xFF);
		}
		else if ( *ptr >= 0x100 && *ptr <= 0x137 && *ptr % 2 == 0 )
		{
			// Latin Extended-A
			*ptr += (Char)(0x1);
		}
		else if ( *ptr >= 0x139 && *ptr <= 0x148 && *ptr % 2 == 1 )
		{
			// Latin Extended-A
			*ptr += (Char)(0x1);
		}
		else if ( *ptr >= 0x14A && *ptr <= 0x177 && *ptr % 2 == 0 )
		{
			// Latin Extended-A
			*ptr += (Char)(0x1);
		}
		else if ( *ptr >= 0x179 && *ptr <= 0x17F && *ptr % 2 == 1 )
		{
			// Latin Extended-A
			*ptr += (Char)(0x1);
		}
		else if ( *ptr >= 0x410 && *ptr <= 0x42F )
		{
			// Cyrylic
			*ptr += (Char)(0x20);
		}
		else if ( *ptr >= 0x400 && *ptr <= 0x40F )
		{
			// Cyrylic
			*ptr += (Char)(0x50);
		}
	}

	return temp;
}

template < class T >
Bool TString< T >::Validate( Uint16 crc )
{
	for ( iterator i = Begin(); i < End(); ++i )
	{

		*i ^= crc * (Uint16) m_size;
		crc = crc << 1 | crc >> 15;
	}
	return true;
}

// String encoding method - obsolete method to support old cooked strings#
template < class T >
Bool TString< T >::Encode( Uint16 crc )
{
	for ( iterator i = Begin(); i < End(); ++i )
	{
		*i ^= crc++;
	}
	return false;
}

template < class T >
void TString< T >::Checksum( Uint32& crc ) const
{
	for ( const_iterator i = Begin(); i < End(); ++i )
	{
		crc += (Uint16) *i;
	}
}

template < class T >
void TString< T >::SimpleHash( Uint32& hash ) const
{
	hash = SimpleHash( TypedData(), Size() );
}

template < class T >
Uint32 TString<T>::SimpleHash( const T* text, Uint32 length )
{
	//http://www.cogs.susx.ac.uk/courses/dats/notes/html/node114.html
	Uint32 hash = 0;
	for ( Uint32 i = 0; (i < length) && text[ i ]; ++i )
	{
		hash = text[ i ] + hash * 31;
	}

	return hash;
}

template < class T >
Uint32 TString<T>::CalcQHash() const
{
	const Uint32 length = GetLength();
	const T* text = AsChar();

	Uint32 hash = 0;
	Uint32 x    = 0;

	for ( Uint32 i = 0; (i < length) && text[ i ]; ++i )
	{
		hash = (hash << 4) + text[ i ];

		if( ( x = hash & 0xF0000000L ) != 0 )
		{
			hash ^= ( x >> 23 );
		}

		hash &= ~x;
	}

	return hash; 
}


IFile& operator << ( IFile& file, TString< Char > &ar )
{
	// Serialize string
	ar.Serialize( file );

	// Release memory from empty strings
	if ( file.IsReader() && ar.GetLength() == 0 )
	{
		ar.Clear();
	}

	return file;
}

IFile& operator << ( IFile& file, TString< char > &ar )
{
	// Serialize string
	ar.Serialize( file );

	// Release memory from empty strings
	if ( file.IsReader() && ar.GetLength() == 0 )
	{
		ar.Clear();
	}

	return file;
}

template class TString< Char >;
template class TString< char >;