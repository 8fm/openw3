/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "abilities.h"
#include "../core/scriptStackFrame.h"

SAbilityAttributeValue SAbilityAttributeValue::ZERO;


IMPLEMENT_ENGINE_CLASS( SAbilityAttributeValue );
IMPLEMENT_ENGINE_CLASS( SAbilityAttribute );
IMPLEMENT_ENGINE_CLASS( SAbility );
IMPLEMENT_RTTI_ENUM( EAbilityAttributeType );


void funcAddAbilityAttributeVal( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SAbilityAttributeValue, a, SAbilityAttributeValue::ZERO );
	GET_PARAMETER( SAbilityAttributeValue, b, SAbilityAttributeValue::ZERO );
	FINISH_PARAMETERS;
	RETURN_STRUCT( SAbilityAttributeValue, a + b );	
}

void funcSubAbilityAttributeVal( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SAbilityAttributeValue, a, SAbilityAttributeValue::ZERO );
	GET_PARAMETER( SAbilityAttributeValue, b, SAbilityAttributeValue::ZERO );
	FINISH_PARAMETERS;
	RETURN_STRUCT( SAbilityAttributeValue, a - b );	
}
void funcAddAssignAbilityAttributeVal( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SAbilityAttributeValue, a, SAbilityAttributeValue::ZERO );
	GET_PARAMETER( SAbilityAttributeValue, b, SAbilityAttributeValue::ZERO );
	FINISH_PARAMETERS;
	a += b;
	RETURN_STRUCT( SAbilityAttributeValue, a );	
}

void funcSubAssignAbilityAttributeVal( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SAbilityAttributeValue, a, SAbilityAttributeValue::ZERO );
	GET_PARAMETER( SAbilityAttributeValue, b, SAbilityAttributeValue::ZERO );
	FINISH_PARAMETERS;
	a -= b ;
	RETURN_STRUCT( SAbilityAttributeValue, a );	
}

void funcMultAbilityAttributeValFloat( CObject*, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SAbilityAttributeValue, a, SAbilityAttributeValue::ZERO );
	GET_PARAMETER( Float, b, 1.f );
	FINISH_PARAMETERS;
	RETURN_STRUCT( SAbilityAttributeValue, a*b );	
}


void RegisterAbilitiesFunctions()
{
	NATIVE_BINARY_OPERATOR( Add, funcAddAbilityAttributeVal, SAbilityAttributeValue, SAbilityAttributeValue, SAbilityAttributeValue );
	NATIVE_BINARY_OPERATOR( Subtract, funcSubAbilityAttributeVal, SAbilityAttributeValue, SAbilityAttributeValue, SAbilityAttributeValue );
	NATIVE_BINARY_OPERATOR( AssignAdd, funcAddAssignAbilityAttributeVal, SAbilityAttributeValue, SAbilityAttributeValue, SAbilityAttributeValue );
	NATIVE_BINARY_OPERATOR( AssignSubtract, funcSubAssignAbilityAttributeVal, SAbilityAttributeValue, SAbilityAttributeValue, SAbilityAttributeValue );

	NATIVE_BINARY_OPERATOR( Multiply, funcMultAbilityAttributeValFloat, SAbilityAttributeValue, SAbilityAttributeValue, Float );
}

const CName* SAbility::GetType() const
{
    return &m_crafting.m_type;
}

const Uint32* SAbility::GetLevel() const
{
    return &m_crafting.m_level;
}

const SAbilityAttribute *SAbility::GetAttribute( CName attributeName ) const
{
	for ( Uint32 i = 0; i < m_attributes.Size(); ++i )
	{
		if ( m_attributes[ i ].m_name == attributeName )
		{
			return &m_attributes[ i ];
		}
	}
	return NULL;	
}

const TDynArray< SAbilityAttribute >* SAbility::GetAllAtributes()  const
{
	return &m_attributes;
}
