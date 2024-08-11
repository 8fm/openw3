/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

// This is only to be used to uncook DDS textures extracted from *.srt files.
// It builds perfectly legal DDS headers, but it only takes into consideration
// the formats we are able to compress, so it must not be considered a full
// fledged DDS builder.

namespace UncookDDSUtils
{
	const Uint32 DDS_MAGIC = 0x20534444;

	struct DDS_PIXELFORMAT
	{
		Uint32	size;
		Uint32	flags;
		Uint32	fourCC;
		Uint32	RGBBitCount;
		Uint32	RBitMask;
		Uint32	GBitMask;
		Uint32	BBitMask;
		Uint32	ABitMask;
	};

	struct DDS_HEADER
	{
		Uint32			size;
		Uint32			flags;
		Uint32			height;
		Uint32			width;
		Uint32			pitchOrLinearSize;
		Uint32			depth;
		Uint32			mipMapCount;
		Uint32			reserved1[ 11 ];
		DDS_PIXELFORMAT	ddspf;
		Uint32			caps;
		Uint32			caps2; // Only used for cube maps (we don't support them in *.srt).
		Uint32			caps3;
		Uint32			caps4;
		Uint32			reserved2;
	};

	// Not used right now (only when fourCC is 'DX10', not supported).

	typedef Uint32 DXGI_FORMAT;
	struct DDS_HEADER_DXT10
	{
		DXGI_FORMAT	dxgiFormat;
		Uint32		resourceDimension;
		Uint32		miscFlag;
		Uint32		arraySize;
		Uint32		miscFlags2;
	};

	struct DDSFileHeader
	{
		struct DDSHeader
		{
			Uint32				m_magic;
			DDS_HEADER			m_mainHeader;
			DDS_HEADER_DXT10	m_dxt10Header;
		};

		DDSHeader				m_header;
		Uint32					m_size;
	};

	// The reserved field should be all zeros, but nVidia fills it with garbage.
	// Not needed, but makes it easier to compare against original files.

#ifdef RED_CONFIGURATION_NOPTS
	static const Uint8 nVidiaReserved[ 11 * 4 ] =
	{
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x4e, 0x56, 0x54, 0x54,
		0x08, 0x00, 0x02, 0x00
	};
#endif

	Uint32 MakeFourCC( uint8_t ch0, uint8_t ch1, uint8_t ch2, uint8_t ch3 )
	{
		return ( ( Uint32 )( uint8_t )( ch0 ) | ( ( Uint32 )( uint8_t )( ch1 ) << 8 ) |
			( ( Uint32 )( uint8_t )( ch2 ) << 16 ) | ( ( Uint32 )( uint8_t )( ch3 ) << 24 ) );
	}

	Uint32 GetFourCC( Uint16 encodedFormat )
	{
		// Apparently we only support these encoding formats. For more info check texture.cpp,
		// ITexture::EncodeTextureFormat(), and static const TexEncodePair TEX_ENCODE_PAIRS[];.

		switch( encodedFormat )
		{
			/*GpuApi::TEXFMT_R8G8B8A8:			 */ case 1021: return 0; // Detected via RGB bit count.
			/*GpuApi::TEXFMT_BC1:				 */ case 1031: return MakeFourCC( 'D', 'X', 'T', '1' );
			/*GpuApi::TEXFMT_BC3:				 */ case 1032: return MakeFourCC( 'D', 'X', 'T', '5' );
			/*GpuApi::TEXFMT_BC6H:				 */ case 1033: return 0; // This has no fourCC?!
			/*GpuApi::TEXFMT_BC7:				 */ case 1034: return 0; // This has no fourCC?!
			/*GpuApi::TEXFMT_Float_R16G16B16A16: */ case 1035: return 113;
			/*GpuApi::TEXFMT_Float_R32G32B32A32: */ case 1036: return 116;
			/*GpuApi::TEXFMT_BC2:				 */ case 1037: return MakeFourCC( 'D', 'X', 'T', '3' );
			/*GpuApi::TEXFMT_BC4:				 */ case 1038: return MakeFourCC( 'A', 'T', 'I', '1' );
			/*GpuApi::TEXFMT_BC5:				 */ case 1039: return MakeFourCC( 'A', 'T', 'I', '2' );
			default: return 0;
		}
		return 0;
	}

