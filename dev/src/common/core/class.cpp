/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "serializable.h"
#include "defaultValue.h"
#include "softHandle.h"
#include "resource.h"
#include "fileSkipableBlock.h"
#include "serializationStreamData.h"
#include "fileSys.h"
#include "datetime.h"
#include "dataBuffer.h"
#include "sharedDataBuffer.h"

//#pragma optimize("",off)

//#define USE_SERIALIZATION_STREAM
//#define DUMP_SERIALIZATION_FILE_LAYOUT

namespace // CNAME_HACK:
{
	Bool LoadCNameSavedAsString( IFile& file, void* propData )
	{
		if ( !file.IsReader() )
			return false;

		String s;
		file << s;

		// chack consistency
		//for ( Uint32 i=0; i<s.GetLength(); ++i )
		//	if ( s[i] > 0xff || s[i] == 0 )
		//		return false;

		*( reinterpret_cast< CName* >( propData ) )	= CName( s );
		return true;
	}
}

CClass::CClass( const CName& name, Uint32 size, Uint32 flags )
	: m_name( name )
	, m_size( size )
	, m_alignment( 4 ) // default class alignment is 4 bytes, this affects only how the property of this structure type is packed inside other classes
	, m_scriptSize( 0 )
	, m_flags( flags )
	, m_stateMachineClass( NULL )
	, m_customSerializer( NULL )
	, m_arePropertiesCached( false )
	, m_areScriptPropertiesToDestroyCached( false )
	, m_needsCleaningWasChecked( false )
	, m_areGCPropertiesCached( false )
	, m_isSerializable( false )
	, m_isObject( false )
	, m_classIndex( -1 )
	, m_lastChildClassIndex( -1 )
{
}

CClass::~CClass()
{
}

void CClass::CacheClassFlags() const
{
	m_isObject = IsA< CObject >();
	m_isSerializable = m_isObject || IsA< ISerializable >();
	m_isScriptable = m_isObject || IsA< IScriptable >();
}

void CClass::ReuseScriptStub( Uint32 flags )
{
	ASSERT( !m_localFunctions.Size() );
	ASSERT( !m_localProperties.Size() );
	ASSERT( !m_defaultValues.Size() );
	ASSERT( m_baseClass.m_parentClass == nullptr );
	ASSERT( flags & CF_Scripted );

	m_flags = flags;
	m_scriptSize = 0;
	m_size = 0;
}

void CClass::AddDefaultValue( CDefaultValue* defaultValue )
{
	ASSERT( defaultValue );
	m_defaultValues.PushBack( defaultValue );
}

void CClass::ApplyDefaultValues( void* object, Bool createInlinedObjects, Bool applyBaseClassDefaults ) const
{
	if ( IsScriptable() )
	{
		// Use specified memory as a owner in case if memory is an CObject
		IScriptable* owner = reinterpret_cast< IScriptable* >( object );
		ApplyDefaultValues( owner, object, createInlinedObjects, applyBaseClassDefaults );
	}
	else
	{
		// No owner given, do not create inlined values
		ApplyDefaultValues( NULL, object, false, applyBaseClassDefaults );
	}
}

void CClass::ApplyDefaultValues( IScriptable* owner, void* object, Bool createInlinedObjects, Bool applyBaseClassDefaults ) const
{
	// Apply base class default values
	if ( applyBaseClassDefaults )
	{
		if ( m_baseClass.m_parentClass )
		{
			m_baseClass.m_parentClass->ApplyDefaultValues( owner, object, createInlinedObjects, applyBaseClassDefaults );
		}
	}

	// Apply class default values
	for ( Uint32 i=0; i<m_defaultValues.Size(); i++ )
	{
		CDefaultValue* val = m_defaultValues[i];
		val->Apply( owner, object, createInlinedObjects );
	}
}

Red::Threads::AtomicOps::TAtomic64	st_allObjectMemory;

Uint64 GetAllObjectMemory()
{
	return st_allObjectMemory;
}

void* CClass::CreateObject( Uint32 sizeCheck, const Bool callConstruct /*= true*/, void* inMemory /*= nullptr*/ ) const
{
	RED_UNUSED( sizeCheck );
	ASSERT( !sizeCheck || GetSize() >= sizeCheck, TXT( "Class '%ls' has failed size check while creating an object. Probable corrupt data" ), m_name.AsString().AsChar() );

	// Get memory for object
	void* mem = inMemory;
	if ( !mem )
	{
		// Allocate dynamic memory block
		mem = AllocateClass();

		// Memory allocation request failed (f.e. creating abstract class)
		if ( mem == nullptr )
		{
			return nullptr;
		}
	}

	// Bind class (only scriptable objects)
	if ( IsScriptable() )
	{
		((IScriptable*)mem)->m_class = (CClass*)this;
	}

	// Call the constructor
	// Constructor is not called if this function is called from "operator new"
	if ( callConstruct )
	{
		Construct( mem );
	}

	// Count allocated objects
	Red::Threads::AtomicOps::Increment32( &m_numAllocatedObjects );
	Red::Threads::AtomicOps::ExchangeAdd64( &st_allObjectMemory, +(Int32)(GetSize() + GetScriptDataSize()) );

	// Return created object
	return mem;
}

void CClass::DestroyObject( void* mem, const Bool callDestructor /*= true*/, const Bool freeMemory /*= true*/ ) const
{
	// Empty memory
	if ( !mem )
		return;

	// Prevent from freeing memory from object's that don't own them
	Bool canFreeMemory = true;
	if ( m_isObject )
	{
		const CObject* object = static_cast< const CObject* >( mem );
		/*if ( object->HasFlag( OF_ExternalMemory ) )
		{
			canFreeMemory = false;
		}*/
	}

	// Call the destructor
	// Destructor is not called if this function is called from operator delete
	if ( callDestructor )
	{
		Destruct( mem );
	}

	// Free object memory
	if ( freeMemory & canFreeMemory )
	{
#ifndef RED_FINAL_BUILD
		// Cleanup memory but leave the vptr so we can know what type of class it was
		const Uint32 classSize = GetSize();
		Red::System::MemorySet( mem, 0xEE, classSize );
#endif

		// Free the memory
		FreeClass( mem );
	}

	// Count allocated objects
	const Int32 count = Red::Threads::AtomicOps::Decrement32( &m_numAllocatedObjects );
	RED_FATAL_ASSERT( count >= 0, "Invalid object count for class '%ls'. Something went horribly wrong. Check object allocations/deallocations.", GetName().AsChar() );
	RED_UNUSED( count );

	// Count used object memory
	Red::Threads::AtomicOps::ExchangeAdd64( &st_allObjectMemory, -(Int32)(GetSize() + GetScriptDataSize()) );
}

Bool CClass::DeepCompare( const void* data1, const void* data2 ) const
{
	ASSERT( data1 );
	ASSERT( data2 );

	// Get properties
	TDynArray< CProperty* > properties;
	properties.Reserve( GetLocalProperties().Size() );
	GetProperties( properties );

	// Compare properties
	for ( Uint32 i=0; i<properties.Size(); i++ )
	{
		CProperty* prop = properties[i];
		const void* propData1 = prop->GetOffsetPtr( data1 );
		const void* propData2 = prop->GetOffsetPtr( data2 );
		if ( !prop->GetType()->Compare( propData1, propData2, 0 ) )
		{
			return false;
		}
	}

	// Equal
	return true;
}

void CClass::MarkAsScripted()
{
	ASSERT( IsNative() );
	ASSERT( !IsScripted() );
	m_flags |= CF_Scripted;
}

void CClass::MarkAsIncomplete()
{
	m_flags |= CF_UndefinedFunctions;
}

void CClass::MarkAsExported()
{
	ASSERT( IsNative() );
	ASSERT( !IsExported() );
	m_flags |= CF_Exported;
}

void CClass::MarkAsStateMachine()
{
	m_flags |= CF_StateMachine;
}

const CName CClass::GetAutoStateName() const
{
	if ( m_autoStateName )
	{
		return m_autoStateName;
	}

	if ( HasBaseClass() )
	{
		return GetBaseClass()->GetAutoStateName();
	}

	return CName::NONE;
}

void CClass::SetAutoStateName( const CName autoStateName )
{
	m_autoStateName = autoStateName;
}

void CClass::SetAlignment( const Uint32 alignment )
{
	m_alignment = Max( alignment, 4u );
}

void CClass::SetCustomSerializer( TCustomClassSerializer serializer )
{
	m_customSerializer = serializer;
}

void* NoClassCast( void* obj )
{
	return obj;
}

void CClass::AddParentClass( CClass* baseClass )
{
	ASSERT( !HasBaseClass() );
	ASSERT( m_scriptSize == 0 );
	ASSERT( m_size == 0 );

	// Set base class
	m_baseClass.m_parentClass = baseClass;
	m_baseClass.m_castFromParentFunction = &NoClassCast;
	m_baseClass.m_castToParentFunction = &NoClassCast;
}

void CClass::AddProperty( CProperty* property )
{
	ASSERT( property, TXT( "Trying to add NULL property to class '%ls'" ), GetName().AsString().AsChar() );
	ASSERT( !FindProperty( property->GetName() ), TXT( "The property with name '%ls' already exists in class '%ls'" ), property->GetName().AsString().AsChar(), GetName().AsString().AsChar() );
	
	m_localProperties.PushBack( property );
	m_arePropertiesCached = false;
	m_areScriptPropertiesToDestroyCached = false;
	m_areGCPropertiesCached = false;

	// Assign unique hash to the property
	property->AssignHash( m_name );

	// Register property in the global hash
	SRTTI::GetInstance().RegisterProperty( property );
}

