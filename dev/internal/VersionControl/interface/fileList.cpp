/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "fileList.h"
#include "allocator.h"

#include <cstring>

namespace VersionControl
{
	Filelist::Filelist()
	:	m_output( nullptr )
	,	m_start( nullptr )
	,	m_end( nullptr )
	,	m_size( 0 )
	{

	}

	Filelist::~Filelist()
	{
		if( m_output )
		{
			Allocator::Free( m_output );
			m_output = nullptr;
		}

		Item* current = m_start;
		unsigned int index = 0;

		while( current )
		{
			Item* prev = current;
			current = current->next;

			delete prev;
		}

		m_start = nullptr;
		m_end = nullptr;
		m_size = 0;
	}

	bool Filelist::Add( const char* file )
	{
		if( !m_output )
		{
			Item* item = new Item( file );

			if( !m_start )
			{
				m_start = item;
			}

			if( m_end )
			{
				m_end->next = item;
			}

			m_end = item;

			++m_size;

			return true;
		}

		return false;
	}

	const char** Filelist::Get()
	{
		if( !m_output )
		{
			void* output = Allocator::Malloc( m_size * sizeof( const char* ) );

			m_output = static_cast< const char** >( output );

			Item* current = m_start;
			unsigned int index = 0;

			while( current )
			{
				m_output[ index ] = current->file;
				++index;

				current = current->next;
			}
		}

		return m_output;
	}

	void* Filelist::operator new( size_t size )
	{
		return Allocator::Malloc( size );
	}

	void Filelist::operator delete( void* ptr )
	{
		return Allocator::Free( ptr );
	}

	//////////////////////////////////////////////////////////////////////////

	Filelist::Item::Item( const char* file_in )
	:	file( nullptr )
	,	next( nullptr )
	{
		size_t size = std::strlen( file_in ) + 1;
		file = static_cast< char* >( Allocator::Malloc( size * sizeof( char ) ) );
		::strncpy_s( file, size, file_in, _TRUNCATE );
	}

	Filelist::Item::~Item()
	{
		Allocator::Free( file );
		
		file = nullptr;
		next = nullptr;
	}

	void* Filelist::Item::operator new( size_t size )
	{
		return Allocator::Malloc( size );
	}

	void Filelist::Item::operator delete( void* ptr )
	{
		return Allocator::Free( ptr );
	}
}
