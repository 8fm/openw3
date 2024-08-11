/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptSyntaxNode.h"
#include "rttiType.h"
#include "class.h"

enum EInternalType
{
	IT_Void,
	IT_Null,
	IT_Bool,
	IT_Byte,
	IT_Int,
	IT_Float,
	IT_Name,
	IT_String,
	IT_Object,
	IT_Enum,
	IT_Other,
};

static EInternalType TranslateType( const ScriptSyntaxNodeType& type )
{
	if ( type.m_isNull )
	{
		return IT_Null;
	}
	else if ( type.m_type == NULL )
	{
		return IT_Void;
	}
	else if ( type.m_type->GetName() == ::GetTypeName< Int32 >() )
	{
		return IT_Int;
	}
	else if ( type.m_type->GetName() == ::GetTypeName< Float >() )
	{
		return IT_Float;
	}
	else if ( type.m_type->GetName() == ::GetTypeName< Uint8 >() )
	{
		return IT_Byte;
	}
	else if ( type.m_type->GetName() == ::GetTypeName< Bool >() )
	{
		return IT_Bool;
	}
	else if ( type.m_type->GetName() == ::GetTypeName< CName >() )
	{
		return IT_Name;
	}
	else if ( type.m_type->GetName() == ::GetTypeName< String >() )
	{
		return IT_String;
	}
	else if ( type.m_type->GetType() == RT_Handle )
	{
		return IT_Object;
	}
	else if ( type.m_type->GetType() == RT_Enum )
	{
		return IT_Enum;
	}

	return IT_Other;
}

Bool ScriptSyntaxNodeType::CheckCompatibleTypes( const ScriptSyntaxNodeType& srcType, const ScriptSyntaxNodeType& destType )
{
	return srcType.m_type == destType.m_type && srcType.m_type;
}

#define CAST_EXPLICIT	-2
#define CAST_NOCAST		-1
#define CAST_FREE		0
#define CAST_MODERATE	5
#define CAST_EXPENSIVE	10

// -2 means only explicit conversion is possible
// -1 means no conversion
// >0 means implicit conversion is possible and its relative "cost" is given
// VD = void, NL = null, BL = bool, BY = byte, IN = int, FL = float, NA = name, ST = string, OB = object, EN = enum
// arrays are not castable
Int32 gTypeCost[10][10] =
{ 
  //{        Void,        null,          Bool,          Byte,            Int,         Float,        Name,         String,        Object,           Enum  }
	{ CAST_NOCAST, CAST_NOCAST,   CAST_NOCAST,   CAST_NOCAST,    CAST_NOCAST,   CAST_NOCAST, CAST_NOCAST,    CAST_NOCAST,   CAST_NOCAST,    CAST_NOCAST, }, // VD
	{ CAST_NOCAST, CAST_NOCAST,   CAST_NOCAST,   CAST_NOCAST,    CAST_NOCAST,   CAST_NOCAST, CAST_NOCAST,    CAST_NOCAST,     CAST_FREE,    CAST_NOCAST, }, // NL
	{ CAST_NOCAST, CAST_NOCAST,     CAST_FREE, CAST_EXPLICIT,  CAST_EXPLICIT, CAST_EXPLICIT, CAST_NOCAST, CAST_EXPENSIVE,   CAST_NOCAST,    CAST_NOCAST, }, // BL
	{ CAST_NOCAST, CAST_NOCAST, CAST_MODERATE,     CAST_FREE,              1,             2, CAST_NOCAST, CAST_EXPENSIVE,   CAST_NOCAST,    CAST_NOCAST, }, // BY
	{ CAST_NOCAST, CAST_NOCAST, CAST_MODERATE,             1,      CAST_FREE,             1, CAST_NOCAST, CAST_EXPENSIVE,   CAST_NOCAST, CAST_EXPENSIVE, }, // IN
	{ CAST_NOCAST, CAST_NOCAST, CAST_MODERATE, CAST_EXPLICIT,  CAST_EXPLICIT,     CAST_FREE, CAST_NOCAST, CAST_EXPENSIVE,   CAST_NOCAST,    CAST_NOCAST, }, // FL
	{ CAST_NOCAST, CAST_NOCAST, CAST_EXPLICIT,   CAST_NOCAST,    CAST_NOCAST,   CAST_NOCAST,   CAST_FREE, CAST_EXPENSIVE,   CAST_NOCAST,    CAST_NOCAST, }, // NA
	{ CAST_NOCAST, CAST_NOCAST, CAST_MODERATE, CAST_EXPLICIT,  CAST_EXPLICIT, CAST_EXPLICIT, CAST_NOCAST,      CAST_FREE,   CAST_NOCAST,    CAST_NOCAST, }, // ST
	{ CAST_NOCAST, CAST_NOCAST, CAST_MODERATE,   CAST_NOCAST,    CAST_NOCAST,   CAST_NOCAST, CAST_NOCAST, CAST_EXPENSIVE, CAST_EXPLICIT,    CAST_NOCAST, }, // OB
	{ CAST_NOCAST, CAST_NOCAST,   CAST_NOCAST,   CAST_NOCAST, CAST_EXPENSIVE,   CAST_NOCAST, CAST_NOCAST, CAST_EXPENSIVE,   CAST_NOCAST,  CAST_EXPLICIT, }, // EN
};

