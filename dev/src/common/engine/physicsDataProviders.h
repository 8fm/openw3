/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef PHYSICS_DATA_PROVIDERS_H
#define PHYSICS_DATA_PROVIDERS_H

#include "..\physics\physicsWrapper.h"
#include "..\physics\physicsWorld.h"

class CPhysicsWrapperParentComponentProvider : public IPhysicsWrapperParentProvider
{
	THandle< CComponent > m_parent;

public:
	CPhysicsWrapperParentComponentProvider( const THandle< CComponent > parent ) : m_parent( parent ) {}

	void operator= (const IPhysicsWrapperParentProvider& other);

	IScriptable* GetParentObject() const
	{
		return m_parent.Get();
	}

	class CPhysicsWorld* GetPhysicsWorld() const;

	virtual Bool HasParent() const
	{
		return m_parent.Get() != nullptr;
	}

	virtual const Matrix& GetLocalToWorld() const
	{
		if( CComponent* component = m_parent.Get() )
		{
			return component->GetLocalToWorld();
		}
		else return Matrix::ZEROS;
	}

	virtual const EngineTransform& GetTransform() const
	{
		if( CComponent* component = m_parent.Get() )
		{
			return component->GetTransform();
		}
		else return EngineTransform::ZEROS;
	}

	virtual Bool isRoot()
	{
		Uint32 count = ForEachSumResult< Uint32 >(
			m_parent.Get()->GetEntity()->GetComponents(),
			[]( CComponent* component )-> int
		{
			CPhysicsWrapperInterface* wrapper = component->GetPhysicsRigidBodyWrapper();
			return ( wrapper && component->IsRootComponent() ) ? 1 : 0;
		}
		);
		return count == 1;
	}

	virtual Uint32 GetVisibilityQuerty()
	{
		if( CComponent* component = m_parent.Get() )
		{
			if( CEntity* parentEntity = component->GetEntity() )
			{
				return parentEntity->GetVisibilityQuery();
			}
		}
		return 0;
	}

	virtual const String GetFriendlyName()
	{
		if( CComponent* component = m_parent.Get() )
		{
			if( CEntity* entity = component->GetEntity() )
			{
				return entity->GetFriendlyName();
			}
		}
		return String::EMPTY;
	}

	Bool SetRawPlacementNoScale( const Matrix& pose )
	{
		if( CComponent* component = m_parent.Get() )
		{
			if( CEntity* entity = component->GetEntity() )
			{
				entity->SetRawPlacementNoScale( pose );
				return true;
			}
		}
		return false;
	}

	Bool ForceUpdateTransformWithGlobalPose( const Matrix& pose )
	{
		if( CComponent* component = m_parent.Get() )
		{
			component->HACK_ForceUpdateTransformWithGlobalPose( pose );
			return true;
		}
		return false;
	}

	void DataHalt( IScriptable* parent, const Char* category, const Char* reason );

};


class CPhysicsWrapperParentResourceProvider : public IPhysicsWrapperParentProvider
{
	THandle< CResource > m_parent;

public:
	CPhysicsWrapperParentResourceProvider( const THandle< CResource > parent ) : m_parent( parent ) {}

	void operator= (const IPhysicsWrapperParentProvider& other);

	IScriptable* GetParentObject() const
	{
		return m_parent.Get();
	}

	class CPhysicsWorld* GetPhysicsWorld() const
	{
		return nullptr;
	}

	virtual Bool HasParent() const
	{
		return m_parent.Get() != nullptr;
	}

	virtual const Matrix& GetLocalToWorld() const
	{
		return Matrix::ZEROS;
	}

	virtual const EngineTransform& GetTransform() const
	{
		return EngineTransform::ZEROS;
	}

	virtual Bool isRoot()
	{
		return true;
	}

	virtual Uint32 GetVisibilityQuerty()
	{
		return 0;
	}

	virtual const String GetFriendlyName()
	{
		return String::EMPTY;
	}

	Bool SetRawPlacementNoScale( const Matrix& pose )
	{
		return false;
	}

	Bool ForceUpdateTransformWithGlobalPose( const Matrix& pose )
	{
		return false;
	}

	void DataHalt( IScriptable* parent, const Char* category, const Char* reason );

};


class CPhysicsWorldParentEngineWorldProvider : public IPhysicsWorldParentProvider
{
	THandle< CWorld > m_world;

public:
	CPhysicsWorldParentEngineWorldProvider( CWorld* world ) : m_world( world ) {}

	const Vector& GetCameraPosition() const
	{
		if( CWorld* world = m_world.Get() )
		{
			return world->GetCameraPosition();
		}
		return Vector::ZEROS;
	}

	const Vector& GetCameraForward() const
	{
		if( CWorld* world = m_world.Get() )
		{
			return world->GetCameraForward();
		}
		return Vector::ZEROS;
	}

	const Vector& GetCameraUp() const
	{
		if( CWorld* world = m_world.Get() )
		{
			return world->GetCameraUp();
		}
		return Vector::ZEROS;
	}

	Vector GetWindAtPoint( const Vector& point )
	{
		if( CWorld* world = m_world.Get() )
		{
			return world->GetWindAtPointForVisuals( point, false );
		}
		return Vector::ZEROS;
	}

	void GetWindAtPoint( Uint32 elementsCount, void* inputPos, void* outputPos, size_t stride ) const
	{
		if( CWorld* world = m_world.Get() )
		{
			world->GetWindAtPoint( elementsCount, inputPos, outputPos, stride );
		}
	}

	Bool IsWaterAvailable() const
	{
		if( CWorld* world = m_world.Get() )
		{
			return world->IsWaterShaderEnabled();
		}
		return false;
	}

	Float GetWaterLevel( const Vector &point, Uint32 lodApproximation, Float* heightDepth ) const
	{
		if( CWorld* world = m_world.Get() )
		{
			return world->GetWaterLevel( point, lodApproximation, heightDepth );
		}
		return 0.0f;
	}

	Bool GetWaterLevelBurst( Uint32 elementsCount, void* inputPos, void* outputPos, void* outputHeightDepth, size_t stride, Vector referencePosition, Bool useApproximation ) const
	{
		if( CWorld* world = m_world.Get() )
		{
			return world->GetWaterLevelBurst( elementsCount, inputPos, outputPos, outputHeightDepth, stride, referencePosition, useApproximation );
		}
		return 0.0f;

	}

	Bool GetVisibilityQueryResult( Uint32 elementsCount, Int16* indexes, void* inputPos, void* outputPos, size_t stride ) const
	{
		if( CWorld* world = m_world.Get() )
		{
			return world->GetRenderSceneEx()->GetVisibilityQueryResult( elementsCount, indexes, inputPos, outputPos, stride );
		}
		return false;
	}

};
void PhysicsGenerateEditorFragments( CRenderFrame* frame, CWorld* activeWorld );

#endif