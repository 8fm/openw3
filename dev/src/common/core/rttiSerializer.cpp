/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "rttiSerializer.h"

#include "enum.h"
#include "fileSys.h"
#include "engineTime.h"
#include "feedback.h"
#include "profiler.h"
#include "scriptingSystem.h"
#include "defaultValue.h"
#include "version.h"
#include "depot.h"
#include "objectGC.h"

// Introduced to be able to force recompilation manually when the format changes enough to warrant it
#define SCRIPT_VERSION	5

#define COMPILED_SCRIPTS_EXTENSION	TXT( "redscripts" )

//-----

CRTTISerializer::RTTIHeader::RTTIHeader()
	: m_scriptVersion(0) // basically - invalid header
	, m_crc(0)
{
}

void CRTTISerializer::RTTIHeader::Serialize( IFile& file )
{
	file << m_scriptVersion;
	file << m_platform;
	file << m_appVersion;
	file << m_timestamp;
	file << m_configuration;
	file << m_crc;
}

const CRTTISerializer::RTTIHeader& CRTTISerializer::RTTIHeader::GetPlatformDefault()
{
	static CRTTISerializer::RTTIHeader platformDefault;

	if ( platformDefault.m_scriptVersion != SCRIPT_VERSION )
	{
		platformDefault.m_timestamp		= GFileManager->GetFolderTimestamp( GFileManager->GetDataDirectory() + TXT( "scripts\\" ), true );
		platformDefault.m_platform		= MACRO_TXT( RED_EXPAND_AND_STRINGIFY( PROJECT_PLATFORM ) );
		platformDefault.m_appVersion	= MACRO_TXT( APP_VERSION_BUILD );
		platformDefault.m_scriptVersion	= SCRIPT_VERSION;
		platformDefault.m_configuration	= MACRO_TXT( RED_EXPAND_AND_STRINGIFY( PROJECT_CONFIGURATION ) );

		// Sanitize, as some options may contain spaces that we don't want
		platformDefault.m_configuration.RemoveWhiteSpaces();

		// crc is an odd one out since we set it directly at a higher level before saving the scripts
	}

	return platformDefault;
}

const String CRTTISerializer::RTTIHeader::GetFileNamePrefix() const
{
	// "x64.Noopts.redscripts"
	const Char* format = TXT( "%ls.%ls." ) COMPILED_SCRIPTS_EXTENSION;

	String filename = String::Printf( format, m_platform.AsChar(), m_configuration.AsChar() );
	filename.MakeLower();

	return filename;
}

bool CRTTISerializer::RTTIHeader::ValidateLoading( const RTTIHeader& expected, String& outReason, Uint32 validateHeaderMask ) const
{
	if ( m_scriptVersion != expected.m_scriptVersion && (validateHeaderMask & eHeaderValidateScriptVersion) )
	{
		outReason = String::Printf( TXT( "Compiled scripts version is out of date (%u), current version is %u" ), m_scriptVersion, expected.m_scriptVersion );
		return false;
	}

	if ( m_platform != expected.m_platform && (validateHeaderMask & eHeaderValidatePlatform) )
	{
		outReason = String::Printf( TXT( "Expected platform ('%ls') is different than the loaded scripts were compiled for ('%ls')" ), expected.m_platform.AsChar(), m_platform.AsChar() );
		return false;
	}

	if ( m_appVersion != expected.m_appVersion && (validateHeaderMask & eHeaderValidateAppVersion) )
	{
		outReason = String::Printf( TXT( "Compiler version ('%ls') used to compile the loaded scripts is not the same as actual ('%ls')" ), m_appVersion.AsChar(), expected.m_appVersion.AsChar() );
		return false;
	}

	if ( m_timestamp != expected.m_timestamp && (validateHeaderMask & eHeaderValidateTimestamp) )
	{
		outReason = String::Printf( TXT( "The scripts directory has changed since scripts were compiled" ) );
		return false;
	}

	if ( m_configuration != expected.m_configuration && (validateHeaderMask & eHeaderValidateConfig) )
	{
		outReason = String::Printf( TXT( "Scripts were compiled for '%ls' expected '%ls' -> recompilation needed." ), m_configuration.AsChar(), expected.m_configuration.AsChar() );
		return false;
	}

	return true;
}

//-----

CRTTISerializer::CRTTISerializer()
{

}

CRTTISerializer::~CRTTISerializer()
{

}

const String CRTTISerializer::GetDefaultFileName()
{
	return TXT("scripts.") COMPILED_SCRIPTS_EXTENSION;
}

const String CRTTISerializer::GetPlatformFileName()
{
	return RTTIHeader::GetPlatformDefault().GetFileNamePrefix();
}

Bool CRTTISerializer::SaveScriptData( const String& absoluteFilePath, Uint64 crc /*=0*/ )
{
	// create writer
	IFile* fileWriter = GFileManager->CreateFileWriter( absoluteFilePath, FOF_AbsolutePath | FOF_Buffered );
	if ( !fileWriter )
	{
		ERR_CORE( TXT("Failed to create output file '%ls'"), absoluteFilePath.AsChar() );
		return false;
	}

	// build header for local platform
	RTTIHeader header = RTTIHeader::GetPlatformDefault();
	header.m_crc = crc;
	header.Serialize( *fileWriter );

	// save data
	const bool saved = SaveScriptDataToFile( fileWriter );
	delete fileWriter;

	// not saved - delete output file
	if ( !saved )
	{
		ERR_CORE( TXT("Failed to save RTTI to file '%ls'"), absoluteFilePath.AsChar() );
		GFileManager->DeleteFile( absoluteFilePath );
	}

	return saved;
}