Int32 ScriptSyntaxNodeType::GetCastCost( const ScriptSyntaxNodeType& srcRawType, const ScriptSyntaxNodeType& destRawType )
{
	// Translate types
	const EInternalType srcType = TranslateType( srcRawType );
	const EInternalType destType = TranslateType( destRawType );

	// One of these types can't be cast to anything
	if( srcType == IT_Void || destType == IT_Void )
	{
		return CAST_NOCAST;
	}

	// Complex types are generally not castable
	if ( srcType == IT_Other || destType == IT_Other )
	{
		// If both types are the same then yeah, casting is possible
		if ( srcRawType.m_type == destRawType.m_type )
		{
			return CAST_FREE;
		}

		// Arrays - in scripts (as in C++) it's possible to cast from one array type to other array type as long as they share the element type (memory class or pool is not important)
		if ( srcRawType.m_type->GetType() == RT_Array && destRawType.m_type->GetType() == RT_Array )
		{
			const CRTTIArrayType* srcArrayType = static_cast< const CRTTIArrayType* >( srcRawType.m_type );
			const CRTTIArrayType* destArrayType = static_cast< const CRTTIArrayType* >( destRawType.m_type );

			if ( srcArrayType->GetInnerType() == destArrayType->GetInnerType() )
			{
				// no casting cost
				return CAST_FREE;
			}
		}

		// No casting
		return CAST_NOCAST;
	}

	// Get the assumed cost of casting
	Int32 cost = gTypeCost[ srcType ][ destType ];

	// There's no cast possible at all, do not bother tighter checking
	if ( cost == CAST_NOCAST )
	{
		return CAST_NOCAST;
	}

	// Object casting, check classes
	if ( srcType == IT_Object && destType == IT_Object )
	{
		ASSERT( srcRawType.m_type && srcRawType.m_type->GetType() == RT_Handle );
		ASSERT( destRawType.m_type && destRawType.m_type->GetType() == RT_Handle );

		CClass* srcClass = static_cast< CRTTIHandleType* >( srcRawType.m_type )->GetPointedType();
		CClass* destClass = static_cast< CRTTIHandleType* >( destRawType.m_type )->GetPointedType();

		// Upcasts are always possible
		if ( srcClass->IsA( destClass ) )
		{
			return CAST_FREE;
		}

		// Downcasts are possible with explicit casting
		if ( destClass->IsA( srcClass ) )
		{
			return CAST_EXPLICIT;
		}

		// Types are not related, no casting is possible !
		return CAST_NOCAST;
	}  
	else if( srcType == IT_Enum && destType == IT_Enum )
	{
		if( srcRawType.m_type->GetName() == destRawType.m_type->GetName() )
		{
			return CAST_FREE;
		}
	}

	// Return assumed cost of casting
	return cost;  
}

#define NONE &CName::NONE
#define XX &CNAME( SyntaxNoCast )

#define BLBY &CNAME( SyntaxBoolToByte )
#define BLIN &CNAME( SyntaxBoolToInt )
#define BLFL &CNAME( SyntaxBoolToFloat )
#define BLST &CNAME( SyntaxBoolToString )

#define BYBL &CNAME( SyntaxByteToBool )
#define BYIN &CNAME( SyntaxByteToInt )
#define BYFL &CNAME( SyntaxByteToFloat )
#define BYST &CNAME( SyntaxByteToString )

#define INBL &CNAME( SyntaxIntToBool )
#define INBY &CNAME( SyntaxIntToByte )
#define INFL &CNAME( SyntaxIntToFloat )
#define INST &CNAME( SyntaxIntToString )
#define INEN &CNAME( SyntaxIntToEnum )

#define FLBL &CNAME( SyntaxFloatToBool )
#define FLBY &CNAME( SyntaxFloatToByte )
#define FLIN &CNAME( SyntaxFloatToInt )
#define FLST &CNAME( SyntaxFloatToString )

#define NABL &CNAME( SyntaxNameToBool )
#define NAST &CNAME( SyntaxNameToString )

#define STBL &CNAME( SyntaxStringToBool )
#define STBY &CNAME( SyntaxStringToByte )
#define STIN &CNAME( SyntaxStringToInt )
#define STFL &CNAME( SyntaxStringToFloat )

