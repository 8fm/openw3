/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "rttiSystem.h"

#include "profiler.h"
#include "classBuilder.h"
#include "enum.h"
#include "bitField.h"
#include "function.h"
#include "softHandle.h"
#include "object.h"
#include "xmlWriter.h"
#include "importer.h"
#include "exporter.h"
#include "factory.h"
#include "cooker.h"
#include "commandlet.h"
#include "fileSys.h"
#include "xmlReader.h"
#include "resource.h"

#include <ctime>

/*
FIXME:
Makes loading the scripts take forever because of ScriptSyntaxNode::CheckNodeTypeIdentifier( CScriptFunctionCompiler* compiler )

// It may be a structure...
CClass* structure = SRTTI::GetInstance().FindClass( CName( m_value.m_string ) );
if ( structure != NULL )
*/
#ifndef RED_PLATFORM_ORBIS
# define NO_RTTI_LOG_SPAM_TMPHACK
#endif

CRTTISystem::CRTTISystem()
{
}

CRTTISystem::~CRTTISystem()
{
}

namespace 
{
#ifndef RED_FINAL_BUILD
	static Bool IsPointerToResource( const IRTTIType* propType )
	{
		if ( propType->GetType() == RT_Pointer )
		{
			const IRTTIPointerTypeBase* pointerType = static_cast<const IRTTIPointerTypeBase*>( propType );
			if ( pointerType->GetPointedType()->GetType() == RT_Class )
			{
				const CClass* pointedClass = pointerType->GetPointedType();
				return pointedClass && pointedClass->IsA< CResource >();
			}
		}
		else if ( propType->GetType() == RT_Array || propType->GetType() == RT_NativeArray || propType->GetType() == RT_StaticArray )
		{
			const IRTTIBaseArrayType* arrayType = static_cast< const IRTTIBaseArrayType* >( propType );
			return IsPointerToResource( arrayType->ArrayGetInnerType() );
		}

		return false;
	}

	static Bool IsHandleToResource( const IRTTIType* propType )
	{
		if ( propType->GetType() == RT_Handle )
		{
			const IRTTIPointerTypeBase* pointerType = static_cast<const IRTTIPointerTypeBase*>( propType );
			if ( pointerType->GetPointedType()->GetType() == RT_Class )
			{
				const CClass* pointedClass = pointerType->GetPointedType();
				return pointedClass && pointedClass->IsA< CResource >();
			}
		}
		else if ( propType->GetType() == RT_Array || propType->GetType() == RT_NativeArray || propType->GetType() == RT_StaticArray )
		{
			const IRTTIBaseArrayType* arrayType = static_cast< const IRTTIBaseArrayType* >( propType );
			return IsHandleToResource( arrayType->ArrayGetInnerType() );
		}

		return false;
	}

	static void ExtractPointers( const IRTTIType* propType, THashMap< const CClass*, Uint32 >& outClasses )
	{
		if ( propType->GetType() == RT_Handle || propType->GetType() == RT_Pointer )
		{
			const IRTTIPointerTypeBase* pointerType = static_cast<const IRTTIPointerTypeBase*>( propType );
			if ( pointerType->GetPointedType()->GetType() == RT_Class )
			{
				const CClass* pointedClass = pointerType->GetPointedType();
				outClasses.Set( pointedClass, 1 );
			}
		}
		else if ( propType->GetType() == RT_Array || propType->GetType() == RT_NativeArray || propType->GetType() == RT_StaticArray )
		{
			const IRTTIBaseArrayType* arrayType = static_cast< const IRTTIBaseArrayType* >( propType );
			return ExtractPointers( arrayType->ArrayGetInnerType(), outClasses );
		}
	}
#endif
}


Red::System::Bool DumpRTTIClassListOOM( Red::MemoryFramework::PoolLabel pool )
{
	if ( pool == Memory::GetPoolLabel< MemoryPool_CObjects>() )
	{
		const auto& classes = SRTTI::GetInstance().GetIndexedClasses();

		TDynArray< Uint32 > totalSizes;
		TDynArray< Uint32 > totalCounts;
		TDynArray< Uint32 > totalLevel;

		totalSizes.Resize( classes.Size() );
		totalCounts.Resize( classes.Size() );
		totalLevel.Resize( classes.Size() );

		Uint32 allObjects = 0;
		Uint32 allMemory = 0;
		for ( Uint32 i=0; i<classes.Size(); ++i )
		{
			const CClass* rttiClass = classes[i];
			if ( rttiClass )
			{
				const Uint32 numObjects = rttiClass->GetNumAllocatedObjects();
				const Uint32 numBytes = rttiClass->GetNumAllocatedObjects() * rttiClass->GetSize();

				allObjects += numObjects;
				allMemory += numBytes;

				Uint32 level = 0;
				while ( rttiClass )
				{
					totalCounts[ rttiClass->GetClassIndex() ] += numObjects;
					totalSizes[ rttiClass->GetClassIndex() ] += numBytes;
					rttiClass = rttiClass->GetBaseClass();
					level += 1;
				}

				totalLevel[ classes[i]->GetClassIndex() ] = level;
			}
		}

		for ( Uint32 i=0; i<classes.Size(); ++i )
		{
			const CClass* rttiClass = classes[i];
			if ( rttiClass && totalCounts[i] > 0 )
			{
				AnsiChar margin[50];

				const Uint32 level = totalLevel[i];
				for ( Uint32 j=0; j<level; ++j )
				{
					margin[j] = ' ';
				}
				margin[level] = 0;

				LOG_CORE( TXT("| %06d | %10.2f KB | %hs%hs "), 
					totalCounts[ i ], totalSizes[ i ] / 1024.0f,
					margin, rttiClass->GetName().AsAnsiChar() );
			}
		}

		LOG_CORE( TXT("Total objects: %d"), allObjects );
		LOG_CORE( TXT("Total size: %1.2fKB"), allMemory / 1024.0f );
	}

	return false;
}

void DumpRTTIClassList()
{
	DumpRTTIClassListOOM( Memory::GetPoolLabel< MemoryPool_CObjects >() );
}

void CRTTISystem::Init()
{
	m_classBuilders.Shrink(); // all class builder were added. Get rid of memory excess.

	// Load class remap table
	LoadClassRemapTable();

	// Initialize all class builders
	for( IClassBuilder* classBuilder : m_classBuilders )
	{
		classBuilder->InitClass();
	}

	// Reindex classes
	ReindexClasses();

#ifndef RED_FINAL_BUILD
	RED_MEMORY_REGISTER_OOM_CALLBACK( DumpRTTIClassListOOM );
#endif

#ifdef RED_PLATFORM_WINPC
#ifndef RED_FINAL_BUILD
	// Validate all properties and classes that have a pointer to CResource in it
	{
		Uint32 numPropertiesToChange = 0;
		for ( auto i=m_types.Begin(); i != m_types.End(); ++i )
		{
			const IRTTIType* type = i->m_second;
			if ( type->GetType() == RT_Class )
			{
				const CClass* classType = static_cast< const CClass* >( type );
				const auto& props = classType->GetLocalProperties();

				for ( Uint32 j=0; j<props.Size(); ++j )
				{
					const CProperty* prop = props[j];
					const IRTTIType* propType = prop->GetType();

					if ( IsPointerToResource( propType ) )
					{
						RED_FATAL( "Property '%ls' in class '%ls' is pointer to resource and should be a THandle, type '%ls'", 
							prop->GetName().AsChar(), classType->GetName().AsChar(), propType->GetName().AsChar() );
						numPropertiesToChange += 1;
					}
				}
			}
		}

		if ( numPropertiesToChange > 0 )
		{
			ERR_CORE( TXT( "Found %d properties that are pointers to resources"), numPropertiesToChange );
		}
	}

	// Create a list of classes that are kept via pointer (or handle)
	THashMap< const CClass*, Uint32 > classesAccessedByPointers;
	for ( auto i=m_types.Begin(); i != m_types.End(); ++i )
	{
		const IRTTIType* type = i->m_second;
		if ( type->GetType() == RT_Class )
		{
			const CClass* classType = static_cast< const CClass* >( type );

			const auto& props = classType->GetLocalProperties();
			for ( Uint32 j=0; j<props.Size(); ++j )
			{
				const CProperty* prop = props[j];
				const IRTTIType* propType = prop->GetType();

				ExtractPointers( propType, classesAccessedByPointers );
			}
		}
	}

	// stats
	LOG_CORE( TXT("%d classes are accessed via pointers"), classesAccessedByPointers.Size() );

	// Validate that handles/pointers to resources are at least in something that is an ISerializable
	// We don't care about structures or embedded classes
	{
		Uint32 numPropertiesToChange = 0;
		for ( auto it = classesAccessedByPointers.Begin(); it != classesAccessedByPointers.End(); ++it )
		{
			const CClass* classType = (*it).m_first;

			if ( classType->IsSerializable() )
				continue;

			const auto& props = classType->GetLocalProperties();
			for ( Uint32 j=0; j<props.Size(); ++j )
			{
				const CProperty* prop = props[j];
				const IRTTIType* propType = prop->GetType();

				if ( IsPointerToResource( propType ) )
				{
					RED_HALT( "Property '%ls' in class '%ls' is a pointer to resource, THIS IS NOT SUPPORTED. The owning class is not an ISerializable but is created dynamically and stored somewhere via pointer. Full type: (type '%ls')", 
						prop->GetName().AsChar(), classType->GetName().AsChar(), propType->GetName().AsChar() );
					numPropertiesToChange += 1;
				}
				else if ( IsHandleToResource( propType ) )
				{
					RED_HALT( "Property '%ls' in class '%ls' is a handle to resource, THIS IS NOT SUPPORTED. The owning class is not an ISerializable but is created dynamically and stored somewhere via pointer. Full type: (type '%ls')", 
						prop->GetName().AsChar(), classType->GetName().AsChar(), propType->GetName().AsChar() );
					numPropertiesToChange += 1;
				}
			}
		}

		if ( numPropertiesToChange > 0 )
		{
			ERR_CORE( TXT( "Found %d references to resources inside a non ISerializable classes"), numPropertiesToChange );
		}
	}
#endif // !FINAL
#endif // WINPC
}

void CRTTISystem::CreateDefaultObjects()
{
	if( !m_indexedClasses.Empty() )
	{
		// First class is always null; skip!
		for( auto iter = m_indexedClasses.Begin() + 1, end = m_indexedClasses.End(); iter != end; ++iter )
		{
			(*iter)->CreateDefaultObject();
		}
	}
}

void CRTTISystem::DestroyDefaultObjects()	
{
	for ( auto i=m_types.Begin(); i != m_types.End(); ++i )
	{
		IRTTIType* type = i->m_second;
		if ( type->GetType() == RT_Class )
		{
			// Clear script data from class
			CClass* classType = static_cast< CClass* >( type );
			classType->DestroyDefaultObject();
		}
	}
}

void CRTTISystem::Deinit()
{
	for( IClassBuilder* classBuilder : m_classBuilders )
	{
		classBuilder->DeinitClass();
	}
}

