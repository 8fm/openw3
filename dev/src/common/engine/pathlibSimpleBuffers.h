/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once


class CSimpleBufferWriter : public Red::System::NonCopyable
{
public:
	CSimpleBufferWriter( TDynArray< Int8 >& data, Uint16 fileVersion )
		: m_data( data )													{ Put( fileVersion ); }

	template< class T >
	RED_INLINE void Put( const T& val )
	{
		Write( &val, sizeof( T ) );
	}
	template< class T >
	RED_INLINE void Pop( const T& val )
	{
		m_data.ResizeFast( m_data.Size() - sizeof( T ) );
	}
	template< class T >
	RED_INLINE void SmartPut( const T& val )								{ RED_HALT( "Type not implemented" ); }
	template< class T, EMemoryClass memoryClass >
	RED_INLINE void SmartPut( const TDynArray< T, memoryClass >& val )
	{
		ASSERT( val.Size() <= 0xffff );
		Uint16 count = Uint16( val.Size() );
		Put( count );
		if ( count )
		{
			Write( val.TypedData(), static_cast< Uint32 >( val.DataSize() ) );
		}
	}
	template < class T, class CompareFunc >
	RED_INLINE void SmartPut( const TSortedArray< T, CompareFunc >& val )
	{
		SmartPut( static_cast< const TDynArray< T >& >( val ) );
	}

	RED_INLINE void Write( const void* dataPtr, Uint32 dataSize )
	{
		Uint32 prevSize = m_data.Size();
		Uint32 newSize = prevSize + dataSize;
		if ( newSize > m_data.Capacity() )
		{
			m_data.Reserve( Max( 128U, newSize*2 ) );
		}
		m_data.ResizeFast( newSize );
		Red::System::MemoryCopy( &m_data[ prevSize ], dataPtr, dataSize );
	}

	template < class T >
	RED_INLINE Uint32 ReserveSpace( const T& val )
	{
		Uint32 place = m_data.Size();
		Put( val );
		return place;
	}
	template < class T >
	RED_INLINE void PutReserved( const T& val, Uint32 place )
	{
		Red::System::MemoryCopy( &m_data[ place ], &val, sizeof( T ) );
	}
	RED_INLINE Uint32 GetCurrentPosition()
	{
		return m_data.Size();
	}
protected:
	TDynArray< Int8 >&				m_data;
};

class CSimpleBufferReader : public Red::System::NonCopyable
{
public:
	CSimpleBufferReader( const TDynArray< Int8 >& data )
		: m_data( data.TypedData() )
		, m_dataSize( data.Size() )
		, m_currentPos( 0 )												{ if ( !Get( m_fileVersion ) ) m_fileVersion = 0xffff; }

	CSimpleBufferReader( const Int8* data, Uint32 dataSize )
		: m_data( data )
		, m_dataSize( dataSize )
		, m_currentPos( 0 )												{ if ( !Get( m_fileVersion ) ) m_fileVersion = 0xffff; }

	template< class T >
	RED_INLINE Bool Get( T& val )
	{
		return Read( &val, sizeof( T ) );
	}
	template< class T >
	RED_INLINE Bool SmartGet( T& val )									{ RED_HALT( "Type not implemented" ); }
	template< class T, EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
	RED_INLINE Bool SmartGet( TDynArray< T, memoryClass, memoryPool >& val )
	{
		ASSERT( val.Size() <= 0xffff );
		Uint16 count;
		if ( !Get( count ) )
			return false;
		val.Resize( count );
		if ( count )
		{
			if( !Read( val.TypedData(), static_cast< Uint32 >( val.DataSize() ) ) )
				return false;
		}

		return true;
	}
	template < class T, class CompareFunc >
	RED_INLINE Bool SmartGet( TSortedArray< T, CompareFunc >& val )
	{
		return SmartGet( static_cast< TDynArray< T >& >( val ) );
	}

	RED_INLINE Bool Read( void* dataPtr, Uint32 dataSize )
	{
		if ( m_currentPos + dataSize > m_dataSize )
		{
			return false;
		}
		Red::System::MemoryCopy( dataPtr, &m_data[ m_currentPos ], dataSize );
		m_currentPos += dataSize;
		return true;
	}
	RED_INLINE Bool Skip( Uint32 dataSize )
	{
		if ( m_currentPos + dataSize > m_dataSize )
		{
			return false;
		}
		m_currentPos += dataSize;
		return true;
	}
	RED_INLINE Uint16 GetVersion() const							{ return m_fileVersion; }
	RED_INLINE Uint32 GetCurrentPos() const							{ return m_currentPos; }
	RED_INLINE void SetCurrentPos( Uint32 pos )						{ m_currentPos = pos; }

protected:
	RED_INLINE const void* GetCurrentDataPtr() const				{ return &m_data[ m_currentPos ]; }

	const Int8* const		m_data;
	const Uint32			m_dataSize;

	Uint32					m_currentPos;
	Uint16					m_fileVersion;
};


template<>
RED_INLINE void CSimpleBufferWriter::SmartPut< String >( const String& val )
{
	SmartPut( static_cast< const String::BaseClass& >( val ) );
}	
template<>
RED_INLINE Bool CSimpleBufferReader::SmartGet< String >( String& val )
{
	return SmartGet( static_cast< String::BaseClass& >( val ) );
}
