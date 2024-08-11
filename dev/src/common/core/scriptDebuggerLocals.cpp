/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifdef RED_NETWORK_ENABLED

#include "scriptingSystem.h"
#include "scriptDebugger.h"
#include "scriptableState.h"
#include "scriptableStateMachine.h"
#include "namesRegistry.h"
#include "engineTime.h"
#include "scriptStackFrame.h"

#include "../redNetwork/manager.h"

// SHOULD MATCH enumerator in ScriptStudio
enum EScriptDebugLocalIcon
{
	SDLI_PublicMember,
	SDLI_PrivateMember,
	SDLI_Object,
	SDLI_StateMachine,
	SDLI_State,
	SDLI_Component,
};

class IScriptDebugVariable
{
public:
	String		m_name;
	void*		m_data;

public:
	IScriptDebugVariable( const String& name, void* data )
		: m_name( name )
		, m_data( data )
	{};

	virtual ~IScriptDebugVariable()
	{}

	virtual Bool CanBeModifiedByDebugger() const { return false; }
	virtual Bool ModifyValue( const String& ) { return false; }

	virtual String GetName() const { return m_name; }
	virtual String GetValue() const = 0;
	virtual EScriptDebugLocalIcon GetIcon() { return SDLI_PublicMember; }
	virtual bool IsExpandable() const { return false; }
	virtual void EnumerateChildren( TDynArray< String >& /*children*/ ) const { }
	virtual IScriptDebugVariable* GetChild( const String& /*name*/ ) const { return NULL; }

	void GetImportedProperties( CClass* theClass, TDynArray< CProperty* >& properties ) const
	{
		// Get all properties from the class
		TDynArray< CProperty* > allProperties;
		theClass->GetProperties( allProperties );

		// Filter only the exporter properties
		for ( Uint32 i = 0; i < allProperties.Size(); ++i )
		{
			CProperty* prop = allProperties[i];
			if ( GScriptingSystem->IsDebugFlagSet( CScriptingSystem::DF_UnfilteredLocals ) || ( prop->IsExported() || prop->IsScripted() || ( !prop->IsNative() ) ) )
			{
				properties.PushBack( prop );
			}
		}
	}

	String GetStructValue( CClass* structType, const void* data ) const
	{
		// Header
		String ret;// = structType->GetName().AsChar();
		ret += TXT("{");

		// Add properties
		TDynArray< CProperty* > properties;
		GetImportedProperties( structType, properties );
		for ( Uint32 i=0; i<properties.Size(); i++ )
		{
			// Separator
			if ( i )
			{
				ret += TXT(", ");
			}

			// Add property name
			CProperty* prop = properties[i];
			ret += prop->GetName().AsString().AsChar();
			ret += TXT("=");

			// Get property data
			const void* propData = prop->GetOffsetPtr( data );

			// Structure, recurse
			String value;
			IRTTIType* type = prop->GetType();
			if ( type->GetType() == RT_Class )
			{
				CClass* subStructure = (CClass*) type;
				ret += GetStructValue( subStructure, propData );
			}
			else if ( type->ToString( propData, value ) )
			{
				if ( value.Empty() )
				{
					ret += TXT("\"\"");
				}
				else
				{
					ret += value;
				}
			}
			else
			{
				ret += TXT("<Unknown>");
			}
		}

		// Tail
		ret += TXT("}");
		return ret;
	}
};

IScriptDebugVariable* CreateScriptDebugObject( const String& name, IScriptable* object );

/// States
class CScriptDebugStateMachine : public IScriptDebugVariable
{
public:
	CScriptDebugStateMachine( void* data )
		: IScriptDebugVariable( TXT("States"), data )
	{};

