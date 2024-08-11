/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlib.h"
#include "pathlibObstacleSpawnContext.h"

// HACK: IQueueMetalinkInterface is defined in game project because, its interconnected with AI, but also available only
// for Metalinks.
class IAIQueueMetalinkInterface;


namespace PathLib
{

class CAgent;
class CComponentRuntimeProcessingContext;
class IMetalinkComponent;
class CObstacleProcessingJob;
class IObstacleComponent;


////////////////////////////////////////////////////////////////////////////
// IComponent is an interface that engine components can implement to integrate smoothly with PathLib.
class IComponent
{
protected:
	void						Attach( CWorld* world );
	void						Detach( CWorld* world );
	void						Remove( CWorld* world );
	void						Update( CWorld* world );
	void						Enable( CWorld* world );
	void						Disable( CWorld* world );

	~IComponent()															{} // you can not delete IComponent directly
public:
	class SafePtr
	{
	private:
		IComponent*				m_component;
		THandle< CComponent >	m_engineObj;
	public:
		SafePtr()
			: m_component( nullptr )										{}
		SafePtr( const SafePtr& p )
			: m_component( p.m_component )
			, m_engineObj( p.m_engineObj )									{}
		SafePtr( SafePtr&& p )
			: m_component( p.m_component )
			, m_engineObj( Move( p.m_engineObj ) )							{}

		void operator=( IComponent* c );
		SafePtr& operator=( const SafePtr& ptr )							{ m_component = ptr.m_component; m_engineObj = ptr.m_engineObj; return *this; }
		SafePtr& operator=( SafePtr&& ptr )									{ m_component = ptr.m_component; m_engineObj = Move( ptr.m_engineObj ); return *this; }
		SafePtr( IComponent* c );
		
		IComponent*				Get() const									{ if ( m_engineObj.Get() ) return m_component; return nullptr; }
		CComponent*				GetEngineComponent() const					{ return m_engineObj.Get(); }
	};


	static IComponent*			Get( CWorld* world, const SComponentMapping& mapping );

	////////////////////////////////////////////////////////////////////////
	// This functions lets pathlib component implement 'what to do' when its component get created or attached.
	// NOTICE: Its not suitable for Detach or Destroy behavior, because we don't have guarantee that
	// component object will be valid at time of destruction.
	virtual Bool				RuntimeProcessing( CComponentRuntimeProcessingContext& context, const CProcessingEvent& e )																				= 0;
	virtual Bool				ToolAsyncProcessing( CObstaclesLayerAsyncProcessing* processingJob, const CProcessingEvent& e, IGenerationManagerBase::CAsyncTask::CSynchronousSection& section )			= 0;

	////////////////////////////////////////////////////////////////////////
	// Functionality for unified response for events.
	// It is response for problem with Detach/Destroy behavior descrived in
	// comments above.
	void						ProcessingEventAttach( CProcessingEvent& e );
	void						ProcessingEventUpdate( CProcessingEvent& e );
	virtual void				ProcessingEventDetach( CProcessingEvent& e );
	virtual void				ProcessingEventRemove( CProcessingEvent& e );

	////////////////////////////////////////////////////////////////////////
	virtual Bool				IsLayerBasedGrouping() const;
	virtual Bool				IsNoticableInGame( CProcessingEvent::EType eventType ) const;
	virtual Bool				IsNoticableInEditor( CProcessingEvent::EType eventType ) const;

	virtual void				OnPathLibReload( CWorld* world );

	virtual IObstacleComponent*	AsObstacleComponent();
	virtual IMetalinkComponent* AsMetalinkComponent();
	virtual CComponent*			AsEngineComponent()													= 0;
};

////////////////////////////////////////////////////////////////////////////
// Static mesh, destructable systems and denied areas and all that stuff
// that produce "obstacles".
class IObstacleComponent : public IComponent
{
	typedef IComponent Super;
protected:
	void						OnCollisionGroupUpdated( CWorld* world, EPathLibCollision newCollisionGroup );

	virtual void				SetPathLibCollisionGroupInternal( EPathLibCollision collisionGroup )= 0;
public:
	virtual EPathLibCollision	GetPathLibCollisionGroup() const									= 0;
	void						SetPathLibCollisionGroup( EPathLibCollision collisionGroup );

	Bool						RuntimeProcessing( CComponentRuntimeProcessingContext& context, const CProcessingEvent& e ) override;
	Bool						ToolAsyncProcessing( CObstaclesLayerAsyncProcessing* processingJob, const CProcessingEvent& e, IGenerationManagerBase::CAsyncTask::CSynchronousSection& section ) override;

	void						ProcessingEventDetach( CProcessingEvent& e ) override;
	void						ProcessingEventRemove( CProcessingEvent& e ) override;

	Bool						IsNoticableInGame( CProcessingEvent::EType eventType ) const override;
	Bool						IsNoticableInEditor( CProcessingEvent::EType eventType ) const override;

	Bool						IsEffectingInstanceAreas( EPathLibCollision colGroup ) const;
	Bool						IsEffectingInstanceAreas() const									{ return IsEffectingInstanceAreas( GetPathLibCollisionGroup() ); }

	// General processing mechanisms implementation
	static void					RemoveImmediateObstacle( CComponentRuntimeProcessingContext& context, const CProcessingEvent& e );
	static void					RemoveDynamicObstacle( CComponentRuntimeProcessingContext& context, const CProcessingEvent& e );

	IObstacleComponent*			AsObstacleComponent() override;

};

};			// namespace PathLib