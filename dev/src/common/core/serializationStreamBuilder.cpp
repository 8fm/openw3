/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "rttiSystem.h"
#include "object.h"
#include "fileSkipableBlock.h"
#include "serializationStreamData.h"
#include "serializationStreamOpcodes.h"
#include "serializationStreamBuilder.h"
#include "bitField.h"
#include "enum.h"

namespace Helper
{
	// TODO: move to RTTI
	class InlineStructuresCache
	{
	public:
		InlineStructuresCache()
		{
			BuildCache();
		}

		static InlineStructuresCache& GetInstance()
		{
			static InlineStructuresCache theCache;
			return theCache;
		}

		const Bool IsInlined( const IRTTIType* type ) const
		{
			if ( type->GetType() == RT_Class )
			{
				const CClass* structType = static_cast< const CClass* >( type );
				if ( !structType->IsScripted() )
				{
					return m_inlinedStructures.Exist( structType );
				}
			}

			return false;
		}

	private:
		THashSet< const CClass* >	m_inlinedStructures;
		THashSet< const CClass* >	m_notInlinedStructures;

		bool IsTypeInlinable( const CClass* info )
		{
			if ( m_inlinedStructures.Exist( info ) )
				return true;

			if ( m_notInlinedStructures.Exist( info ) )
				return false;

			if ( info->IsScripted() )
				return false;

			if ( info->IsSerializable() )
				return false;

			if ( info->IsAbstract() )
				return false;

			// we should have some properties
			const auto& props = info->GetCachedProperties();
			if ( props.Empty() )
				return false;

			// we must have fundamental properties only
			Bool hasOnlyFundamentalProps = true;
			Uint32 propSizeSum = 0;
			for ( const CProperty* prop : props )
			{
				if ( prop->IsScripted() )
				{
					hasOnlyFundamentalProps = false;
					break;
				}

				propSizeSum += prop->GetType()->GetSize();

				if ( prop->GetType()->GetType() == RT_Fundamental )
				{
					continue;
				}
				else if ( prop->GetType()->GetType() == RT_Class )
				{
					const CClass* propClass = static_cast< const CClass* >( prop->GetType() );
					if ( IsTypeInlinable( propClass ) )
					{
						continue;
					}
				}

				// not a property we can store inplace
				hasOnlyFundamentalProps = false;
				break;
			}

			// add to list
			if ( hasOnlyFundamentalProps && (propSizeSum == info->GetSize()) )
			{
				LOG_CORE( TXT("Structure '%ls' will be inlined (%d props, %d bytes)"), 
					info->GetName().AsChar(), props.Size(), info->GetSize() );

				m_inlinedStructures.Insert(info);
				return true;
			}
			else
			{
				m_notInlinedStructures.Insert(info);
				return false;
			}
		}

		void BuildCache()
		{
			const auto& classes = SRTTI::GetInstance().GetIndexedClasses();

			for ( Uint32 i=0; i<classes.Size(); ++i )
			{
				const CClass* info = classes[i];
				if ( !info )
					continue;

				IsTypeInlinable( info );
			}
		}
	};
} // Helper

CSerializationStreamBuilder::CSerializationStreamBuilder()
{
	// cache types from RTTI
	m_cachedTypeBool = GetTypeObject<Bool>();
	m_cachedTypeString = GetTypeObject<String>();
	m_cachedTypeName = GetTypeObject<CName>();
}

