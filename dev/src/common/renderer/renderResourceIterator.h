/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // 

template< typename T >
class TRenderResourceCollection : public Red::System::NonCopyable
{
	template< typename RES > friend class TRenderResourceList;

public:
	enum { CAPACITY = 8 * 1024 };
	typedef T* const * iterator;

public:
	~TRenderResourceCollection ()
	{
		Clear();
	}

	RED_FORCE_INLINE Bool Empty() const
	{
		return m_res.Empty();
	}

	RED_FORCE_INLINE Uint32 Size() const
	{
		return m_res.Size();
	}

	RED_FORCE_INLINE void Clear()
	{
		for ( T *res : *this )
		{
			res->Release();
		}

		m_res.ClearFast();
	}

	RED_FORCE_INLINE T* operator[]( Uint32 index ) const
	{
		return m_res[index];
	}

	RED_FORCE_INLINE iterator Begin() const
	{
		return m_res.Begin();
	}

	RED_FORCE_INLINE iterator End() const
	{
		return m_res.End();
	}

private:
	RED_FORCE_INLINE void AddResource( T *res )
	{
		RED_FATAL_ASSERT( res, "Expected resource" );
		m_res.PushBack( res );
		res->AddRef();
	}

private:
	TStaticArray< T*, CAPACITY > m_res;
};

template < typename T > typename TRenderResourceCollection< T >::iterator begin( const TRenderResourceCollection< T > &collection ) { return collection.Begin(); }
template < typename T > typename TRenderResourceCollection< T >::iterator end( const TRenderResourceCollection< T > &collection ) { return collection.End(); }

// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // 

/// Resource iterator
template< typename RES >
class TRenderResourceIterator : public Red::System::NonCopyable
{
	typedef TRenderResourceCollection< RES > TCollection;
	TCollection m_collection;
	typename TCollection::iterator m_cur;
	typename TCollection::iterator m_end;

public:
	explicit TRenderResourceIterator( Bool build = true )
		: m_cur( nullptr )
		, m_end( nullptr )
	{
		if ( build )
		{
			Rebuild();
		}
	}

	void Rebuild()
	{
		m_collection.Clear();
		RES::GetResourceCollection( m_collection );
		Rewind();
	}

	void Rewind()
	{
		m_cur = m_collection.Begin();
		m_end = m_collection.End();
	}

	void Clear()
	{
		m_collection.Clear();
		Rewind();
	}

	RED_INLINE void operator++()
	{
		RED_FATAL_ASSERT( m_cur != m_end, "Invalid call" );
		++m_cur;
	}

	RED_INLINE operator Bool() const
	{
		return m_cur != m_end;
	}

	RED_INLINE RES* operator->()
	{
		return *m_cur;
	}

	RED_INLINE RES* operator*()
	{
		return *m_cur;
	}
};

// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // 

template< typename T >
class TRenderResourceList : public Red::System::NonCopyable
{
	template< typename R > friend class TRenderResourceListWithCache;
	template< typename R > friend class TScopedRenderResourceCreationObject;

private:
#ifdef RED_PLATFORM_ORBIS
	typedef Red::MemoryFramework::CAdaptiveMutexBase< Red::MemoryFramework::OSAPI::CAdaptiveMutexImpl > TLock;
#else
	typedef Red::Threads::CSpinLock				TLock;
#endif

	typedef Red::Threads::CScopedLock< TLock >  TScopedLock;

public:
	TRenderResourceList ()
		: m_nextObject( nullptr )
		, m_prevObject( nullptr )
	{}

	virtual ~TRenderResourceList()
	{
		RED_FATAL_ASSERT( !IsResourceListLinked(), "Expected unlinked object" );
	}

