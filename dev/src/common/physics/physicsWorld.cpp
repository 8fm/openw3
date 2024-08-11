#include "build.h"
#include "..\physics\physicsWrapper.h"
#include "physicsWorld.h"
#include "physicsSettings.h"
#include "physicsContactListener.h"
#include "..\core\object.h"
#include "..\core\scriptable.h"

CPhysicsWorld* CPhysicsWorld::m_top = 0;

CPhysicsWorld::CPhysicsWorld( IPhysicsWorldParentProvider* parentProvider, Uint32 areaNumTilesPerEdge, Float areaSize, const Vector2& areaCornerPosition, Uint32 clipSize, Uint32 areaRes ) 
	: m_upVector( 0.0f, 0.0f, 1.0f )
	, m_gravityVector( 0.0f, 0.0f, -9.81f )
	, m_worldParentProvider( parentProvider )
	, m_currentDelta( 0.0f )
	, m_deltaAcumulator( 0.0f )
	, m_deltaAcumulatorFrameCount( 0 )
	, m_areaNumTilesPerEdge( areaNumTilesPerEdge )
	, m_areaSize( areaSize )
	, m_area( nullptr )
	, m_movementLocked( false )
	, m_positionValid( false )
	, m_viewValid( false )
	, m_position( Vector::ZEROS )
	, m_viewMatrix( Matrix::IDENTITY )
	, m_stuffAdded( false )
	, m_areaCornerPosition( areaCornerPosition )
	, m_clipMapSize( clipSize )
	, m_areaRes( areaRes )
{
	memset( m_wrapperPools, 0, sizeof( m_wrapperPools ) );
	m_next = m_top;
	m_top = this;

	if( m_areaNumTilesPerEdge )
	{
		m_area = reinterpret_cast< Uint64* >( RED_MEMORY_ALLOCATE( MemoryPool_Physics, MC_PhysicsEngine, sizeof( Uint64 ) * m_areaNumTilesPerEdge * m_areaNumTilesPerEdge ) );
		Red::MemoryZero( m_area, sizeof( Uint64 ) * m_areaNumTilesPerEdge * m_areaNumTilesPerEdge );
		size_t test = sizeof( Red::Threads::CAtomic< class CPhysicsTileWrapper* > );
		m_tilesCache = reinterpret_cast< Red::Threads::CAtomic< class CPhysicsTileWrapper* >* >( RED_MEMORY_ALLOCATE( MemoryPool_Physics, MC_PhysicsEngine, sizeof( Red::Threads::CAtomic< class CPhysicsTileWrapper* > ) * m_areaNumTilesPerEdge * m_areaNumTilesPerEdge ) );
	}
}

CPhysicsWorld::~CPhysicsWorld()
{
	for( Uint16 i = 0; i != EPW_COUNT; i += EPW_INTERVAL )
	{
		TWrappersPool< CPhysicsWrapperInterface, SWrapperContext >* pool = ( TWrappersPool< CPhysicsWrapperInterface, SWrapperContext >* )&m_wrapperPools[ i ];
		pool->~TWrappersPool< CPhysicsWrapperInterface, SWrapperContext >();
	}

	if( m_area ) RED_MEMORY_FREE( MemoryPool_Physics, MC_PhysicsEngine, m_area );
	if( m_tilesCache ) RED_MEMORY_FREE( MemoryPool_Physics, MC_PhysicsEngine, m_tilesCache );

	for( CPhysicsContactListener* listener : m_contactListeners )
	{
		delete listener;
	}
	m_contactListeners.Clear();
}

void CPhysicsWorld::AddRef()
{
	m_ref.Increment();
}

void CPhysicsWorld::TickRemovalWrappers( bool force, TDynArray< void* >* toRemove )
{
	for( Uint16 i = 0; i != EPW_COUNT; i += EPW_INTERVAL )
	{
		TWrappersPool< CPhysicsWrapperInterface, SWrapperContext >* pool = ( TWrappersPool< CPhysicsWrapperInterface, SWrapperContext >* )&m_wrapperPools[ i ];
		bool isAddRemoveLimited = pool->GetFlag0();
		TickToRemove< CPhysicsWrapperInterface, SWrapperContext, TDynArray< void* > >( pool, toRemove, !isAddRemoveLimited || force ? INT_MAX : 1 );
	}
}

