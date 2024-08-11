/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __RED_BUNDLE_BUILDER_AUTOCACHE_BUILDER_H__
#define __RED_BUNDLE_BUILDER_AUTOCACHE_BUILDER_H__

#include "../../common/core/core.h"

#include "compressionProfiler.h"
#include "producer.h"
#include "consumer.h"
#include "bundlewriter.h"

namespace Bundler
{
	namespace AutoCache
	{
		class Bundle;
		class Definition;
	}

	//////////////////////////////////////////////////////////////////////////
	// CProfilerWorker
	//////////////////////////////////////////////////////////////////////////
	class CAutoCacheBuilderWorker : public CConsumer< Payload >
	{
	public:
		CAutoCacheBuilderWorker( Red::Threads::CSemaphore* lock, const COptions& options );
		virtual ~CAutoCacheBuilderWorker();

	private:
		virtual void Do() override final;

	private:
		Uint32 m_numTestIterations;

		// Don't compress files below this size
		static const Uint32 MINIMUM_SIZE_THRESHOLD	= 4096u;
	};

	//////////////////////////////////////////////////////////////////////////
	// CCompressionProfiler
	//////////////////////////////////////////////////////////////////////////
	class CAutoCacheBuilder: public CProducer< CAutoCacheBuilderWorker, Payload >
	{
	public:
		CAutoCacheBuilder( Red::Core::BundleDefinition::IBundleDefinition& bundleDef );
		virtual ~CAutoCacheBuilder();

	private:
		virtual void Initialize( const COptions& options ) override final;
		virtual void FillPayload( Payload& payload, const StringAnsi& bundleName, Red::Core::BundleDefinition::CBundleDataContainer* bundleData ) override final;
		virtual void Shutdown( const COptions& options ) override final;

	private:
		AutoCache::Definition* m_definitionAutoCache;
	};
}

#endif // __RED_BUNDLE_BUILDER_AUTOCACHE_BUILDER_H__
