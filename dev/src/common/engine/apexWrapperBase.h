#pragma once
#include "..\physics\physicsWrapper.h"
#include "renderObject.h"


enum EDispatcherSelection
{
	EDS_CPU_ONLY,
	EDS_GPU_ONLY,
	EDS_GPU_IF_AVAILABLE,
};

BEGIN_ENUM_RTTI( EDispatcherSelection );
ENUM_OPTION( EDS_CPU_ONLY );
ENUM_OPTION( EDS_GPU_ONLY );
ENUM_OPTION( EDS_GPU_IF_AVAILABLE );
END_ENUM_RTTI();

#ifdef USE_APEX
namespace physx
{
	namespace apex
	{
		class NxApexActor;
		class NxApexRenderable;
	}
};
#endif

void RegisterApexMaterial( const String& name, class IRenderResource* material, class IRenderResource* parameters );
void UnregisterApexMaterial( const String& name );
struct SApexMaterialMapping* GetMappingForApexMaterial( const String& name );
void UnregisterApexMaterials();

class CApexWrapper : public CPhysicsWrapperInterface, public IRenderObject
{
	DECLARE_RENDER_OBJECT_MEMORYCLASS( MC_ApexRenderObject )

protected:
	class CPhysicsWorldPhysXImpl*	m_world;
	Float							m_autoHideDistance;
	Box								m_bounds;

public:
	CApexWrapper() : CPhysicsWrapperInterface(), m_bounds( Box::EMPTY ) {}
	virtual CPhysicsWorld* GetPhysicsWorld();

#ifdef USE_APEX
	virtual physx::apex::NxApexActor* GetApexActor() = 0;
	virtual physx::apex::NxApexRenderable* AcquireRenderable() = 0;
	virtual void ReleaseRenderable( physx::apex::NxApexRenderable* renderable ) = 0;
#endif

	/// Returns false if bounds cannot be gotten. This could happen if the apex actor has not been created.
	const Box& GetBounds() const { return m_bounds; };
	// Getter
	Float GetAutoHideDistance() const { return m_autoHideDistance; };
	// Global setter for autohide distance for all apex resources
	void SetAutoHideDistance( Float autohide ) { m_autoHideDistance = autohide; }

#ifdef USE_APEX
	//
	// TODO: When Apex gets updated, with its own proper handling of actor/renderable references, this internal ref-counting will need to be removed.
	//
	/// Default actor ref-counting is done through the actor's userData field, storing a record that includes a refcount. Subclasses may
	/// provide an alternative if needed, but this should be good enough.
	virtual void AddActorRef( physx::apex::NxApexActor* actor );
	virtual void ReleaseActorRef( physx::apex::NxApexActor* actor );
#endif

};

