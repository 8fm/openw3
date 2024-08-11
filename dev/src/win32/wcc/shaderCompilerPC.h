#pragma once

#include "shaderCompiler.h"

class ShaderCompilerPC : public IShaderCompiler
{
public:
	ShaderCompilerPC( const SMaterialCookingContext& context, const String& dumpFilename, const String& dumpDirPath )
		: IShaderCompiler( context, dumpFilename, dumpDirPath )
	{ }

public:
	virtual Bool Compile( const AnsiChar* code, Uint32 codeLength, DataBuffer& shaderData, const Uint64& shaderHash );
	virtual Bool GetShaderTargetAndEntryPoint( StringAnsi& outShaderTarget, StringAnsi& outEntryPoint );
};