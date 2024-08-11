/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

namespace PropertyConverter
{

	struct SIntNumericPropertiesConverter
	{
		static Bool ConvertNumericType( const IScriptable* container, const IRTTIType* destType, const IRTTIType* sourceType, CProperty* prop, Int32& outVal ); 
	};

	struct SFloatNumericPropertiesConverter
	{
		static Bool ConvertNumericType( const IScriptable* container, const IRTTIType* destType, const IRTTIType* sourceType, CProperty* prop, Float& outVal );
	};

	template < class T >
	struct TGenericIntPropertiesConverter : public SIntNumericPropertiesConverter
	{
		static RED_INLINE Bool ConvertNumericType( const IScriptable* container, const IRTTIType* destType, const IRTTIType* sourceType, CProperty* prop, T& outVal )
		{
			Int32 v = 0;
			if ( SIntNumericPropertiesConverter::ConvertNumericType( container, destType, sourceType, prop, v ) )
			{
				outVal = T( v );
				return true;
			}
			return false;
		}
	};

	template < class T >
	struct TNumericPropertiesConverter
	{
		static RED_INLINE Bool ConvertNumericType( const IScriptable* container, const IRTTIType* destType, const IRTTIType* sourceType, CProperty* prop, T& outVal )	{ return false; }
	};



	template < class T >
	struct TPointerPropertiesConverter
	{
		static RED_INLINE Bool ConvertPointerType( const IScriptable* container, const IRTTIType* destType, const IRTTIType* sourceType, CProperty* prop, T& outVal )	{ return false; }
		static RED_INLINE Bool ConvertValue( const void* sourcePtr, const IRTTIType* sourceType, T& outVal )														{ return false; }
		static RED_INLINE Bool SupportType()																														{ return false; }
	};

	template < class T >
	struct TArrayPropertiesConverter
	{
		static RED_INLINE Bool ConvertArrayType( const IScriptable* container, const IRTTIType* destType, const IRTTIType* sourceType, CProperty* prop, T& outVal )		{ return false; }
	};

	template < class T >
	struct TTypeSpecifiedCode
	{
		static RED_INLINE Bool PostConversionAcceptance( const T& val )																								{ return true; }
	};

	template < class T >
	Bool ConvertProperty( const IScriptable* obj, CProperty* prop, T& outVal );

};

#include "propertyConverter.inl"