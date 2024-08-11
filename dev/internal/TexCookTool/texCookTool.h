#pragma once

namespace TexCookTool
{

	// No members here, since we're going to be attaching to DLL at runtime, not linking to any .lib

	// Make sure we have the same packing from wcc and tool.
#pragma pack( push, 8 )
	struct SourceData
	{
		const void* const* data;
		const Uint32* pitches;
		const Uint32* sizes;

		GpuApi::eTextureFormat texFormat;
		GpuApi::eTextureType texType;

		Uint16 mipCount, texCount;
		Uint16 baseWidth, baseHeight;
	};
#pragma pack( pop )


	RED_INLINE Uint32 GetTexMipIndex( Uint32 mipIndex, Uint32 texIndex, Uint32 numMips )
	{
		return mipIndex + texIndex * numMips;
	}

	typedef Bool (__cdecl *ProcessDataFunc)( const SourceData& sourceData, Uint32* outMipOffsets, void*& outData, Uint32& outSize, Uint32& outAlign );
	typedef void (__cdecl *ReleaseDataFunc)( void* data );
	typedef Bool (__cdecl *VerifyFunc)( size_t sizeOfSourceData, GpuApi::eTextureFormat maxTextureFormat );
	typedef const Char* (__cdecl *GetOutputStringFunc)();

}
