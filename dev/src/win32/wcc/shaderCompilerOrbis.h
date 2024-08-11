#pragma once

#include "shaderCompiler.h"

class ShaderCompilerOrbis : public IShaderCompiler
{
public:
	ShaderCompilerOrbis( const SMaterialCookingContext& context, const String& dumpFilename, const String& dumpDirPath )
		: IShaderCompiler( context, dumpFilename, dumpDirPath )
	{
		m_appName = TXT( ".\\tools\\PSSLC\\orbis-wave-psslc.exe" );
	}

public:
	virtual Bool Compile( const AnsiChar* code, Uint32 codeLength, DataBuffer& shaderData, const Uint64& shaderHash );

protected:
	virtual Bool GetShaderTargetAndEntryPoint( StringAnsi& outShaderTarget, StringAnsi& outEntryPoint );
	virtual String GetCommandline( const TParameterDictionary& params );
	virtual String GetStreamOutDesc();
	virtual void AddPlatformSpecificDefines();
};