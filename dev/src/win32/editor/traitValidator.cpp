#include "build.h"
#if 0
#include "traitValidator.h"
#include "../../games/r6/traitData.h"


CTraitValidator::CTraitValidator( CTraitData* traitData )
	 : m_traitData( traitData )
	 , m_index( 0 )
{
	RED_ASSERT( m_traitData );
}

void CTraitValidator::CheckUniqueTraitNames()
{
	TDynArray< STraitTableEntry >& traitTable = m_traitData->GetTraitTable();

	TSet< CName > traitNames;
	for ( TDynArray< STraitTableEntry >::iterator traitIt = traitTable.Begin(); traitIt != traitTable.End(); ++traitIt )
	{
		if ( !traitNames.Insert( traitIt->m_traitName ) )
		{
			String orgName = traitIt->m_traitName.AsString();
			String newName;
			GenerateNewName( orgName, traitNames, CName( TXT( "trait" ) ), newName );
			traitIt->m_traitName = CName( newName );
		}
	}
}
void CTraitValidator::DeleteSkillRequirement( CName traitName, CName skillName, Int32 levelId )
{
	STraitTableEntry* trait = m_traitData->GetTrait( traitName );

	if ( !trait )
	{
		return;
	}
	
	if ( !trait->m_requirements )
	{
		return;
	}
	TDynArray< IRequirement* >& requirements = trait->m_requirements->m_requirements;
	TDynArray< IRequirement* >::iterator traitRequirementsIt = requirements.Begin();
	TDynArray< IRequirement* >::iterator traitRequirementsItEnd = requirements.End();

	// try to find skill requirement with given skill name and level
	for ( traitRequirementsIt; traitRequirementsIt != traitRequirementsItEnd; ++traitRequirementsIt )
	{
		IRequirement* traitRequirement = *traitRequirementsIt;
		RED_ASSERT( traitRequirement );
		if ( traitRequirement->IsA( CSkillRequirement::GetStaticClass() ) )
		{
			CSkillRequirement* skillRequirement = Cast<CSkillRequirement>( traitRequirement );
			if ( skillRequirement->m_skillName == skillName && skillRequirement->m_skillLevel == levelId )
			{
				requirements.Erase( traitRequirementsIt );
				return;
			}
		}
	}
}

void CTraitValidator::CheckUniqueUnlockedTraitNames()
{
	TDynArray< SSkillTableEntry >& skillTable = m_traitData->GetSkillTable();

	TSet< CName > traitNames;
	for ( TDynArray< SSkillTableEntry >::iterator skillIt = skillTable.Begin(); skillIt != skillTable.End(); ++skillIt )
	{
		traitNames.Clear();
		for( Int32 i = SSkillTableEntry::SKILL_HIGHEST_LEVEL; i > 0; --i )
		{
			RED_ASSERT( skillIt->GetLevel( i ) );
			TDynArray< CName >& traits = *skillIt->GetLevel( i );
			for ( Uint32 j = 0; j < traits.Size(); --j )
			{
				CName name = traits[j];
				if ( !traitNames.Insert( name ) )
				{
					// there has already been this trait unlocked by the level
					DeleteSkillRequirement( name, skillIt->m_skillName, i );
					AddErrorMsg( String::Printf( TXT( "Trait %s is already unlocked by skill %s on higher level. Removed trait data from level %d" ), name.AsChar(), skillIt->m_skillName.AsString().AsChar(), i ) );
					traits.Erase( traits.Begin() + j );
					--j;
				}
			}
		}
	}
}
	
void CTraitValidator::GenerateNewName( String& oldName, TSet< CName > names, CName dataInfo, String& newName )
{
	newName = String::Printf( TXT( "%s%d" ), oldName.AsChar(), m_index++ );

	while ( !names.Insert( CName( newName ) ) )
	{	
		newName = String::Printf( TXT( "%s%d" ), oldName.AsChar(), m_index++ );
	}

	AddErrorMsg( String::Printf( TXT( "Double %s with name %s. Name changed to %s" ), dataInfo.AsChar(), oldName.AsChar(), newName.AsChar() ) );
}

