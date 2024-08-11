/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _RED_BUNDLE_H_
#define _RED_BUNDLE_H_

#include "../../common/core/core.h"

#include "../../common/redSystem/crc.h"

#include "../../common/core/bundlePreamble.h"
#include "../../common/core/bundledefinition.h"

#include "feedback.h"
#include "bufferedFileWriter.h"
#include "options.h"
#include "creationParams.h"

namespace Red
{
	namespace Core
	{
		namespace Bundle
		{
			struct SBundleHeaderItem;
		}

		namespace BundleDefinition
		{
			struct SBundleFileDesc;
			enum ECompressionType;
		}

		namespace Compressor
		{
			class Base;
		}
	}
}

namespace Bundler
{
	using namespace Red::Core::Bundle;

	struct SProfilerItem;

	class CBundleHeader
	{
	public:
		CBundleHeader();
		~CBundleHeader();

		void SetPreamble( const SBundleHeaderPreamble& preamble );
		void AddItem( const SBundleHeaderItem& bundleHeaderItem, const StringAnsi& dataPath );

		Uint32 GetBundleDataSize() const;

		RED_INLINE Uint32 GetHeaderCount() const { return m_bundleItems.Size(); }
		RED_INLINE const SBundleHeaderPreamble& GetPreamble() const { return m_preamble; }
		RED_INLINE const StringAnsi& GetDataPath( Uint32 index ) const { return m_dataPaths[ index ]; }
		RED_INLINE const SBundleHeaderItem* operator[]( Uint32 index ) const { return m_bundleItems[ index ]; }
		RED_INLINE SBundleHeaderItem* operator[]( Uint32 index ) { return m_bundleItems[ index ]; }

	private:
		SBundleHeaderPreamble m_preamble;
		TDynArray< SBundleHeaderItem* > m_bundleItems;
		TDynArray< StringAnsi > m_dataPaths;
	};

	class CBundle
	{
	public:
		explicit CBundle( const BundleCreationParameters& creationParameters );
		~CBundle();

		void Construct( Uint8* buffer, Uint32 size );

		void AddItem( const SBundleHeaderItem& bundleHeaderItem, const StringAnsi& dataPath );
		RED_INLINE Uint32 GetNumItems() const { return m_bundleHeader.GetHeaderCount(); }

		void operator=( const CBundle& );

	private:
		Bool CreateFile();
		Bool WriteBuffer( Uint8* buffer, Uint32 bufferSize );
		Bool WriteBufferItem( const AnsiChar* dataPath, SBundleHeaderItem* item, Uint8* buffer, Uint32 bufferSize ) const;

		// For uncompressed files too large to fit into a single allocation
		Bool WriteLargeBufferItem( const AnsiChar* dataPath, SBundleHeaderItem* item, CBufferedFileWriter& writer, Uint8* buffer, Uint32 bufferSize ) const;

		Bool MeetsSizeThreshold( Uint32 size ) const;
		Bool MeetsCompressionRatioThreshold( Uint32 uncompressedSize, const SProfilerItem& profilingData ) const;
		void* AllocateUncompressedBuffer( Red::Core::Bundle::ECompressionType compressionType, void* compressedBuffer, Uint32 size ) const;
		void SelectCompressionType( Red::Core::Bundle::ECompressionType& compressionType, const void* buffer, Uint32 size ) const;
		Red::Core::Compressor::Base* CreateCompressor( const SBundleHeaderItem* item ) const;
		void CalculateCRC( SBundleHeaderItem* item, const void* buffer, Uint32 size ) const;

	private:
		static const Uint32 BUNDLE_NAME_MAX_SIZE	= 256u;

		// Don't compress files below this size
		static const Uint32 MINIMUM_SIZE_THRESHOLD	= 4096u;

		Feedback*					m_feedback;
		AutoCache::Bundle*			m_autoCache;

		AnsiChar					m_bundleName[ BUNDLE_NAME_MAX_SIZE ];
		CBundleHeader				m_bundleHeader;

		const COptions&				m_options;
		CBufferedFileWriter&		m_writer;
		const Red::System::CRC32&	m_crcCalculator;

		class FeedbackHelper
		{
		public:
			FeedbackHelper( Uint32 total, Feedback* feedback );

			void Update( Uint32 increment );
			void AdjustTotal( Int32 adjustment );

		private:
			// This is the size of all data uncompressed, headers and preamble.
			// It does not include padding
			Float m_total;
			Uint32 m_progress;
			Feedback* m_feedback;
		};
	};
};

#endif // _RED_BUNDLE_H_
