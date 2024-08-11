/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "entityHandle.h"
#include "../core/scriptStackFrame.h"
#include "game.h"
#include "persistentEntity.h"
#include "entity.h"
#include "layer.h"
#include "world.h"


////////////////////////////////////////////////////////////////////////////////////

// TODO: Add ref counting to IEntityHandleData

////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SIMPLE_RTTI_TYPE( EntityHandle );

////////////////////////////////////////////////////////////////////////////////////

/// Entity handle data type
enum EEntityHandleDataType
{
	EHDT_None=0,
	EHDT_Static,			// By layer::entity GUID pair
	EHDT_Persistent,		// By IdTag
	EHDT_Player,			// Always GGame->GetPlayerEntity()
};

/// Entity handle data interface
class IEntityHandleData
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Engine );

public:
	virtual ~IEntityHandleData() {}
	virtual EEntityHandleDataType GetType() const = 0;
	virtual IEntityHandleData* Clone() const = 0;
	virtual CEntity* GetEntity() = 0;
	virtual void Serialize( IFile& file ) = 0;
	virtual Bool Compare( IEntityHandleData* data ) const = 0;
	virtual Bool Less( IEntityHandleData* data ) const = 0;
	virtual String ToString() = 0;
	virtual const CGUID& GetEntityHandleGUID() const { return CGUID::ZERO; }
	virtual const IdTag& GetEntityHandleIdTag() const { return IdTag::Empty(); }
};

////////////////////////////////////////////////////////////////////////////////////

/// Static entity data handle
class CStaticEntityHandleData : public IEntityHandleData
{
private:
	THandle< CEntity >		m_entity;			//!< Resolved entity
	CGUID					m_guidEntity;		//!< Saved entity GUID

public:
	//! Get type of handle
	virtual EEntityHandleDataType GetType() const
	{
		return EHDT_Static;
	}

	virtual const CGUID& GetEntityHandleGUID() const
	{
		return m_guidEntity;
	}

	//! Resolve entity
	virtual CEntity* GetEntity()
	{
		// Already cached?

		if ( CEntity* entity = m_entity.Get() )
		{
			return entity;
		}

		// Find and cache the entity

		if ( CWorld* world = GGame->GetActiveWorld() )
		{
			if ( CEntity* entity = world->FindEntity( m_guidEntity ) )
			{
				m_entity = entity;
				return entity;
			}
		}

		// Not found

		return nullptr;
	}

	//! Serialize handle data crap
	void Serialize( IFile& file )
	{
		file << m_guidEntity;

		CGUID layerGUID;
		file << layerGUID;
	}

	//! Clone this data
	virtual IEntityHandleData* Clone() const
	{
		CStaticEntityHandleData* data = new CStaticEntityHandleData;
		data->m_entity = m_entity;
		data->m_guidEntity = m_guidEntity;
		return data;
	}

	//! Convert to string
	virtual String ToString()
	{
		if ( CEntity* entity = GetEntity() )
		{
			if ( entity->GetLayer() )
			{
				return String::Printf( TXT("%s on %s"), entity->GetName().AsChar(), entity->GetLayer()->GetDepotPath().AsChar() );
			}
			else
			{
				return String::Printf( TXT("%s on Unknown Layer"), entity->GetName().AsChar() );
			}
		}

		// Entity not found

		Char entityGuidStr[ RED_GUID_STRING_BUFFER_SIZE ];
		m_guidEntity.ToString( entityGuidStr, RED_GUID_STRING_BUFFER_SIZE );

		return String::Printf( TXT("Entity '%ls'"), entityGuidStr );
	}

	//! Compare data
	virtual Bool Compare( IEntityHandleData* data ) const
	{
		CStaticEntityHandleData* localData = static_cast< CStaticEntityHandleData* >( data );
		return m_guidEntity == localData->m_guidEntity;
	}