	virtual String GetValue() const
	{
		IScriptable* machine = (IScriptable* )m_data;

		// Get states
		TDynArray< CScriptableState* > states;
		machine->GetStates( states );

		if ( states.Size() > 1 )
		{
			const CName activeState = machine->GetCurrentStateName();
			if ( activeState )
			{
				return String::Printf( TXT("<%i states> CurrentState = %ls"), states.Size(), activeState.AsString().AsChar() );
			}
			else
			{
				return String::Printf( TXT("<%i states>"), states.Size() );
			}
		}
		else if ( states.Size() == 1 )
		{
			const CName activeState = machine->GetCurrentStateName();
			if ( activeState )
			{
				return String::Printf( TXT("<%i state> Current = %ls"), states.Size(), activeState.AsString().AsChar() );
			}
			else
			{
				return String::Printf( TXT("<%i state>"), states.Size() );
			}
		}
		else
		{
			return String::Printf( TXT("<No states>"), states.Size() );
		}
	}

	virtual EScriptDebugLocalIcon GetIcon()
	{
		return SDLI_StateMachine;
	}

	virtual bool IsExpandable() const
	{
		IScriptable* machine = (IScriptable*)m_data;

		TDynArray< CScriptableState* > states;
		machine->GetStates( states );

		return states.Size() > 0;
	}

	virtual void EnumerateChildren( TDynArray< String >& children ) const
	{
		IScriptable* machine = (IScriptable*) m_data;

		// Get states
		TDynArray< CScriptableState* > states;
		machine->GetStates( states );

		// Use state names as children
		for ( Uint32 i=0; i<states.Size(); i++ )
		{
			CScriptableState* state = states[i];
			children.PushBack( state->GetStateName().AsString() );
		}
	}

	virtual IScriptDebugVariable* GetChild( const String& name ) const
	{
		IScriptable* machine = (IScriptable* )m_data;

		CName childName( name );

		// Get states
		TDynArray< CScriptableState* > states;
		machine->GetStates( states );

		// Search for matching state
		for ( Uint32 i=0; i<states.Size(); i++ )
		{
			CScriptableState* state = states[i];
			if ( state->GetStateName() == childName )
			{
				return CreateScriptDebugObject( name, state );
			}
		}

		// Not found
		return NULL;
	}
};

/// Single property
class CScriptDebugProperty : public IScriptDebugVariable
{
public:
	Bool		m_readOnly;
	IRTTIType*	m_type;

public:
	CScriptDebugProperty( const String& name, void* propData, IRTTIType* type, Bool readOnly )
		: IScriptDebugVariable( name, propData )
		, m_type( type )
		, m_readOnly( readOnly )
	{};

	CScriptDebugProperty( CProperty* prop, void* propData )
		: IScriptDebugVariable( prop->GetName().AsString(), propData )
		, m_type( prop->GetType() )
		, m_readOnly( prop->IsReadOnly() || prop->IsNative()  )
	{};

	virtual EScriptDebugLocalIcon GetIcon()
	{
		if ( m_readOnly )
		{
			return SDLI_PrivateMember;
		}
		else
		{
			return SDLI_PublicMember;
		}
	}

	virtual bool IsExpandable() const
	{
		if ( m_type->GetType() == RT_Handle )
		{
			BaseSafeHandle& handle = *(BaseSafeHandle* )m_data;
			CObject* object = (CObject*)handle.Get();
			if ( object )
			{
				TDynArray< CProperty* > properties;
				GetImportedProperties( object->GetClass(), properties );
				return properties.Size() > 0;
			}
		}
		else if ( m_type->GetType() == RT_Class )
		{
			// Get structure
			TDynArray< CProperty* > properties;
			GetImportedProperties( (CClass*) m_type, properties );
			return properties.Size() > 0;
		}
		else if ( m_type->GetType() == RT_Array )
		{
			CRTTIArrayType* arrayType = (CRTTIArrayType* ) m_type;
			return arrayType->GetArraySize( m_data ) > 0;
		}

		// Not expandable
		return false;
	}

