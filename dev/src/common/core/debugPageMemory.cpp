/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "filePath.h"
#include "depot.h"
#include "debugPageHandler.h"
#include "debugPageHTMLDoc.h"
#include "httpResponseData.h"

#ifndef NO_DEBUG_PAGES

class CDebugPageMemory : public IDebugPageHandlerHTML
{
public:
	CDebugPageMemory()
		: IDebugPageHandlerHTML( "/memory/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Memory pools"; }
	virtual StringAnsi GetCategory() const override { return "Core"; }
	virtual Bool HasIndexPage() const override { return true; }

	// information about import
	class MemInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		StringAnsi		m_class;
		Int64			m_allocs;
		Int64			m_size;

	public:
		MemInfo( const StringAnsi& memoryClass, const Int64 allocs, const Int64 size )
			: m_class( memoryClass)
			, m_allocs( allocs )
			, m_size( size )
		{
		}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID ) override
		{
			const auto* b = ((const MemInfo*)other);

			switch ( columnID )
			{
				case 1: return m_class < b->m_class;
				case 2: return m_allocs < b->m_allocs;
			}
			return m_size < b->m_size; // 3
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID ) override
		{
			switch ( columnID )
			{
				case 1: doc.Write(m_class.AsChar()); break;
				case 2: doc.Writef("%d", (Int32)m_allocs); break;
				case 3: doc.Writef("%1.2f KB", (Double)m_size / 1024.0f); break;
			}
		}
	};

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// get general memory statistics
		{
			CDebugPageHTMLInfoBlock info( doc, "General information" );

			const Uint64 size = Memory::GetTotalBytesAllocated();
			info.Info( "Total allocated memory: ").Writef( "%1.2f MB", (Double)size  / (1024.0*1024.0) );

#ifdef ENABLE_EXTENDED_MEMORY_METRICS
			{
				Uint64 numTotalAllocations = 0;
				for ( Uint32 poolIndex=0; poolIndex<Memory::GetPoolCount(); ++poolIndex )
				{
					if ( poolIndex != Memory::GetPoolLabel<MemoryPool_SmallObjects >() )
					{
						numTotalAllocations += Memory::GetTotalAllocations( poolIndex );
					}
				}
				info.Info( "Total allocator overhead: " ).Writef( "%1.2f KB", (numTotalAllocations * 24) / 1024.0f );
			}
#endif
		}

		// per pool statistics
		const Uint32 numPools = Memory::GetPoolCount();
		for ( Uint32 poolIndex=0; poolIndex<numPools; ++poolIndex )
		{
			const AnsiChar* poolName = Memory::GetPoolName( (Red::MemoryFramework::PoolLabel) poolIndex );
	
			CDebugPageHTMLInfoBlock info( doc, "Pool '%hs'", poolName );
			info.Info( "Total bytes: " ).Writef( "%1.2fKB", Memory::GetPoolTotalBytesAllocated( poolIndex ) / 1024.0f );
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
			info.Info( "Peek bytes: " ).Writef( "%1.2fKB", Memory::GetPoolTotalBytesAllocatedPeak( poolIndex ) / 1024.0f );
			info.Info( "Total allocations: " ).Writef( "%d", Memory::GetTotalAllocations( poolIndex ) );
			info.Info( "Peek allocations: " ).Writef( "%d", Memory::GetPoolTotalBytesAllocatedPeak( poolIndex ) );
			if ( poolIndex != Memory::GetPoolLabel<MemoryPool_SmallObjects>() )
			{
				info.Info( "Allocator overhead: " ).Writef( "%1.2f KB", (Memory::GetTotalAllocations( poolIndex ) * 24) / 1024.0f );
			}
			info.Info( "" ).Link( "/memory/?pool=%hs&action=fragmentation", poolName ).Write( "Show fragmentation" );
#endif

			// get number of allocation for given memory class
			if ( Memory::GetPoolTotalBytesAllocated( poolIndex ) > 0 )
			{
				// used to calculate the "unaccounted for" memory
				Uint64 totalBytesAllocated = 0;
				Uint64 totalAllocations =  0;

				// colums
				CDebugPageHTMLTable table( doc, "mem" );
				table.AddColumn( "Class", 1.0f, true );
				table.AddColumn( "Allocs", 120, true );
				table.AddColumn( "Size", 120, true );

				// per class stats
				for ( Uint32 j=0; j<Red::MemoryFramework::k_MaximumMemoryClasses; ++j )
				{
					// get name of the memory class - can be empty
					const AnsiChar* memoryClassName = Memory::GetMemoryClassName( j );
					if ( !memoryClassName || !memoryClassName[0] )
						continue;

#ifdef ENABLE_EXTENDED_MEMORY_METRICS
					const Uint64 classBytes = Memory::GetAllocatedBytesPerMemoryClass( j, poolIndex );
					const Uint64 classAllocs = Memory::GetAllocationPerMemoryClass( j, poolIndex );
					if ( classAllocs > 0 )
					{
						table.AddRow( new MemInfo( memoryClassName, classAllocs, classBytes ) );
						totalBytesAllocated += classBytes;
						totalAllocations += classAllocs;
					}
#endif
				}

#ifdef ENABLE_EXTENDED_MEMORY_METRICS
				// leftovers ?
				if ( totalBytesAllocated != Memory::GetPoolTotalBytesAllocated( poolIndex ) )
				{
					const Int64 leftOverBytes = (Int64)Memory::GetPoolTotalBytesAllocated( poolIndex ) - (Int64)totalBytesAllocated;
					const Int64 leftOverAllocs = (Int64)Memory::GetTotalAllocations( poolIndex ) - (Int64)totalAllocations;
					table.AddRow( new MemInfo( "Inaccuracy", leftOverAllocs, leftOverBytes) );
				}
#endif

				// render table
				table.Render( 400, "generic", fullURL );
			}
		}


		return true;
	}
};

void InitMemoryDebugPages()
{
	new CDebugPageMemory(); // autoregister
}

#endif