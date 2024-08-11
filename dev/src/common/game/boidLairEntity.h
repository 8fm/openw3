#pragma once

#include "boidPointOfInterestComponent.h"
#include "swarmSound.h"
#include "boidSpecies.h"
#include "swarmUtils.h"
#include "boidAreaComponent.h"
#include "boidNodeData.h"
#include "pointOfInterestSpeciesConfig.h"
#include "../engine/playedAnimation.h"
#include "../engine/entityHandle.h"
#include "gameplayEntity.h"

class CBoidInstance;
class CBoidLairParams;
class CBaseBoidNode;
class CSwarmRenderComponent;
class CSwarmSoundEmitterComponent;

///////////////////////////////////////////////////

struct CPoiItem
{
	CPoiItem( CBoidPointOfInterestComponent* item = NULL, Boids::PointOfInterestId uid = (Uint32)-1 )
		: m_item( item )
		, m_uid( uid )												{}

	THandle< CBoidPointOfInterestComponent >			m_item;
	Boids::PointOfInterestId							m_uid;
};

typedef TDynArray< CPoiItem >									CPoiItem_Array;
typedef TArrayMap< Boids::PointOfInterestType, CPoiItem_Array >	TPointsMap;



/////////////////////////////////////////////////////////////////////////
class IBoidLairEntity : public CGameplayEntity, IPlayedAnimationListener
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IBoidLairEntity, CGameplayEntity);

public:
	/////////////////////////////////////////////////////////////////////
	// filters for dynamic points of interest
	struct CDynamicPointsAcceptor
	{
		CDynamicPointsAcceptor( CName filterTag )
			: m_filterTag ( filterTag )
		{}
		CName m_filterTag;

		const CName& GetFilterTag() const { return m_filterTag; }
		virtual Bool AcceptEntity( CEntity* entity ) = 0;
		virtual Bool IsGlobal() const;
		virtual ~CDynamicPointsAcceptor();
	};
	struct CGlobalDynamicPointsAcceptor : public CDynamicPointsAcceptor
	{
		CGlobalDynamicPointsAcceptor( CName filterTag )
			: CDynamicPointsAcceptor( filterTag )
		{}
		Bool IsGlobal() const override;
	};

	struct CPlayerDynamicPointsAcceptor : CGlobalDynamicPointsAcceptor
	{
		CPlayerDynamicPointsAcceptor( CName filterTag = CNAME( Player ) )
			: CGlobalDynamicPointsAcceptor( filterTag )
		{}
		Bool AcceptEntity( CEntity* entity );

		static CPlayerDynamicPointsAcceptor* GetInstance();
	};

	struct CActorDynamicPointsAcceptor : CGlobalDynamicPointsAcceptor
	{
		CActorDynamicPointsAcceptor( CName filterTag = CNAME( ActorFilter ) )
			: CGlobalDynamicPointsAcceptor( filterTag )
		{}
		Bool AcceptEntity( CEntity* entity );

		static CActorDynamicPointsAcceptor* GetInstance();
	};

	struct CTagDynamicPointsAcceptor : CDynamicPointsAcceptor
	{
		CTagDynamicPointsAcceptor( CName tag, CName filterTag )
			: CDynamicPointsAcceptor( filterTag)
			, m_tag( tag )												{}
		Bool AcceptEntity( CEntity* entity );

		CName						m_tag;
	};

	struct CComplexDynamicPointsAcceptor : CDynamicPointsAcceptor
	{
		CComplexDynamicPointsAcceptor( CDynamicPointsAcceptor* a1, CDynamicPointsAcceptor* a2, CName filterTag )
			: CDynamicPointsAcceptor( filterTag ) 
			, m_a1( a1 )
			, m_a2( a2 )												{}
		~CComplexDynamicPointsAcceptor();

		CDynamicPointsAcceptor*		m_a1;
		CDynamicPointsAcceptor*		m_a2;
	};

	struct CANDDynamicPointsAcceptor : CComplexDynamicPointsAcceptor
	{
		CANDDynamicPointsAcceptor( CDynamicPointsAcceptor* a1, CDynamicPointsAcceptor* a2, CName filterTag )
			: CComplexDynamicPointsAcceptor( a1, a2, filterTag )					{}

		Bool AcceptEntity( CEntity* entity );
	};

	struct CORDynamicPointsAcceptor : CComplexDynamicPointsAcceptor
	{
		CORDynamicPointsAcceptor( CDynamicPointsAcceptor* a1, CDynamicPointsAcceptor* a2, CName filterTag )
			: CComplexDynamicPointsAcceptor( a1, a2, filterTag )					{}

		Bool AcceptEntity( CEntity* entity );
	};
