#pragma once
#include "../../common/engine/entitytemplateparams.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CBuffImmunity
{
	DECLARE_RTTI_SIMPLE_CLASS( CBuffImmunity );
public:	
	CBuffImmunity() :
	m_potion(false), m_positive(false), m_negative(false), m_neutral(false), m_immobilize(false), m_confuse(false), m_damage(false)
	{}

	void Clear()
	{
		m_immunityTo.ClearFast();
		m_potion		= false;
		m_positive		= false;
		m_negative		= false;
		m_neutral		= false;
		m_immobilize	= false;
		m_confuse		= false;
		m_damage		= false;
	}

	TDynArray< Int32 >			m_immunityTo;
	//temp while flags dont work in scripts 
	Bool						m_potion, m_positive, m_negative, m_neutral, m_immobilize, m_confuse, m_damage	;
public:

};

BEGIN_CLASS_RTTI( CBuffImmunity )
	PROPERTY( m_immunityTo )
	PROPERTY( m_potion	 )
	PROPERTY( m_positive )
	PROPERTY( m_negative )
	PROPERTY( m_neutral )
	PROPERTY( m_immobilize )
	PROPERTY( m_confuse	 )
	PROPERTY( m_damage	 )
END_CLASS_RTTI();



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum EImmunityFlags
{
	IF_Potion	 = FLAG( 0 ),
	IF_Positive  = FLAG( 1 ),
	IF_Negative  = FLAG( 2 ),
	IF_Neutral	 = FLAG( 3 ),
	IF_Immobilize= FLAG( 4 ),
	IF_Confuse	 = FLAG( 5 ),
	IF_Damage	 = FLAG( 6 )  
};


BEGIN_BITFIELD_RTTI( EImmunityFlags, 1 );
	BITFIELD_OPTION( IF_Potion );
	BITFIELD_OPTION( IF_Positive );
	BITFIELD_OPTION( IF_Negative );
	BITFIELD_OPTION( IF_Neutral );
	BITFIELD_OPTION( IF_Immobilize );
	BITFIELD_OPTION( IF_Confuse );
	BITFIELD_OPTION( IF_Damage );
END_BITFIELD_RTTI();


class CBuffImmunityParam : public CGameplayEntityParam
{
	DECLARE_ENGINE_CLASS( CBuffImmunityParam, CGameplayEntityParam, 0 );


protected:
	TDynArray< Int32 >			m_immunityTo;
	Int32							m_flags;
public:

	CBuffImmunityParam() : m_flags(0) 
	{}

	const TDynArray< Int32 >	 &  GetImmunities() const  { return m_immunityTo; }
	Int32 						GetFlags()		const  { return m_flags; }
};

BEGIN_CLASS_RTTI( CBuffImmunityParam );
	PARENT_CLASS( CGameplayEntityParam );
	PROPERTY_CUSTOM_EDIT_ARRAY( m_immunityTo, TXT("Immune to:"), TXT("ScriptedEnum_EEffectType") );
	PROPERTY_BITFIELD_EDIT( m_flags ,EImmunityFlags  , TXT(" ") )
END_CLASS_RTTI();
