#include "build.h"
#include "physicsEngine.h"
#include "../physics/physicsWrapper.h"
#include "../../common/core/gatheredResource.h"
#include "physicsDebugger.h"
#include "../core/scriptStackFrame.h"
#include "physicsWorld.h"
#include "../core/2darray.h"
#include "physicsIncludes.h"
#include "../core/depot.h"

CGatheredResource resTexturesPhysicalMaterials( TXT("gameplay\\globals\\terrain_textures_physical_materials.csv"), RGF_Startup );
CGatheredResource resPhysicalMaterials( TXT("gameplay\\globals\\physical_materials.csv"), RGF_Startup );
CGatheredResource resCollisionTypes( TXT("gameplay\\globals\\collision_matrix.csv"), RGF_Startup );

SPhysicalMaterial::~SPhysicalMaterial()
{
	if( m_ansiName ) RED_MEMORY_FREE( MemoryPool_Physics, MC_PhysicsMaterial, m_ansiName ); 
	m_ansiName = 0;
	if ( m_decalFootstepsDiff )	m_decalFootstepsDiff->RemoveFromRootSet();
	m_decalFootstepsDiff = nullptr;
	if ( m_decalFootstepsNormal ) m_decalFootstepsNormal->RemoveFromRootSet();
	m_decalFootstepsNormal = nullptr;
}

CPhysicsEngine* GPhysicEngine = NULL;

CPhysicsEngine::CPhysicsEngine() : m_collisionSetMasks( 0 )
{

}

CPhysicsEngine::~CPhysicsEngine()
{
#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().UnregisterListener( this );
#endif
	//ClearMaterials();
	if( m_collisionSetMasks )
	{
		RED_MEMORY_FREE( MemoryPool_Physics, MC_PhysicsEngine, m_collisionSetMasks );
		m_collisionSetMasks = 0;
	}
}

void CPhysicsEngine::ShutDown()
{
	CPhysicsWorld* current = CPhysicsWorld::m_top;
	while( current )
	{
		CPhysicsWorld* toDelete = current;
		current = current->m_next;
		delete toDelete;
	}
}

void CPhysicsEngine::Update( Float timeDelta )
{
	CPhysicsWorld* current = CPhysicsWorld::m_top;
	CPhysicsWorld* previous = 0;
	while( current )
	{
		CPhysicsWorld* toDelete = 0;
		if( current->IsToDelete() )
		{
			current->TickRemovalWrappers( true );
			if(  current->m_ref.GetValue() == 0 && !current->HasWrappers() )
			{
				toDelete = current;
				if( CPhysicsWorld::m_top == current )
				{
					previous = 0;
					CPhysicsWorld::m_top = current->m_next;
				}
				if( previous )
				{
					previous->m_next = current->m_next;
				}
			}
			else
			{
				previous = current;
			}
		}
		else
		{
			previous = current;
		}

		current = current->m_next;
		if( toDelete )
		{
			delete toDelete;
		}
	}
}

