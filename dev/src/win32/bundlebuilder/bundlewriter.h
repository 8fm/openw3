/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _RED_BUNDLE_WRITER_H_
#define _RED_BUNDLE_WRITER_H_

#include "../../common/core/core.h"

#include "../../common/redThreads/redThreadsThread.h"
#include "../../common/redSystem/crc.h"

#include "producer.h"
#include "consumer.h"
#include "feedback.h"
#include "bufferedFileWriter.h"
#include "options.h"
#include "creationParams.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
namespace Red
{
	namespace Core
	{
		namespace BundleDefinition
		{
			struct SBundleFileDesc;
			class CBundleDataContainer;
			class IBundleDefinition;
			class CBundleDefinitionReader;
		}

		namespace Bundle
		{
			struct SBundleHeaderItem;
		}
	}
}

namespace Bundler
{
	//////////////////////////////////////////////////////////////////////////
	// Forward declarations
	//////////////////////////////////////////////////////////////////////////
	class CBundle;

	namespace AutoCache
	{
		class Bundle;
		class Definition;
	}

	//////////////////////////////////////////////////////////////////////////
	// Payload
	//////////////////////////////////////////////////////////////////////////
	struct Payload
	{
		Red::Core::BundleDefinition::CBundleDataContainer* m_bundle;
		AutoCache::Bundle* m_autoCache;
	};

	//////////////////////////////////////////////////////////////////////////
	// CBundleWriterWorker
	//////////////////////////////////////////////////////////////////////////
	class CBundleWriterWorker : public CConsumer< Payload >
	{
	public:
		CBundleWriterWorker( Red::Threads::CSemaphore* lock, const COptions& options );
		virtual ~CBundleWriterWorker();

	private:
		virtual void Do() override final;

	private:
		const COptions& m_options;
		CBufferedFileWriter m_writer;
		Uint8* m_inputBuffer;
		Uint32 m_inputBufferSize;
		Red::System::CRC32 m_crcCalculator;
	};

	//////////////////////////////////////////////////////////////////////////
	// CBundleWriter
	//////////////////////////////////////////////////////////////////////////
	class CBundleWriter : public CProducer< CBundleWriterWorker, Payload >
	{
	public:
		CBundleWriter( Red::Core::BundleDefinition::IBundleDefinition& bundleDef );
		virtual ~CBundleWriter();

	private:
		virtual void Initialize( const COptions& options ) override final;
		virtual void FillPayload( Payload& payload, const StringAnsi& bundleName, Red::Core::BundleDefinition::CBundleDataContainer* bundleData ) override final;
		virtual void Shutdown( const COptions& options ) override final;

	private:
		void InitializeAutoCache( const COptions& options );
		void InitializeBundleDirectories( const COptions& options );

	private:
		AutoCache::Definition* m_definitionAutoCache;
	};

	//////////////////////////////////////////////////////////////////////////
	// CBundleWriterCommon
	//////////////////////////////////////////////////////////////////////////
	class CBundleWriterCommon
	{
	public:
		void CreateBundle( const BundleCreationParameters& params ) const;

	private:
		Bool FillInDetails( const StringAnsi& path, Red::Core::Bundle::SBundleHeaderItem& header ) const;
		void FillInModificationTime( HANDLE& fileHandle, Red::Core::Bundle::SBundleHeaderItem& header ) const;
		void FillInDataSize( HANDLE& fileHandle, Red::Core::Bundle::SBundleHeaderItem& header ) const;
	};

};
#endif // _RED_BUNDLE_WRITER_H_
