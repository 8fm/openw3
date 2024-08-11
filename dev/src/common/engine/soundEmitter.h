#pragma once

#include "component.h"
#include "../core/objectReachability.h"

class ITriggerActivator;

enum ESoundEmitterFlags
{
	SEF_WhileStopping					= FLAG( 0 ),
	SEF_ProcessTiming					= FLAG( 1 ),
	SEF_WaitForImmediateOcclusionUpdate = FLAG( 2 ), 
	SEF_ForceUpdateReverb				= FLAG( 3 ), 
};

enum ESoundParameterCurveType
{
	ESPCT_Log3 = 0,
	ESPCT_Sine = 1,
	ESPCT_Log1 = 2,
	ESPCT_InversedSCurve = 3,
	ESPCT_Linear = 4,
	ESPCT_SCurve = 5,
	ESPCT_Exp1 = 6,
	ESPCT_ReciprocalOfSineCurve	= 7,
	ESPCT_Exp3 = 8,
};

struct STimedSoundEvent
{
	STimedSoundEvent()
		: duration(0.f)
		, lastEngineTime(0.f)
		, currentTime(0.f)
		, boneNum(-1)
		, updateTimeParameter(false)
		, startEventId(0)
	{

	}
	Uint64 startEventId;
	String onStopEvent;
	Float duration;
	Float currentTime;
	Bool updateTimeParameter;
	Int32 boneNum;
	EngineTime lastEngineTime;
};

struct SSoundSequenceCallbackData
{
	Red::Threads::AtomicOps::TAtomic32	m_duartion;
	Red::Threads::AtomicOps::TAtomic32	m_itemsCompleted;
	Red::Threads::AtomicOps::TAtomic32	m_eventCompleted;

	SSoundSequenceCallbackData()
		: m_duartion( 0 )
		, m_itemsCompleted( 0 )
		, m_eventCompleted( 0 )
	{
	}

	~SSoundSequenceCallbackData()
	{
		// for debug
		#ifndef RED_FINAL_BUILD
			m_eventCompleted = m_itemsCompleted = m_duartion = 0xbaadca11;	
		#endif
	}

	RED_INLINE Red::Threads::AtomicOps::TAtomic32 ReadDuration() const { return m_duartion; } 
	RED_INLINE Red::Threads::AtomicOps::TAtomic32 ReadItemsCompleted() const { return m_itemsCompleted; } 
	RED_INLINE Bool IsEventCompleted() const { return 0 != m_eventCompleted; } 
};

class CLightweightSoundEmitter
{
	friend class CSoundSystem;
	static TFastMuiltiStreamList< Uint64, Uint64, 0 > m_gameObjects;
	Uint64 m_gameObject;

	static void Process();

#ifndef RED_FINAL_BUILD
	static Red::Threads::CAtomic< Bool > m_swapping;
#endif

public:

	CLightweightSoundEmitter( const StringAnsi& objectName);
	CLightweightSoundEmitter( const Vector& position, const StringAnsi& objectName);


	~CLightweightSoundEmitter();

	CLightweightSoundEmitter& Event( const char* eventName );
	CLightweightSoundEmitter& Parameter( const char* name, Float value, Float duration = 0.0f, ESoundParameterCurveType curveType = ESPCT_Linear );
	CLightweightSoundEmitter& Switch( const char* name, const char* value );
	CLightweightSoundEmitter& Reverb( class ITriggerManager* triggerManager, const Vector& position );
#ifndef NO_EDITOR
	CLightweightSoundEmitter& ListenerSwitch(  Int32 listnerMask );
#endif
	CLightweightSoundEmitter& OcclusionParams( Float occlusion, Float obstruction);
};

class CSoundEmitterOneShot
{
	friend class CDebugPageSound;

public:
	typedef Uint32 TSoundSequenceID;

protected:
	Uint64 m_gameObject;
	Uint8 m_flags;

	class CSwitchOrParameterNameHashValue
	{
		Uint64 m_stateOrParameterNameHash;
		union 
		{
			struct 
			{
				Uint32 m_switchValue;
			};
			struct 
			{
				Float m_parameterValue;
			};
		};
		
	public:
		Bool operator == ( const CSwitchOrParameterNameHashValue& other ) const
		{
			return m_stateOrParameterNameHash == other.m_stateOrParameterNameHash;
		}
		CSwitchOrParameterNameHashValue( Uint32 nameHash, Uint32 switchValue ) : m_stateOrParameterNameHash( Uint64( nameHash ) | 0xFFFFFFFF00000000 ), m_switchValue( switchValue ) {}
		CSwitchOrParameterNameHashValue( Uint32 nameHash, Float parameterValue ) : m_stateOrParameterNameHash( nameHash ), m_parameterValue( parameterValue ) {}
		CSwitchOrParameterNameHashValue( Bool isSwitch, Uint32 nameHash ) : m_stateOrParameterNameHash( Uint64( nameHash ) | ( isSwitch ? 0xFFFFFFFF00000000 : 0 ) ){}