void CTraitValidator::CheckUniqueSkillNames()
{
	TDynArray< SSkillTableEntry >& skillTable = m_traitData->GetSkillTable();

	TSet< CName > skillNames;
	for ( TDynArray< SSkillTableEntry >::iterator skillIt = skillTable.Begin(); skillIt != skillTable.End(); ++skillIt )
	{
		if ( !skillNames.Insert( skillIt->m_skillName ) )
		{
			String orgName = skillIt->m_skillName.AsString();
			String newName;
			GenerateNewName( orgName, skillNames, CName( TXT( "skill" ) ), newName );
			skillIt->m_skillName = CName( newName );
		}
	}
}

void CTraitValidator::CheckSkillTrait()
{
	TDynArray< SSkillTableEntry >& skillTable = m_traitData->GetSkillTable();

	for ( TDynArray< SSkillTableEntry >::iterator skillIt = skillTable.Begin(); skillIt != skillTable.End(); ++skillIt )
	{
		for ( UINT32 i = 1; i <= SSkillTableEntry::SKILL_HIGHEST_LEVEL; ++i )
		{
			TDynArray< CName >* level = skillIt->GetLevel(i);

			if ( level->Size() < 1 )
			{
				AddErrorMsg( String::Printf( TXT( "Skill %s doesn't have any trait on level %d related to this skill" ), skillIt->m_skillName.AsChar(), i ) );
			}
		}
	}
}


Bool CTraitValidator::Validate( String& errorMsg /* out */ )
{
	m_index = 0;
	m_isValid = true;
	m_errorMessages.Clear();

	CheckUniqueTraitNames();
	CheckUniqueSkillNames();
	CheckUniqueUnlockedTraitNames();
	CheckSkillTrait();

	if ( !m_isValid )

	{
		errorMsg = GetErrorMsg();
	}
	return m_isValid;
}

void CTraitValidator::AddErrorMsg( const String &errorMsg )
{
	m_isValid = false;
	m_errorMessages.PushBack( errorMsg );
}

String CTraitValidator::GetErrorMsg()
{
	String res;

	for ( TDynArray< String >::const_iterator errMsg = m_errorMessages.Begin();
		  errMsg != m_errorMessages.End();
		  ++errMsg )
	{
		res += *errMsg + TXT( "\n" );
	}

	return res;
}

void CTraitValidator::CompareSkillsToTraitRequirements( bool addAndNotRemove )
{
	// for all skills
	TDynArray< SSkillTableEntry >& skills = m_traitData->GetSkillTable();
	TDynArray< SSkillTableEntry >::iterator skillIt = skills.Begin();
	TDynArray< SSkillTableEntry >::iterator skillItEnd = skills.End();

	for ( skillIt; skillIt != skillItEnd; ++skillIt )
	{
		SSkillTableEntry& skill = *skillIt;

		// for all levels
		for ( Int32 i = SSkillTableEntry::SKILL_HIGHEST_LEVEL; i > 0; --i )
		{
			RED_ASSERT( skill.GetLevel( i ) );
			TDynArray<CName>& levelTraits = *skill.GetLevel( i );

			// for all unlocked trait on level list
			for ( Uint32 j = 0; j < levelTraits.Size(); ++j )
			{
				CName traitName = levelTraits[j];
				
				// find the trait with name
				STraitTableEntry* foundTrait = m_traitData->GetTrait( traitName );
				if ( !foundTrait )
				{
					levelTraits.Erase( levelTraits.Begin() + j );
					--j;
					continue;
				}

				bool foundSkillRequirement = false;

				// check if it contains requirement list
				if ( foundTrait->m_requirements )
				{
					TDynArray< IRequirement* >& requirements = foundTrait->m_requirements->m_requirements;
					TDynArray< IRequirement* >::iterator traitRequirementsIt = requirements.Begin();
					TDynArray< IRequirement* >::iterator traitRequirementsItEnd = requirements.End();

					// try to find skill requirement with given skill name and level
					for ( traitRequirementsIt; traitRequirementsIt != traitRequirementsItEnd; ++traitRequirementsIt )
					{
						IRequirement* traitRequirement = *traitRequirementsIt;
						RED_ASSERT( traitRequirement );
						if ( traitRequirement->IsA( CSkillRequirement::GetStaticClass() ) )
						{
							CSkillRequirement* skillRequirement = Cast<CSkillRequirement>( traitRequirement );
							if ( skillRequirement->m_skillName == skill.m_skillName && skillRequirement->m_skillLevel == i )
							{
								foundSkillRequirement = true;
							}
						}
					}
				}


				// if not found update data
				if ( !foundSkillRequirement )
				{
					if ( addAndNotRemove )
					{
						if ( !foundTrait->m_requirements )
						{
							foundTrait->m_requirements = new CTraitRequirementWrapper();
						}
						// add not found skill to trait requirement
						foundTrait->m_requirements->m_requirements.PushBack( new CSkillRequirement( skill.m_skillName, i ) );
					}
					else
					{
						// remove not found trait from level list with unlocked traits
						levelTraits.Erase( levelTraits.Begin() + j );
						--j;
					}
				}
			}
		}
	}
}