Bool CRTTISerializer::SaveScriptDataToFile( IFile* file )
{
	// Find their properties etc.
	for ( auto i = SRTTI::GetInstance().m_types.Begin(); i != SRTTI::GetInstance().m_types.End(); ++i )
	{
		IRTTIType* type = i->m_second;
		MapType( type, false );

		if ( type->GetType() == RT_Class )
		{
			CClass* classType = static_cast< CClass* >( type );

			Uint32 scriptedPropertiesCounter = 0;
			Uint32 exportedPropertiesCounter = 0;

			// Collect all properties
			for ( Uint32 i = 0; i < classType->m_localProperties.Size(); ++i )
			{
				CProperty* prop = classType->m_localProperties[i];

				if ( !classType->IsNative() || prop->IsScripted() )
				{
					++scriptedPropertiesCounter;
					MapType( prop->GetType(), false );
				}
				else if ( prop->IsExported() )
				{
					++exportedPropertiesCounter;
					MapType( prop->GetType(), false );
				}
			}

			Uint32 scriptedFuncCounter = 0;
			Uint32 exportedFuncCounter = 0;
			
			// Collect all functions
			for ( Uint32 i = 0; i < classType->m_localFunctions.Size(); ++i )
			{
				CFunction* func = classType->m_localFunctions[i];

				if ( !func->IsNative() )
				{
					MapFunction( func, MapType( type, true ), false );
					++scriptedFuncCounter;
				}

				if ( func->IsExported() || func->IsExec() )
				{
					MapFunction( func, MapType( type, false ), false );
					++exportedFuncCounter;
				}
			}
			
			// Gather info about base class
			if ( (scriptedPropertiesCounter > 0) || (scriptedFuncCounter > 0) || classType->IsScripted() )
			{
				if ( classType->HasBaseClass() )
				{
					MapType( classType->GetBaseClass(), false );
				}

				Uint32 index = MapType( type, true );
				m_allTypesDefinitions[ index ].m_numScriptedProperties = scriptedPropertiesCounter;
				m_allTypesDefinitions[ index ].m_numExportedProperties = exportedPropertiesCounter;
			}

			// Auto state name
			MapName( classType->m_autoStateName );

			//Gather info about state machine
			if ( classType->m_stateMachineClass )
			{
				MapType( classType->m_stateMachineClass, false );
			}

			//Gather info about all state classes
			if ( classType->m_stateClasses.Size() > 0 )
			{
				for ( Uint32 i = 0; i < classType->m_stateClasses.Size(); ++i )
				{
					MapType( classType->m_stateClasses[ i ], false );
				}
			}
		}
	}

	// Find interesting enums
	for ( auto i=SRTTI::GetInstance().m_types.Begin(); i != SRTTI::GetInstance().m_types.End(); ++i )
	{
		IRTTIType* type = i->m_second;
		if ( type->GetType() == RT_Enum )
		{
			CEnum* enumType = static_cast< CEnum* >( type );
			if ( enumType->IsScripted() )
			{
				MapType( type, true );
			}
		}
	}

	// Find interesting global functions
	for ( auto i=SRTTI::GetInstance().m_globalFunctions.Begin(); i != SRTTI::GetInstance().m_globalFunctions.End(); ++i	)
	{
		CFunction* func = i->m_second;
		MapFunction( func, 0, true );
	}

	SerializeIndexMaps( file );
	
	struct Local
	{
		static void UpdateSplash( EngineTime& lastSplashUpdate, Uint32& savingItem, Uint32 totalSize )
		{
			// Splash screen info, twice per second
			++savingItem;
			if ( EngineTime::GetNow() - lastSplashUpdate > 0.5f )
			{
				lastSplashUpdate = EngineTime::GetNow();
				GSplash->UpdateProgress( TXT("Saving scripts (item %i/%i)..."), savingItem, totalSize );
			}
		}
	};

	EngineTime lastSplashUpdate( 0.f );
	Uint32 savingItem( 0 ), totalItems( m_allTypesDefinitions.Size() + m_allFunctionsDefinitions.Size() );

	// enums have to go first
	for ( Uint32 i = 0; i < m_allTypesDefinitions.Size(); ++i )
	{
		if ( m_allTypesDefinitions[i].m_hasScriptData && ( m_allTypesDefinitions[i].m_type == RT_Enum ) )
		{
			Local::UpdateSplash( lastSplashUpdate, savingItem, totalItems );
			SerializeType( file, m_allTypes[i] );
		}
	}

	// then classes
	for ( Uint32 i = 0; i < m_allTypesDefinitions.Size(); ++i )
	{
		if ( m_allTypesDefinitions[i].m_hasScriptData && ( m_allTypesDefinitions[i].m_type == RT_Class ) )
		{
			Local::UpdateSplash( lastSplashUpdate, savingItem, totalItems );
			SerializeType( file, m_allTypes[i] );
		}
	}

	// then classes default values
	for ( Uint32 i = 0; i < m_allTypesDefinitions.Size(); ++i )
	{
		if ( m_allTypesDefinitions[i].m_hasScriptData && ( m_allTypesDefinitions[i].m_type == RT_Class ) )
		{
			Local::UpdateSplash( lastSplashUpdate, savingItem, totalItems );
			SerializeDefaultValues( file, m_allTypes[i] );
		}
	}

	// finally functions
	for ( Uint32 i = 0; i < m_allFunctionsDefinitions.Size(); ++i )
	{
		if ( m_allFunctionsDefinitions[i].m_hasScriptData )
		{
			Local::UpdateSplash( lastSplashUpdate, savingItem, totalItems );
			SerializeFunction( file, m_allFunctions[i] );
		}
	}

	for ( Uint32 i = 0; i < m_allTypesDefinitions.Size(); ++i )
	{
		if ( m_allTypesDefinitions[i].m_hasScriptData && m_allTypesDefinitions[i].m_type == RT_Class && m_allTypes[i] != NULL )
		{
			static_cast<CClass*>( m_allTypes[i] )->RecalculateClassDataSize();
		}
	}

	Uint32 offset = static_cast< Uint32 >( file->GetOffset() );

	Uint32 siz = m_names.Size();
	*file << siz;

	for ( Uint32 i = 0; i < m_names.Size(); ++i )
	{
		*file << m_names[i].m_name;
	}

	*file << offset;
	return true;
}