#define OBBL &CNAME( SyntaxObjectToBool )
#define OBST &CNAME( SyntaxObjectToString )
#define OBOB &CNAME( SyntaxDynamicCast )

#define ENST &CNAME( SyntaxEnumToString )
#define ENIN &CNAME( SyntaxEnumToInt )

// Casting nodes
const CName* TypeCastNodes[10][10] =
{
  //{  VD,   NL,   BL,   BY,   IN,   FL,   NA,   ST,   OB,  EN }
	{  XX,   XX,   XX,   XX,   XX,   XX,   XX,   XX,   XX,   XX }, // VD
	{  XX,   XX,   XX,   XX,  NONE,  XX,   XX,   XX, NONE,   XX }, // NL
	{  XX,   XX, NONE, BLBY, BLIN, BLFL,   XX, BLST,   XX,   XX }, // BL   
	{  XX,   XX, BYBL, NONE, BYIN, BYFL,   XX, BYST,   XX,   XX }, // BY   
	{  XX,   XX, INBL, INBY, NONE, INFL,   XX, INST,   XX, INEN }, // IN   
	{  XX,   XX, FLBL, FLBY, FLIN, NONE,   XX, FLST,   XX,   XX }, // FL   
	{  XX,   XX, NABL,   XX,   XX,  XX,  NONE, NAST,   XX,   XX }, // NA   
	{  XX,   XX, STBL, STBY, STIN, STFL,   XX, NONE,   XX,   XX }, // ST   
	{  XX,   XX, OBBL,   XX,   XX,   XX,   XX, OBST, OBOB,   XX }, // OB   
	{  XX,   XX,   XX,   XX, ENIN,   XX,   XX, ENST,   XX, NONE }, // EN   
};

#undef NONE

CName ScriptSyntaxNodeType::GetCastNode( const ScriptSyntaxNodeType& srcRawType, const ScriptSyntaxNodeType& destRawType )
{
	// Translate types
	const EInternalType srcType = TranslateType( srcRawType );
	const EInternalType destType = TranslateType( destRawType );

	// Get the casting node
	if ( srcType < IT_Other && destType < IT_Other )
	{
		return *TypeCastNodes[ srcType ][ destType ];
	}

	// No casting possible
	return CName::NONE;
}

Bool ScriptSyntaxNodeType::IsVoid() const
{
	return m_type == NULL && !m_isNull;
}

Bool ScriptSyntaxNodeType::IsArray() const
{
	return m_type && m_type->GetType() == RT_Array;
}

Bool ScriptSyntaxNodeType::IsPointer() const
{
	return m_type && m_type->GetType() == RT_Handle;
}

CClass* ScriptSyntaxNodeType::GetStruct() const
{
	if ( m_type && m_type->GetType() == RT_Class )
	{
		return (CClass*) m_type;
	}

	return NULL;
}

CClass* ScriptSyntaxNodeType::GetPtrClass() const
{
	if ( m_type && m_type->GetType() == RT_Handle )
	{
		CRTTIHandleType* handle = static_cast< CRTTIHandleType* >( m_type );
		return static_cast< CClass* >( handle->GetPointedType() );
	}

	return NULL;
}

String ScriptSyntaxNodeType::ToString() const
{
	if ( m_type == NULL )
	{
		return TXT("void");
	}
	else if ( m_isNull )
	{
		return TXT("NULL");
	}
	else
	{
		String typeName = m_isAssignable ? TXT("&") : TXT("");
		typeName += m_type->GetName().AsString();
		return typeName;
	}
}

void ScriptSyntaxNodeType::InitNULL()
{
	m_isAssignable = false;
	m_isNull = true;
	m_type = NULL;
}

void ScriptSyntaxNodeType::InitSimple( CName typeName, Bool isAssignable/*=false*/ )
{
	m_isNull = false;
	m_type = SRTTI::GetInstance().FindType( typeName );
	m_isAssignable = isAssignable && ( m_type != NULL );
}

void ScriptSyntaxNodeType::InitSimple( IRTTIType* type, Bool isAssignable/*=false*/ )
{
	m_isNull = false;
	m_type = type;
	m_isAssignable = isAssignable && ( m_type != NULL );
}

void ScriptSyntaxNodeType::InitPointer( CClass* typeClass, Bool isAssignable/*=false*/ )
{
	m_isNull = false;

	if ( NULL != typeClass )
	{
		// format pointer type name
		// TODO: this is a singular use of such code, but in future we may consider putting this into nicer function
		Char typeName[ RED_NAME_MAX_LENGTH ];
		Red::System::SNPrintF( typeName, ARRAY_COUNT(typeName), TXT("handle:%ls"), typeClass->GetName().AsChar() ); 

		m_type = SRTTI::GetInstance().FindType( CName( typeName ) );
		m_isAssignable = isAssignable;		
	}
	else
	{
		m_type = NULL;
		m_isAssignable = false;
	}
}