void CPhysicsWorld::PreSimulateWrappers( Float timeDelta, Bool blankFrame, TDynArray< void* >* toAdd, TDynArray< void* >* toRemove, TDynArray< void* >* toRelease )
{

	for( Uint16 i = 0; i != EPW_COUNT; i += EPW_INTERVAL )
	{
		TWrappersPool< CPhysicsWrapperInterface, SWrapperContext >* pool = ( TWrappersPool< CPhysicsWrapperInterface, SWrapperContext >* )&m_wrapperPools[ i ];
		Uint32 test = sizeof( TWrappersPool< CPhysicsWrapperInterface, SWrapperContext > );
		TickContext( pool, m_position, timeDelta, m_worldParentProvider );
	}

	Uint64 tickMarker = 0;
	if( !blankFrame )
	{
		tickMarker = Red::System::Clock::GetInstance().GetTimer().GetTicks();
	}

	for( Uint16 i = 0; i != EPW_COUNT; i += EPW_INTERVAL )
	{
		TWrappersPool< CPhysicsWrapperInterface, SWrapperContext >* pool = ( TWrappersPool< CPhysicsWrapperInterface, SWrapperContext >* )&m_wrapperPools[ i ];
		Bool notflushing = pool->GetWrappersRemoveQue().Empty();
		TickPreSimulation< CPhysicsWrapperInterface, SWrapperContext, TDynArray< void* > >( pool, m_position, timeDelta, tickMarker, toAdd, toRemove, toRelease, notflushing );
	}

	if( !m_area ) return;

	m_invalidArea.CacheCurrentData();

	Red::MemoryZero( m_area, sizeof( Uint64 ) * m_areaNumTilesPerEdge * m_areaNumTilesPerEdge );

	if ( m_movementLocked ) return;

	Vector2 pos = m_position;
	float halfAreaSize = m_areaSize;
	halfAreaSize *= 0.5f;
	pos.X += halfAreaSize;
	pos.Y += halfAreaSize;

	float areaSectorSize = m_areaSize;
	areaSectorSize /= m_areaNumTilesPerEdge;

	float distanceLimit = SPhysicsSettings::m_staticBodiesDistanceLimit;

	Int32 xMin = Int32( ( pos.X - distanceLimit ) / areaSectorSize );
	Int32 yMin = Int32( ( pos.Y - distanceLimit ) / areaSectorSize );
	Int32 xMax = Int32( ( pos.X + distanceLimit ) / areaSectorSize );
	Int32 yMax = Int32( ( pos.Y + distanceLimit ) / areaSectorSize );

	if( SPhysicsSettings::m_staticBodiesDistanceLimit == 0.0f )
	{
		for( Int32 y = yMin; y != yMax + 1; ++y )
			for( Int32 x = xMin; x != xMax + 1; ++x )
			{
				Uint64& areaSector = m_area[ x * m_areaNumTilesPerEdge + y ];
				areaSector = 0xFFFFFFFFFFFFFFFF;
			}
		return;
	}

	if( xMin < 0 ) xMin = 0;
	if( yMin < 0 ) yMin = 0;
	if( xMax < 0 ) xMax = 0;
	if( yMax < 0 ) yMax = 0;

	if( xMin >= ( Int32 ) m_areaNumTilesPerEdge ) xMin = m_areaNumTilesPerEdge;
	if( yMin >= ( Int32 ) m_areaNumTilesPerEdge ) yMin = m_areaNumTilesPerEdge;
	if( xMax >= ( Int32 ) m_areaNumTilesPerEdge ) xMax = m_areaNumTilesPerEdge;
	if( yMax >= ( Int32 ) m_areaNumTilesPerEdge ) yMax = m_areaNumTilesPerEdge;

	float distanceLimitSquared = distanceLimit;
	distanceLimitSquared *= distanceLimitSquared;

	for( Int32 y = yMin; y != yMax + 1; ++y )
		for( Int32 x = xMin; x != xMax + 1; ++x )
		{
			Uint64& areaSector = m_area[ y * m_areaNumTilesPerEdge + x ];

			Vector2 areaSectorPos( -halfAreaSize, -halfAreaSize );
			areaSectorPos.X += areaSectorSize * x;
			areaSectorPos.Y += areaSectorSize * y;

			float sectorSize = areaSectorSize / 8;
			for( Uint8 yy = 0; yy != 8 ; ++yy )
				for( Uint8 xx = 0; xx != 8; ++xx )
				{
					Float position = sectorSize * ( xx + 0.5f );
					position += areaSectorPos.X;
					position -= m_position.X;
					float distance = position * position;
					position = sectorSize * ( yy + 0.5f );
					position += areaSectorPos.Y;
					position -= m_position.Y;
					distance += position * position;

					if( distance > distanceLimitSquared ) continue;

					areaSector |= 0x1LL << ( yy * 8 + xx );
				}

		}

}

