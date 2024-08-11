/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "object.h"
#include "objectDiscardList.h"
#include "configVar.h"

//----

CObjectsDiscardList* GObjectsDiscardList = NULL;

//---

namespace Config
{
	TConfigVar<Int32, Validation::IntRange<1,INT_MAX> > cvMaxDiscardListCount( "Memory/GC", "MaxDiscardListCount", 2 << 20, Config::eConsoleVarFlag_ReadOnly );
	TConfigVar<Int32, Validation::IntRange<1,1024> > cvAutoDiscardCount( "Memory/GC", "AutoDiscardCount", 400 );
}

//----

CObjectsDiscardList::List::List( const Uint32 maxCount )
	: m_count( 0 )
{
	m_ptr = (CObject**) RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_ObjectMap, sizeof(CObject*) * maxCount );
}

CObjectsDiscardList::List::~List()
{
	RED_MEMORY_FREE( MemoryPool_Default, MC_ObjectMap, m_ptr );
}

//----

CObjectsDiscardList::CObjectsDiscardList()
	: m_maxObjects( Config::cvMaxDiscardListCount.Get() )
	, m_pendingRequest(0)
{
	// allocate object arrays (double buffered)
	m_mainList = new List( m_maxObjects );
	m_tempList = new List( m_maxObjects );

	m_lock.SetSpinCount(100);
}

CObjectsDiscardList::~CObjectsDiscardList()
{
	delete m_mainList;
	delete m_tempList;
}

void CObjectsDiscardList::Add( CObject* object )
{
	RED_ASSERT( object != NULL );

	// non finalized objects cannot be added to the discard list
	// TODO: remove the OnFinalize - this check may be removed after that
	RED_FATAL_ASSERT( object->HasFlag( OF_Finalized ), "Trying to discard non finalized object" );

	// process only once
	if ( !object->HasFlag( OF_Discarded ))
	{
		// always mark the object as discarded
		object->SetFlag( OF_Discarded );

		{
			// TODO: we may replace this with proper double buffered lock less list
			Red::Threads::CScopedLock<Red::Threads::CMutex> lock(m_lock);

			// get list to update
			List* list = m_mainList;

			// allocate index
			const Uint32 index = list->m_count++;
			if (index < m_maxObjects)
			{
				// signal request
				if (index >= (Uint32) Config::cvAutoDiscardCount.Get())
					m_pendingRequest = 1; // ctremblay: Lock protect this variable, no need for atomics operation. 

				// write
				list->m_ptr[ index ] = object;
			}
			else if ( index == m_maxObjects )
			{
				ERR_CORE( TXT("!!! TO MANY OBJECTS IN THE DISCARD LIST !!!") );
				ERR_CORE( TXT("Please increase the object limit in the Engine.ini!!!") );
				ERR_CORE( TXT("The current limit is %d as is obviously to small"), m_maxObjects );
			}
		}
	}
}

void CObjectsDiscardList::AddNoLock( CObject* object )
{
	RED_ASSERT( object != NULL );

	// non finalized objects cannot be added to the discard list
	// TODO: remove the OnFinalize - this check may be removed after that
	RED_FATAL_ASSERT( object->HasFlag( OF_Finalized ), "Trying to discard non finalized object" );

	// process only once
	if ( !object->HasFlag( OF_Discarded ))
	{
		// always mark the object as discarded
		object->SetFlag( OF_Discarded );

		{
			// get list to update
			List* list = m_mainList;

			// allocate index
			const Uint32 index = list->m_count++;
			if (index < m_maxObjects)
			{
				// signal request
				if (index >= (Uint32) Config::cvAutoDiscardCount.Get())
					m_pendingRequest = 1; // ctremblay: Only GC call this function. No need for atomics operation.

				// write
				list->m_ptr[ index ] = object;
			}
			else if ( index == m_maxObjects )
			{
				ERR_CORE( TXT("!!! TO MANY OBJECTS IN THE DISCARD LIST !!!") );
				ERR_CORE( TXT("Please increase the object limit in the Engine.ini!!!") );
				ERR_CORE( TXT("The current limit is %d as is obviously to small"), m_maxObjects );
			}
		}
	}
}

void CObjectsDiscardList::ProcessList( const Bool forceDiscardNow /*= false*/ )
{
	if ( m_pendingRequest || forceDiscardNow )
	{
		InternalProcessDiscardList();
	}
}

void CObjectsDiscardList::Acquire()
{
	m_lock.Acquire();
}

void CObjectsDiscardList::Release()
{
	m_lock.Release();
}

//----

#ifndef RED_FINAL_BUILD

class CGCDiscardObject_Marker : public Red::Error::StackMessage
{
private:
	const CObject*		m_object;

public:
	CGCDiscardObject_Marker( const CObject* obj )
		: m_object( obj )
	{
	}

	virtual void Report( Char* outText, Uint32 outTextLength )
	{
		Int32 charsWritten = 0;
		
		charsWritten = Red::System::SNPrintF( outText, outTextLength, TXT("DiscardObject: 0x%016llX "), m_object );
		outText += charsWritten;	outTextLength -= charsWritten;

#ifdef RED_PLATFORM_WINPC
		if ( IsBadReadPtr( m_object, sizeof(CObject) ) == 0 )
		{
			charsWritten = Red::System::SNPrintF( outText, outTextLength, TXT("Index: %d "), m_object->GetObjectIndex() );
			outText += charsWritten;	outTextLength -= charsWritten;

			CClass* cls = m_object->GetLocalClass();
			if ( IsBadReadPtr( cls, sizeof(CClass) ) == 0 )
			{
				if ( cls->GetType() == RT_Class )
				{
					charsWritten = Red::System::SNPrintF( outText, outTextLength, TXT("%ls "), cls->GetName().AsChar() );
					outText += charsWritten;	outTextLength -= charsWritten;
				}
				else
				{
					charsWritten = Red::System::SNPrintF( outText, outTextLength, TXT("BAD CLS2!!") );
					outText += charsWritten;	outTextLength -= charsWritten;
				}
			}
			else
			{
				charsWritten = Red::System::SNPrintF( outText, outTextLength, TXT("BAD CLS!!") );
				outText += charsWritten;	outTextLength -= charsWritten;
			}
		}
		else
		{
			charsWritten = Red::System::SNPrintF( outText, outTextLength, TXT("BAD!!") );
			outText += charsWritten;	outTextLength -= charsWritten;
		}
#endif
	}
};

#endif

//----

void CObjectsDiscardList::InternalProcessDiscardList()
{
	// swap the lists
	List* discardList = NULL;
	{
		Red::Threads::CScopedLock<Red::Threads::CMutex> lock(m_lock);
		
		// clear flag
		m_pendingRequest = 0;

		discardList = m_mainList;
		Swap( m_mainList, m_tempList );
		m_mainList->m_count = 0; // reset list
	}

	// process the discard list
	const Uint32 numObjects = Red::Math::NumericalUtils::Min( m_maxObjects, (Uint32)discardList->m_count );
	for ( Uint32 i=0; i<numObjects; ++i )
	{
		CObject* objectToDiscard = discardList->m_ptr[i];

		// TODO: remove the OnFinalize crap....
		RED_FATAL_ASSERT( objectToDiscard->HasFlag( OF_Finalized ), "Destroying object that was not finalized" );

#ifndef RED_FINAL_BUILD
		CGCDiscardObject_Marker crash_message( objectToDiscard );
#endif

		const CClass* objectClass = objectToDiscard->GetClass();
		objectClass->DestroyObject< CObject >( objectToDiscard );
	}
}