	//! Compare data
	virtual Bool Less( IEntityHandleData* data ) const
	{
		CStaticEntityHandleData* localData = static_cast< CStaticEntityHandleData* >( data );
		return m_guidEntity < localData->m_guidEntity;
	}

public:
	//! Create static handle from entity
	static CStaticEntityHandleData* Create( CEntity* entity )
	{
		// Entity is not valid or is not on persistent layer
		if ( !entity || !entity->GetLayer() || !entity->GetLayer()->GetLayerInfo() )
		{
			return NULL;
		}

		// Entity has no GUID ? WTF ?
		if ( entity->GetGUID().IsZero() )
		{
			WARN_ENGINE( TXT("Trying to create static entity handle data from entity '%ls' that has no GUID. Please resave the layer."), entity->GetFriendlyName().AsChar() );
			return NULL;
		}

		// Layer has no GUID ? WTF ?
		if ( entity->GetLayer()->GetGUID().IsZero() )
		{
			WARN_ENGINE( TXT("Trying to create static entity handle data from entity '%ls' that has no layer GUID. Please resave the layer."), entity->GetFriendlyName().AsChar() );
			return NULL;
		}

		// Store data
		CStaticEntityHandleData* data = new CStaticEntityHandleData;
		data->m_entity = entity;
		data->m_guidEntity = entity->GetGUID();
		return data;
	}
};

////////////////////////////////////////////////////////////////////////////////////

/// Persistent entity data handle
class CPersistentEntityHandleData : public IEntityHandleData
{
private:
	THandle< CEntity >		m_entity;			//!< Resolved entity
	IdTag					m_idTag;			//!< Resolved tag

public:
	//! Get type of handle
	virtual EEntityHandleDataType GetType() const
	{
		return EHDT_Persistent;
	}

	virtual const IdTag& GetEntityHandleIdTag() const
	{
		return m_idTag;
	}

	//! Resolve entity
	virtual CEntity* GetEntity()
	{
		// We have entity
		CEntity* entity = m_entity.Get();
		if ( !entity )
		{
			// Find in persistent entities list
			entity = CPeristentEntity::FindByIdTag( m_idTag );
			if ( entity )
			{
				// Remember
				m_entity = entity;
			}
		}

		// Return resolved entity
		return entity;
	}

	//! Serialize handle data crap
	void Serialize( IFile& file )
	{
		file << m_idTag;
	}

	//! Clone this data
	virtual IEntityHandleData* Clone() const
	{
		CPersistentEntityHandleData* data = new CPersistentEntityHandleData;
		data->m_entity = m_entity;
		data->m_idTag = m_idTag;
		return data;
	}

	//! Compare data
	virtual Bool Compare( IEntityHandleData* data ) const
	{
		CPersistentEntityHandleData* localData = static_cast< CPersistentEntityHandleData* >( data );
		return ( m_idTag == localData->m_idTag );
	}

	//! Compare data
	virtual Bool Less( IEntityHandleData* data ) const
	{
		CPersistentEntityHandleData* localData = static_cast< CPersistentEntityHandleData* >( data );
		return ( m_idTag < localData->m_idTag );
	}

	//! Convert to string
	virtual String ToString()
	{
		// Try to get from entity
		CEntity* entity = GetEntity();
		if ( entity )
		{
			if ( entity->GetLayer() )
			{
				return String::Printf( TXT("%s on %s"), entity->GetName().AsChar(), entity->GetLayer()->GetDepotPath().AsChar() );
			}
			else
			{
				return String::Printf( TXT("%s on Unknown Layer"), entity->GetName().AsChar() );
			}
		}

		// Show IdTag
		String tagString;
		m_idTag.ToString( tagString );
		return String::Printf( TXT("IdTag '%ls'"), tagString.AsChar() );
	}

public:
	//! Create static handle from entity
	static CPersistentEntityHandleData* Create( CEntity* entity )
	{
		// Entity is not valid or is not on persistent layer
		CPeristentEntity* pEntity = Cast< CPeristentEntity >( entity );
		if ( !pEntity || !pEntity->GetIdTag().IsValid() )
		{
			return NULL;
		}

		// Store data
		CPersistentEntityHandleData* data = new CPersistentEntityHandleData;
		data->m_entity = pEntity;
		data->m_idTag = pEntity->GetIdTag();
		return data;
	}
};

