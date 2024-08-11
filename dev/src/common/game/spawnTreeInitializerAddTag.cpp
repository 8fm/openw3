#include "build.h"
#include "spawnTreeInitializerAddTag.h"
#include "actor.h"

IMPLEMENT_ENGINE_CLASS( CSpawnTreeInitializerAddTag );

////////////////////////////////////////////////////////////////////
// CSpawnTreeInitializerDrawWeapon
////////////////////////////////////////////////////////////////////
void CSpawnTreeInitializerAddTag::OnCreatureSpawn( EntitySpawnInfo& entityInfo, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const
{
	entityInfo.m_tags.AddTags( m_tag );	
}

Bool CSpawnTreeInitializerAddTag::Accept( CActor* actor ) const
{
	if ( m_onlySetOnSpawnAppearance && !TagList::MatchAll( m_tag, actor->GetTags() ) )
	{
		return false;
	}
	return true;
}
ISpawnTreeInitializer::EOutput CSpawnTreeInitializerAddTag::Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const
{
	if ( !m_onlySetOnSpawnAppearance || reason == ISpawnTreeInitializer::EAR_GameIsRestored )
	{
		if( !m_tag.Empty() )
		{
			TagList tagList = actor->GetTags();
			tagList.AddTags( m_tag );

			actor->SetTags( Move( tagList ) );
		}
	}

	return OUTPUT_SUCCESS;
}

void CSpawnTreeInitializerAddTag::Deactivate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const
{
	if ( !m_onlySetOnSpawnAppearance && !m_tag.Empty() )
	{
		TagList tagList = actor->GetTags();
		tagList.SubtractTags( m_tag );

		actor->SetTags( Move( tagList ) );
	}
}

Bool CSpawnTreeInitializerAddTag::CallActivateOnSpawn() const
{
	return false;
}

String CSpawnTreeInitializerAddTag::GetBlockCaption() const
{	
	String ret = TXT( "Add tag:");
	if ( m_tag.Empty() )
	{
		ret += TXT( " EMPTY" );
	}
	else
	{
		for ( auto it = m_tag.GetTags().Begin(), end = m_tag.GetTags().End(); it != end; ++it )
		{
			ret += String::Printf( TXT(" '%ls'"), (*it).AsChar() );
		}
	}
	return ret;
}	

String CSpawnTreeInitializerAddTag::GetEditorFriendlyName() const
{
	static String STR( TXT("AddTag") );
	return STR;
}

Bool CSpawnTreeInitializerAddTag::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
	if ( propertyName.AsString() == TXT( "tag" ) )
	{
		const CName* prevName = reinterpret_cast< const CName* >( readValue.GetData() );
		m_tag.AddTag( *prevName );
		return true;
	}
	return OnPropertyTypeMismatch( propertyName, existingProperty, readValue );
}

#ifndef NO_EDITOR
CEntityTemplate* CSpawnTreeInitializerAddTag::Editor_GetEntityTemplate()
{
	return GetCreatureEntityTemplate();
}
#endif

void CSpawnTreeInitializerAddTag::funcAddTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tag, CName::NONE );
	FINISH_PARAMETERS;
	m_tag.AddTag( tag );
}