/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

//////////////////////////////////////////////////////////////////////////////////////////////
enum EDoorState
{
	Door_Closed,
	Door_Open,
};

BEGIN_ENUM_RTTI( EDoorState );
	ENUM_OPTION( Door_Closed );
	ENUM_OPTION( Door_Open );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////////////////////////
class CDoorComponent : public CInteractionComponent
{
	DECLARE_ENGINE_CLASS( CDoorComponent, CInteractionComponent, 0 );
private:
	EDoorState				m_initialState;
	EDoorState				m_currentState;
	EDoorState				m_desiredState;
	Bool					m_doorsEnebled;
	Bool					m_isTrapdoor;

	String					m_openName;
	String					m_closeName;
	
	TDynArray< THandle< CActor > >	m_doorUsers;

	Bool					m_initialized;

	Bool					m_streamInit;
	CName					m_lastCalledCombatNotification;
	Float					m_savedPushableAccumulatedYaw;
public:
	CDoorComponent();

	// functions to get current state of the door
	Bool IsOpen()			{ return m_currentState == m_desiredState && m_currentState == Door_Open; }
	Bool IsClosed()			{ return m_currentState == m_desiredState && m_currentState == Door_Closed; }
	Bool IsTrapdoor()		{ return m_isTrapdoor; }
	Bool IsMoving()			{ return m_currentState != m_desiredState; }
	Bool IsLocked();

	Float SavedPushableAccumulatedYaw() const { return m_savedPushableAccumulatedYaw; }

	// manipulate door functions
	void Open( Bool instant = false, Bool openLock = false, Bool otherSide = false );
	void Close( Bool instant = false );

	void SetStateForced();

	EDoorState GetCurrentState() const				{ return m_currentState; }
	EDoorState GetDesiredState() const				{ return m_desiredState; }

	const TDynArray< THandle< CActor > >& GetDoorUsers()		{ return m_doorUsers; }
	
	virtual void OnPostLoad();
	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;
	virtual void OnSaveGameplayState( IGameSaver* saver ) override;
	virtual void OnLoadGameplayState( IGameLoader* loader ) override;
	virtual Bool CheckShouldSave() const override { return true; }

	// This component was created by the streaming system
	virtual void OnStreamIn() override;
	virtual void OnStreamOut() override;

	// tick only active when door is changing state (otherwise suppressed)
	void OnTick( Float timeDelta );

	void AddDoorUser( CActor* actor );

	void EnebleDoors( Bool enable );
	Bool IsDoorEnabled() const { return m_doorsEnebled; }

	void SetEnabled( Bool enabled ) override;

	Bool IsInteractive();

protected:
	IDoorAttachment* FindDoorAttachment();
	virtual Bool NotifyActivation( CEntity* activator ) override;
	void UpdateActionName();
	void SetDesiredState( EDoorState newState );
	virtual void OnExecute() override;

	void NotifyEntityInNeeded();

private:
	void Unsuppress();
	void Init();
	void Deinit();

	Bool LazyInit( Bool openOtherSise = false );

	void HandleCombat();

private:
	void funcOpen( CScriptStackFrame& stack, void* result );	
	void funcClose( CScriptStackFrame& stack, void* result );
	void funcIsOpen( CScriptStackFrame& stack, void* result );
	void funcIsLocked( CScriptStackFrame& stack, void* result );
	void funcAddForceImpulse( CScriptStackFrame& stack, void* result );
	void funcInstantClose( CScriptStackFrame& stack, void* result );
	void funcInstantOpen( CScriptStackFrame& stack, void* result );
	void funcAddDoorUser( CScriptStackFrame& stack, void* result );
	void funcEnebleDoors( CScriptStackFrame& stack, void* result );
	void funcIsInteractive( CScriptStackFrame& stack, void* result );
	void funcIsTrapdoor( CScriptStackFrame& stack, void* result );
	void funcInvertMatrixForDoor( CScriptStackFrame& stack, void* result );
	void funcUnsuppress( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CDoorComponent );
	PARENT_CLASS( CInteractionComponent );

	PROPERTY_EDIT( m_initialState, TXT( "Initial Door State" ) );
	PROPERTY_EDIT( m_isTrapdoor, TXT( "Initial Door State" ) );
	PROPERTY_SAVED( m_doorsEnebled );	
	PROPERTY_CUSTOM_EDIT( m_openName, TXT("Open name"), TXT("2daValueSelection") );
	PROPERTY_CUSTOM_EDIT( m_closeName, TXT("Close name"), TXT("2daValueSelection") );

	NATIVE_FUNCTION( "Open", funcOpen );	
	NATIVE_FUNCTION( "Close", funcClose );
	NATIVE_FUNCTION( "IsOpen", funcIsOpen );
	NATIVE_FUNCTION( "IsLocked", funcIsLocked );
	NATIVE_FUNCTION( "AddForceImpulse", funcAddForceImpulse );
	NATIVE_FUNCTION( "InstantClose", funcInstantClose );
	NATIVE_FUNCTION( "InstantOpen", funcInstantOpen );
	NATIVE_FUNCTION( "AddDoorUser", funcAddDoorUser );
	NATIVE_FUNCTION( "EnebleDoors", funcEnebleDoors );
	NATIVE_FUNCTION( "IsInteractive", funcIsInteractive );
	NATIVE_FUNCTION( "IsTrapdoor", funcIsTrapdoor );
	NATIVE_FUNCTION( "InvertMatrixForDoor", funcInvertMatrixForDoor );
	NATIVE_FUNCTION( "Unsuppress", funcUnsuppress );
END_CLASS_RTTI();