void CClass::AddPropertyBinding( const CName propertyName, const CName binding )
{
	RED_ASSERT( !FindLocalPropertyBinding( propertyName ), TXT("Trying to add duplicated property binding '%ls'->'%ls' to class '%ls'"), propertyName.AsChar(), binding.AsChar(), nullptr );

	SAutoBindingInfo info;
	info.m_propertyName = propertyName;
	info.m_bindingInformation = binding;
	m_localAutoBinding.PushBack( info );
}

void CClass::AddFunction( CFunction* function )
{
	ASSERT( function, TXT( "Trying to add NULL function to class '%ls'" ), GetName().AsString().AsChar() );
	//ASSERT( !FindFunction( function->GetName() ) );
	ASSERT( function->GetClass() == this, TXT( "Function '%ls' belongs to class '%ls', not class '%ls'" ), function->GetName().AsString().AsChar(), function->GetClass()->GetName().AsString().AsChar(), GetName().AsString().AsChar() );
	
	m_localFunctions.PushBack( function );
}

void CClass::GetProperties( TDynArray< CProperty* > &properties ) const
{
	if ( HasBaseClass() )
	{
		GetBaseClass()->GetProperties( properties );
	}

	// add local and parent properties
	for ( CProperty* prop : m_localProperties )
		properties.PushBack( prop );
}

void CClass::AddState( CName stateName, CClass* stateClass )
{
	ASSERT( stateName );
	ASSERT( stateClass );
	ASSERT( !stateClass->IsState() );
	ASSERT( !stateClass->m_stateMachineClass );

	// Add to state machine
	stateClass->m_flags |= CF_State;
	stateClass->m_stateName = stateName;
	stateClass->m_stateMachineClass = this;
	m_stateClasses.PushBack( stateClass );
}

CClass* CClass::FindStateClass( CName name ) const
{
	// Search for class with valid state name
	for ( Uint32 i = 0; i < m_stateClasses.Size(); ++i )
	{
		CClass* stateClass = m_stateClasses[ i ];
		if ( stateClass->GetStateName() == name )
		{
			return stateClass;
		}
	}

	// Search in base class
	if ( HasBaseClass() )
	{
		return GetBaseClass()->FindStateClass( name );
	}

	// Not found
	return NULL;
}

void CClass::CreateDefaultObject()
{}

void CClass::DestroyDefaultObject()
{
	// Dex says: This should be done in Deinit class, but before "DestroyDefaultObject",
	// leave it that way for now, but this is a HACK
	m_defaultValues.ClearPtr();
}

void CClass::ClearScriptData()
{
	// Clear export flag
	m_flags &= ~CF_Exported;

	// Clear state binding
	m_stateName = CName::NONE;
	m_stateMachineClass = NULL;
	m_stateClasses.Clear();
	m_flags &= ~CF_State;
	m_flags &= ~CF_StateMachine;

	// Clear cached data
	m_arePropertiesCached = false;
	m_cachedProperties.ClearFast();
	m_areGCPropertiesCached = false;
	m_cachedGCProperties.ClearFast();
	m_propertiesToDestroy.ClearFast();
	m_areScriptPropertiesToDestroyCached = false;
	m_cachedScriptPropertiesToDestory.ClearFast();

	// We need to reset the following flag to rebuild m_propertiesToDestroy inside NeedsCleaning
	m_needsCleaningWasChecked = false;

	// Clear cached functions
	m_allFunctionsMap.ClearFast();

	// Clear property binding information
	m_localAutoBinding.Clear();

	// This is a scripted class, delete everything :)
	if ( !IsNative() )
	{
		// Clear properties
		for ( Uint32 i = 0; i < m_localProperties.Size(); ++i )
		{
			SRTTI::GetInstance().UnregisterProperty( m_localProperties[ i ] );
		}

		m_localProperties.ClearPtr();

		// Clear functions
		for ( Uint32 i = 0; i < m_localFunctions.Size(); ++i )
		{
			CFunction* func = m_localFunctions[i];
			RED_FATAL_ASSERT( !func->IsNative(), "Purely scripted class '%ls' somehow has a native function '%ls'", GetName().AsChar(), func->GetName().AsChar() );

			// Remove script related data
			func->ClearScriptData();
		}

		m_localFunctions.ClearPtr();

		// Clear everything else
		m_defaultValues.ClearPtr();
		m_baseClass.m_parentClass = nullptr;
		m_baseClass.m_castFromParentFunction = nullptr;
		m_baseClass.m_castToParentFunction = nullptr;
		return;
	}

	// Collect script properties to delete
	TDynArray< CProperty* > propertiesToDelete;
	for ( Uint32 i = 0; i < m_localProperties.Size(); ++i )
	{
		CProperty* prop = m_localProperties[i];
		if ( prop->IsScripted() )
		{
			propertiesToDelete.PushBack( prop );
		}

		// Unexport
		if ( prop->IsExported() )
		{
			CProperty::ChangePropertyFlag( this, prop->GetName(), PF_Exported, 0 );
		}
	}

	// Delete all scripted properties
	for ( Uint32 i = 0; i < propertiesToDelete.Size(); ++i )
	{
		m_localProperties.Remove( propertiesToDelete[i] );
		SRTTI::GetInstance().UnregisterProperty( propertiesToDelete[i] );
		delete propertiesToDelete[i];
	}

	// Collect script functions to delete
	TDynArray< CFunction* > functionsToDelete;
	for ( Uint32 i = 0; i < m_localFunctions.Size(); ++i )
	{
		CFunction* func = m_localFunctions[i];
		if ( !func->IsNative() )
		{
			functionsToDelete.PushBack( func );
		}

		// Remove script related data
		func->ClearScriptData();
	}

	// Delete script functions
	for ( Uint32 i = 0; i < functionsToDelete.Size(); ++i )
	{
		m_localFunctions.Remove( functionsToDelete[i] );
		delete functionsToDelete[i];
	}

	// Delete default properties definitions
	m_defaultValues.ClearPtr();
}

void CClass::ClearProperties()
{
	for ( CProperty* prop : m_localProperties )
	{
		SRTTI::GetInstance().UnregisterProperty( prop );
		delete prop;
	}

	m_localProperties.Clear();
	m_cachedProperties.Clear();
	m_cachedGCProperties.Clear();
	m_cachedScriptPropertiesToDestory.Clear();
	m_arePropertiesCached = false;
	m_areScriptPropertiesToDestroyCached = false;
	m_areGCPropertiesCached = false;
}

CProperty* CClass::FindProperty( const CName &name ) const
{
	for ( Uint32 i=0; i<m_localProperties.Size(); ++i )
	{
		if ( m_localProperties[i]->GetName() == name )
		{
			return m_localProperties[i];
		}
	}

	if ( HasBaseClass() )
	{
		CProperty* property = GetBaseClass()->FindProperty( name );
		if ( property )
		{
			return property;
		}
	}

	return NULL;
}

CName CClass::FindPropertyBinding( const CName name ) const
{
	// this happens VERY rarely - no need for a hash map, etc
	for ( Uint32 i=0; i<m_localAutoBinding.Size(); ++i )
	{
		if ( m_localAutoBinding[i].m_propertyName == name )
		{
			return m_localAutoBinding[i].m_bindingInformation;
		}
	}

	// recurse (naive)
	if ( HasBaseClass() )
		return GetBaseClass()->FindPropertyBinding( name );

	// no binding found
	return CName::NONE;
}

CName CClass::FindLocalPropertyBinding( const CName name ) const
{
	for ( Uint32 i=0; i<m_localAutoBinding.Size(); ++i )
	{
		if ( m_localAutoBinding[i].m_propertyName == name )
		{
			return m_localAutoBinding[i].m_bindingInformation;
		}
	}

	return CName::NONE;
}

const CFunction* CClass::FindFunction( const CName& name ) const
{
#ifdef NO_EDITOR
	TFunctionMap::const_iterator iter = m_allFunctionsMap.Find( name );
	if( iter == m_allFunctionsMap.End() )		
	{
		return nullptr;
	}

	return iter->m_second;
#else
	return FindFunctionNonCached( name );
#endif
}

const Bool CClass::HasStateClasses() const
{
	if ( !m_stateClasses.Empty() )
	{
		return true;
	}

	if ( HasBaseClass() )
	{
		if ( GetBaseClass()->HasStateClasses() )
		{
			return true;
		}
	}

	return false;
}

const CFunction* CClass::FindFunctionNonCached( const CName& name ) const
{
	for ( Uint32 i=0; i<m_localFunctions.Size(); ++i )
	{
		if ( m_localFunctions[i]->GetName() == name )
		{
			return m_localFunctions[i];
		}
	}

	if ( HasBaseClass() )
	{
		const CFunction* function = GetBaseClass()->FindFunctionNonCached( name );
		if ( function )
		{
			return function;
		}
	}

	return NULL;
}


Bool CClass::ToString( const void* object, String& valueString ) const
{
	TDynArray< CProperty* > properties;
	GetProperties( properties );	

	for ( Uint32 i=0; i<properties.Size(); i++ )
	{
		String value;
		const void* ptrData = properties[i]->GetOffsetPtr( object );
		if ( properties[i]->GetType()->ToString( ptrData, value ) )
		{
			// Add separator
			if ( !valueString.Empty() )
			{
				valueString += TXT("; ");
			}

			// Append sub property value
			valueString += value;
		}
	}

	return true;
}