void CPhysicsEngine::ReloadCollisionGroupDefinitions()
{
	if ( m_collisionSetMasks )
	{
		RED_MEMORY_FREE( MemoryPool_Physics, MC_PhysicsEngine, m_collisionSetMasks );
		m_collisionSetMasks = NULL;
	}

	m_collisionTypeNames.Clear();

	C2dArray* collisionTypes = resCollisionTypes.LoadAndGet< C2dArray >();
	if ( collisionTypes )
	{
		Uint32 collisionTypesCount = static_cast<Uint32>( collisionTypes->GetNumberOfColumns() ) - 1; // take header column into account

		if ( collisionTypesCount > COLLISION_TYPE_MAX )
		{
			ASSERT( 0 && TXT("collision_matrix.csv has too many elements") );
			collisionTypesCount = COLLISION_TYPE_MAX;
		}

		for ( Uint32 index = 1; index <= collisionTypesCount; ++index )
		{
			CName collisionGroupName( collisionTypes->GetHeader( index ) );
			m_collisionTypeNames.PushBack( collisionGroupName );
		}

		Uint32 size = sizeof( CollisionMask ) * m_collisionTypeNames.Size() + sizeof( CollisionMask );
		m_collisionSetMasks = reinterpret_cast< CollisionMask* > ( RED_MEMORY_ALLOCATE( MemoryPool_Physics, MC_PhysicsEngine, size ) );
		Red::System::MemoryZero( m_collisionSetMasks, size );

		for ( Uint32 row = 0; row < collisionTypesCount; ++row )
		{
			CName collisionGroupName( m_collisionTypeNames[ row ] );
			for ( Uint32 col = 0; col < collisionTypesCount; ++col )
			{
				String collisionTarget = collisionTypes->GetValue( col + 1, row ); // take header column into account (header line is taken into account automatically in CSV loading code)
				if ( collisionTarget == TXT("1") )
				{
					Uint64 collisionTargetBit = 1LL << Uint64( col );
					m_collisionSetMasks[ row ] |= collisionTargetBit;

					collisionTargetBit = 1LL << Uint64( row );
					m_collisionSetMasks[ col ] |= collisionTargetBit;
				}
			}
		}
	}
}

void CPhysicsEngine::ClearMaterials()
{
	if( m_physicalMaterials.Size() )
	{
		FlushMaterialInstances();
		m_physicalMaterials.Clear();
	}
}

void CPhysicsEngine::ReloadPhysicalMaterials()
{
	ClearMaterials();

	C2dArray* physicalMaterials = resPhysicalMaterials.LoadAndGet< C2dArray >();

	Uint32 physicalMaterialCount = 1;
	if( physicalMaterials )
	{
		physicalMaterialCount += ( unsigned char ) physicalMaterials->GetNumberOfRows();
	}

	m_physicalMaterials.Grow( physicalMaterialCount );
	SPhysicalMaterial& defaultMaterial = *m_physicalMaterials.Begin();
	defaultMaterial.SetName( CNAME( default ) );
	defaultMaterial.m_middlewareInstance = CreateMaterialInstance( 0.0f, 0.0f, 0.1f, &defaultMaterial );
	defaultMaterial.m_density = 1000.0f;
#ifndef NO_EDITOR
	defaultMaterial.m_debugColor = Color( 10, 10, 10 );
#endif

	if( !physicalMaterials ) return;

	for( unsigned char i = 0; i < physicalMaterialCount - 1; ++i )
	{
		String caseInsensitiveName = physicalMaterials->GetValue( TXT( "MaterialName" ), i );
		caseInsensitiveName.MakeLower();

		SPhysicalMaterial& material = m_physicalMaterials[ i + 1 ];

		CName materialName( caseInsensitiveName );
		material.SetName( materialName );

		Float dynamicFriction, staticFriction, restitution, floatRatio;
		C2dArray::ConvertValue( physicalMaterials->GetValue( TXT( "DynamicFriction" ), i ), dynamicFriction );
		C2dArray::ConvertValue( physicalMaterials->GetValue( TXT( "StaticFriction" ), i ), staticFriction );
		C2dArray::ConvertValue( physicalMaterials->GetValue( TXT( "Restitution" ), i ), restitution );
		C2dArray::ConvertValue( physicalMaterials->GetValue( TXT( "Density" ), i ), material.m_density );
		C2dArray::ConvertValue( physicalMaterials->GetValue( TXT( "SoundDirectFilter" ), i ), material.m_soundDirectFilter );
		C2dArray::ConvertValue( physicalMaterials->GetValue( TXT( "SoundReverbFilter" ), i ), material.m_soundReverbFilter );
		C2dArray::ConvertValue( physicalMaterials->GetValue( TXT( "FloatingRatio" ), i ),  floatRatio);

		material.m_floatingRatio = floatRatio == 0.5f ? 0.51f : floatRatio;
		material.m_particleFootsteps = CName( physicalMaterials->GetValue( TXT( "ParticleFootsteps" ), i ) );
		material.m_particleBodyroll = CName( physicalMaterials->GetValue( TXT( "ParticleBodyroll" ), i ) );
		String diff = physicalMaterials->GetValue( TXT( "DecalFootstepsDiff" ), i );
		String norm = physicalMaterials->GetValue( TXT( "DecalFootstepsNorm" ), i );
		if( !diff.Empty() )
		{
			
			material.m_decalFootstepsDiff = GDepot->LoadResource( diff );
			
			if (material.m_decalFootstepsDiff)
			{
				material.m_decalFootstepsDiff->AddToRootSet();
			}
		}
		if( !norm.Empty() )
		{
			material.m_decalFootstepsNormal = GDepot->LoadResource( norm );
			if (material.m_decalFootstepsNormal)
			{
				material.m_decalFootstepsNormal->AddToRootSet();
			}
		}

#ifndef NO_EDITOR
		Color debugMaterialColor;
		C2dArray::ConvertValue( physicalMaterials->GetValue( TXT( "DebugColor" ), i ), material.m_debugColor );
#endif
		material.m_middlewareInstance = CreateMaterialInstance( dynamicFriction, staticFriction, restitution, &material );
	}

	if( m_texturesPhysicalMaterials.Size() )
	{
		m_texturesPhysicalMaterials.Clear();
	}

	C2dArray* texturesPhysicalMaterials = resTexturesPhysicalMaterials.LoadAndGet< C2dArray >();

	Uint32 texturesPhysicalMaterialCount = static_cast< Uint32 >( texturesPhysicalMaterials->GetNumberOfRows() ) + 1;

	if(texturesPhysicalMaterialCount > 0)
	{
		for( Uint32 j = 0; j < texturesPhysicalMaterialCount - 1; ++j )
		{
			const CName texturePhysicalMaterialName( texturesPhysicalMaterials->GetValue( TXT( "TextureFileName" ), j ) );
			const CName physicalMaterialName( texturesPhysicalMaterials->GetValue( TXT( "PhysicalMaterialName" ), j ) );
			for( unsigned char i = 0; i < physicalMaterialCount - 1; ++i )
			{
				SPhysicalMaterial& material = m_physicalMaterials[ i + 1 ];
				if( material.m_name == physicalMaterialName )
				{
					m_texturesPhysicalMaterials[ texturePhysicalMaterialName ] = i + 1;
					break;
				}
			}
			if( !m_texturesPhysicalMaterials.FindPtr( texturePhysicalMaterialName ) )
			{
				ASSERT( 0 && "texture physical material doesn't exist in physical materials definition");
			}
		}
	}
}