Bool CRTTISerializer::LoadScriptData( const String& depotFilePath, Uint64 expectedCrc, Uint32 headerValidationMask /*= eHeaderValidateAll*/ )
{
	// assemble path to script file
	String scriptsPath;
	GDepot->GetAbsolutePath(scriptsPath);
	scriptsPath += depotFilePath;

	// open the file
	Red::TScopedPtr<IFile> file( GFileManager->CreateFileReader( scriptsPath, FOF_AbsolutePath | FOF_MapToMemory ) );
	if ( !file )
	{
		WARN_CORE( TXT("Unable to open depot file '%ls'. Unable to load scripts from it."), depotFilePath.AsChar() );
		return false;
	}

	// load header
	RTTIHeader header;
	header.Serialize( *file );

	if ( expectedCrc != 0 && expectedCrc != header.m_crc )
	{
		WARN_CORE( TXT("Unable to load scripts from file '%ls' because expectedCrc 0x%016llX is not zero and they do not match scripts CRC 0x%016llX"), 
			depotFilePath.AsChar(), expectedCrc, header.m_crc );

		return false;
	}

	// validate the header against our platform if we are asked to do so
	if ( headerValidationMask != eHeaderValidateNone )
	{
		String validationError;
		if ( !header.ValidateLoading( RTTIHeader::GetPlatformDefault(), /*out*/ validationError, headerValidationMask ) )
		{
			WARN_CORE( TXT("Unable to load scripts from file '%ls' because they do not match current platform. Reason: %ls"), 
				depotFilePath.AsChar(), validationError.AsChar() );

			return false;
		}
	}

	// print stats
	LOG_CORE( TXT("Loading compiled scripts:") );
	LOG_CORE( TXT(" - version: %d"), header.m_scriptVersion );
	LOG_CORE( TXT(" - time stamp: %ls"), ToString( header.m_timestamp ).AsChar() );
	LOG_CORE( TXT(" - app version: %ls"), header.m_appVersion.AsChar() );
	LOG_CORE( TXT(" - platform: %ls"), header.m_platform.AsChar() );
	LOG_CORE( TXT(" - configuration: %ls"), header.m_configuration.AsChar() );
	LOG_CORE( TXT(" - crc: 0x%016llX"), header.m_crc );

	// load the data
	if ( !LoadScriptDataFromFile( file.Get() ) )
		return false;

	// scripts loaded
	return true;
}

Bool CRTTISerializer::LoadScriptDataFromFile( IFile* file )
{
	CTimeCounter timer;

	Bool failed = false;

	Uint32 namesOffset;
	Uint32 oldOffset = static_cast< Uint32 >( file->GetOffset() );
	file->Seek( file->GetSize() - sizeof(namesOffset) );
	*file << namesOffset;
	file->Seek( namesOffset );

	Uint32 siz;
	*file << siz;
	m_names.Resize( siz );

	for ( Uint32 i = 0; i < m_names.Size(); ++i )
	{
		*file << m_names[i].m_name;

		m_names[i].m_cName = CName( m_names[i].m_name );
	}

	file->Seek( oldOffset );


	SerializeIndexMaps( file );

	// enums have to go first
	for ( Uint32 i = 0; i < m_allTypesDefinitions.Size(); ++i )
	{
		if ( m_allTypesDefinitions[i].m_hasScriptData && ( m_allTypesDefinitions[i].m_type == RT_Enum ) )
		{
			DeserializeType( file );
		}
	}

	for ( Uint32 i = 0; i < m_allTypesDefinitions.Size(); ++i )
	{
		// create or bind classes
		if ( m_allTypesDefinitions[i].m_type == RT_Class )
		{
			if ( !m_allTypesDefinitions[i].m_isNative )
			{
				m_allTypes[i] = SRTTI::GetInstance().CreateScriptedClass( m_names[m_allTypesDefinitions[i].m_nameAsString].m_cName, CF_Scripted );
			}
			else
			{
				CClass* existing = static_cast<CClass*>( SRTTI::GetInstance().FindType( m_names[m_allTypesDefinitions[i].m_nameAsString].m_cName ) );
				if ( existing )
				{
					existing->MarkAsExported();

					if ( m_allTypesDefinitions[i].m_isScripted )
					{
						existing->MarkAsScripted();
					}

					m_allTypes[i] = existing;
				}
				else
				{
					m_allTypes[i] = NULL;
				}
			}
		}
		else
		{
			m_allTypes[i] = SRTTI::GetInstance().FindType( m_names[m_allTypesDefinitions[i].m_nameAsString].m_cName );
		}
	}

	for ( Uint32 i = 0; i < m_allFunctionsDefinitions.Size(); ++i )
	{
		const CName& funcName = m_names[m_allFunctionsDefinitions[i].m_nameAsString].m_cName;

		if ( m_allFunctionsDefinitions[i].m_isNative )
		{
			// bind
			if ( m_allFunctionsDefinitions[i].m_isGlobal )
			{
				m_allFunctions[i] = SRTTI::GetInstance().FindGlobalFunction( funcName );
			}
			else
			{
				CClass* classType = static_cast<CClass*>( m_allTypes[ m_allFunctionsDefinitions[i].m_baseClassType ] );
				const CFunction* func = classType->FindFunctionNonCached( funcName );
				m_allFunctions[i] = const_cast< CFunction* >( func );
			}
		}
		else
		{
			if ( m_allFunctionsDefinitions[i].m_isGlobal )
			{
				CFunction* func = new CFunction( NULL, funcName, (Uint32)0 );
				SRTTI::GetInstance().RegisterGlobalFunction( func );
				m_allFunctions[i] = func;
			}
			else
			{
				CClass* classType = static_cast<CClass*>( m_allTypes[ m_allFunctionsDefinitions[i].m_baseClassType ] );
				CFunction* func = new CFunction( classType, funcName, (Uint32)0 );
				classType->AddFunction( func );
				m_allFunctions[i] = func;
			}
		}

		if ( NULL == m_allFunctions[i] )
		{
			WARN_CORE( TXT("Missing function: '%ls'"), funcName.AsString().AsChar() );
		}
	}

	// then load classes
	for ( Uint32 i = 0; i < m_allTypesDefinitions.Size(); ++i )
	{
		if ( m_allTypesDefinitions[i].m_hasScriptData && m_allTypesDefinitions[i].m_type == RT_Class )
		{
			if( !DeserializeType( file ) )
			{
				failed = true;
				break;
			}
		}
	}

	for ( Uint32 i = 0; i < m_allTypesDefinitions.Size(); ++i )
	{
		if ( m_allTypesDefinitions[i].m_hasScriptData && m_allTypesDefinitions[i].m_type == RT_Class && m_allTypes[i] != NULL )
		{
			static_cast<CClass*>( m_allTypes[i] )->RecalculateClassDataSize();
		}
	}

	if( !failed )
	{
		// then load default values
		for ( Uint32 i = 0; i < m_allTypesDefinitions.Size(); ++i )
		{
			if ( m_allTypesDefinitions[i].m_hasScriptData && m_allTypesDefinitions[i].m_type == RT_Class )
			{
				DeserializeDefaultValues( file );
			}
		}
	}

	if( !failed )
	{
		for ( Uint32 i = 0; i < m_allFunctionsDefinitions.Size(); ++i )
		{
			if ( m_allFunctionsDefinitions[i].m_hasScriptData )
			{
				if( !DeserializeFunction( file ) )
				{
					failed = true;
					break;
				}
			}
		}
	}

	if( !failed )
	{
		RecalculateDataLayout();

		LOG_CORE( TXT( "Cached scripts loaded in %.2fs" ), timer.GetTimePeriod() );

		GScriptingSystem->m_isValid = true;
	}

	return !failed;
}