	virtual String GetValue() const
	{
		if ( m_type->GetType() == RT_Handle )
		{
			BaseSafeHandle& handle = *(BaseSafeHandle* )m_data;
			CObject* object = (CObject*)handle.Get();
			if ( object )
			{
				String debugName;
				if( object->GetScriptDebuggerName( debugName ) )
				{
					return String::Printf( TXT("0x%p %ls (Name: %ls)"), object, object->GetClass()->GetName().AsString().AsChar(), debugName.AsChar() );
				}
				else
				{
					return String::Printf( TXT("0x%p %ls"), object, object->GetClass()->GetName().AsString().AsChar() );
				}
			}
			else
			{
				return TXT("NULL");
			}
		}
		else if ( m_type->GetType() == RT_Class )
		{
			// Get structure
			CClass* theClass = (CClass* ) m_type;
			if( m_type->GetName() == CNAME( EngineTime ) )
			{
				return ToString( Double( *((EngineTime*)m_data) ) );
			}
			else
			{
				return GetStructValue( theClass, m_data );
			}
		}
		else if ( m_type->GetType() == RT_Array )
		{
			CRTTIArrayType* arrayType = (CRTTIArrayType* ) m_type;
			const Uint32 size = arrayType->GetArraySize( m_data );
			if ( size )
			{
				return String::Printf( TXT("[Size = %i]"), size );
			}
			else
			{
				return TXT("[Empty]");
			}
		}

		// Get default value
		String text = TXT("<Unknown>");
		m_type->ToString( m_data, text );
		return text;
	}

	virtual void EnumerateChildren( TDynArray< String >& children ) const 
	{
		if ( m_type->GetType() == RT_Handle )
		{
			BaseSafeHandle& handle = *(BaseSafeHandle* )m_data;
			CObject* object = (CObject*)handle.Get();
			if ( object )
			{
				// Get properties exposed in object
				TDynArray< CProperty* > properties;
				GetImportedProperties( object->GetClass(), properties );

				// State machine
				if ( object->GetClass()->HasStateClasses() )
				{
					children.PushBack( TXT("States") );
				}

				// State
				if ( object->IsA< CScriptableState >() )
				{
					children.PushBack( TXT("parent") );
				}

				// Use properties as children
				for ( Uint32 i=0; i<properties.Size(); i++ )
				{
					CProperty* prop = properties[i];
					children.PushBack( prop->GetName().AsString() );
				}
			}
		}
		else if ( m_type->GetType() == RT_Class )
		{
			// Get structure
			CClass* theClass = (CClass* ) m_type;

			// Get properties exposed in object
			TDynArray< CProperty* > properties;
			GetImportedProperties( theClass, properties );

			// Use properties as children
			for ( Uint32 i=0; i<properties.Size(); i++ )
			{
				CProperty* prop = properties[i];
				children.PushBack( prop->GetName().AsString().AsChar() );
			}
		}
		else if ( m_type->GetType() == RT_Array )
		{
			CRTTIArrayType* arrayType = (CRTTIArrayType* ) m_type;
			const Uint32 size = arrayType->GetArraySize( m_data );
			if ( size )
			{
				// Array elements
				for ( Uint32 i=0; i<size; i++ )
				{
					String indexName = String::Printf( TXT("[%i]"), i );
					children.PushBack( indexName );
				}
			}
		}
	}