Bool CClass::FromString( void* object, const String& valueString ) const
{
	// Attempt to set the properties from semicolon-separated string
	// (this may will fail for nested types but it is useful for
	// copy/pasting primitive types like vectors)

	// Get properties
	TDynArray< CProperty* > properties;
	GetProperties( properties );	

	// Split the string to its parts
	TDynArray< String > parts = valueString.Split( TXT(";"), true );

	// Make sure parts and properties are the same count
	if ( parts.Size() != properties.Size() )
	{
		return false;
	}

	// Attempt to set each one of the properties
	for ( Uint32 i=0; i<properties.Size(); i++ )
	{
		String value;
		void* ptrData = properties[i]->GetOffsetPtr( object );
		if ( properties[i]->GetType()->ToString( ptrData, value ) )
		{
			if ( !properties[i]->GetType()->FromString( ptrData, parts[i] ) )
			{
				return false;
			}
		}
	}

	return true;
}

Bool CClass::Serialize( IFile& file, void* data ) const
{
	if ( m_customSerializer != nullptr )
	{
		return m_customSerializer( file, data );
	}
	else
	{
		return DefaultSerialize( file, data );
	}
}

Bool CClass::DefaultSerialize( IFile& file, void* data ) const
{
	if ( file.IsGarbageCollector() )
	{
		return SerializeGC( file, data );
	}
	else if ( IsSerializable() )
	{
		const void* defualtObjectData = GetDefaultObjectImp();
		return SerializeDiff( NULL, file, data, defualtObjectData, const_cast<CClass*>( this ) );
	}
	else
	{
		return SerializeDiff( NULL, file, data, IsDefaultObjectSerialization() ? GetDefaultObjectImp() : NULL, const_cast<CClass*>( this ) );
	}
}

Uint32 CClass::CalcScriptClassAlignment() const
{
	TDynArray< CProperty* > allProps;
	GetProperties( allProps );

	// Make sure data in scripted classes is aligned as well
	Uint32 alignment = 4; // default
	for ( Uint32 i=0; i<allProps.Size(); ++i )
	{
		const CProperty* prop = allProps[i];
		if ( prop->IsScripted() || !HasBaseClass() )
		{
			Uint32 propAlignment = prop->GetType()->GetAlignment();			

			// special case for scripted structures
			if ( prop->GetType()->GetType() == RT_Class )
			{
				const CClass* propClass = static_cast< const CClass* >( prop->GetType() );
				if ( propClass->IsScripted() && !propClass->HasBaseClass() && !propClass->IsNative() )
				{
					// override the alignment with true structure alignment
					const Uint32 propStructAlignment = propClass->CalcScriptClassAlignment();
					ASSERT( propStructAlignment != 0 );
					propAlignment = propStructAlignment;
				}
			}

			alignment = Max< Uint32 >( propAlignment, alignment );
		}
	}

	// Return calculated alignment
	return alignment;
}

void CClass::RecalculateClassDataSize()
{
	// Class is native scripted class, update the script size only
	if ( IsScripted() )
	{
		// The script properties
		TDynArray< CProperty*, MC_RTTI > localScriptProperties;
		for ( Uint32 i=0; i<m_localProperties.Size(); i++ )
		{
			CProperty* prop = m_localProperties[i];
			if ( prop && prop->IsScripted() )
			{
				localScriptProperties.PushBack( prop );
			}
		}

		// Base offset from base class
		Uint32 baseOffset = 0;
		if ( HasBaseClass() )
		{
			CClass* baseClass = GetBaseClass();
			baseOffset = baseClass->GetScriptDataSize();

			// Update native data size from the base class
			if ( 0 == m_size )
			{
				ASSERT( !IsNative() );
				baseClass->RecalculateClassDataSize();
				m_size = baseClass->GetSize();
			}
		}

		// Calculate required initial alignment
		const Uint32 classAlignment = CalcScriptClassAlignment();		
		m_scriptSize = CProperty::CalcDataLayout( localScriptProperties, baseOffset, classAlignment );
	}
	else if ( HasBaseClass() )
	{
		// Get size of scripted data from base class
		m_scriptSize = GetBaseClass()->GetScriptDataSize();
	}
	ASSERT( m_size > 0 );
}

Bool CClass::ValidateLayout() const
{
	Bool result = true;

	TDynArray< CProperty* > properties;
	GetProperties( properties );

	for ( Uint32 i=0; i<properties.Size(); ++i )
	{
		const CProperty* prop = properties[i];

		const Uint32 dataStart = prop->GetDataOffset();
		const Uint32 dataEnd = dataStart + prop->GetType()->GetSize();

		// check limits
		if ( prop->IsScripted() && dataEnd > GetScriptDataSize() )
		{
			WARN_CORE( TXT("Property '%ls' in '%ls' is out bounds (script data)"), 
				prop->GetName().AsString().AsChar(),
				GetName().AsString().AsChar() );

			result = false;
		}
		else if ( !prop->IsScripted() && dataEnd > GetSize() )
		{
			WARN_CORE( TXT("Property '%ls' in '%ls' is out bounds (native data)"), 
				prop->GetName().AsString().AsChar(),
				GetName().AsString().AsChar() );

			result = false;
		}

		// check overlap with class properties
		for ( Uint32 j=0; j<properties.Size(); ++j )
		{
			const CProperty* otherProp = properties[j];
			if ( (otherProp != prop) && otherProp->IsScripted() == prop->IsScripted() )
			{
				const Uint32 otherStart = otherProp->GetDataOffset();
				const Uint32 otherEnd = otherStart + otherProp->GetType()->GetSize();

				if ( !(dataEnd <= otherStart || dataStart >= otherEnd) )
				{
					WARN_CORE( TXT("Property '%ls' in '%ls' overlaps property '%ls'"), 
						prop->GetName().AsString().AsChar(),
						GetName().AsString().AsChar(),
						otherProp->GetName().AsString().AsChar());
					result = false;
				}
			}
		}
	}

	return result;
}

void CScriptedClass::RecalculateClassDataSize()
{
	ASSERT( IsScripted() );

	// Scripted struct
	if ( !HasBaseClass() )
	{
		// Build data layout
		const Uint32 classAlignment = CalcScriptClassAlignment();
		const auto& allProperties = GetCachedProperties();
		const Uint32 propsSize = CProperty::CalcDataLayout( allProperties, 0, classAlignment );

		// Scripted classes should be padded to keep the size of the script class data matched with the aligned
		const Uint32 paddedSize = static_cast< Uint32 >( AlignOffset( propsSize, classAlignment ) );
		if ( paddedSize > m_size )
		{
			m_size = paddedSize;
		}
	}
	else
	{
		// Treat as normal scripted class
		CClass::RecalculateClassDataSize();
	}
}

static Bool ConvertHandleToSoftHandleTypes( CVariant& value, CProperty* prop, void* data )
{
	if ( value.GetRTTIType()->GetType() == RT_Handle && prop->GetType()->GetType() == RT_SoftHandle )
	{
		BaseSoftHandle* softHandle = ( BaseSoftHandle* ) prop->GetOffsetPtr( data );
		BaseSafeHandle* safeHandle = ( BaseSafeHandle* ) value.GetData();
		if ( safeHandle && safeHandle->Get() )
		{
			CResource* resource = Cast< CResource >( (CObject*) safeHandle->Get() );
			*softHandle = BaseSoftHandle( resource );
		}
		else
		{
			softHandle->Clear();
		}

		return true;
	}

	// Not converted
	return false;
}

static Bool ConvertSoftHandleToHandleTypes( CVariant& value, CProperty* prop, void* data )
{
	if ( value.GetRTTIType()->GetType() == RT_SoftHandle && prop->GetType()->GetType() == RT_Handle )
	{
		BaseSoftHandle* softHandle = ( BaseSoftHandle* ) value.GetData();
		BaseSafeHandle* safeHandle = ( BaseSafeHandle* ) prop->GetOffsetPtr( data );

		if ( softHandle && softHandle->Get() )
		{
			THandle< CResource > resource = softHandle->Get();
			*safeHandle = (const BaseSafeHandle&) resource;
		}
		else
		{
			safeHandle->Clear();
		}

		return true;
	}

	// Not converted
	return false;
}

static Bool ConvertPointerToSoftHandleTypes( CVariant& value, CProperty* prop, void* data )
{
	// Convert from pointers to safe handles automatically 
	if ( value.GetRTTIType()->GetType() == RT_Pointer && prop->GetType()->GetType() == RT_SoftHandle )
	{
		BaseSoftHandle* softHandle = ( BaseSoftHandle* ) prop->GetOffsetPtr( data );
		CObject* ptr = *( CObject** ) value.GetData();
		if ( ptr )
		{
			CResource* resource = Cast< CResource >( ptr );
			*softHandle = BaseSoftHandle( resource );
		}
		else
		{
			softHandle->Clear();
		}

		return true;
	}

	// Not converted
	return false;
}

static Bool ConvertUint64ToCDateTimeTypes( CVariant& value, CProperty* prop, void* data )
{
	if
	(
		value.GetRTTIType()->GetType() == RT_Fundamental &&
		prop->GetType()->GetType() == RT_Simple &&

		value.GetRTTIType()->GetName() == ::GetTypeName< Uint64 >() &&
		prop->GetType()->GetName() == ::GetTypeName< CDateTime >()
	)
	{
		CDateTime* dest = static_cast< CDateTime* >( prop->GetOffsetPtr( data ) );
		Uint64* source = static_cast< Uint64* >( value.GetData() );

		dest->ImportFromOldFileTimeFormat( *source );

		//RED_LOG_SPAM( RTTI, TXT( "Converted %ls::%ls (%p) from %ull to %ull" ), prop->GetParent()->GetName().AsChar(), prop->GetName().AsChar(), dest, *source, dest->GetRaw() );

		return true;
	}

	// Not converted
	return false;
}

