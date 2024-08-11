/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

namespace DebugWindows
{
	class MemoryMetricsUsageControlWalker : public Red::MemoryFramework::PoolAreaWalker
	{
	public:
		struct Area
		{
			Red::MemoryFramework::IAllocator* m_allocator;
			MemUint m_address;
			MemSize m_size;

			Area( Red::MemoryFramework::IAllocator* poolAllocator, MemUint address, MemSize size )
			:	m_allocator( poolAllocator )
			,	m_address( address )
			,	m_size( size )
			{
			}

			RED_INLINE Bool operator==( const Area& other ) const { return m_address == other.m_address; }
		};

		struct Block
		{
			MemUint address;
			Uint32 metricsStart;
			Uint32 metricsCount;

			RED_INLINE Bool operator==( const Block& other ) const
			{
				return address == other.address;
			}
		};

		struct BlockMetrics
		{
			Uint32 size;
			Uint16 memoryClass;

			Bool used		: 1;
			Bool locked		: 1;
			Bool free		: 1;

			RED_INLINE Bool operator==( const BlockMetrics& other ) const
			{
				return size != other.size && memoryClass != other.memoryClass && used != other.used && locked != other.locked && free != other.free;
			}

			RED_INLINE Bool operator!=( const BlockMetrics& other ) const { return !operator==( other ); }

			RED_INLINE Bool operator<( const BlockMetrics& other ) const
			{
				return size < other.size;
			}
		};

		MemoryMetricsUsageControlWalker();
		virtual ~MemoryMetricsUsageControlWalker();

		void AddArea( Red::MemoryFramework::IAllocator* poolAllocator, MemUint address, MemSize size );

		void SetResolution( Uint32 x, Uint32 y );
		void Clear();

		void Walk();

		void ZoomIn( MemUint address );
		void ZoomOut();

		Uint32 BlockIndexFromAddress( MemUint address ) const;

		RED_INLINE Uint32 GetBytesPerBlock() const { return m_bytesPerBlock; }
		RED_INLINE MemSize GetLargestFreeArea() const { return m_largestFreeArea; }

		RED_INLINE const Block& GetBlock( Uint32 index ) const { return m_blocks[ index ]; }
		RED_INLINE const Block& GetBlock( Uint32 x, Uint32 y ) const { return m_blocks[ x + ( y * m_width ) ]; }
		RED_INLINE Uint32 GetNumBlocks() const { return m_blocks.Size(); }

		RED_INLINE Uint32 GetWidth() const { return m_width; }
		RED_INLINE Uint32 GetHeight() const { return m_height; }

		RED_INLINE Uint32 GetNumMetricsBlocks() const { return m_metrics.Size(); }
		RED_INLINE const BlockMetrics* GetMetricsForBlock( const Block& block, Uint32 index = 0 ) const
		{
			if( block.metricsStart < GetNumMetricsBlocks() && block.metricsCount > 0 )
			{
				return &m_metrics[ block.metricsStart + index ];
			}

			return nullptr;
		}

	protected:
		void OnAreaCommon( MemUint address, MemSize size, std::function< void( BlockMetrics& ) > callback );
		virtual void OnUsedArea( MemUint address, MemSize size, Uint16 memoryClass ) override final;
		virtual void OnFreeArea( MemUint address, MemSize size ) override final;
		virtual void OnLockedArea( MemUint address, MemSize size ) override final;

	private:
		Uint32 CalculateBytesPerBlock() const;

		template< typename TA, typename TB >
		RED_INLINE Uint32 SafeDivide( TA a, TB b ) const
		{
			// First calculate it as a float as we will need to round up the remainder 
			Float resultFlt = a / static_cast< Float >( b );

			// Add 0.1f to avoid any floating point rounding errors
			return static_cast< Uint32 >( MCeil( resultFlt ) + 0.1f );
		}

		TDynArray< Area, MC_Debug, MemoryPool_Debug > m_areas;
		TDynArray< Block, MC_Debug, MemoryPool_Debug > m_blocks;
		TDynArray< BlockMetrics, MC_Debug, MemoryPool_Debug > m_metrics;

		MemSize m_totalAreaSize;

		Uint32 m_width;
		Uint32 m_height;
		Uint32 m_bytesPerBlock;

		MemSize m_largestFreeArea;
		MemUint m_zoomAddress;
		Uint32 m_zoom;

		// Used during walk

		struct WalkData
		{
			MemSize m_totalSizeUsage;
			MemSize m_currentBlockUsage;

			Uint32 m_previousBlockTouched;
			Uint32 m_areaIndex;

			Uint32 m_blockIndexShift;

			Uint32 m_metricsCount;
			Uint32 m_nextFreeMetricsBlock;
			
			Bool m_zoomOnThisArea;
		};

		WalkData m_walkData;
	};
}