protected:
	CName								m_boidSpeciesName; 
	const CBoidLairParams *				m_params;				// Note: Initialized in derived classes CHumbleCrittersLairEntity and CFlyingCrittersLairEntity
	/// Time since entity has been attached to world
	Float								m_time;
	CLoopedAnimPriorityQueue			m_loopedAnimPriorityQueue;

	Bool								m_isActivated;
	Bool								m_isVisible;

	EngineTime							m_visibilityUpdateDelay;
	Float								m_spawnFrequency;
	Float								m_range;
	Float								m_visibilityRange;
	Int32								m_spawnLimit;
	Int32								m_totalLifetimeSpawnLimit;
	Int32								m_activeBoids;
	Int32								m_maxActiveBoidIndex;
	Boids::PointOfInterestId			m_nextPointOfInterestId;
	EntityHandle						m_lairBoundings;

	TDynArray< THandle< CEntity > >				m_triggers;
	CPoiItem_Array								m_spawnPoints;
	TPointsMap									m_staticPointsOfInterest;
	TDynArray< CDynamicPointsAcceptor* >		m_dynamicPointTypes;

	
	Box									m_boundingBox;

	TDynArray< CBoidInstance* >			m_boidInstances;
	CBoidSoundsCollection_PointerArray	m_soundsCollectionArray;

	THandle< CSwarmRenderComponent >	m_swarmRenderComponent;
	CSwarmSoundEmitterComponent*		m_swarmSoundComponent;

	Bool								m_fullyInitialized;

