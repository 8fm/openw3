#pragma once

enum EActorRoleEnum
{
	ARE_Solo,
	ARE_Techie,
	ARE_Fixer,
	ARE_Netrunner,
	ARE_General,

	ARE_Size
};


BEGIN_ENUM_RTTI( EActorRoleEnum );
ENUM_OPTION( ARE_Solo );
ENUM_OPTION( ARE_Techie );
ENUM_OPTION( ARE_Fixer );
ENUM_OPTION( ARE_Netrunner );
ENUM_OPTION( ARE_General );
ENUM_OPTION( ARE_Size );
END_ENUM_RTTI();


class IRequirement : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IRequirement, CObject );
};

BEGIN_ABSTRACT_CLASS_RTTI( IRequirement )
	PARENT_CLASS( CObject )
END_CLASS_RTTI();

class CTraitRequirement : public IRequirement
{
	DECLARE_ENGINE_CLASS( CTraitRequirement, IRequirement, 0 );

	CName m_traitName;

	Bool IsMet( CName traitName ) const { return traitName == m_traitName; }
}; 

BEGIN_CLASS_RTTI( CTraitRequirement )
	PARENT_CLASS( IRequirement )
	PROPERTY_CUSTOM_EDIT_NAME( m_traitName, TXT( "i_traitName" ), TXT( "Trait name" ), TXT( "TraitNameEditor" ) );
END_CLASS_RTTI();


class CImplantRequirement : public IRequirement
{
	DECLARE_ENGINE_CLASS( CImplantRequirement, IRequirement, 0 );

	CName m_implantName;
};

BEGIN_CLASS_RTTI( CImplantRequirement )
	PARENT_CLASS( IRequirement )
	PROPERTY_EDIT_NAME( m_implantName, TXT( "i_implantName" ), TXT( "Implant name" ) );
END_CLASS_RTTI();


class CSkillRequirement : public IRequirement
{
	DECLARE_ENGINE_CLASS( CSkillRequirement, IRequirement, 0 );

	CName m_skillName;
	Int32	m_skillLevel;
public:

	CSkillRequirement() : m_skillName( TXT( "" ) ), m_skillLevel( 0 ) {}
	CSkillRequirement( CName name, int level ) : m_skillName( name ), m_skillLevel( level ) {}

	Bool IsMet( CName skillName, Int32 skillLevel ) const {  return skillName == m_skillName && skillLevel >= m_skillLevel; }
};

BEGIN_CLASS_RTTI( CSkillRequirement )
	PARENT_CLASS( IRequirement )
	PROPERTY_CUSTOM_EDIT_NAME( m_skillName, TXT( "i_skillName" ), TXT( "Skill name" ), TXT( "SkillNameEditor" ) );
	PROPERTY_EDIT_NAME( m_skillLevel, TXT( "i_skillLevel" ), TXT( "Skill level" ) );
END_CLASS_RTTI();

class ITraitAbility : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ITraitAbility, CObject );
public:
	virtual void Activate() {}
};

BEGIN_ABSTRACT_CLASS_RTTI( ITraitAbility )
	PARENT_CLASS( CObject )
END_CLASS_RTTI();

class CPassiveTraitAbility : public ITraitAbility
{
	DECLARE_ENGINE_CLASS( CPassiveTraitAbility, ITraitAbility, 0 );
public:
	virtual void Activate() {}
};

BEGIN_CLASS_RTTI( CPassiveTraitAbility )
	PARENT_CLASS( ITraitAbility )
END_CLASS_RTTI();

class CActiveTraitAbility : public ITraitAbility
{
	DECLARE_ENGINE_CLASS( CActiveTraitAbility, ITraitAbility, 0 );
public:
	virtual void Activate() {}
};

BEGIN_CLASS_RTTI( CActiveTraitAbility )
	PARENT_CLASS( ITraitAbility )
END_CLASS_RTTI();

class CTraitAbilityWrapper : public CObject
{
	DECLARE_ENGINE_CLASS( CTraitAbilityWrapper, CObject, 0 );
public:
	ITraitAbility*		m_ability;
};

BEGIN_CLASS_RTTI( CTraitAbilityWrapper )
	PARENT_CLASS( CObject );
	PROPERTY_INLINED( m_ability,  TXT( "Ability" ) );
END_CLASS_RTTI();

class CTraitRequirementWrapper : public CObject
{
	DECLARE_ENGINE_CLASS( CTraitRequirementWrapper, CObject, 0 );
public:
	TDynArray< IRequirement* >	m_requirements;
};

BEGIN_CLASS_RTTI( CTraitRequirementWrapper )
	PARENT_CLASS( CObject );
	PROPERTY_INLINED( m_requirements,  TXT( "Requirements" ) );
END_CLASS_RTTI();

struct STraitTableEntry
{
	DECLARE_RTTI_STRUCT( STraitTableEntry );

public:
	CName								m_traitName;		
	LocalizedString						m_GUIName;
	String								m_traitIcon;			
	LocalizedString						m_traitDesc;
	Float								m_traitCost;
	CTraitRequirementWrapper*			m_requirements;
	CTraitAbilityWrapper*				m_ability;

public:
	STraitTableEntry();

	void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings, CResource& parent ); /*const */
};

BEGIN_CLASS_RTTI( STraitTableEntry )
	PROPERTY_EDIT_NAME  ( m_traitName,		TXT( "i_traitName" ),		TXT( "Name" ) );
	PROPERTY_EDIT       ( m_GUIName,				TXT( "GUI name" ) );
	PROPERTY_EDIT       ( m_traitIcon,				TXT( "Icon" ) );
	PROPERTY_EDIT       ( m_traitDesc,				TXT( "Decsription" ) );
	PROPERTY_EDIT       ( m_traitCost,				TXT( "Cost" ) );
	PROPERTY_INLINED    ( m_requirements,			TXT( "Requirements to unlock this trait" ) );
	PROPERTY_INLINED    ( m_ability,				TXT( "Ability activated by this trait" ) );
