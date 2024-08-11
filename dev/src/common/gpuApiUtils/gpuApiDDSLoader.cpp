#include "gpuApiUtils.h"
#include "gpuApiDDSLoader.h"

namespace GpuApi
{

	//--------------------------------------------------------------------------------------
	// Macros
	//--------------------------------------------------------------------------------------
#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
	((Uint32)(uint8_t)(ch0) | ((Uint32)(uint8_t)(ch1) << 8) |       \
	((Uint32)(uint8_t)(ch2) << 16) | ((Uint32)(uint8_t)(ch3) << 24 ))
#endif /* defined(MAKEFOURCC) */

	//--------------------------------------------------------------------------------------
	// DDS file structure definitions
	//
	// See DDS.h in the 'Texconv' sample and the 'DirectXTex' library
	//--------------------------------------------------------------------------------------
#pragma pack(push,1)

	const Uint32 DDS_MAGIC = 0x20534444; // "DDS "

	struct DDS_PIXELFORMAT
	{
		Uint32    size;
		Uint32    flags;
		Uint32    fourCC;
		Uint32    RGBBitCount;
		Uint32    RBitMask;
		Uint32    GBitMask;
		Uint32    BBitMask;
		Uint32    ABitMask;
	};

#define DDS_FOURCC      0x00000004  // DDPF_FOURCC
#define DDS_RGB         0x00000040  // DDPF_RGB
#define DDS_LUMINANCE   0x00020000  // DDPF_LUMINANCE
#define DDS_ALPHA       0x00000002  // DDPF_ALPHA

#define DDS_HEADER_FLAGS_VOLUME         0x00800000  // DDSD_DEPTH

#define DDS_HEIGHT 0x00000002 // DDSD_HEIGHT
#define DDS_WIDTH  0x00000004 // DDSD_WIDTH

#define DDS_CUBEMAP_POSITIVEX 0x00000600 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
#define DDS_CUBEMAP_NEGATIVEX 0x00000a00 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
#define DDS_CUBEMAP_POSITIVEY 0x00001200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
#define DDS_CUBEMAP_NEGATIVEY 0x00002200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
#define DDS_CUBEMAP_POSITIVEZ 0x00004200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
#define DDS_CUBEMAP_NEGATIVEZ 0x00008200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

#define DDS_CUBEMAP_ALLFACES ( DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |\
	DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY |\
	DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ )

