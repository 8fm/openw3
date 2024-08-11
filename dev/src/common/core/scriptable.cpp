/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptable.h"
#include "scriptStackFrame.h"

//#define DUMP_CAPTURED_SCRIPTABLES

IMPLEMENT_RTTI_CLASS( IScriptable );

IScriptable::IScriptable()
	: m_scriptMachineData( nullptr )
	, m_scriptData( nullptr )
{
	// when object was not allocated via the CClass::CreateObject the m_class field will be empty
	if ( nullptr == m_class )
	{
		// in thas case we know its a fully C++ object
		m_class = GetNativeClass();
	}
	else
	{
		// create the scriptable data buffer, requires propper m_class field so be carefull how
		CreateScriptPropertiesBuffer();
	}
}

IScriptable::~IScriptable()
{
	// release script data buffers
	ReleaseScriptPropertiesBuffer();

	// release the state machine data (will kill all states)
	if ( m_scriptMachineData != nullptr )
	{
		delete m_scriptMachineData;
		m_scriptMachineData = nullptr;
	}
}

String IScriptable::GetFriendlyName() const
{
	return GetClass()->GetName().AsString();
}

void IScriptable::funcToString( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRING( GetFriendlyName() );
}

const CFunction* IScriptable::FindFunction( IScriptable*& context, CName functionName, const Bool allowStates /*= true*/ ) const
{
	// initial context if required to be that of the object
	ASSERT( context == this );

	// state machine override
	if ( allowStates && m_scriptMachineData != nullptr )
	{
		const CFunction* func = CScriptableStateMachine::FindStateFunction( context, functionName );
		if ( nullptr != func )
		{
			return const_cast< CFunction* >( func );
		}
	}

	// normal function
	return GetClass()->FindFunction( functionName );
}

Bool IScriptable::FindEvent( CName functionName ) const
{
	const CFunction* function = NULL;
	IScriptable* context = const_cast< IScriptable* >( this );
	return ::FindFunction( context, functionName, function );
}

void IScriptable::funcGetClass( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_NAME( GetClass()->GetName() );
}

void IScriptable::funcIsA( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, className, CName::NONE );
	FINISH_PARAMETERS;

	CClass* testClass = SRTTI::GetInstance().FindClass( className );

	RETURN_BOOL( IsA( testClass ) );
}

void IScriptable::funcIsExactlyA( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, className, CName::NONE );
	FINISH_PARAMETERS;
	RETURN_BOOL( GetClass()->GetName() == className );
}

void IScriptable::OnSerialize( IFile& file )
{
	ISerializable::OnSerialize( file );

	if ( file.IsGarbageCollector() )
	{
		DumpStatesForGC( file );
	}
}

void IScriptable::OnScriptPreCaptureSnapshot()
{
}

void IScriptable::OnScriptPostCaptureSnapshot()
{
}

void IScriptable::OnScriptReloaded()
{
}

IScriptable* IScriptable::GetStateMachineOwner() const
{
	// this is a little tricky unfortunatelly :(
	return const_cast< IScriptable* >( this );
}

CScriptableStateMachine::SActiveData* IScriptable::GetStateMachineData( const Bool createIfNotFound /*= false*/ ) const
{
	// create the state machine data
	if ( nullptr == m_scriptMachineData && createIfNotFound )
	{
		m_scriptMachineData = new CScriptableStateMachine::SActiveData();
	}
	
	// already exits
	return m_scriptMachineData;
}

Bool IScriptable::IsAutoBindingPropertyTypeSupported( const IRTTIType* type ) const
{
	RED_UNUSED( type );
	return false;
}

Bool IScriptable::ValidateAutoBindProperty( const CProperty* autoBindProperty, CName bindingName ) const
{
	RED_UNUSED( autoBindProperty );
	RED_UNUSED( bindingName );
	return false;
}

Bool IScriptable::ResolveAutoBindProperty( const CProperty* autoBindProperty, CName bindingName, void* resultData ) const
{
	RED_UNUSED( autoBindProperty );
	RED_UNUSED( bindingName );
	RED_UNUSED( resultData );
	return false;
}

void IScriptable::ReleaseScriptPropertiesBuffer()
{
	if ( m_scriptData != nullptr )
	{
		// destroy scripted properties
		bool hasCachedProperties = false;
#ifdef NO_EDITOR
		const auto& cachedProperties = m_class->GetCachedScriptPropertiesToDestroy( hasCachedProperties );
		if ( hasCachedProperties )
		{
			// use much faster cached list
			for ( Uint32 i=0; i<cachedProperties.Size(); ++i )
			{
				const CClass::PropInfo& info = cachedProperties[i];

#ifndef RED_FINAL_BUILD
				RED_FATAL_ASSERT( info.m_offset == info.m_prop->GetDataOffset(), "Invalid data offset at property %s (actual: %d, captured:%d)",
					info.m_name, info.m_prop->GetDataOffset(), info.m_offset );
#endif

				void* propData = OffsetPtr( m_scriptData, info.m_offset );
				info.m_type->Destruct( propData );
			}
		}
		else
#endif
		{
			// use default method
			const auto& allProps = m_class->GetCachedProperties();
			for ( const CProperty* prop : allProps )
			{
				if ( prop->IsScripted() )
				{
					void* propData = prop->GetOffsetPtr( this );
					prop->GetType()->Destruct( propData );
				}
			}

		}

		// free script data buffer
		m_class->FreeRelatedMemory( m_scriptData );
		m_scriptData = nullptr;
	}


	if ( m_scriptMachineData != nullptr )
	{
		m_scriptMachineData->m_stateStack.Clear();
		m_scriptMachineData->m_spawnedStates.Clear();

		delete m_scriptMachineData;
		m_scriptMachineData = nullptr;
	}
}

