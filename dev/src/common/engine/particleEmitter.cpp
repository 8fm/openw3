/**
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "particleEmitter.h"

#include "../physics/physicsParticleWrapper.h"
#include "particleSystem.h"
#include "particleInitializer.h"
#include "particleDrawer.h"
#include "particleModificator.h"
#include "renderer.h"
#include "material.h"
#include "environmentAreaParams.h"

#include "evaluatorFloat.h"

#include "../renderer/renderParticleBuffer.h"
#include "../renderer/renderParticleEmitter.h"
#include "baseEngine.h"


IMPLEMENT_ENGINE_CLASS( CParticleEmitter );
IMPLEMENT_ENGINE_CLASS( ParticleBurst );
IMPLEMENT_ENGINE_CLASS( EmitterDurationSettings );
IMPLEMENT_ENGINE_CLASS( EmitterDelaySettings );
IMPLEMENT_ENGINE_CLASS( SParticleEmitterLODLevel );
IMPLEMENT_ENGINE_CLASS( SSeedKeyValue );


SParticleEmitterLODLevel::SParticleEmitterLODLevel()
	: m_birthRate( nullptr )
	, m_sortBackToFront( false )
	, m_isEnabled( true )
{
}

SParticleEmitterLODLevel::SParticleEmitterLODLevel( const SParticleEmitterLODLevel& other )
	: m_burstList( other.m_burstList )
	, m_birthRate( nullptr )
	, m_emitterDurationSettings( other.m_emitterDurationSettings )
	, m_emitterDelaySettings( other.m_emitterDelaySettings )
	, m_sortBackToFront( other.m_sortBackToFront )
	, m_isEnabled( other.m_isEnabled )
{
	if ( other.m_birthRate != nullptr )
	{
		m_birthRate = Cast< IEvaluatorFloat >( other.m_birthRate->Clone( other.m_birthRate->GetParent() ) );
	}
}

SParticleEmitterLODLevel& SParticleEmitterLODLevel::operator =( const SParticleEmitterLODLevel& other )
{
	m_burstList = other.m_burstList;
	m_birthRate = nullptr;
	m_emitterDurationSettings = other.m_emitterDurationSettings;
	m_emitterDelaySettings = other.m_emitterDelaySettings;
	m_sortBackToFront = other.m_sortBackToFront;
	m_isEnabled = other.m_isEnabled;

	if ( other.m_birthRate != nullptr )
	{
		m_birthRate = Cast< IEvaluatorFloat >( other.m_birthRate->Clone( other.m_birthRate->GetParent() ) );
	}
	else
	{
		m_birthRate = nullptr;
	}

	return *this;
}

SParticleEmitterLODLevel::SParticleEmitterLODLevel( SParticleEmitterLODLevel&& other )
	: m_burstList( other.m_burstList )
	, m_birthRate( other.m_birthRate )
	, m_emitterDurationSettings( other.m_emitterDurationSettings )
	, m_emitterDelaySettings( other.m_emitterDelaySettings )
	, m_sortBackToFront( other.m_sortBackToFront )
	, m_isEnabled( other.m_isEnabled )
{
	other.m_birthRate = nullptr;
}

SParticleEmitterLODLevel& SParticleEmitterLODLevel::operator =( SParticleEmitterLODLevel&& other )
{
	m_burstList = other.m_burstList;
	m_birthRate = other.m_birthRate;
	m_emitterDurationSettings = other.m_emitterDurationSettings;
	m_emitterDelaySettings = other.m_emitterDelaySettings;
	m_sortBackToFront = other.m_sortBackToFront;
	m_isEnabled = other.m_isEnabled;

	other.m_birthRate = nullptr;

	return *this;
}


CParticleEmitter::CParticleEmitter()
	: m_material( NULL )
	, m_maxParticles( 55 )
	, m_emitterLoops( 0 )	// Infinite
	, m_positionX(0)
	, m_positionY(0)
	, m_particleDrawer( NULL )
	, m_keepSimulationLocal( false )
	, m_envColorGroup( ECG_Default )
	, m_initializerSetMask( 0 )
	, m_numInitializers( 0 )
	, m_modifierSetMask( 0 )
	, m_numModifiers( 0 )
	, m_renderResource( NULL )
	, m_updaterData( NULL )
	, m_windInfluence( 0.0f )
	, m_internalPriority( 0 )
	, m_collisionDecalSpawner( NULL )
	, m_decalSpawner( NULL )
	, m_motionDecalSpawner( NULL )
#ifndef NO_EDITOR
	, m_uniqueId( -1 )
#endif
{
#ifndef NO_EDITOR
	CClass* emitterClass = SRTTI::GetInstance().FindClass( CNAME( CParticleDrawerBillboard ) );
	m_particleDrawer = CreateObject< IParticleDrawer >( emitterClass, this );
#endif
}

CParticleEmitter::~CParticleEmitter()
{
	if ( m_renderResource )
	{
		m_renderResource->Release();
		m_renderResource = NULL;
	}

	if ( m_updaterData )
	{
		delete m_updaterData;
		m_updaterData = NULL;
	}
}

// Returns a random color
static Color RandomPastelColor()
{
	Uint8 r = GEngine->GetRandomNumberGenerator().Get< Uint8 >();
	Uint8 g = GEngine->GetRandomNumberGenerator().Get< Uint8 >();
	Uint8 b = GEngine->GetRandomNumberGenerator().Get< Uint8 >();

	r = ( 127 + r & 127 );
	g = ( 127 + g & 127 );
	b = ( 127 + b & 127 );

	return Color( r, g, b, 255 );
}

void CParticleEmitter::OnPropertyPostChange( IProperty* property )
{
	// Pass to base class
	TBaseClass::OnPropertyPostChange( property );

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	if ( property->GetName() == TXT("particleDrawer") )
	{
		if ( m_material ) m_material->ForceRecompilation();
		return;
	}
#endif

	if ( property->GetName() == TXT("windInfluence") )
	{
		//m_windInfluence = Clamp( m_windInfluence, 0.0f, 1.0f );
	}
}

void CParticleEmitter::ResetInstances()
{
	CParticleSystem* system = FindParent< CParticleSystem >();
	system->ResetInstances();
}

IParticleModule* CParticleEmitter::AddModule( CClass* moduleClass, const String& moduleName /*=String::EMPTY*/ )
{
	// Create module
	IParticleModule* module = ::CreateObject< IParticleModule >( moduleClass, this );
	if ( !module )
	{
		WARN_ENGINE( TXT("Unable to add particle module") );
		return NULL;
	}

	// Add to list
	m_modules.PushBack( module );
	module->SetEditorColor( RandomPastelColor() );

	return module;
}

