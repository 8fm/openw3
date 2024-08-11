/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __RTTI_SERIALIZER_H__
#define __RTTI_SERIALIZER_H__

#include "uniPointer.h"
#include "rttiSystem.h"
#include "scriptCompiledCode.h"
#include "hashmap.h"
#include "sortedmap.h"
#include "rttiType.h"
#include "class.h"
#include "functionBuilder.h"
#include "datetime.h"

class CScriptCodeRewriter;
class CDefaultValue;

class CRTTISerializer
{
public:
	CRTTISerializer();
	~CRTTISerializer();

	enum EHeaderValidationMask
	{
		eHeaderValidateNone				= 0,
		eHeaderValidateScriptVersion	= FLAG(0),
		eHeaderValidatePlatform			= FLAG(1),
		eHeaderValidateAppVersion		= FLAG(2),
		eHeaderValidateTimestamp		= FLAG(3),
		eHeaderValidateConfig			= FLAG(4),

		eHeaderValidateAll				= eHeaderValidateScriptVersion|
										  eHeaderValidatePlatform|
										  eHeaderValidateAppVersion|
										  eHeaderValidateTimestamp|
										  eHeaderValidateConfig,
	};

	// Get default file name for compiled scripts
	static const String GetDefaultFileName();

	// Get platform related file path for current settings (internal)
	static const String GetPlatformFileName();

	// Saves current RTTI script data to file
	// Writes the script data header that matches current settings
	Bool SaveScriptData( const String& absoluteFilePath, Uint64 crc = 0);

	// Loads RTTI script data from file
	// If the validateHeader option is set the loading will fail if it's not matching the default header
	Bool LoadScriptData( const String& depotFilePath, Uint64 expectedCrc, Uint32 headerValidationMask = eHeaderValidateAll );
	
private:
	// Serializes m_allTypesDefinitions and m_allFunctionsDefinitions
	void SerializeIndexMaps( IFile* file );

	// Serializes type to file
	void SerializeType( IFile* file, IRTTIType* type );

	// Serializes type to file
	void SerializeDefaultValues( IFile* file, IRTTIType* type );

	// Deserializes type from file
	Bool DeserializeType( IFile* file );

	// Deserializes type from file
	void DeserializeDefaultValues( IFile* file );

	// Serializes function to file
	void SerializeFunction( IFile* file, CFunction* func );

	// Deserializes function from file
	Bool DeserializeFunction( IFile* file );

	// Serializes type's default value
	void SerializeDefaultValue( IFile* file, CDefaultValue* val, CClass* classType );

	// Recalculates new data layout in classes etc.
	void RecalculateDataLayout();

public:
	struct RTTIHeader
	{
		CDateTime	m_timestamp;
		String		m_platform;
		String		m_appVersion;
		String		m_configuration;
		Uint64		m_crc;
		Uint32		m_scriptVersion;

		RTTIHeader(); // empty

		// Make default header for current platform
		static const RTTIHeader& GetPlatformDefault(); 

		// Get best filename for give configuration
		const String GetFileNamePrefix() const;

		// Serialize the header
		void Serialize( IFile& file );

	

		// Validate that scripts can be loaded from a file with this header
		bool ValidateLoading( const RTTIHeader& header, String& outReason, Uint32 validateHeaderMask ) const;
	};
private:

	struct RTTITypeDef
	{
		RTTITypeDef()
			:	m_nameAsString( 0 ),
			m_type( RT_Simple ),
			m_hasScriptData( false ),
			m_isNative( false ),
			m_isScripted( false ),
			m_numScriptedProperties( 0 ),
			m_numExportedProperties( 0 )
		{}

		Uint32			m_nameAsString;
		ERTTITypeType	m_type;
		Bool			m_hasScriptData;
		Bool			m_isNative;
		Bool			m_isScripted;
		Uint32			m_numScriptedProperties;
		Uint32			m_numExportedProperties;
	};

	struct RTTIFuncDef
	{
		RTTIFuncDef()
			:	m_nameAsString( 0 ),
			m_isNative( false ),
			m_isGlobal( false ),
			m_baseClassType( 0 ),
			m_hasScriptData( false )
		{}

		Uint32			m_nameAsString;
		Bool			m_isNative;
		Bool			m_isGlobal;
		Uint32			m_baseClassType;
		Bool			m_hasScriptData;
	};

	struct RTTISymbol
	{
		String			m_name;
		CName			m_cName;
	};

	TDynArray< RTTITypeDef >		m_allTypesDefinitions;
	TDynArray< IRTTIType* >			m_allTypes;
	THashMap< IRTTIType*, Uint32 >	m_mappedTypes;

	TDynArray< RTTIFuncDef >		m_allFunctionsDefinitions;
	TDynArray< CFunction* >			m_allFunctions;
	THashMap< CFunction*, Uint32 >	m_mappedFunctions;

