#pragma once

#ifndef __COOK_DATA_BASE_HELPERS__
#define __COOK_DATA_BASE_HELPERS__

namespace CookDataBaseHelper
{
	// filters resources by class
	struct PerClassFilter : public Red::NonCopyable
	{
		RED_INLINE PerClassFilter(const TDynArray< const CClass* >& classes)
			: m_allowedClassNames( classes.Size() )
		{
			for ( Uint32 i=0; i<classes.Size(); ++i )
				m_allowedClassNames[i] = classes[i]->GetName();
		}

		RED_INLINE bool operator()(const CCookerDataBase& db, const CCookerResourceEntry& entry) const
		{
			// query resource class
			CName resourceClass;
			const CCookerDataBase::TCookerDataBaseID dbId = db.GetFileEntry( entry.GetFileId() );
			if ( db.GetFileResourceClass(dbId, resourceClass ) )
			{
				for ( CName className : m_allowedClassNames )
				{
					if ( resourceClass == className )
						return true;
				}
			}

			// not allowed
			return false;
		}

	private:
		TDynArray< CName >	m_allowedClassNames;
	};

	// filters resources by extension
	struct PerExtensionFilter : public Red::NonCopyable
	{
		RED_INLINE PerExtensionFilter(const TDynArray< String >& extensions)
			: m_allowedExtensions( extensions )
		{
		}

		RED_INLINE bool operator()(const CCookerDataBase& db, const CCookerResourceEntry& entry) const
		{
			// query resource class
			CCookerResourceEntry resourceEntry;
			const CCookerDataBase::TCookerDataBaseID dbId = db.GetFileEntry( entry.GetFileId() );
			if ( db.GetFileEntry(dbId, resourceEntry) )
			{
				const CFilePath filePath( ANSI_TO_UNICODE( resourceEntry.GetFilePath().AsChar() ) ); // TODO: optmize!!, at least remove the unicode conversions
				for ( const String& allowedExtension : m_allowedExtensions )
				{
					if ( filePath.GetExtension() == allowedExtension )
						return true;
				}
			}

			// not allowed
			return false;
		}

	private:
		TDynArray< String >	m_allowedExtensions;
	};

	// collects resources into a container
	template< typename Container >
	struct FileCollector : public Red::NonCopyable
	{
		RED_INLINE FileCollector( Container& outContainer )
			: m_container( &outContainer )
		{}

		RED_INLINE bool operator()(const CCookerDataBase& db, const CCookerResourceEntry& entry)
		{
			const CCookerDataBase::TCookerDataBaseID dbId = db.GetFileEntry( entry.GetFileId() );

			if ( db != CCookerDataBase::NO_ENTRY )
				m_container->PushBack( dbId );

			return true;
		}

	private:
		Container*		m_container;
	};

	// collects files modulo N (with offset)
	template< typename Container >
	struct FileCollectorModulo : public Red::NonCopyable
	{
		RED_INLINE FileCollectorModulo( Container& outContainer, const Uint32 modulo, const Uint32 offset )
			: m_container( &outContainer )
			, m_modulo( modulo )
			, m_offset( offset )
			, m_fileIndex( 0 )
		{}

		RED_INLINE bool operator()(const CCookerDataBase& db, const CCookerResourceEntry& entry)
		{
			const CCookerDataBase::TCookerDataBaseID dbId = db.GetFileEntry( entry.GetFileId() );
			if ( dbId != CCookerDataBase::NO_ENTRY )
			{
				if ( (m_fileIndex % m_modulo) == m_offset)
					m_container->PushBack( dbId );

				m_fileIndex += 1;
			}

			return true;
		}

	private:
		Container*		m_container;
		Uint32			m_modulo;
		Uint32			m_offset;
		Uint32			m_fileIndex;
	};

} // CookDataBaseHelper

#endif