void CPhysicsWorld::PostSimulateWrappers()
{
	for( Uint16 i = 0; i != EPW_COUNT; i += EPW_INTERVAL )
	{
		TWrappersPool< CPhysicsWrapperInterface, SWrapperContext >* pool = ( TWrappersPool< CPhysicsWrapperInterface, SWrapperContext >* )&m_wrapperPools[ i ];
		if( !pool->GetFlag1() ) continue;;

		TickPostSimulation( pool );

	}
}

Bool CPhysicsWorld::HasWrappers()
{
	for( Uint16 i = 0; i != EPW_COUNT; i += EPW_INTERVAL )
	{
		TWrappersPool< CPhysicsWrapperInterface, SWrapperContext >* pool = ( TWrappersPool< CPhysicsWrapperInterface, SWrapperContext >* )&m_wrapperPools[ i ];
		if( !pool->IsEmpty() ) return true;
	}
	return false;
}

CPhysicsWorld* CPhysicsWorld::ReleaseRef()
{
	m_ref.Decrement();
	if( m_ref.GetValue() != 0 )
		return this;

	return 0;
}

void CPhysicsWorld::SendReceivedScriptEvents()
{
	PC_SCOPE_PHYSICS( Physics scene SendReceivedScriptEvents );

	Uint32 count = m_receivedEvents.Size();
	for( Uint32 i = 0; i != count; ++i )
	{
		SObjectHandleNameComponentHandle& eventData = m_receivedEvents[ i ];
		eventData.m_eventReceiverObject.Get()->CallEvent( eventData.m_onEventName, eventData.m_otherObject, eventData.m_index.m_actorIndex, eventData.m_index.m_shapeIndex );
	}
	m_receivedEvents.ClearFast();
}

Bool CPhysicsWorld::IsAvaible( const Vector& pos, Bool checkInvalidAreas )
{
	PC_SCOPE_PHYSICS( IsAvaible );
	if( !m_area ) return false;
	if( pos.X < -m_areaSize * 0.5f || pos.Y < -m_areaSize * 0.5f || pos.X > m_areaSize * 0.5f || pos.Y > m_areaSize * 0.5f ) return false;

	float areaSectorSize = m_areaSize / m_areaNumTilesPerEdge;

	float xx = pos.X + m_areaSize * 0.5f;
	float yy = pos.Y + m_areaSize * 0.5f;

	Uint32 x = Uint32( xx / areaSectorSize );
	Uint32 y = Uint32( yy / areaSectorSize );

	const Uint64& areaSector = m_area[ y * m_areaNumTilesPerEdge + x ];
	if( !areaSector ) return false;

	if( checkInvalidAreas )
	{
		PC_SCOPE_PHYSICS( invalid areas );
		if ( m_invalidArea.TestPoint_Cached( pos ) )
		{
			return false;
		}
	}

	float div = areaSectorSize / 8;
	Vector2 sectorCorner = Vector2( -m_areaSize * 0.5f + areaSectorSize * x, -m_areaSize * 0.5f + areaSectorSize * y );

	x = ( Uint32 ) ( ( pos.X - sectorCorner.X ) / div ); 
	y = ( Uint32 ) ( ( pos.Y - sectorCorner.Y ) / div );

	Uint8 counter = x + y * 8;
	Uint64 bit = 0x1LL << counter;
	return ( areaSector & bit ) != 0;
}