void CRTTISystem::ClearScriptData( const void* ptr )
{
	const TDynArray< THandle< IScriptable > >& scriptablesToClean = *reinterpret_cast< const TDynArray< THandle< IScriptable > >*>( ptr );

	// Release script data buffers
	for ( Uint32 i=0; i<scriptablesToClean.Size(); ++i )
	{
		IScriptable* scriptable = scriptablesToClean[i].Get();
		if ( nullptr != scriptable )
		{
			scriptable->ReleaseScriptPropertiesBuffer();
		}
	}

	// Cleanup script data from classes
	TDynArray< CClass* > classesToDelete;
	for ( auto i=m_types.Begin(); i != m_types.End(); ++i )
	{
		IRTTIType* type = i->m_second;

		if ( type->GetType() == RT_Class )
		{
			// Clear script data from class
			CClass* classType = static_cast< CClass* >( type );
			classType->ClearScriptData();

			// Fully scripted class ?
			if ( !classType->IsNative() )
			{
				classesToDelete.PushBack( classType );
			}

			// Clear the scripted flag on class
			classType->m_flags &= ~CF_Scripted;
			classType->m_scriptSize = 0;
		}
	}

	// Cleanup script data from enumes
	TDynArray< CEnum* > enumsToDelete;
	for ( auto i=m_types.Begin(); i != m_types.End(); ++i )
	{
		IRTTIType* type = i->m_second;
		if ( type->GetType() == RT_Enum )
		{
			// Clear script data from enum
			CEnum* enumType = static_cast< CEnum* >( type );
			if ( enumType->IsScripted() )
			{
				enumsToDelete.PushBack( enumType );
			}
		}
	}

	// Cleanup global functions
	TDynArray< CFunction* > functionsToDelete;
	for ( auto i=m_globalFunctions.Begin(); i != m_globalFunctions.End(); ++i )
	{
		// Clear script data from enum
		CFunction* func = i->m_second;
		if ( func->IsNative() )
		{
			func->ClearScriptData();
		}
		else
		{
			// Script functions are deleted
			functionsToDelete.PushBack( func );
		}
	}

	// Delete classes
	for ( Uint32 i = 0; i < classesToDelete.Size(); ++i )
	{
		m_types.Erase( classesToDelete[ i ]->GetName() );
		m_stubScriptClasses.PushBack( classesToDelete[ i ] );
	}

	// Delete enums
	for ( Uint32 i = 0; i < enumsToDelete.Size(); ++i )
	{
		m_types.Erase( enumsToDelete[ i ]->GetName() );
		m_stubScriptEnums.PushBack( enumsToDelete[ i ] );
	}

	// Delete functions
	for ( Uint32 i = 0; i < functionsToDelete.Size(); ++i )
	{
		m_globalFunctions.Erase( functionsToDelete[ i ]->GetName() );
		delete functionsToDelete[ i ];
	}
}

void CRTTISystem::RestoreScriptData( const void* ptr )
{
	const TDynArray< THandle< IScriptable > >& scriptablesToRestore = *reinterpret_cast< const TDynArray< THandle< IScriptable > >*>( ptr );

	/// Get the default objects
	TDynArray< CObject* > defaultObjects;
	for ( BaseObjectIterator it( OF_DefaultObject, 0 ); it; ++it )
	{
		defaultObjects.PushBack( *it );
	}

	// Restore data buffers of script objects
	for ( Uint32 i = 0; i < scriptablesToRestore.Size(); ++i )
	{
		IScriptable* scriptable = scriptablesToRestore[ i ].Get();
		if ( nullptr != scriptable )
		{
			scriptable->CreateScriptPropertiesBuffer();
			scriptable->GetClass()->ApplyDefaultValues( scriptable, Cast< IScriptable >( scriptable ), true, true );

			if ( scriptable && scriptable->IsA< CObject >() )
			{
				CObject* object = static_cast< CObject* >( scriptable );
				if ( object->HasFlag( OF_DefaultObject ) )
				{
					defaultObjects.RemoveFast( object );
				}
			}
		}
	}

	// Create scripting buffers in the default objects that were created during compilation
	for ( Uint32 i=0; i<defaultObjects.Size(); ++i )
	{
		CObject* defaultObject = defaultObjects[i];
		RED_ASSERT( defaultObject->GetScriptPropertyData() == nullptr );

		if ( !defaultObject->GetScriptPropertyData() )
		{
			defaultObject->CreateScriptPropertiesBuffer();
			defaultObject->GetClass()->ApplyDefaultValues( defaultObject, Cast< IScriptable >( defaultObject ), true, true );
		}
	}
}

void CRTTISystem::RegisterType( IRTTIType *type )
{
	ASSERT( type );

	const CName typeName( type->GetName() );

	if ( FindType( typeName ) )
	{
		HALT( "Type %s is being registered for the second time!", type->GetName().AsString().AsChar() );
	}

	VERIFY( m_types.Insert( typeName, type ) ); 
}

void CRTTISystem::UnregisterType( IRTTIType *type )
{
	ASSERT( type );

	auto it = m_types.Find( CName( type->GetName() ) );
	if ( it !=  m_types.End() )
	{
		m_types.Erase( it );
	}
	else
	{
		WARN_CORE( TXT( "Type %s not registered in RTTI system !" ), type->GetName().AsString().AsChar() );
	}
}

void CRTTISystem::RegisterProperty( CProperty* prop )
{
	RED_FATAL_ASSERT( prop->GetHash() != 0, "Trying to register prop '%ls' with no hash", prop->GetName().AsChar() );

	CProperty* existingProp = nullptr;
	m_properties.Find( prop->GetHash(), existingProp );
	if ( existingProp )
	{
		if ( existingProp == prop )
		{
			RED_FATAL( "Double registration of property '%ls' in '%ls'",
				prop->GetName().AsChar(), prop->GetParent()->GetName().AsChar() );
		}
		else
		{
			RED_FATAL( "Property hash collision between '%ls' in '%ls' and already registered '%ls' in '%ls'", 
				prop->GetName().AsChar(), prop->GetParent()->GetName().AsChar(),
				existingProp->GetName().AsChar(), existingProp->GetParent()->GetName().AsChar() );
		}
	}
	else
	{
		m_properties.Insert( prop->GetHash(), prop );
	}
}

void CRTTISystem::UnregisterProperty( CProperty* prop )
{
	if ( prop->GetHash() )
	{
		m_properties.Erase( prop->GetHash() );
	}
}

void CRTTISystem::RegisterClassBuilder( IClassBuilder *builder )
{
	ASSERT( !m_classBuilders.Exist( builder ) );
	m_classBuilders.PushBack( builder );
}

void CRTTISystem::UnregisterClassBuilder( IClassBuilder *builder )
{
	m_classBuilders.RemoveFast( builder );
}

CClass* CRTTISystem::FindClass( const CName& name )
{
	// Find in types list, if not found use name remapping
	return static_cast< CClass* >( FindType( name, RT_Class ) );
}

CEnum* CRTTISystem::FindEnum( const CName& name )
{
	return static_cast< CEnum* >( FindType( name, RT_Enum ) );
}

CBitField* CRTTISystem::FindBitField( const CName& name )
{
	return static_cast< CBitField* >( FindType( name, RT_BitField ) );
}

IRTTIType* CRTTISystem::FindSimpleType( const CName& name )
{
	return FindType( name, RT_Simple );
}

IRTTIType* CRTTISystem::FindFundamentalType( const CName & name )
{
	return FindType( name, RT_Fundamental );
}

IRTTIType* CRTTISystem::HandleOldWrappedTypes( const CName& typeName )
{
	// we still need a way to resolve CName -> Char* in order for RTTI to work
	const Char* typeNameStr = typeName.AsChar();

	// old RTTI type naming was somehow cryptic
	// '@' - Dynamic array
	//  '*' - C pointer
	//  '#' - Handle
	//  '~' - Soft handle
	if ( typeNameStr[0] == '@' )
	{
		// resolve the inner type
		const CName innerTypeName( typeNameStr + 1 );
		IRTTIType* innerType = FindType( innerTypeName );
		if ( NULL == innerType )
		{
			WARN_CORE( TXT("Failed to resolve old array type '%ls'. Inner type is unknown."), typeNameStr + 1 );
			return NULL;
		}

		// get the propper type
		const CName fullTypeName = CRTTIArrayType::FormatName( innerType, MC_DynArray, Memory::GetPoolLabel< MemoryPool_Default >() );
		IRTTIType* wrappedType = FindType( fullTypeName );
		if ( NULL != wrappedType )
		{
			// register with the old name
			m_types.Set( typeName, wrappedType );
		}
		return wrappedType;
	}
	else if ( typeNameStr[0] == '*' )
	{
		// resolve the inner type
		const CName innerTypeName( typeNameStr + 1 );
		IRTTIType* innerType = FindType( innerTypeName );
		if ( NULL == innerType )
		{
			WARN_CORE( TXT("Failed to resolve old pointer type '%ls'. Inner type is unknown."), typeNameStr + 1 );
			return NULL;
		}

		// get the propper type
		const CName fullTypeName = CRTTIPointerType::FormatName( innerType );
		IRTTIType* wrappedType = FindType( fullTypeName );
		if ( NULL != wrappedType )
		{
			// register with the old name
			m_types.Set( typeName, wrappedType );
		}
		return wrappedType;
	}
	else if ( typeNameStr[0] == '#' )
	{
		// resolve the inner type
		const CName innerTypeName( typeNameStr + 1 );
		IRTTIType* innerType = FindType( innerTypeName );
		if ( NULL == innerType )
		{
			WARN_CORE( TXT("Failed to resolve old handle type '%ls'. Inner type is unknown."), typeNameStr + 1 );
			return NULL;
		}

		// get the propper type
		const CName fullTypeName = CRTTIHandleType::FormatName( innerType );
		IRTTIType* wrappedType = FindType( fullTypeName );
		if ( NULL != wrappedType )
		{
			// register with the old name
			m_types.Set( typeName, wrappedType );
		}
		return wrappedType;
	}
	else if ( typeNameStr[0] == '~' )
	{
		// resolve the inner type
		const CName innerTypeName( typeNameStr + 1 );
		IRTTIType* innerType = FindType( innerTypeName );
		if ( NULL == innerType )
		{
			WARN_CORE( TXT("Failed to resolve old soft handle type '%ls'. Inner type is unknown."), typeNameStr + 1 );
			return NULL;
		}

		// get the propper type
		const CName fullTypeName = CRTTISoftHandleType::FormatName( innerType );
		IRTTIType* wrappedType = FindType( fullTypeName );
		if ( NULL != wrappedType )
		{
			// register with the old name
			m_types.Set( typeName, wrappedType );
		}
		return wrappedType;
	}

	// Not an old type
	return NULL;
}

CProperty* CRTTISystem::FindProperty( const Uint64 propertyHash )
{
	CProperty* ret = nullptr;
	m_properties.Find( propertyHash, ret );
	return ret;
}