	TDynArray< RTTISymbol >			m_names;
	THashMap< CName, Uint32 >		m_mappedNames;

	// Maps function to RTTIFuncDef and m_allFunctionsDefinitions
	Uint32 MapFunction( CFunction* f, Uint32 baseClassTypeIndex, Bool isGlobal );

	// Maps type to RTTITypeDef and m_allTypesDefinitions
	Uint32 MapType( IRTTIType* type, Bool hasScriptData );

	// Map name to runtime index
	Uint32 MapName( CName name );

	// Internal save
	Bool SaveScriptDataToFile( IFile* file );

	// Internal load
	Bool LoadScriptDataFromFile( IFile* file );

	friend class CScriptCodeRewriter;
	friend class CScriptCodeRewriterSaver;
	friend class CScriptCodeRewriterLoader;
	friend class CDumpScriptsCommandlet;
};

class CScriptCodeRewriter
{
protected:
	Uint32 m_offset;
	Uint8* m_buffer;
	IFile* m_file;
	CRTTISerializer* m_parent;
	CFunction* m_function;

public:
	CScriptCodeRewriter( Uint8* buffer, Uint32 offset, IFile* file, CRTTISerializer* parent, CFunction* func )
		: m_offset( offset )
		, m_buffer( buffer )
		, m_file( file )
		, m_parent( parent )
		, m_function( func )
	{}

	Uint32 GetOffset() const 
	{
		return m_offset;
	}

	virtual void RewriteInt() = 0;
	virtual Uint8 RewriteByte() = 0;
	virtual void RewriteShort() = 0; 
	virtual void RewriteWord() = 0;
	virtual void RewriteFloat() = 0;
	virtual void RewriteString() = 0;
	virtual void RewriteName() = 0;
	virtual void RewriteClassPtr() = 0;
	virtual void RewriteFuncPtr() = 0;
	virtual void RewritePropertyPtr( EScriptOpcode op ) = 0;

};

class CScriptCodeRewriterSaver : public CScriptCodeRewriter
{
public:
	CScriptCodeRewriterSaver( Uint8* buffer, Uint32 offset, IFile* file, CRTTISerializer* parent, CFunction* func  )
		: CScriptCodeRewriter( buffer, offset, file, parent, func )
	{}

	template< typename T >
	RED_INLINE void ReadStream( T& val )
	{	
		Red::System::MemoryCopy( &val, &m_buffer[m_offset], sizeof( val ) );
		m_offset += sizeof( val );
	}

	Uint8 RewriteByte()
	{
		Uint8 val;
		ReadStream( val );
		*m_file << val;
		return val;
	}

	void RewriteInt()
	{
		Int32 val;
		ReadStream( val );
		*m_file << CCompressedNumSerializer( val );
	}

	void RewriteShort()
	{
		Int16 val;
		ReadStream( val );
		*m_file << val;
	}

	void RewriteWord()
	{
		Uint16 val;
		ReadStream( val );
		*m_file << val;
	}

	void RewriteFloat()
	{
		Float val;
		ReadStream( val );
		*m_file << val;
	}

	void RewriteString()	
	{		
		// save the length of the string directly
		Uint32 length;
		ReadStream( length );
		*m_file << CCompressedNumSerializer( length );

		// save chars
		for ( Uint32 i=0; i<length; ++i )
		{
			Uint16 chr;
			ReadStream( chr );
			*m_file << chr;
		}
	}

	void RewriteName()
	{
		CName* val = (CName*) &m_buffer[m_offset];
		m_offset += sizeof( CName );

		Uint32 nameIndex = m_parent->MapName( *val );
		*m_file << CCompressedNumSerializer( nameIndex );
	}

	void RewriteClassPtr()
	{
		TUniPointer<CClass> c;
		ReadStream( c ); // 64-bit compatible

		// save the index
		Uint32* index = m_parent->m_mappedTypes.FindPtr( c.Get() );
		ASSERT( index != NULL );
		*m_file << CCompressedNumSerializer( *index );
	}

	void RewriteFuncPtr()
	{
		TUniPointer<CFunction> f;
		ReadStream( f );

		const Uint32* functionIndex = m_parent->m_mappedFunctions.FindPtr( f.Get() );
		if ( functionIndex != NULL )
		{
			// save the index to valid functions as positive numbers
			Int32 saveValue = (Int32)( (*functionIndex) + 1 );
			*m_file << CCompressedNumSerializer( saveValue );
		}
		else
		{
			// is this an operator ?
			const TDynArray< CScriptOperator* >& operators = CScriptOperator::GetOperators();
			CScriptOperator* oper = static_cast<CScriptOperator*>( f.Get() );
			Int32 index = (Int32)operators.GetIndex( oper );
			if ( index != -1 )
			{
				// save the opereator index as negaive value
				Int32 saveValue = -( index + 1 );
				*m_file << CCompressedNumSerializer( saveValue );
			}
		}

	}