	virtual IScriptDebugVariable* GetChild( const String& name ) const
	{
		IRTTIType* type = m_type;
		if ( type->GetType() == RT_Handle )
		{
			BaseSafeHandle& handle = *(BaseSafeHandle* )m_data;

			IScriptable* scriptableObject = (IScriptable*) handle.Get();
			if ( scriptableObject )
			{
				// State machine
				if ( name == TXT("States") )
				{
					return new CScriptDebugStateMachine( scriptableObject );
				}

				// Parent object
				if ( name == TXT("parent") )
				{
					if ( scriptableObject->IsA< CObject >() )
					{
						CObject* parentObject = static_cast< CObject* >( scriptableObject )->GetParent();
						if ( parentObject )
						{
							return CreateScriptDebugObject( TXT("parent"), parentObject );
						}
					}
					else if ( scriptableObject->IsA< CScriptableState >() )
					{
						IScriptable* parentObject = static_cast< CScriptableState* >( scriptableObject )->GetStateMachine();
						if ( parentObject )
						{
							return CreateScriptDebugObject( TXT("parent"), parentObject );
						}
					}

					return NULL;
				}

				// Normal property
				CProperty* prop = scriptableObject->GetClass()->FindProperty( CName( name ) );
				if ( prop )
				{
					void* propData = prop->GetOffsetPtr( scriptableObject );
					return new CScriptDebugProperty( prop, propData );
				}
			}
		}
		else if ( type->GetType() == RT_Class )
		{
			// Get structure property
			CClass* theClass = (CClass* ) type;
			CProperty* prop = theClass->FindProperty( CName( name ) );
			if ( prop )
			{
				void* propData = prop->GetOffsetPtr( m_data );
				return new CScriptDebugProperty( prop, propData );
			}
		}
		else if ( type->GetType() == RT_Array )
		{
			CRTTIArrayType* arrayType = (CRTTIArrayType* ) type;
			const Uint32 size = arrayType->GetArraySize( m_data );

			// Format child name
			Uint32 index = 0;
			if ( FromString( name.AsChar()+1, index ) )
			{
				if ( index < size )
				{
					void* elementData = arrayType->GetArrayElement( m_data, index );
					return new CScriptDebugProperty( name, elementData, arrayType->GetInnerType(), m_readOnly );
				}
			}

			// Not recognized as index
			return NULL;
		}

		return NULL; 
	}

	virtual Bool CanBeModifiedByDebugger() const override final
	{
		return
			m_type->GetType() == RT_Fundamental ||
			m_type->GetType() == RT_Enum ||
			m_type->GetName() == CNAME( CName ) ||
			m_type->GetName() == CNAME( String );
	}

	virtual Bool ModifyValue(const String& value )
	{
		RED_FATAL_ASSERT( CanBeModifiedByDebugger(), "Tried to modify a script debug property that does not support being modified" );

		// If it's an enum, see if they entered the value as a raw number instead of typing the named value by hand
		if( m_type->GetType() == RT_Enum )
		{
			Int32 rawNumberValue = 0;

			if( FromString( value, rawNumberValue ) )
			{
				const CEnum* rttiEnum = static_cast< const CEnum* >( m_type );

				// Make sure the number is valid for this enum
				CName enumElementName;
				if( rttiEnum->FindName( rawNumberValue, enumElementName ) )
				{
					rttiEnum->SetAsInt( m_data, rawNumberValue );
					return true;
				}
			}
		}

		return m_type->FromString( m_data, value );
	}
};

/// Object 
class CScriptDebugObject : public IScriptDebugVariable
{
public:
	CScriptDebugObject( const String& name, IScriptable* object )
		: IScriptDebugVariable( name, object )
	{};

	virtual EScriptDebugLocalIcon GetIcon()
	{
		IScriptable* object = (IScriptable* )m_data;
		if ( m_name == TXT("this") )
		{
			return SDLI_Object;
		}
		else if ( object->IsA< CScriptableState >() )
		{
			return SDLI_State;
		}
		else
		{
			return SDLI_PublicMember;
		}
	}

	virtual String GetValue() const
	{
		IScriptable* object = (IScriptable* )m_data;
		String debugName;
		if( object && object->GetScriptDebuggerName( debugName ) )
		{
			return String::Printf( TXT("0x%p %ls (Name: %ls)"), object, object->GetClass()->GetName().AsString().AsChar(), debugName.AsChar() );
		}
		else
		{			
			return String::Printf( TXT("0x%p %ls"), object, object->GetClass()->GetName().AsString().AsChar() );
		}
	}

	virtual bool IsExpandable() const
	{
		return true;
	}