static Bool ConvertEnumToIntTypes( CVariant& value, CProperty* prop, void* data )
{
	if ( ( value.GetRTTIType()->GetType() == RT_Enum && prop->GetType()->GetName() == ::GetTypeName< Int32 >() )
		|| ( value.GetType() == ::GetTypeName< Int32 >() && prop->GetType()->GetType() == RT_Enum ) )
	{
		Int32* src  = reinterpret_cast< Int32* >( value.GetData() );
		Int32* dest = reinterpret_cast< Int32* >( prop->GetOffsetPtr( data ) );
		
		*dest = *src;

		return true;
	}

	// Not converted
	return false;
}

static Bool ConvertStringToStringAnsiTypes( CVariant& value, CProperty* prop, void* data )
{
	if ( value.GetType() == ::GetTypeName< String >() && prop->GetType()->GetName() == ::GetTypeName< StringAnsi >() )
	{

		String* src  = reinterpret_cast< String* >( value.GetData() );
		StringAnsi* dest = reinterpret_cast< StringAnsi* > ( prop->GetOffsetPtr( data ) );
		*dest = UNICODE_TO_ANSI( src->AsChar() );

		return true;
	}

	if ( value.GetType() == ::GetTypeName< StringAnsi >() && prop->GetType()->GetName() == ::GetTypeName< String >() )
	{
		StringAnsi* src  = reinterpret_cast< StringAnsi* >( value.GetData() );
		String* dest = reinterpret_cast< String* > ( prop->GetOffsetPtr( data ) );
		*dest = ANSI_TO_UNICODE( src->AsChar() );

		return true;
	}

	// Not converted
	return false;
}

static Bool ConvertStringToCnameTypes( CVariant& value, CProperty* prop, void* data )
{
	// Convert from strings to cnames automatically
	if ( value.GetType() == ::GetTypeName< String >() && prop->GetType()->GetName() == ::GetTypeName< CName >() )
	{
		
		CName cnameValue( value.ToString() );
		CName* cname = ( CName* ) prop->GetOffsetPtr( data );
		if ( cname )
		{
			*cname = cnameValue;
		}

		return true;
	}
	
	if ( value.GetType() == ::GetTypeName< CName >() && prop->GetType()->GetName() == ::GetTypeName< String >() )
	{
		String stringValue( value.ToString() );
		String* propertyString = (String*) prop->GetOffsetPtr( data );
		if ( propertyString )
		{
			*propertyString = stringValue;
		}

		return true;
	}

	// Not converted
	return false;
}

static Bool ConvertArrayStringsToCNamesTypes( CVariant& value, CProperty* prop, void* data )
{
	if ( value.GetType() == ::GetTypeName< TDynArray< String > >()
		&& prop->GetType()->GetName() == ::GetTypeName< TDynArray< CName > > () )
	{
		TDynArray< String > strings;
		value.AsType( strings );

		TDynArray< CName >* names = reinterpret_cast< TDynArray< CName >* >( prop->GetOffsetPtr( data ) );
		if ( names != NULL )
		{
			for( Uint32 i = 0; i < strings.Size(); ++i )
			{
				names->PushBack( CName( strings[ i ] ) );
			}
		}

		return true;
	}

	if ( value.GetType() == ::GetTypeName< TDynArray< CName > >()
		&& prop->GetType()->GetName() == ::GetTypeName< TDynArray< String > > () )
	{
		TDynArray< CName > names;
		value.AsType( names );

		TDynArray< String >* strings = reinterpret_cast< TDynArray< String >* >( prop->GetOffsetPtr( data ) );
		if ( strings != NULL )
		{
			for( Uint32 i = 0; i < names.Size(); ++i )
			{
				strings->PushBack( names[ i ].AsString() );
			}
		}

		return true;
	}

	// Not converted
	return false;
};

class CGCStackMessage_Object : public Red::Error::StackMessage
{
private:
	const CClass*	m_class;
	const void*		m_object;

public:
	CGCStackMessage_Object( const CClass* cls, const void* obj )
		: m_class( cls )
		, m_object( obj )
	{
	}

	virtual void Report( Char* outText, Uint32 outTextLength )
	{
		Int32 charsWritten = 0;
		charsWritten = Red::System::SNPrintF( outText, outTextLength, TXT("GCObject: 0x%016llX "), m_object );
		outText += charsWritten;	outTextLength -= charsWritten;

#ifdef RED_PLATFORM_WINPC
		if ( m_class->IsA< CObject >() )
		{
			const CObject* obj = static_cast< const CObject* >( m_object );
			if ( IsBadReadPtr( obj, sizeof(CObject) ) == 0 )
			{
				charsWritten = Red::System::SNPrintF( outText, outTextLength, TXT("Index: %d "), obj->GetObjectIndex() );
				outText += charsWritten;	outTextLength -= charsWritten;

				CClass* cls = obj->GetLocalClass();
				if ( IsBadReadPtr( cls, sizeof(CClass) ) == 0 )
				{
					if ( cls->GetType() == RT_Class )
					{
						charsWritten = Red::System::SNPrintF( outText, outTextLength, TXT("%ls "), cls->GetName().AsChar() );
						outText += charsWritten;	outTextLength -= charsWritten;
					}
					else
					{
						charsWritten = Red::System::SNPrintF( outText, outTextLength, TXT("BAD CLS2!!") );
						outText += charsWritten;	outTextLength -= charsWritten;
					}
				}
				else
				{
					charsWritten = Red::System::SNPrintF( outText, outTextLength, TXT("BAD CLS!!") );
					outText += charsWritten;	outTextLength -= charsWritten;
				}
			}
			else
			{
				charsWritten = Red::System::SNPrintF( outText, outTextLength, TXT("BAD!!") );
				outText += charsWritten;	outTextLength -= charsWritten;
			}
		}
#endif
	}
};

class CGCStackMessage_Property : public Red::Error::StackMessage
{
private:
	const CProperty*	m_property;

public:
	CGCStackMessage_Property( const CProperty* prop )
		: m_property( prop )
	{
	}

	virtual void Report( Char* outText, Uint32 outTextLength )
	{
		Red::System::SNPrintF( outText, outTextLength, TXT("GCProperty %ls, type '%ls'"), m_property->GetName().AsChar(), m_property->GetType()->GetName().AsChar() );
	}
};

Bool CClass::SerializeGC( IFile& file, void* data ) const
{
#ifndef RED_FINAL_BUILD
	CGCStackMessage_Object crash_message( this, data );
#endif

	const auto& properties = GetCachedGCProperties();

	for ( Uint32 i=0, end = properties.Size(); i<end; i++ )
	{
		CProperty* prop = properties[i];
		void* propData = prop->GetOffsetPtr( data );
#ifndef RED_FINAL_BUILD
		CGCStackMessage_Property crash_message_prop( prop );
#endif
		// Avoid crash during script recompilation if properties aren't created yet
#ifdef RED_PLATFORM_WINPC
		if ( propData )
#else
		RED_FATAL_ASSERT( propData, "Property data missing!" );
#endif
		{
			prop->GetType()->Serialize( file, propData );
		}
	}
	
	// Done
	return true;
}

static Bool AreTypesSerializationCompatible( const IRTTIType* originalType, const IRTTIType* currentType )
{
	// same type
	if ( originalType == currentType ) return true;

	// if any type is null then types are not compatible
	if ( !originalType || !currentType ) return false;

	// arrays are compatible if their inner type is compatible regardless of anything else
	const Bool isOriginalTypeArray = (originalType->GetType() == RT_Array) || (originalType->GetType() == RT_NativeArray);
	const Bool isCurrentTypeArray = (currentType->GetType() == RT_Array) || (currentType->GetType() == RT_NativeArray);
	if ( isOriginalTypeArray && isCurrentTypeArray )
	{
		const IRTTIBaseArrayType* originalArrayType = static_cast< const IRTTIBaseArrayType* >( originalType );
		const IRTTIBaseArrayType* currentArrayType = static_cast< const IRTTIBaseArrayType* >( currentType );

		// make sure that inner types are compatible
		return AreTypesSerializationCompatible( originalArrayType->ArrayGetInnerType(), currentArrayType->ArrayGetInnerType() );
	}

	// pointers are handles are serialization compatible
	const Bool isOriginalTypePointerLike = ( originalType->GetType() == RT_Pointer || originalType->GetType() == RT_Handle );
	const Bool isCurrentTypePointerLike = ( currentType->GetType() == RT_Pointer || currentType->GetType() == RT_Handle );
	if ( isOriginalTypePointerLike && isCurrentTypePointerLike )
	{
		// upcast is always safe and downcast is handled in the serialization code so whatever the actual types are we can assume that it's safe to serialize them directly
		return true;
	}

	// simple types conversions
	{
		// Uint32 -> Int32 conversion
		if ( originalType->GetName() == ::GetTypeName< Uint32 >() && currentType->GetName() == ::GetTypeName< Int32 >() )
		{
			return true;
		}

		// Int32 -> Uint32 conversion
		if ( originalType->GetName() == ::GetTypeName< Int32 >() && currentType->GetName() == ::GetTypeName< Uint32 >() )
		{
			return true;
		}
	}

	// data buffer and shared data buffers are compatible
	if ( (originalType == GetTypeObject< DataBuffer >() && currentType == GetTypeObject< SharedDataBuffer >()) ||
		(originalType == GetTypeObject< SharedDataBuffer >() && currentType == GetTypeObject< DataBuffer >()) )
	{
		return true;
	}

	// not directly compatible
	return false;
}