IRTTIType* CRTTISystem::FindType( const CName& oldName )
{	
	// NULL type?
	if ( !oldName )
	{
		return NULL;
	}

	// Hack for remapping some old class names
	CName name = oldName;
	m_classRemapTable.Find( name, name );

	// Find type by name
	IRTTIType* type = NULL;
	if ( m_types.Find( name, type ) )
	{
		return type;
	}

	// Consider old type names
	type = HandleOldWrappedTypes( name );
	if ( type != NULL )
	{
		// type was handled by the old naming
		return type;
	}

	// Note about type name parsing:
	// The parsing is done manutally in a very straighforward way (unsafe under some conditions).
	// I didn't want to use any heavy code here like tokenization, string splitting, etc. for performance reasons.
	// Note that the type names are generated automatically anyway so there's no direct risk of messing something up here.
	const Char* typeNameStrOrg = name.AsChar();
	const Char* typeNameStr = typeNameStrOrg; // this gets shifted by parsing

	// Dynamic array
	if ( GParseKeyword( typeNameStr, TXT("array:") ) )
	{
		// Rule here is: we don't want to complicate the parsing rules so the memory class and memory pool information goes FIRST
		// The array type is the last parameter in the array description. This way we can have a lot simpler parsing code.

		// Parse memory class
		Uint32 memoryClassValue = 0;
		if ( !GParseInteger( typeNameStr, memoryClassValue ) )
		{
#ifndef	NO_RTTI_LOG_SPAM_TMPHACK
			ERR_CORE( TXT("RTTI type name '%ls' is in invalid format"), typeNameStrOrg );
#endif
			return NULL;
		}

		// A comma separator
		if ( !GParseKeyword( typeNameStr, TXT(",") ) )
		{
#ifndef	NO_RTTI_LOG_SPAM_TMPHACK
			ERR_CORE( TXT("RTTI type name '%ls' is in invalid format"), typeNameStrOrg );
#endif
			return NULL;
		}

		// Parse memory pool (value of 0 means MemPool_Default)
		Uint32 memoryPoolValue = 0;
		if ( !GParseInteger( typeNameStr, memoryPoolValue ) )
		{
#ifndef	NO_RTTI_LOG_SPAM_TMPHACK
			ERR_CORE( TXT("RTTI type name '%ls' is in invalid format"), typeNameStrOrg );
#endif
			return NULL;
		}

		// A comma separator
		if ( !GParseKeyword( typeNameStr, TXT(",") ) )
		{
#ifndef	NO_RTTI_LOG_SPAM_TMPHACK
			ERR_CORE( TXT("RTTI type name '%ls' is in invalid format"), typeNameStrOrg );
#endif
			return NULL;
		}

		// Here comes the actual inner type name, find it
		const CName innerTypeName( typeNameStr ); 
		IRTTIType* innerType = FindType( innerTypeName );
		if ( NULL == innerType )
		{
#ifndef	NO_RTTI_LOG_SPAM_TMPHACK
			ERR_CORE( TXT("RTTI array type '%ls' uses unknown or invalid inner type '%ls'"), typeNameStrOrg, typeNameStr );
#endif
			return NULL;
		}

		// Create final wrapper and register it in the type array
		CRTTIArrayType* arrayType = new CRTTIArrayType( innerType, (EMemoryClass)memoryClassValue, (EMemoryPoolLabel)memoryPoolValue );
		VERIFY( m_types.Insert( name, arrayType ) );
		return arrayType;
	}

	// Native static array
	else if ( GParseKeyword( typeNameStr, TXT("[") ) )
	{
		// Size of the array
		Uint32 numElements = 0;
		if ( !GParseInteger( typeNameStr, numElements ) )
		{
#ifndef	NO_RTTI_LOG_SPAM_TMPHACK
			ERR_CORE( TXT("RTTI type name '%ls' is in invalid format"), typeNameStrOrg );
#endif
			return NULL;
		}

		// end of element count
		if ( !GParseKeyword( typeNameStr, TXT("]") ) )
		{
#ifndef	NO_RTTI_LOG_SPAM_TMPHACK
			ERR_CORE( TXT("RTTI type name '%ls' is in invalid format"), typeNameStrOrg );
#endif
			return NULL;
		}

		// Here comes the actual inner type name, find it
		const CName innerTypeName( typeNameStr ); 
		IRTTIType* innerType = FindType( innerTypeName );
		if ( NULL == innerType )
		{
#ifndef	NO_RTTI_LOG_SPAM_TMPHACK
			ERR_CORE( TXT("RTTI static array type '%ls' uses unknown or invalid inner type '%ls'"), typeNameStrOrg, typeNameStr );
#endif
			return NULL;
		}

		// Create final static array type wrapper and register it in the type array
		CRTTINativeArrayType* arrayType = new CRTTINativeArrayType( innerType, numElements );
		VERIFY( m_types.Insert( name, arrayType ) );
		return arrayType;
	}

	// Static array (TStaticArray)
	else if ( GParseKeyword( typeNameStr, TXT("static:") ) )
	{
		// Size of the array
		Uint32 numElements = 0;
		if ( !GParseInteger( typeNameStr, numElements ) )
		{
#ifndef	NO_RTTI_LOG_SPAM_TMPHACK
			ERR_CORE( TXT("RTTI type name '%ls' is in invalid format"), typeNameStrOrg );
#endif
			return NULL;
		}

		// end of element count
		if ( !GParseKeyword( typeNameStr, TXT(",") ) )
		{
#ifndef	NO_RTTI_LOG_SPAM_TMPHACK
			ERR_CORE( TXT("RTTI type name '%ls' is in invalid format"), typeNameStrOrg );
#endif
			return NULL;
		}

		// Here comes the actual inner type name, find it
		const CName innerTypeName( typeNameStr ); 
		IRTTIType* innerType = FindType( innerTypeName );
		if ( NULL == innerType )
		{
#ifndef	NO_RTTI_LOG_SPAM_TMPHACK
			ERR_CORE( TXT("RTTI static array type '%ls' uses unknown or invalid inner type '%ls'"), typeNameStrOrg, typeNameStr );
#endif
			return NULL;
		}

		// Create final static array type wrapper and register it in the type array
		CRTTIStaticArrayType* arrayType = new CRTTIStaticArrayType( innerType, numElements );
		VERIFY( m_types.Insert( name, arrayType ) );
		return arrayType;
	}

	// Pointer (C style)
	else if ( GParseKeyword( typeNameStr, TXT("ptr:") ) )
	{
		// Now comes the actual pointed type name, find it
		const CName pointedTypeTypeName( typeNameStr ); 
		IRTTIType* pointedType = FindType( pointedTypeTypeName );
		if ( NULL == pointedType )
		{
#ifndef	NO_RTTI_LOG_SPAM_TMPHACK
			ERR_CORE( TXT("RTTI pointer type '%ls' uses unknown or invalid pointed type '%ls'"), typeNameStrOrg, typeNameStr );
#endif
			return NULL;
		}

		// Create final wrapper and register it in the type array
		CRTTIPointerType* pointerType = new CRTTIPointerType( pointedType );
		VERIFY( m_types.Insert( name, pointerType ) );
		return pointerType;
	}

	// Handle
	else if ( GParseKeyword( typeNameStr, TXT("handle:") ) )
	{
		// Now comes the actual pointed type name, find it
		const CName pointedTypeTypeName( typeNameStr ); 
		IRTTIType* pointedType = FindType( pointedTypeTypeName );
		if ( NULL == pointedType )
		{
#ifndef	NO_RTTI_LOG_SPAM_TMPHACK
			ERR_CORE( TXT("RTTI handle type '%ls' uses unknown or invalid pointed type '%ls'"), typeNameStrOrg, typeNameStr );
#endif
			return NULL;
		}

		// Create final wrapper and register it in the type array
		CRTTIHandleType* pointerType = new CRTTIHandleType( pointedType );
		VERIFY( m_types.Insert( name, pointerType ) );
		return pointerType;
	}

	// Soft handle
	else if ( GParseKeyword( typeNameStr, TXT("soft:") ) )
	{
		// Now comes the actual pointed type name, find it
		const CName pointedTypeTypeName( typeNameStr ); 
		IRTTIType* pointedType = FindType( pointedTypeTypeName );
		if ( NULL == pointedType )
		{
#ifndef	NO_RTTI_LOG_SPAM_TMPHACK
			ERR_CORE( TXT("RTTI soft handle type '%ls' uses unknown or invalid pointed type '%ls'"), typeNameStrOrg, typeNameStr );
#endif
			return NULL;
		}

		// Create final wrapper and register it in the type array
		CRTTISoftHandleType* pointerType = new CRTTISoftHandleType( pointedType );
		VERIFY( m_types.Insert( name, pointerType ) );
		return pointerType;
	}
 
	// Not found and no wrapping type created
	return NULL;
}

IRTTIType* CRTTISystem::FindType( const CName& name, ERTTITypeType type )
{
	IRTTIType* rttiType = FindType( name );

	if ( rttiType && rttiType->GetType() == type )
	{
		return rttiType;
	}
	else
	{
		return NULL;
	}
}

void CRTTISystem::EnumFunctions( TDynArray< CFunction* >& functions )
{
	// Global functions
	for ( auto i=m_globalFunctions.Begin(); i != m_globalFunctions.End(); ++i )
	{
		CFunction* function = i->m_second;
		functions.PushBack( function );
	}

	// Class functions
	for ( auto i=m_types.Begin(); i != m_types.End(); ++i )
	{
		IRTTIType* type = i->m_second;
		if ( type->GetType() == RT_Class )
		{
			CClass* classType = static_cast< CClass* >( type );
			const auto& localFunctions = classType->GetLocalFunctions();
			for ( CFunction* func : localFunctions )
				functions.PushBack( func );
		}
	}
}

void CRTTISystem::EnumClasses( CClass* baseClass, TDynArray< CClass* > &classes, Bool classFilter( CClass * ) /* = NULL */, Bool allowAbstract /*= false*/  )
{
	classes.Reserve( m_types.Size() ); // Worst case scenario

	if ( baseClass == nullptr )
	{
		for ( auto i=m_types.Begin(); i != m_types.End(); ++i )
		{
			IRTTIType* type = i->m_second;
			if ( type->GetType() == RT_Class )
			{
				CClass* classType = static_cast< CClass* >( type );
				if ( classFilter == NULL || classFilter( classType ) )
				{
					classes.PushBack( classType );
				}
			}
		}
	}
	else
	{
		for ( auto i=m_types.Begin(); i != m_types.End(); ++i )
		{
			IRTTIType* type = i->m_second;
			if ( type->GetType() == RT_Class )
			{
				CClass* classType = static_cast< CClass* >( type );
				if ( classType->DynamicIsA( baseClass ) && ( allowAbstract || !classType->IsAbstract() ) &&
					( classFilter == NULL || classFilter( classType ) ) )
				{
					classes.PushBack( classType );
				}
			}
		}
	}
}

void CRTTISystem::EnumDerivedClasses( CClass* baseClass, TDynArray< CClass* > &classes )
{
	for ( auto i=m_types.Begin(); i != m_types.End(); ++i )
	{
		IRTTIType* type = i->m_second;
		if ( type->GetType() == RT_Class )
		{
			CClass* classType = static_cast< CClass* >( type );
			if ( classType->GetBaseClass() == baseClass )
			{
				classes.PushBack( classType );
			}
		}
	}
}

void CRTTISystem::EnumEnums( TDynArray< CEnum* > &enums )
{
	for ( auto i=m_types.Begin(); i != m_types.End(); ++i )
	{
		IRTTIType* type = i->m_second;
		if ( type->GetType() == RT_Enum )
		{
			CEnum* enumType = static_cast< CEnum* >( type );
			enums.PushBack( enumType );
		}
	}
}

CClass* CRTTISystem::CreateScriptedClass( CName className, Uint32 flags )
{
	// Make sure we do not duplicate types
	ASSERT( !FindClass( className ) );

	// First check in the stub list
	for ( Uint32 i = 0; i < m_stubScriptClasses.Size(); ++i )
	{
		CClass* stubClass = m_stubScriptClasses[i];
		if ( stubClass->GetName() == className )
		{
			stubClass->ReuseScriptStub( flags );
			m_stubScriptClasses.Remove( stubClass );
			RegisterType( stubClass );
			return stubClass;
		}
	}

	// Create new scripted class
	CClass* newClass = new CScriptedClass( className, flags );
	RegisterType( newClass );
	return newClass;
}

CEnum* CRTTISystem::CreateScriptedEnum( CName enumName, Uint32 enumSize )
{
	// Make sure we do not duplicate types
	ASSERT( !FindEnum( enumName ) );

	// First check in the stub list
	for ( Uint32 i = 0; i < m_stubScriptEnums.Size(); ++i )
	{
		CEnum* stubEnum = m_stubScriptEnums[ i ];
		if ( stubEnum->GetName() == enumName )
		{
			stubEnum->ReuseScriptStub( enumSize );
			m_stubScriptEnums.Remove( stubEnum );
			RegisterType( stubEnum );
			return stubEnum;
		}
	}

	// Create new scripted enum
	CEnum* newEnum = new CEnum( enumName, enumSize, true );
	RegisterType( newEnum );
	return newEnum;
}

Bool CRTTISystem::CanCast( const CName& sourceType, const CName& destType ) const
{
	const IRTTIType* sourceRealType = const_cast< CRTTISystem* >( this )->FindType( sourceType );
	const IRTTIType* destRealType = const_cast< CRTTISystem* >( this )->FindType( destType );
	return CanCast( sourceRealType, destRealType );
}

