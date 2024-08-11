#include "build.h"

#include "r6InteractionTurnOnOff.h"
#include "..\..\common\engine\appearanceComponent.h"
#include "..\..\common\game\factsDB.h"
#include "..\..\common\engine\soundEmitter.h"
#include "../../common/core/dataError.h"
#include "../../common/engine/gameTimeManager.h"
#include "../../common/engine/utils.h"

IMPLEMENT_ENGINE_CLASS( CR6InteractionTurnOnOff );






CR6InteractionTurnOnOff::CR6InteractionTurnOnOff()
	: m_on					( false )
	, m_onAppearance		( CName::NONE )
	, m_offAppearance		( CName::NONE )
{
}










void CR6InteractionTurnOnOff::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	ApplyOnOffValue();
}














Bool CR6InteractionTurnOnOff::CanInteract( CComponent* interactor ) const
{
	return true;
}











Bool CR6InteractionTurnOnOff::OnStartInteraction( CComponent* interactor )
{
	R6_ASSERT( GetEntity() );

	m_on = !m_on;
	ApplyOnOffValue();

	return true;
}













void CR6InteractionTurnOnOff::OnStopInteraction( CComponent* interactor )
{

}












void CR6InteractionTurnOnOff::OnAbortInteraction( CComponent* interactor )
{

}










void CR6InteractionTurnOnOff::ApplyOnOffValue()
{
	if( m_on )
	{
		TryApplyAppearance( m_onAppearance );
		TryApplySoundEvent( m_onSoundEvent );
		TryActivateFact();
		CallEvent( CNAME( OnInteractionTurnedOn ) );
	}
	else
	{
		TryApplyAppearance( m_offAppearance );
		TryApplySoundEvent( m_offSoundEvent );
		TryDeactivateFact();
		CallEvent( CNAME( OnInteractionTurnedOff ) );
	}
}










void CR6InteractionTurnOnOff::TryApplyAppearance( const CName& appearanceName )
{
	if( appearanceName == CName::NONE )
	{
		return;
	}

	auto cit = ComponentIterator< CAppearanceComponent >( GetEntity() );
	
	if( !cit )
	{
		DATA_HALT( DES_Minor, CResourceObtainer::GetResource( this ), TXT( "Interaction" ), TXT( "Probably CAppearanceComponent was ment to exist for this entity - interaction tried to change it." ) );
	}
		 
	while( cit )
	{
		R6_ASSERT( *cit );
		(*cit)->ApplyAppearance( appearanceName );
		++cit;
	}
}













void CR6InteractionTurnOnOff::TryApplySoundEvent( const String& soundEvent )
{
	if( soundEvent.Empty() )
	{
		return;
	}


	CSoundEmitterComponent* soundEmitterComponent = GetEntity()->GetSoundEmitterComponent();
	if( !soundEmitterComponent )
	{
		DATA_HALT( DES_Minor, CResourceObtainer::GetResource( this ), TXT( "Interaction" ), TXT( "Probably CSoundEmitterComponent was ment to exist for this entity - interaction tried to reference it." ) );
		return;
	}

	soundEmitterComponent->SoundEvent( UNICODE_TO_ANSI( soundEvent.AsChar() ) );
}












void CR6InteractionTurnOnOff::TryActivateFact()
{
	if( !( m_turnedOnFact.Empty() ) && !( FactsDoesExist() ) )
	{
		const EngineTime& time = GGame->GetEngineTime();
		if ( GCommonGame && GCommonGame->GetSystem< CFactsDB >() )
		{
			GCommonGame->GetSystem< CFactsDB >()->AddFact( m_turnedOnFact, 1, time );
		}
	}
}












void CR6InteractionTurnOnOff::TryDeactivateFact()
{
	
	if( !( m_turnedOnFact.Empty() ) && FactsDoesExist() )
	{
		if ( GCommonGame && GCommonGame->GetSystem< CFactsDB >() )
		{	
			GCommonGame->GetSystem< CFactsDB >()->RemoveFact( m_turnedOnFact );
		}
	}
}










Bool CR6InteractionTurnOnOff::FactsDoesExist()
{
	Bool doesExist = false;

	if ( GCommonGame && GCommonGame->GetSystem< CFactsDB >() )
	{	
		doesExist = GCommonGame->GetSystem< CFactsDB >()->DoesExist( m_turnedOnFact );
	}

	return doesExist;
}