void CParticleEmitter::RemoveModule( IParticleModule* module )
{
	// Remove from list
	if ( !m_modules.Remove( module ) )
	{
		WARN_ENGINE( TXT("Unable to remove particle module") );
		return;
	}

	// Remove from list
	module->Discard();
}

Bool CParticleEmitter::MoveModule( IParticleModule* module, Int32 delta )
{
	// Find module in the list
	const Int32 count = m_modules.Size();
	for ( Int32 i=0; i<count; i++ )
	{
		IParticleModule* foundModule = m_modules[i];
		if ( foundModule == module )
		{
			// Check range
			Int32 newIndex = i + delta;
			if ( newIndex >= 0 && newIndex < count )
			{
				// Swap modules
				::Swap( m_modules[i], m_modules[newIndex] );
				return true;
			}

			// Not moved
			return false;
		}
	}

	// Not found
	return false;
}

void CParticleEmitter::GenerateApproximatedUpdaterData( SParticleUpdaterData& updaterData ) const
{
	if ( IsCooked() )
	{
		updaterData = *m_updaterData;
	}
	else
	{
		EParticleType particleType = GetParticleType();

		// Get all mask flags from modifiers
		for ( Uint32 i=0; i<m_modules.Size(); i++ )
		{
			IParticleInitializer* module = Cast< IParticleInitializer >( m_modules[i] );
			if ( module && module->IsEnabled() ) // ignore disabled modifiers
			{
				module->SetupParticleUpdateData( updaterData, particleType );
			}
		}
	}
}