	static void GetResourceCollection( TRenderResourceCollection< T > &outCollection )
	{
		RED_FATAL_ASSERT( outCollection.Empty(), "Expected empty collection - otherwise some duplicated could be produced" );

		TScopedLock lock ( st_lock );

		const Uint32 neededCapacity = outCollection.Size() + st_count.GetValue(); //< We can safely use st_count in here because it's always guarded by a spinlock
		if ( neededCapacity <= outCollection.CAPACITY )
		{
			for ( T *iter = st_head; iter; iter = iter->m_nextObject )
			{
				outCollection.AddResource( iter );
			}

			RED_FATAL_ASSERT( outCollection.Size() == neededCapacity, "Invalid st_count?" );
		}
		else
		{
			RED_FATAL( "Slower path, and not all objects will be collected" );
			for ( T *iter = st_head; iter && outCollection.Size() < outCollection.CAPACITY; iter = iter->m_nextObject )
			{
				outCollection.AddResource( iter );
			}

			RED_FATAL_ASSERT( outCollection.Size() <= neededCapacity, "Invalid st_count?" );
		}
	}

	template< typename TCondition >
	static void GetResourceCollection( TRenderResourceCollection< T > &outCollection, TCondition &&condition )
	{
		RED_FATAL_ASSERT( outCollection.Empty(), "Expected empty collection - otherwise some duplicated could be produced" );

		TScopedLock lock ( st_lock );

		// We're keeping the same logic (regarding capacity) here as in the version of this function without a condition, because
		// it will be slightly faster this way, and situation where there are too many objects to collect is invalid anyway.

		const Uint32 neededCapacity = outCollection.Size() + st_count.GetValue(); //< We can safely use st_count in here because it's always guarded by a spinlock
		if ( neededCapacity <= outCollection.CAPACITY )
		{
			for ( T *iter = st_head; iter; iter = iter->m_nextObject )
			{
				if ( condition( iter ) )
				{
					outCollection.AddResource( iter );
				}
			}

			RED_FATAL_ASSERT( outCollection.Size() <= neededCapacity, "Invalid st_count?" );
		}
		else
		{
			RED_FATAL( "Slower path, and not all objects will be collected" );
			for ( T *iter = st_head; iter && outCollection.Size() < outCollection.CAPACITY; iter = iter->m_nextObject )
			{
				if ( condition( iter ) )
				{
					outCollection.AddResource( iter );
				}
			}

			RED_FATAL_ASSERT( outCollection.Size() <= neededCapacity, "Invalid st_count?" );
		}
	}

	template< typename TVisitor >
	static void VisitResourcesConditional( TVisitor &visitor )
	{
		TScopedLock lock ( st_lock );

		for ( T *iter = st_head; iter; iter = iter->m_nextObject )
		{
			if ( !visitor.Visit( iter ) )
			{
				break;
			}
		}
	}

	template< typename TVisitor >
	static void VisitResourcesAll( TVisitor &visitor )
	{
		TScopedLock lock ( st_lock );

		for ( T *iter = st_head; iter; iter = iter->m_nextObject )
		{
			visitor.Visit( iter );
		}
	}

private:
	RED_FORCE_INLINE Bool IsResourceListLinked() const
	{
		RED_FATAL_ASSERT( !(!m_prevObject && m_nextObject), "Internal error" );
		return nullptr != m_prevObject;
	}

	static void ResourceCachePartialCreationEarlyExit( Uint64 partialRegistrationHash )
	{
		// empty
	}

	static void ResourceCachePartialCreationBegin( T *obj, Uint64 partialRegistrationHash )
	{
		RED_FATAL_ASSERT( 0 == partialRegistrationHash, "Registration not supported in this class - this may happen when called on pointer on TRenderResourceList when the resource is TRenderResourceListWithCache" );
		RED_FATAL_ASSERT( !obj->IsResourceListLinked(), "Invalid call - did you call begin/end twice?" );		
	}

	static void ResourceCachePartialCreationEnd( T *obj, Uint64 partialRegistrationHash, Bool succeeded )
	{
		RED_FATAL_ASSERT( 0 == partialRegistrationHash, "Registration not supported in this class - this may happen when called on pointer on TRenderResourceList when the resource is TRenderResourceListWithCache" );
		RED_FATAL_ASSERT( !obj->IsResourceListLinked(), "Invalid call - did you call begin/end twice?" );

		if ( succeeded )
		{
			TScopedLock lock ( st_lock );
			ResourceListLink( obj );
		}
	}

