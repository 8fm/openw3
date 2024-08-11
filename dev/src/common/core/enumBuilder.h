/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "rttiType.h"
#include "enum.h"

#define BEGIN_ENUM_RTTI( enumName )					RED_DECLARE_RTTI_NAME( enumName ) \
													template<> struct TTypeName< enumName > { static const CName& GetTypeName() { return CNAME( enumName ); } };	\
													class enumName##EnumBuilder																\
													{																						\
														CEnum		*m_registeredEnum;														\
													public:																					\
														~enumName##EnumBuilder()															\
														{																					\
															delete m_registeredEnum;														\
															m_registeredEnum = NULL;														\
														}																					\
																																			\
														enumName##EnumBuilder()																\
															:  m_registeredEnum( NULL )														\
														{																					\
															m_registeredEnum = new CEnum( CNAME( enumName ), sizeof( enumName ), false );	\

#define ENUM_OPTION( x )									m_registeredEnum->Add( CName( TXT(#x) ), x );

#define ENUM_OPTION_DESC( desc, x )							m_registeredEnum->Add( CName( desc ), x );


#define END_ENUM_RTTI()										SRTTI::GetInstance().RegisterType( m_registeredEnum );							\
														}																					\
													};

#define IMPLEMENT_RTTI_ENUM(enumName)  RED_DEFINE_RTTI_NAME( enumName ); static enumName##EnumBuilder UNIQUE_NAME( enumRegistrator );
