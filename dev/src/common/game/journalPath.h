/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#ifndef __JOURNAL_PATH_H__
#define __JOURNAL_PATH_H__

#pragma once

#include "journalBase.h"
#include "journalResource.h"
#include "../core/diskFile.h"

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

#ifdef NO_EDITOR
#	define JOURNAL_PATH_PRECACHE_RESOURCES
#endif

class CJournalPath : public ISerializable
{
	DECLARE_RTTI_SIMPLE_CLASS( CJournalPath );

private:
	CGUID						m_guid;
	TSoftHandle< CResource >	m_resource;
	THandle< CJournalPath >		m_child;

	static THashMap< CGUID, THandle< CJournalPath > >		g_cache;
	static TSortedMap< CGUID, THandle< CJournalResource > > g_resources;

	enum ESaveFlags
	{
		FLAG_Nothing = 0,
		FLAG_NoMoreChildren = FLAG( 0 ),
	};

public:
	CJournalPath();
	virtual ~CJournalPath();

	virtual String GetFriendlyName() const { return GetPathAsString(); }

	static THandle< CJournalPath > ConstructPathFromTargetEntry( CJournalBase* entry, CJournalResource* resource = nullptr, Bool forceCreation = false );

#ifdef JOURNAL_PATH_PRECACHE_RESOURCES
public:
	// Game side journal resource pre-loading and caching
	static Bool LoadResources( const Char* journalDepotPath );
	static Bool UpdateResources( const Char* journalDepotPath );
	static void ClearResources();

private:
	static Uint32 CountResources( const CDirectory* directory );
	static void LoadResources( const CDirectory* directory );
	static void ConstructPathFromTargetEntryInternalGame( THandle< CJournalPath >& path, CJournalChildBase* parent );

#else
private:
	// Editor side load-on-demand
	static void ConstructPathFromTargetEntryInternalEditor( THandle< CJournalPath >& path, CJournalResource* parentObjectResource, CJournalChildBase* parent );
#endif

private:
	static THandle< CJournalPath > ConstructPathFromTargetEntryInternal( CJournalBase* entry, CJournalResource* resource = nullptr, THandle< CJournalPath > child = nullptr );

	void SaveGameRecursively( IGameSaver* saver );
	static CJournalPath* LoadGameRecursively( IGameLoader* loader, CGUID& targetEntryGUID, Uint32 recursionLevel );
	static CJournalPath* Create( const CGUID& guid, const String& res, CJournalPath* child );

public:
	void SaveGame( IGameSaver* saver );	
	static CJournalPath* LoadGame( IGameLoader* loader );
	static void ClearCache() { g_cache.Clear(); }

	RED_INLINE String GetPathAsString() const
	{
		String path;

		for( const_iterator iter = Begin(); iter != End(); ++iter )
		{
			path += iter->GetName();
			path += TXT( "/" );
		}

		return path;
	}

	RED_INLINE const CJournalBase* GetTarget() const
	{
		const CJournalBase* target = NULL;

		for( const_iterator iter = Begin(); iter != End(); ++iter )
		{
			target = *iter;
		}

		return target;
	}

	RED_INLINE CJournalBase* GetTarget()
	{
		return const_cast< CJournalBase* >( static_cast< const CJournalPath* >( this )->GetTarget() );
	}

	template< typename TClass >
	const TClass* GetTargetAs() const
	{
		const CJournalBase* target = GetTarget();
		if( target )
		{
			ASSERT( target->IsA< TClass >(), TXT( "'%ls' is a %s and cannot be cast to a %s" ), target->GetName().AsChar(), target->GetClass()->GetName().AsString().AsChar(), ClassID< TClass >()->GetName().AsString().AsChar() );
			return static_cast< const TClass* >( target );
		}

		return NULL;
	}

	template< typename TClass >
	RED_INLINE TClass* GetTargetAs()
	{
		return const_cast< TClass* >( static_cast< const CJournalPath* >( this )->GetTargetAs< TClass >() );
	}

	template< typename TClass >
	const RED_INLINE TClass* GetEntryAs() const
	{
		const TClass* entry = nullptr;
		for( const_iterator iter = Begin(); iter != End(); ++iter )
		{
			entry = Cast< TClass >( *iter );
			if ( entry )
			{
				return entry;
			}
		}
		return nullptr;
	}

	template< typename TClass >
	RED_INLINE TClass* GetEntryAs()
	{
		TClass* entry = nullptr;
		for( iterator iter = Begin(); iter != End(); ++iter )
		{
			entry = Cast< TClass >( *iter );
			if ( entry )
			{
				return entry;
			}
		}
		return nullptr;
	}


	Bool IsValid() const;

