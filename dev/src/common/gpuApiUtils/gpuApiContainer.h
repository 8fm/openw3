/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "..\redThreads\redThreadsAtomic.h"
#include "..\redThreads\redThreadsThread.h"

namespace GpuApi
{

	template< typename _Type, Uint32 _DefaultPoolSize >
	class ResourceContainer
	{
	public:
		enum { _MaxResCount = _DefaultPoolSize };

	public:
		ResourceContainer ( Red::Threads::CMutex& mutex )
			: m_pMutex( mutex )
			, m_UnusedCountDefault( 0 )
		{	
			Uint32 indexGen = 0;

			// Init default pool
			m_UnusedCountDefault.SetValue( _DefaultPoolSize );
			for ( Uint32 i=0; i<_DefaultPoolSize; ++i )
			{
				m_UnusedDefaultIndices[i] = indexGen++;
			}

			GPUAPI_ASSERT( IsEmpty() );
		}

		/// Is given id in use (id 0 is never in use)
		Uint32 IsInUse( Uint32 id ) const
		{
			return IsValidId(id) && IdToIndex(id) < _MaxResCount && m_Resources[IdToIndex(id)].refCount.GetValue() >= 0;
		}

		/// Is container empty?
		Bool IsEmpty() const
		{
			GPUAPI_ASSERT( m_UnusedCountDefault.GetValue() <= _MaxResCount );
			return ( _MaxResCount == m_UnusedCountDefault.GetValue());
		}

		/// Will creation of given number of objects succeed
		Bool IsCapableToCreate( Uint32 testCount )
		{
			Uint32 unusedCount = m_UnusedCountDefault.GetValue();
			return testCount && testCount <= unusedCount;
		}

		/// Create new resource, returns id.
		Uint32 Create( Int32 initialRefCount )
		{
			GPUAPI_ASSERT( initialRefCount >= 0 );

			// Lock mutex
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_pMutex );

			// Pop unused index
			
			Uint32 newIndex = _MaxResCount;
			if ( m_UnusedCountDefault.GetValue() > 0 )
			{
				newIndex = m_UnusedDefaultIndices[ m_UnusedCountDefault.Decrement() ];
				GPUAPI_ASSERT( newIndex < _MaxResCount );
			}

			// Leave if allocation failed
			if ( newIndex >= _MaxResCount )
			{
				Uint32 result = 0;
				GPUAPI_ASSERT( !IsValidId(result) );
				GPUAPI_LOG_WARNING( TXT( "Failed to create resource" ) );
				return result;
			}

			// Build id from index
			Uint32 newId = IndexToId( newIndex );

			// Init internal data
			SData &data = m_Resources[newIndex];
			GPUAPI_ASSERT( -1 == data.refCount.GetValue() );
			data.refCount.SetValue( initialRefCount );
			GPUAPI_ASSERT( data.refCount.GetValue() >= 0 );

			// Finalize
			GPUAPI_ASSERT( IsInUse( newId ) );
			return newId;
		}

		/// Destroy resource with given id
		void Destroy( Uint32 id )
		{
			GPUAPI_ASSERT( IsInUse(id) );
			Uint32 index = IdToIndex( id );
			GPUAPI_ASSERT( 0 == m_Resources[index].refCount.GetValue() );

			// Lock mutex
			Red::Threads::CScopedLock< Red::Threads::CMutex >  lock( m_pMutex );

			// Give back resource index
			if ( m_UnusedCountDefault.GetValue() >= _DefaultPoolSize )
			{
				GPUAPI_HALT( "GPUAPI RESOURCE CONTAINER MEMORY WILL GET CORRUPTED IN A MOMENT!!!" );
			}
			m_UnusedDefaultIndices[ m_UnusedCountDefault.Increment() - 1 ] = index;

			// Reset resource
			m_Resources[index].refCount.SetValue( -1 );
			m_Resources[index].data = _Type();

			// Post check
			GPUAPI_ASSERT( !IsInUse(id) );
		}

#if WAITING_FOR_DEX_TO_FIX_GLOBAL_TEXTURES
		/// Destroy all identifiers
		void DestroyAll()
		{
			CMutexScopedLock lock( m_pMutex );

			if ( !IsEmpty() )
			{
				for ( Uint32 index=0; index<_MaxResCount; ++index )
				{
					SData &data = m_Resources[index];
					GPUAPI_ASSERT( 0 != data.refCount.GetValue() );
					if ( data.refCount.GetValue() < 0 )
					{
						continue;
					}

					data.refCount.SetValue( 0 ); //< hack to prevent ASSERT in destroy
					Destroy( IndexToId(index) );
				}
			}

			GPUAPI_ASSERT( IsEmpty() );
		}
#endif

