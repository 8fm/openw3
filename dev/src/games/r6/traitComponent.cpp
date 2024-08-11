#include "build.h"
#include "traitComponent.h"
#include "traitData.h"
#include "../../common/core/gameSave.h"

IMPLEMENT_ENGINE_CLASS( CTraitComponent );
IMPLEMENT_RTTI_ENUM( ETraitState );

RED_DEFINE_STATIC_NAME ( SkillNum );
RED_DEFINE_STATIC_NAME ( SkillInfo );
RED_DEFINE_STATIC_NAME ( SkillName );
RED_DEFINE_STATIC_NAME ( SkillLevel );
RED_DEFINE_STATIC_NAME ( TraitNum );
RED_DEFINE_STATIC_NAME ( TraitInfo );
RED_DEFINE_STATIC_NAME ( TraitName );
RED_DEFINE_STATIC_NAME ( TraitState );
RED_DEFINE_STATIC_NAME ( TraitRequirement );
RED_DEFINE_STATIC_NAME ( TraitRequirementInfo );
RED_DEFINE_STATIC_NAME ( TraitRequirementNum );

void CTraitComponent::OnPostInstanced()
{
	 TBaseClass::OnPostInstanced(); 
	 InitRuntimeData();
}

void CTraitComponent::OnLoadGameplayState( IGameLoader* loader ) 
{ 
	TBaseClass::OnLoadGameplayState( loader ); 

	Uint32 skillNum = loader->ReadValue< Uint32 >( CNAME( SkillNum ) );

	TDynArray< SSkillTableEntry >& skills = m_traitData->GetSkillTable();

	for ( Uint32 i = 0; i < skillNum; i++ )
	{		
		CGameSaverBlock block( loader, CNAME( SkillInfo ) );
		CName skillName = loader->ReadValue< CName >( CNAME( SkillName ), CName::NONE );
		Int32 skillLevel = loader->ReadValue< Int32 >( CNAME( SkillLevel ) );
		Int32* level = m_skillLevels.FindPtr( skillName );
		if ( level )
		{
			*level = skillLevel;
		}
	}

	Uint32 traitNum = loader->ReadValue< Uint32 >( CNAME( TraitNum ) );

	TDynArray< STraitTableEntry >& traits = m_traitData->GetTraitTable();

	for ( Uint32 i = 0; i < traitNum; i++ )
	{	
		CGameSaverBlock block( loader, CNAME( TraitInfo ) );
		CName traitName = loader->ReadValue< CName >( CNAME( TraitName ), CName::NONE );

		TraitInfo* info = m_traitStates.FindPtr( traitName );
		if ( !info )
		{
			continue;
		}
		ETraitState traitState = loader->ReadValue< ETraitState >( CNAME( TraitState ) );
		info->m_state = traitState;

		Uint32 traitRequirementNum = loader->ReadValue< Uint32 >( CNAME( TraitRequirementNum ) );

		if ( traitRequirementNum != info->m_requirements.Size() )
		{
			continue;
		}
		for ( Uint32 j = 0; j < traitRequirementNum; ++j )
		{
			CGameSaverBlock blockR( loader, CNAME( TraitRequirementInfo ) );
			Bool reqMet = loader->ReadValue< Bool >( CNAME( TraitRequirement ) );
			info->m_requirements[j] = reqMet;
		}
	}
}