Bool CPhysicsWorld::IsPositionInside( const Vector2& pos )
{
	if( !m_tilesCache ) return false;
	return !( pos.X < -m_areaSize * 0.5f || pos.Y < -m_areaSize * 0.5f || pos.X > m_areaSize * 0.5f || pos.Y > m_areaSize * 0.5f );
}

Red::Threads::CAtomic< class CPhysicsTileWrapper* >* CPhysicsWorld::GetTerrainTileWrapperAtomic( const Vector2& pos )
{
	if( !m_tilesCache ) return nullptr;
	if( pos.X < -m_areaSize * 0.5f || pos.Y < -m_areaSize * 0.5f || pos.X > m_areaSize * 0.5f || pos.Y > m_areaSize * 0.5f ) return nullptr;

	//copy pasted from clip map...
	Vector2 areaSpacePosition( pos.X - m_areaCornerPosition.X, pos.Y - m_areaCornerPosition.Y );

	areaSpacePosition /= m_areaSize;

	Int32 clipCenterLevel0SpaceCol = (Int32)( areaSpacePosition.X * ( m_clipMapSize - 1 ) );
	Int32 clipCenterLevel0SpaceRow = (Int32)( areaSpacePosition.Y * ( m_clipMapSize - 1 ) );

	Uint32 tileX = clipCenterLevel0SpaceCol >= 0 ? clipCenterLevel0SpaceCol / m_areaRes : -1;
	Uint32 tileY = clipCenterLevel0SpaceRow >= 0 ? clipCenterLevel0SpaceRow / m_areaRes : -1;

	return &m_tilesCache[ tileY * m_areaNumTilesPerEdge + tileX ];
	//copy pasted from clip map...
}

class CPhysicsTileWrapper* CPhysicsWorld::GetTerrainTileWrapper( const Vector2& pos )
{
	Red::Threads::CAtomic< class CPhysicsTileWrapper* >* atomic = GetTerrainTileWrapperAtomic( pos );
	if( !atomic ) return nullptr;
	return atomic->GetValue();
}

Bool CPhysicsWorld::MarkSectorAsUnready( const Vector& pos )
{
	if( !m_area ) return false;
	if( pos.X < -m_areaSize * 0.5f || pos.Y < -m_areaSize * 0.5f || pos.X > m_areaSize * 0.5f || pos.Y > m_areaSize * 0.5f ) return false;

	float areaSectorSize = m_areaSize / m_areaNumTilesPerEdge;

	float xx = pos.X + m_areaSize * 0.5f;
	float yy = pos.Y + m_areaSize * 0.5f;

	Uint32 x = Uint32( xx / areaSectorSize );
	Uint32 y = Uint32( yy / areaSectorSize );

	Uint64& areaSector = m_area[ y * m_areaNumTilesPerEdge + x ];
	areaSector = 0;

	return true;
}

void CPhysicsWorld::ToggleMovementLock( const Bool isLocked )
{
	if ( m_movementLocked != isLocked )
	{
		m_movementLocked = isLocked;

		// disallow all movement now
		if ( isLocked && m_area )
		{
			Red::MemoryZero( m_area, sizeof( Uint64 ) * m_areaNumTilesPerEdge * m_areaNumTilesPerEdge );
		}
	}
}

void CPhysicsWorld::SetReferencePosition( const Vector& position )
{
	m_positionValid = true;
	m_position = position;
}

void CPhysicsWorld::SetRenderingView( const Matrix& viewMatrix )
{
	m_viewValid = true;
	m_viewMatrix = viewMatrix;
}