	void RewritePropertyPtr( EScriptOpcode )
	{
		TUniPointer<CProperty> c;
		ReadStream( c );

		const CName name = c->GetName();
		Uint32 index = m_parent->MapName( name );
		*m_file << CCompressedNumSerializer( index );

		const Uint32* parentTypeIndex = m_parent->m_mappedTypes.FindPtr( c->GetParent() );
		if ( parentTypeIndex != NULL )
		{
			Int32 saveValue = *parentTypeIndex;
			*m_file << CCompressedNumSerializer( saveValue );
		}
		else
		{
			// no parent type
			Int32 saveValue = -1;
			*m_file << CCompressedNumSerializer( saveValue );
		}		
	}


};

class CScriptCodeRewriterLoader : public CScriptCodeRewriter
{
public:
	CScriptCodeRewriterLoader( Uint8* buffer, Uint32 offset, IFile* file, CRTTISerializer* parent, CFunction* func  )
		: CScriptCodeRewriter( buffer, offset, file, parent, func )
	{}

	template< typename T >
	RED_INLINE void WriteStream( const T& val )
	{
		Red::System::MemoryCopy( &m_buffer[m_offset], &val, sizeof( val ) );
		m_offset += sizeof( val );
	}

	Uint8 RewriteByte()
	{
		Uint8 val;
		*m_file << val;
		WriteStream( val );
		return val;
	}

	void RewriteInt()
	{
		Int32 val;
		*m_file << CCompressedNumSerializer( val );
		WriteStream( val );
	}

	void RewriteShort()
	{
		Int16 val;
		*m_file << val;
		WriteStream( val );
	}

	void RewriteWord()
	{
		Uint16 val;
		*m_file << val;
		WriteStream( val );
	}

	void RewriteFloat()
	{
		Float val;
		*m_file << val;
		WriteStream( val );
	}

	void RewriteString()
	{
		// load length of the string directly
		Uint32 length;
		*m_file << CCompressedNumSerializer( length );
		WriteStream( length );

		// load string chars
		for ( Uint32 i=0; i<length; ++i )
		{
			Uint16 chr;
			*m_file << chr;
			WriteStream( chr );
		}
	}

	void RewriteName()
	{
		Uint32 index;
		*m_file << CCompressedNumSerializer( index );
		CName val( m_parent->m_names[index].m_cName );
		WriteStream( val );
	}

	void RewriteClassPtr()
	{
		Uint32 classIndex;
		*m_file << CCompressedNumSerializer( classIndex );

		TUniPointer< CClass > val;
		ASSERT( classIndex < m_parent->m_allTypes.Size() );
		if ( classIndex < m_parent->m_allTypes.Size() )
		{			
			val = static_cast< CClass* >( m_parent->m_allTypes[classIndex] );
		}
		WriteStream( val );
	}

	void RewriteFuncPtr()
	{
		Int32 funcIndex;
		*m_file << CCompressedNumSerializer( funcIndex );

		TUniPointer< CFunction > func;
		if ( funcIndex > 0 )
		{
			// positive indices are for functions
			CFunction* funcPtr = m_parent->m_allFunctions[ funcIndex - 1 ];
			func = funcPtr;
		}
		else if ( funcIndex < 0 )
		{
			// negative indices are for operators
			const TDynArray< CScriptOperator* >& operators = CScriptOperator::GetOperators();
			func = operators[ -( funcIndex + 1 ) ];
		}

		if ( func.Get() == NULL )
		{
			HALT( "No function at index: %d", funcIndex );
		}

		WriteStream( func );
	}

	void RewritePropertyPtr( EScriptOpcode op )
	{
		Uint32 index, num;
		*m_file << CCompressedNumSerializer( index );
		*m_file << CCompressedNumSerializer( num );

		CName cN( m_parent->m_names[index].m_cName );

		TUniPointer<CProperty> c( NULL );

		switch ( op )
		{
			case OP_LocalVar:
			{
				for ( Uint32 i = 0; i < m_function->m_localVars.Size(); ++i )
				{
					if ( m_function->m_localVars[i]->GetName() == cN )
					{
						c = m_function->m_localVars[i];
						break;
					}
				}
				break;
			}

			case OP_ParamVar:
			{
				c = m_function->FindProperty( cN );
				break;
			}

			case OP_ObjectVar:
			case OP_StructMember:
			case OP_DefaultVar:
			case OP_ObjectBindableVar:
			{
				CClass* parentType = static_cast< CClass* >( m_parent->m_allTypes[num] );
				c = parentType->FindProperty( cN );
				break;
			}

			default:
			{
				RED_WARNING( 0, "Unkown property" );
			}
		}

		WriteStream( c );
	}
};

#endif // __RTTI_SERIALIZER_H__