void IScriptable::CreateScriptPropertiesBuffer()
{
	// Process only scripted classes
	const Uint32 scriptDataSize = m_class->GetScriptDataSize();
	if ( scriptDataSize > 0 )
	{
		// Recreate data buffer
		if ( nullptr == m_scriptData )
		{
			m_scriptData = m_class->AllocateRelatedMemory( scriptDataSize, 4 ); // we allow the class to allocate memory (so we get the right memory class)
			Red::System::MemorySet( m_scriptData, 0, scriptDataSize );

			// construct scripted properties
			/*const auto& properties = m_class->GetCachedProperties();
			for ( Uint32 i=0; i<properties.Size(); i++ )
			{
				CProperty* prop = properties[i];
				if ( prop->IsScripted() )		// ToDo (Performance): Build table of properties to construct in class type, rather than hitting property RTTI types
				{
					void* propData = prop->GetOffsetPtr( this );
					prop->GetType()->Construct( propData );
				}
			}*/
		}
	}
}

class CScriptableCollector : public IFile
{
private:
	THashMap< void*, Uint32 >				m_visitedObjects;
	TDynArray< THandle< IScriptable > >*	m_outScriptableObjects;

public:
	CScriptableCollector( TDynArray< THandle< IScriptable > >& outArray )
		: IFile( FF_Writer | FF_GarbageCollector | FF_ScriptCollector )
		, m_outScriptableObjects( &outArray )
	{
		m_outScriptableObjects->Reserve( 65536 );
	}

	// dead IFile interface (we are interested only in pointers)
	virtual void Serialize( void* , size_t ) {};
	virtual Uint64 GetOffset() const { return 0; }
	virtual Uint64 GetSize() const { return 0; }
	virtual void Seek( Int64  ) {};
	virtual void SerializeName( class CName& ) {};
	virtual void SerializeSoftHandle( class BaseSoftHandle& ) {};
	virtual void SerializeTypeReference( class IRTTIType*& ) {}

	virtual void SerializePointer( const class CClass* pointerClass, void*& pointer )
	{
		if ( pointer != nullptr )
		{
			Uint32 visited = 0;
			if ( !m_visitedObjects.Find( pointer, visited ) || !visited )
			{
				// mark as visited
				m_visitedObjects.Insert( pointer, 1 );

				// if it's an scriptable object store it
				if ( pointerClass->IsSerializable() )
				{
					// collect
					AddSerializable( static_cast<ISerializable*>( pointer ) );
				}
				else
				{
					// static recurse
					pointerClass->Serialize( *this, pointer );
				}
			}
		}
	}

	void AddSerializable( ISerializable* object )
	{
		// objects will be unique, we can skip the test part
		m_visitedObjects.Insert( object, 1 );

		// output scriptables
		if ( object->IsA< IScriptable >() )
		{
			// all CObjects are serializables
#ifdef DUMP_CAPTURED_SCRIPTABLES
			LOG_CORE( TXT("Captured scriptable[%d] 0x%LLX, '%ls'"), m_outScriptableObjects->Size(), (Uint64)object, object->GetClass()->GetName().AsChar() );
#endif
			m_outScriptableObjects->PushBack( static_cast< IScriptable*>( object ) );

#ifdef DUMP_CAPTURED_SCRIPTABLES
			if ( object->GetClass()->GetName() == TXT("W3PlayerWitcherStateCombatFists") )
			{
				const TDynArray< CProperty* >& allProps = object->GetClass()->GetCachedProperties();
				for ( Uint32 i=0; i<allProps.Size(); ++i )
				{
					CProperty* prop = allProps[i];
					IRTTIType* propType = prop->GetType();

					if ( propType->GetType() == RT_Handle )
					{
						const BaseSafeHandle* val = (const BaseSafeHandle*) prop->GetOffsetPtr( object );
						LOG_CORE
						(
							TXT("PreTest dump '%ls' (0x%llX) = 0x%llX, '%ls')"),
							prop->GetName().AsChar(),
							(Uint64)val, 
							val ? val->Get() : 0, (val && val->Get()) ? val->Get()->GetClass()->GetName().AsChar() : TXT("NULL")
						);
					}
				}
			}
#endif
		}

		// serialize the object to get more pointers
		object->OnSerialize( *this );
	}

	void AddObject( CObject* object )
	{
		AddSerializable( object );		
	}
};

void IScriptable::CollectAllScriptableObjects( TDynArray< THandle< IScriptable > >& outScriptables )
{
	CScriptableCollector collector( outScriptables );

	// right now lets just iterate over all CObjects...
	for( BaseObjectIterator it; it; ++it )
	{
		CObject* object = *it;
		collector.AddObject( object );
	}
}

void IScriptable::CollectAllDefaultScriptableObjects( TDynArray< THandle< IScriptable > >& outScriptables )
{
	CScriptableCollector collector( outScriptables );

	// right now lets just iterate over all CObjects...
	const EObjectFlags excludeFilter = static_cast< EObjectFlags >( ~OF_DefaultObject & BaseObjectIterator::DEFAULT_EXCLUDE_FLAGS );
	for ( BaseObjectIterator it( BaseObjectIterator::DEFAULT_INCLUDE_FLAGS, excludeFilter ); it; ++it )
	{
		CObject* object = *it;
		collector.AddObject( object );
	}
}