Uint32 CRTTISerializer::MapType( IRTTIType* type, Bool hasScriptData )
{
	Uint32 index; 

	if (type->GetType() == RT_Array) 
	{
		CRTTIArrayType* arrayType = static_cast< CRTTIArrayType* >( type );
		IRTTIType* elementType = arrayType->GetInnerType();
		MapType( elementType, hasScriptData );
	}

	else if (type->GetType() == RT_NativeArray)
	{
		CRTTINativeArrayType* arrayType = static_cast< CRTTINativeArrayType* >( type );
		IRTTIType* elementType = arrayType->GetInnerType();
		MapType( elementType, hasScriptData );
	}

	else if ( (type->GetType() == RT_Pointer) || 
		 (type->GetType() == RT_Handle) || 
		 (type->GetType() == RT_SoftHandle) )
	{
		IRTTIPointerTypeBase* pointerType = static_cast<IRTTIPointerTypeBase*>( type );
		IRTTIType* pointedType = pointerType->GetPointedType();
		MapType( pointedType, hasScriptData );
	}


	if ( !m_mappedTypes.Find( type, index ) )
	{
		RTTITypeDef typeDef;
		typeDef.m_nameAsString = MapName( type->GetName() );
		typeDef.m_type = type->GetType();
		typeDef.m_hasScriptData = hasScriptData;
		typeDef.m_isScripted = false;
		if ( type->GetType() == RT_Class )
		{
			CClass* theClass = static_cast< CClass* >( type );
			typeDef.m_isNative = theClass->IsNative();
			typeDef.m_isScripted = theClass->IsScripted();
		}
		else
		{
			typeDef.m_isNative = false;
			typeDef.m_isScripted = false;
		}

		typeDef.m_numExportedProperties = 0;
		typeDef.m_numScriptedProperties = 0;

		m_allTypesDefinitions.PushBack( typeDef );
		m_allTypes.PushBack( type );
		m_mappedTypes.Insert( type, m_allTypesDefinitions.Size() - 1 );
		return m_allTypesDefinitions.Size() - 1;
	}
	else
	{
		if ( !m_allTypesDefinitions[index].m_hasScriptData && hasScriptData )
		{
			m_allTypesDefinitions[index].m_hasScriptData = hasScriptData;
		}

		return index;
	}
}

Uint32 CRTTISerializer::MapFunction( CFunction* f, Uint32 baseClassTypeIndex, Bool isGlobal )
{
	Uint32 index; 

	if ( !m_mappedFunctions.Find( f, index ) )
	{
		RTTIFuncDef funcDef;
//		funcDef.m_name = f->GetName();
		funcDef.m_nameAsString = MapName( f->GetName() );
		funcDef.m_isGlobal = isGlobal;
		funcDef.m_baseClassType = baseClassTypeIndex;

		funcDef.m_isNative = f->IsNative();

		funcDef.m_hasScriptData = !f->IsNative() || f->IsExec() || f->IsExported();

		m_allFunctionsDefinitions.PushBack( funcDef );
		m_allFunctions.PushBack( f );
		m_mappedFunctions.Insert( f, m_allFunctionsDefinitions.Size() - 1 );
		return m_allFunctionsDefinitions.Size() - 1;
	}
	else
	{
		return index;
	}
}

void CRTTISerializer::SerializeIndexMaps( IFile* file )
{
	Uint32 numTypes;
	Uint32 numFunctions;

	if ( file->IsWriter() )
	{
		numTypes = m_allTypesDefinitions.Size();
		*file << numTypes;
		numFunctions = m_allFunctionsDefinitions.Size();
		*file << numFunctions;
	}
	else
	{
		*file << numTypes;
		*file << numFunctions;
		m_allTypesDefinitions.Resize( numTypes );
		m_allTypes.Resize( numTypes );
		m_allFunctionsDefinitions.Resize( numFunctions );
		m_allFunctions.Resize( numFunctions );
	}

	for ( Uint32 i = 0; i < numTypes; ++i )
	{
		*file << CCompressedNumSerializer( m_allTypesDefinitions[i].m_nameAsString );
		Uint32 type = static_cast<Uint32>( m_allTypesDefinitions[i].m_type );
		*file << CCompressedNumSerializer( type );

		if ( file->IsReader() )
		{
			m_allTypesDefinitions[i].m_type = static_cast<ERTTITypeType>( type );
		}

		*file << CCompressedNumSerializer( m_allTypesDefinitions[i].m_numExportedProperties );
		*file << CCompressedNumSerializer( m_allTypesDefinitions[i].m_numScriptedProperties );
		Uint32 flag = 0;
		flag |= m_allTypesDefinitions[i].m_isScripted ? 4 : 0;
		flag |= m_allTypesDefinitions[i].m_isNative ? 2 : 0;
		flag |= m_allTypesDefinitions[i].m_hasScriptData ? 1 : 0;
		*file << CCompressedNumSerializer( flag );
		
		if ( file->IsReader() )
		{
			if ( flag & 1 )
			{
				m_allTypesDefinitions[i].m_hasScriptData = true;
			}
			if ( flag & 2 )
			{
				m_allTypesDefinitions[i].m_isNative = true;
			}
			if ( flag & 4 )
			{
				m_allTypesDefinitions[i].m_isScripted = true;
			}
		}
	}

	for ( Uint32 i = 0; i < numFunctions; ++i )
	{
		*file << CCompressedNumSerializer( m_allFunctionsDefinitions[i].m_nameAsString );
		*file << CCompressedNumSerializer( m_allFunctionsDefinitions[i].m_baseClassType );
		Uint32 flag = 0;
		flag |= m_allFunctionsDefinitions[i].m_isGlobal ? 4 : 0;
		flag |= m_allFunctionsDefinitions[i].m_isNative ? 2 : 0;
		flag |= m_allFunctionsDefinitions[i].m_hasScriptData ? 1 : 0;;
		*file << CCompressedNumSerializer( flag );

		if ( file->IsReader() )
		{
			if ( flag & 1 )
			{
				m_allFunctionsDefinitions[i].m_hasScriptData = true;
			}
			if ( flag & 2 )
			{
				m_allFunctionsDefinitions[i].m_isNative = true;
			}
			if ( flag & 4 )
			{
				m_allFunctionsDefinitions[i].m_isGlobal = true;
			}
		}
	}
}

