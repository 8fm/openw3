/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "object.h"
#include "objectMap.h"
#include "objectIterator.h"
#include "objectRootSet.h"
#include "file.h"
#include "loadingJobManager.h"
#include "memoryFileAnalizer.h"
#include "softHandle.h"

class CReferenceReplacer : public IFile
{
protected:
	void*		m_replace;
	void*		m_replaceWith;
	Uint32		m_numReplaced;

public:
	RED_INLINE CReferenceReplacer( void* pointer, void* replaceWith )
		: IFile( FF_Writer | ( GCNameAsNumberSerialization ? FF_HashNames : 0 ) )
		, m_replace( pointer )
		, m_replaceWith( replaceWith )
		, m_numReplaced( 0 )
	{};

	// Get number of references replaced
	RED_INLINE Int32 GetNumReplaced() const { return m_numReplaced; }

	// Pointer serialization
	virtual void SerializePointer( const class CClass* pointerClass, void*& pointer )
	{
		RED_UNUSED( pointerClass );

		if ( pointer == m_replace )
		{
			pointer = m_replaceWith;
			m_numReplaced++;
		}
	}

protected:
	// Fake IFile interface
	virtual void Serialize( void*, size_t ) {}
	virtual Uint64 GetOffset() const { return 0; };
	virtual Uint64 GetSize() const { return 0; }
	virtual void Seek( Int64 ) {};
};

Bool CObject::IsValidObject( CObject* object )
{
	// NULL object is valid
	if ( !object )
	{
		return true;
	}

	// Get object index
	Uint32 objectIndex = object->GetObjectIndex();
	if ( objectIndex >= GObjectsMap->GetMaxObjectIndex() )
	{
		// Object index out of bounds
		return false;
	}

	// Check vtable
	//RED_MESSAGE("64B: test is iffy. E.g., not 0xFEEEFEEE or 0xFEEEFEEEFEEEFEEEui64?")
	uintptr_t ptrVtable = ( ( uintptr_t* ) object )[ 0 ];
#ifndef RED_ARCH_X64
	if ( ptrVtable == 0xEDEDEDED || ptrVtable == 0xCDCDCDCD )
	{
		// vtable seems to be invalid
		return false;
	}
#else
	if ( ptrVtable == 0xEDEDEDEDEDEDEDEDULL || ptrVtable == 0xCDCDCDCDCDCDCDCDULL )
	{
		// vtable seems to be invalid
		return false;
	}
#endif

	// Check mapping
	/*if ( SObjectsMap::GetInstance().m_allObjects[ object->GetIndex() ] != object )
	{
		// Object mapping is invalid
		return false;
	}
	*/
	// Seems to be valid
	return true;
}

Uint32 CObject::ReplaceReferences( CObject* object, CObject* replaceWith )
{
	// Replace referenced in all objects
	TDynArray< CObject* > objectToReplace;
	objectToReplace.Reserve( 200000 );
	for ( BaseObjectIterator it; it; ++it )
	{
		objectToReplace.PushBack( *it );
	}

	CReferenceReplacer replacer( object, replaceWith );
	for ( CObject* object : objectToReplace )
	{
		object->OnSerialize( replacer );
	}

	// Return number of replaced references
	return replacer.GetNumReplaced();
}

#if !defined(NO_DEBUG_PAGES) && !defined(NO_LOG)

void CObject::DebugDumpList()
{
	// Dump list
	Uint32 numActiveObjects=0;
	Uint32 numRootObjects=0;
	Uint32 numEmptySlots=0;
	for ( BaseObjectIterator it; it; ++it )
	{
		CObject* object = *it;
		String text;

		if ( object )
		{
			numActiveObjects++;

			// Flags
			if ( object->HasFlag( OF_Root ))
			{
				text = TXT("ROOT ");
				numRootObjects++;
			}

			// Class
			if ( object->GetClass() )
			{
				text += object->GetClass()->GetName().AsString();
				text += TXT(" ");
			}
			
			// Parent
			if ( object->m_parent )
			{
				text += String::Printf( TXT("[Parent: %d] "), object->GetParent()->GetObjectIndex() );
			}
		}
		else
		{
			numEmptySlots++;
			text = TXT("EMPTY ");
		}

		LOG_CORE( TXT("[%04d]: %s"), object->GetObjectIndex(), text.AsChar() );
	}
	LOG_CORE( TXT("%i active objects, %i roots"), numActiveObjects, numRootObjects );
}

