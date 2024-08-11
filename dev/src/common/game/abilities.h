/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Crafting values per ability
struct SAbilityCrafting
{
    CName       m_type;             //!< Type
    Uint32        m_level;            //!< Level
};

struct SAbilityAttributeValue
{
	DECLARE_RTTI_STRUCT( SAbilityAttributeValue );

	Float	m_valueAdditive;
	Float	m_valueMultiplicative;
	Float	m_valueBase;

	static SAbilityAttributeValue ZERO;

	SAbilityAttributeValue( Float additive = 0.f, Float multiplicative = 0.f, Float base = 0.f )
		: m_valueAdditive( additive )
		, m_valueMultiplicative( multiplicative )
		, m_valueBase( base )
	{}

	SAbilityAttributeValue operator+( const SAbilityAttributeValue & sec ) const
	{
		return SAbilityAttributeValue( m_valueAdditive + sec.m_valueAdditive, m_valueMultiplicative + sec.m_valueMultiplicative, m_valueBase + sec.m_valueBase	);
	}

	void operator+=( const SAbilityAttributeValue & sec )
	{
		m_valueAdditive += sec.m_valueAdditive;
		m_valueMultiplicative += sec.m_valueMultiplicative;
		m_valueBase += sec.m_valueBase;
	}

	SAbilityAttributeValue operator-( const SAbilityAttributeValue & sec ) const
	{
		return SAbilityAttributeValue( m_valueAdditive - sec.m_valueAdditive, m_valueMultiplicative - sec.m_valueMultiplicative, m_valueBase - sec.m_valueBase	);
	}

	void operator-=( const SAbilityAttributeValue & sec )
	{
		m_valueAdditive -= sec.m_valueAdditive;
		m_valueMultiplicative -= sec.m_valueMultiplicative;
		m_valueBase -= sec.m_valueBase;
	}

	SAbilityAttributeValue operator*( const Float scalar ) const
	{
		return SAbilityAttributeValue( m_valueAdditive * scalar , m_valueMultiplicative * scalar, m_valueBase * scalar );
	}

	RED_INLINE Bool IsEmpty() const { return m_valueAdditive == 0.0f && m_valueMultiplicative == 0.0f && m_valueBase == 0.0f; }
};

BEGIN_CLASS_RTTI( SAbilityAttributeValue );
	PROPERTY_EDIT( m_valueAdditive, String::EMPTY );
	PROPERTY_EDIT( m_valueMultiplicative, String::EMPTY );
	PROPERTY_EDIT( m_valueBase, String::EMPTY );
END_CLASS_RTTI();

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Attributes modified by ability

enum EAbilityAttributeType
{
	EAttrT_Base,
	EAttrT_Additive,
	EAttrT_Multi
};

BEGIN_ENUM_RTTI( EAbilityAttributeType )
	ENUM_OPTION( EAttrT_Base );
	ENUM_OPTION( EAttrT_Additive );
	ENUM_OPTION( EAttrT_Multi );
END_ENUM_RTTI()


struct SAbilityAttribute
{
	DECLARE_RTTI_STRUCT( SAbilityAttribute );



	CName	m_name;				//!< Attribute name	
	EAbilityAttributeType	m_type;
	Bool	m_alwaysRandom;		//!< Attribute value will not be initialized on start
	Float	m_min;				//!< Minimal value
	Float	m_max;				//!< Maximal value
	Int8	m_precision;		//!< Decimal precision
	Bool	m_displayPerc;		//!< Display as percentage switch
	

	SAbilityAttribute()
		: m_type( EAttrT_Base )
		, m_alwaysRandom( false )
		, m_min( 0.0f )
		, m_max( 0.0f )
		, m_precision( -1 )
		, m_displayPerc( false )
	{}
};

BEGIN_CLASS_RTTI( SAbilityAttribute );
	PROPERTY( m_name );
	PROPERTY( m_type );
	PROPERTY( m_alwaysRandom );
	PROPERTY( m_min );
	PROPERTY( m_max );
	PROPERTY( m_precision );
	PROPERTY( m_displayPerc );
END_CLASS_RTTI();

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Single ability
struct SAbility
{
	DECLARE_RTTI_STRUCT( SAbility );

	CName							m_name;				//!< Ability name
	TDynArray< SAbilityAttribute > 	m_attributes;		//!< Attribute and its modifiers
	TDynArray< CName >				m_prerequisites;	//!< Abilities that has to be available for buying this ability
	TagList							m_tags;				//!< Tags
    SAbilityCrafting                m_crafting;         //!< Crafting Data
	TDynArray< CName >				m_abilities;		//!< Contained ability names
	TDynArray< SAbility* >			m_cachedAbilities;	//!< Contained abilities
	Int32							m_type;				//!< Scripted enum type of ability
	
	Red::System::GUID				m_creatorTag; //! tag is used for determination of definition creator (e.g. DLC mounter)
	
	const SAbilityAttribute *GetAttribute( CName attributeName ) const;
	const TDynArray< SAbilityAttribute >* GetAllAtributes() const;
    const CName* GetType() const;
    const Uint32* GetLevel() const;

	RED_INLINE Bool HasAttribute( CName attributeName )	const { return GetAttribute( attributeName ) != NULL; }
	RED_INLINE Bool HasTag( const CName &tag )			const { return m_tags.HasTag( tag ); }
	RED_INLINE Bool HasAnyOfTags( const TagList& tags )	const { return TagList::MatchAny( tags, m_tags ); }

	SAbility(){}
};

BEGIN_CLASS_RTTI( SAbility );
	PROPERTY( m_attributes );
	PROPERTY( m_prerequisites );
	PROPERTY( m_tags );
	PROPERTY( m_abilities );
	PROPERTY( m_type );
END_CLASS_RTTI();
