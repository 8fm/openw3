/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "common.h"


// This is a basic entry point, used by both TexCook tools.

std::wstringstream GOutput;


// Stubs just to fix compile errors (since we aren't linking with anything from redengine)

Red::System::Error::Handler * __cdecl Red::System::Error::Handler::GetInstance(void)
{
	return nullptr;
}
Red::System::Error::EAssertAction __cdecl Red::System::Error::HandleAssertion(wchar_t const *,unsigned int,wchar_t const *,wchar_t const *,...)
{
	return AA_ContinueAlways;
}



void ProcessingData::AllocateOutput( size_t size, size_t align )
{
	output = _aligned_malloc( size, align );
	outputSize = size;
	outputAlign = align;
}

extern "C" {

__declspec(dllexport) Bool __cdecl ProcessData( const SourceData& sourceData, Uint32* outMipOffsets, void*& outData, Uint32& outSize, Uint32& outAlign )
{
	GOutput.clear();

	GOutput << "mipCount:" << sourceData.mipCount << "\ttexCount:" << sourceData.texCount << "\tbaseWidth:" << sourceData.baseWidth << "\tbaseHeight" << sourceData.baseHeight << std::endl;

	ProcessingData data;
	data.mipCount	= sourceData.mipCount;
	data.texCount	= sourceData.texCount;
	data.baseWidth	= sourceData.baseWidth;
	data.baseHeight	= sourceData.baseHeight;
	data.texFormat	= sourceData.texFormat;
	data.texType	= sourceData.texType;

	data.texs.resize( sourceData.texCount );
	for ( Uint16 tex_i = 0; tex_i < sourceData.texCount; ++tex_i )
	{
		TexData& texData = data.texs[ tex_i ];
		texData.mips.resize( sourceData.mipCount );

		for ( Uint16 mip_i = 0; mip_i < sourceData.mipCount; ++mip_i )
		{
			Uint32 idx = mip_i + tex_i * sourceData.mipCount;

			texData.mips[ mip_i ].pitch	= sourceData.pitches[ idx ];
			texData.mips[ mip_i ].size	= sourceData.sizes[ idx ];
			texData.mips[ mip_i ].data	= sourceData.data[ idx ];
		}
	}

	if ( outMipOffsets == nullptr )
	{
		GOutput << "Null mip offsets output!" << std::endl;
		return false;
	}

	data.mipOffsets = outMipOffsets;

	if ( !DoProcess( data ) )
	{
		_aligned_free( data.output );
		return false;
	}

	outData		= data.output;
	outSize		= data.outputSize;
	outAlign	= data.outputAlign;

	return true;
}

__declspec(dllexport) void __cdecl ReleaseData( void* data )
{
	_aligned_free( data );
}


__declspec(dllexport) Bool __cdecl Verify( size_t sizeOfSourceData, GpuApi::eTextureFormat maxTextureFormat )
{
	return sizeOfSourceData == sizeof( SourceData ) && maxTextureFormat == GpuApi::TEXFMT_Max;
}

__declspec(dllexport) const Char* __cdecl GetOutputString()
{
	// HACK : Keep a static copy of the string around, so it doesn't get cleaned up. GOutput.str() just returns a temp variable.
	static std::wstring str;
	str = GOutput.str();
	return str.c_str();
}

}


BOOL WINAPI DllMain( HINSTANCE dllHandle, DWORD reason, LPVOID reserved )
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		/* Init Code here */
		break;

	case DLL_THREAD_ATTACH:
		/* Thread-specific init code here */
		break;

	case DLL_THREAD_DETACH:
		/* Thread-specific cleanup code here. */
		break;

	case DLL_PROCESS_DETACH:
		/* Cleanup code here */
		break;
	}

	/* The return value is used for successful DLL_PROCESS_ATTACH */
	return TRUE;
}