	RED_INLINE static void ResourceListLink( T *obj )
	{
		RED_FATAL_ASSERT( obj, "Expected object" );
		RED_FATAL_ASSERT( !obj->IsResourceListLinked(), "Expected unlinked object" );
		
		if ( st_head ) st_head->m_prevObject = &obj->m_nextObject;
		obj->m_nextObject = st_head;
		obj->m_prevObject = &st_head;
		st_head = obj;

		st_count.Increment();
	}

	RED_INLINE static void ResourceListUnlink( T *obj )
	{
		RED_FATAL_ASSERT( obj, "Expected object" );
		RED_FATAL_ASSERT( obj->IsResourceListLinked(), "Expected linked object" );
		
		if ( obj->m_nextObject ) obj->m_nextObject->m_prevObject = obj->m_prevObject;
		if ( obj->m_prevObject ) *obj->m_prevObject = obj->m_nextObject;
		obj->m_prevObject = NULL;
		obj->m_nextObject = NULL;
		
		const Int32 newStCount = st_count.Decrement();
		RED_UNUSED( newStCount );
		RED_FATAL_ASSERT( newStCount >= 0, "Internal error (resource wasn't linked?)" );
	}

protected:
	RED_INLINE static Int32 ResourceListRelease( T *obj, Red::Threads::CAtomic< Int32 > &refCount )
	{
		RED_FATAL_ASSERT( obj, "Expected object" );

		TScopedLock lock ( st_lock );

		RED_FATAL_ASSERT( refCount.GetValue() > 0, "WTF" );

		const Int32 newRefCount = refCount.Decrement();
		if ( 0 == newRefCount )
		{
			if ( obj->IsResourceListLinked() ) //< resource could not be linked in case of creation failure (it gets linked only after successfull initialization)
			{
				ResourceListUnlink( obj );			
			}
		}

		return newRefCount;
	}

private:	
	T**										m_prevObject;
	T*										m_nextObject;
	static T*								st_head;
	static Red::Threads::CAtomic< Int32 >	st_count;
	static TLock							st_lock;
};

#define IMPLEMENT_RENDER_RESOURCE_ITERATOR( _class ) \
	template<> _class*								TRenderResourceList<_class>::st_head = nullptr; \
	template<> TRenderResourceList<_class>::TLock	TRenderResourceList<_class>::st_lock {}; \
	template<> Red::Threads::CAtomic< Int32 >		TRenderResourceList<_class>::st_count ( 0 );

// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // 

template< typename T >
class TRenderResourceListWithCache : public TRenderResourceList<T>
{
	template< typename R > friend class TScopedRenderResourceCreationObject;

	typedef THashMap< Uint64, T*, DefaultHashFunc< Uint64 >, DefaultEqualFunc< Uint64 >, MC_HashMap >		TResourceCache;

	enum eResourceCreationState
	{
		RCS_Initial,
		RCS_DuringCreationWithCache,
		RCS_DuringCreationWithoutCache,
		RCS_Failed,
		RCS_Created,
	};

private:
	Uint64					m_resourceCacheHash;
	eResourceCreationState	m_resourceCreationState : 8;

public:
	TRenderResourceListWithCache ()
		: m_resourceCacheHash( 0 )
		, m_resourceCreationState( RCS_Initial )
	{}

	static Uint64 CalcResourceHash( const CResource *resource )
	{
		// invalid resource
		if ( !resource )
			return 0;

		// not a root resource - not a good candidate for caching
		if ( resource->GetParent() != nullptr )
			return 0;

		// use the depot path to cache the file
		const String depotPath = resource->GetDepotPath().AsChar();
		if ( depotPath.Empty() )
			return 0;

		// calculate 64-bit hash from the depot path
		const Uint64 hash = Red::CalculateHash64( depotPath.Data(), depotPath.DataSize() );
		RED_FATAL_ASSERT( 0 != hash, "Hash value zero is reserved for invalid hash, so we have some ambiguity here!" );
		return hash;
	}	
	