public:

	IBoidLairEntity();
	virtual ~IBoidLairEntity();

	void SetupFromTool( CEntity *const boidArea, CName speciesName = CName::NONE );

	virtual void OnTimer( const CName name, Uint32 id, Float timeDelta ) override;

	// Update functions
	void UpdateTime(Float timeDelta);
	void UpdateAnimation();
	void UpdateSwarmBoundingBox();

	// Script :
	virtual void NoticeFireInCone( const Vector& position, const Vector2& coneDir, Float coneHalfAngle, Float coneRange );
	virtual void OnBoidPointOfInterestReached( Uint32 count, CEntity *const entity, Float deltaTime ){}
	
	void OnStaticPointOfInterestAdded( CBoidPointOfInterestComponent* poi );
	void OnStaticPointOfInterestRemoved( CBoidPointOfInterestComponent* poi );

	Bool OnDynamicComponentEntered( CComponent* poi );
	void OnDynamicComponentLeft( CComponent* poi );

	virtual CBoidInstance* SpawnInstance( Uint32 index, const Vector& position, const EulerAngles& orientation );

	virtual void OnInitialized();
	virtual void OnUninitialized();
	void OnAttached( CWorld* world ) override;
	void OnDetached( CWorld* world ) override;
	Bool IsGameplayLODable()override{ return false;}

	virtual void DelayedInitialize();		// Called only inside LazyInitialize, but virtual so derived classes can use it
	void LazyInitialize();

	void OnActivationTriggerAdded( CEntity *const activationtrigger );
	void OnActivationTriggerRemoved( CEntity *const activationtrigger );

	Bool ContainsPoint( const Vector& location, Bool testBothArea );

	void OnBoundingBoxUpdated()																{}
	const Box& GetBoundingBox() const														{ return m_boundingBox; }

	Int32 GetBoidsCountLimit() const														{ return m_spawnLimit; }
	Int32 GetTotalLifetimeSpawnLimit() const												{ return m_totalLifetimeSpawnLimit; }
	Float GetSpawnFrequency() const															{ return m_spawnFrequency; }

	const TPointsMap&				GetStaticPointsOfInterest() const						{ return m_staticPointsOfInterest; }
	const TDynArray< CDynamicPointsAcceptor* > GetDynamicPointsOfInterestTypes() const		{ return m_dynamicPointTypes; }
	const CBoidLairParams *const	GetBoidLairParams()const								{ return m_params;}
	CAreaComponent*					GetLairBoundings() const;
	Uint32							GetSoundCollectionIndexFromId( Uint32 id )const;
	CSkeleton*						GetSkeleton() const;
	

	void OnSerialize( IFile& file ) override;

	void AddActiveBoid()																	{ ++m_activeBoids; ASSERT( m_activeBoids <= m_spawnLimit ); }
	void RemoveActiveBoid();

	CBoidInstance* GetBoidInstance( Uint32 i ) const										{ return m_boidInstances[ i ]; }

	Bool GetSubObjectWorldMatrix( Uint32 index, Matrix& matrix ) const override;
	Int32 GetSoundSubObjectCount() const override { return NumericLimits<Int32>::Max(); }
	CBoidSoundsCollection_PointerArray& GetSoundCollectionArray( )							{ return m_soundsCollectionArray; }

	// CGamePlayEntity virtual functions
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );
protected:
	virtual Bool ActivateLair();
	virtual void DeactivateLair();

	void HandleStaticPointOfInterestNoticed( CBoidPointOfInterestComponent* poi );

	void AddStaticPointOfInterestType( Boids::PointOfInterestType poiType );
	void AddDynamicPointOfInterestAcceptor( CDynamicPointsAcceptor* poiAcceptor );
	virtual void DeterminePointsOfInterestTypes();

	virtual void CollectStaticPointOfInterest( CBoidPointOfInterestComponent* poi, Boids::PointOfInterestId id );
	virtual void RemoveStaticPointOfInterest( CBoidPointOfInterestComponent* poi, Boids::PointOfInterestId id );

	virtual void CollectDynamicPointOfInterest( CEntity* entity, const CName& filterTag );
	virtual void RemoveDynamicPointOfInterest( CEntity* entity );

	virtual void UpdateLogic() {};

private:
	void ComputeStaticPointsOfInterest();

};

BEGIN_ABSTRACT_CLASS_RTTI( IBoidLairEntity )
	PARENT_CLASS( CGameplayEntity );
	PROPERTY_EDIT( m_boidSpeciesName, TXT("Name of the species you want to use in the boid_species.xml file"))
	PROPERTY_EDIT( m_spawnFrequency, TXT("Maximum spawns per second") )
	PROPERTY_EDIT( m_spawnLimit, TXT("Maximum number of boids alive at one time") )
	PROPERTY_EDIT( m_totalLifetimeSpawnLimit, TXT("Maximum number of boids that could be spawned from this lair, ever.") )
	PROPERTY_EDIT( m_lairBoundings, TXT("Boid area boundings component") )
	PROPERTY_EDIT( m_range, TXT("Activation range") )
	PROPERTY_EDIT( m_visibilityRange, TXT("Visibility range") )
END_CLASS_RTTI();


/////////////////////////////////////////////////////////////////////////
// CSwarmSoundEmitterComponent
class CSwarmSoundEmitterComponent : public CSoundEmitterComponent
{
	DECLARE_ENGINE_CLASS( CSwarmSoundEmitterComponent, CSoundEmitterComponent, 0 );
public :
	CSwarmSoundEmitterComponent();
	~CSwarmSoundEmitterComponent();