	virtual void EnumerateChildren( TDynArray< String >& children ) const
	{
		IScriptable* object = (IScriptable*) m_data;

		// Get properties exposed in object
		TDynArray< CProperty* > properties;
		GetImportedProperties( object->GetClass(), properties );

		// State machine
		if ( object->GetClass()->HasStateClasses() )
		{
			children.PushBack( TXT("States") );
		}

		// State
		if ( object->IsA< CScriptableState >() )
		{
			children.PushBack( TXT("parent") );
		}

		// Use properties as children
		for ( Uint32 i=0; i<properties.Size(); i++ )
		{
			CProperty* prop = properties[i];
			children.PushBack( prop->GetName().AsString().AsChar() );
		}
	}

	virtual IScriptDebugVariable* GetChild( const String& name ) const
	{
		IScriptable* object = (IScriptable* )m_data;

		// State machine
		if ( name == TXT("States") )
		{
			return new CScriptDebugStateMachine( object );
		}

		// Parent object
		if ( name == TXT("parent") )
		{
			if ( object->IsA< CObject >() )
			{
				CObject* parentObject = static_cast< CObject* >( object )->GetParent();
				if ( parentObject )
				{
					return new CScriptDebugObject( TXT("parent"), parentObject );
				}
			}
			else if ( object->IsA< CScriptableState >() )
			{
				IScriptable* parentStateMachine = static_cast< CScriptableState* >( object )->GetStateMachine();
				if ( parentStateMachine )
				{
					return new CScriptDebugObject( TXT("parent"), parentStateMachine );
				}
			}

			return NULL;
		}

		// Basic property children
		CProperty* prop = object->GetClass()->FindProperty( CName( name ) );
		if ( prop )
		{
			void* propData = prop->GetOffsetPtr( m_data );
			return new CScriptDebugProperty( prop, propData );
		}

		// Not evaluated
		return NULL;
	}
};

IScriptDebugVariable* CreateScriptDebugObject( const String& name, IScriptable* object )
{
	return new CScriptDebugObject( name, object );
}

/// Stack frame locals
class CScriptDebugStackFrame : public IScriptDebugVariable
{
public:
	const CScriptStackFrame* m_frame;

public:
	CScriptDebugStackFrame( const CScriptStackFrame* frame )
		: IScriptDebugVariable( TXT("StackFrame"), frame->m_locals )
		, m_frame( frame )
	{};

	virtual String GetValue() const
	{
		return TXT("StackFrame");
	}

	virtual bool IsExpandable() const
	{
		return true;
	}

	virtual void EnumerateChildren( TDynArray< String >& children ) const
	{
		// We have the "this" if context is set
		if ( m_frame->GetContext() )
		{
			children.PushBack( TXT("this") );
		}

		// Add local properties from functions
		TDynArray< CProperty*, MC_RTTI > props;
		m_frame->m_function->GetProperties( props );
		for ( Uint32 i=0; i<props.Size(); i++ )
		{
			CProperty* prop = props[i];
			children.PushBack( prop->GetName().AsString().AsChar() );
		}
	}

	virtual IScriptDebugVariable* GetChild( const String& name ) const
	{
		// This
		if ( name == TXT("this") )
		{
			IScriptable* context = m_frame->GetContext();
			if( context )
			{
				return new CScriptDebugObject( name, context );
			}
			else
			{
				//This means we're in a function that isn't a member of a class (i.e. there is no 'this' pointer)
				return nullptr;
			}
		}

		// Property
		CProperty* prop = m_frame->m_function->FindProperty( CName( name ) );
		if ( prop )
		{
			void* propData = prop->GetOffsetPtr( prop->IsFuncLocal() ? m_frame->m_locals : m_frame->m_params );
			return new CScriptDebugProperty( prop, propData );
		}

		// Not created
		return NULL;
	}
};