void CRTTISerializer::SerializeDefaultValue( IFile* file, CDefaultValue* val, CClass* classType )
{
	if( file->IsWriter() )
	{
		CName name = val->m_property->GetName();
		Uint32 nameIndex = MapName( name );
		*file << CCompressedNumSerializer( nameIndex );

		ERTTITypeType typeType = val->m_property->GetType()->GetType();

		Uint32 typeAsInt = static_cast<Uint32>( typeType );
		*file << CCompressedNumSerializer( typeAsInt );

		if ( typeType == RT_Enum || typeType == RT_Simple || typeType == RT_Fundamental )
		{
			*file << val->m_data;
		}
		else if( typeType == RT_Class )
		{
			const TDynArray< CDefaultValue* >& subValues = val->GetSubValues();
			
			CName className( val->GetProperty()->GetType()->GetName() );
			*file << className;

			Uint32 numSubValues = subValues.Size();
			*file << CCompressedNumSerializer( numSubValues );

			for( Uint32 i = 0; i < numSubValues; ++i )
			{
				SerializeDefaultValue( file, subValues[ i ], ( CClass* ) val->GetProperty()->GetType() );
			}
		} 
		else
		{
			RED_WARNING( 0, "TODO: AFTER BETA! Array, class and handle type" );
		}
	}
	else
	{
		// property name
		Uint32 name;
		*file << CCompressedNumSerializer( name );

		CName propName = m_names[name].m_cName;

		// property type
		Uint32 typeAsInt;
		*file << CCompressedNumSerializer( typeAsInt );

		ERTTITypeType typeType = static_cast<ERTTITypeType>( typeAsInt );

		ASSERT( classType->FindProperty( propName ) );
		val->SetProperty( classType->FindProperty( propName ) );

		if ( typeType == RT_Enum || typeType == RT_Simple || typeType == RT_Fundamental )
		{
			*file << val->m_data;
		}
		else if( typeType == RT_Class )
		{
			CName className;
			*file << className;

			CClass* subClassType = SRTTI::GetInstance().FindClass( className );
			ASSERT( subClassType != NULL );

			Uint32 numSubValues;
			*file << CCompressedNumSerializer( numSubValues );

			for( Uint32 i = 0; i < numSubValues; ++i )
			{
				CDefaultValue* defValue = new CDefaultValue();
				SerializeDefaultValue( file, defValue, subClassType );

				val->AddSubValue( defValue );
			}
		}
		else
		{
			HALT( "TODO: AFTER BETA! Array and handle type" );
		}
	}
}

void CRTTISerializer::SerializeType( IFile* file, IRTTIType* type )
{
	ASSERT( ( type->GetType() == RT_Class ) || ( type->GetType() == RT_Enum ) );

	ASSERT( m_mappedTypes.KeyExist( type ) );

	Uint32 typeNum = static_cast<Uint32>( type->GetType() );
	*file << CCompressedNumSerializer( typeNum );
	*file << CCompressedNumSerializer( *m_mappedTypes.FindPtr( type ) );

	if ( type->GetType() == RT_Class )
	{
		CClass* classType = static_cast< CClass* >( type );
		
		// Num of base classes, 0 if struct
		Uint32 num = classType->HasBaseClass();
		*file << CCompressedNumSerializer( num );
		
		if ( classType->HasBaseClass() )
		{
			ASSERT( m_mappedTypes.KeyExist( classType->GetBaseClass() ) );

			// Write our index of base class 
			*file << CCompressedNumSerializer( *m_mappedTypes.FindPtr( classType->GetBaseClass() ) );
		}

		// Write class flags
		const Uint32 flagMask = ~( (Uint32) CF_UndefinedFunctions );
		Uint32 flagsToSave = classType->m_flags & flagMask;
		*file << flagsToSave;

		// Write auto state name
		Uint32 autoStateName = MapName( classType->m_autoStateName );
		*file << CCompressedNumSerializer( autoStateName );

		//Write info about num of states
		num = classType->m_stateClasses.Size();
		*file << CCompressedNumSerializer( num );

		// Write info about each state
		for( Uint32 i = 0; i < num; ++i )
		{
			ASSERT( m_mappedTypes.KeyExist( classType->m_stateClasses[i] ) );
			Uint32 stateName = MapName( classType->m_stateClasses[i]->m_stateName );
			*file << CCompressedNumSerializer( stateName );
			*file << CCompressedNumSerializer( *m_mappedTypes.FindPtr( classType->m_stateClasses[i] ) );
		}


		// Gather info about scripted properties
		{
			Uint32 counter = 0;
			for ( Uint32 i=0; i<classType->m_localProperties.Size(); i++ )
			{
				CProperty* prop = classType->m_localProperties[i];

				if ( !classType->IsNative() || prop->IsScripted() )
				{
					ASSERT( m_mappedTypes.KeyExist( prop->GetType() ) );
					*file << CCompressedNumSerializer( *m_mappedTypes.FindPtr( prop->GetType() ) );
					Uint32 name = MapName( prop->GetName() );
					*file << CCompressedNumSerializer( name );
					Uint32 flags = prop->GetFlags();
					*file << CCompressedNumSerializer( flags );
					++counter;
				}
			}

			ASSERT( counter == m_allTypesDefinitions[ *m_mappedTypes.FindPtr( type )].m_numScriptedProperties );
		}

		// Gather info about exported properties
		{
			Uint32 counter = 0;
			for ( Uint32 i=0; i<classType->m_localProperties.Size(); i++ )
			{
				CProperty* prop = classType->m_localProperties[i];

				if ( prop->IsExported() )
				{
					Uint32 name = MapName( prop->GetName() );
					*file << CCompressedNumSerializer( name );
					++counter;
				}
			}

			ASSERT( counter == m_allTypesDefinitions[ *m_mappedTypes.FindPtr( type )].m_numExportedProperties );
		}

		// Save auto binding information
		{
			Uint32 count = classType->m_localAutoBinding.Size();
			*file << CCompressedNumSerializer( count );

			for ( Uint32 i=0; i<classType->m_localAutoBinding.Size(); i++ )
			{
				auto& prop = classType->m_localAutoBinding[i];

				Uint32 propertyName = MapName( prop.m_propertyName );
				*file << CCompressedNumSerializer( propertyName );

				Uint32 bindingName = MapName( prop.m_bindingInformation );
				*file << CCompressedNumSerializer( bindingName );
			}
		}
	}

	if ( type->GetType() == RT_Enum )
	{
		CEnum* enumType = static_cast< CEnum* >( type );
		Uint32 num = enumType->GetSize();
		*file << CCompressedNumSerializer( num );
		Uint32 numOptions = enumType->GetOptions().Size();
		*file << CCompressedNumSerializer( numOptions );
		for ( Uint32 i = 0; i < numOptions; ++i )
		{
			Uint32 name = MapName( enumType->GetOptions()[i] );
			*file << CCompressedNumSerializer( name );
			Int32 value;
			enumType->FindValue( enumType->GetOptions()[i], value );
			*file << CCompressedNumSerializer( value );
		}
	}

}


