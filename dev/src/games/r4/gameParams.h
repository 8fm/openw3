/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class W3GameParams : public CObject
{
	DECLARE_RTTI_SIMPLE_CLASS( W3GameParams );

	THashMap< CName, Float >	m_floatParams;
	THashMap< CName, Int32 >	m_intParams;
	THashMap< CName, CName >	m_nameParams;

	template< typename T >
	T GetParam( CName paramName, THashMap< CName, T > & paramsMap )
	{
		typename THashMap< CName, T >::const_iterator it = paramsMap.Find( paramName );
		if ( it != paramsMap.End() )
		{
			return it->m_second;
		}
		T newValue = T();
		const IScriptable* context = this;
		CProperty* prop = GetClass()->FindProperty( paramName );
		if ( prop != nullptr )
		{
			prop->Get( context, &newValue );
		}
		paramsMap[ paramName ] = newValue;
		return newValue;
	}

public:

	Float GetFloatParam( CName paramName );
	Int32 GetIntParam( CName paramName );
	CName GetNameParam( CName paramName );

};

BEGIN_CLASS_RTTI( W3GameParams );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();