void CParticleEmitter::SetupSimulationFlags( Uint32& modifierSetMask, Uint32& initializerSetMask, Uint32& numModifiers, Uint32& numInitializers, TDynArray< SSeedKeyValue >& seeds ) const
{
	if ( IsCooked() )
	{
		initializerSetMask = m_initializerSetMask;
		modifierSetMask = m_modifierSetMask;
		numModifiers = m_numModifiers;
		numInitializers = m_numInitializers;
		seeds = m_seeds;
	}
	else
	{
		modifierSetMask = 0;
		initializerSetMask = 0;
		numModifiers = 0;
		numInitializers = 0;
		seeds.Reserve( m_modules.Size() );

		EParticleType particleType = GetParticleType();
		if ( particleType == PT_Invalid )
		{
			WARN_ENGINE( TXT("Invalid particle type") );
			return;
		}

		// Generate initializer and modifier masks
		for ( Uint32 i=0; i<m_modules.Size(); i++ )
		{
			IParticleInitializer* initializer = Cast< IParticleInitializer >( m_modules[i] );
			if ( initializer && initializer->IsEnabled() ) // ignore disabled modules
			{
				Bool isParticleTypeSupported = false;
				switch( particleType )
				{
				case PT_Simple:
					isParticleTypeSupported = initializer->SupportsParticleType< SimpleParticle >();
					break;
				case PT_Trail:
					isParticleTypeSupported = initializer->SupportsParticleType< TrailParticle >();
					break;
				case PT_FacingTrail:
					isParticleTypeSupported = initializer->SupportsParticleType< FacingTrailParticle >();
					break;
				case PT_Mesh:
					isParticleTypeSupported = initializer->SupportsParticleType< MeshParticle >();
					break;
				case PT_Beam:
					isParticleTypeSupported = initializer->SupportsParticleType< BeamParticle >();
					break;
				default:
					WARN_ENGINE( TXT( "Initializing particle emitter without drawer" ) );
				}

				if ( !isParticleTypeSupported )
				{
					continue;
				}

				IParticleModificator* modifier = Cast< IParticleModificator >( initializer );
				if ( modifier )
				{
					Uint32 flag = modifier->GetModificatorId();
					if ( flag && ( flag & modifierSetMask ) == 0 )
					{
						modifierSetMask |= flag;
						++( numModifiers );
					}
				}

				Uint32 flag = initializer->GetInitializerId();
				if ( flag && ( flag & initializerSetMask ) == 0 )
				{
					initializerSetMask |= flag;
					++( numInitializers );
					InsertSeedInOrder( flag, initializer->m_seed, seeds );
				}
			}
		}
	}
}

EParticleType CParticleEmitter::GetParticleType() const
{
	if ( m_particleDrawer )
	{
		return m_particleDrawer->GetParticleType();
	}

	return PT_Invalid;
}

//////////////////////////////////////////////////////////////////////////

void CParticleEmitter::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	if ( IsCooked() )
	{
		ASSERT( m_modules.Empty() );
	}
}

#ifndef NO_RESOURCE_COOKING
void CParticleEmitter::OnCook( class ICookerFramework& cooker )
{
	TBaseClass::OnCook( cooker );

	// Prepare masks based on a set of modules. Modules are dropped when cooked.
	SetupSimulationFlags( m_modifierSetMask, m_initializerSetMask, m_numModifiers, m_numInitializers, m_seeds );

	// Generate approximations
	if ( m_updaterData )
	{
		delete m_updaterData;
		m_updaterData = NULL;
	}
	m_updaterData = new SParticleUpdaterData;
	GenerateApproximatedUpdaterData( *m_updaterData );
}
#endif // NO_RESOURCE_COOKING

void CParticleEmitter::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( file.IsCooker() )
	{
		if( m_updaterData != nullptr )
		{
			file << *m_updaterData;
			SetFlag( OF_WasCooked );
		}
	}
	else if ( IsCooked() )
	{
		if ( file.IsReader() )
		{
			m_updaterData = new SParticleUpdaterData;
		}

		if ( !file.IsGarbageCollector() )
		{
			file << *m_updaterData;
		}
	}
}

Bool CParticleEmitter::CollectMeshes( TDynArray< CMesh* >& meshes ) const
{
	if ( m_particleDrawer )
	{
		const IParticleDrawer::TMeshList* inMeshes = m_particleDrawer->GetMeshes();
		if ( nullptr != inMeshes )
		{
			for ( Uint32 i=0; i<inMeshes->Size(); ++i )
			{
				meshes.PushBack( (*inMeshes)[i].Get() );
			}

			return true;
		}
	}
	return false;
}

void CParticleEmitter::CreateRenderResource()
{
	SAFE_RELEASE( m_renderResource );

	m_renderResource = GRender->CreateParticleEmitter( this );
}

void CParticleEmitter::ReleaseRenderResource()
{
	SAFE_RELEASE( m_renderResource );
}

IRenderResource* CParticleEmitter::GetRenderResource() const 
{
	if ( !m_renderResource )
	{
		const_cast< CParticleEmitter* >( this )->CreateRenderResource();
	}

	return m_renderResource; 
}