void CRTTISerializer::SerializeDefaultValues( IFile* file, IRTTIType* type )
{
	ASSERT( ( type->GetType() == RT_Class ) );

	ASSERT( m_mappedTypes.KeyExist( type ) );

	*file << CCompressedNumSerializer( *m_mappedTypes.FindPtr( type ) );

	if ( type->GetType() == RT_Class )
	{
		CClass* classType = static_cast< CClass* >( type );

		Uint32 num;

		// Gather info about default values
		num = classType->m_defaultValues.Size();
		*file << CCompressedNumSerializer( num );
		for ( Uint32 i = 0; i < classType->m_defaultValues.Size(); ++i )
		{
			SerializeDefaultValue( file, classType->m_defaultValues[i], classType );
		}
	}

}


void CRTTISerializer::DeserializeDefaultValues( IFile* file )
{
	Uint32 definitionIndex;

	*file << CCompressedNumSerializer( definitionIndex );

	RTTITypeDef* t = &m_allTypesDefinitions[ definitionIndex ];

	if ( t->m_type == RT_Class )
	{
		Uint32 num; 

		// it should be created by now
		CClass* classType = static_cast< CClass* >( m_allTypes[ definitionIndex ] );

		// Read info about default values
		*file << CCompressedNumSerializer( num );
		for ( Uint32 i = 0; i < num; ++i )
		{
			CDefaultValue* defVal = new CDefaultValue();
			SerializeDefaultValue( file, defVal, classType );
			ASSERT( defVal->GetProperty() );
			classType->AddDefaultValue( defVal );
		}
	}
}


Bool CRTTISerializer::DeserializeType( IFile* file )
{
	Uint32 typeNum;
	*file << CCompressedNumSerializer( typeNum );

	Uint32 definitionIndex;
	
	*file << CCompressedNumSerializer( definitionIndex );

	RTTITypeDef* t = &m_allTypesDefinitions[ definitionIndex ];

	if ( t->m_type == RT_Enum )
	{
		Uint32 size;
		*file << CCompressedNumSerializer( size );
		CEnum* enumType = SRTTI::GetInstance().CreateScriptedEnum( m_names[t->m_nameAsString].m_cName, size );

		Uint32 numOptions;
		*file << CCompressedNumSerializer( numOptions );

		for ( Uint32 i = 0; i < numOptions; ++i )
		{
			Uint32 name;
			*file << CCompressedNumSerializer( name );
			Int32 value;
			*file << CCompressedNumSerializer( value );
			enumType->Add( m_names[name].m_cName, value );
		}
	}


	if ( t->m_type == RT_Class )
	{

		// it should be created by now
		CClass* classType = static_cast< CClass* >( m_allTypes[ definitionIndex ] );


		// Num of base classes, 0 if struct
		Uint32 num;
		*file << CCompressedNumSerializer( num );

		if ( num > 0 )
		{
			Uint32 parentNum;
			*file << CCompressedNumSerializer( parentNum );
			if ( !t->m_isNative )
			{
				classType->AddParentClass( static_cast< CClass* >( m_allTypes[ parentNum ] ) );
			}
		}

		// Read and restore class flags
		const Uint32 flagMask = ~( (Uint32) CF_UndefinedFunctions );
		Uint32 flags;
		*file << flags;
		classType->m_flags &= ~classType->m_flags;
		classType->m_flags |= flags & flagMask;

		// Auto state name
		Uint32 autoStateName;
		*file << CCompressedNumSerializer( autoStateName );
		classType->m_autoStateName = m_names[autoStateName].m_cName;

		// Read info about num of states
		*file << CCompressedNumSerializer( num );

		// Read info about each state
		for( Uint32 i = 0; i < num; ++i )
		{
			Uint32 stateNum;
			Uint32 stateName;
			*file << CCompressedNumSerializer( stateName );
			*file << CCompressedNumSerializer( stateNum );

			CClass* stateClass = static_cast< CClass* >( m_allTypes[ stateNum ] );
			stateClass->m_flags |= CF_State;
			stateClass->m_stateName = m_names[stateName].m_cName;
			stateClass->m_stateMachineClass = classType;
			classType->m_stateClasses.PushBack( stateClass );
		}

		// Read info about scripted properties
		for ( Uint32 i=0; i< t->m_numScriptedProperties; ++i )
		{
			Uint32 propertyTypeNum;
			Uint32 name;
			Uint32 flags;
	
			*file << CCompressedNumSerializer( propertyTypeNum );
			*file << CCompressedNumSerializer( name );
			*file << CCompressedNumSerializer( flags );
			CProperty* prop = new CProperty( m_allTypes[ propertyTypeNum ], classType, 0, m_names[name].m_cName, TXT(""), flags );
			classType->m_localProperties.PushBack( prop );
			prop->AssignHash( classType->GetName() );
			SRTTI::GetInstance().RegisterProperty( prop );
			classType->m_arePropertiesCached = false;
			classType->m_areScriptPropertiesToDestroyCached = false;
			classType->m_areGCPropertiesCached = false;
		}

		// Gather info about exported properties
		for ( Uint32 i=0; i< t->m_numExportedProperties; i++ )
		{
			// property name
			Uint32 name;
			*file << CCompressedNumSerializer( name );
			CProperty* prop = classType->FindProperty( m_names[name].m_cName );

			if( prop == NULL )
			{
				return false;
			}

			prop->m_flags |= PF_Exported;
		}

		// Load binding information
		{
			classType->m_localAutoBinding.Clear();

			Uint32 count = 0;
			*file << CCompressedNumSerializer( count );

			for ( Uint32 i=0; i<count; ++i )
			{
				Uint32 propertyName;
				*file << CCompressedNumSerializer( propertyName );

				Uint32 bindingName;
				*file << CCompressedNumSerializer( bindingName );

				classType->AddPropertyBinding( m_names[propertyName].m_cName, m_names[bindingName].m_cName );
			}
		}
	}

	return true;
}

