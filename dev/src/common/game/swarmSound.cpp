#include "build.h"
#include "swarmSound.h"
#include "baseCrittersAI.h"
#include "boidInstance.h"
#include "../core/feedback.h"
#include "../engine/renderFrame.h"

////////////////////////////////////////////////////////////
// CBaseSwarmSoundFilter
CBaseSwarmSoundFilter::CBaseSwarmSoundFilter( )
{
}

////////////////////////////////////////////////////////////
// COrSwarmSoundFilter
COrSwarmSoundFilter::COrSwarmSoundFilter() 
	: CBaseSwarmSoundFilter( )
	, m_childrenArray( )
{
}

COrSwarmSoundFilter::~COrSwarmSoundFilter()
{
	for ( Uint32 i = 0; i < m_childrenArray.Size(); ++i )
	{
		delete m_childrenArray[ i ];
	}
}

Bool COrSwarmSoundFilter::FilterLair( const CSwarmAlgorithmData & swarmAlgoData )const
{
	for ( Uint32 i = 0; i < m_childrenArray.Size(); ++i )
	{
		const CBaseSwarmSoundFilter *const filter = m_childrenArray[ i ];
		if ( filter->FilterLair( swarmAlgoData ) )
		{
			return true;
		}
	}
	return false;
}
Bool COrSwarmSoundFilter::FilterBoid( const CBaseCritterAI & baseAI )const
{
	for ( Uint32 i = 0; i < m_childrenArray.Size(); ++i )
	{
		const CBaseSwarmSoundFilter *const filter = m_childrenArray[ i ];
		if ( filter->FilterBoid( baseAI ) )
		{
			return true;
		}
	}
	return false;
}

Bool COrSwarmSoundFilter::FilterFlyingGroup( const CFlyingSwarmGroup & group )const
{
	for ( Uint32 i = 0; i < m_childrenArray.Size(); ++i )
	{
		const CBaseSwarmSoundFilter *const filter = m_childrenArray[ i ];
		if ( filter->FilterFlyingGroup( group ) )
		{
			return true;
		}
	}
	return false;
}