		bool isSwitch() const { return ( m_stateOrParameterNameHash & 0xFFFFFFFF00000000 ) == 0xFFFFFFFF00000000; }
		Uint32 getNameHash() const { return m_stateOrParameterNameHash & 0x00000000FFFFFFFF; }

		Uint32 getSwitchValue() const { return m_switchValue; }
		Float getParameterValue() const { return m_parameterValue; }

		void setSwitchValue( Uint32 value ) { m_switchValue = value; }
		void setParameterValue( Float value ) { m_parameterValue = value; }
	};

	TDynArray< CSwitchOrParameterNameHashValue > m_switchesAndParameters;

	virtual void Flush();

#ifndef RED_FINAL_BUILD
	virtual StringAnsi GetSoundObjectName() const = 0;
#endif


public:

	CSoundEmitterOneShot() : m_gameObject( 0 ), m_flags( 0 ) {}
	virtual ~CSoundEmitterOneShot() { Flush(); }


	Bool SoundFlagGet( ESoundEmitterFlags flag ) const;
	void SoundFlagSet( ESoundEmitterFlags flag, Bool decision );

	Float GetHighestTime();
	static Int32 GetHighestTime( Uint64 gameObjectid );

	Uint64 SoundEvent( const char* eventName );
	TSoundSequenceID PlaySoundSequence( const wchar_t* sequenceName, const wchar_t** sequenceElements, Uint32 sequenceElementsCount, SSoundSequenceCallbackData* callbackData = nullptr );
	void StopSoundSequence( TSoundSequenceID id ); 
	void SoundParameter( const char* name, Float value, Float duration = 0.0f, ESoundParameterCurveType curveType = ESPCT_Linear );
	Uint64 GetGameObjectId();
	void SoundSwitch( const char* name, const char* value );
		
	virtual void SoundPause();
	virtual void SoundResume();
	static void SoundState( const char* name, const char* value );
	static void SoundGlobalParameter( const char* name, Float value, Float duration = 0.0f, ESoundParameterCurveType curveType = ESPCT_Linear );

};


class CSoundEmitter : public CSoundEmitterOneShot
{
	friend class CSoundEmitterComponent;
	friend class CSoundSystem;
	friend class CDebugPageSound;

protected:
	struct SEmitterComponent
	{
		CSoundEmitter* m_emitter;
		THandle< CComponent> m_parentComponent;

		SEmitterComponent() : m_emitter( nullptr ), m_parentComponent( nullptr ) {}
		SEmitterComponent( void* ) : m_emitter( nullptr ), m_parentComponent( nullptr ) {}
		SEmitterComponent( CSoundEmitter* emitter, CComponent* parentComponent ) : m_emitter( emitter ), m_parentComponent( parentComponent ) {}

		Bool IsValid() { return m_parentComponent.Get() != nullptr; }
		Bool IsDefault() { return m_emitter == nullptr && m_parentComponent == nullptr; }

		Bool operator==( const SEmitterComponent &other ) const
		{
			return m_emitter == other.m_emitter && m_parentComponent == other.m_parentComponent;
		}
	};

	static TFastMuiltiStreamList< SEmitterComponent, void*, nullptr > m_playingEmitters;
	Red::Threads::CAtomic< Int32 > m_maxPlayedRef;

	TDynArray< CName > m_banksDependency;
	
	Bool m_banksAquired;
	Float m_maxDistance;
	Bool m_occlusionEnabled;
	Bool m_occlusionUpdateRequired;
	Float m_targetObstruction;
	Float m_targetOcclusion;
	Float m_currentObstruction;
	Float m_currentOcclusion;
	double m_lastProcessMarker;
	static double m_lastProcessingMarker;
	CPhysicsBatchQueryManager::SQueryId m_occlusionQueryId;

	CSoundEmitter() 
		: m_banksAquired( false )
		, m_maxDistance( 30.0f )
		, m_occlusionEnabled( true )
		, m_occlusionUpdateRequired( false )
		, m_targetObstruction( 0.0f )
		, m_targetOcclusion( 0.0f )
		, m_currentObstruction( 0.0f )
		, m_currentOcclusion( 0.0f )
		, m_lastProcessMarker( 0.0f )
	{
	} 
	virtual ~CSoundEmitter();

