#pragma once



#include "r6InteractionComponent.h"




/// 
/// @created 2014-03-12
/// @author M.Sobiecki
/// 
/// @todo MS: We still have to distinguish situation when component is turned on after starting the game or off after ending
///				- otherwise, each time game starts some objects may start sound events and so on.
/// 
class CR6InteractionTurnOnOff : public CR6InteractionComponent
{
	DECLARE_ENGINE_CLASS( CR6InteractionTurnOnOff, CR6InteractionComponent, 0 );
	
private:

	Bool	m_on;

	CName	m_onAppearance;
	CName	m_offAppearance;

	String	m_onSoundEvent;
	String	m_offSoundEvent;

	String	m_turnedOnFact;

public:
	CR6InteractionTurnOnOff();


	virtual void OnAttached( CWorld* world ) override;

	virtual Bool CanInteract( CComponent* interactor ) const override;
	virtual Bool OnStartInteraction( CComponent* interactor ) override;

	

	virtual void OnStopInteraction( CComponent* interactor ) override;
	virtual void OnAbortInteraction( CComponent* interactor ) override;


private:


	void ApplyOnOffValue();
	void TryApplyAppearance( const CName& appearanceName );
	void TryApplySoundEvent( const String& soundEvent);

	void TryActivateFact();
	void TryDeactivateFact();




	Bool FactsDoesExist();
};





BEGIN_CLASS_RTTI( CR6InteractionTurnOnOff );
	
	PARENT_CLASS( CR6InteractionComponent );

	PROPERTY_EDIT( m_on					, TXT( "Should this device be initially on or off?" ) );
	PROPERTY_EDIT( m_onAppearance		, TXT( "Appearance to be set in CAppearanceComponent when interaction is 'on'. CName::NONE for no action." ) );
	PROPERTY_EDIT( m_offAppearance		, TXT( "Appearance to be set in CAppearanceComponent when interaction is 'on'. CName::NONE for no action." ) );
	PROPERTY_EDIT( m_onSoundEvent		, TXT( "Sound event to be raised when interaction is turned on." ) );
	PROPERTY_EDIT( m_offSoundEvent		, TXT( "Sound event to be raised when interaction is turned off." ) );
	PROPERTY_EDIT( m_turnedOnFact		, TXT( "Fact to be added to facts database when interaction is turned on (and removed when turned off)." ) );

END_CLASS_RTTI();
