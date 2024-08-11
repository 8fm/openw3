#pragma once

#include "materialCooker.h"
#include "processRunner.h"
#include "../../common/core/dependencyMapper.h"
#include "../../common/gpuApiUtils/gpuApiShaderParsing.h"
#include "../../common/redIO/redIO.h"
#include "../../common/redIO/redIOCommon.h"
#include "../../common/redSystem/crt.h"

typedef THashMap< String, String > TParameterDictionary;

class IShaderCompiler
{
public:
	enum EParameterType
	{
		STREAMOUT_DESC,
		INPUT_FILE,
		OUTPUT_FILE,
		ENTRY_POINT,
		SHADER_TARGET,
		SDB_DIR,
		INCLUDES_DIRECTORY,
		UPDB_FILE,
		ASSEMBLY_FILE,
	};

public:
	IShaderCompiler( const SMaterialCookingContext& context, const String& dumpFilename, const String& dumpDirPath )
		: m_id( InterlockedIncrement( &s_id ) )
		, m_context( context )
	{
		m_dumpFilenameValid = !dumpFilename.Empty();
		m_dumpFilename = dumpDirPath + TXT("dump_") + dumpFilename;
		m_errorsFilename = dumpDirPath + TXT("compilerErrors\\errors_") + dumpFilename;
		SetupFilesAndPaths( dumpDirPath );
	}
	virtual ~IShaderCompiler() {}

	void Initialize();

public:
	virtual Bool Compile( const AnsiChar* code, Uint32 codeLength, DataBuffer& shaderData, const Uint64& shaderHash ) = 0;

	RED_INLINE const TDynArray< GpuApi::ShaderDefine >& GetDefines() const { return m_defines; }
	RED_INLINE const AnsiChar* GetEntryPoint() const { return m_entryPoint.AsChar(); }
	RED_INLINE const AnsiChar* GetShaderTarget() const { return m_shaderTarget.AsChar(); }

public:
	static IShaderCompiler* Create( ECookingPlatform platform, const SMaterialCookingContext& context, const String& dumpFileName, const String& dumpDirPath );

protected:
	virtual Bool			GetShaderTargetAndEntryPoint( StringAnsi& outShaderTarget, StringAnsi& outEntryPoint ) = 0;
	virtual String			GetCommandline( const TParameterDictionary& params ) { return String::EMPTY; }
	virtual String			GetStreamOutDesc() { return String::EMPTY; }
	virtual void			AddPlatformSpecificDefines() { /*do nothing by default*/ }

protected:
	Bool					PrepareInputFile( const String& inFile, const AnsiChar* code, Uint32 codeLength );
	Bool					DumpShaderInfo( Uint64 shaderHash, const String& inFile = String::EMPTY );
	Bool					DumpError( const String& error );

protected:
	static String			GetParameterName( EParameterType paramType );

private:
	void					SetupFilesAndPaths( const String& dumpDirPath );
	void					ResolveDefines();

protected:
	String								m_appName;
	String								m_dumpFilename;
	String								m_errorsFilename;
	String								m_dumpDir;
	String								m_shortName;
	StringAnsi							m_shaderTarget;
	StringAnsi							m_entryPoint;
	TDynArray< GpuApi::ShaderDefine >	m_defines;
	LONG								m_id;
	SMaterialCookingContext				m_context;
	Bool								m_dumpFilenameValid;

protected:
	static volatile LONG	s_id;
};