struct ClassInfo
{
	const CClass*				m_class;
	ClassInfo*					m_parent;
	Bool						m_isScripted;
	Bool						m_hasScriptedChildren;
	TDynArray< ClassInfo* >		m_children;
	Uint32						m_depth;
	Uint32						m_memoryInclusive;
	Uint32						m_countInclusive;
	Uint32						m_memoryExclusive;
	Uint32						m_countExclusive;

	ClassInfo( const CClass* info, ClassInfo* parentInfo )
		: m_class( info )
		, m_parent( parentInfo )
		, m_depth( parentInfo ? parentInfo->m_depth + 1 : 0 )
		, m_memoryInclusive( 0 )
		, m_countInclusive( 0 )
		, m_memoryExclusive( 0 )
		, m_countExclusive( 0 )
		, m_isScripted( info->IsScripted() )
		, m_hasScriptedChildren( false )
	{
		if ( m_parent != NULL )
		{
			m_parent->m_children.PushBack( this );
		}

		if ( m_isScripted )
		{
			ClassInfo* cur = m_parent;
			while ( cur )
			{
				cur->m_hasScriptedChildren |= 1;
				cur = cur->m_parent;
			}
		}
	}

	~ClassInfo()
	{
		m_children.ClearPtr();
	}

	static Int32 SortFunc( const void* a, const void* b )
	{
		const ClassInfo* dataA = *(const ClassInfo**)a;
		const ClassInfo* dataB = *(const ClassInfo**)b;
		//return Red::System::StringCompare( dataA->m_class->GetName().AsString().AsChar(), dataB->m_class->GetName().AsString().AsChar() );
		return dataB->m_countInclusive - dataA->m_countInclusive;
	}

	void Print( const Uint32 depth )
	{
		Char leadText[ 256 ];
		for ( Uint32 i=0; i<depth; ++i )
		{
			leadText[i] = ' ';
		}
		leadText[depth] = 0;

		if ( m_countExclusive > 0 )
		{
			LOG_CORE( TXT("%s%d: %s - %1.2fKB, EXC( #%d, MEM: %1.2fKB ) %s"), 
				leadText, 
				m_countInclusive, 
				m_class->GetName().AsChar(),
				m_memoryInclusive / 1024.0f,
				m_countExclusive, 
				m_countExclusive / 1024.0f,
				(m_isScripted | m_hasScriptedChildren) ? TXT("S") : TXT(""));
		}
		else
		{
			LOG_CORE( TXT("%s%d: %s - %1.2fKB, EXC( #%d, MEM: %1.2fKB ) %s"), 
				leadText, 
				m_countInclusive, 
				m_class->GetName().AsChar(),
				m_memoryInclusive / 1024.0f,
				m_countExclusive, 
				m_countExclusive / 1024.0f,
				(m_isScripted | m_hasScriptedChildren) ? TXT("S") : TXT(""));
		}

		qsort( m_children.TypedData(), m_children.Size(), sizeof( ClassInfo* ), ClassInfo::SortFunc );

		for ( Uint32 i=0; i<m_children.Size(); ++i )
		{
			m_children[i]->Print( depth + 4 );
		}
	}

	static ClassInfo* GetClassInfoForRTTIClass( const CClass* rttiClass, ClassInfo* rootClass )
	{
		// we got the CObject class
		if ( rttiClass == ClassID< CObject >() )
		{
			ASSERT( rootClass );
			return rootClass;
		}

		// no base class, not a CObject class
		if ( !rttiClass->HasBaseClass() )
		{
			return NULL;
		}

		// get parent class
		ClassInfo* parentClassInfo = GetClassInfoForRTTIClass( rttiClass->GetBaseClass(), rootClass );
		if ( NULL == parentClassInfo )
		{
			return NULL;
		}

		// find matching class info for given class
		for ( Uint32 i=0; i<parentClassInfo->m_children.Size(); ++i )
		{
			ClassInfo* info = parentClassInfo->m_children[i];
			if ( info->m_class == rttiClass )
			{
				return info;
			}
		}

		// not found, create new entry
		ClassInfo* info = new ClassInfo( rttiClass, parentClassInfo );
		return info;
	}