	static void ProcessPlayingEmitters( class CSoundListenerComponent* listener );
	Bool PendProcessing( CSoundEmitter* emitter, THandle< CComponent > component );

	void OnTick( Float timeDelta );

	void Acquire(  bool async = false );
	Bool LoadedFully();
	void UnAcquire();

	virtual void SetOcclusionParameters( Float obstruction, Float occlusion );
	virtual void UpdateOcclusionParameters();

	void PrepareOcclusionRaycast( CPhysicsBatchQueryManager* physicsBatchManager, const Vector& listenerPosition );
	void ProcessOcclusionRaycast( CPhysicsBatchQueryManager* physicsBatchManager );

public:
	Uint64 SoundEvent( const char* eventName );
	void SoundSeek( const char* eventName, float percent );

	// updated only for moving sources, see IAmbientSound
	virtual const Matrix GetSoundPlacementMatrix() const { return Matrix::IDENTITY; }

	virtual Bool SoundIsActive() const;

	virtual void SoundStopAll();
	virtual void SoundStop( Float duration = 0.0f );
	virtual void SoundStop( const char* eventName, Float duration = 0.0f );
	virtual void SoundStop( Uint64 handle, Float duration = 0.0f );

	virtual Vector GetOcclusionPosition() { return Vector::ZERO_3D_POINT; }

	static Uint32 GetIdFromName( const char* eventName );

#ifndef RED_FINAL_BUILD
	static StringAnsi m_soundNameToBreakOn;
#endif // !RED_FINAL_BUILD

};

class CSoundEmitterComponent;

class CAuxiliarySoundEmitter : public CSoundEmitterOneShot
{
	friend class CSoundEmitterComponent;

	Vector m_position;
	const CSoundEmitterComponent * m_component;
	Float m_speed;
	Float m_decelDistance;

public:

	CAuxiliarySoundEmitter(const CSoundEmitterComponent *component);

#ifndef RED_FINAL_BUILD
	virtual StringAnsi GetSoundObjectName() const;
#endif

	void SetSpeed(Float speed, Float decelDistance) 
	{ 
		m_speed = speed; 
		m_decelDistance = decelDistance;
	}

	void Update(Float timeDelta);
};

struct SSoundSwitch
{
	DECLARE_RTTI_STRUCT( SSoundSwitch );
	StringAnsi m_name;
	StringAnsi m_value;
};
BEGIN_CLASS_RTTI( SSoundSwitch );
	PROPERTY_CUSTOM_EDIT_NAME( m_name, TXT( "SoundSwitch" ), TXT( "" ), TXT( "AudioSwitchBrowser" ) )
	PROPERTY_EDIT( m_value, TXT( "Value" ) );
END_CLASS_RTTI();

struct SSoundProperty
{
	DECLARE_RTTI_STRUCT( SSoundProperty );
	StringAnsi m_name;
	Float m_value;
};
BEGIN_CLASS_RTTI( SSoundProperty );
	PROPERTY_CUSTOM_EDIT_NAME( m_name, TXT( "SoundProperty" ), TXT( "" ), TXT( "SoundGameParamterEditor" ) )
	PROPERTY_EDIT( m_value, TXT( "Value" ) );
END_CLASS_RTTI();

struct SSoundAnimHistoryInfo
{
	SSoundAnimHistoryInfo()
		:time(0)
		,bone(-1)
	{

	}
	StringAnsi eventName;
	Float time;
	Int64 bone;
};

class CSoundEmitterComponent : public CComponent, public CSoundEmitter
{
	DECLARE_ENGINE_CLASS( CSoundEmitterComponent, CComponent, 0 );

	friend class CSoundSystem;

	template < typename T >
	struct SProcessingContext
	{
		THandle< T > m_componentHandle;
		Float m_x;
		Float m_y;
		Float m_z;
		Float m_previousResultDistanceSquared;
		Float m_resultDistanceSquared;
		Float m_desiredDistanceSquared;

		void Clear()
		{
			 m_componentHandle = nullptr;
			 m_desiredDistanceSquared = FLT_MAX;
		}
		SProcessingContext() { Clear(); }
		SProcessingContext( T* component, const Vector& pos, float desiredDistance ) : m_componentHandle( component ) , m_x( pos.X ), m_y( pos.Y ), m_z( pos.Z ), m_previousResultDistanceSquared( FLT_MAX ), m_resultDistanceSquared( FLT_MAX ), m_desiredDistanceSquared( desiredDistance * desiredDistance ) {}

	};
	static TDynArray< SProcessingContext< CSoundEmitterComponent > > m_processingContextPool;
	Int32 m_processingContextPoolIndex;