Bool CParticleEmitter::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	// Convert old non-LOD-ed data to LOD-ready. Any settings just go into LOD0.
	if ( propertyName == TXT("emitterDuration") )
	{
		Float duration;
		readValue.AsType< Float >( duration );

		RED_ASSERT( m_lods.Size() <= 1 );
		if ( m_lods.Size() < 1 )
		{
			m_lods.Resize( 1 );
		}
		for ( SParticleEmitterLODLevel& lod : m_lods )
		{
			lod.m_emitterDurationSettings.m_emitterDuration = duration;
		}
		return true;
	}
	else if ( propertyName == TXT("emitterDurationSettings") )
	{
		EmitterDurationSettings emitterDurationSettings;
		readValue.AsType< EmitterDurationSettings >( emitterDurationSettings );

		RED_ASSERT( m_lods.Size() <= 1 );
		if ( m_lods.Size() < 1 )
		{
			m_lods.Resize( 1 );
		}
		for ( SParticleEmitterLODLevel& lod : m_lods )
		{
			lod.m_emitterDurationSettings = emitterDurationSettings;
		}
		return true;
	}
	else if ( propertyName == TXT("emitterDelaySettings") )
	{
		EmitterDelaySettings emitterDelaySettings;
		readValue.AsType< EmitterDelaySettings >( emitterDelaySettings );

		RED_ASSERT( m_lods.Size() <= 1 );
		if ( m_lods.Size() < 1 )
		{
			m_lods.Resize( 1 );
		}
		for ( SParticleEmitterLODLevel& lod : m_lods )
		{
			lod.m_emitterDelaySettings = emitterDelaySettings;
		}
		return true;
	}
	else if ( propertyName == TXT("burstList") )
	{
		TDynArray< ParticleBurst > burstList;
		readValue.AsType< TDynArray< ParticleBurst > >( burstList );

		RED_ASSERT( m_lods.Size() <= 1 );
		if ( m_lods.Size() < 1 )
		{
			m_lods.Resize( 1 );
		}
		for ( SParticleEmitterLODLevel& lod : m_lods )
		{
			lod.m_burstList = burstList;
		}
		return true;
	}
	else if ( propertyName == TXT("birthRate") )
	{
		IEvaluatorFloat* birthRate;
		readValue.AsType< IEvaluatorFloat* >( birthRate );

		RED_ASSERT( m_lods.Size() <= 1 );
		if ( m_lods.Size() < 1 )
		{
			m_lods.Resize( 1 );
		}
		for ( SParticleEmitterLODLevel& lod : m_lods )
		{
			lod.m_birthRate = birthRate;
		}
		return true;
	}
	else if ( propertyName == TXT("sortBackToFront") )
	{
		Bool sortBackToFront;
		readValue.AsType< Bool >( sortBackToFront );

		RED_ASSERT( m_lods.Size() <= 1 );
		if ( m_lods.Size() < 1 )
		{
			m_lods.Resize( 1 );
		}
		for ( SParticleEmitterLODLevel& lod : m_lods )
		{
			lod.m_sortBackToFront = sortBackToFront;
		}
		return true;
	}

	// Pass to base class
	return TBaseClass::OnPropertyMissing( propertyName, readValue );
}


void CParticleEmitter::AddLOD()
{
	if ( m_lods.Size() > 0 )
	{
		// Clone last LOD.
		m_lods.PushBack( SParticleEmitterLODLevel( m_lods.Back() ) );
	}
	else
	{
		m_lods.Grow( 1 );
	}
}

void CParticleEmitter::RemoveLOD( Uint32 level )
{
	RED_ASSERT( level < m_lods.Size(), TXT("Trying to remove out-of-range LOD %u"), level );
	RED_ASSERT( m_lods.Size() > 1, TXT("Must have at least one LOD") );

	if ( level >= m_lods.Size() )
	{
		return;
	}

	m_lods.RemoveAt( level );
}


void CParticleEmitter::SetLODCount( Uint32 lodCount )
{
	if ( m_lods.Size() > lodCount )
	{
		m_lods.Resize( lodCount );
	}
	else
	{
		// Add copies of the last LOD until we have enough.
		while ( m_lods.Size() < lodCount )
		{
			AddLOD();
		}
	}
}

void CParticleEmitter::InsertSeedInOrder(Uint32 flag, Uint32 seed, TDynArray< SSeedKeyValue >& seeds) const
{
	Uint32 i = 0; 
	Uint32 numSeeds = seeds.Size();
	while( i < numSeeds && seeds[i].m_key < flag ){ ++i; }
	
	seeds.Insert( i, SSeedKeyValue( flag, seed ) );
}