	void AddObject( const CObject* object )
	{
#ifndef RED_ASSERTS_ENABLED
		RED_UNUSED( object );
#endif
		ASSERT( object->GetClass() == m_class );
		const Uint32 memorySize = m_class->GetSize();

		m_memoryExclusive += memorySize;
		m_countExclusive += 1;
		m_memoryInclusive += memorySize;
		m_countInclusive += 1;

		ClassInfo* parent = m_parent;
		while ( NULL != parent )
		{
			parent->m_memoryInclusive += memorySize;
			parent->m_countInclusive += 1;
			parent = parent->m_parent;
		}
	}
};

void CObject::DebugDumpClassList()
{
	// Root class info
	ClassInfo rootClass( ClassID<CObject>(), NULL );

	// Generate info
	for ( BaseObjectIterator it; it; ++it )
	{
		CObject* object = (*it);

		if ( !object->HasFlag( OF_DefaultObject ) )
		{
			ClassInfo* info = ClassInfo::GetClassInfoForRTTIClass( object->GetClass(), &rootClass );
			if ( NULL != info )
			{
				info->AddObject( object );
			}
		}
	}

	// Print the list
	LOG_CORE( TXT("========================= Object count and memory =============================") );
	rootClass.Print( 4 );
	LOG_CORE( TXT("================================== END ========================================") );
}

void CObject::ValidateObjectStructure()
{
	// Validate all objects, will crash on any errors
	for ( BaseObjectIterator it; it; ++it )
	{
		CObject* object = (*it);
		object->OnDebugValidate();
	}
}

void CObject::DebugDumpRootsetList()
{
#if 0
	Uint32 totalSizeStatic = 0;
	Uint32 totalSizeSerialize = 0;
	Uint32 totalSizeDynamic = 0;
	Uint32 totalSizeAll = 0;

	for ( THashMap< CObject*, Uint32 >::iterator it = SObjectsMap::GetInstance().m_rootSet.Begin(); it != SObjectsMap::GetInstance().m_rootSet.End(); ++it )
	{
		CObject* object = (*it).m_first;

		if ( object->HasFlag( OF_DefaultObject ) )
		{
			continue;
		}

		Uint32 count = (*it).m_second;

		Uint32 staticSize = object->GetClass()->GetSize();
		Uint32 serializeSize = CObjectMemoryAnalizer::CalcObjectSize( object );
		Uint32 dynamicSize = object->CalcObjectDynamicDataSize();
		Uint32 allSize = staticSize + serializeSize + dynamicSize;

		totalSizeStatic += staticSize;
		totalSizeSerialize += serializeSize;
		totalSizeDynamic += dynamicSize;
		totalSizeAll += allSize;

		LOG_CORE( TXT("all %6d - static %6d - ser %6d - dynamic %6d - ref %6d - %s - %s"), 
			allSize, staticSize, serializeSize, dynamicSize, count,
			object->GetClass()->GetName().AsString().AsChar(), 
			object->GetFriendlyName().AsChar() );
	}

	LOG_CORE( TXT("all %6d - static %6d - ser %6d - dynamic %6d - ALL"), 
		totalSizeAll, totalSizeStatic, totalSizeSerialize, totalSizeDynamic );
#endif
}

void CObject::DebugDumpDefaultObjectList()
{
#if 0
	Uint32 size = 0;

	for ( THashMap< CObject*, Uint32 >::iterator it = SObjectsMap::GetInstance().m_rootSet.Begin(); it != SObjectsMap::GetInstance().m_rootSet.End(); ++it )
	{
		CObject* object = (*it).m_first;

		if ( object->HasFlag( OF_DefaultObject ) )
		{
			Uint32 objSize = object->GetClass()->GetSize();
			LOG_CORE( TXT("%46s - %46s - %d"), object->GetClass()->GetName().AsString().AsChar(), object->GetFriendlyName().AsChar(), objSize );	
			size += objSize;
		}
	}

	LOG_CORE( TXT("All %d"), size );	
#endif
}

#endif

//////////////////////////////////////////////////////////////////////////
namespace 
{

class CFullReachablityMarker : public IFile
{
protected:
	TDynArray< Bool, MC_Engine >*		m_unreachables;
	TDynArray< CObject*, MC_Engine >*	m_objectsToProcess;
	TDynArray< CObject*, MC_Engine >	m_newObjects;

	THashMap< const ISerializable*, Uint32 >			m_visitedSerializables;

public:
	CFullReachablityMarker()
		: IFile( FF_Writer | FF_GarbageCollector | FF_FileBased )
		, m_unreachables( nullptr )
		, m_objectsToProcess( nullptr )
	{
		m_newObjects.Reserve( 2048 );
	}

