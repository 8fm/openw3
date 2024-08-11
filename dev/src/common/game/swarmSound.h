#pragma once

#include "swarmUtils.h"
#include "definitionsManager.h"

class CBaseCritterAI;
class CSwarmAlgorithmData;
class CBoidInstance;
class CBoidLairParams;
class CFlyingSwarmGroup;
class CSwarmSoundConfig;
class CBoidSpecies;

/// Base class for sound filters
class CBaseSwarmSoundFilter
{	
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Gameplay );

public :
	CBaseSwarmSoundFilter( );
	virtual ~CBaseSwarmSoundFilter(){}

	virtual Bool FilterLair( const CSwarmAlgorithmData & swarmAlgoData )const = 0;
	virtual Bool FilterBoid( const CBaseCritterAI & baseAI )const = 0;
	virtual Bool FilterFlyingGroup( const CFlyingSwarmGroup & group )const = 0;

	virtual Bool ParseXml( const SCustomNode & parentNode, const CBoidLairParams *const boidLairParams, CSwarmSoundConfig *const soundConfig ) = 0;
};
typedef TDynArray< const CBaseSwarmSoundFilter *> CBaseSwarmSoundFilter_CPointerArray;

/// "Or" filter, if any of its children passes then the "Or" filter passes
class COrSwarmSoundFilter : public CBaseSwarmSoundFilter
{
public :
	COrSwarmSoundFilter();
	virtual ~COrSwarmSoundFilter();

	Bool FilterLair( const CSwarmAlgorithmData & swarmAlgoData )const override;
	Bool FilterBoid( const CBaseCritterAI & baseAI )const override;
	Bool FilterFlyingGroup( const CFlyingSwarmGroup & group )const override;

	Bool ParseXml( const SCustomNode & parentNode, const CBoidLairParams *const boidLairParams, CSwarmSoundConfig *const soundConfig )override;

private:
	CBaseSwarmSoundFilter_CPointerArray	m_childrenArray;
};

/// "And" filter, all of its children has to pass in order that the "Or" filter passes
class CAndSwarmSoundFilter : public CBaseSwarmSoundFilter
{
public :
	CAndSwarmSoundFilter();
	virtual ~CAndSwarmSoundFilter();

	Bool FilterLair( const CSwarmAlgorithmData & swarmAlgoData )const override;
	Bool FilterBoid( const CBaseCritterAI & baseAI )const override;
	Bool FilterFlyingGroup( const CFlyingSwarmGroup & group )const override;

	Bool ParseXml( const SCustomNode & parentNode, const CBoidLairParams *const boidLairParams, CSwarmSoundConfig *const soundConfig )override;
private:
	CBaseSwarmSoundFilter_CPointerArray	m_childrenArray;
};

/// This filter uses the boid state from the critter ai to do its thing
class CBoidStateSwarmSoundFilter : public CBaseSwarmSoundFilter
{
public :
	CBoidStateSwarmSoundFilter();

	Bool FilterLair( const CSwarmAlgorithmData & swarmAlgoData )const override;
	Bool FilterBoid( const CBaseCritterAI & baseAI )const override;
	Bool FilterFlyingGroup( const CFlyingSwarmGroup & group )const override;

	Bool ParseXml( const SCustomNode & parentNode, const CBoidLairParams *const boidLairParams, CSwarmSoundConfig *const soundConfig );
private:
	EBoidState			m_boidState;
};

/// This filter uses wether or not the boid is in a boid effector or not
class CPoiSwarmSoundFilter : public CBaseSwarmSoundFilter
{
public :
	CPoiSwarmSoundFilter();

	Bool FilterLair( const CSwarmAlgorithmData & swarmAlgoData )const override;
	Bool FilterBoid( const CBaseCritterAI & baseAI )const override;
	Bool FilterFlyingGroup( const CFlyingSwarmGroup & group )const override;

	Bool ParseXml( const SCustomNode & parentNode, const CBoidLairParams *const boidLairParams, CSwarmSoundConfig *const soundConfig );
private:
	CName			m_poiType;
};

/// Sound config found in the XML file
class CSwarmSoundConfig
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );

public :
	CSwarmSoundConfig();
	virtual ~CSwarmSoundConfig();
	StringAnsi				m_eventStart;
	StringAnsi				m_eventStop;
	StringAnsi				m_countParameter;
	StringAnsi				m_distanceParameter;
	StringAnsi				m_radiusParameter;
	StringAnsi				m_yawParameter;
	StringAnsi				m_pitchParameter;
	CBaseSwarmSoundFilter *	m_rootFilter;

	Bool FilterLair( const CSwarmAlgorithmData & swarmAlgoData )const;
	Bool FilterBoid( const CBaseCritterAI & baseAI )const;

	Bool ParseXml( const SCustomNode & parentNode, const CBoidLairParams *const boidLairParams, CBoidSpecies *const boidSpecies );

	Bool operator==( const CSwarmSoundConfig & soundConfig )const;

	CBaseSwarmSoundFilter*const CreateFilterFromXml( const SCustomNode & parentNode, const CBoidLairParams *const boidLairParams );

	virtual CBaseSwarmSoundFilter*const CreateXmlFromXmlAtt( const SCustomNodeAttribute & att );
};