void CSerializationStreamBuilder::WriteStream( IFile& file, const CClass* objectClass, const void* data, const void* base )
{
	TDynArray< const CProperty* > nativePropsToSave;
	TDynArray< const CProperty* > scriptedPropsToSave;
	nativePropsToSave.Reserve( objectClass->GetCachedProperties().Size() );
	scriptedPropsToSave.Reserve( objectClass->GetCachedProperties().Size() );

	// get the properties to save
	// make sure we use the same logic as in normal save
	for ( const CProperty* propToSave : objectClass->GetCachedProperties() )
	{
		// Skip properties that should not be saved
		if ( !propToSave->IsSerializable() )
		{
			continue;
		}

		// When cooking skip properties that are not for cooked builds
		if ( file.IsCooker() && !propToSave->IsSerializableInCookedBuilds() )
		{
			continue;
		}

		// Do not save scripted properties that are not editable (internal scripted properties)
		if ( file.IsFilteringScriptedProperties() )
		{
			const Bool isScriptedStruct = (objectClass->IsScripted() && !objectClass->HasBaseClass());
			const Bool isScriptedProp = propToSave->IsScripted() || isScriptedStruct;
			if ( isScriptedProp && !propToSave->IsEditable() && !propToSave->IsSaved() )
			{
				continue;
			}
		}

		// Skip property if it has the same value as in the default object
		if ( base )
		{
			const void* srcData1 = propToSave->GetOffsetPtr( data );
			const void* srcData2 = propToSave->GetOffsetPtr( base );
			if ( srcData1 && srcData2 )
			{
				if ( propToSave->GetType()->Compare( srcData1, srcData2, 0 ) )
				{
					continue;
				}
			}
		}

		// Ask object if we should serialize this property
		if ( !file.IsGarbageCollector() && objectClass->IsSerializable() )
		{
			const ISerializable* serializable = static_cast< const ISerializable* >( data );
			if ( !serializable->OnPropertyCanSave( file, propToSave->GetName(), (CProperty*)propToSave ) )
			{
				continue;
			}
		}

		// Property should be saved
		if ( propToSave->IsScripted() )
		{
			scriptedPropsToSave.PushBack(propToSave);
		}
		else
		{
			nativePropsToSave.PushBack(propToSave);
		}
	}

	// sort properties by offset in the class - most likely we will be reading in the same order
	::Sort( nativePropsToSave.Begin(), nativePropsToSave.End(), []( const CProperty* propA, const CProperty*& propB ) { return propA->GetDataOffset() < propB->GetDataOffset(); } );
	::Sort( scriptedPropsToSave.Begin(), scriptedPropsToSave.End(), []( const CProperty* propA, const CProperty*& propB ) { return propA->GetDataOffset() < propB->GetDataOffset(); } );

	// start new stream block
	WriteUint8( file, eSerializationOpcode_Start );

	// write native properties
	for ( const CProperty* propToSave : nativePropsToSave )
	{
		const void* propData = propToSave->GetOffsetPtr( data );
		const void* baseData = base ? propToSave->GetOffsetPtr( base ) : nullptr;
		WritePropertyData( file, propToSave, propData, baseData );
	}

	// if we have scripted properties we will deserialize them into different scope
	if ( !scriptedPropsToSave.Empty() )
	{
		// enter scripted scope
		WriteUint8( file, eSerializationOpcode_PushScript );

		// write scripted properties
		for ( const CProperty* propToSave : scriptedPropsToSave )
		{
			const void* propData = propToSave->GetOffsetPtr( data );
			const void* baseData = base ? propToSave->GetOffsetPtr( base ) : nullptr;
			WritePropertyData( file, propToSave, propData, baseData );
		}

		// leave scripted scope
		WriteUint8( file, eSerializationOpcode_Pop );
	}

	// finish with the end of stream marker
	WriteUint8( file, eSerializationOpcode_End );
}

void CSerializationStreamBuilder::WritePropertyData( IFile& file, const CProperty* prop, const void* propData, const void* baseData )
{
	// push the property scope
	WriteUint8( file, eSerializationOpcode_PushProp );
	WriteProperty( file, prop );

	// save property data
	WriteTypeData( file, prop->GetType(), propData, baseData );

	// end of the property scope
	WriteUint8( file, eSerializationOpcode_Pop );
}

void CSerializationStreamBuilder::WriteArrayData( IFile& file, const IRTTIType* type, const void* propData, const void* baseData )
{
	const IRTTIBaseArrayType* arrayType = static_cast< const IRTTIBaseArrayType* >( type );

	const Uint32 elemCount = arrayType->ArrayGetArraySize( propData );
	const Uint32 baseCount = baseData ? arrayType->ArrayGetArraySize( baseData ) : 0;

	// save the array elements
	for ( Uint32 i=0; i<elemCount; ++i )
	{
		// advance to next element (stride was setup by ResizeArray call)
		if ( i > 0 )
		{
			WriteUint8( file, eSerializationOpcode_Advance );
		}

		// write array element
		const IRTTIType* arrayElementType = arrayType->ArrayGetInnerType();
		const void* arrayElementData = arrayType->ArrayGetArrayElement(propData, i);
		const void* arrayElementBase = (i < baseCount) ? arrayType->ArrayGetArrayElement(baseData, i) : nullptr;
		WriteTypeData( file, arrayElementType, arrayElementData, arrayElementBase );
	}
}

