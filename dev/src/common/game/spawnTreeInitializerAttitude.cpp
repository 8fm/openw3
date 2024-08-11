#include "build.h"
#include "spawnTreeInitializerAttitude.h"

IMPLEMENT_ENGINE_CLASS( CSpawnTreeInitializerAttitude );

////////////////////////////////////////////////////////////////////
// CSpawnTreeInitializerAttitude
////////////////////////////////////////////////////////////////////

Int32 CSpawnTreeInitializerAttitude::s_attitudeGroupPriority = -1;

ISpawnTreeInitializer::EOutput CSpawnTreeInitializerAttitude::Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const
{
	if ( !m_attitudeGroup.Empty() )
	{
		actor->SetTemporaryAttitudeGroup( m_attitudeGroup, GetAttitudeGroupPriority() );
	}
	return OUTPUT_SUCCESS;
}
void CSpawnTreeInitializerAttitude::Deactivate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const
{
	actor->ResetTemporaryAttitudeGroup( GetAttitudeGroupPriority() );
}
String CSpawnTreeInitializerAttitude::GetBlockCaption() const
{
	return String::Printf( TXT("Attitude '%ls'"), m_attitudeGroup.AsString().AsChar() );
}
String CSpawnTreeInitializerAttitude::GetEditorFriendlyName() const
{
	static String STR( TXT("Attitude") );
	return STR;
}

Int32 CSpawnTreeInitializerAttitude::GetAttitudeGroupPriority()
{
	if ( s_attitudeGroupPriority != -1 )
	{
		return s_attitudeGroupPriority;
	}
	s_attitudeGroupPriority = 0; // default - the safest value
	CEnum* agpEnum = SRTTI::GetInstance().FindEnum( CNAME( EAttitudeGroupPriority ) );
	RED_ASSERT( agpEnum != nullptr , TXT( "Cannot find EAttitudeGroupPriority enum. Please synchronize enum names" ) );
	if ( agpEnum != nullptr )
	{
		RED_VERIFY( agpEnum->FindValue( CNAME( AGP_SpawnTree ), s_attitudeGroupPriority ), TXT( "Cannot find AGP_SpawnTree in EAttitudeGroupPriority enum." ) );
	}
	return s_attitudeGroupPriority;
}