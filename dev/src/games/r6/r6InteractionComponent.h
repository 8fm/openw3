#pragma once


/// 
/// DESCRIPTION OF HOW DID I LEAVE THIS TASK WHEN I WAS TAKEN TO THE WITCHER:
///
/// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
/// 
/// @todo MS:
///		In case of problems (actions are not performed) look at if ( node->IsActive( *this ) ) line in CBehaviorGraphInstance::FindNodeByName
/// 
/// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
/// 
/// @todo MS:
///		Also you might want to do this:
///		, m_isPlayerControlled				( false ) //m_parent.Get()->GetEntity()->IsPlayer() )
///		In: CPhysicsCharacterWrapper::CPhysicsCharacterWrapper
/// 
class CR6InteractionComponent : public CScriptedComponent
{
	DECLARE_ENGINE_CLASS( CR6InteractionComponent, CScriptedComponent, 0 );

private:
	Float								m_range;
	Float								m_radius;
	Float								m_positionTolerance;
	Float								m_rotationTolerance;
	Bool								m_useExactLocation;

	CComponent*							m_interactorComponent;
	Float								m_timeAccum;

	TDynArray< IQuestCondition* >		m_questConditions; // tw> not yet sure if this should use IQuestCondition, most probably not  
	THandle< CActor >					m_interactor;  // tw> deprecated, please don't use - leaving it here for a while to keep the backwards compatibility, but TODO: remove this

	static THashMap< CEntity*, TDynArray< CR6InteractionComponent* > >	sm_worldInteractions; // tw> keeping this as static variable is not the best idea ever, TODO: refactor, create CInteractionSystem as IGameSystem and keep the list there

public:
	CR6InteractionComponent();

	Bool				AreAllQuestConditionsFullfilled() const;

	virtual void		OnAttached( CWorld* world ) override;
	virtual void		OnDetached( CWorld* world ) override;

	RED_INLINE Float	GetRange() const { return m_range; }
	RED_INLINE Float	GetRadius() const { return m_radius; }
	RED_INLINE Float	GetPositionTolerance() const { return m_positionTolerance; }
	RED_INLINE Float	GetRotationTolerance() const { return m_rotationTolerance; }

	virtual Bool		OnPropertyMissing( CName propertyName, const CVariant& readValue ) override;

	Vector				GetInteractLocationForNode( const CNode* node ) const;
	EulerAngles			GetInteractRotationForNode( const CNode* node ) const;
	static CR6InteractionComponent* CheckNodeForInteraction( CNode* node );

	// script interface has to be refactored and moved to c++
	// this is more or less how the c++ interface should look like.
	// temporal solution is that it calls old script interface (based on CActor), but 
	// TODO: refactor it, rely only on components, put all the base code in c++
	Bool				IsUsableFor( CComponent* interactor ) const;
	Bool				Use( CComponent* interactor );
	void				FinishUsing( CComponent* interactor ); // this informs the interaction that it should end soon, but doesn't force it to stop now
	void				Abort( CComponent* interactor ); // this forces the intercaction to stop now, even if it looks bad
	Bool				Update( Float timeDelta ); // called by aiaction, returns false if interaction is completed
	void				CleanUp();

	RED_INLINE Bool	IsInUse() const { return m_interactorComponent != nullptr; };
	RED_INLINE Bool	IsUsedBy( CComponent* interactor ) const { return m_interactorComponent == interactor; }

protected:

	virtual Bool		CanInteract( CComponent* interactor ) const;
	virtual Bool		OnStartInteraction( CComponent* interactor );
	virtual void		OnStopInteraction( CComponent* interactor );
	virtual void		OnAbortInteraction( CComponent* interactor );
	virtual Bool		OnInteractionUpdate( Float timeDelta );

private:
	void				funcAreAllQuestConditionsFullfilled( CScriptStackFrame& stack, void* result );
	void				funcGetInteractLocation( CScriptStackFrame& stack, void* result );
	void				funcGetInteractRotation( CScriptStackFrame& stack, void* result );
	void				funcGetInteractRadius( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CR6InteractionComponent );
	PARENT_CLASS( CScriptedComponent );
	PROPERTY_EDIT_RANGE( m_range, TXT(""), FLT_EPSILON, FLT_MAX )
	PROPERTY_EDIT_RANGE( m_radius, TXT(""), 0.f, FLT_MAX )
	PROPERTY_EDIT_RANGE( m_positionTolerance, TXT("Position tolerance in meters."), FLT_EPSILON, FLT_MAX )
	PROPERTY_EDIT_RANGE( m_rotationTolerance, TXT("Rotation tolerance in degrees."), FLT_EPSILON, FLT_MAX )
	PROPERTY_EDIT( m_useExactLocation, TXT("") )
	PROPERTY_INLINED( m_questConditions, TXT( "Quest Conditions" ) )
	
	PROPERTY( m_interactorComponent )

	PROPERTY_NAME( m_interactor, TXT("i_interactor") ) // deprecation warning

	NATIVE_FUNCTION( "I_AreAllQuestConditionsFullfilled", funcAreAllQuestConditionsFullfilled )
	NATIVE_FUNCTION( "GetInteractLocation", funcGetInteractLocation )
	NATIVE_FUNCTION( "GetInteractRotation", funcGetInteractRotation )
	NATIVE_FUNCTION( "GetInteractRadius", funcGetInteractRadius )
END_CLASS_RTTI();