#ifndef NO_EDITOR_EVENT_SYSTEM
void CPhysicsEngine::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( CSVFileSaved ) )
	{
		if( GetCollisionMaskFileResourceName().ContainsSubstring( GetEventData< String >( data ) ) )
			ReloadCollisionGroupDefinitions();
		else if( GetPhysicalMaterialFileResourceName().ContainsSubstring( GetEventData< String >( data ) ) )
			ReloadPhysicalMaterials();
	}
}
#endif // NO_EDITOR_EVENT_SYSTEM

Bool CPhysicsEngine::Init()
{
#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().RegisterListener( CNAME( CSVFileSaved ), this );
#endif

	ReloadCollisionGroupDefinitions();
	ReloadPhysicalMaterials();
	return true;
}

CPhysicsWorld* CPhysicsEngine::CreateWorld( IPhysicsWorldParentProvider* parentProvider, Uint32 areaResolution, Float areaSize, Vector2 areaCornerPosition, Uint32 clipMapSize, Uint32 tileRes, Bool useMemorySettings, Bool criticalDispatch, Bool allowGpu )
{
	return new CPhysicsWorld( parentProvider, areaResolution, areaSize, areaCornerPosition, clipMapSize, tileRes );
}

void CPhysicsEngine::DestroyWorld( CPhysicsWorld* world )
{
	delete world->m_worldParentProvider;
	world->m_worldParentProvider = 0;
}