Bool CClass::WritePropertyList( class ISerializable* owner, IFile& file, void* data, const void* defaultData, CClass* defaultDataClass ) const
{
	// Grab all struct properties
	const auto& properties = GetCachedProperties();

#ifdef DUMP_SERIALIZATION_FILE_LAYOUT
	LOG_CORE( TXT("Write Start class '%ls', offset %d"),
		GetName().AsChar(), file.GetOffset() );
#endif

	// Serialize them
	for ( Uint32 i=0; i<properties.Size(); i++ )
	{
		CProperty* prop = properties[i];

		// Skip unserializable properties
		if ( !prop->IsSerializable() )
		{
			continue;
		}

		// When cooking skip properties that are not for cooked builds
		if ( file.IsCooker() && !prop->IsSerializableInCookedBuilds() )
		{
			continue;
		}

		// Do not save scripted properties that are not editable (internal scripted properties)
		if ( file.IsFilteringScriptedProperties() )
		{
			const Bool isScriptedStruct = (IsScripted() && !HasBaseClass());
			const Bool isScriptedProp = prop->IsScripted() || isScriptedStruct;
			if ( isScriptedProp && !prop->IsEditable() && !prop->IsSaved() )
			{
				continue;
			}
		}

		// Skip property if it has the same value as in the default object
		if ( defaultData )
		{
			// Property should be defined in the default class, if it's not defined that we cannot diff it
			if ( (defaultDataClass == this) || defaultDataClass->FindProperty( prop->GetName() ) == prop )
			{
				const void* srcData1 = prop->GetOffsetPtr( data );
				const void* srcData2 = prop->GetOffsetPtr( defaultData );
				if ( srcData1 && srcData2 )
				{
					if ( prop->GetType()->Compare( srcData1, srcData2, 0 ) )
					{
						continue;
					}
				}
			}
		}

		// Ask object if we should serialize this property
		if ( !file.IsGarbageCollector() && owner )
		{
			if ( !owner->OnPropertyCanSave( file, prop->GetName(), prop ) )
			{
				continue;
			}
		}

		// Save property name
		// TODO: in cooked data consider using a property index instead
		CName propName = prop->GetName();
		file << propName;

		// Save property type reference
		// TODO: this can be skipped in cooked data (we should be 100% sure about the property type)
		IRTTIType* propType = prop->GetType();
		file << propType;

#ifdef DUMP_SERIALIZATION_FILE_LAYOUT
		LOG_CORE( TXT("Write prop '%ls', class '%ls', type '%ls', offset %d"),
			propName.AsChar(), GetName().AsChar(), propType->GetName().AsChar(), file.GetOffset() );
#endif

		// Save property value, contain it inside a skipable block so in case of any mismatch during loading we can skip it+
		{
			// TODO: this should not be used in cooked data
			CFileSkipableBlock block( file );

			// Save property value
			void* propData = prop->GetOffsetPtr( data );
			if ( !prop->GetType()->Serialize( file, propData ) )
			{
				// Serialization failed
				if ( !file.IsCooker() )
				{
					ERR_CORE( TXT("Serialization of property '%ls' has failed"), prop->GetName().AsString().AsChar() );
				}

				return false;
			}
		}
	}

	// Use empty property name as marker
	CName emptyName;
	file << emptyName;

#ifdef DUMP_SERIALIZATION_FILE_LAYOUT
	LOG_CORE( TXT("Write end class '%ls', offset %d"),
		GetName().AsChar(), file.GetOffset() );
#endif

	// Done
	return true;
}

Bool CClass::ReadPropertyList( class ISerializable* owner, IFile& file, void* data ) const
{
	// Store all unknown properties so we can process them at the end. This way, a handler can
	// assume that the rest of the object has already been fully read in.
	TDynArray< TPair<CName, CVariant> > unknownProperties;

#ifdef DUMP_SERIALIZATION_FILE_LAYOUT
	LOG_CORE( TXT("Start class '%ls', offset %d"),
		GetName().AsChar(), file.GetOffset() );
#endif

	// Safe serialization
	for ( ;; )
	{
		CName propName;
		file << propName;

		// End Of List marker
		if ( !propName )
		{
			break;
		}

		// Load property typ (reads a CName and automatically finds the type - cached results!)
		IRTTIType* orginalType = NULL;
		file << orginalType;

#ifdef DUMP_SERIALIZATION_FILE_LAYOUT
		LOG_CORE( TXT("Prop '%ls', class '%ls', type '%ls', offset %d"),
			propName.AsChar(), GetName().AsChar(), orginalType ? orginalType->GetName().AsChar() : TXT("NULL"), file.GetOffset() );
#endif

		// Load 2 bytes (placeholder)
		// TODO: I will get rid of this as soon as the new serialization is in
		if ( file.GetVersion() < VER_CLASS_PROPERTIES_DATA_CLEANUP )
		{
			Uint16 crap;
			file << crap;
		}

		{
			// We save each property inside a skippable block in order to be able to skip the data fully in case the type is gone
			// TODO: this should be skipped in final/cooked data - it is unnecesarry stuff
			CFileSkipableBlock block( file );

			// Original type is not know - there's not way to read the property value
			if ( NULL == orginalType )
			{
				if ( !GFileManager->IsReadOnly() )
				{
					WARN_CORE( TXT("Property '%ls' in '%ls' uses unknown type. Property value will be lost."),
						propName.AsChar(), GetName().AsChar() );
				}

				block.Skip();
				continue;
			}

			// Get the property of given name
			CProperty* prop = FindProperty( propName );

			// Check if property exists.
			if ( !prop || !prop->IsSerializable() )
			{
				// Read the propertie's value to a univeral variant data structure for handling
				// This requires the RTTI type to be avaiable for the property
				CVariant value( orginalType->GetName(), NULL );
				if ( value.IsValid() )
				{
					// Load data
					if ( value.GetRTTIType()->Serialize( file, value.GetData() ) )
					{
						// Let the object handle the property
						if ( owner && owner->OnPropertyMissing( propName, value ) )
						{
							block.Skip();
							continue;
						}

						// We'll try to handle it ourselves later.
						unknownProperties.PushBack(MakePair(propName, value));
					}
				}

				// Property not found
				block.Skip();
				continue;
			}

			// Find original type the value was saved with, this is done to support seamless type conversion
			// This may return NULL in case when the original sub-type (for example a class) no longer exists so be careful.
			// Also, for performance reason if the type name already matches the type lookup is skipped.
			if ( !AreTypesSerializationCompatible( orginalType, prop->GetType() ) )
			{
				// If we are in here it means that the original type the value was saved with does not have compatible serialization with new type.
				// In general if we are able to load the original value we can try to convert it. If this fails we unfortunately need to skip the 
				// serialized value never knowing what it was.

				// Original type was not found, we cannot read the value
				if ( NULL == orginalType )
				{
					if ( !GFileManager->IsReadOnly() )
					{	
						WARN_CORE( TXT("Property '%ls' in '%ls' has different type '%ls' than the one that was serialized with ('%ls'). Original type does not exist any more. Property value will be lost."),
							propName.AsChar(), GetName().AsChar(), prop->GetType()->GetName().AsChar(), orginalType->GetName().AsChar() );
					}

					block.Skip();
					continue;
				}

				// Load the old value
				CVariant value( orginalType->GetName(), NULL );
				if ( value.IsValid() && value.GetRTTIType()->Serialize( file, value.GetData() ) )
				{
					// Let the object handle the conversion shit
					if ( owner && owner->OnPropertyTypeMismatch( propName, prop, value ) )
					{
						// Either way, skip the to the end of original data
						block.Skip();
						continue;
					}

					// Convert from normal handles to safe handles automatically
					if ( ConvertHandleToSoftHandleTypes( value, prop, data ) )
					{
						block.Skip();
						continue;
					}

					// Convert from normal soft handles to handles automatically
					if ( ConvertSoftHandleToHandleTypes( value, prop, data ) )
					{
						block.Skip();
						continue;
					}

					// Convert from pointers to safe handles automatically 
					if ( ConvertPointerToSoftHandleTypes( value, prop, data ) )
					{
						block.Skip();
						continue;
					}

					// Convert from strings to cnames automatically
					if ( ConvertStringToCnameTypes( value, prop, data ) )
					{
						block.Skip();
						continue;
					}

					// Convert arrays of strings to cnames automatically
					if ( ConvertArrayStringsToCNamesTypes( value, prop, data ) )
					{
						block.Skip();
						continue;
					}

					// Converts enums to ints automatically
					if ( ConvertEnumToIntTypes( value, prop, data ) )
					{
						block.Skip();
						continue;
					}

					// Converts enums to ints automatically
					if ( ConvertStringToStringAnsiTypes( value, prop, data ) )
					{
						block.Skip();
						continue;
					}

					// Converts old style time format to new style time format automagically
					if( ConvertUint64ToCDateTimeTypes( value, prop, data ) )
					{
						block.Skip();
						continue;
					}

					// We'll try to handle it ourselves later.
					unknownProperties.PushBack( MakePair( propName, value ) );
				}

				// Property type mismatched
				if ( !GFileManager->IsReadOnly() )
				{
					WARN_CORE
						(
						TXT( "Property '%ls' in '%ls' has different type '%ls' than the one that was serialized ('%ls'). No value conversion found, property value will be lost." ),
						propName.AsChar(),
						GetName().AsChar(),
						prop->GetType()->GetName().AsChar(),
						orginalType->GetName().AsChar()
						);
				}

				// Skip the whole block
				block.Skip();
				continue;
			}

			// Get the target offset
			void* propData = prop->GetOffsetPtr( data );

			// Load value
			if ( !prop->GetType()->Serialize( file, propData ) )
			{
				// Unable to deserialize the value
				block.Skip();
				continue;
			}

			// Check data consistency
			if ( file.GetOffset() != block.GetEndOffset() )
			{
				if ( !GFileManager->IsReadOnly() )
				{
					WARN_CORE( TXT("Data inconsistency when reading property '%ls' in '%ls', offset %i, exptected %i"), propName.AsString().AsChar(), GetName().AsString().AsChar(), file.GetOffset(), block.GetEndOffset() );
				}
				block.Skip();
				continue;
			}
		}
	}

#ifdef DUMP_SERIALIZATION_FILE_LAYOUT
	LOG_CORE( TXT("End class '%ls', offset %d"),
		GetName().AsChar(), file.GetOffset() );
#endif

	// We've read in all properties, so process any unknowns.
	for ( Uint32 i = 0; i < unknownProperties.Size(); ++i )
	{
		if ( !OnReadUnknownProperty( data, unknownProperties[i].m_first, unknownProperties[i].m_second ) )
		{
			// log spam
			//WARN_CORE( TXT("Could not read unknown property '%ls' in '%ls'"), unknownProperties[i].m_first.AsString().AsChar(), GetName().AsString().AsChar() );
		}
	}

	// Done
	return true;
}