void CTraitComponent::OnSaveGameplayState( IGameSaver* saver )
{
	TBaseClass::OnSaveGameplayState( saver );

	saver->WriteValue( CNAME( SkillNum ), m_skillLevels.Size() );

	for ( auto it = m_skillLevels.Begin(); it != m_skillLevels.End(); ++it )
	{
		CGameSaverBlock block( saver, CNAME( SkillInfo ) );
		saver->WriteValue( CNAME( SkillName ), it->m_first );
		saver->WriteValue( CNAME( SkillLevel ), it->m_second );
	}

	saver->WriteValue( CNAME( TraitNum ), m_traitStates.Size() );

	for ( auto it = m_traitStates.Begin(); it != m_traitStates.End(); ++it )
	{
		CGameSaverBlock block( saver, CNAME( TraitInfo ) );
		saver->WriteValue( CNAME( TraitName ), it->m_first );
		TraitInfo& info = it->m_second;
		saver->WriteValue( CNAME( TraitState ), info.m_state );

		saver->WriteValue( CNAME( TraitRequirementNum ), info.m_requirements.Size() );
		for ( Uint32 i = 0; i < info.m_requirements.Size(); i++ )
		{
			CGameSaverBlock blockR( saver, CNAME( TraitRequirementInfo ) );
			saver->WriteValue( CNAME( TraitRequirement ), info.m_requirements[i] );
		}
	}
}


bool CTraitComponent::SkillRequirementChecker::Check( IRequirement* req ) const
{
	if( req->IsA( CSkillRequirement::GetStaticClass() ) )
	{
		return ( ( CSkillRequirement* ) req )->IsMet( m_skill, m_level );
	}

	return false;
}


bool CTraitComponent::TraitRequirementChecker::Check( IRequirement* req ) const
{
	if( req->IsA( CTraitRequirement::GetStaticClass() ) )
	{
		return ( ( CTraitRequirement* ) req )->IsMet( m_trait );
	}

	return false;
}

void CTraitComponent::CheckSkillRequirement( CName skillName, Int32 level )
{
	CTraitComponent::SkillRequirementChecker skillReqChecker;
	skillReqChecker.m_skill = skillName;
	skillReqChecker.m_level = level;
	CheckRequirement( skillReqChecker );
}

void CTraitComponent::CheckTraitRequirement( CName traitName )
{
	CTraitComponent::TraitRequirementChecker traitReqChecker;
	traitReqChecker.m_trait = traitName;
	CheckRequirement( traitReqChecker );
}

void CTraitComponent::CheckRequirement( CTraitComponent::RequirementChecker& checker )
{
	TDynArray< STraitTableEntry >& traits = m_traitData->GetTraitTable();
	for ( Uint32 i = 0; i < traits.Size(); ++i )
	{
		STraitTableEntry& traitEntry = traits[i];
		TraitInfo* info = m_traitStates.FindPtr( traitEntry.m_traitName );
		RED_ASSERT( info );

		// trait has already been unlocked
		if ( info->m_state != TS_locked )
		{
			continue;
		}
		
		RED_ASSERT( info->m_requirements.Size() > 0, TXT( "trait with no requirement should be unlocked" ) );

		// assume we have to change trait state
		Bool changed = true; 
		for ( Uint32 j = 0; j < info->m_requirements.Size(); j++ )
		{
			// requirement is met
			if ( info->m_requirements[ j ] )
			{
				continue;
			}
			RED_ASSERT( traitEntry.m_requirements, TXT( "Incompatible editor and runtime data" ) );
			IRequirement* req = traitEntry.m_requirements->m_requirements[j];
			if ( checker.Check( req ) )
			{
				info->m_requirements[ j ] = true;
			}
			else
			{
				// if at least one requirement isn't met state of trait remains locked
				changed = false;
			}
		}
		if ( changed )
		{
			info->m_state = TS_unlocked;
		}
	}
}

Bool CTraitComponent::TryUsingResource( CResource * resource )
{
	CTraitData* traitData = Cast< CTraitData >( resource );
	if ( traitData )
	{
		m_traitData = traitData;
		return true;
	}
	return false;
}

bool CTraitComponent::BuySkill( CName skill )
{
	if ( !GetSkill( skill ) )
	{
		return false;
	}
	Int32* level = m_skillLevels.FindPtr( skill );
	RED_ASSERT( level, TXT( "Incompatible editor and runtime data" ) );
	if ( level && *level == 0 )
	{
		m_skillLevels.Set( skill, 1 );
		CheckSkillRequirement( skill, 1 );
		return true;
	}
	return false;
}