	void Reset()
	{
		m_unreachables = nullptr;
		m_objectsToProcess = nullptr;
		m_newObjects.ClearFast();
		m_visitedSerializables.ClearFast();
	}

	RED_INLINE void SetUnreachables( TDynArray< Bool, MC_Engine >& unreachables )
	{
		m_unreachables = &unreachables;
	}

	RED_INLINE void SetObjectsToProcess( TDynArray< CObject*, MC_Engine >& objects )
	{
		m_objectsToProcess = &objects;
	}

	RED_INLINE void MarkReachable( CObject* object )
	{
		(*m_unreachables)[ object->GetObjectIndex() ] = true;
	}

	RED_INLINE Bool IsReachable( CObject* object )
	{
		return (*m_unreachables)[ object->GetObjectIndex() ];
	}

	RED_INLINE void Process()
	{
		while ( !m_objectsToProcess->Empty() )
		{
			while ( !m_objectsToProcess->Empty() )
			{
				CObject* obj = m_objectsToProcess->PopBackFast();
				if ( obj )
				{
					if ( !IsReachable( obj ) )
					{
						MarkReachable( obj );

						obj->OnSerialize( *this );
					}
				}
			}

			Swap( m_newObjects, *m_objectsToProcess );
			m_newObjects.ClearFast();
		}
	}

	virtual void SerializePointer( const class CClass* pointerClass, void*& pointer )
	{
		if ( nullptr != pointer && pointerClass->IsSerializable() )
		{
			ISerializable* obj = (ISerializable*&) pointer;
			if ( obj->GetClass()->IsObject() )
			{
				CObject* realObject = static_cast< CObject* >( obj );
				MarkObject( realObject );
			}
			else if ( !obj->CanIgnoreInGC() )
			{
				// visit only once
				const Uint32* visited = m_visitedSerializables.FindPtr( obj );
				if ( nullptr == visited )
				{
					// mark as visited
					m_visitedSerializables[ obj ] = 1;

					// recurse in place
					obj->OnSerialize( *this );
				}
			}
		}
	}

	void MarkObject( CObject* obj )
	{
		if ( obj )
		{
			// discarded objects should not be visited by GC
			if ( !obj->HasFlag( OF_Discarded ) )
			{
				if ( !IsReachable(obj) )
				{
					m_newObjects.PushBack( obj );
				}
			}
		}
	}

protected:
	// Fake IFile interface
	virtual void Serialize( void*, size_t ) {}
	virtual Uint64 GetOffset() const { return 0; };
	virtual Uint64 GetSize() const { return 0; }
	virtual void Seek( Int64 ) {};
};

}

class CDebugReachablityMarker : public CFullReachablityMarker
{
protected:
	// Dependency print
	TDynArray< Int32, MC_Engine >			m_objectsParent;
	Int32									m_currParent;
	CClass*									m_classToDebug;

	// Reachability test
	CObject*								m_ignoredOwner;
	const CObject*							m_testedObject;
	CObject*								m_currentOwner;

public:
	RED_INLINE CDebugReachablityMarker( const CObject* testedObject,  CObject* ignoredOwner )
		: CFullReachablityMarker()
		, m_currParent( -1 )
		, m_classToDebug( NULL )
		, m_currentOwner( NULL )
		, m_testedObject( testedObject )
		, m_ignoredOwner( ignoredOwner )
	{
	}

	RED_INLINE CDebugReachablityMarker( const CName& className )
		: CFullReachablityMarker()
		, m_currParent( -1 )
		, m_classToDebug( NULL )
		, m_currentOwner( NULL )
		, m_testedObject( NULL )
	{
		m_classToDebug = SRTTI::GetInstance().FindClass( className );
	}

	void MarkChildHierarchy( CObject* rootObject )
	{
		m_objectsParent.ClearFast();
		m_objectsToProcess->ClearFast();
		m_currParent = -1;


		// Add object to check
		m_objectsToProcess->PushBack( rootObject );
		m_objectsParent.PushBack( m_currParent );

		MarkReachable( rootObject );

		// Process crap
		Uint32 obIndex=0;
		while ( obIndex<m_objectsToProcess->Size() )
		{
			CObject* obj = (*m_objectsToProcess)[obIndex];
			m_currParent = obIndex;

			m_currentOwner = obj;
			obj->OnSerialize( *this );

			++obIndex;
		}
	}