Red::TSharedPtr< IScriptDebugVariable > CScriptDebugger::FindLocal( Uint32 stackFrameIndex, const String& path ) const
{
	Red::TSharedPtr< IScriptDebugVariable > returnVal;

	// Invalid path to process
	if ( stackFrameIndex >= m_stack.Size() )
	{
		return returnVal;
	}

	// Get the stack frame to process
	CScriptStackFrame* frame = m_stack[ stackFrameIndex ];
	RED_FATAL_ASSERT( frame, "Invalid script stack frame (index: %u)", stackFrameIndex );

	// Tokenize path
	TDynArray< String > tokens;
	path.Slice( tokens, TXT( "." ) );

	IScriptDebugVariable* cur = new CScriptDebugStackFrame( frame );
	for ( Uint32 i = 0; i < tokens.Size(); ++i )
	{
		// Get child element
		IScriptDebugVariable* newElement = cur->GetChild( tokens[i] );
		delete cur;

		// Not reached
		if ( !newElement )
		{
			RED_LOG_ERROR( RED_LOG_CHANNEL( ScriptDebugger ), TXT( "Locals: unable to process path '%" ) RED_PRIWs TXT( "', failed at '%" ) RED_PRIWs TXT( "'" ), path.AsChar(), tokens[ i ].AsChar() );
			return returnVal;
		}

		// Recurse
		cur = newElement;
	}

	returnVal.Reset( cur );
	return returnVal;
}

