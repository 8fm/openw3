/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "propertyConverter.h"

namespace PropertyConverter
{

	////////////////////////////////////////////////////////////////////
	// conversion stuff
	Bool SIntNumericPropertiesConverter::ConvertNumericType( const IScriptable* container, const IRTTIType* destType, const IRTTIType* sourceType, CProperty* prop, Int32& outVal )
	{
		ERTTITypeType sourceTypeType = sourceType->GetType();
		if ( sourceTypeType == RT_Fundamental )
		{
			CName propTypeName = sourceType->GetName();
			if ( propTypeName == CNAME( Bool ) )
			{
				Bool b = 0;
				prop->Get( const_cast< IScriptable* >( container ), &b );
				outVal = b;
				return true;
			}
			else if ( propTypeName == CNAME( Int32 ) )
			{
				prop->Get( const_cast< IScriptable* >( container ), &outVal );
				return true;
			}
			else if ( propTypeName ==  CNAME( Float ) )
			{
				Float f = 0.f;
				prop->Get( const_cast< IScriptable* >( container ), &f );
				outVal = Int32(f);
				return true;
			}
			return false;
		}
		else if ( sourceTypeType == RT_Enum )
		{
			outVal = 0;
			prop->Get( const_cast< IScriptable* >( container ), &outVal );
			return true;
		}
		return false;
	}

	Bool SFloatNumericPropertiesConverter::ConvertNumericType( const IScriptable* container, const IRTTIType* destType, const IRTTIType* sourceType, CProperty* prop, Float& outVal )
	{
		ERTTITypeType sourceTypeType = sourceType->GetType();
		if ( sourceTypeType == RT_Fundamental )
		{
			CName propTypeName = sourceType->GetName();
			if ( propTypeName == CNAME( Bool ) )
			{
				Bool b = 0;
				prop->Get( const_cast< IScriptable* >( container ), &b );
				outVal = Float( b );
				return true;
			}
			else if ( propTypeName == CNAME( Int32 ) )
			{
				Int32 val;
				prop->Get( const_cast< IScriptable* >( container ), &val );
				outVal = Float( val );
				return true;
			}
			else if ( propTypeName ==  CNAME( Float ) )
			{
				prop->Get( const_cast< IScriptable* >( container ), &outVal );
				return true;
			}
			return false;
		}
		else if ( sourceTypeType == RT_Enum )
		{
			Int32 val = 0;
			prop->Get( const_cast< IScriptable* >( container ), &val );
			outVal = Float( val );
			return true;
		}
		return false;
	}



};			// namespace PropertyConverter