CPhysicsEngine::CollisionMask CPhysicsEngine::GetCollisionTypeBit( const CName& name )
{
	for ( Uint32 i = 0; i < m_collisionTypeNames.Size(); ++i )
	{
		if ( m_collisionTypeNames[ i ] == name )
		{
			return 1LL << i;
		}
	}
	return 0;
}

CPhysicsEngine::CollisionMask CPhysicsEngine::GetCollisionTypeBit( const TDynArray< CName >& names )
{
	CollisionMask result = 0;
	for ( Uint32 i = 0; i < names.Size(); ++i )
	{
		result |= GetCollisionTypeBit( names[ i ] );
	}
	return result;
}

void CPhysicsEngine::GetGroupsNamesByCollisionMask( CPhysicsEngine::CollisionMask mask, TDynArray< CName > & names )
{
	Uint32 m = static_cast< Uint32 >( mask );
	Uint32 flag = 1;
	for ( Uint32 i = 0; i < m_collisionTypeNames.Size(); ++i )
	{
		if ( m & flag )
		{
			names.PushBack( m_collisionTypeNames[i] );
		}
		flag = flag << 1;
	}
}

CPhysicsEngine::CollisionMask CPhysicsEngine::GetCollisionGroupMask( const CName& name )
{
	for ( Uint32 i = 0; i < m_collisionTypeNames.Size(); ++i )
	{
		if ( m_collisionTypeNames[ i ] == name )
		{
			return m_collisionSetMasks[ i ];
		}
	}
	return 0;
}

CPhysicsEngine::CollisionMask CPhysicsEngine::GetCollisionGroupMask( const TDynArray< CName >& names )
{
	CollisionMask result = 0;
	for ( Uint32 i = 0; i != names.Size(); ++i )
	{
		result |= GetCollisionGroupMask( names[ i ] );
	}
	return result;
}

CPhysicsEngine::CollisionMask CPhysicsEngine::GetCollisionGroupMask( const CollisionMask& mask )
{
	CollisionMask collisionMask = 0;
	for ( Uint64 i = 0; i < m_collisionTypeNames.Size(); ++i )
	{
		const Uint64 isSet = ( ( mask & ( 1LL << i ) ) >> i );
		collisionMask |= ( m_collisionSetMasks[ i ] * isSet );
	}
	return collisionMask;
}

const String CPhysicsEngine::GetCollisionMaskFileResourceName() const
{
	return resCollisionTypes.GetPath().ToString();
}

const String CPhysicsEngine::GetPhysicalMaterialFileResourceName() const
{
	return resPhysicalMaterials.GetPath().ToString();
}

const String CPhysicsEngine::GetTerrainPhysicalMaterialFileResourceName() const
{
	return resTexturesPhysicalMaterials.GetPath().ToString();
}

const SPhysicalMaterial* CPhysicsEngine::GetMaterial( const CName& name )
{
	for( Uint32 i = 0; i != m_physicalMaterials.Size(); ++i )
	{
		SPhysicalMaterial& material = m_physicalMaterials[ i ];
		if( material.m_name == name )
		{
			return &material;
		}
	}
	return 0;
}

const SPhysicalMaterial* CPhysicsEngine::GetMaterial( Uint32 index )
{
	if( index >= m_physicalMaterials.Size() ) return 0;
	return &m_physicalMaterials[ index ];
}


const SPhysicalMaterial* CPhysicsEngine::GetTexturePhysicalMaterial( const CName& texture )
{
	unsigned char* index = m_texturesPhysicalMaterials.FindPtr( texture );
	if( !index ) return 0;
	if( *index >= m_physicalMaterials.Size() ) return 0;
	return &( m_physicalMaterials[ *index ] );
}

char CPhysicsEngine::GetTexturePhysicalMaterialIndex( const CName& texture )
{
	unsigned char* index = m_texturesPhysicalMaterials.FindPtr( texture );
	return index ? *index : -1;
}

void CPhysicsEngine::GenerateEditorFragments( CRenderFrame* frame )
{
}

