/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "class.h"
#include "memoryHelpers.h"

// templated version of CClass providing implementation of ctors/copy/etc
template< class _Type, EMemoryClass _MemoryClass, typename _MemoryPool >
class TTypedClass : public CClass
{
private:
	mutable void*	m_defaultObject;

public:
	TTypedClass( const CName& name, Uint32 size, Uint32 flags )
		: CClass( name, size, flags | CF_Native )
		, m_defaultObject( NULL )
	{
	}

	virtual EMemoryClass GetMemoryClass() const
	{
		return _MemoryClass;
	}

	virtual EMemoryPoolLabel GetMemoryPool() const
	{
		return Memory::GetPoolLabel< RED_TYPED_CLASS_CAST _MemoryPool::Type >();
	}

	virtual void* AllocateRelatedMemory( Uint32 size, Uint32 alignment ) const
	{
		return RED_MEMORY_ALLOCATE_ALIGNED_HYBRID( RED_TYPED_CLASS_CAST _MemoryPool::Type, _MemoryClass, size, alignment );
	}

	virtual void FreeRelatedMemory( void* mem ) const
	{
		RED_MEMORY_FREE_HYBRID( RED_TYPED_CLASS_CAST _MemoryPool::Type, _MemoryClass, mem );
	}

	virtual void * AllocateClass() const override final
	{
		return RED_MEMORY_ALLOCATE_ALIGNED_HYBRID( RED_TYPED_CLASS_CAST _MemoryPool::Type, _MemoryClass, sizeof( _Type ), __alignof( _Type ) );
	}

	virtual void FreeClass( void * mem ) const override final
	{
		RED_MEMORY_FREE_HYBRID( RED_TYPED_CLASS_CAST _MemoryPool::Type, _MemoryClass, mem );
	}

	virtual void Construct( void* mem, Bool isDefaultObject ) const
	{
		new (mem) _Type;

		// Apply default values
		const Bool createInlinedObjects = !isDefaultObject;
		ApplyDefaultValues( mem, createInlinedObjects, true );
	}

	virtual void Construct( void *mem ) const
	{
		new (mem) _Type;

		// Apply default values
		ApplyDefaultValues( mem, true, true );
	}

	virtual void Destruct( void *mem ) const
	{
		for ( CProperty* prop : m_propertiesToDestroy )
		{
			prop->GetType()->Destruct( prop->GetOffsetPtr( mem ) );
		}

		((_Type*)mem)->~_Type();
	}
		
	virtual Bool Compare( const void* data1, const void* data2, Uint32 ) const
	{
		return CClass::DeepCompare( data1, data2 );
	}

	virtual void Copy( void* dest, const void* src ) const
	{
		RED_ASSERT( m_baseClass.m_parentClass == nullptr );
		*((_Type*)dest) = *((const _Type*)src);
	}

	virtual void Clean( void* data ) const
	{
		Destruct( data );
		Construct( data );
	}

	virtual void* GetDefaultObjectImp() const
	{
		if ( !m_defaultObject )
		{
			// Create default object in zero initialized memory
			const Bool callConstructor = false;
			m_defaultObject = CreateObject( sizeof( _Type ), callConstructor ); // we call the constructor manually
			Construct( m_defaultObject, true );

			// Register objects in the root set
			if ( IsObject() )
			{
				// Register in the root set to keep it alive
				void AddDefaultObjectToRootSet( CObject *obj );
				AddDefaultObjectToRootSet( (CObject*)m_defaultObject );
			}
		}

		return m_defaultObject;
	}

	virtual void CreateDefaultObject() override final
	{
		if ( !m_defaultObject )
		{
			GetDefaultObjectImp();
		}
	}

	virtual void DestroyDefaultObject()
	{
		if ( m_defaultObject )
		{
			if ( IsObject() )
			{
				// Remove the object from the root set, it will be deleted by GC
				void RemoveFromRootSet( CObject *obj );
				RemoveFromRootSet( (CObject*)m_defaultObject );
			}

			// Free memory
			// TODO: default objects are leaking but this happens at the exit...
			//DestroyObject( this, m_defaultObject );
			m_defaultObject = NULL;
		}
	}

	// By default, do not handle any unknown properties. Template specialization can be used to add support
	// for specific types.
	virtual Bool OnReadUnknownProperty( void*, const CName&, const CVariant& ) const { return false; }
};