Bool CClass::WriteDataStream( class ISerializable* owner, IFile& file, const void* data, const void* defaultData, CClass* defaultDataClass ) const
{
	RED_UNUSED(owner);
	CSerializationStream::Build( file, this, data, defaultData );
	return true;
}

Bool CClass::ReadDataStream( class ISerializable* owner, IFile& file, void* data ) const
{
	RED_UNUSED(owner);
	CSerializationStream::Parse( file, data, this );
	return true;
}

void CClass::HandleSerializationFlags( IFile& file, Uint8& flags ) const
{
	if ( file.GetVersion() >= VER_SERIALIZATION_STREAM )
	{
		// always use the serialization stream in cooked data
		if ( (file.IsWriter() || file.IsMapper()) && file.IsCooker() )
		{
#ifdef USE_SERIALIZATION_STREAM
			flags |= eSerializationFlag_CookedStream;
#endif
		}

		// store flags
		file << flags;
	}
}

Bool CClass::SerializeDiff( ISerializable* owner, IFile& file, void* data, const void* defaultData, CClass* defaultDataClass ) const
{
	// We should not use this method with GC serialization - it works but is to slow!
	ASSERT( !file.IsGarbageCollector() && "Please use SerializeGC instead - it's faster" );

	// Save/Load the serialization flags
	Uint8 serializationFlags = 0;
	HandleSerializationFlags( file, serializationFlags );

	// Save/Load the object data
	if ( file.IsReader() )
	{
		if ( serializationFlags & eSerializationFlag_CookedStream )
		{
			return ReadDataStream( owner, file, data );
		}
		else
		{
			return ReadPropertyList( owner, file, data );
		}
	}
	else if ( file.IsWriter() )
	{
		if ( serializationFlags & eSerializationFlag_CookedStream )
		{
			return WriteDataStream( owner, file, data, defaultData, defaultDataClass );
		}
		else
		{
			return WritePropertyList( owner, file, data, defaultData, defaultDataClass );
		}
	}

	// Done
	return true;
}

Bool CClass::NeedsCleaning()
{
	// Cache the list on first use
	if ( !m_needsCleaningWasChecked )
	{
		// Mark as checked
		m_needsCleaningWasChecked = true;
		m_propertiesToDestroy.ClearFast();

		// Scan properties
		const auto& properties = GetCachedProperties();
		for ( TPropertyList::const_iterator it=properties.Begin(); it!=properties.End(); ++it )
		{
			CProperty* prop = *it;
			if ( prop->GetType() && prop->GetType()->NeedsCleaning() )
			{
				m_propertiesToDestroy.PushBack( prop );
			}
		}

		m_propertiesToDestroy.Shrink();
	}

	// We need cleaning if there is at least one property that needs to be cleaned
	return !m_propertiesToDestroy.Empty();
}

Bool CClass::RecalculateGCCachedProperties( ) const
{
	if ( !m_areGCPropertiesCached )
	{
		// Mark as cached
		m_areGCPropertiesCached = true;
		m_cachedGCProperties.Clear();

		// Test local properties
		const auto& properties = GetCachedProperties();
		for ( Uint32 i=0; i<properties.Size(); ++i )
		{

			CProperty* prop = properties[i];
			if ( prop && prop->GetType()->NeedsGC() )
			{
				m_cachedGCProperties.PushBack( prop );
			}
		}
	}

	// Done
	return true;
}

Bool CClass::NeedsGC()
{
	RecalculateGCCachedProperties();
	return !m_cachedGCProperties.Empty();
}

const CClass::TPropertyList& CClass::GetCachedGCProperties( ) const
{
	RecalculateGCCachedProperties();
	return m_cachedGCProperties;	
}

// Could be per-class (probably deadlock, but w/e.)
static Red::Threads::CMutex stupidGCMutex;

namespace Helper
{
	void CollectLocalProperties( const CClass* theClass, TDynArray< CProperty*, MC_RTTI >& allProps )
	{
		if ( theClass && theClass->HasBaseClass() )
		{
			CollectLocalProperties( theClass->GetBaseClass(), allProps );
		}

		for ( CProperty* prop : theClass->GetLocalProperties() )
		{
			allProps.PushBack( prop );
		}
	}
}

void CClass::RecalculateCachedProperties( const Bool force ) const
{
	if ( !m_arePropertiesCached || force )
	{
		m_cachedProperties.ClearFast();
		Helper::CollectLocalProperties( this, m_cachedProperties );

		m_cachedProperties.Shrink();
		m_arePropertiesCached = true;
		m_areGCPropertiesCached = false;
	}
}

void CClass::RecalculateCachedScriptPropertiesToDestroy()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > scopedLock( stupidGCMutex );

	m_cachedScriptPropertiesToDestory.ClearFast();

	const auto& allProps = GetCachedProperties();
	for ( CProperty* prop : allProps )
	{
		if ( (( IsScripted() && !HasBaseClass()) || prop->IsScripted()) && prop->GetType()->NeedsCleaning() )
		{
			PropInfo propInfo;
			propInfo.m_offset = prop->GetDataOffset();
			propInfo.m_type = prop->GetType();

#ifndef RED_FINAL_BUILD
			propInfo.m_prop = prop;
			propInfo.m_name = prop->GetName().AsAnsiChar();
#endif

			m_cachedScriptPropertiesToDestory.PushBack( propInfo );
		}
	}

	m_cachedScriptPropertiesToDestory.Shrink();
	m_areScriptPropertiesToDestroyCached = true;
}

void CClass::RecalculateFunctions()
{
	CClass* c = this;
	while ( c )
	{
		for ( Uint32 i = 0; i < c->m_localFunctions.Size(); ++i )
		{
			m_allFunctionsMap.Insert( c->m_localFunctions[i]->GetName(), c->m_localFunctions[i] );
		}
		c = c->GetBaseClass();
	}
}

const CClass::TPropertyList& CClass::GetCachedProperties() const
{
	RecalculateCachedProperties();
	return m_cachedProperties;
}

const CClass::TDestroyPropList& CClass::GetCachedScriptPropertiesToDestroy( Bool& outIsValid ) const
{
	if ( m_areScriptPropertiesToDestroyCached )
	{
		outIsValid = true;
		return m_cachedScriptPropertiesToDestory;
	}

	static CClass::TDestroyPropList emptyList;
	return emptyList;
}

CScriptedClass::CScriptedClass( const CName& name, Uint32 flags )
	: CClass( name, 0, flags | CF_Scripted )
	, m_defaultObject( NULL )
{
	// we no longer can assume that scripted class derives from CObject,
}

EMemoryClass CScriptedClass::GetMemoryClass() const
{
	if ( HasBaseClass() )
	{
		CClass* baseClass = GetBaseClass();
		return baseClass->GetMemoryClass();
	}
	else
	{
		return MC_ScriptObject;
	}
}

EMemoryPoolLabel CScriptedClass::GetMemoryPool() const
{
	if ( HasBaseClass() )
	{
		CClass* baseClass = GetBaseClass();
		return baseClass->GetMemoryPool();
	}
	else
	{
		return Memory::GetPoolLabel< MemoryPool_CObjects >();
	}
}

void* CScriptedClass::AllocateRelatedMemory( Uint32 size, Uint32 alignment ) const
{
	if ( HasBaseClass() )
	{
		CClass* baseClass = GetBaseClass();
		return baseClass->AllocateRelatedMemory( size, alignment );
	}
	else
	{
#ifndef RED_FINAL_BUILD
		CoreMemory::ScopedMemoryDebugString memoryAllocationTag( StringAnsi::Printf( "CScriptedClass:%s\n", GetName().AsAnsiChar() ).AsChar() );
#endif
		return RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_CObjects, MC_ScriptObject, size, alignment );
	}
}

void CScriptedClass::FreeRelatedMemory( void* mem ) const
{
	if ( HasBaseClass() )
	{
		CClass* baseClass = GetBaseClass();
		baseClass->FreeRelatedMemory( mem );
	}
	else
	{
		RED_MEMORY_FREE( MemoryPool_CObjects, MC_ScriptObject, mem );
	}
}

