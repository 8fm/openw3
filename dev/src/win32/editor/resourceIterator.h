
#pragma once

#include "../../common/core/feedback.h"
#include "../../common/core/objectDiscardList.h"

class CResource;

template < typename ResourceT >
inline void GetAllResourceFilesInDirectory( CDirectory* dir, TDynArray< CDiskFile* >& files, Bool isExactly = false )
{
	ASSERT( dir );

	for ( CDirectory* child : dir->GetDirectories() )
	{
		GetAllResourceFilesInDirectory< ResourceT >( child, files, isExactly );
	}

	for ( CDiskFile* file : dir->GetFiles() )
	{
		if ( const CClass* resClass = file->GetResourceClass() )
		{
			if ( isExactly && resClass == ClassID< ResourceT >() )
			{
				files.PushBack( file );
			}
			else if ( !isExactly && resClass->IsA< ResourceT >() )
			{
				files.PushBack( file );
			}
		}
	}
}

enum EResourceIteratorFlags
{
	RIF_Default   = 0,
	RIF_IsExactly = FLAG(0),
	RIF_ReadOnly  = FLAG(1),
	RIF_NoCancel  = FLAG(2)
};

//! Iterates through all resources of given type in directories or set of files, giving access to fully loaded resource.
//! Keeps the memory footprint low unloading resources when processed (if possible).
template < typename ResourceT >
class CResourceIterator
{
public:

	CResourceIterator( CDirectory* dir, const String& taskName = String::EMPTY, EResourceIteratorFlags flags = RIF_Default )
		: m_flags( flags )
		, m_taskName( taskName )
	{
		m_dirs.PushBack( dir );
		Init();
	}

	CResourceIterator( TDynArray< CDirectory* > dirs, const String& taskName = String::EMPTY, EResourceIteratorFlags flags = RIF_Default)
		: m_dirs( dirs )
		, m_flags( flags )
		, m_taskName( taskName )
	{
		Init();
	}

	CResourceIterator( TDynArray< CDiskFile* > files, const String& taskName = String::EMPTY, EResourceIteratorFlags flags = RIF_Default)
		: m_flags( flags )
		, m_taskName( taskName )
	{
		m_files = files;
		Init();
	}

	~CResourceIterator() 
	{
		CleanUp();
	}

	operator Bool() const
	{
		return !m_ended;
	}

	ResourceT& operator * ()
	{
		ASSERT( !m_ended && m_curRes.IsValid() );
		return *m_curRes;
	}

	ResourceT* operator -> ()
	{
		ASSERT( !m_ended && m_curRes.IsValid() );
		return m_curRes.Get();
	}

	ResourceT* Get()
	{
		ASSERT( !m_ended && m_curRes.IsValid() );
		return m_curRes.Get();
	}

	void operator ++ ()
	{
		Unlock();

		if ( m_ended )
		{
			return;
		}

		if ( !m_taskName.Empty() && GFeedback->IsTaskCanceled() )
		{
			m_ended = true;
			return;
		}

		do 
		{
			if ( ++m_filesCursor == m_files.End() )
			{
				if ( !m_dirs.Empty() )
				{
					++m_dirCursor;
					ReadDirectory();
				}
				else
				{
					m_ended = true;
				}

				if ( m_ended )
				{
					break;
				}
			}
		}
		while ( !Lock() );

		UpdateProgress();
	}

private:

	void Init()
	{
		m_ended = false;
		m_needsUnload = false;

		if ( !m_taskName.Empty() )
		{
			GFeedback->BeginTask( m_taskName.AsChar(), !(m_flags & RIF_NoCancel) );
		}

		if ( m_dirs.Empty() )
		{
			if ( m_files.Empty() )
			{
				m_ended = true;
			}
			else
			{
				m_filesCursor = m_files.Begin();
			}
		}
		else
		{
			m_dirCursor = m_dirs.Begin();
			ReadDirectory();
		}

		if ( !Lock() )
		{ // if we cannot lock the first resource, try to find the first lockable
			operator ++();
		}
	}