Bool CRTTISystem::CanCast( const IRTTIType* sourceType, const IRTTIType* destType ) const
{
	// Source or destination type is invalid
	if ( !sourceType || !destType )
	{
		return false;
	}

	// The same type, casting obviously possible
	if ( sourceType == destType )
	{
		return true;
	}

	// We cannot cast between totally different types
	const ERTTITypeType sourceTypeType = sourceType->GetType();
	const ERTTITypeType destTypeType = destType->GetType();
	if ( sourceTypeType != destTypeType )
	{
		return false;
	}

	// We can cast only between the same simple types
	if ( sourceTypeType == RT_Enum || sourceTypeType == RT_Simple || sourceTypeType == RT_Class )
	{
		// Compare types directly
		return sourceType->GetName() == destType->GetName();
	}

	// Arrays, recurse
	if ( sourceTypeType == RT_Array )
	{
		const CRTTIArrayType* sourceArray = static_cast< const CRTTIArrayType* >( sourceType );
		const CRTTIArrayType* destArray = static_cast< const CRTTIArrayType* >( destType );
		return CanCast( sourceArray->GetInnerType(), destArray->GetInnerType() );
	}

	// Arrays, recurse
	if ( sourceTypeType == RT_NativeArray )
	{
		const CRTTINativeArrayType* sourceArray = static_cast< const CRTTINativeArrayType* >( sourceType );
		const CRTTINativeArrayType* destArray = static_cast< const CRTTINativeArrayType* >( destType );
		return CanCast( sourceArray->GetInnerType(), destArray->GetInnerType() );
	}

	// Pointers/Handles, check classes
	if ( sourceTypeType == RT_Pointer || sourceTypeType == RT_Handle )
	{
		const IRTTIType* sourcePointedType = static_cast< const IRTTIPointerTypeBase* >( sourceType )->GetPointedType();
		const IRTTIType* destPointedType = static_cast< const IRTTIPointerTypeBase* >( destType )->GetPointedType();
		if ( sourcePointedType && destPointedType )
		{
			if ( sourcePointedType->GetType() == RT_Class && destPointedType->GetType() == RT_Class )
			{
				const CClass* sourceClass = static_cast< const CClass* >( sourcePointedType );
				const CClass* destClass = static_cast< const CClass* >( destPointedType );

				// TODO: ehh, this is needed because of some stupid assertions in Brix
				// there is a lot of places when CNode* needs to be fitted inside a CEntity* type...
				if ( destClass->IsA( sourceClass ) )
					return true;

				return sourceClass->IsA( destClass );
			}
		}
	}

	// No casting possible
	return false;
}

void* CRTTISystem::CastPointer( const CName& destTypeName, const CName& srcTypeName, void* ptr )
{
	// Get the destination type and class
	IRTTIType* destType = SRTTI::GetInstance().FindType( destTypeName );
	ASSERT( destType && destType->GetType() == RT_Pointer );
	IRTTIType* destClass = static_cast< CRTTIPointerType* >( destType )->GetPointedType();
	ASSERT( destClass && destClass->GetType() == RT_Class );

	// Get the source type and class
	IRTTIType* sourceType = SRTTI::GetInstance().FindType( srcTypeName );
	ASSERT( sourceType && sourceType->GetType() == RT_Pointer );
	IRTTIType* sourceClass = static_cast< CRTTIPointerType* >( sourceType )->GetPointedType();
	ASSERT( sourceClass && sourceClass->GetType() == RT_Class );

	// Do the cast
	CClass* realSourceClass = static_cast< CClass* >( sourceClass );
	CClass* realDestClass = static_cast< CClass* >( destClass );
	if ( realDestClass->IsA( realSourceClass ) )
	{
		return ptr;
	}
	else
	{
		return NULL;
	}
}

void CRTTISystem::RegisterGlobalFunction( CFunction* function )
{
	ASSERT( function );
	if ( FindGlobalFunction( function->GetName() ) )
	{
		WARN_CORE( TXT("Global function '%ls' already defined"), function->GetName().AsString().AsChar() );
		return;
	}

	// Register
	m_globalFunctions.Insert( function->GetName(), function );
}

CFunction* CRTTISystem::FindGlobalFunction( const CName& name )
{
	CFunction* function = NULL;
	if ( m_globalFunctions.Find( name, function ) )
	{
		return function;
	}

	return NULL;
}

CEnum* CRTTISystem::FindEnumValue( const String& name, Int32& value )
{
    CName valueToSearch( name ); 

	for ( auto i=m_types.Begin(); i != m_types.End(); ++i )
	{
		IRTTIType* type = i->m_second;
		if ( type->GetType() == RT_Enum )
		{
			CEnum* enumerator = ( CEnum* ) type;

            if ( enumerator->FindValue( valueToSearch, value ) )
			{
				return enumerator;
			}
		}
	}

	// Not found
	return NULL;
}

#define REMAP( _x, _y ) VERIFY( m_classRemapTable.Insert(CName(TXT(_x)), CName(TXT(_y))) )

void CRTTISystem::LoadClassRemapTable()
{
	// Clear mapping
	m_classRemapTable.Clear();

	// Manual remaps
	REMAP( "Uint", "Uint32" );
	REMAP( "Int", "Int32" );
	REMAP( "CGameTime", "GameTime" );
	REMAP( "CGameTimeInterval", "GameTimeInterval" );
	REMAP( "CCharacterComponent", "CMovingAgentComponent" );
	REMAP( "float", "Float" );
	REMAP( "@*IMeshLODLevel", "@*CMeshLOD" );
	REMAP( "*IMeshLODLevel", "*CMeshLOD" );
	REMAP( "CMeshLODLevelCustomMesh", "CMeshLOD" );
	REMAP( "CMeshLODLevelExplicitMesh", "CMeshLOD" );
	REMAP( "CMeshLODLevelBaseMesh", "CMeshLOD" );	
	REMAP( "CMeshLODLevelHide", "CMeshLOD" );	
	REMAP( "CSoundComponent", "CWayPointComponent" );
	REMAP( "CEffectComponent", "CWayPointComponent" );
	REMAP( "CEffectMeshComponent", "CMeshComponent" );
	REMAP( "CEnvironmentComponent", "CSpriteComponent" );
	REMAP( "CDoorAnimatedComponent", "CAnimatedComponent" );
	REMAP( "CExternalPhysicsSystemComponent", "CPhysicsSystemComponent" );
	REMAP( "CEffectDummyPoint", "CEffectDummyComponent" );
	REMAP( "CWitcherWorld", "CGameWorld" );
	REMAP( "CJournalGraphSocket", "CQuestGraphSocket" );
	REMAP( "CEnvAmbientOnlyParameters", "CEnvReflectionProbesGenParameters" );
	REMAP( "CEnvSSAOParameters", "CEnvNVSSAOParameters" );
//	REMAP( "CDoorSaveable", "CDoor" );

	//REMAP( "*CTagList", "TagList" );

	// We cannot use file until we solve the build problem...
/*	// Parse file
	String str;
	String fileName = GFileManager->GetBaseDirectory() + TXT("classes.ini");
	if ( GFileManager->LoadFileToString( fileName, str, true ) )
	{
		const Char* txt = str.AsChar();
		while ( *txt )
		{
			// Load class name
			String className, newClassName;
			if ( !GParseWhitespaces( txt ) )
			{
				break;
			}
			if ( !GParseString( txt, className ) )
			{
				WARN_CORE( TXT("Error in classes.ini near '%ls'. Repair or else you'r doomed !"), className.AsChar() );
				break;
			}
			if ( !GParseKeyword( txt, TXT("=") ) )
			{
				WARN_CORE( TXT("Error in classes.ini near '%ls'. Repair or else you'r doomed !"), className.AsChar() );
				break;
			}
			if ( !GParseString( txt, newClassName ) )
			{
				WARN_CORE( TXT("Error in classes.ini near '%ls'. Repair or else you'r doomed !"), className.AsChar() );
				break;
			}
			
			// Define new entry
			if ( !className.Empty() && !newClassName.Empty() && className != newClassName )
			{
				m_classRemapTable[ CName( className.AsChar() ) ] = CName( newClassName.AsChar() );
			}
		}

		// Show stats
		LOG_CORE( TXT("%i entries in class remap table"), m_classRemapTable.Size() );
	}
	else
	{
		LOG_CORE( TXT("Class remap table not loaded" ) );
	}*/
}

Bool GProfileFunctionCalls = false;

void CRTTISystem::BeginFunctionProfiling()
{
	ASSERT( !GProfileFunctionCalls );
	if ( !GProfileFunctionCalls )
	{
		// Set flag
		LOG_CORE( TXT("Started function profiling") );
		GProfileFunctionCalls = true;

		// Get functions
		TDynArray< CFunction* > functions;
		EnumFunctions( functions );

		// Extract profiling data
		for ( Uint32 i=0; i<functions.Size(); i++ )
		{
			FuncPerfData* perfData = functions[i]->m_perfData;
			if ( perfData )
			{
				functions[i]->m_perfData = NULL;
				delete perfData;				
			}
		}
	}
}

void CRTTISystem::EndFunctionProfiling( TDynArray< FuncPerfData* >& data )
{
	ASSERT( GProfileFunctionCalls );
	if ( GProfileFunctionCalls )
	{
		// Get functions
		TDynArray< CFunction* > functions;
		EnumFunctions( functions );

		// Extract profiling data
		for ( Uint32 i=0; i<functions.Size(); i++ )
		{
			FuncPerfData* perfData = functions[i]->m_perfData;
			if ( perfData )
			{
				data.PushBack( perfData );
				functions[i]->m_perfData = NULL;
			}
		}

		// Reset flag
		LOG_CORE( TXT("Finished function profiling, %i functions analyzed"), data.Size() );
		GProfileFunctionCalls = false;
	}

}

void CRTTISystem::RecacheClassProperties()
{
	for ( auto i=m_types.Begin(); i != m_types.End(); ++i )
	{
		IRTTIType* type = i->m_second;
		if ( type->GetType() == RT_Class )
		{
			CClass* classType = static_cast< CClass* >( type );
			classType->RecalculateCachedProperties( true );
			classType->RecalculateGCCachedProperties();
			classType->RecalculateCachedScriptPropertiesToDestroy();
			classType->RecalculateFunctions();
		}
	}

}

struct ClassFilter
{
	static CClass* baseClass;
	static Bool Filter( CClass * c )
	{
		if ( c->HasBaseClass() )
		{
			if ( c->GetBaseClass() == baseClass )
			{
				return true;
			}
		}

		return false;
	};
};

CClass* ClassFilter::baseClass = NULL;

void CRTTISystem::DumpToLogFun( CClass * c, Int32 level )
{
	String text;
	for( Int32 i=0; i<level; i++ )
	{
		text.Append( TXT("  "), 2 );
	}
	text += c->GetName().AsString();
	LOG_CORE( text.AsChar() );
}

void CRTTISystem::DumpClassHierarchy( CClass* baseClass, void dumpFunction( CClass *, Int32 ) /*= DumpToLogFun*/, Int32 level /* =0 */ )
{
	dumpFunction( baseClass, level );
	
	TDynArray< CClass * > classes;
	ClassFilter::baseClass = baseClass;
	SRTTI::GetInstance().EnumClasses( baseClass, classes, ClassFilter::Filter, true );
	for( Uint32 i=0; i<classes.Size(); i++ )
	{
		DumpClassHierarchy( classes[i], dumpFunction, level + 1 );		
	}
}

struct ClassInfo
{
	CClass*					m_class;
	ClassInfo*				m_parent;
	TDynArray< ClassInfo* >	m_children;
	Int32					m_classIndex;
	Int32					m_lastClassIndex;

	ClassInfo()
		: m_class( nullptr )
		, m_parent( nullptr )
		, m_classIndex( -1 )
	{}

	void Setup( CClass* classInfo )
	{
		m_class = classInfo;
		classInfo->SetUserData( this );
	}

	void Link( ClassInfo* parentClassInfo )
	{
		m_parent = parentClassInfo;

		if ( parentClassInfo )
			parentClassInfo->m_children.PushBack( this );
	}

	void ReindexClass( Uint32& walkingIndex )
	{
		RED_FATAL_ASSERT( walkingIndex < 32767, "Reached maximum mappable class index" );
		m_classIndex = walkingIndex++;

		// visit child classes (O(N^2) right now)
		for ( ClassInfo* childClass : m_children )
			childClass->ReindexClass( walkingIndex );

		// remember last child index
		m_lastClassIndex = walkingIndex-1;
	}
};

