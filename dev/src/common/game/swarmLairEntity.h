#pragma once

#include "boidLairEntity.h"

class CSwarmUpdateJob;
class CSwarmAlgorithmData;

struct SSwarmMemberStateData
{
	Int32				m_boidState;
	Float				m_boidStateUpdateRatio;
	Vector				m_position;
	EulerAngles			m_orientation;
	Uint32				m_flags;
};

struct SDelayedPoiRequest
{
	//Bool							m_dynamic;
	Bool							m_insert;
	THandle< CEntity >				m_entity;
	CBoidPointOfInterestComponent *	m_poiCpnt;
	CName							m_filterTag;
	Boids::PointOfInterestId		m_id;
};

typedef SSwarmMemberStateData* TSwarmStatesList;

///////////////////////////////////////////////////////////////
// Lair entity
class CSwarmLairEntity : public IBoidLairEntity
{
	typedef IBoidLairEntity Super;
	DECLARE_ENGINE_ABSTRACT_CLASS( CSwarmLairEntity, IBoidLairEntity );
public:
	
protected:
	static Uint32 const MEMBER_STATES_LISTS = 3;

	EngineTime					m_lastUpdate;
	EngineTime					m_nextUpdateTime;
	TSwarmStatesList			m_memberLists[MEMBER_STATES_LISTS];
	Uint32						m_currentStateIndex;
	volatile Bool				m_forceDataSync;
	volatile Bool				m_isProcessing;
	volatile Bool				m_requestOutOfTime;
	volatile Bool				m_requestShouldTerminate;

	Bool						m_isDefeated;

	volatile CSwarmUpdateJob*		m_job;
	volatile CSwarmAlgorithmData*	m_swarmAlgorithmData;

	TDynArray< SDelayedPoiRequest > m_delayedPois;

	String							m_defeatedStateFact;
	Int32							m_defeatedStateFactValue;

	Uint32							m_dynamicPoiCount;

	Bool							m_lairDisabledFromScript;
	Bool							m_lairDisabledAtStartup;

	// IBoidLairEntity virtual functions
	void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )override;
	void GenerateSwarmVisualDebug( CRenderFrame* frame, EShowFlags flag );
	
	virtual void OnTimer( const CName name, Uint32 id, Float timeDelta ) override;

	void IssueNewUpdateJob(const EngineTime& currentTime);
	void InterpolateBoidStates();

	void DeterminePointsOfInterestTypes()override;
	void OnAttachFinished( CWorld* world ) override;

	virtual void SynchroniseData();
	
	virtual CSwarmUpdateJob* NewUpdateJob() = 0;
	virtual CSwarmAlgorithmData* NewAlgorithmData() = 0;

	void CollectStaticPointOfInterest( CBoidPointOfInterestComponent* poi, Boids::PointOfInterestId id ) override;
	void RemoveStaticPointOfInterest( CBoidPointOfInterestComponent* poi, Boids::PointOfInterestId id )override;
	void AddStaticPoiToData( CSwarmAlgorithmData* data, CBoidPointOfInterestComponent *const poi, Boids::PointOfInterestId id );
	void RemoveStaticPoiFromData( CSwarmAlgorithmData* data, CBoidPointOfInterestComponent *const poi, Uint32 id );

	void CollectDynamicPointOfInterest( CEntity* entity, const CName& filterTag ) override;
	void RemoveDynamicPointOfInterest( CEntity* entity ) override;
	void AddDynamicPoiToData( CSwarmAlgorithmData* data, CEntity* entity, const CName& filterTag );
	void RemoveDynamicPoiFromData( CSwarmAlgorithmData* data, CEntity* entity );

	void InterpolateWithBezierCurve( Vector* output, const Float progress, const Vector& positionA, const Vector& controlPointA, const Vector& positionB, const Vector& controlPointB ) const;

public:
	CSwarmLairEntity();
	virtual ~CSwarmLairEntity();

	void Initialize();

	Bool ActivateLair() override;
	void DeactivateLair() override;
	void OnDefeated();

	// job interface
	void OnJobFinished( CSwarmUpdateJob* job ) volatile;
	Bool IsJobOutOfTime() volatile									{ return m_requestOutOfTime; }
	Bool IsJobTerminationRequest() volatile							{ return m_requestShouldTerminate; }
	volatile CSwarmAlgorithmData* GetAlgorithmData() const volatile	{ return m_swarmAlgorithmData; }

	TSwarmStatesList GetPreviousStateList()						{ return m_memberLists[ m_currentStateIndex == 0 ? MEMBER_STATES_LISTS-1 : m_currentStateIndex-1 ]; }
	TSwarmStatesList GetCurrentStateList()						{ return m_memberLists[ m_currentStateIndex ]; }
	TSwarmStatesList GetTargetStateList()						{ return m_memberLists[ m_currentStateIndex == MEMBER_STATES_LISTS-1 ? 0 : m_currentStateIndex+1 ]; }

	Bool GetLairDisabledFromScript()const{ return m_lairDisabledFromScript; }

	void funcDisable( CScriptStackFrame& stack, void* result );	
};

BEGIN_ABSTRACT_CLASS_RTTI( CSwarmLairEntity )
	PARENT_CLASS( IBoidLairEntity );
	PROPERTY_EDIT( m_defeatedStateFact, TXT("Fact added to database when swarm is defeated.") );
	PROPERTY_EDIT( m_defeatedStateFactValue, TXT("Fact value added to database when swarm is defeated.") );
	PROPERTY_EDIT( m_lairDisabledAtStartup, TXT("The lair start disabled and will spawn nothing until it is re-enabled through scripts, also once the lair completely despawns this behaviour is re-enabled") );
	NATIVE_FUNCTION( "Disable", funcDisable );
END_CLASS_RTTI();

class CSwarmLairParams : public CBoidLairParams
{
public:
	Float						m_updateTime;
	Float						m_maxSoundDistance;
	CSwarmLairParams(Uint32 type = TYPE_SWARM_LAIR_PARAMS, CName typeName = CNAME( SwarmLair ), Bool isValid = false)
		: CBoidLairParams( type, typeName, isValid )
		, m_updateTime( 0.25f )
		, m_maxSoundDistance( 500.0f )
	{

	}

	virtual Bool ParseXmlAttribute(const SCustomNodeAttribute & att);

	enum ETypes
	{
		TYPE_SWARM_LAIR_PARAMS	= TYPE_BOID_LAIR_PARAMS | FLAG( 1 ),
	};
	enum 
	{ 
		E_TYPE = TYPE_SWARM_LAIR_PARAMS, 
	};
private:
	Bool VirtualCopyTo(CBoidLairParams* const params)const override;
};