	THashMap< Int32, Uint64 > m_gameObjects;
	THashMap< Int32, CAuxiliarySoundEmitter> m_auxiliaryEmitters;
	StringAnsi m_intensityBasedLoopStart;
	StringAnsi m_intensityBasedLoopStop;
	StringAnsi m_intensityParameter;

	enum EOnAttachProcessingStatus
	{
		EOAPS_NotAttached,
		EOAPS_Postponed,
		EOAPS_Processed
	};

	TDynArray< StringAnsi >		m_eventsOnAttach;
	TDynArray< StringAnsi >		m_eventsOnDetach;

	TDynArray< SSoundSwitch >	m_switchesOnAttach;
	TDynArray< SSoundProperty > m_rtpcsOnAttach;

	EOnAttachProcessingStatus m_onAttachProcessingStatus;

	SSoundAnimHistoryInfo m_previousFootstep;
	SSoundAnimHistoryInfo m_previousAnimEvent;

	struct SStringAnsiInt32
	{
		StringAnsi m_eventName;
		Int32 m_boneId;
		SStringAnsiInt32() : m_boneId( -1 ) {}
		SStringAnsiInt32( const StringAnsi& eventName, Int32 boneId ) : m_boneId( boneId ), m_eventName( eventName ) {}
	};
	TDynArray< SStringAnsiInt32 > m_eventsPosponedTilReady;

	TDynArray< STimedSoundEvent > m_timedSoundEvents;

	ITriggerActivator* m_activator;

	Bool m_isReady;

#ifndef NO_EDITOR
	Int32	m_listenerBitmask;
#endif

#ifndef NO_EDITOR_FRAGMENTS
	String m_debugBuffer;
	THandle< CBitmapTexture > m_icon;
#endif

	Bool m_isInGameMusic;	// Specifies if emitter is playing in-game music e.g. musicians
	static Float sm_inGameMusicListenerDistance; //Min distance from in-game music emitters to the listener
	
	Bool	m_updateAzimuth;

	String	m_listenerOverride;

protected:
	CSoundEmitterComponent();
	virtual ~CSoundEmitterComponent();

	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;

	virtual void UpdateOcclusionParameters();

	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;

	void OnMaxDistanceEntered();
	void OnMaxDistanceExited();
	void OnTick( SProcessingContext< CSoundEmitterComponent >* context, Float timeDelta );
	static void ProcessTick( Float timeDelta, const Vector& position );

	virtual Uint32 GetMinimumStreamingDistance() const;

#ifndef NO_EDITOR_FRAGMENTS
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags ) override;
#endif

private:
	Bool IsManualCreationAllowed() const { return true; }

	Bool IsIndividual() const { return true; }

	void ProcessOnAttach();
	void ProcessOnAttachEvents();
	void ProcessOnAttachSwitches();
	void ProcessOnAttachProperties();

public:
#ifndef RED_FINAL_BUILD
	virtual StringAnsi GetSoundObjectName() const;
#endif

	void NotifyFootstepEvent(StringAnsi eventName, Int64 bone);
	void NotifyAnimEvent(StringAnsi eventName, Int64 bone);

	const SSoundAnimHistoryInfo &GetPreviousFootstepEvent() { return m_previousFootstep; }
	const SSoundAnimHistoryInfo &GetPreviousAnimEvent() { return m_previousAnimEvent; }


	StringAnsi GetLoopStart() const { return m_intensityBasedLoopStart; }
	StringAnsi GetLoopStop() const { return m_intensityBasedLoopStop; }
	StringAnsi GetLoopIntensity() const { return m_intensityParameter; }
	Float GetMaxDistance() const { return m_maxDistance; }
	Float GetDistanceFromCameraSquared() const;

	virtual void Flush();

	//Finds and returns the gameobject id for the specified bone, if none is available for that bone the default id is returned
	Uint64 FindGameObjectIdForBone(Int32 boneNum);

	void TimedSoundEvent(String startEvent, String stopEvent, Float duration, Bool updateTimeParameter = false, Int32 boneNum = -1 );

	Uint64 SoundEvent( const char* eventName, Int32 boneNum = -1 );
	Uint64 SoundEvent( const char* eventName, void* data, Uint32 compresion, Uint32 size, float percent = -1.0f );
	Uint64 SoundEvent( const char* eventName, void* data, Uint32 compresion, Uint32 size, Int32 boneNum, float percent = -1.0f );

	Uint64 AuxiliarySoundEvent(const char * eventName, Float speed = -1.f, Float decelDist = 0.f, Int32 boneNum = -1);

	CAuxiliarySoundEmitter* GetAuxiliaryEmitter(Uint32 boneNum);

	void SetParameterOnAllObjects(const char* name, Float value, Float duration);
	void SoundParameter( const char* name, Float value, Float duration = 0.0f, Int32 boneNum = -1, ESoundParameterCurveType curveType = ESPCT_Linear );
	void SoundSwitch( const char* name, const char* value, Int32 boneNum = -1 );
	Bool CopySwitchFromRoot( const char* switchName, Int32 boneNum );
	Bool CopyPrameterFromRoot( const char* paramterName, Int32 boneNum );

	virtual const Matrix GetSoundPlacementMatrix() const { return GetLocalToWorld(); }

	virtual Bool SoundIsActive() const;
	Bool SoundIsActive( const char* eventName ) const;
	Bool SoundIsActive( Int32 boneNum ) const;
	Bool SoundIsActive( Uint64 handle, Int32 boneNum );

	virtual const Bool IsLoadedAndReady() { return m_isReady && LoadedFully();	}
	void SetOcclusionEnable( Bool enable );

	Uint32 GetSoundsActiveCount();

	void SetMaxDistance( Float maxDistance ){ m_maxDistance = maxDistance; }
	void SetSoundLoop( StringAnsi name ) { m_intensityBasedLoopStart = name; }

	void AddSoundBankDependency( CName soundBank );