void CRTTISystem::ReindexClasses()
{
	PC_SCOPE( ReindexClasses );

	CTimeCounter timer;

	// preallocate class info objects
	TDynArray< ClassInfo > classInfos;
	classInfos.Reserve( m_types.Size() );

	// Reset indices and create class map
	for ( auto i=m_types.Begin(); i != m_types.End(); ++i )
	{
		IRTTIType* type = i->m_second;
		if ( type->GetType() == RT_Class )
		{
			CClass* classType = static_cast< CClass* >( type );
			classType->m_classIndex = -1;
			classType->m_lastChildClassIndex = -1;

			// allocate class info object
			ClassInfo* newClassInfo = ::new ( classInfos ) ClassInfo();
			newClassInfo->Setup( classType );
		}
	}

	// Link class infos with parent classes
	for ( ClassInfo& info : classInfos )
	{
		CClass* classType = info.m_class;
		if ( classType->HasBaseClass() )
		{
			CClass* baseClassType = classType->GetBaseClass();
			RED_FATAL_ASSERT( baseClassType->GetUserData(), "Invalid base class" );

			ClassInfo* baseClassInfo = static_cast< ClassInfo* >( baseClassType->GetUserData() );
			info.Link( baseClassInfo );
		}
	}

	// Tweek: Assign the IReferencable to the index 1
	Uint32 classIndex = 1; // index 0 is preallocated to "NULL CLASS"
	static_cast< ClassInfo* >( ClassID< IReferencable >()->GetUserData() )->ReindexClass( classIndex );

	// Reindex reset of the classes only the parentless classes
	for ( auto i=m_types.Begin(); i != m_types.End(); ++i )
	{
		IRTTIType* type = i->m_second;
		if ( type->GetType() == RT_Class )
		{
			CClass* classType = static_cast< CClass* >( type );
			ClassInfo* classInfo = static_cast< ClassInfo* >( classType->GetUserData() );

			// Reindex class chain, start with base classes only
			if ( classInfo->m_classIndex == -1 && !classType->HasBaseClass() )
			{
				classInfo->ReindexClass( classIndex );
			}
		}
	}

	// Prepare new class map
	RED_FATAL_ASSERT( classIndex < m_indexedClasses.Capacity(), "There are to many classes in the RTTI. Increaset the static pool size." );
	m_indexedClasses.Resize( classIndex );
	Red::MemoryZero( m_indexedClasses.Data(), m_indexedClasses.DataSize() );

	// Update the class map
	for ( auto i=m_types.Begin(); i != m_types.End(); ++i )
	{
		IRTTIType* type = i->m_second;
		if ( type->GetType() == RT_Class )
		{
			CClass* classType = static_cast< CClass* >( type );

			// extract class info
			ClassInfo* classInfo = static_cast< ClassInfo* >( classType->GetUserData() );
			RED_FATAL_ASSERT( classInfo != nullptr, "Class '%ls' has no class info helper", classType->GetName().AsChar() );
			classType->SetUserData( nullptr );

			// copy data
			RED_FATAL_ASSERT( classInfo->m_classIndex != -1, "Class '%ls' was not indexed", classType->GetName().AsChar() );
			classType->m_classIndex = classInfo->m_classIndex;
			classType->m_lastChildClassIndex = classInfo->m_lastClassIndex;

			// put class in class list
			RED_FATAL_ASSERT( m_indexedClasses[ classType->m_classIndex ] == nullptr, "Class index %d was already used", classType->m_classIndex );
			m_indexedClasses[ classType->m_classIndex ] = classType;
		}
	}

	// let classes cache their class flags
	for ( auto i=m_types.Begin(); i != m_types.End(); ++i )
	{
		IRTTIType* type = i->m_second;
		if ( type->GetType() == RT_Class )
		{
			CClass* classType = static_cast< CClass* >( type );

			classType->CacheClassFlags();
		}
	}

	// Refresh cached properties
	for ( Uint32 i=0; i<m_indexedClasses.Size(); ++i )
	{
		CClass* indexedClass = m_indexedClasses[i];
		if ( indexedClass )
		{
			indexedClass->RecalculateCachedProperties();
		}
	}

	// Stats
	LOG_CORE( TXT("Reindexed %d classes in %1.3fms"), m_indexedClasses.Size(), timer.GetTimePeriodMS() );
}

static void DumpPropertyLayout( const CClass*, const CProperty* theProp, CXMLWriter& file )
{
	// Begin node
	file.BeginNode( TXT("property") );

	// Set the property name
	file.AttributeT<String>( TXT("Name"), theProp->GetName().AsString().AsChar() );
	file.AttributeT<String>( TXT("Type"), theProp->GetType()->GetName().AsString().AsChar() );
	file.AttributeT<Bool>( TXT("IsScripted"), theProp->IsScripted() );
	file.AttributeT<Bool>( TXT("IsNative"), theProp->IsNative() );
	file.AttributeT<Bool>( TXT("IsEditable"), theProp->IsEditable() );
	file.AttributeT<Bool>( TXT("IsExported"), theProp->IsExported() );
	file.AttributeT<Bool>( TXT("IsFuncLocal"), theProp->IsFuncLocal() );
	file.AttributeT<Bool>( TXT("IsInFunction"), theProp->IsInFunction() );
	file.AttributeT<Bool>( TXT("IsInlined"), theProp->IsInlined() );
	file.AttributeT<Bool>( TXT("IsSerializable"), theProp->IsSerializable() );
	file.AttributeT<Int32>( TXT("Offset"), (Int32)theProp->GetDataOffset() );
	file.AttributeT<Int32>( TXT("Size"), (Int32)theProp->GetType()->GetSize()  );

	// End node
	file.EndNode();
}

static void DumpFunctionLayout( const CClass*, const CFunction* theProp, CXMLWriter& file )
{
	// Begin node
	file.BeginNode( TXT("function") );

	// Set the property name
	file.AttributeT<String>( TXT("Name"), theProp->GetName().AsString().AsChar() );
	file.AttributeT<Bool>( TXT("IsNative"), theProp->IsNative() );
	file.AttributeT<Uint32>( TXT("StackSize"), theProp->GetStackSize() );
	if ( theProp->GetReturnValue() != NULL )
	{
		file.AttributeT<String>( TXT("RetType"), theProp->GetReturnValue()->GetType()->GetName().AsString().AsChar() );
	}

	// params
	file.BeginNode( TXT("params") );
	for ( Uint32 i=0; i<theProp->GetNumParameters(); ++i )
	{
		DumpPropertyLayout( NULL, theProp->GetParameter(i), file );
	}
	file.EndNode();

	// locals
	file.BeginNode( TXT("locals") );
	for ( Uint32 i=0; i<theProp->GetNumLocals(); ++i )
	{
		DumpPropertyLayout( NULL, theProp->GetLocal(i), file );
	}
	file.EndNode();

	// loaded code
	if ( !theProp->IsNative() && theProp->GetCode().GetCode() != NULL )
	{
		const Uint32 len = (const Uint32)( theProp->GetCode().GetCodeEnd() - theProp->GetCode().GetCode() );
		file.BeginNode( TXT("code") );
		file.AttributeT<Uint32>( TXT("Size"), len );
		file.EndNode();
	}

	// End node
	file.EndNode();
}

static Uint32 DumpClassLayout( const CClass* theClass, CXMLWriter& file )
{
	// Begin node
	file.BeginNode( TXT("class") );

	// Set the class name
	file.AttributeT<String>( TXT("Name"), theClass->GetName().AsString().AsChar() );

	// Set the base class name
	if ( theClass->HasBaseClass() )
	{
		file.AttributeT<String>( TXT("BaseClass"), theClass->GetBaseClass()->GetName().AsString().AsChar() );
	}

	// Is class native
	file.AttributeT<Bool>( TXT("IsNative"), theClass->IsNative() );

	// Is class abstract ?
	file.AttributeT<Bool>( TXT("IsAbstract"), theClass->IsAbstract() );

	// Size of class
	file.AttributeT<Uint32>( TXT("Size"), theClass->GetSize() );

	// Size of scripted crap
	file.AttributeT<Uint32>( TXT("ScriptedSize"), theClass->GetScriptDataSize() );

	// Alignment
	file.AttributeT<Uint32>( TXT("Alignment"), theClass->GetAlignment() );
	file.AttributeT<Uint32>( TXT("ScriptAlignment"), theClass->CalcScriptClassAlignment() );

	// Properties
	TDynArray< CProperty* > properties;
	theClass->GetProperties( properties );

	// Save class
	LOG_CORE( TXT("Dumping class '%ls': %i properties"), theClass->GetName().AsString().AsChar(), properties.Size() );

	// Save properties
	for ( Uint32 i=0; i<properties.Size(); i++ )
	{
		CProperty* theProp = properties[i];
		DumpPropertyLayout( theClass, theProp, file );
	}

	// Save functions
	const auto& functions = theClass->GetLocalFunctions();
	for ( CFunction* theFunc : functions )
	{
		DumpFunctionLayout( theClass, theFunc, file );
	}

	// End node
	file.EndNode();

	// Return number of written properties
	return properties.Size();
}

static bool ShouldSkipClass( const CClass* theClass )
{
	// Some classes are used in editor only
	if ( theClass->IsA< IImporter >() ) return true;
	if ( theClass->IsA< IExporter >() ) return true;
	if ( theClass->IsA< IFactory >() ) return true;
#ifndef NO_RESOURCE_COOKING
	if ( theClass->IsA< ICooker >() ) return true;
#endif
	if ( theClass->IsA< ICommandlet >() ) return true;

	// Based 
	const CClass* cur = theClass;
	while ( cur )
	{
		if ( cur->IsEditorOnly() )
		{
			return true;
		}

		if ( cur->HasBaseClass() )
		{
			cur = cur->GetBaseClass();
		}
		else
		{
			cur = NULL;
		}
	}

	// Do not skip
	return false;
}

static int CompareClassName( const void* dataA, const void* dataB )
{
	CClass* classA = * ( CClass** ) dataA;
	CClass* classB = * ( CClass** ) dataB;

	// Compare by name
	RED_MESSAGE( "CompareClassName -> is there any need to do a case insensitive comparison, or can we compare the cnames directly?" );
	return Red::System::StringCompareNoCase( classA->GetName().AsString().AsChar(), classB->GetName().AsString().AsChar() );
}

bool CRTTISystem::DumpRTTILayout( const Char* absoluteFileName )
{
	CXMLWriter fileWriter;

	// Begin node
	fileWriter.BeginNode( TXT("layout") );

	// Dump RTTI layout of all classes
	Uint32 numClassesWritten = 0;
	Uint32 numPropertiesWritten = 0;
	TDynArray< CClass* > classesToSave;
	for ( auto it=m_types.Begin(); it!=m_types.End(); ++it )
	{
		IRTTIType* rttiType = it->m_second;
		if ( rttiType->GetType() == RT_Class )
		{
			CClass* classType = static_cast< CClass* >( rttiType );

			// Skip abstract classes
			/*if ( classType->IsAbstract() )
			{
				LOG_CORE( TXT("Dumping of class '%ls' skipped: abstract"), classType->GetName().AsString().AsChar() );
				continue;
			}*/

			// Skip class
			if ( ShouldSkipClass( classType ) )
			{
				LOG_CORE( TXT("Dumping of class '%ls' skipped: editor only"), classType->GetName().AsString().AsChar() );
				continue;
			}

			// Remember
			classesToSave.PushBack( classType );
		}
	}

	// Sort
	qsort( classesToSave.TypedData(), classesToSave.Size(), sizeof( CClass* ), &CompareClassName );

	// Dump classes, sorted
	for ( Uint32 i=0; i<classesToSave.Size(); i++ )
	{
		CClass* classType = classesToSave[i];

		// Dump class
		numPropertiesWritten += DumpClassLayout( classType, fileWriter );
		numClassesWritten += 1;
	}

	// Count
	fileWriter.AttributeT( TXT("NumClasses"), (Int32)numClassesWritten );
	fileWriter.AttributeT( TXT("NumProperties"), (Int32)numPropertiesWritten );
	fileWriter.AttributeT( TXT("NumTypes"), (Int32)m_types.Size() );

	// End node
	fileWriter.EndNode();

	// Finalize file
	fileWriter.Flush();

	// Save
	return GFileManager->SaveStringToFile( absoluteFileName, fileWriter.GetContent() );
}