////////////////////////////////////////////////////////////////////////////////////

/// Player  entity data handle
class CPlayerEntityHandleData : public IEntityHandleData
{
public:
	//! Get type of handle
	virtual EEntityHandleDataType GetType() const
	{
		return EHDT_Player;
	}

	//! Resolve entity
	virtual CEntity* GetEntity()
	{
		return GGame->GetPlayerEntity();
	}

	//! Serialize handle data crap
	void Serialize( IFile& file )
	{
	}

	//! Clone this data
	virtual IEntityHandleData* Clone() const
	{
		return new CPlayerEntityHandleData;
	}

	//! Compare data
	virtual Bool Compare( IEntityHandleData* data ) const
	{
		return true;
	}

	//! Compare data
	virtual Bool Less( IEntityHandleData* data ) const
	{
		return false;
	}

	//! Convert to string
	virtual String ToString()
	{
		return TXT("Player");
	}

public:
	//! Create static handle from entity
	static CPlayerEntityHandleData* Create( CEntity* entity )
	{
		// This works only for the player
		if ( entity != GGame->GetPlayerEntity() )
		{
			return NULL;
		}

		// Create data
		return new CPlayerEntityHandleData;
	}
};

////////////////////////////////////////////////////////////////////////////////////

EntityHandle::~EntityHandle()
{
	// Destroy handle data
	if ( m_data )
	{
		delete m_data;
		m_data = NULL;
	}
}

EntityHandle::EntityHandle( const EntityHandle& other )
	: m_data( NULL )
{
	// Copy source data
	if ( other.m_data )
	{
		m_data = other.m_data->Clone();
	}
}

Bool EntityHandle::operator==( const EntityHandle& other ) const
{
	// Both data should exist
	if ( m_data && other.m_data )
	{
		// Types should match
		if ( m_data->GetType() == other.m_data->GetType() )
		{
			// Compare data directly
			return m_data->Compare( other.m_data );
		}
	}

	// Equal
	if ( !m_data && !other.m_data )
	{
		return true;
	}

	// Not the same
	return false;
}

Bool EntityHandle::operator<( const EntityHandle& other ) const
{
	// Both data should exist
	if ( m_data && other.m_data )		
	{
		// Types should match
		if ( m_data->GetType() == other.m_data->GetType() )
		{
			// Compare data directly
			return m_data->Less( other.m_data );
		}
		else
		{
			return m_data->GetType() < other.m_data->GetType();
		}
	}
	else if ( !m_data && other.m_data )
	{
		return true;
	}
	else
	{
		return false;
	}
}

EntityHandle& EntityHandle::operator=( const EntityHandle& other )
{
	// Do not assign to self
	if ( this != &other )
	{
		// Destroy current data
		if ( m_data )
		{
			delete m_data;
			m_data = NULL;
		}

		// Clone source
		if ( other.m_data )
		{
			m_data = other.m_data->Clone();
		}
	}

	// Fallthrough
	return *this;
}

void EntityHandle::Serialize( IFile& file )
{
	// We do not need to serialize for this
	if ( file.IsMapper() || file.IsGarbageCollector() )
	{
		return;
	}

	// Write crap
	if ( file.IsWriter() )
	{
		if ( m_data )
		{
			// Save data type
			Uint8 type = (Uint8)m_data->GetType();
			file << type;

			// Save data
			m_data->Serialize( file );		
		}
		else
		{
			// No data 
			Uint8 type = 0;
			file << type;
		}
	}

	// Read crap
	if ( file.IsReader() )
	{
		// Load data type
		Uint8 type = 0;
		file << type;

		// Destroy current data
		if ( type == EHDT_Static )
		{
			m_data = new CStaticEntityHandleData();
			m_data->Serialize( file );
		}
		else if ( type == EHDT_Persistent )
		{
			m_data = new CPersistentEntityHandleData();
			m_data->Serialize( file );
		}
		else if ( type == EHDT_Player )
		{
			m_data = new CPlayerEntityHandleData();
			m_data->Serialize( file );
		}
	}
}