#ifndef NO_EDITOR
	void ListenerSwitch( unsigned char listnerIndex );
	Int32 GetListenerMask() const { return m_listenerBitmask; }
#endif
	virtual void SoundPause();
	virtual void SoundResume();
	virtual void SoundStopAll();
	virtual void SoundStop( Float duration = 0.0f );
	virtual void SoundStop( const char* eventName, Float duration = 0.0f );
	virtual void SoundStop( Uint64 handle, Float duration = 0.0f );

	virtual void SoundSeek( const char* eventName, float percent );

	virtual void OnPropertyPostChange( IProperty* property );

	Vector GetOcclusionPosition();

	virtual Bool ShouldWriteToDisk() const override;

	Bool IsDefault() const;

#ifndef NO_EDITOR
	static Bool IsMaxDistanceReached( const Vector& emitterPosition, float maxDistance, Int32 listenerBitmask = 0 );
#else
	static Bool IsMaxDistanceReached( const Vector& emitterPosition, float maxDistance );
#endif
	static CSoundEmitterComponent* GetSoundEmitterIfMaxDistanceIsntReached( CEntity* entity, float maxDistance );

#ifndef NO_EDITOR
	virtual Bool CheckMassActionCondition( const Char* condition ) const override;
#endif

	Float GetCurrentOcclusion();
	Float GetCurrentObstruction();
};

BEGIN_CLASS_RTTI( CSoundEmitterComponent );
PARENT_CLASS( CComponent );
	PROPERTY_CUSTOM_EDIT_NAME( m_intensityBasedLoopStart, TXT( "loopStart" ),TXT( "" ), TXT( "AudioEventBrowser" ) )
	PROPERTY_CUSTOM_EDIT_NAME( m_intensityBasedLoopStop, TXT( "loopStop" ), TXT( "" ), TXT( "AudioEventBrowser" ) )
	PROPERTY_EDIT( m_intensityParameter, TXT( "" ) )
	PROPERTY_CUSTOM_EDIT_ARRAY( m_eventsOnAttach, TXT( "" ), TXT( "AudioEventBrowser" ) )
	PROPERTY_CUSTOM_EDIT_ARRAY( m_eventsOnDetach, TXT( "" ), TXT( "AudioEventBrowser" ) )
	PROPERTY_CUSTOM_EDIT_ARRAY( m_banksDependency, TXT( "" ), TXT( "SoundBankEditor" ) )
	PROPERTY_EDIT_ARRAY( m_switchesOnAttach, TXT( "Sound switches on attach" ) )
	PROPERTY_EDIT_ARRAY( m_rtpcsOnAttach, TXT( "Sound properties on attach" ) )
	PROPERTY_EDIT( m_maxDistance, TXT( "Maximum distance from which ambient will be played" ) )
	PROPERTY_EDIT( m_occlusionEnabled, TXT( "Occlusion" ) )
	PROPERTY_EDIT( m_isInGameMusic, TXT( "Emitter is playing in-game music e.g. musicians"));
	PROPERTY_EDIT( m_listenerOverride, TXT("Add a name to make this emitter available as a listener override"));
	PROPERTY_EDIT( m_updateAzimuth, TXT("Set the angle between this emitter and the listener"));
END_CLASS_RTTI();

