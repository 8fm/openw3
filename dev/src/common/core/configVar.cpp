/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "configVar.h"
#include "configVarSystem.h"
#include "configVarRegistry.h"

//---

/* This is an example usage for refrerence
namespace Config
{
	TConfigVar<Int32 > cvExampleProp1( "Example", "TestInt", 30 );
	TConfigVar<Int32, Validation::IntRange<-5,5> > cvExampleProp2( "Example", "TestIntClamped", 30 );
	TConfigVar<Float > cvExampleProp3( "Example", "TestFloat", 15.0f );
	TConfigVar<Float, Validation::FloatRange<-5000,5000,1000> > cvExampleProp4( "Example", "TestTestFloatClamped", 1.1f );
	TConfigVar<Bool> cvExampleProp5( "Example", "TestBool", false );
	TConfigVar<Bool> cvExampleProp6( "Example", "TestBool2", true );
	TConfigVar<Int32> cvExampleProp7( "Example", "TestReadOnly", 666, eConsoleVarFlag_ReadOnly );
	TConfigVar<String> cvExampleProp8( "Example", "TestString", TXT("FooBar") );
	TConfigVar<Int32> cvExampleProp9( "Example/SubGroup", "TestChild1", 1 );
	TConfigVar<Int32> cvExampleProp10( "Example/SubGroup", "TestChild2", 2 );
	TConfigVar<Int32> cvExampleProp11( "Example/SubGroup", "TestChild3", 3 );
	TConfigVar<Int32> cvExampleProp12( "Example/SubGroup2", "TestChild1", 4 );
	TConfigVar<Int32> cvExampleProp13( "Example/SubGroup2", "TestChild2", 5 );
	TConfigVar<Int32> cvExampleProp14( "Example/SubGroup2", "TestChild3", 6 );
	TConfigVar<Int32> cvExampleProp15( "Example/SubGroup2/Nested", "TestNestedChild1", -100 );
	TConfigVar<Int32> cvExampleProp16( "Example/SubGroup2/Nested", "TestNestedChild2", 0 );
	TConfigVar<Int32> cvExampleProp17( "Example/SubGroup2/Nested", "TestNestedChild3", 100 );
}
*/

//---


namespace Config
{

#ifndef RED_FINAL_BUILD
	namespace Helpers
	{

		static Bool IsAlphaNum( const AnsiChar ch )
		{
			if ( ( ch >= '0' && ch <= '9' ) || ( ch >= 'A' && ch <= 'Z' ) || ( ch >= 'a' && ch <= 'z' ) )
				return true;

			return false;
		}

		static void ValidateConfigVarName( const AnsiChar* name )
		{
			RED_FATAL_ASSERT( name != nullptr, "Variable name should be specified" );

			while ( *name )
			{
				RED_FATAL_ASSERT( *name > ' ', "Variable name should not contain white spaces" );
				RED_FATAL_ASSERT( IsAlphaNum(*name) || *name == '_', "Variable name should only contain alphanumeric characters or _" );
				++name;
			}
		}

		static void ValidateConfigVarGroup( const AnsiChar* name )
		{
			RED_FATAL_ASSERT( name != nullptr, "Variable group should be specified" );

			while ( *name )
			{
				RED_FATAL_ASSERT( *name > ' ', "Variable group should not contain white spaces" );
				RED_FATAL_ASSERT( IsAlphaNum(*name) || *name == '_' || *name == '/', "Variable group should only contain alphanumeric characters or '_' or '/'" );
				++name;
			}
		}

	}
#endif

	IConfigVar::IConfigVar( const AnsiChar* group, const AnsiChar* name, EConfigVarType type, const Uint32 flags /*= 0*/ )
		: m_name( name )
		, m_group( group )
		, m_type( type )
		, m_flags( flags )
	{
#ifndef RED_FINAL_BUILD
		Helpers::ValidateConfigVarName( name );
		Helpers::ValidateConfigVarGroup( group );
#endif
	}

	IConfigVar::~IConfigVar()
	{
		SConfig::GetInstance().GetRegistry().Unregister( *this );
	}

	void IConfigVar::Register()
	{
		// modify the value to whatever we have in the config files
		String value;
		if ( SConfig::GetInstance().GetValue( GetGroup(), GetName(), value ) )
		{
			if ( !SetText( value, eConfigVarSetMode_Initial ) )
			{
				WARN_CORE( TXT("Unable to set initial value of config var '%ls' in '%ls'"), 
					ANSI_TO_UNICODE( GetName() ), ANSI_TO_UNICODE( GetGroup() ) );
			}
		}

		// add to the system
		SConfig::GetInstance().GetRegistry().Register( *this );
	}


} // Console