void CTraitValidator::CompareTraitRequirementsToSkills( bool addAndNotRemove )
{
	// For all traits
	TDynArray< STraitTableEntry >& traits = m_traitData->GetTraitTable();
	TDynArray< STraitTableEntry >::iterator traitIt = traits.Begin();
	TDynArray< STraitTableEntry >::iterator traitItEnd = traits.End();

	for ( traitIt; traitIt != traitItEnd; ++traitIt )
	{
		STraitTableEntry& trait = *traitIt;

		if ( !trait.m_requirements )
		{
			continue;
		}
		// for all trait requirements
		TDynArray< IRequirement* > requirements = trait.m_requirements->m_requirements;
		for ( Uint32 i = 0; i < requirements.Size(); ++i )
		{
			IRequirement* traitRequirement = requirements[i];
			RED_ASSERT( traitRequirement );
			if ( !traitRequirement )
			{
				requirements.Erase( requirements.Begin() + i );
				continue;
			}
			// get only skill requirements
			if ( !traitRequirement->IsA( CSkillRequirement::GetStaticClass() ) )
			{
				continue;
			}
			CSkillRequirement* skillRequirement = Cast<CSkillRequirement>( traitRequirement );

			// find skill with given name
			SSkillTableEntry* foundSkill = m_traitData->GetSkill( skillRequirement->m_skillName );

			if ( !foundSkill )
			{
				requirements.Erase( requirements.Begin() + i );
				--i;
				continue;
			}

			skillRequirement->m_skillLevel = Clamp<Uint32>( skillRequirement->m_skillLevel, 1, 10 );
			// add level with given index
			TDynArray<CName>* levelTraits = foundSkill->GetLevel( skillRequirement->m_skillLevel );
			RED_ASSERT( levelTraits );

			TDynArray< CName >::iterator levelTraitsNameIt = levelTraits->Begin();
			TDynArray< CName >::iterator levelTraitsNameItEnd = levelTraits->End();

			bool found = false;
			// try to find trait name on level unlocked trait list
			for ( levelTraitsNameIt; levelTraitsNameIt != levelTraitsNameItEnd; ++levelTraitsNameIt )
			{
				CName& traitName = *levelTraitsNameIt;

				if ( traitName == trait.m_traitName )
				{
					found = true;
				}
			}
			// if not found update data
			if ( !found )
			{
				if ( addAndNotRemove )
				{
					// add not found trait name to the level unlocked traits list
					levelTraits->PushBack( trait.m_traitName );
				}
				else
				{
					// delete not found requirement from trait requirements list
					requirements.Erase( requirements.Begin() + i );
					--i;

					if ( requirements.Size() == 0 )
					{
						trait.m_requirements = NULL;
						break;
					}
				}
			}
		}
	}
}

void CTraitValidator::UpdateSkillWithTraitData()
{
	// Compare skills to traits requirements and remove old trait names from level unlocked trait list
	CompareSkillsToTraitRequirements( false );
	// Compare trait requirements to skills and add new trait names to level unlocked trait list
	CompareTraitRequirementsToSkills( true );
}

void CTraitValidator::UpdateTraitWithSkillData()
{
	// Compare skills to traits requirements and add new requirements 
	CompareTraitRequirementsToSkills( false );
	// Compare trait requirements to skills and delete old requirements
	CompareSkillsToTraitRequirements( true );
}
#endif