#define DDS_CUBEMAP 0x00000200 // DDSCAPS2_CUBEMAP

	// Mimicing dxgi enum for reading the dds
	typedef enum DXGI_FORMAT
	{
		DXGI_FORMAT_UNKNOWN	                    = 0,
		DXGI_FORMAT_R32G32B32A32_TYPELESS       = 1,
		DXGI_FORMAT_R32G32B32A32_FLOAT          = 2,
		DXGI_FORMAT_R32G32B32A32_UINT           = 3,
		DXGI_FORMAT_R32G32B32A32_SINT           = 4,
		DXGI_FORMAT_R32G32B32_TYPELESS          = 5,
		DXGI_FORMAT_R32G32B32_FLOAT             = 6,
		DXGI_FORMAT_R32G32B32_UINT              = 7,
		DXGI_FORMAT_R32G32B32_SINT              = 8,
		DXGI_FORMAT_R16G16B16A16_TYPELESS       = 9,
		DXGI_FORMAT_R16G16B16A16_FLOAT          = 10,
		DXGI_FORMAT_R16G16B16A16_UNORM          = 11,
		DXGI_FORMAT_R16G16B16A16_UINT           = 12,
		DXGI_FORMAT_R16G16B16A16_SNORM          = 13,
		DXGI_FORMAT_R16G16B16A16_SINT           = 14,
		DXGI_FORMAT_R32G32_TYPELESS             = 15,
		DXGI_FORMAT_R32G32_FLOAT                = 16,
		DXGI_FORMAT_R32G32_UINT                 = 17,
		DXGI_FORMAT_R32G32_SINT                 = 18,
		DXGI_FORMAT_R32G8X24_TYPELESS           = 19,
		DXGI_FORMAT_D32_FLOAT_S8X24_UINT        = 20,
		DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS    = 21,
		DXGI_FORMAT_X32_TYPELESS_G8X24_UINT     = 22,
		DXGI_FORMAT_R10G10B10A2_TYPELESS        = 23,
		DXGI_FORMAT_R10G10B10A2_UNORM           = 24,
		DXGI_FORMAT_R10G10B10A2_UINT            = 25,
		DXGI_FORMAT_R11G11B10_FLOAT             = 26,
		DXGI_FORMAT_R8G8B8A8_TYPELESS           = 27,
		DXGI_FORMAT_R8G8B8A8_UNORM              = 28,
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB         = 29,
		DXGI_FORMAT_R8G8B8A8_UINT               = 30,
		DXGI_FORMAT_R8G8B8A8_SNORM              = 31,
		DXGI_FORMAT_R8G8B8A8_SINT               = 32,
		DXGI_FORMAT_R16G16_TYPELESS             = 33,
		DXGI_FORMAT_R16G16_FLOAT                = 34,
		DXGI_FORMAT_R16G16_UNORM                = 35,
		DXGI_FORMAT_R16G16_UINT                 = 36,
		DXGI_FORMAT_R16G16_SNORM                = 37,
		DXGI_FORMAT_R16G16_SINT                 = 38,
		DXGI_FORMAT_R32_TYPELESS                = 39,
		DXGI_FORMAT_D32_FLOAT                   = 40,
		DXGI_FORMAT_R32_FLOAT                   = 41,
		DXGI_FORMAT_R32_UINT                    = 42,
		DXGI_FORMAT_R32_SINT                    = 43,
		DXGI_FORMAT_R24G8_TYPELESS              = 44,
		DXGI_FORMAT_D24_UNORM_S8_UINT           = 45,
		DXGI_FORMAT_R24_UNORM_X8_TYPELESS       = 46,
		DXGI_FORMAT_X24_TYPELESS_G8_UINT        = 47,
		DXGI_FORMAT_R8G8_TYPELESS               = 48,
		DXGI_FORMAT_R8G8_UNORM                  = 49,
		DXGI_FORMAT_R8G8_UINT                   = 50,
		DXGI_FORMAT_R8G8_SNORM                  = 51,
		DXGI_FORMAT_R8G8_SINT                   = 52,
		DXGI_FORMAT_R16_TYPELESS                = 53,
		DXGI_FORMAT_R16_FLOAT                   = 54,
		DXGI_FORMAT_D16_UNORM                   = 55,
		DXGI_FORMAT_R16_UNORM                   = 56,
		DXGI_FORMAT_R16_UINT                    = 57,
		DXGI_FORMAT_R16_SNORM                   = 58,
		DXGI_FORMAT_R16_SINT                    = 59,
		DXGI_FORMAT_R8_TYPELESS                 = 60,
		DXGI_FORMAT_R8_UNORM                    = 61,
		DXGI_FORMAT_R8_UINT                     = 62,
		DXGI_FORMAT_R8_SNORM                    = 63,
		DXGI_FORMAT_R8_SINT                     = 64,
		DXGI_FORMAT_A8_UNORM                    = 65,
		DXGI_FORMAT_R1_UNORM                    = 66,
		DXGI_FORMAT_R9G9B9E5_SHAREDEXP          = 67,
		DXGI_FORMAT_R8G8_B8G8_UNORM             = 68,
		DXGI_FORMAT_G8R8_G8B8_UNORM             = 69,
		DXGI_FORMAT_BC1_TYPELESS                = 70,
		DXGI_FORMAT_BC1_UNORM                   = 71,
		DXGI_FORMAT_BC1_UNORM_SRGB              = 72,
		DXGI_FORMAT_BC2_TYPELESS                = 73,
		DXGI_FORMAT_BC2_UNORM                   = 74,
		DXGI_FORMAT_BC2_UNORM_SRGB              = 75,
		DXGI_FORMAT_BC3_TYPELESS                = 76,
		DXGI_FORMAT_BC3_UNORM                   = 77,
		DXGI_FORMAT_BC3_UNORM_SRGB              = 78,
		DXGI_FORMAT_BC4_TYPELESS                = 79,
		DXGI_FORMAT_BC4_UNORM                   = 80,
		DXGI_FORMAT_BC4_SNORM                   = 81,
		DXGI_FORMAT_BC5_TYPELESS                = 82,
		DXGI_FORMAT_BC5_UNORM                   = 83,
		DXGI_FORMAT_BC5_SNORM                   = 84,
		DXGI_FORMAT_B5G6R5_UNORM                = 85,
		DXGI_FORMAT_B5G5R5A1_UNORM              = 86,
		DXGI_FORMAT_B8G8R8A8_UNORM              = 87,
		DXGI_FORMAT_B8G8R8X8_UNORM              = 88,
		DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM  = 89,
		DXGI_FORMAT_B8G8R8A8_TYPELESS           = 90,
		DXGI_FORMAT_B8G8R8A8_UNORM_SRGB         = 91,
		DXGI_FORMAT_B8G8R8X8_TYPELESS           = 92,
		DXGI_FORMAT_B8G8R8X8_UNORM_SRGB         = 93,
		DXGI_FORMAT_BC6H_TYPELESS               = 94,
		DXGI_FORMAT_BC6H_UF16                   = 95,
		DXGI_FORMAT_BC6H_SF16                   = 96,
		DXGI_FORMAT_BC7_TYPELESS                = 97,
		DXGI_FORMAT_BC7_UNORM                   = 98,
		DXGI_FORMAT_BC7_UNORM_SRGB              = 99,
		DXGI_FORMAT_AYUV                        = 100,
		DXGI_FORMAT_Y410                        = 101,
		DXGI_FORMAT_Y416                        = 102,
		DXGI_FORMAT_NV12                        = 103,
		DXGI_FORMAT_P010                        = 104,
		DXGI_FORMAT_P016                        = 105,
		DXGI_FORMAT_420_OPAQUE                  = 106,
		DXGI_FORMAT_YUY2                        = 107,
		DXGI_FORMAT_Y210                        = 108,
		DXGI_FORMAT_Y216                        = 109,
		DXGI_FORMAT_NV11                        = 110,
		DXGI_FORMAT_AI44                        = 111,
		DXGI_FORMAT_IA44                        = 112,
		DXGI_FORMAT_P8                          = 113,
		DXGI_FORMAT_A8P8                        = 114,
		DXGI_FORMAT_B4G4R4A4_UNORM              = 115,
		DXGI_FORMAT_R10G10B10_7E3_A2_FLOAT      = 116,
		DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT      = 117,
		DXGI_FORMAT_D16_UNORM_S8_UINT           = 118,
		DXGI_FORMAT_R16_UNORM_X8_TYPELESS       = 119,
		DXGI_FORMAT_X16_TYPELESS_G8_UINT        = 120,
		DXGI_FORMAT_FORCE_UINT                  = 0xffffffff
	} DXGI_FORMAT;

	enum DDS_MISC_FLAGS2
	{
		DDS_MISC_FLAGS2_ALPHA_MODE_MASK = 0x7L,
	};

	struct DDS_HEADER
	{
		Uint32        size;
		Uint32        flags;
		Uint32        height;
		Uint32        width;
		Uint32        pitchOrLinearSize;
		Uint32        depth; // only if DDS_HEADER_FLAGS_VOLUME is set in flags
		Uint32        mipMapCount;
		Uint32        reserved1[11];
		DDS_PIXELFORMAT ddspf;
		Uint32        caps;
		Uint32        caps2;
		Uint32        caps3;
		Uint32        caps4;
		Uint32        reserved2;
	};

	struct DDS_HEADER_DXT10
	{
		DXGI_FORMAT     dxgiFormat;
		Uint32        resourceDimension;
		Uint32        miscFlag; // see D3D11_RESOURCE_MISC_FLAG
		Uint32        arraySize;
		Uint32        miscFlags2;
	};

	enum D3D11_RESOURCE_DIMENSION
	{
		D3D11_RESOURCE_DIMENSION_UNKNOWN	= 0,
		D3D11_RESOURCE_DIMENSION_BUFFER		= 1,
		D3D11_RESOURCE_DIMENSION_TEXTURE1D	= 2,
		D3D11_RESOURCE_DIMENSION_TEXTURE2D	= 3,
		D3D11_RESOURCE_DIMENSION_TEXTURE3D	= 4
	} 	D3D11_RESOURCE_DIMENSION;

	eTextureFormat MapDXGIToTexFormat( DXGI_FORMAT format )
	{
		switch ( format )
		{
		case DXGI_FORMAT_A8_UNORM:				return TEXFMT_A8;
		case DXGI_FORMAT_R8_UNORM:				return TEXFMT_L8;
		case DXGI_FORMAT_R8_UINT:				return TEXFMT_R8_Uint;
		case DXGI_FORMAT_R8G8B8A8_UNORM:		return TEXFMT_R8G8B8A8;
		case DXGI_FORMAT_R8G8_TYPELESS:			return TEXFMT_A8L8;
		case DXGI_FORMAT_R8G8_UNORM:			return TEXFMT_A8L8;
		case DXGI_FORMAT_R16_UNORM:				return TEXFMT_Uint_16_norm;
		case DXGI_FORMAT_R16_UINT:				return TEXFMT_Uint_16;
		case DXGI_FORMAT_R32_UINT:				return TEXFMT_Uint_32;
		case DXGI_FORMAT_R16G16_UINT:			return TEXFMT_R16G16_Uint;
		case DXGI_FORMAT_R10G10B10A2_UNORM:		return TEXFMT_Float_R10G10B10A2;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:	return TEXFMT_Float_R16G16B16A16;
		case DXGI_FORMAT_R11G11B10_FLOAT:		return TEXFMT_Float_R11G11B10;
		case DXGI_FORMAT_R16G16_FLOAT:			return TEXFMT_Float_R16G16;
		case DXGI_FORMAT_R32G32_FLOAT:			return TEXFMT_Float_R32G32; //dex
		case DXGI_FORMAT_R32G32B32A32_FLOAT:	return TEXFMT_Float_R32G32B32A32;
		case DXGI_FORMAT_R32_FLOAT:				return TEXFMT_Float_R32;
		case DXGI_FORMAT_R16_FLOAT:				return TEXFMT_Float_R16;
		case DXGI_FORMAT_R24G8_TYPELESS:		return TEXFMT_D24S8;
		case DXGI_FORMAT_BC1_UNORM:				return TEXFMT_BC1;
		case DXGI_FORMAT_BC2_UNORM:				return TEXFMT_BC2;
		case DXGI_FORMAT_BC3_UNORM:				return TEXFMT_BC3;
		case DXGI_FORMAT_BC4_UNORM:				return TEXFMT_BC4;
		case DXGI_FORMAT_BC6H_UF16:				return TEXFMT_BC6H;
		case DXGI_FORMAT_BC7_UNORM:				return TEXFMT_BC7;
		default:							GPUAPI_HALT( "invalid" ); return TEXFMT_Max;
		}
	}