void * CScriptedClass::AllocateClass() const
{	
	if ( HasBaseClass() )
	{
		CClass* baseClass = GetBaseClass();
		return baseClass->AllocateClass();
	}
	else
	{
#ifndef RED_FINAL_BUILD
		// Mark allocations with class name
		CoreMemory::ScopedMemoryDebugString memoryAllocationTag( StringAnsi::Printf( "CScriptedClass:%s\n", GetName().AsAnsiChar() ).AsChar() );
#endif
		return RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_CObjects, MC_ScriptObject, GetSize(), GetAlignment() );
	}
}

void CScriptedClass::FreeClass( void * mem ) const
{
	if ( HasBaseClass() )
	{
		CClass* baseClass = GetBaseClass();
		baseClass->FreeClass( mem );
	}
	else
	{
		RED_MEMORY_FREE( MemoryPool_CObjects, MC_ScriptObject, mem );
	}
}

void CScriptedClass::Construct( void* mem, Bool isDefaultObject ) const
{
	// Pass down the hierarchy to let the first native class initialize the object with native data
	if ( HasBaseClass() )
	{
		CClass* baseClass = GetBaseClass();
		baseClass->Construct( mem, isDefaultObject );
	}

	// Apply default values
	const Bool createInlinedObjects = !isDefaultObject;
	ApplyDefaultValues( mem, createInlinedObjects, false );

	ConstructProperties( mem, isDefaultObject );
}

void CScriptedClass::Construct( void* mem ) const
{
	Construct( mem, false );
}

void CScriptedClass::Destruct( void* mem ) const
{
	// Let the base class initialize script object
	if ( HasBaseClass() )
	{
		CClass* baseClass = GetBaseClass();
		baseClass->Destruct( mem );
	}
	else
	{
		for ( CProperty* prop : m_propertiesToDestroy )
		{
			prop->GetType()->Destruct( prop->GetOffsetPtr( mem ) );
		}
	}
}

Bool CScriptedClass::Compare( const void* data1, const void* data2, Uint32 ) const
{
	return CClass::DeepCompare( data1, data2 );
}

void CScriptedClass::Copy( void* dest, const void* src ) const
{
	RED_ASSERT( !HasBaseClass() );

	// Copy property by property
	for ( Uint32 i=0; i<m_localProperties.Size(); i++ )
	{
		CProperty* prop = m_localProperties[i];
		const void* srcPropData = prop->GetOffsetPtr( src );
		void* destPropData = prop->GetOffsetPtr( dest );
		prop->GetType()->Copy( destPropData, srcPropData );
	}
}

void CScriptedClass::Clean( void* data ) const
{
	Destruct( data );
	Construct( data );
}

void* CScriptedClass::GetDefaultObjectImp() const
{
	if ( !m_defaultObject )
	{
		// Create default object in zero initialized memory
		const Bool callConstructor = false;
		m_defaultObject = CreateObject( 0, callConstructor ); // we call the constructor manually
		Construct( m_defaultObject, true );

		// Register objects in the root set
		if ( IsObject() )
		{
			// Register in the root set to keep it alive
			void AddDefaultObjectToRootSet( CObject* obj );
			AddDefaultObjectToRootSet( (CObject*)m_defaultObject );
		}
	}

	return m_defaultObject;
}

void CScriptedClass::DestroyDefaultObject()
{
	if ( m_defaultObject )
	{
		if ( IsObject() )
		{
			// Remove the object from the root set, it will be deleted by GC
			void RemoveFromRootSet( CObject* obj );
			RemoveFromRootSet( (CObject*)m_defaultObject );
		}

		// Free memory
		//DestroyObject( this, m_defaultObject );
		m_defaultObject = NULL;
	}
}

void CScriptedClass::ConstructProperties( void* mem, Bool isDefaultObject ) const
{
	for ( CProperty* prop : m_localProperties )
	{
		CClass* propClass = SRTTI::GetInstance().FindClass( prop->GetType()->GetName() );
		if( propClass )
		{
			propClass->Construct( prop->GetOffsetPtr( mem ), isDefaultObject );
		}
	}
}

CAbstractClass::CAbstractClass( const CName& name, Uint32 size, Uint32 flags )
	: CClass( name, size, flags | CF_Abstract | CF_Native )
{
}

void* CAbstractClass::AllocateRelatedMemory( Uint32, Uint32 ) const 
{
	HALT( "Cannot instantiate abstact classes !" );
	return nullptr;
}

void CAbstractClass::FreeRelatedMemory( void* ) const
{
	HALT( "Cannot instantiate abstact classes !" );
}

void * CAbstractClass::AllocateClass() const 
{
	HALT( "Cannot instantiate abstact classes !" );
	return nullptr;
}

void CAbstractClass::FreeClass( void * ) const
{
	HALT( "Cannot instantiate abstact classes !" );
}

EMemoryClass CAbstractClass::GetMemoryClass() const
{
	HALT( "Cannot instantiate abstact classes !" );
	return MC_Default;
}

EMemoryPoolLabel CAbstractClass::GetMemoryPool() const
{
	HALT( "Cannot instantiate abstact classes !" );
	return Memory::GetPoolLabel< MemoryPool_Default >();
}

void CAbstractClass::Construct( void*, Bool ) const
{
	RED_HALT( "Cannot instantiate abstact classes !" );
}

void CAbstractClass::Construct( void* ) const
{
	RED_HALT( "Cannot instantiate abstact classes !" );
}

void CAbstractClass::Destruct( void* ) const
{
	RED_HALT( "Cannot destroy abstact classes !" );
}

Bool CAbstractClass::Compare( const void*, const void*, Uint32 ) const
{
	RED_HALT( "Cannot operate on abstact classes!" );
	return false;
}

void CAbstractClass::Copy( void*, const void* ) const
{
	RED_HALT( "Cannot operate on abstact classes!" );
}

void CAbstractClass::Clean( void* ) const
{
	RED_HALT( "Cannot operate on abstact classes!" );
}

void* CAbstractClass::GetDefaultObjectImp() const
{
	RED_HALT( "Cannot operate on abstract classes!" );
	return NULL;
}