void CSerializationStreamBuilder::WriteTypeData( IFile& file, const IRTTIType* type, const void* propData, const void* baseData )
{
	// boolean
	if ( type == m_cachedTypeBool )
	{
		const Bool& val = *(const Bool*)propData;
		WriteUint8( file, val ? eSerializationOpcode_DataTrue : eSerializationOpcode_DataFalse );
	}

	// fundamental types
	else if ( type->GetType() == RT_Fundamental )
	{
		// save the data directly
		const Uint8 size = type->GetSize();
		if ( size == 1 )
		{
			const Uint8& val = *(const Uint8*) propData;
			if ( val == 0 )
			{
				WriteUint8( file, eSerializationOpcode_Zero8 );
			}
			else
			{
				WriteUint8( file, eSerializationOpcode_Data8 );
				WriteUint8( file, val );
			}
		}
		else if ( size == 2 )
		{
			const Uint16& val = *(const Uint16*) propData;
			if ( val == 0 )
			{
				WriteUint8( file, eSerializationOpcode_Zero16 );
			}
			else
			{
				WriteUint8( file, eSerializationOpcode_Data16 );
				WriteUint16( file, val );
			}
		}
		else if ( size == 4 )
		{
			const Uint32& val = *(const Uint32*) propData;
			if ( val == 0 )
			{
				WriteUint8( file, eSerializationOpcode_Zero32 );
			}
			else
			{
				WriteUint8( file, eSerializationOpcode_Data32 );
				WriteUint32( file, val );
			}
		}
		else if ( size == 8 )
		{
			const Uint64& val = *(const Uint64*) propData;
			if ( val == 0 )
			{
				WriteUint8( file, eSerializationOpcode_Zero64 );
			}
			else
			{
				WriteUint8( file, eSerializationOpcode_Data64 );
				WriteUint64( file, val );
			}
		}
		else
		{
			RED_FATAL( "Unsupported size of inplace data for type %ls (size=%d)", type->GetName().AsChar(), size );
		}
	}

	// string type
	else if ( type == m_cachedTypeString )
	{
		const String& val = *(const String*) propData;
		const Uint32 length = val.GetLength();
		const Bool isLongString = (length >= 255);

		// check if string is an ANSI string - they are stored more efficiently
		Bool noUnicodeChars = true;
		const Char* txt = val.AsChar();
		for ( Uint32 i=0; i<length; ++i )
		{
			if ( txt[i] > (Char)255 )
			{
				noUnicodeChars = false;
				break;
			}
		}

		// store opcode depending on the string type, then store the string
		if ( length == 0 )
		{
			WriteUint8( file, eSerializationOpcode_StringEmpty );
		}
		else if ( noUnicodeChars )
		{
			const StringAnsi tempString( UNICODE_TO_ANSI( txt ) );
			if ( isLongString )
			{
				WriteUint8( file, eSerializationOpcode_StringAnsi32 );
				WriteUint32( file, length );
			}
			else
			{
				WriteUint8( file, eSerializationOpcode_StringAnsi );
				WriteUint8( file, (Uint8)length );
			}
			WriteBuf( file, tempString.AsChar(), (1+length) * sizeof(AnsiChar) );
		}
		else
		{
			if ( isLongString )
			{
				WriteUint8( file, eSerializationOpcode_StringUnicode32 );
				WriteUint32( file, length );
			}
			else
			{
				WriteUint8( file, eSerializationOpcode_StringUnicode );
				WriteUint8( file, (Uint8)length );
			}
			WriteBuf( file, txt, (1+length) * sizeof(Char) );
		}
	}

	// name
	else if ( type == m_cachedTypeName )
	{
		const CName& val = *(const CName*) propData;
		WriteUint8( file, eSerializationOpcode_Name );
		WriteName( file, val );
	}

	// structures
	else if ( type->GetType() == RT_Class )
	{
		// inlined structure ?
		const Bool isInlined = Helper::InlineStructuresCache::GetInstance().IsInlined( type );
		if ( isInlined )
		{
			// we save the inlined data
			WriteUint8( file, eSerializationOpcode_InlinedStruct );

			// save the type information
			const CClass* classToSerialize = static_cast< const CClass* >( type );
			WriteType( file, type );
			WriteUint16( file, (Uint16)classToSerialize->GetSize() );

			// save the data directly
			WriteBuf( file, propData, classToSerialize->GetSize() );
		}
		else
		{
			// push current context (all offsets will be relative)
			WriteUint8( file, eSerializationOpcode_PushCurrent );

			// create the serialization stream for the inner class
			const CClass* classToSerialize = static_cast< const CClass* >( type );
			WriteStream( file, classToSerialize, propData, baseData );

			// pop back the context
			WriteUint8( file, eSerializationOpcode_Pop );
		}
	}

	// native array - constant size always
	else if ( type->GetType() == RT_NativeArray )
	{
		const CRTTINativeArrayType* arrayType = static_cast< const CRTTINativeArrayType* >( type );
		const Uint32 elemCount = arrayType->GetArraySize( propData );
		const Bool isBigArray = (elemCount > 255);

		// write the array header and the number of elements
		if ( isBigArray )
		{
			WriteUint8( file, eSerializationOpcode_PushNativeArray32 );
			WriteType( file, type );	
			WriteUint32( file, elemCount );
		}
		else
		{
			WriteUint8( file, eSerializationOpcode_PushNativeArray8 );
			WriteType( file, type );	
			WriteUint8( file, (Uint8)elemCount );
		}

		// store the shit only if not zero
		if ( elemCount > 0 )
		{
			// write the data
			WriteArrayData( file, arrayType, propData, baseData );

			// pop the array back
			WriteUint8( file, eSerializationOpcode_Pop );
		}
	}

	// static array
	else if ( type->GetType() == RT_StaticArray )
	{
		const CRTTIStaticArrayType* arrayType = static_cast< const CRTTIStaticArrayType* >( type );
		const Uint32 elemCount = arrayType->ArrayGetArraySize( propData );
		const Bool isBigArray = (elemCount > 255);

		// write the array header and the number of elements
		if ( isBigArray )
		{
			WriteUint8( file, eSerializationOpcode_PushStaticArray32 );
			WriteType( file, type );
			WriteUint32( file, elemCount );
		}
		else
		{
			WriteUint8( file, eSerializationOpcode_PushStaticArray8 );
			WriteType( file, type );
			WriteUint8( file, (Uint32)elemCount );
		}

		// store the shit only if not zero
		if ( elemCount > 0 )
		{
			// write the data
			WriteArrayData( file, arrayType, propData, baseData );

			// pop the array back
			WriteUint8( file, eSerializationOpcode_Pop );
		}
	}

	// dynamic array
	else if ( type->GetType() == RT_Array )
	{
		// in native array number of elements is always linked to the type
		const CRTTIArrayType* arrayType = static_cast< const CRTTIArrayType* >( type );
		const Uint32 elemCount = arrayType->GetArraySize( propData );

		// fundamental array ?
		if ( arrayType->GetInnerType()->GetType() == RT_Fundamental && (elemCount > 2) )
		{
			const CBaseArray* srcData = (const CBaseArray*) propData;
			const Uint32 elemSize = arrayType->GetInnerType()->GetSize();

			WriteUint8( file, eSerializationOpcode_RawBlock );
			WriteType( file, arrayType );
			WriteUint32( file, elemCount );
			WriteUint8( file, elemSize );
			WriteBuf( file, srcData->Data(), elemCount * elemSize );
		}
		else
		{
			const Bool isBigArray = (elemCount > 255);

			// write the array header and the number of elements
			if ( isBigArray )
			{
				WriteUint8( file, eSerializationOpcode_PushDynArray32 );
				WriteType( file, type );
				WriteUint32( file, elemCount );
			}
			else
			{
				WriteUint8( file, eSerializationOpcode_PushDynArray8 );
				WriteType( file, type );
				WriteUint8( file, elemCount );
			}

			// store the shit only if not zero
			if ( elemCount > 0 )
			{
				// write the data
				WriteArrayData( file, arrayType, propData, baseData );

				// pop the array back
				WriteUint8( file, eSerializationOpcode_Pop );
			}
		}
	}

	// pointers
	else if ( type->GetType() == RT_Pointer )
	{
		WriteUint8( file, eSerializationOpcode_Pointer );
		WriteType( file, type );

		type->Serialize( file, (void*)propData );
	}
	else if ( type->GetType() == RT_Handle )
	{
		WriteUint8( file, eSerializationOpcode_Handle );
		WriteType( file, type );

		type->Serialize( file, (void*)propData );
	}
	else if ( type->GetType() == RT_SoftHandle )
	{
		WriteUint8( file, eSerializationOpcode_SoftHandle );
		WriteType( file, type );

		type->Serialize( file, (void*)propData );
	}

	// enum
	else if ( type->GetType() == RT_Enum )
	{
		const CEnum* enumType = static_cast< const CEnum* >( type );

		const Uint32 size = type->GetSize();
		if ( size == 1 )
		{
			// find the name of the enum option selected and store it
			CName enumValueName;
			const Uint8 enumValue = *(const Uint8*)( propData );
			enumType->FindName( enumValue, enumValueName );

			// save the value and the name
			WriteUint8( file, eSerializationOpcode_Enum1 );
			WriteUint8( file, enumValue );
			WriteName( file, enumValueName );
		}
		else if ( size == 2 )
		{
			// find the name of the enum option selected and store it
			CName enumValueName;
			const Uint16 enumValue = *(const Uint16*)( propData );
			enumType->FindName( enumValue, enumValueName );

			// save the value and the name
			WriteUint8( file, eSerializationOpcode_Enum2 );
			WriteUint16( file, enumValue );
			WriteName( file, enumValueName );
		}
		else if ( size == 4 )
		{
			// find the name of the enum option selected and store it
			CName enumValueName;
			const Uint32 enumValue = *(const Uint32*)( propData );
			enumType->FindName( enumValue, enumValueName );

			// save the value and the name
			WriteUint8( file, eSerializationOpcode_Enum4 );
			WriteUint32( file, enumValue );
			WriteName( file, enumValueName );
		}
		else
		{
			RED_FATAL( "Enum %ls has unsupported size %d", enumType->GetName().AsChar(), enumType->GetSize() );
		}
	}

	// bitfields
	else if ( type->GetType() == RT_BitField )
	{
		const CBitField* bitType = static_cast< const CBitField* >( type );

		const Uint32 size = type->GetSize();
		if ( size == 1 )
		{
			const Uint8 bitValue = *(const Uint8*)( propData );

			// save the value and the name
			WriteUint8( file, eSerializationOpcode_BitField1 );
			WriteUint8( file, bitValue );
			WriteName( file, bitType->GetName() );
		}
		else if ( size == 2 )
		{
			const Uint16 bitValue = *(const Uint16*)( propData );

			// save the value and the name
			WriteUint8( file, eSerializationOpcode_BitField2 );
			WriteUint16( file, bitValue );
			WriteName( file, bitType->GetName() );
		}
		else if ( size == 4 )
		{
			const Uint32 bitValue = *(const Uint32*)( propData );

			// save the value and the name
			WriteUint8( file, eSerializationOpcode_BitField4 );
			WriteUint32( file, bitValue );
			WriteName( file, bitType->GetName() );
		}
		else
		{
			RED_FATAL( "Bitfield %ls has unsupported size %d", bitType->GetName().AsChar(), bitType->GetSize() );
		}
	}

	// generic type
	else
	{
		WriteUint8( file, eSerializationOpcode_Generic );
		WriteType( file, type );

		// generic type data is skipable
		{
			CFileSkipableBlock skipBlock( file );
			type->Serialize( file, (void*)propData );
		}
	}
}