	template< typename TDest >
	static Bool ResourceCacheRequestPartialCreate( Uint64 hash, TDest *&outRes )
	{
		RED_FATAL_ASSERT( !outRes, "Input resource must be null to exclude any ambiguity in logic" );

		if ( !hash )
		{
			return true;
		}

		TRenderResourceList<T>::st_lock.Acquire();

		T* object = nullptr;
		if ( st_resources.Find( hash, object ) )
		{
			LOG_RENDERER( TXT("Reused rendering resource: %ls"), object->GetCategory().AsChar() );
			RED_FATAL_ASSERT( object->m_resourceCacheHash == hash, "Hashes should be the same" );
			object->AddRef();

			TRenderResourceList<T>::st_lock.Release();

			while ( RCS_DuringCreationWithCache == object->m_resourceCreationState )
			{
				Red::Threads::YieldCurrentThread();
			}

			if ( RCS_Created != object->m_resourceCreationState )
			{
				RED_FATAL_ASSERT( RCS_Failed == object->m_resourceCreationState, "Partial creation not performed?" );
				object->Release();
				object = nullptr;
			}

			outRes = static_cast<TDest*>( object );
			return false;
		}
		else
		{
			RED_FATAL_ASSERT( !object, "Internal error" );
		}

		return true;
	}

private:
	static void ResourceCachePartialCreationEarlyExit( Uint64 partialRegistrationHash )
	{
		if ( 0 != partialRegistrationHash )
		{
			TRenderResourceList<T>::st_lock.Release();
		}
	}

	static void ResourceCachePartialCreationBegin( T *obj, Uint64 partialRegistrationHash )
	{
		RED_FATAL_ASSERT( 0 == obj->m_resourceCacheHash, "This is supposed to be freshly created resource..." );
		RED_FATAL_ASSERT( RCS_Initial == obj->m_resourceCreationState, "This is supposed to be freshly created resource..." );
		RED_FATAL_ASSERT( !obj->TRenderResourceList<T>::IsResourceListLinked(), "This is supposed to be freshly created resource..." );

		if ( 0 == partialRegistrationHash ) // Apparently we are not supposed to perform partial creation
		{
			obj->m_resourceCreationState = RCS_DuringCreationWithoutCache;
		}
		else
		{
			// setup partial creation flags
			obj->m_resourceCreationState = RCS_DuringCreationWithCache;

			// store in cache
			RED_FATAL_ASSERT( !st_resources.KeyExist( partialRegistrationHash ), "This is supposed to be freshly created resource..." );
			st_resources.Insert( partialRegistrationHash, obj );
			obj->m_resourceCacheHash = partialRegistrationHash;

			// release lock
			TRenderResourceList<T>::st_lock.Release();
		}
	}

	static void ResourceCachePartialCreationEnd( T *obj, Uint64 partialRegistrationHash, Bool succeeded )
	{
		RED_FATAL_ASSERT( partialRegistrationHash == obj->m_resourceCacheHash, "We are supposed to be during creation" );
		RED_FATAL_ASSERT( !obj->TRenderResourceList<T>::IsResourceListLinked(), "Internal error" );
				
		if ( 0 == partialRegistrationHash ) // Apparently we are not supposed to perform partial creation
		{
			RED_FATAL_ASSERT( RCS_DuringCreationWithoutCache == obj->m_resourceCreationState, "We are supposed to be during creation without cache" );

			if ( succeeded )
			{
				typename TRenderResourceList<T>::TScopedLock lock ( TRenderResourceList<T>::st_lock );

				TRenderResourceList<T>::ResourceListLink( obj );

				obj->m_resourceCreationState = RCS_Created;
			}
			else
			{	
				obj->m_resourceCreationState = RCS_Failed;
			}
		}
		else
		{
			RED_FATAL_ASSERT( RCS_DuringCreationWithCache == obj->m_resourceCreationState, "We are supposed to be during creation with cache" );
			
			#ifdef RED_ASSERTS_ENABLED
			{
				T* existing = nullptr;
				Bool exists = st_resources.Find( partialRegistrationHash, existing );
				RED_FATAL_ASSERT( exists && existing == obj, "Internal error" );
			}
			#endif

			if ( succeeded )
			{
				typename TRenderResourceList<T>::TScopedLock lock ( TRenderResourceList<T>::st_lock );
				
				TRenderResourceList<T>::ResourceListLink( obj );

				obj->m_resourceCreationState = RCS_Created;			
			}
			else
			{				
				typename TRenderResourceList<T>::TScopedLock lock ( TRenderResourceList<T>::st_lock );

				st_resources.Erase( partialRegistrationHash );
				obj->m_resourceCacheHash = 0;

				obj->m_resourceCreationState = RCS_Failed;
			}
		}
	}

protected:
	RED_INLINE static Int32 ResourceListRelease( T *obj, Red::Threads::CAtomic< Int32 > &refCount )
	{
		RED_FATAL_ASSERT( obj, "Expected object" );

		typename TRenderResourceList<T>::TScopedLock lock ( TRenderResourceList<T>::st_lock );

		RED_FATAL_ASSERT( refCount.GetValue() > 0, "WTF" );

		const Int32 newRefCount = refCount.Decrement();
		if ( 0 == newRefCount )
		{
			if ( 0 != obj->m_resourceCacheHash )
			{
				st_resources.Erase( obj->m_resourceCacheHash );
				obj->m_resourceCacheHash = 0;
			}

			if ( obj->IsResourceListLinked() ) //< resource could not be linked in case of creation failure (it gets linked only after successfull initialization)
			{
				TRenderResourceList<T>::ResourceListUnlink( obj );
			}
		}

		return newRefCount;
	}

private:
	static TResourceCache	st_resources;
};

