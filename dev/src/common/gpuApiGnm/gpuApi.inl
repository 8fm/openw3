#pragma once


namespace GpuApi {

#ifdef FENCE_MANAGER_TEMPL
#	error FENCE_MANAGER_TEMPL already defined :/
#endif

#define FENCE_MANAGER_TEMPL( Ret ) template< Uint32 CONTEXT_NUM_BITS, Uint32 COUNTER_NUM_BITS, Uint32 BUCKET_NUM_BITS > Ret CFenceManager< CONTEXT_NUM_BITS, COUNTER_NUM_BITS, BUCKET_NUM_BITS >

	FENCE_MANAGER_TEMPL()::CFenceManager()
	{
		// Each marker needs to be 8-byte aligned...
		m_markers = static_cast< Uint32* >( GPU_API_ALLOCATE( GpuMemoryPool_GPUInternal, MC_Label, NUM_CONTEXTS * NUM_BUCKETS * 8, 8 ) );
		Red::System::MemoryZero( m_markers, NUM_CONTEXTS * NUM_BUCKETS * 8 );

		for ( Uint32 i = 0; i < NUM_CONTEXTS; ++i )
		{
			m_lastValue[ i ].m_index = 0;
			m_lastValue[ i ].m_data.m_context = i;
		}
	}

	FENCE_MANAGER_TEMPL()::~CFenceManager()
	{
		GPU_API_FREE( GpuMemoryPool_GPUInternal, MC_Label, m_markers );
		m_markers = nullptr;
	}


	FENCE_MANAGER_TEMPL( Uint64 )::GetFence( Uint32 context, Uint32*& outAddressToWrite, Uint32& outValueToWrite )
	{
		RED_FATAL_ASSERT( context < NUM_CONTEXTS, "Fence context out of range! %u >= %u", context, NUM_CONTEXTS );

		SFenceValue& lastValue = m_lastValue[ context ];

		// Increment first. m_counter = 0 isn't allowed, otherwise it would always be non-pending (no marker value would be less).
		++lastValue.m_data.m_counter;

		// If we wrapped, update marker index
		if ( lastValue.m_data.m_counter == 0 )
		{
			// See above, can't have counter == 0
			lastValue.m_data.m_counter = 1;

			++lastValue.m_data.m_bucket;
			RED_FATAL_ASSERT( lastValue.m_data.m_bucket != 0, "Bucket index has wrapped around!" );

			// We have to assume that there isn't still an old pending fence waiting to write here. That would require overflowing
			// the counter NUM_BUCKETS times without the gpu processing anything, so it's not likely to happen.
			*GetMarkerForFence( lastValue ) = 0;
		}

		outAddressToWrite = GetMarkerForFence( lastValue );
		outValueToWrite = lastValue.m_data.m_counter;

		return lastValue.m_index;
	}

	FENCE_MANAGER_TEMPL( Bool )::CanGetFence( Uint32 context ) const
	{
		RED_FATAL_ASSERT( context < NUM_CONTEXTS, "Fence context out of range! %u >= %u", context, NUM_CONTEXTS );

		SFenceValue testValue = m_lastValue[ context ];
		++testValue.m_data.m_counter;
		++testValue.m_data.m_bucket;
		// If counter != 0, then we wouldn't increment bucket to get a fence. If m_bucket != 0, then we didn't exceed max bucket count.
		return ( testValue.m_data.m_counter != 0 ) || ( testValue.m_data.m_bucket != 0 );
	}