void EntityHandle::Set( CEntity* entity )
{
	// Resolve entity
	CEntity* current = Get();
	if ( current != entity || ( !entity && m_data ) )
	{
		// Destroy current data
		if ( m_data )
		{
			delete m_data;
			m_data = NULL;
		}

		// Create handle data		
		if ( entity )
		{
			// Try the player first
			m_data = CPlayerEntityHandleData::Create( entity );
			if ( !m_data )
			{
				// Try IdTagged entities 
				m_data = CPersistentEntityHandleData::Create( entity );
				if ( !m_data )
				{
					// As a last resort - try the static entities on layers
					m_data = CStaticEntityHandleData::Create( entity );
					if ( !m_data )
					{
						WARN_ENGINE( TXT("EntityHandle to '%ls' cannot be created."), entity->GetFriendlyName().AsChar() );
					}
				}
			}
		}
	}
}

CEntity* EntityHandle::Get()
{
	// Use handle data to resolve entity
	if ( m_data )
	{
		return m_data->GetEntity();
	}

	// Not resolved
	return NULL;
}

const CEntity* EntityHandle::Get() const
{
	// Use handle data to resolve entity
	if ( m_data )
	{
		return m_data->GetEntity();
	}

	// Not resolved
	return NULL;
}

String EntityHandle::ToString() const
{
	// Use handle data to resolve entity
	if ( m_data )
	{
		return m_data->ToString();
	}

	// Empty
	return TXT("None");
}

const CGUID& EntityHandle::GetEntityHandleGUID() const
{
	return m_data ? m_data->GetEntityHandleGUID() : CGUID::ZERO;
}

const IdTag& EntityHandle::GetEntityHandleIdTag() const
{
	return m_data ? m_data->GetEntityHandleIdTag() : IdTag::Empty();
}

////////////////////////////////////////////////////////////////////////////////////
// Functions for script snapshot
void EntityHandleDataGetObjectHandle( const void *handleData, THandle< CObject >& objectHandle )
{
	ASSERT( handleData );
	EntityHandle* handle = ( EntityHandle * ) handleData;	
	objectHandle = handle->Get();
}

void EntityHandleDataSetObjectHandle( void *handleData, THandle< CObject >& objectHandle )
{
	ASSERT( handleData );
	EntityHandle* handle = ( EntityHandle * ) handleData;	
	handle->Set( SafeCast< CEntity >( objectHandle.Get() ) );
}

////////////////////////////////////////////////////////////////////////////////////

static void funcEntityHandleGet( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( EntityHandle, a, EntityHandle() );
	FINISH_PARAMETERS;
	RETURN_OBJECT( a.Get() );
};

static void funcEntityHandleSet( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( EntityHandle, a, EntityHandle() );
	GET_PARAMETER( THandle< CEntity >, b, NULL );
	FINISH_PARAMETERS;
	a.Set( b.Get() );
};

static void funcEntityHandleWaitGet( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( EntityHandle, a, EntityHandle() );
	GET_PARAMETER_OPT( Float, timeout, -1.0f );
	FINISH_PARAMETERS;

	// Yield if not waited long enough
	ASSERT( stack.m_thread );

	CEntity* entity = a.Get();

	if( entity )
	{
		RETURN_OBJECT( entity );
		return;
	}
	else
	{
		if( timeout >= 0.0f )
		{
			const Float waitedTime = stack.m_thread->GetCurrentLatentTime();
			if ( waitedTime >= timeout )
			{
				RETURN_OBJECT( NULL );
				return;
			}
		}

		stack.m_thread->ForceYield();
		return;
	}
};

void ExportEngineEntityHandleNatives()
{
	NATIVE_GLOBAL_FUNCTION( "EntityHandleGet",	funcEntityHandleGet );
	NATIVE_GLOBAL_FUNCTION( "EntityHandleSet",	funcEntityHandleSet );
	NATIVE_GLOBAL_FUNCTION( "EntityHandleWaitGet",	funcEntityHandleWaitGet );
}