template< class T >
static void SerializeFunctionCode( T& rewriter, Uint32 codeSize )
{
	while ( rewriter.GetOffset() < codeSize )
	{
		// Rewrite opcode to stream
		Uint8 opcode = rewriter.RewriteByte();

		// Write opcode data 
		switch ( opcode )
		{
			// Breakpoint
			case OP_Breakpoint:
			{
				rewriter.RewriteInt();
				rewriter.RewriteByte();
				break;
			}

			// Byte constant
			case OP_ByteConst:
			{
				rewriter.RewriteByte();
				break;
			}   

			// Integer constant
			case OP_IntConst:
			{
				rewriter.RewriteInt();
				break;
			}

			// Short constant
			case OP_ShortConst:
			{
				rewriter.RewriteShort();
				break;
			}

			// Float constant
			case OP_FloatConst:
			{
				rewriter.RewriteFloat();
				break;
			}

			// String constant
			case OP_StringConst:
			{
				rewriter.RewriteString();
				break;
			}

			// Name constant
			case OP_NameConst:
			{
				rewriter.RewriteName();
				break;
			}

			// Switch
			case OP_Switch:
			{
				// instead of label
				rewriter.RewriteClassPtr();
				rewriter.RewriteWord();
				break;
			}

			// Switch label
			case OP_SwitchLabel:
			{
				// instead of label
				rewriter.RewriteWord();
				// instead of label
				rewriter.RewriteWord();
				break;
			}     

			// Default switch
			case OP_SwitchDefault:
			{
				break;
			}

			// Jumps
			case OP_Jump:
			case OP_Skip:   
			case OP_JumpIfFalse:
			{
				// instead of label
				rewriter.RewriteWord();
				break;   
			};   

			// Conditional expression
			case OP_Conditional:
			{
				// instead of label
				rewriter.RewriteWord();
				// instead of label
				rewriter.RewriteWord();
				break;
			}

			// Construct struct
			case OP_Constructor:
			{
				rewriter.RewriteByte();
				rewriter.RewriteClassPtr();
				break;
			};   

			// Function call
			case OP_FinalFunc:
			{
				// instead of label
				rewriter.RewriteWord();
				rewriter.RewriteWord();
				rewriter.RewriteFuncPtr();
				break;
			}

			// Entry function call
			case OP_EntryFunc:
			{
				// instead of label
				rewriter.RewriteWord();
				rewriter.RewriteName();
				break;
			}

			// Virtual function
			case OP_VirtualFunc:
			case OP_VirtualParentFunc:
			{
				// instead of label
				rewriter.RewriteWord();
				rewriter.RewriteWord();
				rewriter.RewriteName();
				break;
			}

			// Context
			case OP_Context:
			{
				// instead of label
				rewriter.RewriteWord();
				break;
			}

			// Properties
			case OP_LocalVar:
			{
				rewriter.RewritePropertyPtr( static_cast< EScriptOpcode >( opcode ) );
				break;
			}
			case OP_ParamVar:
			case OP_ObjectVar:
			case OP_ObjectBindableVar:
			case OP_DefaultVar:
			{
				rewriter.RewritePropertyPtr( static_cast< EScriptOpcode >( opcode ) );
				break;
			}

			// Dynamic casting
			case OP_DynamicCast:
			{
				rewriter.RewriteClassPtr();
				break;
			}

			// Generic equality/inequality check
			case OP_TestEqual:
			case OP_TestNotEqual:
			{
				rewriter.RewriteClassPtr();
				break;
			}

			// Access to structure member
			case OP_StructMember:
			{
				rewriter.RewritePropertyPtr( static_cast< EScriptOpcode >( opcode ) );
				break;
			}

			// Object creation
			case OP_New:
			{
				rewriter.RewriteClassPtr();
				break;
			}

			case OP_EnumToString:
			{
				rewriter.RewriteClassPtr();
				break;
			}

			case OP_EnumToInt:
			{
				rewriter.RewriteClassPtr();
				break;
			}

			case OP_IntToEnum:
			{
				rewriter.RewriteClassPtr();
				break;
			}

			// Array element
			case OP_ArrayElement:
			{
				rewriter.RewriteClassPtr();
				break;
			}

			// Array opcodes
			case OP_ArrayClear:
			case OP_ArraySize:
			case OP_ArrayResize:
			case OP_ArrayFindFirst:
			case OP_ArrayFindFirstFast:
			case OP_ArrayFindLast:
			case OP_ArrayFindLastFast:
			case OP_ArrayContains:
			case OP_ArrayContainsFast:
			case OP_ArrayPushBack:
			case OP_ArrayPopBack:
			case OP_ArrayInsert:
			case OP_ArrayRemove:
			case OP_ArrayRemoveFast:
			case OP_ArrayGrow:
			case OP_ArrayErase:
			case OP_ArrayEraseFast:
			case OP_ArrayLast:
			{
				rewriter.RewriteClassPtr();
				break;
			}

			// savepoints code
			case OP_SavePoint:
			{
				rewriter.RewriteWord(); // label
				rewriter.RewriteName();
				break;
			}

			case OP_SaveValue:
			{
				rewriter.RewriteName();
				break;
			}

			case OP_SavePointEnd:
			{
				break;
			}

			default:
			{
			}
		}; 
	}
}

void CRTTISerializer::SerializeFunction( IFile* file, CFunction* func )
{
	ASSERT( m_mappedFunctions.KeyExist( func ) );

	// Write index in our funcs map
	*file << CCompressedNumSerializer( *m_mappedFunctions.FindPtr( func ) );

	// Write flags
	*file << CCompressedNumSerializer( func->m_flags );

	Bool hasReturnProperty = ( func->m_returnProperty != NULL );
	*file << hasReturnProperty;

	// Write return value type 
	if ( hasReturnProperty )
	{
		ASSERT( m_mappedTypes.KeyExist( func->m_returnProperty->GetType() ) );
		*file << CCompressedNumSerializer( *m_mappedTypes.FindPtr( func->m_returnProperty->GetType() ) );
	}

	// Write parameter info
	Uint32 num = static_cast< Uint32 >( func->GetNumParameters() );
	*file << CCompressedNumSerializer( num );

	for ( Uint32 i = 0; i < num; ++i )
	{
		ASSERT( m_mappedTypes.KeyExist( func->GetParameter( i )->GetType() ) );
		*file << CCompressedNumSerializer( *m_mappedTypes.FindPtr( func->GetParameter( i )->GetType() ) );
		Uint32 parameterName = MapName( func->GetParameter( i )->GetName() );
		*file << CCompressedNumSerializer( parameterName );
		Uint32 paramFlags = func->GetParameter( i )->GetFlags();
		*file << CCompressedNumSerializer( paramFlags );
	}

	// Write local variables info
	num = func->m_localVars.Size();
	*file << CCompressedNumSerializer( num );

	for ( Uint32 i = 0; i < num; ++i )
	{
		ASSERT( m_mappedTypes.KeyExist( func->m_localVars[i]->GetType() ) );
		*file << CCompressedNumSerializer( *m_mappedTypes.FindPtr( func->m_localVars[i]->GetType() ) );
		Uint32 variableName = MapName( func->m_localVars[i]->GetName() );
		*file << CCompressedNumSerializer( variableName );
	}

	if( !func->IsNative() )
	{
		Uint32 codeSize = static_cast< Uint32 >( func->m_code.GetCodeEnd() - func->m_code.GetCode() );
		*file << CCompressedNumSerializer( codeSize );

		if ( codeSize > 0 )
		{
			CScriptCodeRewriterSaver rewriter( static_cast<Uint8*>( func->m_code.m_code.GetData() ), 0, file, this, func );
			SerializeFunctionCode( rewriter, codeSize );
		}
	}
}

