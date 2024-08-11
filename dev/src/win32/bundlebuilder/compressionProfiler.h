/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _RED_COMPRESSION_PROFILER_H_
#define _RED_COMPRESSION_PROFILER_H_

#include "../../common/core/core.h"

#include "../../common/core/bundleHeader.h"
#include "../../common/core/bundledefinition.h"
#include "../../common/core/compression/compression.h"

#include "feedback.h"
#include "options.h"
#include "producer.h"
#include "consumer.h"

namespace Bundler
{
	//////////////////////////////////////////////////////////////////////////
	// Profiling data
	//////////////////////////////////////////////////////////////////////////
	struct SProfilerResult
	{
		Double m_compressionTime;
		Double m_decompressionTime;

		Double m_compressedSize;

		Bool m_dataValidAfterDecompression;

		SProfilerResult()
			:	m_compressionTime( 0.0 )
			,	m_decompressionTime( 0.0 )
			,	m_compressedSize( 0 )
			,	m_dataValidAfterDecompression( true )
		{
		}
	};

	struct SProfilerResultStats
	{
		Double m_median;
		Double m_lowerQuartile;
		Double m_upperQuartile;
		Double m_interQuartile;
		Double m_upperFence;
		Double m_lowerFence;
	};

	struct SProfilerItem
	{
		StringAnsi m_bundle;
		StringAnsi m_resource;
		Uint32 m_size;

		typedef TDynArray< SProfilerResult > CompressionTypeResults;

		CompressionTypeResults m_results[ Red::Core::Bundle::CT_Max - 1 ];
		SProfilerResult m_averages[ Red::Core::Bundle::CT_Max - 1 ];
		SProfilerResultStats m_stats[ Red::Core::Bundle::CT_Max - 1 ];
		Double m_score[ Red::Core::Bundle::CT_Max - 1 ];
		Red::Core::Bundle::ECompressionType m_best;
	};

	//////////////////////////////////////////////////////////////////////////
	// CProfilerWorker
	//////////////////////////////////////////////////////////////////////////
	class CProfilerWorker : public CConsumer< Red::Core::BundleDefinition::CBundleDataContainer* >
	{
	public:
		CProfilerWorker( Red::Threads::CSemaphore* lock, const COptions& options );
		virtual ~CProfilerWorker();

		RED_INLINE const TDynArray< SProfilerItem >& GetResults() const { return m_results; }

	private:
		virtual void Do() override final;

	private:
		TDynArray< SProfilerItem > m_results;

		Uint32 m_numTestIterations;
	};

	//////////////////////////////////////////////////////////////////////////
	// CCompressionProfiler
	//////////////////////////////////////////////////////////////////////////
	class CCompressionProfiler : public CProducer< CProfilerWorker, Red::Core::BundleDefinition::CBundleDataContainer* >
	{
	public:
		CCompressionProfiler( Red::Core::BundleDefinition::IBundleDefinition& bundleDef, const AnsiChar* outDir );
		virtual ~CCompressionProfiler();

	private:
		virtual void Initialize( const COptions& options ) override final;
		virtual void FillPayload( Red::Core::BundleDefinition::CBundleDataContainer*& payload, const StringAnsi& bundleName, Red::Core::BundleDefinition::CBundleDataContainer* bundleData ) override final;
		virtual void Shutdown( const COptions& options ) override final;
		virtual void OnWorkFinished( CProfilerWorker* worker ) override final;

		void AppendResults( const TDynArray< SProfilerItem >& results );

	private:
		FILE* m_resultsFile;

		const AnsiChar* m_compressionName[ Red::Core::Bundle::CT_Max ];
	};

	//////////////////////////////////////////////////////////////////////////
	// CProfilerFeedback
	//////////////////////////////////////////////////////////////////////////
	class CProfilerFeedback
	{
	public:
		CProfilerFeedback( Feedback& feedback, Uint32 numItemsInBundle );
		~CProfilerFeedback();

		void Increment();
		void Skip();
		void MarkCompleted( Feedback::ECompletionState success );

	private:
		void Update();

		Feedback& m_feedback;
		Uint32 m_completedIterations;

		const Uint32 m_numItems;
		const Uint32 m_testIterations;
		const Uint32 m_scoreIterations;

		const Uint32 m_totalIterations;
	};

	//////////////////////////////////////////////////////////////////////////
	// CProfilerCommon
	//////////////////////////////////////////////////////////////////////////
	class CProfilerCommon
	{
	public:
		CProfilerCommon( CProfilerFeedback* feedback = nullptr );

		Bool Profile( const AnsiChar* path, SProfilerItem& item, Uint32 numberOfIterations = 1 );
		void Profile( FILE* file, SProfilerItem& item, Uint32 numberOfIterations = 1 );
		void Profile( const void* uncompressedDataBuffer, Uint32 uncompressedDataBufferSize, SProfilerItem& item, Uint32 numberOfIterations = 1 );

	private:

		typedef Red::Core::Compressor::Base CompressorBase;
		typedef Red::Core::Decompressor::Base DecompressorBase;

		void CompressionTests( const void* uncompressedDataBuffer, Uint32 uncompressedBufferSize, SProfilerItem& item, Uint32 numberOfIterations ) const;
		void CompressionTest( CompressorBase* compressor, DecompressorBase* decompressor, const void* data, Uint32 size, SProfilerResult& result ) const;

		void CalculateStats( SProfilerItem::CompressionTypeResults& results, SProfilerResultStats& stats, Uint32 numberOfIterations ) const;
		void CalculateAverages( SProfilerResult& average, const SProfilerItem::CompressionTypeResults& results, const SProfilerResultStats& stats, Uint32 numberOfIterations ) const;

		Double CalculateScore( const SProfilerResult& result, Uint32 uncompressedSize ) const;

	private:
		CProfilerFeedback* m_feedback;
	};
}

#endif // _RED_COMPRESSION_PROFILER_H_