END_CLASS_RTTI();

enum EBaseStat
{
	BS_Intelligence,
	BS_Reflex,
	BS_Dexterity,
	BS_Cool,
	BS_TechAbility,
	BS_Empathy,
	BS_Constitution,
	BS_Strenght,
};

BEGIN_ENUM_RTTI( EBaseStat );
ENUM_OPTION( BS_Intelligence );
ENUM_OPTION( BS_Reflex );
ENUM_OPTION( BS_Dexterity );
ENUM_OPTION( BS_Cool );
ENUM_OPTION( BS_TechAbility );
ENUM_OPTION( BS_Empathy );
ENUM_OPTION( BS_Constitution );
ENUM_OPTION( BS_Strenght );
END_ENUM_RTTI();

struct SSkillTableEntry
{
	DECLARE_RTTI_STRUCT( SSkillTableEntry );

public:
	static const Int32 SKILL_HIGHEST_LEVEL = 10;

	CName								m_skillName;			
	LocalizedString						m_GUIName;
	String								m_skillIcon;			
	LocalizedString						m_skillDesc;
	EBaseStat							m_baseStat;
	Int32								m_roleSkillCostMultiplayer[ARE_Size];
	TDynArray< CName >					m_levTraits[SKILL_HIGHEST_LEVEL];

public:
	
	SSkillTableEntry();
	TDynArray<CName>* GetLevel( Int32 index );

	void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings, CResource& parent ); /*const */
};

BEGIN_CLASS_RTTI( SSkillTableEntry )
	PROPERTY_EDIT_NAME ( m_skillName,		TXT( "i_skillName" ),		TXT( "Name" ) );
	PROPERTY_EDIT ( m_GUIName,			TXT( "GUI name" ) );
	PROPERTY_EDIT ( m_skillIcon,		TXT( "Icon" ) );
	PROPERTY_EDIT ( m_skillDesc,		TXT( "Decsription" ) );
	PROPERTY_EDIT ( m_baseStat,			TXT( "Base statistic related to this skill" ) );
	PROPERTY_EDIT ( m_roleSkillCostMultiplayer[ARE_Solo],		TXT( "Solo's skill cost" ) );
	PROPERTY_EDIT ( m_roleSkillCostMultiplayer[ARE_Techie],		TXT( "Techie's skill cost" ) );
	PROPERTY_EDIT ( m_roleSkillCostMultiplayer[ARE_Fixer],		TXT( "Fixer's skill cost" ) );
	PROPERTY_EDIT ( m_roleSkillCostMultiplayer[ARE_Netrunner],	TXT( "Netrunner's skill cost" ) );
	PROPERTY_EDIT ( m_roleSkillCostMultiplayer[ARE_General],	TXT( "Generals's skill cost" ) );
	PROPERTY_EDIT ( m_levTraits[ 0 ],	TXT( "Level1" ) );
	PROPERTY_EDIT ( m_levTraits[ 1 ],	TXT( "Level2" ) );
	PROPERTY_EDIT ( m_levTraits[ 2 ],	TXT( "Level3" ) );
	PROPERTY_EDIT ( m_levTraits[ 3 ],	TXT( "Level4" ) );
	PROPERTY_EDIT ( m_levTraits[ 4 ],	TXT( "Level5" ) );
	PROPERTY_EDIT ( m_levTraits[ 5 ],	TXT( "Level6" ) );
	PROPERTY_EDIT ( m_levTraits[ 6 ],	TXT( "Level7" ) );
	PROPERTY_EDIT ( m_levTraits[ 7 ],	TXT( "Level8" ) );
	PROPERTY_EDIT ( m_levTraits[ 8 ],	TXT( "Level9" ) );
	PROPERTY_EDIT ( m_levTraits[ 9 ],	TXT( "Level10" ) );
END_CLASS_RTTI();

class CTraitData : public CResource, public ILocalizableObject
{
	DECLARE_ENGINE_RESOURCE_CLASS( CTraitData, CResource, "trait", "Trait" );

private:
	TDynArray< STraitTableEntry > m_traitTable;
	TDynArray< SSkillTableEntry>  m_skillTable;

public:
	//! Get traits table entries
	RED_INLINE TDynArray< STraitTableEntry >& GetTraitTable() { return m_traitTable; }

	//! Get traits table entries
	RED_INLINE const TDynArray< STraitTableEntry >& GetTraitTable() const { return m_traitTable; }

	//! Get skills table
	RED_INLINE TDynArray< SSkillTableEntry >& GetSkillTable() { return m_skillTable; }

	//! Get skills table
	RED_INLINE const TDynArray< SSkillTableEntry >& GetSkillTable() const { return m_skillTable; }

	STraitTableEntry* GetTrait( CName name );
	SSkillTableEntry* GetSkill( CName name );

	const STraitTableEntry* GetTrait( CName name ) const;
	const SSkillTableEntry* GetSkill( CName name ) const;

	virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ); /*const */

public:
	CTraitData();

	virtual void OnSerialize( IFile &file );
};

BEGIN_CLASS_RTTI( CTraitData )
	PARENT_CLASS( CResource )
	PROPERTY_EDIT_NAME( m_traitTable, TXT( "i_traitTable" ), TXT( "Trait table" ) );
	PROPERTY_EDIT_NAME( m_skillTable, TXT( "i_skillTable" ), TXT( "Skill table" ) );
END_CLASS_RTTI()

