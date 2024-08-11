#pragma once
//

// Nice RTTI mixed parser
template< typename T >
RED_INLINE Bool GParseType( const Char*& stream, T& value )
{
	const Char* original = stream;

	// Check if type is registered in RTTI and create matching variant
	CVariant theType( ::GetTypeName<T>(), NULL );
	if ( !theType.IsValid() )
	{
		return false;
	}

	// Parse token string
	String token;
	if ( GParseToken( stream, token ) )
	{
		if ( theType.FromString( token ) )
		{
			if ( theType.AsType( value ) )
			{
				// Parsed
				return true;
			}
		}
	}

	// Not parsed
	stream = original;
	return false;
}

/// Cast from one array to other array
template< class T, class W >
RED_INLINE TDynArray< T* > CastArray( const TDynArray< W* >& source, Bool keepNulls=false )
{
	TDynArray< T* > ret;
	for ( Uint32 i=0; i<source.Size(); i++ )
	{
		T* castedElement = Cast< T >( source[i] );
		if ( castedElement || keepNulls )
		{
			ret.PushBack( castedElement );
		}
	}

	return ret;
}

/// Cast from one array to other array
template< class T, class W, size_t MaxSize >
RED_INLINE void CastArray( const TStaticArray< W*, MaxSize >& source, TStaticArray< T*, MaxSize >& destination, Bool keepNulls=false )
{
	destination.Clear();
	for ( size_t i = 0; i < source.Size(); i++ )
	{
		T* castedElement = Cast< T >( source[i] );
		if ( castedElement || keepNulls )
		{
			destination.PushBack( castedElement );
		}
	}
}

// Not used and broken anyway
#if 0
// Serialization
	IFile& operator<<( IFile& file, THashSetPreallocated<size, K, HashFunc, EqualFunc >& hashset )
	{
		if ( file.IsReader() || file.IsWriter() )
		{
			//Uint32 size = Size();
			file << size;
		}

		/*
#ifdef RED_ENDIAN_SWAP_SUPPORT_DEPRECATED
		if ( TPlainType<T>::Value && !file.IsByteSwapping() )
#else
		if ( TPlainType<T>::Value )
#endif
		{
			// Serialize whole buffer
			if ( ar.m_size )
			{
				file.Serialize( ar.m_buf, ar.m_size * sizeof(T) );
			}
		}
		else
		{
			// Serialize each element
			for ( Uint32 i=0; i<ar.m_size; i++ )
			{
				file << ar.TypedData()[i];
			}
		}
		*/
		return file;
	}
#endif // #if 0