/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "bitField.h"

#define BEGIN_BITFIELD_RTTI( bitFieldName, size )	RED_DECLARE_RTTI_NAME( bitFieldName )																					\
													template<> struct TTypeName< bitFieldName > { static const CName& GetTypeName() { return CNAME( bitFieldName ); } };	\
													class bitFieldName##BitFieldBuilder																						\
													{																														\
														CBitField	*m_registeredBitField;																					\
													public:																													\
														~bitFieldName##BitFieldBuilder()																					\
														{																													\
															delete m_registeredBitField;																					\
															m_registeredBitField = NULL;																					\
														}																													\
																																											\
														bitFieldName##BitFieldBuilder()																						\
															:  m_registeredBitField( NULL )																					\
														{																													\
															m_registeredBitField = new CBitField( CNAME( bitFieldName ), size );											

#define BITFIELD_OPTION( x )								m_registeredBitField->AddBit( CName( TXT(#x) ), x );

#define BITFIELD_OPTION_DESC( desc, x )						m_registeredBitField->AddBit( CName( desc ), x );


#define END_BITFIELD_RTTI()									SRTTI::GetInstance().RegisterType( m_registeredBitField );	\
														}																\
													};

#define IMPLEMENT_RTTI_BITFIELD(bitFieldName) RED_DEFINE_RTTI_NAME( bitFieldName ); static bitFieldName##BitFieldBuilder UNIQUE_NAME( bitFieldRegistrator );