#pragma pack(pop)

	//--------------------------------------------------------------------------------------
	// Return the BPP for a particular format
	//--------------------------------------------------------------------------------------
	static size_t BitsPerPixel( DXGI_FORMAT fmt )
	{
		switch( fmt )
		{
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return 128;

		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			return 96;

		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
			return 64;

		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
		case DXGI_FORMAT_R8G8_B8G8_UNORM:
		case DXGI_FORMAT_G8R8_G8B8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
			return 32;

		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_B5G6R5_UNORM:
		case DXGI_FORMAT_B5G5R5A1_UNORM:
		case DXGI_FORMAT_B4G4R4A4_UNORM:
			return 16;

		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_A8_UNORM:
			return 8;

		case DXGI_FORMAT_R1_UNORM:
			return 1;

		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
			return 4;

		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return 8;

		default:
			return 0;
		}
	}

	//--------------------------------------------------------------------------------------
	// Get surface information for a particular format
	//--------------------------------------------------------------------------------------
	static void GetSurfaceInfo( size_t width,
		size_t height,
		DXGI_FORMAT fmt,
		size_t* outNumBytes,
		size_t* outRowBytes,
		size_t* outNumRows )
	{
		size_t numBytes = 0;
		size_t rowBytes = 0;
		size_t numRows = 0;

		Bool bc = false;
		Bool packed  = false;
		size_t bcnumBytesPerBlock = 0;
		switch (fmt)
		{
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
			bc=true;
			bcnumBytesPerBlock = 8;
			break;

		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			bc = true;
			bcnumBytesPerBlock = 16;
			break;

		case DXGI_FORMAT_R8G8_B8G8_UNORM:
		case DXGI_FORMAT_G8R8_G8B8_UNORM:
			packed = true;
			break;

		default:
			GPUAPI_HALT("Unsupported format");
		}

		if (bc)
		{
			size_t numBlocksWide = 0;
			if (width > 0)
			{
				numBlocksWide = Red::Math::NumericalUtils::Max( (size_t)1, (width + 3) / 4 );
			}
			size_t numBlocksHigh = 0;
			if (height > 0)
			{
				numBlocksHigh = Red::Math::NumericalUtils::Max( (size_t)1u, (height + 3) / 4 );
			}
			rowBytes = numBlocksWide * bcnumBytesPerBlock;
			numRows = numBlocksHigh;
		}
		else if (packed)
		{
			rowBytes = ( ( width + 1 ) >> 1 ) * 4;
			numRows = height;
		}
		else
		{
			size_t bpp = BitsPerPixel( fmt );
			rowBytes = ( width * bpp + 7 ) / 8; // round up to nearest byte
			numRows = height;
		}

		numBytes = rowBytes * numRows;
		if (outNumBytes)
		{
			*outNumBytes = numBytes;
		}
		if (outRowBytes)
		{
			*outRowBytes = rowBytes;
		}
		if (outNumRows)
		{
			*outNumRows = numRows;
		}
	}


	//--------------------------------------------------------------------------------------
