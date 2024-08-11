/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef _RED_BUNDLE_PREAMBLE_H_
#define _RED_BUNDLE_PREAMBLE_H_

#define BUNDLE_PREAMBLE_SIZE 32

namespace Red
{
	namespace Core
	{
		namespace Bundle
		{
			const Uint64 BUNDLE_STAMP			= 0x30374F5441544F50;
			const Uint16 BUNDLE_FILE_VERSION	= 0x01;
			const Uint16 BUNDLE_HEADER_VERSION	= 0x03;
			
			enum class EBundleHeaderConfiguration : Uint8
			{
				BHC_Debug = 0x00,
				BHC_Release = 0x01,
				BHC_Final = 0x02,
			};

			// This struct contains metadata about the bundle file itself, such as header configuration, header version, data version
			struct SBundleHeaderPreamble 
			{
				SBundleHeaderPreamble()
					: m_bundleStamp( 0 )
					, m_fileSize( 0 )
					, m_burstSize( 0 )
					, m_headerSize( 0 )
					, m_headerVersion( BUNDLE_HEADER_VERSION )
					, m_fileVersion( BUNDLE_FILE_VERSION )
					, m_configuration( EBundleHeaderConfiguration::BHC_Debug )
				{
					Red::System::MemorySet( &m_padding, 0x13, ARRAY_COUNT( m_padding ) );
				}

				Uint64						m_bundleStamp;		// 8

				// Total size, in bytes, for this bundle file
				Uint32						m_fileSize;			// 12

				// Total amount we can burst read.
				Uint32						m_burstSize;		// 16

				// Size of the header, in bytes (excluding this preamble)
				Uint32						m_headerSize;		// 20

				// Header version
				Uint16						m_headerVersion;	// 22

				// Data version
				Uint16						m_fileVersion;		// 24				

				// Specifies which type/layout of header is used in this bundle
				EBundleHeaderConfiguration	m_configuration;	// 25

				// Unused
				Uint8						m_padding[ BUNDLE_PREAMBLE_SIZE - 25 ];
			};

			static_assert( sizeof( SBundleHeaderPreamble ) == BUNDLE_PREAMBLE_SIZE, "Bundle Header Preamble is an invalid size" );

			// When adjusting the size of the preamble struct, make sure to abide by this rule
			static_assert( sizeof( SBundleHeaderPreamble ) % 16 == 0, "Bundle Header Preamble size must always be a multiple of 16" );
		}
	}
}

#endif // _RED_BUNDLE_PREAMBLE_H_