	Uint32 GetCompressedPitch( Uint32 width, Uint32 height, Uint16 encodedFormat )
	{
		// I love to see how not even Microsoft follows it's own documentation. Actually, these
		// operations are fake, just to generate the same numbers as the undocumented nVidia tools.

		switch( encodedFormat )
		{
			/*GpuApi::TEXFMT_R8G8B8A8:			 */ case 1021: return ( width * 32 + 7 ) / 8;
			/*GpuApi::TEXFMT_BC1:				 */ case 1031: return ( ( width * height + 3 ) / 4 ) * 2;
			/*GpuApi::TEXFMT_BC3:				 */ case 1032: return ( ( width * height + 3 ) / 4 ) * 4;
			/*GpuApi::TEXFMT_BC6H:				 */ case 1033: return ( ( width * height + 3 ) / 4 ) * 4;
			/*GpuApi::TEXFMT_BC7:				 */ case 1034: return ( ( width * height + 3 ) / 4 ) * 4;
			/*GpuApi::TEXFMT_Float_R16G16B16A16: */ case 1035: return ( width * 64 + 7 ) / 8;
			/*GpuApi::TEXFMT_Float_R32G32B32A32: */ case 1036: return ( width * 128 + 7 ) / 8;
			/*GpuApi::TEXFMT_BC2:				 */ case 1037: return ( ( width * height + 3 ) / 4 ) * 4;
			/*GpuApi::TEXFMT_BC4:				 */ case 1038: return ( ( width * height + 3 ) / 4 ) * 2;
			/*GpuApi::TEXFMT_BC5:				 */ case 1039: return ( ( width * height + 3 ) / 4 ) * 4;
			default: return 0;
		}
		return 0;
	}

	void CreateDDSHeader( DDSFileHeader& outHeader, Uint32 width, Uint32 height, Uint32 mipCount, Uint16 encodedFormat )
	{
		Red::MemoryZero( &outHeader, sizeof( outHeader ) );

		outHeader.m_header.m_magic = DDS_MAGIC;

		DDS_HEADER& ddsHeader = outHeader.m_header.m_mainHeader;

		ddsHeader.size = sizeof( DDS_HEADER );
		ddsHeader.flags = 0x00081007; // DDSD_LINEARSIZE | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS
		if( mipCount > 1 )
			ddsHeader.flags |= 0x20000; // DDSD_MIPMAPCOUNT
		ddsHeader.height = height;
		ddsHeader.width = width;
		ddsHeader.pitchOrLinearSize = GetCompressedPitch( width, height, encodedFormat );
		ddsHeader.mipMapCount = mipCount;
		ddsHeader.ddspf.size = sizeof( DDS_PIXELFORMAT );
		ddsHeader.ddspf.fourCC = GetFourCC( encodedFormat );
		ddsHeader.ddspf.flags = ddsHeader.ddspf.fourCC != 0 ? 0x4 : 0x0; // DDPF_FOURCC
		ddsHeader.caps = 0x1000; // DDSCAPS_TEXTURE
		if( mipCount > 1 )
			ddsHeader.caps |= 0x400008; // DDSCAPS_MIPMAP | DDSCAPS_COMPLEX

		// For this particular case we have to fill RGB bit count.
		if( encodedFormat == 1021 /*TEXFMT_R8G8B8A8*/ )
		{
			ddsHeader.ddspf.flags |= 0x00000040; // DDPF_RGB
			ddsHeader.ddspf.RGBBitCount = 32;
			ddsHeader.ddspf.RBitMask = 0x000000ff;
			ddsHeader.ddspf.RBitMask = 0x0000ff00;
			ddsHeader.ddspf.RBitMask = 0x00ff0000;
			ddsHeader.ddspf.RBitMask = 0xff000000;
		}

#ifdef RED_CONFIGURATION_NOPTS
		Red::MemoryCopy( ddsHeader.reserved1, nVidiaReserved, sizeof( nVidiaReserved ) );
#endif

		outHeader.m_size = sizeof( outHeader.m_header.m_magic ) + sizeof( outHeader.m_header.m_mainHeader );
	}
};