bool CRTTISystem::CheckRTTILayout( const Char* absoluteFileName )
{
	// Load string
	String xmlContent;
	if ( !GFileManager->LoadFileToString(absoluteFileName, xmlContent, true ) )
	{
		WARN_CORE( TXT("No RTTI layout file found") );
		return false;
	}

	// Initialize file
	CXMLReader fileReader( xmlContent );

	// Process layout
	Uint32 numErrors = 0;
	Uint32 numWarnings = 0;
	if ( fileReader.BeginNode( TXT("layout") ) )
	{
		// Process classes
		while ( fileReader.BeginNode( TXT("class") ) )
		{
			// Find class
			String className;
			fileReader.Attribute( TXT("Name"), className );
			CClass* testedClass = SRTTI::GetInstance().FindClass( CName( className.AsChar() ) );
			if ( !testedClass )
			{
				// Class was not found
				numWarnings++;
				WARN_CORE( TXT("RTTILayout: class '%ls' not found"), className.AsChar() ); 
				fileReader.EndNode();
				continue;
			}

			// Check base class
			String baseClassName;
			fileReader.Attribute( TXT("BaseClass"), baseClassName );
			if ( testedClass->HasBaseClass() )	
			{
				CClass* baseClass = testedClass->GetBaseClass();
				if ( baseClass->GetName() != baseClassName.AsChar() )
				{
					ERR_CORE( TXT("RTTILayout: class '%ls' has different base class '%ls' than it was saved with '%ls'"), className.AsChar(), baseClassName.AsChar(),  baseClass->GetName().AsString().AsChar() );
					numErrors++;
					fileReader.EndNode();
					continue;

				}
			}

			// Check native flag
			const Bool isNative = fileReader.AttributeTT( TXT("IsNative"), false );
			if ( testedClass->IsNative() != isNative )
			{
				ERR_CORE( TXT("RTTILayout: class '%ls' native flag changed"), className.AsChar() );
				numErrors++;
			}
				
			// Check abstract flag
			const Bool isAbstract = fileReader.AttributeTT( TXT("IsAbstract"), false );
			if ( testedClass->IsAbstract() != isAbstract )
			{
				ERR_CORE( TXT("RTTILayout: class '%ls' abstract flag changed"), className.AsChar() );
				numErrors++;
			}

			// Get class size
			Uint32 classSize = (Uint32)fileReader.AttributeTT<Int32>( TXT("Size"), 0 );
			if ( testedClass->GetSize() != classSize )
			{
				ERR_CORE( TXT("RTTILayout: class '%ls' has different size %i than it was saved in layout (%i)"), className.AsChar(), testedClass->GetSize(), classSize );
				numErrors++;
			}

			// Get class scripted size
			Uint32 scriptClassSize = (Uint32)fileReader.AttributeTT<Int32>( TXT("ScriptedSize"), 0 );
			if ( testedClass->GetScriptDataSize() != scriptClassSize )
			{
				ERR_CORE( TXT("RTTILayout: class '%ls' has different scripted size %i than it was saved in layout (%i)"), className.AsChar(), testedClass->GetScriptDataSize(), scriptClassSize );
				numErrors++;
			}

			// Process properties
			while ( fileReader.BeginNode( TXT("property") ) )
			{				
				String propNameStr;
				fileReader.Attribute( TXT("Name"), propNameStr );

				// Find property
				CName propertyName( propNameStr.AsChar() );
				CProperty* prop = testedClass->FindProperty( propertyName );
				if ( !prop )
				{
					WARN_CORE( TXT("RTTILayout: property '%ls' not found in '%ls'"), propNameStr.AsChar(), className.AsChar() );
					fileReader.EndNode();
					continue;
				}

				// Check type
				String propTypeStr;
				fileReader.Attribute( TXT("Type"), propTypeStr );
				if ( prop->GetType()->GetName() != propTypeStr.AsChar() )
				{
					ERR_CORE( TXT("RTTILayout: property '%ls' in class '%ls' has different type '%ls' than it was saved in layout ('%ls') with"), propNameStr.AsChar(), className.AsChar(), prop->GetType()->GetName().AsString().AsChar(), propTypeStr.AsChar() );
					fileReader.EndNode();
					numErrors++;
					continue;
				}

				// Check scripted flag
				Bool isScripted = fileReader.AttributeTT< Bool >( TXT("IsScripted"), false );
				if ( prop->IsScripted() != isScripted )
				{
					ERR_CORE( TXT("RTTILayout: property '%ls' in class '%ls' has different scripted flag than it was saved in layout"), propNameStr.AsChar(), className.AsChar() );
					fileReader.EndNode();
					numErrors++;
					continue;
				}

				// Check native flag
				Bool isNative = fileReader.AttributeTT< Bool >( TXT("IsNative"), false );
				if ( prop->IsNative() != isNative )
				{
					ERR_CORE( TXT("RTTILayout: property '%ls' in class '%ls' has different native flag than it was saved in layout"), propNameStr.AsChar(), className.AsChar() );
					fileReader.EndNode();
					numErrors++;
					continue;
				}

				// Check offset
				Uint32 offset = (Int32) fileReader.AttributeTT< Int32 >( TXT("Offset"), 0 );
				if ( prop->GetDataOffset() != offset )
				{
					ERR_CORE( TXT("RTTILayout: property '%ls' in class '%ls' has different data offset %i than it was saved in layout (%i)"), propNameStr.AsChar(), className.AsChar(), prop->GetDataOffset(), offset );
					fileReader.EndNode();
					numErrors++;
					continue;
				}

				// Check size
				Uint32 size = (Int32) fileReader.AttributeTT< Int32 >( TXT("Size"), 0 );
				if ( prop->GetType()->GetSize() != size )
				{
					ERR_CORE( TXT("RTTILayout: property '%ls' in class '%ls' has different data size %i than it was saved in layout (%i)"), propNameStr.AsChar(), className.AsChar(), prop->GetType()->GetSize(), size );
					fileReader.EndNode();
					numErrors++;
					continue;
				}

				// End property block
				fileReader.EndNode();
			}

			// End class block
			fileReader.EndNode();
		}

		// End layout block
		fileReader.EndNode();
	}

	// Stats
	LOG_CORE( TXT("RTTI layout processed: %i error(s), %i warning(s)"), numErrors, numWarnings );
	return numErrors == 0;
}

Bool CRTTISystem::ValidateLayout() const
{
	Bool result = true;

	// Check all classes
	for ( auto it=m_types.Begin(); it!=m_types.End(); ++it )
	{
		IRTTIType* rttiType = it->m_second;
		if ( rttiType->GetType() == RT_Class )
		{
			CClass* classType = static_cast< CClass* >( rttiType );
			result |= classType->ValidateLayout();
		}
	}
	return result;
}

//---------------------------------------------------------------------------

struct EMemoryClassHashFunc
{
	static RED_INLINE Uint32 GetHash( EMemoryClass key )
	{
		return ( Uint32 ) key;
	}
};

class CRTTIObjectDumper : public IFile
{
private:
	struct DumpType;
	struct DumpObject;
	
	// keep synced with ERTTITypeType
	enum EDumpType
	{
		eDumpType_Simple,
		eDumpType_Enum,
		eDumpType_Class,
		eDumpType_Array,
		eDumpType_StaticArray,
		eDumpType_NativeArray,
		eDumpType_Pointer,
		eDumpType_Handle,
		eDumpType_SoftHandle,
		eDumpType_BitField,
		eDumpType_Void,
	};

	struct DumpMemoryBlock
	{
		Uint32		m_index;
		Uint64		m_storageOffset; // offset in file (in the memory block)
		Uint32		m_memoryClassName;
		Uint64		m_memoryPtr;		// saved as address
		Uint32		m_memorySize;

		friend IFile& operator<<( IFile& ar, DumpMemoryBlock& m )
		{
			ar << m.m_storageOffset;
			ar << m.m_memoryPtr;
			ar << m.m_memorySize;
			ar << m.m_memoryClassName;
			return ar;
		}
	};

	struct DumpProperty
	{
		Uint32		m_index;
		Uint32		m_name;
		Uint32		m_offset;
		Uint32		m_flags;		// 1 - scripted
		DumpType*	m_type;

		friend IFile& operator<<( IFile& ar, DumpProperty& m )
		{
			ar << m.m_name;
			ar << m.m_offset;
			ar << m.m_flags;
			ar << m.m_type->m_index;
			return ar;
		}
	};

	struct DumpType
	{
		Uint32						m_index;
		Uint32						m_name;
		Uint32						m_type;		// EDumpType
		DumpType*					m_innerType;
		DumpType*					m_parentType;
		Uint32						m_firstPropertyRef;
		Uint32						m_numPropertyRef;

		friend IFile& operator<<( IFile& ar, DumpType& m )
		{
			Uint32 zero = 0;

			ar << m.m_name;
			ar << m.m_type;		// EDumpType
			ar << (m.m_innerType ? m.m_innerType->m_index : zero);
			ar << (m.m_parentType ? m.m_parentType ->m_index : zero);
			ar << m.m_firstPropertyRef;
			ar << m.m_numPropertyRef;
			return ar;
		}
	};

	struct DumpArray
	{
		Uint32				m_index;
		DumpType*			m_type;
		DumpMemoryBlock*	m_memory;
		Uint32				m_arraySize;
		Uint32				m_arrayCapacity;
		Uint32				m_firstArrayRef;
		Uint32				m_numArrayRef;
		Uint32				m_firstObjectRef;
		Uint32				m_numObjectRef;

		friend IFile& operator<<( IFile& ar, DumpArray& m )
		{
			ar << m.m_type->m_index;
			ar << m.m_memory->m_index;
			ar << m.m_arraySize;
			ar << m.m_arrayCapacity;
			ar << m.m_firstArrayRef;
			ar << m.m_numArrayRef;
			ar << m.m_firstObjectRef;
			ar << m.m_numObjectRef;
			return ar;
		}
	};

	struct DumpObject
	{
		Uint32				m_index;
		Uint32				m_flags; // 1 - root
		DumpType*			m_type;
		DumpMemoryBlock*	m_memory;
		DumpMemoryBlock*	m_scriptMemory;
		Uint32				m_firstArrayRef;
		Uint32				m_numArrayRef;
		Uint32				m_firstObjectRef;
		Uint32				m_numObjectRef;

		friend IFile& operator<<( IFile& ar, DumpObject& m )
		{
			Uint32 zero = 0;

			ar << m.m_flags;
			ar << m.m_type->m_index;
			ar << (m.m_memory ? m.m_memory->m_index : zero);
			ar << (m.m_scriptMemory ? m.m_scriptMemory->m_index : zero);
			ar << m.m_firstArrayRef;
			ar << m.m_numArrayRef;
			ar << m.m_firstObjectRef;
			ar << m.m_numObjectRef;
			return ar;
		}
	};

	TDynArray< DumpMemoryBlock* >				m_memory;
	THashMap< const void*, DumpMemoryBlock* >	m_memoryMap; // by pointer to memory block

	TDynArray< DumpArray* >						m_arrays;
	THashMap< const void*, DumpArray* >			m_arrayMap; // by pointer to array

	TDynArray< DumpObject* >					m_objects;
	THashMap< const void*, DumpObject* >		m_objectMap;	// by pointer to object

	TDynArray< DumpProperty* >					m_properties;
	THashMap< const CProperty*, DumpProperty* >	m_propertiesMap;

	TDynArray< DumpProperty* >					m_propertiesRef;
	TDynArray< DumpObject* >					m_objectRef;
	TDynArray< DumpArray* >						m_arrayRef;

	TDynArray< DumpType* >						m_types;
	THashMap< const IRTTIType*, DumpType* >		m_typeMap;

	TDynArray< CName >							m_names;
	THashMap< CName, Uint32 >					m_nameMap;

	THashMap< EMemoryClass, Uint32, EMemoryClassHashFunc >			m_memoryClassMap; // maped to CNames

	TDynArray< DumpObject* >					m_objectStack; // stack of dumped objects (for native cross-referencing)

private:
	struct SaveChunk
	{
		Uint64		m_offset;
		Uint64		m_size;
		Uint32		m_count;

