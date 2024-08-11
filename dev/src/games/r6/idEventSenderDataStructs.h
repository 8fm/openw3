#pragma once


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
struct SGeneralEventData
{
	DECLARE_RTTI_STRUCT( SGeneralEventData );

	CName			m_EventName;
	EntityHandle	m_EntityHandle;
};
BEGIN_NODEFAULT_CLASS_RTTI( SGeneralEventData );
	PROPERTY_EDIT( m_EventName, TXT("") );
	PROPERTY_EDIT( m_EntityHandle, TXT("") );
END_CLASS_RTTI();

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
struct SInterlocutorEventData
{
	DECLARE_RTTI_STRUCT( SInterlocutorEventData );

	CName		m_EventName;
	CName		m_InterlocutorName;
};
BEGIN_NODEFAULT_CLASS_RTTI( SInterlocutorEventData );
	PROPERTY_EDIT( m_EventName, TXT("") );
	PROPERTY_EDIT( m_InterlocutorName, TXT("") );
END_CLASS_RTTI();

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
struct SAnimationEventData
{
	DECLARE_RTTI_STRUCT( SAnimationEventData );

	CName	m_EventName;
	CName	m_InterlocutorName;
	Bool	m_Force;
	Bool	m_ToAll;
};
BEGIN_NODEFAULT_CLASS_RTTI( SAnimationEventData );
	PROPERTY_EDIT( m_EventName, TXT("") );
	PROPERTY_EDIT( m_InterlocutorName, TXT("") );
	PROPERTY_EDIT( m_Force, TXT("") );
	PROPERTY_EDIT( m_ToAll, TXT("") );
END_CLASS_RTTI();


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
enum EEncounterEventMode
{
	EAM_Enable	,
	EAM_Disable	,
	EAM_ChangePhase
};
BEGIN_ENUM_RTTI( EEncounterEventMode )
	ENUM_OPTION( EAM_Enable );
ENUM_OPTION( EAM_Disable );
ENUM_OPTION( EAM_ChangePhase );
END_ENUM_RTTI();

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
struct SEncounterPhaseData
{
	DECLARE_RTTI_STRUCT( SEncounterPhaseData );

	CName				m_encounterTag;
	CName				m_encounterSpawnPhase;
	EEncounterEventMode	m_enableMode;
};
BEGIN_NODEFAULT_CLASS_RTTI( SEncounterPhaseData );
	PROPERTY_EDIT( m_encounterTag, TXT("") );
	PROPERTY_EDIT( m_encounterSpawnPhase, TXT("") );
	PROPERTY_EDIT( m_enableMode, TXT("") );
END_CLASS_RTTI();


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
struct SIDSoundEventParams
{
	DECLARE_RTTI_STRUCT( SIDSoundEventParams );

	CName	m_entityTag;
	CName	m_boneName;
	String	m_sound;
	Bool	m_stop;
};
BEGIN_NODEFAULT_CLASS_RTTI( SIDSoundEventParams );
	PROPERTY_EDIT( m_entityTag	, TXT("") );
	PROPERTY_EDIT( m_boneName	, TXT("") );
	PROPERTY_EDIT( m_sound		, TXT("") );
	PROPERTY_EDIT( m_stop		, TXT("") );
END_CLASS_RTTI();


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
struct SAIEventData
{
	DECLARE_RTTI_STRUCT( SAIEventData );

	CName				m_InterlocutorName;
	IIDAIEventParam*	m_eventAndParam;
};
BEGIN_NODEFAULT_CLASS_RTTI( SAIEventData );
	PROPERTY_EDIT( m_InterlocutorName, TXT("") );
	PROPERTY_INLINED( m_eventAndParam, TXT("") );
END_CLASS_RTTI();

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class IIDAIEventParam : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IIDAIEventParam, CObject );

protected:
	CName	m_EventName;

public:
	virtual void CallEventOnActor( CActor*	actor ) const = 0;
};
BEGIN_ABSTRACT_CLASS_RTTI( IIDAIEventParam );
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_EventName, TXT("") );
END_CLASS_RTTI();

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CIDAIEventParamInt : public IIDAIEventParam
{
	DECLARE_ENGINE_CLASS( CIDAIEventParamInt, CObject, 0 );

private:
	Int32	m_intParam;

public:
	void CallEventOnActor( CActor*	actor ) const  override { actor->SignalGameplayEvent( m_EventName, m_intParam ); };

};
BEGIN_CLASS_RTTI( CIDAIEventParamInt );
	PARENT_CLASS( IIDAIEventParam );
	PROPERTY_EDIT( m_intParam, TXT("") );
END_CLASS_RTTI();

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CIDAIEventParamFloat : public IIDAIEventParam
{
	DECLARE_ENGINE_CLASS( CIDAIEventParamFloat, CObject, 0 );

private:
	Float	m_FloatParam;

public:
	void CallEventOnActor( CActor*	actor ) const  override { actor->SignalGameplayEvent( m_EventName, m_FloatParam ); };

};
BEGIN_CLASS_RTTI( CIDAIEventParamFloat );
	PARENT_CLASS( IIDAIEventParam );
	PROPERTY_EDIT( m_FloatParam, TXT("") );
END_CLASS_RTTI();

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CIDAIEventParamName : public IIDAIEventParam
{
	DECLARE_ENGINE_CLASS( CIDAIEventParamName, CObject, 0 );

private:
	CName	m_nameParam;

public:
	void CallEventOnActor( CActor*	actor ) const  override { actor->SignalGameplayEvent( m_EventName, m_nameParam ); };

};
BEGIN_CLASS_RTTI( CIDAIEventParamName );
	PARENT_CLASS( IIDAIEventParam );
	PROPERTY_EDIT( m_nameParam, TXT("") );
END_CLASS_RTTI();