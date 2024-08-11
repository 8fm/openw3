#include "build.h"
#include "extAnimProjectileEvent.h"
#include "ProjectileTrajectory.h"
#include "../engine/dynamicLayer.h"


IMPLEMENT_ENGINE_CLASS( CExtAnimProjectileEvent );
IMPLEMENT_RTTI_ENUM( EProjectileCastPosition );

CExtAnimProjectileEvent::CExtAnimProjectileEvent()
: m_spell( NULL )
{
}

CExtAnimProjectileEvent::CExtAnimProjectileEvent( const CName& eventName,
					  const CName& animationName, 
					  Float startTime, 
					  const String& trackName )
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
	, m_spell( NULL )
	, m_castPosition( PCP_EntityRoot )
{
}

void CExtAnimProjectileEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	ASSERT( component );

	if ( !m_spell )
	{
		WARN_GAME( TXT( "Information required to cast a spell is incomplete" ) );
		return;
	}

	CWorld* world = GGame->GetActiveWorld();
	if ( !world )
	{
		WARN_GAME( TXT( "No world to cast the spell in" ) );
		return;
	}

	CEntity* caster = component->GetEntity();

	// Try to create entity on dynamic layer
	EntitySpawnInfo einfo;

	Bool spawnPositioningResult = false;
	switch( m_castPosition )
	{
	case PCP_Bone:
		spawnPositioningResult = GetSituationFromBone( *caster, einfo.m_spawnPosition, einfo.m_spawnRotation );
		break;
	case PCP_EntityRoot:
		spawnPositioningResult = GetSituationFromEntity( *caster, einfo.m_spawnPosition, einfo.m_spawnRotation );
		break;
	default:
		spawnPositioningResult = false;
		break;
	}

	if ( !spawnPositioningResult )
	{
		return;
	}

	einfo.m_template = m_spell;
	einfo.m_appearances = m_spell->GetEnabledAppearancesNames();
	CEntity* spellEntity = world->GetDynamicLayer()->CreateEntitySync( einfo );

	ASSERT( spellEntity );
	ASSERT( spellEntity->IsA< CProjectileTrajectory >() );

	CProjectileTrajectory* spell = Cast< CProjectileTrajectory >( spellEntity );
	spell->Initialize( caster );
}

Bool CExtAnimProjectileEvent::GetSituationFromBone( const CEntity& caster, Vector& outPosition, EulerAngles& outRotation ) const
{
	if ( m_boneName.Empty() )
	{
		ERR_GAME( TXT( "Name of the bone from which a spell should be cast is not specified" ) );
		return false;
	}

	CAnimatedComponent* ac = caster.GetRootAnimatedComponent();
	ASSERT ( ac, TXT( "Caster doesn't have an animated component" ) );
	if ( !ac )
	{
		return false;
	}

	const ISkeletonDataProvider* provider = ac->QuerySkeletonDataProvider();
	ASSERT ( provider, TXT( "Caster doesn't have a skeleton" ) );
	if ( !provider )
	{
		return false;
	}

	Int32 boneIndex = provider->FindBoneByName( m_boneName );
	ASSERT ( boneIndex >= 0, TXT( "Caster doesn't have a bone '%ls' in his skeleton" ), m_boneName.AsString().AsChar() );
	if ( boneIndex < 0 )
	{
		return false;
	}

	Matrix boneWorldMatrix = provider->GetBoneMatrixWorldSpace( boneIndex );
	outPosition = boneWorldMatrix.GetTranslation();
	outRotation = boneWorldMatrix.ToEulerAngles();
	return true;
}

Bool CExtAnimProjectileEvent::GetSituationFromEntity( const CEntity& caster, Vector& outPosition, EulerAngles& outRotation ) const
{
	outPosition = caster.GetWorldPosition();
	outRotation = caster.GetWorldRotation();

	return true;
}

///////////////////////////////////////////////////////////////////////////////