		friend IFile& operator<<( IFile& ar, SaveChunk& h )
		{
			ar << h.m_offset;
			ar << h.m_size;
			ar << h.m_count;
			return ar;
		}

		void Start( IFile& ar, Uint32 count )
		{
			m_offset = ar.GetOffset();
			m_count = count;
		}

		void Finish( IFile& ar )
		{
			m_size = ar.GetOffset() - m_offset;
		}
	};

	struct SaveChunkAuto
	{
		IFile*			m_file;
		SaveChunk*		m_chunk;

		SaveChunkAuto( IFile& file, SaveChunk& chunk, const Uint32 count )
			: m_file( &file )
			, m_chunk( &chunk )
		{
			m_chunk->Start( *m_file, count );
		}

		~SaveChunkAuto()
		{
			m_chunk->Finish( *m_file );
		}
	};

	struct SaveHeader
	{
		Uint32		m_magic;
		Uint32		m_version;
		SaveChunk	m_names;
		SaveChunk	m_types;
		SaveChunk	m_properties;
		SaveChunk	m_propertyRefs;
		SaveChunk	m_objects;
		SaveChunk	m_objectRefs;
		SaveChunk	m_arrays;
		SaveChunk	m_arrayRefs;
		SaveChunk	m_memBlocks;
		SaveChunk	m_memData;

		SaveHeader()
		{
			Red::MemoryZero( this, sizeof(SaveHeader) );

			m_magic = 'PMUD'; // DUMP
			m_version = 1;
		}

		friend IFile& operator<<( IFile& ar, SaveHeader& h )
		{
			ar << h.m_magic;
			ar << h.m_version;
			ar << h.m_names;
			ar << h.m_types;
			ar << h.m_properties;
			ar << h.m_propertyRefs;
			ar << h.m_objects;
			ar << h.m_objectRefs;
			ar << h.m_arrays;
			ar << h.m_arrayRefs;
			ar << h.m_memBlocks;
			ar << h.m_memData;
			return ar;
		}
	};

public:
	CRTTIObjectDumper()
		: IFile( FF_Writer | FF_GarbageCollector )
	{
		// NULL name
		m_names.Reserve( 16000 );
		m_names.PushBack( CName() );
		m_nameMap.Insert( CName(), 0 );

		// NULL type
		m_types.Reserve( 16000 );
		m_types.PushBack( NULL );
		m_typeMap.Insert( NULL, NULL );

		// NULL object
		m_objects.Reserve( 16000 );
		m_objects.PushBack( NULL ); 
		m_objectMap.Insert( NULL, NULL );

		// NULL memory block
		m_memory.Reserve( 100000 );
		m_memory.PushBack( NULL ); 
		m_memoryMap.Insert( NULL, NULL );

		// References
		m_arrays.Reserve( 100000 );
		m_arrayRef.Reserve( 100000 );
		m_objectRef.Reserve( 100000 );
		m_propertiesRef.Reserve( 10000 );
	}

	~CRTTIObjectDumper()
	{
		m_memory.ClearPtr();
		m_arrays.ClearPtr();
		m_objects.ClearPtr();
		m_properties.ClearPtr();
		m_types.ClearPtr();
	}

	Uint32 MapName( const CName& name )
	{
		Uint32 index = 0;
		if ( !m_nameMap.Find( name, index ) )
		{
			index = m_names.Size();
			m_names.PushBack( name );
			m_nameMap.Insert( name, index );
		}
		return index;
	}

	DumpProperty* MapProperty( const CProperty* prop )
	{
		ASSERT( NULL != prop );

		DumpProperty* data = NULL;
		if ( !m_propertiesMap.Find( prop, data ) )
		{
			// create new property data
			data = new DumpProperty;
			data->m_index = m_properties.Size();
			m_properties.PushBack( data );
			m_propertiesMap.Insert( prop, data );
			data->m_name = MapName( prop->GetName() );
			data->m_flags = 0;
			data->m_offset = prop->GetDataOffset();

			// setup flags (manual)
			if ( prop->IsScripted() )
			{
				data->m_flags |= 1;
			}

			// map property type
			data->m_type = MapType( prop->GetType() );
		}
		return data;
	}

	Uint32 MapPropertyRef( const CProperty* prop )
	{
		// get property index
		DumpProperty* propData = MapProperty( prop );

		// insert to property reference table
		const Uint32 index = m_propertiesRef.Size();
		m_propertiesRef.PushBack( propData );
		return index;
	}

	DumpType* MapType( const IRTTIType* type )
	{
		ASSERT( NULL != type ); // we never directly map NULL type

		DumpType* data = NULL;
		if ( !m_typeMap.Find( type, data ) )
		{
			// create new type defintion
			data = new DumpType();
			data->m_index = m_types.Size();
			m_types.PushBack( data );
			m_typeMap.Insert( type, data );

			// copy some important stuff
			data->m_name = MapName( type->GetName() );
			data->m_type = (Uint32) type->GetType();
			data->m_innerType = NULL;
			data->m_parentType = NULL;

			// classes
			if ( type->GetType() == RT_Class )
			{
				// get parent class
				const CClass* thisClass = static_cast< const CClass* >( type );
				const CClass* baseClass = thisClass->HasBaseClass() ? thisClass->GetBaseClass() : NULL;

				// map the base class
				if ( NULL != baseClass )
				{
					data->m_parentType = MapType( baseClass );
				}

				// map the class local properties
				data->m_firstPropertyRef = m_propertiesRef.Size();
				const auto& props = thisClass->GetLocalProperties();
				for ( Uint32 i=0; i<props.Size(); ++i )
				{
					const CProperty* prop = props[i];
					MapPropertyRef( prop );
				}
				data->m_numPropertyRef = m_propertiesRef.Size() - data->m_firstPropertyRef;
			}
			else if ( type->GetType() == RT_Array || type->GetType() == RT_StaticArray || type->GetType() == RT_NativeArray )
			{
				const IRTTIBaseArrayType* arrayType = static_cast< const IRTTIBaseArrayType* >( type );
				data->m_innerType = MapType( arrayType->ArrayGetInnerType() );
			}
			else if ( type->GetType() == RT_Pointer || type->GetType() == RT_Handle )
			{
				const IRTTIPointerTypeBase* pointerType = static_cast< const IRTTIPointerTypeBase* >( type );
				data->m_innerType = MapType( pointerType->GetPointedType() );
			}
		}

		return data;
	}

	Uint32 MapMemoryClass( const EMemoryClass memoryClass )
	{
		Uint32 index = 0;
		if ( !m_memoryClassMap.Find( memoryClass, index ) )
		{
			// get memory class name
			const AnsiChar* nameStr = Memory::GetMemoryClassName( memoryClass );
			const CName name( ANSI_TO_UNICODE( nameStr ) );

			// map as CName
			index = MapName( name );
		}

		return index;
	}

	DumpMemoryBlock* MapMemory( const void* memory, const Uint32 size, const EMemoryClass memoryClass )
	{
		ASSERT( memory != NULL ); // we never directly map NULL memory

		DumpMemoryBlock* data = NULL;
		if ( !m_memoryMap.Find( memory, data ) )
		{
			data = new DumpMemoryBlock();
			data->m_index = m_memory.Size();
			data->m_memoryPtr = (Uint64)memory;
			data->m_memorySize = size;
			data->m_memoryClassName = MapMemoryClass( memoryClass );
			m_memory.PushBack( data );
			m_memoryMap.Insert( memory, data );
		}
		return data;
	}

	DumpObject* MapPointer( const class CClass* pointerClass, void* pointer, const Uint32 customFlags = 0);

	DumpArray* MapArray( const CBaseArray* arrayData, const CRTTIArrayType* arrayType )
	{
		ASSERT( arrayData != NULL ); // we never directly map NULL array

		DumpArray* data = NULL;
		if ( !m_arrayMap.Find( arrayData, data ) )
		{
			data = new DumpArray();
			data->m_index = m_arrays.Size();
			m_arrays.PushBack( data );
			m_arrayMap.Insert( arrayData, data );

			data->m_arraySize = (const Uint32)arrayData->Size();
			data->m_memory = MapMemory( arrayData->Data(), (const Uint32)( arrayData->Capacity( arrayType->GetInnerType()->GetSize() ) ), arrayType->GetMemoryClass() );
			data->m_type = MapType( arrayType->GetInnerType() ); // inner type is mapped here, not the array type

			data->m_firstArrayRef = 0;
			data->m_numArrayRef = 0;
			data->m_firstObjectRef = 0;
			data->m_numObjectRef = 0;

			// special cases - dynamic array of dynamic arrays
			if ( arrayType->GetInnerType()->GetType() == RT_Array )
			{
				data->m_firstArrayRef = m_arrayRef.Size();

				const Uint32 numElements = arrayData->Size();
				for ( Uint32 i=0; i<numElements; ++i)
				{
					const CRTTIArrayType* subArrayType = static_cast< const CRTTIArrayType* >( arrayType->GetInnerType() );
					const CBaseArray* subArray = static_cast< const CBaseArray* >( arrayType->GetArrayElement( arrayData, i ) );
					m_arrayRef.PushBack( MapArray( subArray, subArrayType ) );
				}

				data->m_numArrayRef = m_arrayRef.Size() - data->m_firstArrayRef;
			}

			// special case - array of pointers
			if ( arrayType->GetInnerType()->GetType() == RT_Pointer || arrayType->GetInnerType()->GetType() == RT_Handle )
			{
				const IRTTIPointerTypeBase* pointerType = static_cast< const IRTTIPointerTypeBase* >( arrayType->GetInnerType() );

				// only pointers to classes are supported here (we don't know the size :()
				// if the memory allocator would support the GetPointerSize() we could extend the functionality
				if ( pointerType->GetPointedType() && pointerType->GetPointedType()->GetType() == RT_Class )
				{
					data->m_firstObjectRef = m_objectRef.Size();

					const Uint32 numElements = arrayData->Size();
					for ( Uint32 i=0; i<numElements; ++i)
					{
						const void* pointerData = static_cast< const void* >( arrayType->GetArrayElement( arrayData, i ) );
						const CPointer pointer = pointerType->GetPointer( pointerData );
						m_objectRef.PushBack( MapPointer( pointer.GetClass(), pointer.GetPointer() ) );
					}

					data->m_numObjectRef = m_objectRef.Size() - data->m_firstObjectRef;
				}
			}
		}

		return data;
	}

	virtual void SerializeName( class CName& name )
	{
		MapName( name );
	}

	virtual void SerializePointer( const class CClass* pointerClass, void*& pointer )
	{
		if ( pointer != NULL )
		{
			DumpObject* data = MapPointer( pointerClass, pointer );

			if ( m_objectStack.Size() > 0 )
			{
				m_objectRef.PushBack( data );
			}
		}
	}