	String GetFilePaths() const
	{
		String path = m_resource.GetPath();

		if( m_child && !m_resource.IsEmpty() )
		{
			path += TXT( ", " ) + m_child->GetFilePaths();
		}

		return path;
	}

public:
	template< typename TPosition, typename TEntry, typename TJournalContainer, typename TJournalContainerEntry, typename TJournalResource >
	class CIterator
	{
	public:
		CIterator( TPosition* path )
		:	m_currentPosition( NULL ),
			m_currentEntry( NULL )
		{
			UpdatePosition( path );
		}

		~CIterator() {};

		// Pre increment
		void operator++()
		{
			ASSERT( m_currentPosition != NULL );
			UpdatePosition( m_currentPosition->m_child.Get() );
		}

		TEntry* operator*()
		{
			return m_currentEntry;
		}

		TEntry* operator->()
		{
			return m_currentEntry;
		}

		Bool operator!=( const CIterator& other ) const
		{
			return m_currentEntry != other.m_currentEntry;
		}

		Bool IsLastItem() const
		{
			return !m_currentPosition || !( m_currentPosition->m_child );
		}

		TPosition* GetCurrentPosition() const { return m_currentPosition; }

	private:
		void UpdatePosition( TPosition* path )
		{
			if( path )
			{
				if( path->m_resource.IsEmpty() || ( m_currentPosition && m_currentPosition->m_resource == path->m_resource ) )
				{
					ASSERT( m_currentEntry && m_currentEntry->template IsA< TJournalContainer >() );

					if ( m_currentEntry && m_currentEntry->template IsA< TJournalContainer >() )
					{
						TJournalContainer* parentContainer = static_cast< TJournalContainer* >( m_currentEntry );

						for( Uint32 i = 0; i < parentContainer->GetNumChildren(); ++i )
						{
							TJournalContainerEntry* entry = parentContainer->GetChild( i );
#ifndef NO_EDITOR
							// Emergency surgery, this only occurs when entry class definitions are deleted in code
							if( entry == NULL )
							{
								continue;
							}
#else
							ASSERT
							(
								entry != NULL,
								TXT( "It looks like a type of journal entry has been deleted in code," )
								TXT( "but the parent entries have not been updated. Simply opening the " )
								TXT( "journal editor should fix this. The offending entry is: '%ls' (%s)" ),
								parentContainer->GetName().AsChar(),
								parentContainer->GetClass()->GetName().AsString().AsChar()
							);
#endif
							if( path->m_guid == entry->GetGUID() )
							{
								m_currentEntry = entry;
								break;
							}
						}
					}
				}
				else
				{
					ASSERT( !path->m_resource.IsEmpty() );

					CResource* resourceBase = path->m_resource.Get();

					if( resourceBase != NULL )
					{
#ifndef NO_EDITOR
						ASSERT
						(
							resourceBase->IsA< CJournalResource >(),
							TXT( "Resource '%ls' (%s) is not a CJournalResource!" ),
							resourceBase->GetFile() ? resourceBase->GetFile()->GetFileName().AsChar() : TXT( "No File" ),
							resourceBase->GetFriendlyDescription()
						);
#endif
						TJournalResource* resource = static_cast< TJournalResource* >( resourceBase );

						m_currentEntry = resource->Get();
					}
					else
					{
						// Must be an invalid entry
						m_currentEntry = NULL;
					}
				}
			}
			else
			{
				m_currentEntry = NULL;
			}

			m_currentPosition = path;
		}

		TPosition* m_currentPosition;
		TEntry* m_currentEntry;
	};

	typedef CIterator< CJournalPath, CJournalBase, CJournalContainer, CJournalContainerEntry, CJournalResource > iterator;
	typedef CIterator< const CJournalPath, const CJournalBase, const CJournalContainer, const CJournalContainerEntry, const CJournalResource > const_iterator;
	friend iterator;
	friend const_iterator;

	iterator Begin() { return iterator( this ); }
	iterator End() { return iterator( NULL ); }

	const_iterator Begin() const { return const_iterator( this ); }
	const const_iterator End() const { return const_iterator( NULL ); }

	void DebugPrintTree( Uint32 recursionLevel = 0 );

private:
	virtual void OnPreSave();

	virtual void OnPostLoad();

	virtual Bool OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue );

	// CJournalPath is reference counted and accessed through handles
	virtual const Bool CanIgnoreInGC() const override final { return true; }
};

BEGIN_CLASS_RTTI( CJournalPath )
	PARENT_CLASS( ISerializable )
	PROPERTY_RO_SAVED( m_guid, TXT( "GUID" ) )
	PROPERTY_RO_SAVED( m_resource, TXT( "Resource" ) )
	PROPERTY_RO_SAVED( m_child, TXT( "Child" ) )
END_CLASS_RTTI()

#endif // __JOURNAL_PATH_H__
