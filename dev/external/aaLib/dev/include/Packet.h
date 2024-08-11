
#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <cassert>

typedef unsigned int		aaUint32;
typedef unsigned char		aaUint8;
typedef int					aaInt32;
typedef char				aaInt8;

class aaPacket
{
private:
	void*				m_buf;
	aaUint32			m_size;
	mutable aaUint32	m_offset;

public:
	aaPacket();
	aaPacket( const aaPacket& buffer );
	~aaPacket();

	void* GetData() const;
	aaUint32 GetSize() const;

	void Reserve( aaUint32 size );
	void Rewind();

public:
	void WriteBuffer( const void* data, aaUint32 size );
	void ReadBuffer( void* data, aaUint32 size ) const;

	void WriteString( const char* data );
	void WriteWString( const wchar_t* data );
	void ReadString( char* data );

	template < typename T >
	void Read( T* data )
	{
		ReadBuffer( data, sizeof( T ) );
	}

	template < typename T >
	void Write( const T* data )
	{
		WriteBuffer( data, sizeof( T ) );
	}

	template < typename T >
	void ReadArray( T* data, aaUint32 size )
	{
		aaUint32 byteSize = size * sizeof( T );
		ReadBuffer( data, byteSize );
	}

	template < typename T >
	void WriteArray( const T* data, aaUint32 size )
	{
		aaUint32 byteSize = size * sizeof( T );
		WriteBuffer( data, byteSize );
	}

private:
	aaUint32 Grow( aaUint32 size );
};