	Bool ReachabilityTest( CObject* rootObject )
	{
		m_objectsParent.ClearFast();
		m_objectsToProcess->ClearFast();
		m_currParent = -1;


		// Add object to check
		m_objectsToProcess->PushBack( rootObject );
		m_objectsParent.PushBack( m_currParent );

		MarkReachable( rootObject );

		// Process crap
		Uint32 obIndex=0;
		while ( obIndex<m_objectsToProcess->Size() )
		{
			CObject* obj = (*m_objectsToProcess)[obIndex];
			m_currParent = obIndex;

			if( obj == m_testedObject )
			{
				PrintHere( obj );
				return true;
			}

			m_currentOwner = obj;
			obj->OnSerialize( *this );

			++obIndex;
		}
		return false;
	}

	virtual void SerializePointer( const class CClass* pointerClass, void*& pointer )
	{
		if ( !pointer )
		{
			return;
		}

		if ( pointerClass->IsObject() )
		{
			CObject* obj = static_cast< CObject* >( pointer );
			if ( obj )
			{
				if( obj == m_testedObject && m_ignoredOwner )
				{
					if( m_ignoredOwner == m_currentOwner )
					{
						return;
					}
				}

				// Validate class
				if ( !obj->GetClass() || !CObject::IsValidObject(obj) || (obj->GetClass()->GetType() != RT_Class) )
				{
					HALT( "Invalid GC object class. Please debug." );
					return;
				}

				if ( !IsReachable( obj ) )
				{
					MarkReachable( obj );

					m_objectsToProcess->PushBack( obj );
					m_objectsParent.PushBack( m_currParent );

					if( m_classToDebug )
					{
						CClass* objClass = obj->GetClass();
						if ( objClass->IsA( m_classToDebug ) )
						{
							PrintHere( obj );
						}
					}
				}
			}
		}
	}

	bool IsPointingTo(void* p, IRTTIType* itype, void *obj )
	{
		void* propertyDataPtr = NULL;
		switch ( itype->GetType() )
		{
		case RT_Pointer:
			propertyDataPtr = static_cast<CRTTIPointerType*>( itype )->GetPointed( obj );
			break;
		case RT_Handle:
			propertyDataPtr = static_cast<CRTTIHandleType*>( itype )->GetPointed( obj );
			break;
		case RT_SoftHandle:
			propertyDataPtr = static_cast<CRTTISoftHandleType*>( itype )->GetPointed( obj );
			break;
		case RT_Array:
			{
				CRTTIArrayType* objectsArray=static_cast<CRTTIArrayType*>( itype );

				if ( objectsArray->GetInnerType() && objectsArray->GetInnerType()->GetType()!=RT_Pointer && objectsArray->GetInnerType()->GetType()!=RT_Handle && objectsArray->GetInnerType()->GetType()!=RT_SoftHandle )
				{
					return false;
				}
				for ( Uint32 i = 0; i < objectsArray->GetArraySize( obj ); ++i )
				{
					if ( IsPointingTo( p, objectsArray->GetInnerType(), objectsArray->GetArrayElement( ( void* ) obj, i ) ) )
						return true;
				}
			}
			return false;
			break;
		case RT_NativeArray:
			{
				CRTTINativeArrayType* objectsArray = static_cast<CRTTINativeArrayType*>( itype );

				if ( objectsArray->GetInnerType() && objectsArray->GetInnerType()->GetType()!=RT_Pointer && objectsArray->GetInnerType()->GetType()!=RT_Handle && objectsArray->GetInnerType()->GetType()!=RT_SoftHandle )
				{
					return false;
				}
				for ( Uint32 i = 0; i < objectsArray->GetArraySize( obj ); ++i )
				{
					if ( IsPointingTo( p, objectsArray->GetInnerType(), objectsArray->GetArrayElement( ( void* ) obj, i ) ) )
						return true;
				}
			}
			return false;
			break;
		default:
			propertyDataPtr = obj;
		}
		return propertyDataPtr == p;
	}