/*                  .+~                :xx++::
                   :`. -          .!!X!~"?!`~!~!. :-:.
                  <             .!!!H":.~ ::+!~~!!!~ `%X.
                  '             ~~!M!!>!!X?!!!!!!!!!!...!~.
                              <!:!MM!~:XM!!!!!!.:!..~ !.  `<
                  <: `   :~ .:<~!!M!XXHM!!!X!XXHtMMHHHX!  ~ ~
                ~~~~<' ~!!!:!!!!!XM!!M!!!XHMMMRMSXXX!!!!!!:  <`
                  `<  <::!!!!!X!X?M!!M!!XMMMMXXMMMM??!!!!!?!:~<
               : '~~~<!!!XMMH!!XMXMXHHXXXXM!!!!MMMMSXXXX!!!!!!!~
            :    ::`~!!!MMMMXXXtMMMMMMMMMMMHX!!!!!!HMMMMMX!!!!!: ~
               '~:~!!!!!MMMMMMMMMMMMMMMMMMMMMMXXX!!!M??MMMM!!X!!i:
               <~<!!!!!XMMMMMMMMMMMM8M8MMMMM8MMMMMXX!!!!!!!!X!?t?!:
               ~:~~!!!!?MMMMMM@M@RMRRR$@@MMRMRMMMMMMXSX!!!XMMMX<?X!
             :XX <!!XHMMMM88MM88BR$M$$$$8@8RN88MMMMMMMMHXX?MMMMMX!!!
           .:X! <XMSM8M@@$$$$$$$$$$$$$$$$$$$B8R$8MMMMMMMMMMMMMMMMX!X
          :!?! !?XMMMMM8$$$$8$$$$$$$$$$$$$$BBR$$MMM@MMMMMMMMMMMMMM!!X
        ~<!!~ <!!XMMMB$$$$$$$$$$$$$$$$$$$$$$$$MMR$8MR$MMMMMMMMMMMMM!?!:
        :~~~ !:X!XMM8$$$$$$$$$$$$$$$$$$$$$$$RR$$MMMMR8NMMMMMMMMMMMMM<!`-
    ~:<!:~`~':!:HMM8N$$$$$$$$$$$$$$$$$$$$$$$$$8MRMM8R$MRMMMMMMMMRMMMX!
  !X!``~~   :~XM?SMM$B$$$$$$$$$$$$$$$$$$$$$$BR$$MMM$@R$M$MMMMMM$MMMMX?L
 X~.      : `!!!MM#$RR$$$$$$$$$$$$$$$$$R$$$$$R$M$MMRRRM8MMMMMMM$$MMMM!?:
 ! ~ <~  !! !!~`` :!!MR$$$$$$$$$$RMM!?!??RR?#R8$M$MMMRM$RMMMM8MM$MMM!M!:>
: ' >!~ '!!  !   .!XMM8$$$$$@$$$R888HMM!!XXHWX$8$RM$MR5$8MMMMR$$@MMM!!!< ~
!  ' !  ~!! :!:XXHXMMMR$$$$$$$$$$$$$$$$8$$$$8$$$MMR$M$$$MMMMMM$$$MMM!!!!
 ~<!!!  !!! !!HMMMMMMMM$$$$$$$$$$$$$$$$$$$$$$$$$$MMM$M$$MM8MMMR$$MMXX!!!!/:`
  ~!!!  !!! !XMMMMMMMMMMR$$$$$$$$$$$$R$RRR$$$$$$$MMMM$RM$MM8MM$$$M8MMMX!!!!:
  !~ ~  !!~ XMMM%!!!XMMX?M$$$$$$$$B$MMSXXXH?MR$$8MMMM$$@$8$M$B$$$$B$MMMX!!!!
  ~!    !! 'XMM?~~!!!MMMX!M$$$$$$MRMMM?!%MMMH!R$MMMMMM$$$MM$8$$$$$$MR@M!!!!!
  <>    !!  !Mf x@#"~!t?M~!$$$$$RMMM?Xb@!~`??MS$M@MMM@RMRMMM$$$$$$RMMMMM!!!!
  !    '!~ <!!:!?M   !@!M<XM$$R5M$8MMM$! -XXXMMRMBMMM$RMMM@$R$BR$MMMMX??!X!!
  !    '!  !!X!!!?::xH!HM:MM$RM8M$RHMMMX...XMMMMM$RMMRRMMMMMMM8MMMMMMMMX!!X!
  !     ~  !!?:::!!!MXMR~!MMMRMM8MMMMMS!!M?XXMMMMM$$M$M$RMMMM8$RMMMMMMMM%X!!
  ~     ~  !~~X!!XHMMM?~ XM$MMMMRMMMMMM@MMMMMMMMMM$8@MMMMMMMMRMMMMM?!MMM%HX!
           !!!!XSMMXXMM .MMMMMMMM$$$BB8MMM@MMMMMMMR$RMMMMMMMMMMMMMMMXX!?H!XX
           XHXMMMMMMMM!.XMMMMMMMMMR$$$8M$$$$$M@88MMMMMMMMMMMMMMM!XMMMXX!!!XM
      ~   <!MMMMMMMMRM:XMMMMMMMMMM8R$$$$$$$$$$$$$$$NMMMMMMMM?!MM!M8MXX!!/t!M
      '   ~HMMMMMMMMM~!MM8@8MMM!MM$$8$$$$$$$$$$$$$$8MMMMMMM!!XMMMM$8MR!MX!MM
          'MMMMMMMMMM'MM$$$$$MMXMXM$$$$$$$$$$$$$$$$RMMMMMMM!!MMM$$$$MMMMM<!M
          'MMMMMMMMM!'MM$$$$$RMMMMMM$$$$$$$$$$$$$$$MMM!MMMX!!MM$$$$$M$$M$M!M
           !MMMMMM$M! !MR$$$RMM8$8MXM8$$$$$$$$$$$$NMMM!MMM!!!?MRR$$RXM$$MR!M
           !M?XMM$$M.< !MMMMMMSUSRMXM$8R$$$$$$$$$$#$MM!MMM!X!t8$M$MMMHMRMMX$
    ,-,   '!!!MM$RMSMX:.?!XMHRR$RM88$$$8M$$$$$R$$$$8MM!MMXMH!M$$RMMMMRNMMX!$
   -'`    '!!!MMMMMMMMMM8$RMM8MBMRRMR8RMMM$$$$8$8$$$MMXMMMMM!MR$MM!M?MMMMMM$
          'XX!MMMMMMM@RMM$MM@$$BM$$$M8MMMMR$$$$@$$$$MM!MMMMXX$MRM!XH!!??XMMM
          `!!!M?MHMMM$RMMMR@$$$$MR@MMMM8MMMM$$$$$$$WMM!MMMM!M$RMM!!.MM!%M?~!
           !!!!!!MMMMBMM$$RRMMMR8MMMMMRMMMMM8$$$$$$$MM?MMMM!f#RM~    `~!!!~!
           ~!!HX!!~!?MM?MMM??MM?MMMMMMMMMRMMMM$$$$$MMM!MMMM!!
           '!!!MX!:`~~`~~!~~!!!!XM!!!?!?MMMM8$$$$$MMMMXMMM!!
            !!~M@MX.. <!!X!!!!XHMHX!!``!XMMMB$MM$$B$M!MMM!!
            !!!?MRMM!:!XHMHMMMMMMMM!  X!SMMX$$MM$$$RMXMMM~
             !M!MMMM>!XMMMMMMMMXMM!!:!MM$MMMBRM$$$$8MMMM~
             `?H!M$R>'MMMM?MMM!MM6!X!XM$$$MM$MM$$$$MX$f
              `MXM$8X MMMMMMM!!MM!!!!XM$$$MM$MM$$$RX@"
               ~M?$MM !MMMMXM!!MM!!!XMMM$$$8$XM$$RM!`
                !XMMM !MMMMXX!XM!!!HMMMM$$$$RH$$M!~
                'M?MM `?MMXMM!XM!XMMMMM$$$$$RM$$#
                 `>MMk ~MMHM!XM!XMMM$$$$$$BRM$M"
                  ~`?M. !M?MXM!X$$@M$$$$$$RMM#
                    `!M  !!MM!X8$$$RM$$$$MM#`
                      !% `~~~X8$$$$8M$$RR#`
                       !!x:xH$$$$$$$R$R*`
                        ~!?MMMMRRRM@M#`       -Magic starts here-
                         `~???MMM?M"`
                             ``~~ */

/// Meta template recursion depth
#define CLASS_RTTI_META_RECURSION_DEPTH		16

/// Meta Templated IsA function ( way faster )
template< int N >
RED_FORCE_INLINE Bool IsA_Meta( const CClass* thisClass, const CClass* testClass )
{
	// Direct class match
	if ( testClass == thisClass )
	{
		return true;
	}

	// Test root class
	const CClass* baseClass = thisClass->GetBaseClass();
	if ( baseClass && IsA_Meta<N-1>( baseClass, testClass ) )
	{
		return true;
	}

	// Not a base class
	return false;
}

/// Default level-0 implementation
template<>
RED_FORCE_INLINE Bool IsA_Meta<0>( const CClass*, const CClass* )
{
	// Note: normally, this should recurse to normal IsA implementation
	// but since the class tree is not infinite we can safely drop the recursion.
	HALT( "Class tree to deep. Increase the meta template recursion depth." );
	return false;
}

Bool CClass::IsA_OldTest( const CClass *otherClass ) const
{
	return IsA_Meta< CLASS_RTTI_META_RECURSION_DEPTH >( this, otherClass );
}

Bool CClass::DynamicIsA( const CClass* testedClass ) const
{
	return IsA_Meta< CLASS_RTTI_META_RECURSION_DEPTH >( this, testedClass );
}

Bool CClass::IsBasedOn( const CClass* parentClass ) const
{
	// old test
	const Bool oldTest = IsA_Meta< CLASS_RTTI_META_RECURSION_DEPTH >( this, parentClass );

	// new test
	const Bool newTest = parentClass && (m_classIndex >= parentClass->m_classIndex && m_classIndex <= parentClass->m_lastChildClassIndex);
	RED_ASSERT( oldTest == newTest, TXT("IsA test failed. Please tell Dex!!!!") );
	RED_UNUSED( newTest );

	return oldTest;
}

/// Meta template UpCasting
template< int N >
RED_FORCE_INLINE void* UpCast_Meta( const CClass* destClass, const CClass* thisClass, void* thisObj )
{
	// Target class
	if ( destClass == thisClass )
	{
		return thisObj;
	}

	// Go to base classes for "this class", if any of it returns valid pointer cast it to "this class"
	const CClass* baseClass = thisClass->GetBaseClass();
	if ( baseClass )
	{
		void* basePtrNotYetCasted = UpCast_Meta<N-1>( destClass, baseClass, thisObj );
		if ( basePtrNotYetCasted )
		{
			return thisClass->UpCastUnsafe( basePtrNotYetCasted );
		}
	}

	// No cast
	return NULL;
}

/// Default level-0 implementation
template<>
RED_FORCE_INLINE void* UpCast_Meta<0>( const CClass*, const CClass*, void* )
{
	// Note: normally, this should recurse to normal UpCast implementation
	// but since the class tree is not infinite we can safely drop the recursion.
	HALT( "Class tree to deep. Increase the meta template recursion depth." );
	return NULL;
}

void* CClass::CastTo( const CClass* destClass, void* obj ) const
{	
	// Call the magic
	return UpCast_Meta< CLASS_RTTI_META_RECURSION_DEPTH >( destClass, this, obj );
}

/// Meta template DownCasting
template< int N >
RED_FORCE_INLINE void* DownCast_Meta( const CClass* destClass, const CClass* thisClass, void* thisObj )
{
	// Target class reached !
	if ( destClass == thisClass )
	{
		return thisObj;
	}

	// Go to base classes for "dest class"
	const CClass* baseClass = destClass->GetBaseClass();
	if ( baseClass )
	{
		void* basePtrNotYetCasted = DownCast_Meta<N-1>( baseClass, thisClass, thisObj );
		if ( basePtrNotYetCasted )
		{
			return destClass->DownCastUnsafe( basePtrNotYetCasted );
		}
	}

	// No cast
	return NULL;
}

/// Default level-0 implementation
template<>
RED_FORCE_INLINE void* DownCast_Meta<0>( const CClass*, const CClass*, void* )
{
	// Note: normally, this should recurse to normal UpCast implementation
	// but since the class tree is not infinite we can safely drop the recursion.
	HALT(  "Class tree to deep. Increase the meta template recursion depth." );
	return NULL;
}

void* CClass::CastFrom( const CClass* srcClass, void* obj ) const
{	
	// Do the magic
	return DownCast_Meta< CLASS_RTTI_META_RECURSION_DEPTH >( this, srcClass, obj );
}