void CSerializationStreamBuilder::WriteUint8( IFile& file, const Uint8 value )
{
	file << (Uint8&)value;
}

void CSerializationStreamBuilder::WriteUint16( IFile& file, const Uint16 value )
{
	file << (Uint16&)value;
}

void CSerializationStreamBuilder::WriteUint32( IFile& file, const Uint32 value )
{
	file << (Uint32&)value;
}

void CSerializationStreamBuilder::WriteUint64( IFile& file, const Uint64 value )
{
	file << (Uint64&)value;
}

void CSerializationStreamBuilder::WriteProperty( IFile& file, const CProperty* prop )
{
	const CProperty* tempProp = (const CProperty*) prop;
	file << tempProp;
}

void CSerializationStreamBuilder::WriteType( IFile& file, const IRTTIType* type )
{
	IRTTIType* temp = (IRTTIType*) type;
	file << temp;
}

void CSerializationStreamBuilder::WriteName( IFile& file, const CName name )
{
	CName temp = name;
	file << temp;
}

void CSerializationStreamBuilder::WriteBuf( IFile& file, const void* data, const Uint32 length )
{
	file.Serialize( (void*)data, length );
}

void CSerializationStream::Build( IFile& writer, const CClass* objectClass, const void* data, const void* baseData )
{
	PC_SCOPE(SerializationStreamBuild);

	CSerializationStreamBuilder builder;
	builder.WriteStream( writer, objectClass, data, baseData );
}
