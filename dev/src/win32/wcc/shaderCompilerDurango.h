#pragma once

#include "shaderCompiler.h"

class ShaderCompilerDurango : public IShaderCompiler
{
public:
	ShaderCompilerDurango( const SMaterialCookingContext& context, const String& dumpFilename, const String& dumpDirPath )
		: IShaderCompiler( context, dumpFilename, dumpDirPath )
	{
		m_appName = TXT( ".\\tools\\FXC_Durango\\FXC.exe" );
	}

public:
	virtual Bool	Compile( const AnsiChar* code, Uint32 codeLength, DataBuffer& shaderData, const Uint64& shaderHash );

protected:
	virtual String	GetCommandline( const TParameterDictionary& params );
	virtual String	GetStreamOutDesc();
	virtual Bool	GetShaderTargetAndEntryPoint( StringAnsi& outShaderTarget, StringAnsi& outEntryPoint );
};