#define IMPLEMENT_RENDER_RESOURCE_ITERATOR_WITH_CACHE( _class ) \
	IMPLEMENT_RENDER_RESOURCE_ITERATOR( _class ) \
	template<> TRenderResourceListWithCache<_class>::TResourceCache	TRenderResourceListWithCache<_class>::st_resources {};

// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // 

template< typename T >
class TScopedRenderResourceCreationObject : public Red::System::NonCopyable
{
	T		*m_resource;
	Uint64	m_partialRegHash;
	Bool	m_isResultPending;

public:
	explicit TScopedRenderResourceCreationObject ( Uint64 partialRegistrationHash )
		: m_resource( nullptr )
		, m_partialRegHash( partialRegistrationHash )
		, m_isResultPending( true )
	{}

	~TScopedRenderResourceCreationObject()
	{
		if ( m_isResultPending )
		{
			if ( m_resource )
			{
				T::ResourceCachePartialCreationEnd( m_resource, m_partialRegHash, false );
				m_resource->Release();
			}
			else
			{
				T::ResourceCachePartialCreationEarlyExit( m_partialRegHash );
			}

			m_isResultPending = false;
		}
	}

	void InitResource( T *resource )
	{
		RED_FATAL_ASSERT( resource, "" );
		RED_FATAL_ASSERT( !m_resource, "" );
		RED_FATAL_ASSERT( m_isResultPending, "" );

		if ( resource )
		{
			m_resource = resource; // no addref, we're taking over ownership
			T::ResourceCachePartialCreationBegin( m_resource, m_partialRegHash );
		}
	}

	T* GetResource() const
	{
		RED_FATAL_ASSERT( m_isResultPending, "Invalid call - should be called at most once" );
		return m_resource;
	}

	T* RetrieveSuccessfullyCreated()
	{
		RED_FATAL_ASSERT( m_isResultPending, "Invalid call - should be called at most once" );
		RED_FATAL_ASSERT( m_resource, "Invalid call - should be called only at initialized object" );
		
		T* result = nullptr;
		
		if ( m_resource )
		{
			result = m_resource;
			T::ResourceCachePartialCreationEnd( m_resource, m_partialRegHash, true );
			m_resource = nullptr;
			m_isResultPending = false;
		}
		
		return result;
	}

	RED_FORCE_INLINE T* operator->() const
	{
		RED_FATAL_ASSERT( m_resource, "Invalid call - should be called only at initialized object" );
		RED_FATAL_ASSERT( m_isResultPending, "Invalid call - can be called only before successfull object retrieval" );
		return m_resource;
	}
};

// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // 

#define DECLARE_RENDER_RESOURCE_ITERATOR \
	public: \
	virtual void Release() override \
	{ \
		if ( 0 == ResourceListRelease( this, m_refCount ) ) \
		{ \
			DestroySelfIfRefCountZero(); \
		} \
	} \
	private:

// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // 