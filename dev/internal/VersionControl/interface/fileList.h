/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __VERCON_FILE_LIST_H__
#define __VERCON_FILE_LIST_H__

#include <new>

#include "export.h"

namespace VersionControl
{
	class Filelist
	{
	public:
		Filelist();
		~Filelist();

		virtual bool Add( const char* file ) final;

		virtual const char** Get();
		unsigned int Size() const { return m_size; }

		void* operator new( size_t size );
		void operator delete( void* ptr );

	private:
		struct Item
		{
			char* file;
			Item* next;

			void* operator new( size_t size );
			void operator delete( void* ptr );
			
			Item( const char* file );
			~Item();
		};

	private:
		const char** m_output;
		
		Item* m_start;
		Item* m_end;
		unsigned int m_size;
	};
}

#endif // __VERCON_FILE_LIST_H__