Bool CRTTISerializer::DeserializeFunction( IFile* file )
{
	// Read index in our funcs map
	Uint32 index;
	*file << CCompressedNumSerializer( index );

	CFunction* func = m_allFunctions[index];

	if( func == NULL )
	{
		return false;
	}

	// Read flags
	Uint32 flags;
	*file << CCompressedNumSerializer( flags );
	func->m_flags = flags;

	// Has function defined return property?
	Bool hasReturnProperty;
	*file << hasReturnProperty;

	// Write return value type 
	if ( hasReturnProperty )
	{
		Uint32 propertyTypeIndex;
		*file << CCompressedNumSerializer( propertyTypeIndex );
		CProperty* prop = new CProperty( m_allTypes[propertyTypeIndex], NULL, 0, CNAME( __return ), TXT(""), PF_FuncRetValue );
		func->m_returnProperty = prop;
	}

	// Read parameter num info
	Uint32 num;
	*file << CCompressedNumSerializer( num );

	if( num > 0 )
	{
		func->m_parameters.Grow( num );
	}

	for ( Uint32 i = 0; i < num; ++i )
	{
		Uint32 paramTypeIndex;
		*file << CCompressedNumSerializer( paramTypeIndex );
		Uint32 paramName;
		*file << CCompressedNumSerializer( paramName );
		Uint32 paramFlags;
		*file << CCompressedNumSerializer( paramFlags );
		CProperty* prop = new CProperty( m_allTypes[paramTypeIndex], NULL, 0, m_names[paramName].m_cName, TXT(""), paramFlags );
		func->m_parameters[i] = prop;
	}

	// Read local variables info
	*file << CCompressedNumSerializer( num );

	func->m_localVars.Grow( num );

	for ( Uint32 i = 0; i < num; ++i )
	{
		Uint32 variableTypeIndex;
		*file << CCompressedNumSerializer( variableTypeIndex );
		Uint32 variableName;
		*file << CCompressedNumSerializer( variableName );
		CProperty* prop = new CProperty( m_allTypes[variableTypeIndex], NULL, 0, m_names[variableName].m_cName, TXT(""), PF_FuncLocal );
		func->m_localVars[i] = prop;
	}

	//Bind super function
	if( func->m_class && func->m_class->HasBaseClass() )
	{
		func->m_superFunction = func->m_class->GetBaseClass()->FindFunctionNonCached( func->GetName() );
	}

	if( !func->IsNative() )
	{
		Uint32 codeSize;
		*file << CCompressedNumSerializer( codeSize );

		if ( codeSize > 0 )
		{
			func->GetCode().m_code = DataBuffer( TDataBufferAllocator< MC_BufferScriptCode >::GetInstance(), codeSize, nullptr );

			CScriptCodeRewriterLoader rewriter( static_cast<Uint8*>( func->m_code.m_code.GetData() ), 0, file, this, func );
			SerializeFunctionCode( rewriter, codeSize );
		}
	}

	return true;
}

// shitty hack
Bool GRecomputeClassLayoutAfterRecompilation = true;

void CRTTISerializer::RecalculateDataLayout()
{
	CTimeCounter timer;

	PC_SCOPE( RecalculateDataLayout );

	// get all classes
	TDynArray< CClass* > allClasses;
	SRTTI::GetInstance().EnumClasses( NULL, allClasses, NULL, true );
	LOG_CORE( TXT("Found %d classes in RTTI"), allClasses.Size() );

	// rebuild internal class indexing
	SRTTI::GetInstance().ReindexClasses();

	// Recalculate the class size until it's stable
	Bool sizeChanging = true;
	while ( sizeChanging )
	{
		// Let's hope this is the last iteration
		sizeChanging = false;

		// Update our types
		for ( CClass* theClass : allClasses )
		{
			const Uint32 prevSize = theClass->GetSize();
			const Uint32 prevScriptSize = theClass->GetScriptDataSize();
			theClass->RecalculateClassDataSize();

			// is the size different ?
			if ( prevSize != theClass->GetSize() || prevScriptSize != theClass->GetScriptDataSize() )
			{
				sizeChanging = true;
			}
		}
	}

	// Calculate layout for functions
	for ( Uint32 i = 0; i < m_allFunctionsDefinitions.Size(); ++i )
	{
		if ( m_allFunctionsDefinitions[i].m_hasScriptData )
		{
			m_allFunctions[i]->CalcDataLayout();
		}
	}

	// Recompute the cached data in classes
	if ( GRecomputeClassLayoutAfterRecompilation )
	{
		// Collect all active scriptable objects
		TDynArray< THandle< IScriptable > > allScriptableObjects;
		IScriptable::CollectAllScriptableObjects( allScriptableObjects );

		// Since we got this far restore RTTI data
		SRTTI::GetInstance().RestoreScriptData( &allScriptableObjects );

		// Restore cached class properties
		SRTTI::GetInstance().RecacheClassProperties();
		LOG_CORE( TXT("RTTI data layout recalculated in %1.2fms"), timer.GetTimePeriodMS() );
	}

	// Collect post reload garbage
	GObjectGC->Collect();
}

Uint32 CRTTISerializer::MapName( CName name )
{
	Uint32 index = 0;
	if ( m_mappedNames.Find( name, index ) )
	{
		return index;
	}

	const Uint32 newIndex = m_names.Size();
	RTTISymbol* temp = new ( m_names ) RTTISymbol();
	temp->m_name = name.AsString();
	temp->m_cName = name;

	m_mappedNames.Set( name, newIndex );
	return newIndex;
}