		/// Returns data for given id.
		_Type& Data( Uint32 id )
		{
			GPUAPI_ASSERT( IsInUse(id) );
			return m_Resources[IdToIndex(id)].data;
		}	

		/// Increments ref count, returns new value
		Int32 IncRefCount( Uint32 id )
		{
			GPUAPI_ASSERT( IsInUse(id) );
			SData &data = m_Resources[IdToIndex(id)];
			GPUAPI_ASSERT( data.refCount.GetValue() >= 0 );
			// This is used from multiple threads but it's thread safe because Red::Threads::CAtomic< Int32 > is used
			//Int32 newCount = data.refCount.Increment();
			//GPUAPI_ASSERT( newCount == data.refCount.GetValue() );
			//return newCount;
			return data.refCount.Increment();
		}

		/// Decrements ref count, returns new value
		Int32 DecRefCount( Uint32 id )
		{
			GPUAPI_ASSERT( IsInUse(id) );
			SData &data = m_Resources[IdToIndex(id)];
			GPUAPI_ASSERT( data.refCount.GetValue() >= 0 );
			// This is used from multiple threads but it's thread safe because Red::Threads::CAtomic< Int32 > is used
			//Int32 newCount = data.refCount.Decrement();
			//GPUAPI_ASSERT( newCount == data.refCount.GetValue() );
			//return newCount;
			return data.refCount.Decrement();
		}

		/// Get ref count
		Int32 GetRefCount( Uint32 id ) const
		{
			GPUAPI_ASSERT( IsInUse(id) );
			return m_Resources[IdToIndex(id)].refCount.GetValue();
		}

		/// Find resource with given description. If found, then adds reference to guarantee thread safety for procedures using this functions.
		template< typename _DescType >
		Uint32 FindDescAddRef( const _DescType &desc )
		{
			// Lock mutex
			Red::Threads::CScopedLock< Red::Threads::CMutex >  lock( m_pMutex );
			
			// ace_optimize: we could store used indices data (in the same tables as unused indices)
			for ( Uint32 index = 0; index < _MaxResCount; ++index )
			{
				while ( 1 )
				{
					Int32 refCount = m_Resources[ index ].refCount.GetValue();
					const Bool isInUse = (refCount > 0);
					if ( isInUse && m_Resources[index].data.m_Desc == desc )
					{
						// Try to AddRef
						if ( m_Resources[ index ].refCount.CompareExchange( refCount + 1, refCount ) == refCount )
						{
							return IndexToId( index );
						}
					}
					else
					{
						break;
					}
				}
			}

			
			return 0;
		}

		/// Get all
		void GetAllAddRef( Uint32 &outNum, Uint32 *&outBuffer )
		{
			// Lock mutex
			Red::Threads::CScopedLock< Red::Threads::CMutex >  lock( m_pMutex );

			// Calculate needed capacity
			const Uint32 totalResources = GetUsedCount();

			// Allocate buffer
			outNum = 0;
			outBuffer = totalResources > 0 ? new Uint32[ totalResources ] : nullptr;

			// Get all
			for ( Uint32 index=0; index<_MaxResCount && outNum < totalResources; ++index )
			{
				const Bool isInUse = (m_Resources[index].refCount.GetValue() >= 0);
				if ( isInUse )
				{
					Uint32 resultId = IndexToId( index );
					GPUAPI_ASSERT( IsValidId(resultId) );
					GPUAPI_ASSERT( IdToIndex(resultId) == index );
					m_Resources[index].refCount.Increment();	
					GPUAPI_ASSERT( outNum < totalResources );
					outBuffer[outNum] = resultId;
					++outNum;
				}
			}

			GPUAPI_ASSERT( outNum == totalResources );
		}

		/// Get the number of resources that are in-use.
		Uint32 GetUsedCount() const
		{
			return _MaxResCount - m_UnusedCountDefault.GetValue();
		}

	protected:
		/// Is given id valid (not reserved)
		static RED_FORCE_INLINE bool IsValidId( Uint32 id )
		{
			return 0 != id;
		}

		/// Translates given id to index
		static RED_FORCE_INLINE Uint32 IdToIndex( Uint32 id ) 
		{ 
			GPUAPI_ASSERT( id > 0 ); 
			return id - 1; 
		}

		/// Translates given index to id
		static RED_FORCE_INLINE Uint32 IndexToId( Uint32 index )
		{
			return index + 1;
		}

	private:
		/// SData
		struct SData
		{
			SData ()
				: refCount( -1 )
			{}

			Red::Threads::CAtomic< Int32 > refCount;	// -1 for unused data
			_Type data;
		};

		Red::Threads::CMutex&	m_pMutex;
		Red::Threads::CAtomic< Int32 > m_UnusedCountDefault;
		SData	m_Resources[_MaxResCount];
		Uint32	m_UnusedDefaultIndices[_DefaultPoolSize];
	};

}