Bool CScriptDebugger::GetLocals( Uint32 stackFrameIndex, Uint32 stamp, const String& path, Uint32 id ) const
{
	Red::TSharedPtr< IScriptDebugVariable > cur = FindLocal( stackFrameIndex, path );

	if( !cur )
	{
		return false;
	}

	// Grab and sort the child elements of the specified local
	TDynArray< String > children;
	cur->EnumerateChildren( children );

	if( GScriptingSystem->IsDebugFlagSet( CScriptingSystem::DF_SortLocalsAlphaAsc ) && children.Size() > 2 )
	{
		Sort( children.Begin(), children.End(), []( const String& a, const String& b )
		{ 
			if ( a.BeginsWith( TXT("[") ) && b.BeginsWith( TXT("[") ) )
			{
				String aString = a.MidString( 1, a.GetLength() - 2 );
				String bString = b.MidString( 1, b.GetLength() - 2 );

				Int32 aVal, bVal;
				if ( FromString( aString.AsChar(), aVal ) && FromString( bString.AsChar(), bVal ) )
				{
					return aVal < bVal;
				}
			}
			return Red::StringCompareNoCase( a.AsChar(), b.AsChar() ) < 0;
		} );
	}

	// Send information about the specified local
	Red::Network::ChannelPacket headerPacket( RED_NET_CHANNEL_SCRIPT_DEBUGGER );

	RED_VERIFY( headerPacket.WriteString( "LocalsHeader" ), TXT( "Not enough space in packet for locals header: %" ) RED_PRIWs, path.AsChar() );
	RED_VERIFY( headerPacket.Write( stamp ), TXT( "Not enough space in packet for stamp value: %u" ), stamp );

	// A way to verify that the child packets are part of this set
	const void* hashData		= path.AsChar();
	size_t hashDataSize			= path.GetLength() * sizeof( Char );
	Red::System::THash32 hash	= Red::System::CalculateHash32( hashData, hashDataSize );
	RED_VERIFY( headerPacket.Write( hash ) );

	RED_VERIFY( headerPacket.Write( stackFrameIndex ), TXT( "Not enough space in packet for locals header: %" ) RED_PRIWs, path.AsChar() );
	RED_VERIFY( headerPacket.WriteString( path.AsChar() ), TXT( "Not enough space in packet for locals header: %" ) RED_PRIWs, path.AsChar() );
	RED_VERIFY( headerPacket.Write( id ), TXT( "Not enough space in packet for item id: %u" ), id );

	// Emit element name and value
	RED_VERIFY( headerPacket.WriteString( cur->GetName().AsChar() ), TXT( "Not enough space in packet for locals header (name): %" ) RED_PRIWs, path.AsChar() );
	RED_VERIFY( headerPacket.WriteString( cur->GetValue().AsChar() ), TXT( "Not enough space in packet for locals header (value): %" ) RED_PRIWs, path.AsChar() );

	Uint8 curIcon = static_cast< Uint8 >( cur->GetIcon() );
	RED_VERIFY( headerPacket.Write( curIcon ), TXT( "Not enough space in packet for locals header (icon): %" ) RED_PRIWs, path.AsChar() );

	RED_VERIFY( headerPacket.Write( cur->CanBeModifiedByDebugger() ), TXT( "Not enough space in packet for locals header (isModifiable): %" ) RED_PRIWs, path.AsChar() );


	RED_VERIFY( headerPacket.Write( children.Size() ), TXT( "Not enough space in packet for locals header: %" ) RED_PRIWs, path.AsChar() );

	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_SCRIPT_DEBUGGER, headerPacket );

	// Send the names and values of all the child elements
	
	Uint32 packetStartingIndex = 0;
	Char value[ 1024 ];

	while( packetStartingIndex < children.Size() )
	{
		Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_DEBUGGER );

		// Header
		RED_VERIFY( packet.WriteString( "LocalsBody" ) );
		RED_VERIFY( packet.Write( hash ) );

		// Number of elements in this packet (correct value will be written afterwards, we're simply allocating space in the packet)
		Uint32 childCount = 0;
		Uint16 childCountPosition = packet.GetSize();
		RED_VERIFY( packet.Write( 0 ) );

		for ( Uint32 i = packetStartingIndex; i < children.Size(); ++i )
		{
			IScriptDebugVariable* var = cur->GetChild( children[ i ] );
			Bool isExpandable = false;
			Uint8 icon = static_cast< Uint8 >( SDLI_PublicMember );
			const Char* name = children[ i ].AsChar();
			Bool isModifiable = false;

			if( var )
			{
				isExpandable = var->IsExpandable();
				isModifiable = var->CanBeModifiedByDebugger();

				icon = static_cast< Uint8 >( var->GetIcon() );
				Red::System::StringCopy( value, var->GetValue().AsChar(), ARRAY_COUNT( value ) );

				delete var;
				var = nullptr;
			}
			else
			{
				Red::System::StringCopy( value, TXT( "[Unknown]" ), ARRAY_COUNT( value ) );
			}

			// If any parts fail to write, break out of the loop and don't let this child be included in the count
			if( !packet.Write( isExpandable ) ) break;
			if( !packet.Write( icon ) ) break;
			if( !packet.WriteString( name ) ) break;
			if( !packet.WriteString( value ) ) break;
			if( !packet.Write( isModifiable ) ) break;

			packetStartingIndex = i + 1;
			++childCount;
		}

		// Update number of children stored in packet
		RED_VERIFY( packet.Write( childCount, childCountPosition ) );

		Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_SCRIPT_DEBUGGER, packet );
	}

	// Cleanup
	return true;
}

Bool CScriptDebugger::SetLocal( Uint32 stackFrameIndex, const String& path, const String& value )
{
	Red::TSharedPtr< IScriptDebugVariable > var = FindLocal( stackFrameIndex, path );

	if( var )
	{
		if( var->CanBeModifiedByDebugger() )
		{
			return var->ModifyValue( value );
		}
		else
		{
			RED_LOG_ERROR( RED_LOG_CHANNEL( ScriptDebugger ), TXT( "Variable '%ls' cannot be modified by the debugger!" ), path.AsChar() );
		}
	}
	else
	{
		RED_LOG_WARNING( RED_LOG_CHANNEL( ScriptDebugger ), TXT( "Variable '%ls' not found!" ), path.AsChar() );
	}

	return false;
}

#endif // RED_NETWORK_ENABLED
