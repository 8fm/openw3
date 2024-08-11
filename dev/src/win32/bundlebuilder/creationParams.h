/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _RED_BUNDLE_CREATION_PARAMETERS_H_
#define _RED_BUNDLE_CREATION_PARAMETERS_H_

#include "../../common/core/core.h"

namespace Red
{
	namespace Core
	{
		namespace BundleDefinition
		{
			class CBundleDataContainer;
		}
	}

	namespace System
	{
		class CRC32;
	}
}

namespace Bundler
{
	class Feedback;
	class COptions;
	class CBufferedFileWriter;

	namespace AutoCache
	{
		class Bundle;
	}
}

namespace Bundler
{
	struct BundleCreationParameters
	{
		const Red::Core::BundleDefinition::CBundleDataContainer* m_bundleDataContainer;
		Feedback* m_feedback;
		AutoCache::Bundle* m_autoCache;

		Uint8* m_inputBuffer;
		Uint32 m_inputBufferSize;

		const COptions& m_options;
		CBufferedFileWriter& m_writer;
		const Red::System::CRC32& m_crcCalculator;

		BundleCreationParameters( const COptions& options, CBufferedFileWriter& writer, const Red::System::CRC32& crc )
		:	m_bundleDataContainer( nullptr )
		,	m_feedback( nullptr )
		,	m_autoCache( nullptr )
		,	m_inputBuffer( nullptr )
		,	m_inputBufferSize( 0 )
		,	m_options( options )
		,	m_writer( writer )
		,	m_crcCalculator( crc )
		{

		}

		void operator=( const BundleCreationParameters& );
	};
}

#endif // _RED_BUNDLE_CREATION_PARAMETERS_H_