#define ISBITMASK( r,g,b,a ) ( ddpf.RBitMask == r && ddpf.GBitMask == g && ddpf.BBitMask == b && ddpf.ABitMask == a )

	static DXGI_FORMAT GetDXGIFormat( const DDS_PIXELFORMAT& ddpf )
	{
		if (ddpf.flags & DDS_RGB)
		{
			// Note that sRGB formats are written using the "DX10" extended header

			switch (ddpf.RGBBitCount)
			{
			case 32:
				if (ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0xff000000))
				{
					return DXGI_FORMAT_R8G8B8A8_UNORM;
				}

				if (ISBITMASK(0x00ff0000,0x0000ff00,0x000000ff,0xff000000))
				{
					return DXGI_FORMAT_B8G8R8A8_UNORM;
				}

				if (ISBITMASK(0x00ff0000,0x0000ff00,0x000000ff,0x00000000))
				{
					return DXGI_FORMAT_B8G8R8X8_UNORM;
				}

				// No DXGI format maps to ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0x00000000) aka D3DFMT_X8B8G8R8

				// Note that many common DDS reader/writers (including D3DX) swap the
				// the RED/BLUE masks for 10:10:10:2 formats. We assumme
				// below that the 'backwards' header mask is being used since it is most
				// likely written by D3DX. The more robust solution is to use the 'DX10'
				// header extension and specify the DXGI_FORMAT_R10G10B10A2_UNORM format directly

				// For 'correct' writers, this should be 0x000003ff,0x000ffc00,0x3ff00000 for RGB data
				if (ISBITMASK(0x3ff00000,0x000ffc00,0x000003ff,0xc0000000))
				{
					return DXGI_FORMAT_R10G10B10A2_UNORM;
				}

				// No DXGI format maps to ISBITMASK(0x000003ff,0x000ffc00,0x3ff00000,0xc0000000) aka D3DFMT_A2R10G10B10

				if (ISBITMASK(0x0000ffff,0xffff0000,0x00000000,0x00000000))
				{
					return DXGI_FORMAT_R16G16_UNORM;
				}

				if (ISBITMASK(0xffffffff,0x00000000,0x00000000,0x00000000))
				{
					// Only 32-bit color channel format in D3D9 was R32F
					return DXGI_FORMAT_R32_FLOAT; // D3DX writes this out as a FourCC of 114
				}
				break;

			case 24:
				// No 24bpp DXGI formats aka D3DFMT_R8G8B8
				break;

			case 16:
				if (ISBITMASK(0x7c00,0x03e0,0x001f,0x8000))
				{
					return DXGI_FORMAT_B5G5R5A1_UNORM;
				}
				if (ISBITMASK(0xf800,0x07e0,0x001f,0x0000))
				{
					return DXGI_FORMAT_B5G6R5_UNORM;
				}

				// No DXGI format maps to ISBITMASK(0x7c00,0x03e0,0x001f,0x0000) aka D3DFMT_X1R5G5B5

				if (ISBITMASK(0x0f00,0x00f0,0x000f,0xf000))
				{
					return DXGI_FORMAT_B4G4R4A4_UNORM;
				}

				// No DXGI format maps to ISBITMASK(0x0f00,0x00f0,0x000f,0x0000) aka D3DFMT_X4R4G4B4

				// No 3:3:2, 3:3:2:8, or paletted DXGI formats aka D3DFMT_A8R3G3B2, D3DFMT_R3G3B2, D3DFMT_P8, D3DFMT_A8P8, etc.
				break;
			}
		}
		else if (ddpf.flags & DDS_LUMINANCE)
		{
			if (8 == ddpf.RGBBitCount)
			{
				if (ISBITMASK(0x000000ff,0x00000000,0x00000000,0x00000000))
				{
					return DXGI_FORMAT_R8_UNORM; // D3DX10/11 writes this out as DX10 extension
				}

				// No DXGI format maps to ISBITMASK(0x0f,0x00,0x00,0xf0) aka D3DFMT_A4L4
			}

			if (16 == ddpf.RGBBitCount)
			{
				if (ISBITMASK(0x0000ffff,0x00000000,0x00000000,0x00000000))
				{
					return DXGI_FORMAT_R16_UNORM; // D3DX10/11 writes this out as DX10 extension
				}
				if (ISBITMASK(0x000000ff,0x00000000,0x00000000,0x0000ff00))
				{
					return DXGI_FORMAT_R8G8_UNORM; // D3DX10/11 writes this out as DX10 extension
				}
			}
		}
		else if (ddpf.flags & DDS_ALPHA)
		{
			if (8 == ddpf.RGBBitCount)
			{
				return DXGI_FORMAT_A8_UNORM;
			}
		}
		else if (ddpf.flags & DDS_FOURCC)
		{
			if (MAKEFOURCC( 'D', 'X', 'T', '1' ) == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC1_UNORM;
			}
			if (MAKEFOURCC( 'D', 'X', 'T', '3' ) == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC2_UNORM;
			}
			if (MAKEFOURCC( 'D', 'X', 'T', '5' ) == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC3_UNORM;
			}

			// While pre-mulitplied alpha isn't directly supported by the DXGI formats,
			// they are basically the same as these BC formats so they can be mapped
			if (MAKEFOURCC( 'D', 'X', 'T', '2' ) == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC2_UNORM;
			}
			if (MAKEFOURCC( 'D', 'X', 'T', '4' ) == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC3_UNORM;
			}

			if (MAKEFOURCC( 'A', 'T', 'I', '1' ) == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC4_UNORM;
			}
			if (MAKEFOURCC( 'B', 'C', '4', 'U' ) == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC4_UNORM;
			}
			if (MAKEFOURCC( 'B', 'C', '4', 'S' ) == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC4_SNORM;
			}

			if (MAKEFOURCC( 'A', 'T', 'I', '2' ) == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC5_UNORM;
			}
			if (MAKEFOURCC( 'B', 'C', '5', 'U' ) == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC5_UNORM;
			}
			if (MAKEFOURCC( 'B', 'C', '5', 'S' ) == ddpf.fourCC)
			{
				return DXGI_FORMAT_BC5_SNORM;
			}

			// BC6H and BC7 are written using the "DX10" extended header

			if (MAKEFOURCC( 'R', 'G', 'B', 'G' ) == ddpf.fourCC)
			{
				return DXGI_FORMAT_R8G8_B8G8_UNORM;
			}
			if (MAKEFOURCC( 'G', 'R', 'G', 'B' ) == ddpf.fourCC)
			{
				return DXGI_FORMAT_G8R8_G8B8_UNORM;
			}

			// Check for D3DFORMAT enums being set here
			switch( ddpf.fourCC )
			{
			case 36: // D3DFMT_A16B16G16R16
				return DXGI_FORMAT_R16G16B16A16_UNORM;

			case 110: // D3DFMT_Q16W16V16U16
				return DXGI_FORMAT_R16G16B16A16_SNORM;

			case 111: // D3DFMT_R16F
				return DXGI_FORMAT_R16_FLOAT;

			case 112: // D3DFMT_G16R16F
				return DXGI_FORMAT_R16G16_FLOAT;

			case 113: // D3DFMT_A16B16G16R16F
				return DXGI_FORMAT_R16G16B16A16_FLOAT;

			case 114: // D3DFMT_R32F
				return DXGI_FORMAT_R32_FLOAT;

			case 115: // D3DFMT_G32R32F
				return DXGI_FORMAT_R32G32_FLOAT;

			case 116: // D3DFMT_A32B32G32R32F
				return DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
		}

		return DXGI_FORMAT_UNKNOWN;
	}

	/*static eTextureFormat GetTextureFormat( const DDS_PIXELFORMAT& ddpf )
	{
		if (ddpf.flags & DDS_RGB)
		{
			// Note that sRGB formats are written using the "DX10" extended header

			switch (ddpf.RGBBitCount)
			{
			case 32:
				if (ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0xff000000))
				{
					return TEXFMT_R8G8B8A8;
				}

				if (ISBITMASK(0x00ff0000,0x0000ff00,0x000000ff,0xff000000))
				{
					GPUAPI_HALT("Unsupported DDS format!!!");
					return  TEXFMT_NULL;
				}

				if (ISBITMASK(0x00ff0000,0x0000ff00,0x000000ff,0x00000000))
				{
					GPUAPI_HALT("Unsupported DDS format!!!");
					return  TEXFMT_NULL;
				}

				// No DXGI format maps to ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0x00000000) aka D3DFMT_X8B8G8R8

				// Note that many common DDS reader/writers (including D3DX) swap the
				// the RED/BLUE masks for 10:10:10:2 formats. We assumme
				// below that the 'backwards' header mask is being used since it is most
				// likely written by D3DX. The more robust solution is to use the 'DX10'
				// header extension and specify the DXGI_FORMAT_R10G10B10A2_UNORM format directly

				// For 'correct' writers, this should be 0x000003ff,0x000ffc00,0x3ff00000 for RGB data
				if (ISBITMASK(0x3ff00000,0x000ffc00,0x000003ff,0xc0000000))
				{
					return TEXFMT_Float_R10G10B10A2;
				}

				// No DXGI format maps to ISBITMASK(0x000003ff,0x000ffc00,0x3ff00000,0xc0000000) aka D3DFMT_A2R10G10B10

				if (ISBITMASK(0x0000ffff,0xffff0000,0x00000000,0x00000000))
				{
					GPUAPI_HALT("Unsupported DDS format!!!");
					return  TEXFMT_NULL;
				}

				if (ISBITMASK(0xffffffff,0x00000000,0x00000000,0x00000000))
				{
					// Only 32-bit color channel format in D3D9 was R32F
					return TEXFMT_Float_R32; // D3DX writes this out as a FourCC of 114
				}
				break;

			case 24:
				// No 24bpp DXGI formats aka D3DFMT_R8G8B8
				break;

			case 16:
				if (ISBITMASK(0x7c00,0x03e0,0x001f,0x8000))
				{
					GPUAPI_HALT("Unsupported DDS format!!!");
					return  TEXFMT_NULL;
				}
				if (ISBITMASK(0xf800,0x07e0,0x001f,0x0000))
				{
					GPUAPI_HALT("Unsupported DDS format!!!");
					return  TEXFMT_NULL;
				}

				// No DXGI format maps to ISBITMASK(0x7c00,0x03e0,0x001f,0x0000) aka D3DFMT_X1R5G5B5

				if (ISBITMASK(0x0f00,0x00f0,0x000f,0xf000))
				{
					GPUAPI_HALT("Unsupported DDS format!!!");
					return  TEXFMT_NULL;
				}

				// No DXGI format maps to ISBITMASK(0x0f00,0x00f0,0x000f,0x0000) aka D3DFMT_X4R4G4B4

				// No 3:3:2, 3:3:2:8, or paletted DXGI formats aka D3DFMT_A8R3G3B2, D3DFMT_R3G3B2, D3DFMT_P8, D3DFMT_A8P8, etc.
				break;
			}
		}
		else if (ddpf.flags & DDS_LUMINANCE)
		{
			if (8 == ddpf.RGBBitCount)
			{
				if (ISBITMASK(0x000000ff,0x00000000,0x00000000,0x00000000))
				{
					return TEXFMT_L8; // D3DX10/11 writes this out as DX10 extension
				}

				// No DXGI format maps to ISBITMASK(0x0f,0x00,0x00,0xf0) aka D3DFMT_A4L4
			}

			if (16 == ddpf.RGBBitCount)
			{
				if (ISBITMASK(0x0000ffff,0x00000000,0x00000000,0x00000000))
				{
					return TEXFMT_L8; // D3DX10/11 writes this out as DX10 extension
				}
				if (ISBITMASK(0x000000ff,0x00000000,0x00000000,0x0000ff00))
				{
					return TEXFMT_A8L8; // D3DX10/11 writes this out as DX10 extension
				}
			}
		}
		else if (ddpf.flags & DDS_ALPHA)
		{
			if (8 == ddpf.RGBBitCount)
			{
				return TEXFMT_A8;
			}
		}
		else if (ddpf.flags & DDS_FOURCC)
		{
			if (MAKEFOURCC( 'D', 'X', 'T', '1' ) == ddpf.fourCC)
			{
				return TEXFMT_BC1;
			}
			if (MAKEFOURCC( 'D', 'X', 'T', '3' ) == ddpf.fourCC)
			{
				return TEXFMT_BC2;
			}
			if (MAKEFOURCC( 'D', 'X', 'T', '5' ) == ddpf.fourCC)
			{
				return TEXFMT_BC3;
			}

			// While pre-mulitplied alpha isn't directly supported by the DXGI formats,
			// they are basically the same as these BC formats so they can be mapped
			if (MAKEFOURCC( 'D', 'X', 'T', '2' ) == ddpf.fourCC)
			{
				return TEXFMT_BC2;
			}
			if (MAKEFOURCC( 'D', 'X', 'T', '4' ) == ddpf.fourCC)
			{
				return TEXFMT_BC3;
			}

			if (MAKEFOURCC( 'A', 'T', 'I', '1' ) == ddpf.fourCC)
			{
				return TEXFMT_BC4;
			}
			if (MAKEFOURCC( 'B', 'C', '4', 'U' ) == ddpf.fourCC)
			{
				return TEXFMT_BC4;
			}
			if (MAKEFOURCC( 'B', 'C', '4', 'S' ) == ddpf.fourCC)
			{
				GPUAPI_HALT("Unsupported DDS format!!!");
				return  TEXFMT_NULL;
			}

			if (MAKEFOURCC( 'A', 'T', 'I', '2' ) == ddpf.fourCC)
			{
				GPUAPI_HALT("Unsupported DDS format!!!");
				return  TEXFMT_NULL;
			}
			if (MAKEFOURCC( 'B', 'C', '5', 'U' ) == ddpf.fourCC)
			{
				GPUAPI_HALT("Unsupported DDS format!!!");
				return  TEXFMT_NULL;
			}
			if (MAKEFOURCC( 'B', 'C', '5', 'S' ) == ddpf.fourCC)
			{
				GPUAPI_HALT("Unsupported DDS format!!!");
				return  TEXFMT_NULL;
			}

			// BC6H and BC7 are written using the "DX10" extended header

			if (MAKEFOURCC( 'R', 'G', 'B', 'G' ) == ddpf.fourCC)
			{
				GPUAPI_HALT("Unsupported DDS format!!!");
				return  TEXFMT_NULL;
			}
			if (MAKEFOURCC( 'G', 'R', 'G', 'B' ) == ddpf.fourCC)
			{
				GPUAPI_HALT("Unsupported DDS format!!!");
				return  TEXFMT_NULL;
			}

			// Check for D3DFORMAT enums being set here
			switch( ddpf.fourCC )
			{
			case 36: // D3DFMT_A16B16G16R16
				GPUAPI_HALT("Unsupported DDS format!!!");
					return  TEXFMT_NULL;

			case 110: // D3DFMT_Q16W16V16U16
				GPUAPI_HALT("Unsupported DDS format!!!");
				return  TEXFMT_NULL;

			case 111: // D3DFMT_R16F
				return TEXFMT_Float_R16;

			case 112: // D3DFMT_G16R16F
				return TEXFMT_Float_R16G16;

			case 113: // D3DFMT_A16B16G16R16F
				return TEXFMT_Float_R16G16B16A16;

			case 114: // D3DFMT_R32F
				return TEXFMT_Float_R32;

			case 115: // D3DFMT_G32R32F
				return TEXFMT_Float_R32G32;

			case 116: // D3DFMT_A32B32G32R32F
				return TEXFMT_Float_R32G32B32A32;
			}
		}

		return DXGI_FORMAT_UNKNOWN;
	}*/


	////--------------------------------------------------------------------------------------
	//static DXGI_FORMAT MakeSRGB( DXGI_FORMAT format )
	//{
	//	switch( format )
	//	{
	//	case DXGI_FORMAT_R8G8B8A8_UNORM:
	//		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	//	case DXGI_FORMAT_BC1_UNORM:
	//		return DXGI_FORMAT_BC1_UNORM_SRGB;

	//	case DXGI_FORMAT_BC2_UNORM:
	//		return DXGI_FORMAT_BC2_UNORM_SRGB;

	//	case DXGI_FORMAT_BC3_UNORM:
	//		return DXGI_FORMAT_BC3_UNORM_SRGB;

	//	case DXGI_FORMAT_B8G8R8A8_UNORM:
	//		return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

	//	case DXGI_FORMAT_B8G8R8X8_UNORM:
	//		return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;

	//	case DXGI_FORMAT_BC7_UNORM:
	//		return DXGI_FORMAT_BC7_UNORM_SRGB;

	//	default:
	//		return format;
	//	}
	//}


	//--------------------------------------------------------------------------------------
	static Bool FillInitData( size_t width,
		 size_t height,
		 size_t depth,
		 size_t mipCount,
		 size_t arraySize,
		 DXGI_FORMAT format,
		 size_t maxsize,
		 size_t bitSize,
		 const Uint8* bitData,
		 size_t& twidth,
		 size_t& theight,
		 size_t& tdepth,
		 size_t& skipMip,
		 TextureLevelInitData* initData )
	{
		if ( !bitData || !initData )
		{
			return false;
		}

		skipMip = 0;
		twidth = 0;
		theight = 0;
		tdepth = 0;

		size_t NumBytes = 0;
		size_t RowBytes = 0;
		size_t NumRows = 0;
		const Uint8* pSrcBits = bitData;
		const Uint8* pEndBits = bitData + bitSize;

		size_t index = 0;
		for( size_t j = 0; j < arraySize; j++ )
		{
			size_t w = width;
			size_t h = height;
			size_t d = depth;
			for( size_t i = 0; i < mipCount; i++ )
			{
				GetSurfaceInfo( w,
					h,
					format,
					&NumBytes,
					&RowBytes,
					&NumRows
					);

				if ( (mipCount <= 1) || !maxsize || (w <= maxsize && h <= maxsize && d <= maxsize) )
				{
					if ( !twidth )
					{
						twidth = w;
						theight = h;
						tdepth = d;
					}

					GPUAPI_ASSERT(index < mipCount * arraySize);
					initData[index].m_data = ( const Uint8* )pSrcBits;
					initData[index].m_isCooked = false;
					++index;
				}
				else if ( !j )
				{
					// Count number of skipped mipmaps (first item only)
					++skipMip;
				}

				if (pSrcBits + (NumBytes*d) > pEndBits)
				{
					return false;
				}

				pSrcBits += NumBytes * d;

				w = w >> 1;
				h = h >> 1;
				d = d >> 1;
				if (w == 0)
				{
					w = 1;
				}
				if (h == 0)
				{
					h = 1;
				}
				if (d == 0)
				{
					d = 1;
				}
			}
		}

		return index > 0;
	}


	//--------------------------------------------------------------------------------------
	static void FillTextureDesc( Uint32 resDim,
		size_t width,
		size_t height,
		size_t depth,
		size_t mipCount,
		size_t arraySize,
		eTextureFormat format,
		Bool isCubeMap,
		TextureDesc& textureDesc)
	{
		switch ( resDim ) 
		{
		case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
			{
				GPUAPI_HALT( "1-dimensional DDS not supported!!!" );
			}
			break;

		case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
			{
				textureDesc.width = static_cast<Uint32>( width ); 
				textureDesc.height = static_cast<Uint32>( height ); 
				textureDesc.initLevels = static_cast<Uint16>( mipCount );
				textureDesc.sliceNum = static_cast<Uint16>( arraySize );
				textureDesc.format = format;
				textureDesc.usage = TEXUSAGE_Samplable;
				if ( isCubeMap )
				{
					textureDesc.type = TEXTYPE_CUBE;
				}
				else
				{
					textureDesc.type = TEXTYPE_2D;
				}
			}
			break;

		case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
			{
				textureDesc.width = static_cast<Uint32>( width ); 
				textureDesc.height = static_cast<Uint32>( height ); 
				textureDesc.initLevels = static_cast<Uint16>( mipCount );
				textureDesc.sliceNum = static_cast<Uint16>( arraySize );
				textureDesc.format = format;
				textureDesc.usage = TEXUSAGE_Samplable;
				textureDesc.type = TEXTYPE_ARRAY;
			}
			break; 
		}

		textureDesc.usage |= TEXUSAGE_Immutable;
	}


	//--------------------------------------------------------------------------------------

	static Bool CreateTextureFromDDS( const DDS_HEADER* header, const Uint8* bitData, size_t bitSize, size_t maxsize, TextureDesc& textureDesc, TextureInitData& initData )
	{
		Bool result = true;

		size_t width = header->width;
		size_t height = header->height;
		size_t depth = header->depth;

		Uint32 resDim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
		size_t arraySize = 1;
		DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;
		Bool isCubeMap = false;

		size_t mipCount = header->mipMapCount;
		if (0 == mipCount)
		{
			mipCount = 1;
		}

		if ( ( header->ddspf.flags & DDS_FOURCC ) && ( MAKEFOURCC( 'D', 'X', '1', '0' ) == header->ddspf.fourCC ) )
		{
			auto d3d10ext = reinterpret_cast<const DDS_HEADER_DXT10*>( (const char*)header + sizeof(DDS_HEADER) );

			arraySize = d3d10ext->arraySize;
			if (arraySize == 0)
			{
				return false;
			}

			if ( BitsPerPixel( d3d10ext->dxgiFormat ) == 0 )
			{
				return false;
			}

			dxgiFormat = d3d10ext->dxgiFormat;

			switch ( d3d10ext->resourceDimension )
			{
			case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
				// D3DX writes 1D textures with a fixed Height of 1
				if ((header->flags & DDS_HEIGHT) && height != 1)
				{
					return false;
				}
				height = depth = 1;
				break;

			case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
				if (d3d10ext->miscFlag & 0x4L/*D3D11_RESOURCE_MISC_TEXTURECUBE*/ )
				{
					arraySize *= 6;
					isCubeMap = true;
				}
				depth = 1;
				break;

			case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
				if (!(header->flags & DDS_HEADER_FLAGS_VOLUME))
				{
					return false;
				}

				if (arraySize > 1)
				{
					return false;
				}
				break;

			default:
				return false;
			}

			resDim = d3d10ext->resourceDimension;
		}
		else
		{
			dxgiFormat = GetDXGIFormat( header->ddspf );

			if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
			{
				return false;
			}

			if (header->flags & DDS_HEADER_FLAGS_VOLUME)
			{
				resDim = D3D11_RESOURCE_DIMENSION_TEXTURE3D;
			}
			else 
			{
				if (header->caps2 & DDS_CUBEMAP)
				{
					// We require all six faces to be defined
					if ((header->caps2 & DDS_CUBEMAP_ALLFACES ) != DDS_CUBEMAP_ALLFACES)
					{
						return false;
					}

					arraySize = 6;
					isCubeMap = true;
				}

				depth = 1;
				resDim = D3D11_RESOURCE_DIMENSION_TEXTURE2D;

				// Note there's no way for a legacy Direct3D 9 DDS to express a '1D' texture
			}

			GPUAPI_ASSERT( BitsPerPixel( dxgiFormat ) != 0 );
		}

		//////////////////////////////////////////////////////////////////////////
		// MG NOTE: I'm skipping some checks here, the format checks should be enough and we already know whether the contents are fine based on Windows platform
		//////////////////////////////////////////////////////////////////////////

		// Create the texture
		TextureLevelInitData* initMipData = new TextureLevelInitData[ mipCount * arraySize ];
		if ( !initMipData )
		{
			return false;
		}

		size_t skipMip = 0;
		size_t twidth = 0;
		size_t theight = 0;
		size_t tdepth = 0;
		result = FillInitData( width, height, depth, mipCount, arraySize, dxgiFormat, maxsize, bitSize, bitData,
			twidth, theight, tdepth, skipMip, initMipData );

		if ( result )
		{
			initData.m_isCooked = false;
			initData.m_mipsInitData = initMipData;

			eTextureFormat texFormat = MapDXGIToTexFormat( dxgiFormat );
			FillTextureDesc( resDim, twidth, theight, tdepth, mipCount - skipMip, arraySize, texFormat, isCubeMap, textureDesc );
		}

		return result;
	}

	//------------------------------------------------------------------------------------
	Bool CreateDDSTextureFromMemory( const Uint8* ddsData, size_t ddsDataSize, TextureDesc& textureDesc, TextureInitData& initData )
	{
		// Validate DDS file in memory
		if (ddsDataSize < (sizeof(Uint32) + sizeof(DDS_HEADER)))
		{
			// Minimum size fail
			return false;
		}

		Uint32 dwMagicNumber = *( const Uint32* )( ddsData );
		if (dwMagicNumber != DDS_MAGIC)
		{
			// Magic number fail
			return false;
		}

		const DDS_HEADER* header = reinterpret_cast<const DDS_HEADER*>( ddsData + sizeof( Uint32 ) );

		// Verify header to validate DDS file
		if ( header->size != sizeof(DDS_HEADER) || header->ddspf.size != sizeof(DDS_PIXELFORMAT) )
		{
			// Header contents differ from what's expected - fail
			return false;
		}

		// Check for DX10 extension
		Bool bDXT10Header = false;
		if ( ( header->ddspf.flags & DDS_FOURCC ) && (MAKEFOURCC( 'D', 'X', '1', '0' ) == header->ddspf.fourCC) )
		{
			// Must be long enough for both headers and magic value
			if (ddsDataSize < (sizeof(DDS_HEADER) + sizeof(Uint32) + sizeof(DDS_HEADER_DXT10)))
			{
				// Not long enough - fail
				return false;
			}

			bDXT10Header = true;
		}

		ptrdiff_t offset = sizeof( Uint32 )	+ sizeof( DDS_HEADER ) + (bDXT10Header ? sizeof( DDS_HEADER_DXT10 ) : 0);

		return CreateTextureFromDDS( header, ddsData + offset, ddsDataSize - offset, 0, textureDesc, initData );
	}

	void CleanupInitData( TextureInitData& initData )
	{
		const TextureLevelInitData* initMipData = initData.m_mipsInitData;
		delete [] initMipData;
	}
}