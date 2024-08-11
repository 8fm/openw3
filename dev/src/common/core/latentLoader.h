/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/


#pragma once

#include "fileSkipableBlock.h"
#include "fileLatentLoadingToken.h"

template < typename T > class TLatentLoader
{
protected:
	mutable IFileLatentLoadingToken*	m_token;
	mutable T*							m_data;

public:
	RED_INLINE TLatentLoader()
		: m_data( NULL )
		, m_token( NULL )
	{};

	RED_INLINE ~TLatentLoader()
	{
		delete m_data;
		delete m_token;
	};

	RED_INLINE Bool IsLoaded() const
	{
		return m_data != NULL;
	}

	RED_INLINE Bool IsLinked() const
	{
		return m_token != NULL;
	}

	RED_INLINE T& GetAndUnlink()
	{
		Load();
		Unlink();
		return *m_data;
	}

	RED_INLINE const T& GetConst() const
	{
		Load();
		return *m_data;
	}

	RED_INLINE void Set( const T& data )
	{
		Clear();

		m_data = new T( data );
	}

	RED_INLINE void UnloadIfYouCan() const
	{
		if ( m_token )
		{
			Unload();
		}
	}

	friend IFile& operator<<( IFile& file, TLatentLoader<T>& buf )
	{
		buf.Serialize( file );
		return file;
	}

protected:
	void Clear()
	{
		if ( m_data )
		{
			delete m_data;
			m_data = NULL;
		}

		if ( m_token )
		{
			delete m_token;
			m_token = NULL;
		}
	}

	void Unlink()
	{
		if ( m_token )
		{
			ASSERT( m_data );

			delete m_token;
			m_token = NULL;
		}
	}

	void Load() const
	{
		if ( !m_data )
		{
			ASSERT( m_token );

			IFile* file = m_token->Resume( 0 );

			// Create and load the crap
			m_data = new T();
			
			{
				CFileSkipableBlock block( *file );
				*file << *m_data;
			}

			delete file;
		}
	}

	void Unload() const
	{
		if ( m_data )
		{
			ASSERT( m_token );
			if ( m_token )
			{
				delete m_data;
				m_data = NULL;
			}
		}
	}

	void Serialize( IFile& file )
	{
		if ( file.IsReader() )
		{
			// Clear current data
			Clear();

			// Resume loading
			m_token = file.CreateLatentLoadingToken( file.GetOffset() );

			// Load data
			{
				CFileSkipableBlock block( file );

				if ( m_token )
				{
					// We can read the data later
					block.Skip();
				}
				else
				{
					// Load data now, we cannot load it later
					m_data = new T();
					file << *m_data;
				}
			}
		}
		else if ( file.IsWriter() )
		{
			// Make sure data is loaded
			Load();

			// If we are not saving to the same file we were loaded from we need to unlink the data
			Unlink();

			// Once saved to resource restore the latent loading token
			if ( file.IsResourceResave() )
			{
				// Restore token to load the data back
				m_token = file.CreateLatentLoadingToken( file.GetOffset() );
			}

			// Save the data
			{
				CFileSkipableBlock block( file );

				ASSERT( m_data );
				file << *m_data;
			}

			// If we have the token to load the data back we can unload it from memory
			if ( m_token )
			{
				Unload();
			}
		}
	}
};