/// used in the during the update to avoid racing conditions
class CSwarmSoundJobData
{
public:
	CSwarmSoundJobData( Uint32 soundIndex )
		: m_soundIndex	( soundIndex )
		, m_center( 0.0f, 0.0f, 0.0f )
		, m_intensity( 0.0f )
		, m_radius( 0.0f )
		, m_distance( 0.0f )
		, m_yaw( 0.0f )
		, m_pitch( 0.0f ){}

	// Sound index in the sound config array and other arrays
	Uint32	m_soundIndex;
	
	Vector	m_center;
	Float	m_intensity;
	Float	m_radius;
	Float	m_distance;
	Float	m_yaw;
	Float	m_pitch;
};
typedef TDynArray< CSwarmSoundJobData > CSwarmSoundJobData_Array;
class CSwarmSoundJobDataCollection
{
public:
	CSwarmSoundJobData_Array	m_jobDataArray;
	// Id to the actual sound data collection situated in the lair
	Uint32						m_soundCollectionId;

	CSwarmSoundJobDataCollection( const CBoidLairParams & params );
};

typedef TDynArray< CSwarmSoundJobDataCollection > CSwarmSoundJobDataCollection_Array;

/// Synched from CSwarmSoundJobData, used in pre and post update 
class CBoidSound
{
	friend class CBoidSoundsCollection;
private:
	StringAnsi				m_eventStart;
	StringAnsi				m_eventStop;
	StringAnsi				m_countParameter;
	StringAnsi				m_distanceParameter;
	StringAnsi				m_radiusParameter;
	StringAnsi				m_yawParameter;
	StringAnsi				m_pitchParameter;
protected:
	Float					m_boidsCount;
	Vector					m_position;
	Float					m_radius;
	Float					m_distance;
	Float					m_yaw;
	Float					m_pitch;
private:
	Bool					m_isRunning;
public:
	CBoidSound( const StringAnsi& eventStart, const StringAnsi& eventStop, const StringAnsi& countParameter, const StringAnsi& distanceParameter, const StringAnsi& radiusParameter = StringAnsi::EMPTY, const StringAnsi& yawParameter = StringAnsi::EMPTY, const StringAnsi& pitchParameter = StringAnsi::EMPTY )
		: m_eventStart( eventStart )
		, m_eventStop( eventStop )
		, m_countParameter( countParameter )
		, m_distanceParameter( distanceParameter )
		, m_radiusParameter( radiusParameter )
		, m_yawParameter( yawParameter )
		, m_pitchParameter( pitchParameter )
		, m_boidsCount( 0.0f )
		, m_position( 0.0f, 0.0f, 0.0f )
		, m_radius( 0.0f )
		, m_distance( 0.0f )
		, m_yaw( 0.0f )
		, m_pitch( 0.0f )
		, m_isRunning( false )	{}
	virtual ~CBoidSound()		{}
	void Update( const Vector& cameraPos, const CSwarmSoundJobData & soundJobData );
	void GetLocalToWorld( Matrix& m ) const;

	void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );
};
class CBoidSoundsCollection : public Red::System::NonCopyable
{
protected:
	TDynArray< CBoidSound* >				m_runningSounds;
	THandle< CSoundEmitterComponent >		m_soundComponent;
	// Unique Id to find the sound collection during post update
	Uint32								m_Id;

	static Uint32						s_nextId;
public:
	CBoidSoundsCollection( CSoundEmitterComponent*const soundComponent, const CBoidLairParams & params );
	~CBoidSoundsCollection();

	void								AddSound( CBoidSound* sound )				{ m_runningSounds.PushBack( sound ); }
	CBoidSound*							GetSound( Uint32 index ) const				{ return index < m_runningSounds.Size() ? m_runningSounds[ index ] : NULL; }
	const TDynArray< CBoidSound* > &	GetRunningSoundArray()const					{ return m_runningSounds; }

	void				Update( const Vector& cameraPos, const CSwarmSoundJobDataCollection *const soundJobDataCollection );
	void				FadeOutAll();
	const Uint32 &		GetId()const { return m_Id; }
};

typedef TDynArray< CBoidSoundsCollection * > CBoidSoundsCollection_PointerArray;