	void PrintHere( CObject* obj )
	{
		CObject* currObj = obj;

		// Find first index
		Int32 index = -1;
		for ( Uint32 i=0; i<m_objectsToProcess->Size(); ++i )
		{
			if ( currObj == (*m_objectsToProcess)[ i ] )
			{
				index = i;
			}
		}

		if ( index == -1 )
		{
			HALT( "" );
			return;
		}

		currObj = (*m_objectsToProcess)[ index ];
		ASSERT( currObj == obj );

		Uint32 depth = 0;
		CObject* prevObj = NULL;

		for ( ;; )
		{
			{
				CObject* currObj = (*m_objectsToProcess)[ index ];

				String str;
				for ( Uint32 i=0; i<depth; ++i )
				{
					str += TXT( "\t" );
				}
				str += currObj->GetFriendlyName();

				if (prevObj)
				{
					CClass* objClass = currObj->GetClass();
					if ( objClass )
					{
						TDynArray< CProperty* > props;
						objClass->GetProperties( props );
						for ( Uint32 i = 0; i<props.Size(); ++i )
						{
							CProperty* prop = props[i];
							if ( IsPointingTo( prevObj, prop->GetType(), prop->GetOffsetPtr( currObj ) ) )
							{
								str += TXT( "::" );
								str += props[i]->GetName().AsString();
							}
						}
					}
				}

				LOG_CORE( TXT("%s"), str.AsChar() );
				prevObj = currObj;
			}

			Int32 parentIndex = m_objectsParent[ index ];
			if ( parentIndex != -1 )
			{
				index = parentIndex;
				depth += 1;
			}
			else
			{
				break;
			}
		}
	}
};

Bool CObject::TestObjectReachability( CObject* testedObject, CObject* ignoredParent )
{
	// Lock the job manager - we won't issue any new jobs
	SJobManager::GetInstance().Lock();

	// Mark all objects as unreachable
	const Uint32 allObjectsSize = GObjectsMap->GetMaxObjectIndex();

	// Prepare unreachable array
	TDynArray< Bool, MC_Engine > unreachables;
	unreachables.ClearFast();
	unreachables.ResizeFast(allObjectsSize);
	Red::System::MemorySet( unreachables.Data(), 0, allObjectsSize );

	TDynArray< CObject*, MC_Engine > objectsToProcess;
	objectsToProcess.Reserve( 4096 );

	CDebugReachablityMarker marker( testedObject, ignoredParent );
	marker.SetObjectsToProcess( objectsToProcess );
	marker.SetUnreachables( unreachables );

	struct RootSetVisitor
	{
		CDebugReachablityMarker*	m_marker;
		Bool						m_reached;

		RED_INLINE const Bool operator()( CObject* object )
		{
			if ( m_marker->ReachabilityTest( object ) )
			{
				m_reached = true;
				return false;
			}

			return true;
		}
	};

	RootSetVisitor visitor;
	visitor.m_marker = &marker;
	visitor.m_reached = false;
	GObjectsRootSet->VisitRootSet(visitor);

	// Unlock jobs
	SJobManager::GetInstance().Unlock();

	return visitor.m_reached;
}

void CObject::PrintDependencies( const CName& className )
{
	// Lock the job manager - we won't issue any new jobs
	SJobManager::GetInstance().Lock();

	// Do not garbage collect if we are doing something
	while ( SJobManager::GetInstance().IsBlockingGC() )
	{
		Red::Threads::SleepOnCurrentThread( 0 );
	}

	// Mark all objects as unreachable
	const Uint32 allObjectsSize = GObjectsMap->GetMaxObjectIndex();

	// Prepare unreachable array
	TDynArray< Bool, MC_Engine > unreachables;
	unreachables.ClearFast();
	unreachables.ResizeFast(allObjectsSize);
	Red::System::MemorySet( unreachables.Data(), 0, allObjectsSize );

	TDynArray< CObject*, MC_Engine > objectsToProcess;
	objectsToProcess.Reserve( 4096 );

	CDebugReachablityMarker marker( className );
	marker.SetObjectsToProcess( objectsToProcess );
	marker.SetUnreachables( unreachables );

	struct RootSetVisitor
	{
		CDebugReachablityMarker*	m_marker;

		RED_INLINE const Bool operator()( CObject* object )
		{
			m_marker->MarkChildHierarchy( object );
			return true;
		}
	};

	RootSetVisitor visitor;
	visitor.m_marker = &marker;
	GObjectsRootSet->VisitRootSet(visitor);

	// Unlock jobs
	SJobManager::GetInstance().Unlock();
}

//////////////////////////////////////////////////////////////////////////