	void CleanUp()
	{
		Unlock();

		if ( !m_taskName.Empty() )
		{
			GFeedback->EndTask();
		}

		if ( !m_failedFiles.Empty() )
		{
			String msg = TXT("Following files failed to be processed (locked?):\n");
			for ( const CDiskFile* file : m_failedFiles )
			{
				msg += file->GetDepotPath() + TXT("\n");
			}
			GFeedback->ShowWarn( msg.AsChar() );
		}
	}

	Bool Lock()
	{
		if ( m_ended )
		{
			return false;
		}

		if ( CDiskFile* file = *m_filesCursor )
		{
			if ( ( m_flags & RIF_ReadOnly ) || PrepareToEdit( file ) )
			{
				if ( !m_taskName.Empty() )
				{
					GFeedback->UpdateTaskInfo( file->GetDepotPath().AsChar() );
				}

				if ( file->IsLoaded() )
				{
					m_needsUnload = false;
				}
				else
				{
					file->Load();
					ASSERT( file->IsLoaded() );
					m_needsUnload = true;
				}

				if ( file->IsLoaded() )
				{
					m_curRes = SafeCast< ResourceT >( file->GetResource() );
					return true;
				}
			}
		}

		return false;
	}

	void Unlock()
	{
		if ( m_curRes.IsValid() )
		{
			if ( !( m_flags & RIF_ReadOnly ) )
			{
				m_curRes->Save();
			}

			if ( m_needsUnload )
			{
				m_curRes->GetFile()->Unload();
				m_needsUnload = false;
			}

			m_curRes = nullptr;
		}
		// discard destroyed objects - otherwise we're gonna get ooms
		GObjectsDiscardList->ProcessList(true);
	}

	void ReadDirectory()
	{
		ASSERT ( !m_dirs.Empty() );

		for ( ;; )
		{
			if ( m_dirCursor == m_dirs.End() )
			{
				m_ended = true;
				break;
			}

			m_files.Clear();
			GetAllResourceFilesInDirectory< ResourceT >( *m_dirCursor, m_files, m_flags & RIF_IsExactly );

			if ( !m_files.Empty() )
			{
				m_filesCursor = m_files.Begin();
				break;
			}
			else
			{
				++m_dirCursor;
			}
		}
	}

	Bool PrepareToEdit( CDiskFile* file )
	{
		Bool result = false;

		if ( file )
		{
			file->GetStatus(); // force to update the p4 status

			if ( file->IsLocal() || file->IsCheckedOut() )
			{
				result = file->MarkModified();
			}
			else
			{
				if ( file->SilentCheckOut() )
				{
					result = file->MarkModified();
				}
				else
				{
					result = false;
				}
			}

			if ( !result )
			{
				m_failedFiles.PushBack( file );
			}
		}

		return result;
	}

	void UpdateProgress()
	{
		if ( m_taskName.Empty() )
		{
			return;
		}

		if ( !m_dirs.Empty() )
		{
			// This calculation assumes that each processed dir contains the same number of files, which is not true
			// in most cases, but it's the only sane assumption possible without populating all the dirs upfront
			Uint32 total = 100 * m_dirs.Size(); 
			Uint32 cur = 100 * ( m_dirCursor - m_dirs.Begin() ) + 100 * ( m_filesCursor - m_files.Begin() ) / m_files.Size();
			GFeedback->UpdateTaskProgress( cur, total );
		}
		else
		{
			Uint32 cur = 100 * ( m_filesCursor - m_files.Begin() ) / m_files.Size();
			GFeedback->UpdateTaskProgress( cur, 100 );
		}
	}


	EResourceIteratorFlags m_flags;
	String m_taskName;
	Bool m_ended;

	TDynArray< CDirectory* > m_dirs;
	TDynArray< CDirectory* >::iterator m_dirCursor;

	TDynArray< CDiskFile* > m_files;
	TDynArray< CDiskFile* >::iterator m_filesCursor;

	TDynArray< CDiskFile* > m_failedFiles;

	THandle< ResourceT > m_curRes;
	Bool m_needsUnload;
};