Bool COrSwarmSoundFilter::ParseXml( const SCustomNode & parentNode, const CBoidLairParams *const boidLairParams, CSwarmSoundConfig *const soundConfig )
{
	const TDynArray< SCustomNode >::const_iterator end	= parentNode.m_subNodes.End();
	TDynArray< SCustomNode >::const_iterator  it;

	for(  it = parentNode.m_subNodes.Begin();  it != end; ++it )
	{
		const SCustomNode & node = *it;	
		if ( node.m_nodeName == CNAME( filter ) )
		{
			CBaseSwarmSoundFilter *const filter = soundConfig->CreateFilterFromXml( node, boidLairParams );
			if ( filter == NULL )
			{
				return false;
			}
			m_childrenArray.PushBack( filter );
		}
		else
		{
			GFeedback->ShowError( TXT("Boid XML Error: sound config unrekognized node: %s"), node.m_nodeName.AsString().AsChar() );
			return false;
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////
// CAndFilterSoundFilter
CAndSwarmSoundFilter::CAndSwarmSoundFilter()
	: CBaseSwarmSoundFilter( )
{
}

CAndSwarmSoundFilter::~CAndSwarmSoundFilter()
{
	for ( Uint32 i = 0; i < m_childrenArray.Size(); ++i )
	{
		delete m_childrenArray[ i ];
	}
}

Bool CAndSwarmSoundFilter::FilterLair( const CSwarmAlgorithmData & swarmAlgoData )const
{
	Bool pass = true;
	for ( Uint32 i = 0; i < m_childrenArray.Size(); ++i )
	{
		const CBaseSwarmSoundFilter *const filter = m_childrenArray[ i ];
		if ( filter->FilterLair( swarmAlgoData ) == false )
		{
			pass = false;
		}
	}
	return pass;
}
Bool CAndSwarmSoundFilter::FilterBoid( const CBaseCritterAI & baseAI )const
{
	Bool pass = true;
	for ( Uint32 i = 0; i < m_childrenArray.Size(); ++i )
	{
		const CBaseSwarmSoundFilter *const filter = m_childrenArray[ i ];
		if ( filter->FilterBoid( baseAI ) == false )
		{
			pass = false;
		}
	}
	return pass;
}

Bool CAndSwarmSoundFilter::FilterFlyingGroup( const CFlyingSwarmGroup & group )const
{
	Bool pass = true;
	for ( Uint32 i = 0; i < m_childrenArray.Size(); ++i )
	{
		const CBaseSwarmSoundFilter *const filter = m_childrenArray[ i ];
		if ( filter->FilterFlyingGroup( group ) == false )
		{
			pass = false;
		}
	}
	return pass;
}

Bool CAndSwarmSoundFilter::ParseXml( const SCustomNode & parentNode, const CBoidLairParams *const boidLairParams, CSwarmSoundConfig *const soundConfig )
{
	const TDynArray< SCustomNode >::const_iterator end	= parentNode.m_subNodes.End();
	TDynArray< SCustomNode >::const_iterator  it;

	for(  it = parentNode.m_subNodes.Begin();  it != end; ++it )
	{
		const SCustomNode & node = *it;	
		if ( node.m_nodeName == CNAME( filter ) )
		{
			CBaseSwarmSoundFilter *const filter = soundConfig->CreateFilterFromXml( parentNode, boidLairParams );
			if ( filter == NULL )
			{
				return false;
			}
			m_childrenArray.PushBack( filter );
		}
		else
		{
			GFeedback->ShowError( TXT("Boid XML Error: sound config unrekognized node: %s"), node.m_nodeName.AsString().AsChar() );
			return false;
		}
	}
	return true;
}
////////////////////////////////////////////////////////////
// CBoidStateSwarmSoundFilter
CBoidStateSwarmSoundFilter::CBoidStateSwarmSoundFilter()
	: CBaseSwarmSoundFilter( )
	, m_boidState( BOID_STATE_NOT_SPAWNED )
{

}

Bool CBoidStateSwarmSoundFilter::FilterLair( const CSwarmAlgorithmData & swarmAlgoData )const
{
	if ( m_boidState == BOID_STATE_NOT_SPAWNED )
	{
		return false;
	}
	if ( swarmAlgoData.GetBoidStateCount( m_boidState ) > 0 )
	{
		return true;
	}
	return false;
}

Bool CBoidStateSwarmSoundFilter::FilterBoid( const CBaseCritterAI & baseAI )const
{
	if ( m_boidState == BOID_STATE_NOT_SPAWNED )
	{
		return false;
	}
	if ( baseAI.GetBoidState() == m_boidState )
	{
		return true;
	}
	return false;
}

Bool CBoidStateSwarmSoundFilter::FilterFlyingGroup( const CFlyingSwarmGroup & group )const
{
	return true;
}

Bool CBoidStateSwarmSoundFilter::ParseXml( const SCustomNode & parentNode, const CBoidLairParams *const boidLairParams, CSwarmSoundConfig *const soundConfig )
{
	// First filling generic attributes
	const TDynArray< SCustomNodeAttribute >::const_iterator attEnd	= parentNode.m_attributes.End();
	TDynArray< SCustomNodeAttribute >::const_iterator		attIt;
	for ( attIt = parentNode.m_attributes.Begin();  attIt !=  attEnd; ++attIt )
	{
		const SCustomNodeAttribute & att	= *attIt;
		if ( att.m_attributeName == CNAME( boidState_name ) )
		{
			m_boidState = (EBoidState)boidLairParams->GetBoidStateIndexFromName( att.m_attributeValueAsCName );
			if ( m_boidState == BOID_STATE_NOT_SPAWNED )
			{
				GFeedback->ShowError( TXT("Boid XML Error: sound filter unrekognized boidState: %s"), att.m_attributeValueAsCName.AsString().AsChar() );
				return false;
			}
		}
		else 
		{
			GFeedback->ShowError( TXT("Boid XML Error: sound filter unrekognized attribute: %s"), att.m_attributeName.AsString().AsChar() );
			return false;
		}
	}
	return true;
}

/////////////////////////////////////////////////////////////
// CPoiSwarmSoundFilter
CPoiSwarmSoundFilter::CPoiSwarmSoundFilter()
	: CBaseSwarmSoundFilter( )
	, m_poiType( CName::NONE )
{
}

Bool CPoiSwarmSoundFilter::FilterLair( const CSwarmAlgorithmData & swarmAlgoData )const
{
	if ( m_poiType == CName::NONE )
	{
		return false;
	}
	if ( swarmAlgoData.GetBoidCountInEffector( ) > 0 )
	{
		return true;
	}
	return false;
}
Bool CPoiSwarmSoundFilter::FilterBoid( const CBaseCritterAI & baseAI )const
{
	if ( m_poiType == CName::NONE )
	{
		return false;
	}
	if ( baseAI.GetCurrentEffectorType() == m_poiType )
	{
		return true;
	}
	return false;
}

Bool CPoiSwarmSoundFilter::FilterFlyingGroup( const CFlyingSwarmGroup & group )const
{
	return true;
}
Bool CPoiSwarmSoundFilter::ParseXml( const SCustomNode & parentNode, const CBoidLairParams *const boidLairParams, CSwarmSoundConfig *const soundConfig )
{
	// First filling generic attributes
	const TDynArray< SCustomNodeAttribute >::const_iterator attEnd	= parentNode.m_attributes.End();
	TDynArray< SCustomNodeAttribute >::const_iterator		attIt;
	for ( attIt = parentNode.m_attributes.Begin();  attIt !=  attEnd; ++attIt )
	{
		const SCustomNodeAttribute & att	= *attIt;
		if ( att.m_attributeName == CNAME( poiType_name ) )
		{
			m_poiType = att.m_attributeValueAsCName;
		}
		else 
		{
			GFeedback->ShowError( TXT("Boid XML Error: sound filter unrekognized attribute: %s"), att.m_attributeName.AsString().AsChar() );
			return false;
		}
	}
	return true;
}

/////////////////////////////////////////////////////////////
// CSwarmSoundConfig
CSwarmSoundConfig::CSwarmSoundConfig( )
	: m_eventStart( StringAnsi::EMPTY )
	, m_eventStop( StringAnsi::EMPTY )
	, m_distanceParameter( StringAnsi::EMPTY )
	, m_countParameter( StringAnsi::EMPTY )
	, m_radiusParameter( StringAnsi::EMPTY )
	, m_yawParameter( StringAnsi::EMPTY )
	, m_pitchParameter( StringAnsi::EMPTY )
	, m_rootFilter( NULL )
{
	if ( m_eventStart == m_eventStop )
	{
	}
}

CSwarmSoundConfig::~CSwarmSoundConfig()
{
	delete m_rootFilter;
}

Bool CSwarmSoundConfig::FilterLair( const CSwarmAlgorithmData & swarmAlgoData )const 
{ 
	if ( m_rootFilter == NULL )
	{
		return false;
	}
	return m_rootFilter->FilterLair( swarmAlgoData ); 
}
Bool CSwarmSoundConfig::FilterBoid( const CBaseCritterAI & baseAI )const 
{ 
	if ( m_rootFilter == NULL )
	{
		return false;
	}
	return m_rootFilter->FilterBoid( baseAI ); 
}

Bool CSwarmSoundConfig::ParseXml( const SCustomNode & parentNode, const CBoidLairParams *const boidLairParams, CBoidSpecies *const boidSpecies )
{
	// First filling generic attributes
	const TDynArray< SCustomNodeAttribute >::const_iterator attEnd	= parentNode.m_attributes.End();
	TDynArray< SCustomNodeAttribute >::const_iterator		attIt;
	for ( attIt = parentNode.m_attributes.Begin();  attIt !=  attEnd; ++attIt )
	{
		const SCustomNodeAttribute & att	= *attIt;
		if ( att.m_attributeName == CNAME( startEvent ) )
		{
			m_eventStart = UNICODE_TO_ANSI( att.GetValueAsString().AsChar() );
		}
		else if ( att.m_attributeName == CNAME( stopEvent ) )
		{
			m_eventStop = UNICODE_TO_ANSI( att.GetValueAsString().AsChar() );
		}
		else if ( att.m_attributeName == CNAME( countParam ) )
		{
			m_countParameter = UNICODE_TO_ANSI( att.GetValueAsString().AsChar() );
		}
		else if ( att.m_attributeName == CNAME( distanceParam ) )
		{
			m_distanceParameter = UNICODE_TO_ANSI( att.GetValueAsString().AsChar() );
		}
		else if ( att.m_attributeName == CNAME( yawParam ) )
		{
			m_yawParameter = UNICODE_TO_ANSI( att.GetValueAsString().AsChar() );
		}
		else if ( att.m_attributeName == CNAME( pitchParam ) )
		{
			m_pitchParameter = UNICODE_TO_ANSI( att.GetValueAsString().AsChar() );
		}
		else
		{
			GFeedback->ShowError( TXT("Boid XML Error: sound unrekognized attribute: %s"), att.m_attributeName.AsString().AsChar() );
			return false;
		}
	}

	// parsing filter :
	if( parentNode.m_subNodes.Size() != 0 )
	{
		const SCustomNode & node = parentNode.m_subNodes[ 0 ];	
		if ( node.m_nodeName == CNAME( filter ) )
		{
			m_rootFilter = CreateFilterFromXml( node, boidLairParams );
			if ( m_rootFilter == NULL )
			{
				return false;
			}
			return true;
		}
		else
		{
			GFeedback->ShowError( TXT("Boid XML Error: sound config unrekognized node: %s"), node.m_nodeName.AsString().AsChar() );
			return NULL;
		}
	}
	return true;
}

Bool CSwarmSoundConfig::operator==( const CSwarmSoundConfig & soundConfig )const
{
	if ( Red::System::StringCompare( m_eventStart.AsChar(), soundConfig.m_eventStart.AsChar(),  Max( m_eventStart.GetLength(), soundConfig.m_eventStart.GetLength() ) ) == 0 )
	{
		return true;
	}
	return false;
}


CBaseSwarmSoundFilter*const CSwarmSoundConfig::CreateFilterFromXml( const SCustomNode & parentNode, const CBoidLairParams *const boidLairParams )
{
	CBaseSwarmSoundFilter * filter = NULL;
	if( parentNode.m_attributes.Size() != 0 )
	{
		const SCustomNodeAttribute & att	= parentNode.m_attributes[ 0 ];
		filter								= CreateXmlFromXmlAtt( att );

		if ( filter == NULL ) 
		{
			GFeedback->ShowError( TXT("Boid XML Error: sound config unrekognized filter type: %s"), att.m_attributeName.AsString().AsChar() );
			return NULL;
		}
	}

	if ( filter->ParseXml( parentNode, boidLairParams, this ) == false )
	{
		delete filter;
		return NULL;
	}
	return filter;
}

CBaseSwarmSoundFilter*const CSwarmSoundConfig::CreateXmlFromXmlAtt( const SCustomNodeAttribute & att )
{

	if ( att.m_attributeName == CNAME( type_name ) )
	{
		if ( att.m_attributeValueAsCName == CNAME( or ) )
		{
			return  new COrSwarmSoundFilter();
		}
		else if ( att.m_attributeValueAsCName == CNAME( and ) )
		{
			return  new CAndSwarmSoundFilter();
		}
		else
		{
			GFeedback->ShowError( TXT("Boid XML Error: sound unrekognized sound filter type: %s"), att.m_attributeValueAsCName.AsString().AsChar() );
			return NULL;
		}
	}
	else if ( att.m_attributeName == CNAME( boidState_name ) )
	{
		return new CBoidStateSwarmSoundFilter();			
	}
	else if ( att.m_attributeName == CNAME( poiType_name ) )
	{
		return new CPoiSwarmSoundFilter();			
	}
	
	
	return NULL;
}
/////////////////////////////////////////////////////////////
// CSwarmSoundJobDataCollection
CSwarmSoundJobDataCollection::CSwarmSoundJobDataCollection( const CBoidLairParams & params )
	: m_jobDataArray()
	, m_soundCollectionId( (Uint32)-1 )	
{
	for ( Uint32 soundIndex = 0; soundIndex < params.m_soundConfigArray.Size(); ++soundIndex )
	{
		m_jobDataArray.PushBack( CSwarmSoundJobData( soundIndex ) );
	}
}

/////////////////////////////////////////////////////////////
// CBoidSound 
void CBoidSound::GetLocalToWorld( Matrix& m ) const
{
	m.SetZeros();
	m.SetTranslation( m_position );
}

void CBoidSound::Update( const Vector& cameraPos, const CSwarmSoundJobData& soundJobData )
{
	m_boidsCount	= soundJobData.m_intensity;
	m_position		= soundJobData.m_center;
	m_radius		= soundJobData.m_radius;
	m_distance		= soundJobData.m_distance;
	m_yaw			= soundJobData.m_yaw;
	m_pitch			= soundJobData.m_pitch;
}

void CBoidSound::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	if ( m_boidsCount > 0 )
	{
		const Box box( m_position, 0.5f );
		frame->AddDebugBox( box, Matrix::IDENTITY, Color::BLUE );
		frame->AddDebugText( m_position + Vector3( 0.0f, 0.0f, 0.5f), String::Printf( TXT("%i"), (Int32)m_boidsCount ) );
		frame->AddDebugText( m_position + Vector3( 0.5f, 0.0f, -1.0f), String::Printf( TXT("%i"), (Int32)m_distance ) );
	}
}

/////////////////////////////////////////////////////////////
// CBoidSoundsCollection
Uint32 CBoidSoundsCollection::s_nextId = 0;
CBoidSoundsCollection::CBoidSoundsCollection( CSoundEmitterComponent*const soundComponent, const CBoidLairParams & params )
	: m_runningSounds()
	, m_soundComponent(soundComponent)
	, m_Id( (Uint32)-1 )
{
	m_Id				= s_nextId++;
	
	for ( Uint32 soundIndex = 0; soundIndex < params.m_soundConfigArray.Size(); ++soundIndex )
	{
		const CSwarmSoundConfig *const soundConfig		= params.m_soundConfigArray[ soundIndex ];
		AddSound( new CBoidSound( soundConfig->m_eventStart, soundConfig->m_eventStop, soundConfig->m_countParameter, soundConfig->m_distanceParameter, soundConfig->m_yawParameter, soundConfig->m_pitchParameter ) );
	}
}
CBoidSoundsCollection::~CBoidSoundsCollection()
{
	for ( Uint32 i = 0, n = m_runningSounds.Size(); i != n; ++i )
	{
		CBoidSound* sound = m_runningSounds[ i ];
		delete sound;
	}
}

void CBoidSoundsCollection::Update( const Vector& cameraPos, const CSwarmSoundJobDataCollection *const soundJobDataCollection )
{
	CSoundEmitterComponent *const soundComponent = m_soundComponent.Get();
	if ( soundComponent == nullptr )
	{
		return;
	}
	for ( Uint32 i = 0, n = m_runningSounds.Size(); i != n; ++i )
	{
		if ( i > 99 ) // that would be bad 99 sounds playing on one swarm !
		{
			break;
		}
		const Uint32 soundId = m_Id * 100 + i;
		
		CBoidSound* sound = m_runningSounds[ i ];
		if ( soundJobDataCollection )
		{
			sound->Update( cameraPos, soundJobDataCollection->m_jobDataArray[ i ] );
		}

		if ( sound->m_boidsCount > 0.0f && soundJobDataCollection ) // if no job data we stop the sound 
		{
			// Start event must be done first according to audio guys ( which makes sense )
			if ( sound->m_isRunning == false )
			{
				soundComponent->SoundEvent( sound->m_eventStart.AsChar(), soundId );
				sound->m_isRunning = true;
			}
			if ( sound->m_countParameter.Empty() == false )
			{
				soundComponent->SoundParameter( sound->m_countParameter.AsChar(), sound->m_boidsCount, 0.0f, soundId );
			}
			if ( sound->m_radiusParameter.Empty() == false )
			{
				soundComponent->SoundParameter( sound->m_radiusParameter.AsChar(), sound->m_radius, 0.0f, soundId );
			}
			if ( sound->m_distanceParameter.Empty() == false )
			{
				soundComponent->SoundParameter( sound->m_distanceParameter.AsChar(), sound->m_distance, 0.0f, soundId );
			}
			if ( sound->m_yawParameter.Empty() == false )
			{
				soundComponent->SoundParameter( sound->m_yawParameter.AsChar(), sound->m_yaw, 0.0f, soundId );
			}
			if ( sound->m_pitchParameter.Empty() == false )
			{
				soundComponent->SoundParameter( sound->m_pitchParameter.AsChar(), sound->m_pitch, 0.0f, soundId );
			}
			
		}
		else if ( sound->m_isRunning )
		{
			soundComponent->SoundEvent( sound->m_eventStop.AsChar(), soundId );
			sound->m_isRunning = false;
		}
	}
}
void CBoidSoundsCollection::FadeOutAll()
{
	CSoundEmitterComponent *const soundComponent = m_soundComponent.Get();
	if ( soundComponent == nullptr )
	{
		return;
	}
	for ( Uint32 i = 0, n = m_runningSounds.Size(); i != n; ++i )
	{
		const Uint32 soundId = m_Id * 100 + i;
		if ( m_runningSounds[ i ]->m_isRunning )
		{
			soundComponent->SoundEvent( m_runningSounds[ i ]->m_eventStop.AsChar(), soundId );
			m_runningSounds[ i ]->m_isRunning = false;
		}
	}
}