// templated version of CClass providing implementation of ctors/copy/etc
template< class _Type, EMemoryClass _MemoryClass, typename _MemoryPool >
class TTypedClassNoCopy : public CClass
{
private:
	mutable void*				m_defaultObject;

public:
	TTypedClassNoCopy( const CName& name, Uint32 size, Uint32 flags )
		: CClass( name, size, flags | CF_Native )
		, m_defaultObject( NULL )
	{
	}

	virtual EMemoryClass GetMemoryClass() const
	{
		return _MemoryClass;
	}

	virtual EMemoryPoolLabel GetMemoryPool() const
	{
		return Memory::GetPoolLabel< RED_TYPED_CLASS_CAST _MemoryPool::Type >();
	}

	virtual void* AllocateRelatedMemory( Uint32 size, Uint32 alignment ) const
	{
		return RED_MEMORY_ALLOCATE_ALIGNED_HYBRID( RED_TYPED_CLASS_CAST _MemoryPool::Type, _MemoryClass, size, alignment );
	}

	virtual void FreeRelatedMemory( void* mem ) const
	{
		RED_MEMORY_FREE_HYBRID( RED_TYPED_CLASS_CAST _MemoryPool::Type, _MemoryClass, mem );
	}

	virtual void * AllocateClass() const override final
	{
		return RED_MEMORY_ALLOCATE_ALIGNED_HYBRID( RED_TYPED_CLASS_CAST _MemoryPool::Type, _MemoryClass, sizeof( _Type ), __alignof( _Type ) );
	}

	virtual void FreeClass( void * mem ) const override final
	{
		RED_MEMORY_FREE_HYBRID( RED_TYPED_CLASS_CAST _MemoryPool::Type, _MemoryClass, mem );
	}

	virtual void Construct( void *mem, Bool isDefaultObject ) const
	{	
		// Create the native shit
		new (mem) _Type();

		// Apply default values
		const Bool createInlinedObjects = !isDefaultObject;
		ApplyDefaultValues( mem, createInlinedObjects, true );
	}

	virtual void Construct( void *mem ) const
	{	
		// Create the native shit
		new (mem) _Type();

		// Apply default values
		ApplyDefaultValues( mem, true, true );
	}

	virtual void Destruct( void *mem ) const
	{
		// Destroy local scripted properties
		for ( CProperty* prop : m_propertiesToDestroy )
		{
			prop->GetType()->Destruct( prop->GetOffsetPtr( mem ) );
		}

		// Destroy native shit
		((_Type*)mem)->~_Type();
	}

	virtual Bool Compare( const void* data1, const void* data2, Uint32 flags ) const
	{
		RED_UNUSED( flags );
		return CClass::DeepCompare( data1, data2 );
	}

	virtual void Copy( void* dest, const void* src ) const
	{
		RED_UNUSED( dest );
		RED_UNUSED( src );
		// empty 
	}

	virtual void Clean( void* data ) const
	{
		Destruct( data );
		Construct( data );
	}

	virtual void* GetDefaultObjectImp() const
	{
		if ( !m_defaultObject )
		{
			// Create default object in zero initialized memory
			const Bool callConstructor = false;
			m_defaultObject = CreateObject( sizeof( _Type ), callConstructor ); // we call the constructor manually
			Construct( m_defaultObject, true );

			// Register objects in the root set
			if ( IsA< CObject >() )
			{
				// Register in the root set to keep it alive
				void AddDefaultObjectToRootSet( CObject *obj );
				AddDefaultObjectToRootSet( (CObject*)m_defaultObject );
			}
		}

		return m_defaultObject;
	}

	virtual void CreateDefaultObject() override final
	{
		if ( !m_defaultObject )
		{
			GetDefaultObjectImp();
		}
	}

	virtual void DestroyDefaultObject()
	{
		if ( m_defaultObject )
		{
			if ( IsA< CObject >() )
			{
				// Remove the object from the root set, it will be deleted by GC
				void RemoveFromRootSet( CObject *obj );
				RemoveFromRootSet( (CObject*)m_defaultObject );
			}

			// Free memory
			// TODO: default objects are leaking but this happens at the exit...
			//DestroyObject( this, m_defaultObject );
			m_defaultObject = NULL;
		}
	}
};