	virtual void SerializeTypeReference( class IRTTIType*& type )
	{
		MapType( type );
	}

protected:
	// IFile interface
	virtual void Serialize( void* , size_t ) { };
	virtual Uint64 GetOffset() const { return 0; }
	virtual Uint64 GetSize() const { return 0; }
	virtual void Seek( Int64 ) {};

public:
	// Save dump output to file
	void Save( IFile& outFile )
	{
		CTimeCounter timer;

		// prepare header
		SaveHeader header;
		outFile << header;

		// save names - ANSI style, very manual :)
		{
			SaveChunkAuto chunk( outFile, header.m_names, m_names.Size() );

			// do not store NULL name
			for ( Uint32 i=1; i<m_names.Size(); ++i )
			{
				const CName& name = m_names[i];
				const Char* text = name.AsChar();

				Uint32 length = (const Uint32)(text ? Red::StringLength( text ) : 0);
				outFile << length;

				for ( Uint32 i=0; i<length; ++i )
				{
					AnsiChar ch = (AnsiChar)( text[i] <= 127 ? text[i] : '_' );
					outFile << ch;
				}
			}
		}

		// save types
		{
			SaveChunkAuto chunk( outFile, header.m_types, m_types.Size() );

			// do not store NULL type
			for ( Uint32 i=1; i<m_types.Size(); ++i )
			{
				DumpType& typeData = *m_types[i];
				outFile << typeData;
			}
		}

		// save properties
		{
			SaveChunkAuto chunk( outFile, header.m_properties, m_properties.Size() );

			for ( Uint32 i=0; i<m_properties.Size(); ++i )
			{
				DumpProperty& data = *m_properties[i];
				outFile << data;
			}
		}

		// save objects
		{
			SaveChunkAuto chunk( outFile, header.m_objects, m_objects.Size() );

			// do not save NULL object
			for ( Uint32 i=1; i<m_objects.Size(); ++i )
			{
				DumpObject& data = *m_objects[i];
				outFile << data;
			}
		}

		// save arrays
		{
			SaveChunkAuto chunk( outFile, header.m_arrays, m_arrays.Size() );

			for ( Uint32 i=0; i<m_arrays.Size(); ++i )
			{
				DumpArray& data = *m_arrays[i];
				outFile << data;
			}
		}

		// save property references
		{
			SaveChunkAuto chunk( outFile, header.m_propertyRefs, m_propertiesRef.Size() );

			for ( Uint32 i=0; i<m_propertiesRef.Size(); ++i )
			{
				DumpProperty* data = m_propertiesRef[i];

				Uint32 zero = 0;
				outFile << (data ? data->m_index : zero);
			}
		}

		// save array references
		{
			SaveChunkAuto chunk( outFile, header.m_arrayRefs, m_arrayRef.Size() );

			for ( Uint32 i=0; i<m_arrayRef.Size(); ++i )
			{
				DumpArray* data = m_arrayRef[i];

				Uint32 zero = 0;
				outFile << (data ? data->m_index : zero);
			}
		}

		// save object references
		{
			SaveChunkAuto chunk( outFile, header.m_objectRefs, m_objectRef.Size() );

			for ( Uint32 i=0; i<m_objectRef.Size(); ++i )
			{
				DumpObject* data = m_objectRef[i];

				Uint32 zero = 0;
				outFile << (data ? data->m_index : zero);
			}
		}

		// save memory blocks data
		Uint64 storageMemoryOffset = 0;
		{
			SaveChunkAuto chunk( outFile, header.m_memBlocks, m_memory.Size() );

			// do not store NULL memory block
			for ( Uint32 i=1; i<m_memory.Size(); ++i )
			{
				DumpMemoryBlock& data = *m_memory[i];

				data.m_storageOffset = storageMemoryOffset;
				storageMemoryOffset += data.m_memorySize;

				outFile << data;
			}
		}

		// save memory dump
		{
			SaveChunkAuto chunk( outFile, header.m_memData, 0 );

			// do not store NULL memory block
			for ( Uint32 i=1; i<m_memory.Size(); ++i )
			{
				DumpMemoryBlock& data = *m_memory[i];

				ASSERT( outFile.GetOffset() == ( header.m_memData.m_offset + data.m_storageOffset ) );
				outFile.Serialize( (void*)data.m_memoryPtr, data.m_memorySize );
			}
		}

		// Refresh header with valid data values
		outFile.Seek( 0 );
		outFile << header;

		// Dump stats
		LOG_CORE( TXT("Object memory dump saved in %1.2fs (%1.2fMB)"), timer.GetTimePeriod(), outFile.GetSize() / (1024.0f*1024.0f) );
		LOG_CORE( TXT(" Names: %d"), header.m_names.m_count );
		LOG_CORE( TXT(" Types: %d"), header.m_types.m_count );
		LOG_CORE( TXT(" Properties: %d"), header.m_properties.m_count );
		LOG_CORE( TXT(" Objects: %d"), header.m_objects.m_count );
		LOG_CORE( TXT(" Arrays: %d"), header.m_arrays.m_count );
		LOG_CORE( TXT(" PropertyRefs: %d"), header.m_propertyRefs.m_count );
		LOG_CORE( TXT(" ObjectRefs: %d"), header.m_objectRefs.m_count );
		LOG_CORE( TXT(" ArrayRefs: %d"), header.m_arrayRefs.m_count );
		LOG_CORE( TXT(" MemoryBlocks: %d"), header.m_memBlocks.m_count );
		LOG_CORE( TXT(" MemoryData: %d"), header.m_memData.m_size );
	}

	void MapRootObject( const CPointer& pointer )
	{
		if ( pointer.GetPointer() )
		{
			const Uint32 customFlags = 1;
			MapPointer( pointer.GetClass(), pointer.GetPointer(), customFlags );
		}
	}

	void MapObject( const CPointer& pointer )
	{
		if ( pointer.GetPointer() )
		{
			MapPointer( pointer.GetClass(), pointer.GetPointer(), 0 );
		}
	}
};

CRTTIObjectDumper::DumpObject* CRTTIObjectDumper::MapPointer( const class CClass* pointerClass, void* pointer, const Uint32 customFlags )
{	
	DumpObject* data = NULL;
	if ( !m_objectMap.Find( pointer, data ) )
	{
		data = new DumpObject();
		data->m_index = m_objects.Size();
		data->m_flags = 0 | customFlags;
		m_objects.PushBack( data );
		m_objectMap.Insert( pointer, data );

		// get object runtime type
		const CClass* runtimeClass = pointerClass;
		if ( pointerClass->IsSerializable() )
		{
			const ISerializable* object = static_cast< const ISerializable* >( pointer );
			runtimeClass = object->GetClass();
		}

		// map runtime type
		data->m_type = MapType( runtimeClass );

		// map object memory
		const Uint32 dataSize = runtimeClass->GetSize();
		data->m_memory = MapMemory( pointer, dataSize, runtimeClass->GetMemoryClass() );
		data->m_scriptMemory = NULL;

		// reset
		data->m_firstArrayRef = 0;
		data->m_numArrayRef = 0;
		data->m_firstObjectRef = 0;
		data->m_numObjectRef = 0;

		// IScriptedObjects have also some script data
		if ( runtimeClass->IsScriptable() )
		{
			const Uint32 scriptDataSize = runtimeClass->GetScriptDataSize();
			if ( scriptDataSize > 0 )
			{
				const IScriptable* object = static_cast< const IScriptable* >( pointer );
				data->m_scriptMemory = MapMemory( object->GetScriptPropertyData(), scriptDataSize, runtimeClass->GetMemoryClass() );
			}
		}

		// dump references to all dynamic arrays and their memory accessible via RTTI
		{
			data->m_firstArrayRef = m_arrayRef.Size();
				
			const auto& allProps = runtimeClass->GetCachedProperties();
			for ( Uint32 i=0; i<allProps.Size(); ++i )
			{
				const CProperty* prop = allProps[i];
				IRTTIType* propType = prop->GetType();
				if ( propType->GetType() == RT_Array )
				{
					// array 
					const CRTTIArrayType* arrayType = static_cast< const CRTTIArrayType* >( propType );
					const CBaseArray* arrayData = static_cast< const CBaseArray* >( prop->GetOffsetPtr( pointer ) );
					if ( arrayData && arrayData->Capacity(arrayType->GetInnerType()->GetSize() ) > 0 )
					{
						m_arrayRef.PushBack( MapArray( arrayData, arrayType ) );
					}
				}
			}

			data->m_numArrayRef = m_arrayRef.Size() - data->m_firstArrayRef;
		}

		// internal serialization
		if ( runtimeClass->IsSerializable() )
		{
			m_objectStack.PushBack( data );

			data->m_firstObjectRef = 0;

			ISerializable* object = static_cast< ISerializable* >( pointer );
			object->OnSerialize( *this );

			data->m_numObjectRef = m_objectRef.Size() - data->m_firstObjectRef;

			ASSERT( m_objectStack.Back() == data );
			m_objectStack.PopBack();
		}
	}
	else if ( data != NULL )
	{
		// merge custom flags
		data->m_flags |= customFlags;
	}

	return data;
}

class CRTTIResourceHandleValidator : public IFile
{
public:
	CRTTIResourceHandleValidator()
		: IFile( FF_Writer | FF_GarbageCollector )
	{
	}

	virtual void SerializePointer( const class CClass* pointerClass, void*& pointer )
	{
		if ( pointer != NULL )
		{
			if ( pointer && pointerClass->IsA< ISerializable >() )
			{
				ISerializable* actual = static_cast< ISerializable* >( pointer );

#ifndef RED_FINAL_BUILD
				if ( !CHandleSerializationMarker::st_isInHandleSerialization )
				{
					if ( actual->IsA< CResource >() )
					{
						bool reportError = true;

						// skip layers
						const CName className = actual->GetClass()->GetName();
						if ( className == TXT("CDynamicLayer") || className == TXT("CWorld") || className == TXT("CGameWorld") )
						{
							reportError = false;
						}

						// report the pointers to resources
						if ( reportError )
						{
							RED_LOG_ERROR( TXT("Trying to serialize pointer to resource '%ls' without a THandle"), 
								actual->GetClass()->GetName().AsChar() );
						}
					}
				}
#endif

				// visited ?
				AddObject( actual );
			}					
		}
	}

	void AddObject( ISerializable* obj )
	{
		if ( !m_visited.FindPtr( obj ) )
		{

			m_visited[ obj ] = 1;
			m_stack.PushBack( obj );
		}
	}

	void Process()
	{
		while ( !m_stack.Empty() )
		{
			TDynArray< ISerializable* > temp;
			m_stack.SwapWith( temp );

			for ( Uint32 i=0; i<temp.Size(); ++i )
			{
				ISerializable* a = temp[i];
				a->OnSerialize( *this );
			}
		}
	}

protected:
	// IFile interface
	virtual void Serialize( void* , size_t ) { };
	virtual Uint64 GetOffset() const { return 0; }
	virtual Uint64 GetSize() const { return 0; }
	virtual void Seek( Int64 ) {};
	 
private:
	THashMap< void*, Uint32 >		m_visited;
	TDynArray< ISerializable* >		m_stack;
};
			
//---------------------------------------------------------------------------

void CRTTISystem::DumpRTTIMemory( const Char* customFileName )
{
	CTimeCounter timer;

	RED_UNUSED( customFileName );

	// Validate RTTI handles
	{
		CRTTIResourceHandleValidator validator;

		// extract root objects
		for ( BaseObjectIterator it; it; ++it )
		{
			CObject* obj = (*it);
			if ( obj->IsInRootSet() )
			{
				validator.AddObject( obj );
			}
		}

		// process
		validator.Process();
	}

#pragma message( "FIX THIS SHIT OR DELETE IT!!!>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>")
#if 0
#ifdef _DEBUG
	// Grab root set and resource set (can partially overlap)
	CRTTIObjectDumper dumper;
	{
		TDynArray< CObject* > allObjects;
		SObjectsMap::GetInstance().GetAllObject( allObjects );

		for ( Uint32 i=0; i<allObjects.Size(); ++i )
		{
			CObject* obj = allObjects[i];
			if ( obj->IsInRootSet() )
			{
				dumper.MapRootObject( CPointer( obj ) );
			}
			else
			{
				dumper.MapObject( CPointer( obj ) );
			}
		}
	}
	
	// Format file path
	String filePath;
	filePath += GFileManager->GetBaseDirectory();
	filePath += TXT("objectdumps\\");
	filePath += TXT("ObjectDump_");
#ifdef RED_PLATFORM_WINPC
	if ( customFileName == NULL || customFileName[0] == 0 )
	{
		std::time_t now = std::time(NULL);
		std::tm * ptm = std::localtime(&now);

		wchar_t buffer[64];
		std::wcsftime( buffer, ARRAY_COUNT(buffer), TXT("%d_%m_%Y(%H_%M_%S)"), ptm );
		filePath += buffer;
	}
	else
	{
		filePath += customFileName;
	}
#endif
	filePath += TXT(".mem");

	// Create output file
	GFileManager->CreatePath( GFileManager->GetBaseDirectory() + TXT( "objectdumps\\" ) );
	IFile* outputFile = GFileManager->CreateFileWriter( filePath, FOF_AbsolutePath );
	if ( NULL != outputFile )
	{
		dumper.Save( *outputFile );
		delete outputFile;
	}

	// Stats
	LOG_CORE( TXT("Object memory dumped in %1.2fs"), timer.GetTimePeriod() );
#endif
#endif // if 0
}
