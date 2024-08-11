#include "build.h"
#include "traitData.h"
#include "../../common/core/factory.h"

IMPLEMENT_ENGINE_CLASS( CTraitData );
IMPLEMENT_ENGINE_CLASS( STraitTableEntry );
IMPLEMENT_ENGINE_CLASS( SSkillTableEntry );
IMPLEMENT_ENGINE_CLASS( IRequirement );
IMPLEMENT_ENGINE_CLASS( CTraitRequirement );
IMPLEMENT_ENGINE_CLASS( CImplantRequirement );
IMPLEMENT_ENGINE_CLASS( CSkillRequirement );
IMPLEMENT_ENGINE_CLASS( ITraitAbility );
IMPLEMENT_ENGINE_CLASS( CPassiveTraitAbility );
IMPLEMENT_ENGINE_CLASS( CActiveTraitAbility );
IMPLEMENT_ENGINE_CLASS( CTraitAbilityWrapper );
IMPLEMENT_ENGINE_CLASS( CTraitRequirementWrapper );
IMPLEMENT_RTTI_ENUM( EBaseStat );
IMPLEMENT_RTTI_ENUM( EActorRoleEnum );

STraitTableEntry::STraitTableEntry()
{
}

SSkillTableEntry::SSkillTableEntry()
{
}

void CTraitData::OnSerialize( IFile &file )
{
	TBaseClass::OnSerialize( file );
}

CTraitData::CTraitData()
{

}

STraitTableEntry* CTraitData::GetTrait( CName name )
{
	for ( Uint32 i = 0; i < m_traitTable.Size(); ++i )
	{
		STraitTableEntry& trait = m_traitTable[i];
		if ( trait.m_traitName == name )
		{
			return &trait;
		}
	}
	return NULL;
}

const STraitTableEntry* CTraitData::GetTrait( CName name ) const
{
	for ( Uint32 i = 0; i < m_traitTable.Size(); ++i )
	{
		const STraitTableEntry& trait = m_traitTable[i];
		if ( trait.m_traitName == name )
		{
			return &trait;
		}
	}
	return NULL;
}

SSkillTableEntry* CTraitData::GetSkill( CName name )
{
	for ( Uint32 i = 0; i < m_skillTable.Size(); ++i )
	{
		SSkillTableEntry& skill = m_skillTable[i];
		if ( skill.m_skillName == name )
		{
			return &skill;
		}
	}
	return NULL;
}

const SSkillTableEntry* CTraitData::GetSkill( CName name ) const
{
	for ( Uint32 i = 0; i < m_skillTable.Size(); ++i )
	{
		const SSkillTableEntry& skill = m_skillTable[i];
		if ( skill.m_skillName == name )
		{
			return &skill;
		}
	}
	return NULL;
}

TDynArray< CName >* SSkillTableEntry::GetLevel( Int32 index )
{
	RED_ASSERT( index > 0 && index <= 10, TXT( "Incorrect level index" ) );
	return &m_levTraits[ index - 1 ];
}

class CTraitDataFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CTraitDataFactory, IFactory, 0 );

public:
	CTraitDataFactory()
	{
		m_resourceClass = ClassID< CTraitData >();
	}

	virtual CResource* DoCreate( const FactoryOptions& options );
};

BEGIN_CLASS_RTTI( CTraitDataFactory )
	PARENT_CLASS( IFactory )	
	END_CLASS_RTTI()

	IMPLEMENT_ENGINE_CLASS( CTraitDataFactory );

CResource* CTraitDataFactory::DoCreate( const FactoryOptions& options )
{
	CTraitData *TraitData = ::CreateObject< CTraitData >( options.m_parentObject );
	return TraitData;
}

void STraitTableEntry::GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings, CResource& parent ) /*const */
{
	LocalizedStringEntry entry;
	entry.m_localizedString = &m_GUIName;
	entry.m_parentResource = &parent;
	localizedStrings.PushBack( entry );

	entry.m_localizedString = &m_traitDesc;
	localizedStrings.PushBack( entry );

}

void SSkillTableEntry::GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings, CResource& parent ) /*const */
{
	LocalizedStringEntry entry;
	entry.m_localizedString = &m_GUIName;
	entry.m_parentResource = &parent;
	localizedStrings.PushBack( entry );

	entry.m_localizedString = &m_skillDesc;
	localizedStrings.PushBack( entry );
}

void CTraitData::GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) /*const */
{
	for ( auto it = m_traitTable.Begin(); it != m_traitTable.End(); ++it )
	{
		it->GetLocalizedStrings( localizedStrings, *this );
	}
	for ( auto it = m_skillTable.Begin(); it != m_skillTable.End(); ++it )
	{
		it->GetLocalizedStrings( localizedStrings, *this );
	}
}
