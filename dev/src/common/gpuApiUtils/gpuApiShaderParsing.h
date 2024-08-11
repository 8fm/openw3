/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "gpuApiMemory.h"

namespace GpuApi
{
	Bool	IsCompilerShaderDefine( const ShaderDefine &define );
	Bool	Preprocess( const char* code, const ShaderDefine* defines, Uint32 numDefines, const char* fileName, char*& preprocessedCode, Uint32& preprocessedLength, void* (*Allocate)( Uint32 size ) = nullptr, void (*Free)( void* ptr ) = nullptr );
	Bool	GetShaderHash( Uint64& hash, const AnsiChar* code, const AnsiChar* entryPoint, const ShaderDefine* defines, Uint32 numDefines, const char* fileName, AnsiChar** preprocessedCode = nullptr, Uint32* preprocessedLength = nullptr, void* (*Allocate)( Uint32 size ) = nullptr, void (*Free)( void* ptr ) = nullptr );

	Uint64	GetFilenameHash( const AnsiChar* fileName, const ShaderDefine* defines, Uint32 numDefines, eShaderType shaderType );
}