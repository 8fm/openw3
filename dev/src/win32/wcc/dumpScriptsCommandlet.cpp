/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../../common/core/objectGC.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/fileSys.h"
#include "../../common/core/rttiSerializer.h"
#include "../../common/core/scriptsOpcodeExaminer.h"
#include "reportWriter.h"

/// Commandlet that dumps the content of serialized RTTI data
class CDumpScriptsCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CDumpScriptsCommandlet, ICommandlet, 0 );

public:
	CDumpScriptsCommandlet();
	~CDumpScriptsCommandlet();

	// ICommandlet interface
	virtual const Char* GetOneLiner() const { return TXT("Dump data from compiled scripts"); }
	virtual bool Execute( const CommandletOptions& options );
	virtual void PrintHelp() const;
};

BEGIN_CLASS_RTTI( CDumpScriptsCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CDumpScriptsCommandlet );

CDumpScriptsCommandlet::CDumpScriptsCommandlet()
{
	m_commandletName = CName( TXT("dumpscripts") );
}

CDumpScriptsCommandlet::~CDumpScriptsCommandlet()
{
}

void CDumpScriptsCommandlet::PrintHelp() const
{
	LOG_WCC( TXT("Usage:") );
	LOG_WCC( TXT("  dumpscripts -file=<scriptfile.redscripts> -out=<outputfile.txt>") );
	LOG_WCC( TXT("Exclusive options:") );
	LOG_WCC( TXT("  -file=<scriptfile.redscripts>    - Input file (compiled script binary)") );
	LOG_WCC( TXT("  -out=<outputfile.txt>            - Output report file") );
}

static String GetFuncFullName( const CFunction* func )
{
	if ( func->GetClass() )
	{
		return func->GetClass()->GetName().AsString() + func->GetName().AsString();
	}
	else
	{
		return func->GetName().AsString();
	}
}