	void OnLairActivated();
	void OnLairDeactivated();

private :
	CName m_soundBank;
};

BEGIN_CLASS_RTTI( CSwarmSoundEmitterComponent );
	PARENT_CLASS( CSoundEmitterComponent );
END_CLASS_RTTI();


class CBoidState 
{
public:
	CBoidState( const CBaseBoidNode * rootBoidNode = NULL, CName Name = CName::NONE, Bool doubleSpeed = false, Bool startRandom = false )
		: m_rootBoidNode( rootBoidNode )
		, m_name( Name )
		, m_doubleSpeed( doubleSpeed )
		, m_startRandom( startRandom )
	{
	}

	const CBaseBoidNode *			m_rootBoidNode;
	CName							m_name;
	Bool							m_doubleSpeed;
	Bool							m_startRandom;
};
typedef TDynArray< CBoidState > CBoidState_Array;

class CBoidLairParams
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Boids );

private:
	Bool								m_isValid;

public:
	CName								m_name;		// name of species specified in xml
	Uint32								m_type;		// RTTI type used for casting
	CName								m_typeName; // Type as a CName used for xml parsing

	TDynArray<CName>					m_spawnPointArray;
	Float								m_boidScaleMin;
	Float								m_boidScaleMax;
	TSoftHandle< CEntityTemplate >		m_boidTemplateHandle;
	Bool								m_allowAnimationInteruption;

	CName								m_soundBank;

	CBoidState_Array					m_boidStateArray;
	CPointOfInterestSpeciesConfig_Map	m_poiSpeciesConfigMap;

	CSwarmSoundConfig_CPointerArray		m_soundConfigArray;

	CBoidLairParams( Uint32 type = TYPE_BOID_LAIR_PARAMS, CName typeName = CNAME( BoidLair ), Bool isValid = false );
	virtual ~CBoidLairParams();

	// Copies one params into another by checking type compatibility
	Bool								CopyTo( CBoidLairParams* const params )const;
	virtual Bool						ParseXmlAttribute( const SCustomNodeAttribute & att );
	virtual Bool						ParseXmlNode( const SCustomNode & node, CBoidSpecies *const boidSpecies );
	virtual Bool						ParseXML( const SCustomNode & paramsNode, CBoidSpecies *const boidSpecies);
	virtual Bool						OnParsePoiConfig( const SCustomNode & node, CBoidSpecies *const boidSpecies );
	virtual CSwarmSoundConfig *const	CreateSoundConfig();

	Bool										IsValid() const{return m_isValid;}
	const CAtomicBoidNode *const				GetFirstAtomicNodeFromState( Boids::BoidState state )const;	
	const CPointOfInterestSpeciesConfig *const	GetPOISpeciesConfigFromType(CName poiType)const;
	Uint32										GetBoidStateIndexFromName( CName name )const;

private:
	// Helper functions :
	Bool			ParseBoidStateXML( const SCustomNode & boidStateNode, CBoidSpecies *const boidSpecies );
	virtual Bool	VirtualCopyTo( CBoidLairParams* const params )const;
public :
	enum ETypes
	{
		TYPE_BOID_LAIR_PARAMS			= FLAG( 0 ),
	};
	enum 
	{ 
		E_TYPE = TYPE_BOID_LAIR_PARAMS, 
	};

	template < class TClass >
	const TClass* As()const
	{
		if ( (m_type & TClass::E_TYPE) == TClass::E_TYPE )
		{
			return static_cast<const TClass*>(this);
		}
		return NULL;
	}

	template < class TClass >
	TClass* As()
	{
		if ( (m_type & TClass::E_TYPE) == TClass::E_TYPE )
		{
			return static_cast<TClass*>(this);
		}
		return NULL;
	}
};