bool CTraitComponent::IncreaseSkillLevel( CName skill, Int32 level )
{
	if ( !GetSkill( skill ) )
	{
		return false;
	}
	if ( level <= 0 || level > SSkillTableEntry::SKILL_HIGHEST_LEVEL )
	{
		return false;
	}
	Int32* oldLevel = m_skillLevels.FindPtr( skill );
	RED_ASSERT( oldLevel, TXT( "Incompatible editor and runtime data" ) );
	if ( !oldLevel || *oldLevel > level )
	{
		return false;
	}
	if ( *oldLevel == 0 )
	{
		return false;
	}

	m_skillLevels.Set( skill, level );
	CheckSkillRequirement( skill, 1 );
	return true;
}

bool CTraitComponent::BuyTrait( CName trait )
{
	if ( !GetTrait( trait ) )
	{
		return false;
	}
	TraitInfo* state = m_traitStates.FindPtr( trait );
	RED_ASSERT( state, TXT( "Incompatible editor and runtime data" ) );
	if ( !state || state->m_state != TS_unlocked )
	{		
		return false;
	}
	const STraitTableEntry* traitEntry = m_traitData->GetTrait( trait );
	if ( !traitEntry )
	{
		return false;
	}
	state->m_state = TS_active;
	// check if buying this trait unlocks other traits
	CheckTraitRequirement( trait );

	if ( traitEntry->m_ability )
	{
		traitEntry->m_ability->m_ability->Activate();
	}
		
	return true;
}

ETraitState CTraitComponent::GetTraitState( CName name ) const
{
	const TraitInfo* state = m_traitStates.FindPtr( name );
	return state ? state->m_state : TS_locked;
}

Int32 CTraitComponent::GetSkillLevel( CName name ) const
{
	const Int32* level = m_skillLevels.FindPtr( name );
	return level ? *level : 0;
}

void CTraitComponent::funcGetTraitData( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_HANDLE( CTraitData, GetTraitData() );
}

void CTraitComponent::funcBuySkill( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;
	
	RETURN_BOOL( BuySkill( name ) );
}

void CTraitComponent::funcIncreaseSkillLevel( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( Int32, level, 0 );
	FINISH_PARAMETERS;

	RETURN_BOOL( IncreaseSkillLevel( name, level ) );
}

void CTraitComponent::funcBuyTrait( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( BuyTrait( name ) );
}

void CTraitComponent::funcGetTraitState( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_ENUM( GetTraitState( name ) );
}

void CTraitComponent::funcGetSkillLevel( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_INT( GetSkillLevel( name ) );
}

void CTraitComponent::funcReset( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	InitRuntimeData();

	RETURN_VOID();
}

void CTraitComponent::InitRuntimeData()
{
	R6_ASSERT( m_traitData );

	TDynArray< SSkillTableEntry >& skills = m_traitData->GetSkillTable();
	m_skillLevels.Reserve( skills.Size() );
	for ( Uint32 i = 0; i < skills.Size(); ++i )
	{
		int level = 0;
		m_skillLevels.Set( skills[i].m_skillName, level );
	}

	TDynArray< STraitTableEntry >& traits = m_traitData->GetTraitTable();
	m_traitStates.Reserve( traits.Size() );
	for ( Uint32 i = 0; i < traits.Size(); ++i )
	{
		TraitInfo info;
		info.m_state = TS_locked;
		info.m_requirements.Resize( traits[i].m_requirements ? traits[i].m_requirements->m_requirements.Size() : 0 );
		for ( Uint32 j = 0; j < info.m_requirements.Size(); ++j )
		{
			info.m_requirements[j] = false;
		}
		if ( info.m_requirements.Size() == 0 )
		{
			info.m_state = TS_unlocked;
		}
		m_traitStates.Set( traits[i].m_traitName, info );
	}
}