bool CDumpScriptsCommandlet::Execute( const CommandletOptions& options )
{
	String inputFilePath;
	if ( !options.GetSingleOptionValue( TXT("file"), inputFilePath ) )
	{
		ERR_WCC( TXT("Expecting input file") );
		return false;
	}

	String outputFilePath;
	if ( !options.GetSingleOptionValue( TXT("out"), outputFilePath ) )
	{
		ERR_WCC( TXT("Expecting output file") );
		return false;
	}

	LOG_WCC( TXT("Input file path: '%ls'"), inputFilePath.AsChar() );
	LOG_WCC( TXT("Output file path: '%ls'"), outputFilePath.AsChar() );

	// open source file
	Red::TScopedPtr< IFile > reader( GFileManager->CreateFileReader( inputFilePath, FOF_AbsolutePath | FOF_Buffered )  );
	if ( !reader )
	{
		ERR_WCC( TXT("Failed to create input file '%ls'"), inputFilePath.AsChar() );
		return false;
	}

	// dont bother with GC - it will probably crash
	GObjectGC->DisableGC();

	// read header
	CRTTISerializer::RTTIHeader header;
	header.Serialize( *reader );

	// unload current script data
	TDynArray< THandle< IScriptable > > allScriptables;
	//IScriptable::CollectAllScriptableObjects( allScriptables );
	SRTTI::GetInstance().ClearScriptData( &allScriptables );

	// shitty hack - we may be loading scripts so old that they will crash our engine
	extern Bool GRecomputeClassLayoutAfterRecompilation;
	GRecomputeClassLayoutAfterRecompilation = false;

	// load serialized RTTI data
	CRTTISerializer loader;
	if ( !loader.LoadScriptDataFromFile( reader.Get() ) )
	{
		ERR_WCC( TXT("Failed to load RTTI from '%ls'"), inputFilePath.AsChar() );
		return false;
	}

	// open writer
	Red::TScopedPtr< CHTMLReportWriter > writer( CHTMLReportWriter::Create( outputFilePath ) );
	if ( !writer )
	{
		ERR_WCC( TXT("Failed to create output file '%ls'"), inputFilePath.AsChar() );
		return false;
	}
	
	// report basic data
	writer->Writef( "File header:\n" );
	writer->Writef( "  Time: %hs\n", UNICODE_TO_ANSI( ToString( header.m_timestamp ).AsChar() ) );
	writer->Writef( "  Platform: %hs\n", UNICODE_TO_ANSI( header.m_platform.AsChar() ) );
	writer->Writef( "  AppVersion: %hs\n", UNICODE_TO_ANSI( header.m_appVersion.AsChar() ) );
	writer->Writef( "  Config: %hs\n", UNICODE_TO_ANSI( header.m_configuration.AsChar() ) );
	writer->Writef( "  Version: %d\n", header.m_scriptVersion );

	// get all classes (we will dump only the scripted ones)
	TDynArray< CClass* > allClasses;
	SRTTI::GetInstance().EnumClasses( ClassID< IScriptable >(), allClasses, nullptr, true );
	::Sort( allClasses.Begin(), allClasses.End(), [](CClass* a, CClass* b) { return Red::StringCompare( a->GetName().AsChar(), b->GetName().AsChar() ) < 0; } );

	// count scripted classes
	Uint32 numScriptedClasses = 0;
	for ( CClass* classInfo : allClasses )
	{
		if ( classInfo->IsScripted() )
		{
			numScriptedClasses += 1;
		}
	}

	// print class info
	writer->Writef( "Found %d scripted classes\n", numScriptedClasses );
	for ( CClass* classInfo : allClasses )
	{
		if ( classInfo->IsScripted() )
		{
			if ( classInfo->GetBaseClass() )
			{
				writer->Writef( "  Class %hs, base %hs, %d properties\n", 
					classInfo->GetName().AsAnsiChar(),
					classInfo->GetBaseClass()->GetName().AsAnsiChar(),
					classInfo->GetCachedProperties().Size() );
			}
			else 
			{
				writer->Writef( "  Class %hs, %d properties\n", 
					classInfo->GetName().AsAnsiChar(),
					classInfo->GetCachedProperties().Size() );
			}

			for ( CProperty* props : classInfo->GetLocalProperties() )
			{
				writer->Writef( "    '%hs' : '%hs'\n",
					props->GetName().AsAnsiChar(),
					props->GetType()->GetName().AsAnsiChar() );
			}
		}
	}
	writer->Writef( "\n", numScriptedClasses );

	// get all functions
	TDynArray< CFunction* > allFunctions;
	SRTTI::GetInstance().EnumFunctions( allFunctions );
	::Sort( allFunctions.Begin(), allFunctions.End(), [](CFunction* a, CFunction* b) { return GetFuncFullName(a) < GetFuncFullName(b); } );

	// print functions
	writer->Writef( "Found %d functions\n", allFunctions.Size() );
	for ( CFunction* funcInfo : allFunctions )
	{
		const AnsiChar* funcType = "";
		if ( funcInfo->IsOperator() )
		{
			funcType = "Operator ";
		}
		else if ( funcInfo->IsNative() )
		{
			funcType = "Native ";
		}

		if ( funcInfo->GetClass() )
		{
			writer->Writef( "%hsFunction '%hs' in '%hs'\n",
				funcType,
				funcInfo->GetName().AsAnsiChar(), funcInfo->GetClass()->GetName().AsAnsiChar() );
		}
		else
		{
			writer->Writef( "%hsGlobal Function '%hs'\n",
				funcType, funcInfo->GetName().AsAnsiChar() );
		}
		writer->Write( "---------------------------------\n" );

		// params
		TDynArray< CProperty*, MC_RTTI > allProps;
		funcInfo->GetProperties( allProps );

		// count params & locals
		Uint32 numArgs = 0;
		Uint32 numLocals = 0;
		for ( CProperty* prop : allProps )
		{
			if ( prop->GetFlags() & PF_FuncParam )
			{
				numArgs += 1;
			}
			else if ( prop->GetFlags() & PF_FuncLocal )
			{
				numLocals += 1;
			}
		}

		// params
		if ( numArgs > 0 )
		{
			writer->Writef( "%d parameters:\n", numArgs );
			for ( CProperty* prop : allProps )
			{
				if ( prop->GetFlags() & PF_FuncParam )
				{
					writer->Writef( "  '%hs': %hs\n", 
						prop->GetName().AsAnsiChar(),
						prop->GetType() ? prop->GetType()->GetName().AsAnsiChar() : "Unknown" );
				}
			}
		}

		// locals
		if ( numLocals > 0 )
		{
			writer->Writef( "%d locals:\n", numArgs );
			for ( CProperty* prop : allProps )
			{
				StringAnsi flags;

				if ( prop->GetFlags() & PF_FuncOptionaParam )
					flags += "OPTIONAL ";
				if ( prop->GetFlags() & PF_FuncOutParam )
					flags += "OUT ";
				if ( prop->GetFlags() & PF_FuncSkipParam )
					flags += "SKIP ";

				if ( prop->GetFlags() & PF_FuncLocal )
				{
					writer->Writef( "  %hs '%hs': %hs\n", 
						flags.AsChar(), 
						prop->GetName().AsAnsiChar(),
						prop->GetType() ? prop->GetType()->GetName().AsAnsiChar() : "Unknown" );
				}
			}
		}

		// code
		if ( nullptr != funcInfo->GetCode().GetCode() )
		{
			const Uint32 codeSize = (Uint32)( funcInfo->GetCode().GetCodeEnd() - funcInfo->GetCode().GetCode() );
			writer->Writef( "Code (%d bytes):\n", codeSize );

			CScriptOpCodeExaminer examiner;
			examiner.Examine( funcInfo );

			const auto& data = examiner.GetOutput();

			for ( const auto& op : data )
			{
				const Uint32 numLines = op.m_lines.Size();
				for( Uint32 j = 0; j < numLines; ++j )
				{
					const auto& line = op.m_lines[ j ];
					writer->Write( UNICODE_TO_ANSI( line.m_details.AsChar() ) );
				}
			}
		}

		// separator
		writer->Write( "\n" );
		writer->Write( "\n" );
	}

	// done
	return true;
}
