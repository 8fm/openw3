/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "compressionProfiler.h"

#include "bundleBuilderMemory.h"
#include "bundle.h"

#include "../../common/redSystem/cpuid.h"
#include "../../common/redMath/redmathbase.h"

#include "../../common/core/compression/zlib.h"
#include "../../common/core/compression/doboz.h"
#include "../../common/core/compression/snappy.h"
#include "../../common/core/compression/lz4.h"
#include "../../common/core/compression/lz4hc.h"

using namespace Red::Core;

namespace Bundler {

//////////////////////////////////////////////////////////////////////////
// CCompressionProfiler
//////////////////////////////////////////////////////////////////////////
CCompressionProfiler::CCompressionProfiler( Red::Core::BundleDefinition::IBundleDefinition& bundleDef, const AnsiChar* outDir )
:	CProducer< CProfilerWorker, Red::Core::BundleDefinition::CBundleDataContainer* >( bundleDef )
{
	StringAnsi path = outDir;
	path += "compressionProfilingResults.csv";
	
	m_resultsFile = fopen( path.AsChar(), "w" );

	m_compressionName[ Red::Core::Bundle::CT_Uncompressed ]	= "Uncompressed";
	m_compressionName[ Red::Core::Bundle::CT_Zlib ]			= "Zlib";
	m_compressionName[ Red::Core::Bundle::CT_Snappy ]		= "Snappy";
	m_compressionName[ Red::Core::Bundle::CT_Doboz ]		= "Doboz";
	m_compressionName[ Red::Core::Bundle::CT_LZ4 ]			= "LZ4";
	m_compressionName[ Red::Core::Bundle::CT_LZ4HC ]		= "LZ4HC";

	if( m_resultsFile )
	{
		fprintf( m_resultsFile, "Bundle;Resource;File Size;File Size KB" );

		const Uint32 numColumns = 8;
		const AnsiChar* columnHeaders[ numColumns ];

		columnHeaders[ 0 ] = "Compressed Size";
		columnHeaders[ 1 ] = "Compressed Size KB";
		columnHeaders[ 2 ] = "Compression Ratio";
		columnHeaders[ 3 ] = "Compression Time";
		columnHeaders[ 4 ] = "Decompression Time";
		columnHeaders[ 5 ] = "Data Valid";
		columnHeaders[ 6 ] = "Score";
		columnHeaders[ 7 ] = "Score (Calc)";

		for( Uint32 i = 1; i < Red::Core::Bundle::CT_Max; ++i )
		{
			for( Uint32 j = 0; j < numColumns; ++j )
			{
				fprintf( m_resultsFile, ";%hs %hs", m_compressionName[ i ], columnHeaders[ j ] );
			}
		}
	}
}

CCompressionProfiler::~CCompressionProfiler()
{
	if( m_resultsFile )
	{
		fclose( m_resultsFile );
	}
}

void CCompressionProfiler::Initialize( const COptions& )
{

}

void CCompressionProfiler::FillPayload( Red::Core::BundleDefinition::CBundleDataContainer*& payload, const StringAnsi&, Red::Core::BundleDefinition::CBundleDataContainer* bundleData )
{
	payload = bundleData;
}

void CCompressionProfiler::Shutdown( const COptions& )
{

}

void CCompressionProfiler::OnWorkFinished( CProfilerWorker* worker )  
{
	AppendResults( worker->GetResults() );
}

void CCompressionProfiler::AppendResults( const TDynArray< SProfilerItem >& results )
{
	for( Uint32 iResult = 0; iResult < results.Size(); ++iResult )
	{
		const Uint32 row = iResult + 2;

		const SProfilerItem& item = results[ iResult ];

		if( m_resultsFile )
		{
			fprintf
			(
				m_resultsFile,
				"\n%hs;%hs;%u;%Lf",
				item.m_bundle.AsChar(),
				item.m_resource.AsChar(),
				item.m_size,
				item.m_size / 1024.0
			);
		}

		for( Uint32 iCompressionType = 0; iCompressionType < Red::Core::Bundle::CT_Max - 1 ; ++iCompressionType )
		{
			const SProfilerResult& average = item.m_averages[ iCompressionType ];

			if( m_resultsFile )
			{
				const AnsiChar* scoreFormula = ";=(sqrt(R%uC3)*((((1-R%uC[-3])*100)*16)+((100-R%uC[-5])*2))/1000000)";

				fprintf
				(
					m_resultsFile,
					";%u;%Lf;%Lf;%Lf;%Lf;%hs;%Lf",
					static_cast< Uint32 >( average.m_compressedSize + 0.5 ),
					average.m_compressedSize / 1024.0,
					average.m_compressedSize / static_cast< Double >( item.m_size ) * 100.0,
					average.m_compressionTime,
					average.m_decompressionTime,
					average.m_dataValidAfterDecompression? "yes" : "no",
					item.m_score[ iCompressionType ]
				);

				fprintf( m_resultsFile, scoreFormula, row, row, row );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// CProfilerWorker
//////////////////////////////////////////////////////////////////////////
CProfilerWorker::CProfilerWorker( Red::Threads::CSemaphore* lock, const COptions& options )
:	CConsumer< Red::Core::BundleDefinition::CBundleDataContainer* >( "Bundle Profiler", lock )
,	m_numTestIterations( options.GetCompressionIterations() )
{

}

CProfilerWorker::~CProfilerWorker()
{

}

void CProfilerWorker::Do()
{
	const Red::Core::BundleDefinition::CBundleDataContainer* bundleDataContainer = m_payload;

	m_results.ClearFast();
	
	m_feedback.NewProgressbar( bundleDataContainer->GetBundleName() );

	const Uint32 bundleFileCount = bundleDataContainer->GetFileCount();

	CProfilerCommon common;

	if( m_results.Capacity() < bundleFileCount )
	{
		m_results.Reserve( bundleFileCount );
	}

	for( Uint32 iBundleItem = 0; iBundleItem < bundleFileCount; ++iBundleItem )
	{
		const BundleDefinition::SBundleFileDesc* bundleFileDesc = bundleDataContainer->GetBundleFileDescription( iBundleItem );
		const StringAnsi& path = bundleFileDesc->m_resourcePath;

		SProfilerItem item;

		item.m_bundle = bundleDataContainer->GetBundleName();
		item.m_resource = bundleFileDesc->m_resourcePath;

		if( common.Profile( path.AsChar(), item, m_numTestIterations ) )
		{
			m_results.PushBack( item );
		}
		else
		{
			m_feedback.MarkCompleted( Feedback::CS_Failure );
		}

		m_feedback.SetProgress( iBundleItem / static_cast< Float >( bundleFileCount ) );
	}

	m_feedback.MarkCompleted( Feedback::CS_Success );
}

//////////////////////////////////////////////////////////////////////////
// CProfilerCommon
//////////////////////////////////////////////////////////////////////////
CProfilerCommon::CProfilerCommon( CProfilerFeedback* feedback )
:	m_feedback( feedback )
{

}

Bool CProfilerCommon::Profile( const AnsiChar* path, SProfilerItem& item, Uint32 numberOfIterations )
{
	FILE* file = fopen( path, "rb" );

	if( file )
	{
		Profile( file, item, numberOfIterations );

		fclose( file );

		return true;
	}
	else
	{
		BUNDLE_ERROR( m_feedback, TXT( "Could not find file: %hs" ), path );

		if( m_feedback )
		{
			m_feedback->Skip();
		}
	}

	return false;
}

void CProfilerCommon::Profile( FILE* file, SProfilerItem& item, Uint32 numberOfIterations /*= 1 */ )
{
	// Get size of file
	RED_VERIFY( fseek( file, 0, SEEK_END ) == 0, TXT( "Unable to seek to end of file" ) );
	Uint32 size = ftell( file );

	if( size <= c_defaultPoolGranularity )
	{
		// Move read point to start of file
		RED_VERIFY( fseek( file, 0, SEEK_SET ) == 0, TXT( "Unable to seek to start of file" ) );

		// Allocate memory for file
		void* buffer = BUNDLER_MEMORY_ALLOCATE( MC_BundlerProfiler, size );

		RED_VERIFY( fread( buffer, sizeof( Uint8 ), size, file ) == size, TXT( "Amount Read doesn't match amount expected" ) );

		Profile( buffer, size, item, numberOfIterations );

		// Free file buffer
		BUNDLER_MEMORY_FREE( MC_BundlerProfiler, buffer );
	}
}

void CProfilerCommon::Profile( const void* uncompressedDataBuffer, Uint32 uncompressedBufferSize, SProfilerItem& item, Uint32 numberOfIterations )
{
	// Actually do the compression itself
	CompressionTests( uncompressedDataBuffer, uncompressedBufferSize, item, numberOfIterations );

	// Examine the results
	Double highestScore = 0;
	Red::Core::Bundle::ECompressionType best = Red::Core::Bundle::CT_Uncompressed;

	for( Uint32 iCompressionType = 0; iCompressionType < Red::Core::Bundle::CT_Max - 1 ; ++iCompressionType )
	{
		SProfilerItem::CompressionTypeResults& results = item.m_results[ iCompressionType ];
		SProfilerResultStats& stats = item.m_stats[ iCompressionType ];
		SProfilerResult& average = item.m_averages[ iCompressionType ];
		Red::Core::Bundle::ECompressionType compressionType = static_cast< Red::Core::Bundle::ECompressionType >( iCompressionType + 1 );

		CalculateStats( results, stats, numberOfIterations );
		CalculateAverages( average, results, stats, numberOfIterations );
		item.m_score[ iCompressionType ] = CalculateScore( average, item.m_size );

		// Make sure the compressor actually managed to make the size smaller
		if( item.m_averages[ iCompressionType ].m_compressedSize < item.m_size )
		{
			// If its highscore beats the current champ, we have a new contender
			if( highestScore < item.m_score[ iCompressionType ] )
			{
				highestScore = item.m_score[ iCompressionType ];
				best = compressionType;
			}
		}

		if( m_feedback )
		{
			m_feedback->Increment();
		}
	}

	item.m_best = best;
}

void CProfilerCommon::CompressionTests( const void* uncompressedDataBuffer, Uint32 uncompressedBufferSize, SProfilerItem& item, Uint32 numberOfIterations ) const
{
// 	SRedMemory::GetInstance().BeginMetricsDump( TXT( "D:\\temp\\compressionMemMetrics.rmm" ) );
// 	SRedMemory::GetInstance().GetMetricsCollector().AddTrackingTag( "Start_Compression" );

	item.m_size = uncompressedBufferSize;

	Decompressor::Base* decompressors[ Red::Core::Bundle::CT_Max - 1 ] = {};

	decompressors[ Bundle::CT_Zlib - 1 ]	= new Decompressor::CZLib;
	decompressors[ Bundle::CT_Snappy - 1 ]	= new Decompressor::CSnappy;
	decompressors[ Bundle::CT_Doboz - 1 ]	= new Decompressor::CDoboz;
	decompressors[ Bundle::CT_LZ4 - 1 ]		= new Decompressor::CLZ4;
	decompressors[ Bundle::CT_LZ4HC - 1 ]	= new Decompressor::CLZ4HC;

	for( Uint32 iCompressionType = 0; iCompressionType < Red::Core::Bundle::CT_Max - 1 ; ++iCompressionType )
	{
		item.m_results[ iCompressionType ].Resize( numberOfIterations );
	}

	for( Uint32 iTestIteration = 0; iTestIteration < numberOfIterations; ++iTestIteration )
	{
		Compressor::Base* compressors[ Red::Core::Bundle::CT_Max - 1 ] = {};

		compressors[ Bundle::CT_Zlib - 1 ]		= new Compressor::CZLib;
		compressors[ Bundle::CT_Snappy - 1 ]	= new Compressor::CSnappy;
		compressors[ Bundle::CT_Doboz - 1 ]		= new Compressor::CDoboz;
		compressors[ Bundle::CT_LZ4 - 1 ]		= new Compressor::CLZ4;
		compressors[ Bundle::CT_LZ4HC - 1 ]		= new Compressor::CLZ4HC;

		for( Uint32 iCompressionType = 0; iCompressionType < Red::Core::Bundle::CT_Max - 1 ; ++iCompressionType )
		{
			CompressionTest( compressors[ iCompressionType ], decompressors[ iCompressionType ], uncompressedDataBuffer, uncompressedBufferSize, item.m_results[ iCompressionType ][ iTestIteration ] );

			// We're done with this particular compression type
			delete compressors[ iCompressionType ];

			compressors[ iCompressionType ] = nullptr;
		}

		if( m_feedback )
		{
			m_feedback->Increment();
		}
	}

	for( Uint32 iCompressionType = 0; iCompressionType < Red::Core::Bundle::CT_Max - 1 ; ++iCompressionType )
	{
		delete decompressors[ iCompressionType ];
		decompressors[ iCompressionType ] = nullptr;
	}

// 	SRedMemory::GetInstance().GetMetricsCollector().AddTrackingTag( "End_Compression" );
// 	SRedMemory::GetInstance().EndMetricsDump();
}

void CProfilerCommon::CompressionTest( CompressorBase* compressor, DecompressorBase* decompressor, const void* data, Uint32 size, SProfilerResult& result ) const
{
	if ( compressor == nullptr || decompressor == nullptr )
	{
		// Not valid compression, so exclude it.
		result.m_compressionTime = DBL_MAX;
		result.m_compressedSize = DBL_MAX;
		return;
	}


	result.m_compressionTime	= 0.0;
	result.m_compressedSize		= 0.0;

	{
		Red::System::StopClock compressionTimer;

		if( compressor->Compress( data, size ) )
		{
			result.m_compressionTime	= compressionTimer.GetDelta();
			Uint32 compressedSize		= compressor->GetResultSize();
			const void* compressedData	= compressor->GetResult();

			result.m_compressedSize		= static_cast< Double >( compressedSize );

			void* uncompressedData		= BUNDLER_MEMORY_ALLOCATE( MC_BundlerProfiler, size );

			Red::System::StopClock decompressionTimer;
			if
			(
				decompressor->Initialize( compressedData, uncompressedData, compressedSize, size ) == Red::Core::Decompressor::Status_Success &&
				decompressor->Decompress() == Red::Core::Decompressor::Status_Success
			)
			{
				result.m_decompressionTime				= decompressionTimer.GetDelta();
				result.m_dataValidAfterDecompression	= Red::System::MemoryCompare( data, uncompressedData, size ) == 0;
			}
			else
			{
				result.m_decompressionTime				= 0.0;
				result.m_dataValidAfterDecompression	= false;
			}

			BUNDLER_MEMORY_FREE( MC_BundlerProfiler, uncompressedData );
		}
	}
}

void CProfilerCommon::CalculateStats( SProfilerItem::CompressionTypeResults& results, SProfilerResultStats& stats, Uint32 numberOfIterations ) const
{
	// Sort based on decompression time
	Sort( results.Begin(), results.End(), []( const SProfilerResult& a, const SProfilerResult& b ) { return a.m_decompressionTime < b.m_decompressionTime; } );

	stats.m_median			= 0.0;
	stats.m_lowerQuartile	= 0.0;
	stats.m_upperQuartile	= 0.0;
	stats.m_upperFence		= 0.0;
	stats.m_lowerFence		= 0.0;

	if( numberOfIterations % 2 == 0 )
	{
		// If you have an array of 8 items, you want indices 3 and 4 which represent the middle
		// (0,1,2) 3|4 (5,6,7)
		Uint32 medianIndexB = numberOfIterations / 2;
		Uint32 medianIndexA = medianIndexB - 1;

		const SProfilerResult& medianA = results[ medianIndexA ];
		const SProfilerResult& medianB = results[ medianIndexB ];

		stats.m_median = ( medianA.m_decompressionTime + medianB.m_decompressionTime ) / 2.0;

		// (0) 1|2 (3,4,5,6,7)
		Uint32 lowerQuartileIndexB = numberOfIterations / 4;
		Uint32 lowerQuartileIndexA = lowerQuartileIndexB - 1;

		const SProfilerResult& q1a = results[ lowerQuartileIndexA ];
		const SProfilerResult& q1b = results[ lowerQuartileIndexB ];

		stats.m_lowerQuartile = ( q1a.m_decompressionTime + q1b.m_decompressionTime ) / 2.0;

		// (0,1,2,3,4) 5|6 (7)
		Uint32 upperQuartileIndexB = ( numberOfIterations / 4 ) * 3;
		Uint32 upperQuartileIndexA = upperQuartileIndexB - 1;

		const SProfilerResult& q3a = results[ upperQuartileIndexA ];
		const SProfilerResult& q3b = results[ upperQuartileIndexB ];

		stats.m_upperQuartile = ( q3a.m_decompressionTime + q3b.m_decompressionTime ) / 2.0;
	}
	else
	{
		const SProfilerResult& medianResult = results[ ( numberOfIterations / 2 ) ];
		stats.m_median = medianResult.m_decompressionTime;

		const SProfilerResult& lowerQuartileResult = results[ numberOfIterations / 4 ];
		stats.m_upperFence = lowerQuartileResult.m_decompressionTime;

		const SProfilerResult& upperQuartileResult = results[ ( numberOfIterations / 4 ) * 3 ];
		stats.m_upperQuartile = upperQuartileResult.m_decompressionTime;
	}

	stats.m_interQuartile = stats.m_upperQuartile - stats.m_lowerQuartile;

	// If there is still too much variance, we can switch to the innerFence range which is 1.5 (as opposed to 3)
	Double outerFenceRange = stats.m_interQuartile * 3;

	stats.m_lowerFence = stats.m_lowerQuartile - outerFenceRange;
	stats.m_upperFence = stats.m_upperQuartile + outerFenceRange;
}

void CProfilerCommon::CalculateAverages( SProfilerResult& average, const SProfilerItem::CompressionTypeResults& results, const SProfilerResultStats& stats, Uint32 numberOfIterations ) const
{
	// Total up averages
	Uint32 startIndex = 0;
	for( Uint32 iTestIteration = 0; iTestIteration < numberOfIterations; ++iTestIteration )
	{
		const SProfilerResult& result = results[ iTestIteration ];

		if( result.m_decompressionTime < stats.m_lowerFence )
		{
			++startIndex;
		}
		else
		{
			break;
		}
	}

	Uint32 endIndex = numberOfIterations;
	for( Uint32 iTestIteration = endIndex - 1; iTestIteration > startIndex; --iTestIteration )
	{
		const SProfilerResult& result = results[ iTestIteration ];

		if( result.m_decompressionTime > stats.m_upperFence )
		{
			--endIndex;
		}
		else
		{
			break;
		}
	}

	Double numIterations = static_cast< Double >( endIndex - startIndex );
	for( Uint32 iTestIteration = startIndex; iTestIteration < endIndex; ++iTestIteration )
	{
		const SProfilerResult& result = results[ iTestIteration ];

		average.m_compressionTime				+= result.m_compressionTime / numIterations;
		average.m_decompressionTime				+= result.m_decompressionTime / numIterations;
		average.m_compressedSize				+= result.m_compressedSize / numIterations;
		average.m_dataValidAfterDecompression	&= result.m_dataValidAfterDecompression;
	}
}

Double CProfilerCommon::CalculateScore( const SProfilerResult& result, Uint32 uncompressedSize ) const
{
	//const AnsiChar* scoreFormula = ;=(sqrt(R%uC3)*((((1-R%uC[-2])*100)*16)+((100-R%uC[-4])*2))/1000000);

	Double score = 
		Red::Math::MSqrt( static_cast< Float >( uncompressedSize ) ) *
		(
		( ( 1.0 - result.m_decompressionTime ) * 100 * 16 ) +
		( ( 100.0 - ( ( result.m_compressedSize ) / static_cast< Double >( uncompressedSize ) * 100.0 ) ) * 2.0 )
		) / 1000000.0;

	return score;
}

//////////////////////////////////////////////////////////////////////////
// CProfilerFeedback
//////////////////////////////////////////////////////////////////////////

CProfilerFeedback::CProfilerFeedback( Feedback& feedback, Uint32 numItemsInBundle )
:	m_feedback( feedback )
,	m_completedIterations( 0 )
,	m_numItems( numItemsInBundle )
,	m_testIterations( Red::Core::Bundle::CT_Max - 1 )
,	m_scoreIterations( Red::Core::Bundle::CT_Max - 1 )
,	m_totalIterations( ( m_scoreIterations + m_testIterations ) * m_numItems )
{

}

CProfilerFeedback::~CProfilerFeedback()
{

}

void CProfilerFeedback::Increment()
{
	++m_completedIterations;

	Update();
}

void CProfilerFeedback::Skip()
{
	m_completedIterations += ( m_scoreIterations + m_testIterations );

	Update();
}

void CProfilerFeedback::MarkCompleted( Feedback::ECompletionState success )
{
	m_feedback.MarkCompleted( success );
}

void CProfilerFeedback::Update()
{
	m_feedback.SetProgress( m_completedIterations / static_cast< Float >( m_totalIterations ) );
}

} // namespace Bundler {
