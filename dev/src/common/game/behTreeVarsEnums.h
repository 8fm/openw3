#pragma once

#include "behTreeVars.h"


template < class Enumerator, int DEFAULT_VALUE >
class TBehTreeValEnum : public TBehTreeValue< Enumerator >
{
private:
	typedef TBehTreeValue< Enumerator > TBaseClass;

public:
	TBehTreeValEnum( Enumerator e = DEFAULT_VALUE )
		: TBaseClass( e )
	{}

	Enumerator GetVal( const CBehTreeSpawnContext& context ) const							{ return TBaseClass::TGetVal( context ); }
	void GetValRef( const CBehTreeSpawnContext& context, Enumerator& enumOut ) const		{ enumOut = GetVal( context ); }
};



class CBehTreeValEMoveType : public TBehTreeValEnum< EMoveType, MT_Run >
{
	DECLARE_RTTI_STRUCT( CBehTreeValEMoveType );

private:
	typedef TBehTreeValEnum< EMoveType, MT_Run > TBaseClass;

public:
	CBehTreeValEMoveType( EMoveType e = MT_Run )
		: TBaseClass( e )																	{}
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeValEMoveType );
	PROPERTY_EDIT( m_varName, TXT("Variable") );
	PROPERTY_EDIT( m_value, TXT("Move type") );
END_CLASS_RTTI();


class CBehTreeValEExplorationType : public TBehTreeValEnum< EExplorationType, ET_Jump >
{
	DECLARE_RTTI_STRUCT( CBehTreeValEExplorationType );

private:
	typedef TBehTreeValEnum< EExplorationType, ET_Jump > TBaseClass;

public:
	CBehTreeValEExplorationType( EExplorationType e = ET_Jump )
		: TBaseClass( e )																	{}
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeValEExplorationType );
	PROPERTY_EDIT( m_varName, TXT("Variable") );
	PROPERTY_EDIT( m_value, TXT("Move type") );
END_CLASS_RTTI();