	FENCE_MANAGER_TEMPL( Bool )::IsFencePending( Uint64 fence ) const
	{
		SFenceValue fenceValue;
		fenceValue.m_index = fence;

		const SFenceValue& lastValue = m_lastValue[ fenceValue.m_data.m_context ];
		RED_UNUSED( lastValue );

		GPUAPI_ASSERT( fenceValue.m_data.m_counter != 0, TXT("Fence counter shouldn't be 0") );
		GPUAPI_FATAL_ASSERT( fenceValue.m_data.m_bucket <= lastValue.m_data.m_bucket, "Checking a fence from the future!" );
		GPUAPI_FATAL_ASSERT( fenceValue.m_data.m_bucket < lastValue.m_data.m_bucket || fenceValue.m_data.m_counter <= lastValue.m_data.m_counter, "Checking a fence from the future!" );

		//If the fence is too old, we just count it as finished. Can't really recover otherwise (marker value has been recycled).
		if ( IsOldValue( fenceValue ) )
		{
			GPUAPI_LOG_WARNING( TXT("Using a fence that has been around far too long! Forced not pending") );
			return false;
		}

		const volatile Uint32* marker = GetMarkerForFence( fenceValue );

		return *marker < fenceValue.m_data.m_counter;
	}

	FENCE_MANAGER_TEMPL( Bool )::IsFenceExpired( Uint64 fence ) const
	{
		SFenceValue fenceValue;
		fenceValue.m_index = fence;

		const SFenceValue& lastValue = m_lastValue[ fenceValue.m_data.m_context ];
		RED_UNUSED( lastValue );

		GPUAPI_ASSERT( fenceValue.m_data.m_counter != 0, TXT("Fence counter shouldn't be 0") );
		GPUAPI_FATAL_ASSERT( fenceValue.m_data.m_bucket <= lastValue.m_data.m_bucket, "Checking a fence from the future!" );
		GPUAPI_FATAL_ASSERT( fenceValue.m_data.m_bucket < lastValue.m_data.m_bucket || fenceValue.m_data.m_counter <= lastValue.m_data.m_counter, "Checking a fence from the future!" );

		return IsOldValue( fenceValue );
	}


	FENCE_MANAGER_TEMPL( void )::GetFenceInfo( Uint64 fence, Uint32*& outAddressToRead, Uint32& outValueToCompare ) const
	{
		SFenceValue fenceValue;
		fenceValue.m_index = fence;

		const SFenceValue& lastValue = m_lastValue[ fenceValue.m_data.m_context ];
		RED_UNUSED( lastValue );

		GPUAPI_ASSERT( fenceValue.m_data.m_counter != 0, TXT("Fence counter shouldn't be 0") );
		GPUAPI_FATAL_ASSERT( fenceValue.m_data.m_bucket <= lastValue.m_data.m_bucket, "Checking a fence from the future!" );
		GPUAPI_FATAL_ASSERT( fenceValue.m_data.m_bucket < lastValue.m_data.m_bucket || fenceValue.m_data.m_counter <= lastValue.m_data.m_counter, "Checking a fence from the future!" );

		if ( IsOldValue( fenceValue ) )
		{
			GPUAPI_LOG_WARNING( TXT("Using a fence that has been around far too long! Forced not pending") );
			// If it's old, just give one of our markers, and compare to 0, so that it'll be seen as finished.
			outAddressToRead = &m_markers[ 0 ];
			outValueToCompare = 0;
		}
		else
		{
			outAddressToRead = GetMarkerForFence( fenceValue );
			outValueToCompare = fenceValue.m_data.m_counter;
		}
	}


	FENCE_MANAGER_TEMPL( Bool )::IsOldValue( const SFenceValue& value ) const
	{
		// If the value's bucket is older than all our current buckets, the value is too old and we can't track it any longer.
		return value.m_data.m_bucket + NUM_BUCKETS <= m_lastValue[ value.m_data.m_context ].m_data.m_bucket;
	}


	FENCE_MANAGER_TEMPL( Uint32* )::GetMarkerForFence( const SFenceValue& value ) const
	{
		// Each marker needs to be 8-byte aligned, even though only 32 bits...
		const Uint32 bucket = value.m_data.m_bucket & BUCKETS_BITMASK;
		return &m_markers[ ( value.m_data.m_context * NUM_BUCKETS + bucket ) * 2 ];
	}


#undef FENCE_MANAGER_TEMPL

}
