/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../physics/physicalCollision.h"
#include "../physics/physicsMemory.h"
#include "../core/resource.h"
#include "../physics/physicsRagdollState.h"

/// Physical ragdoll
class CRagdoll : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CRagdoll, CResource, "w2ragdoll", "Ragdoll" );
	
protected:
	DataBuffer					m_repxBuffer;
	DataBuffer					m_physicsBuffer;
	SPhysicsRagdollState		m_state;

	void ReloadBuffers();
public:
	//! Ragdoll editor import  data
	struct FactoryInfo : public CResource::FactoryInfo< CRagdoll >
	{	
		FactoryInfo()
			: m_repxBuffer( PhysXDataBufferAllocator< MC_PhysxRagdollBuffer >::GetInstance() )
		{
		}

		DataBuffer m_repxBuffer;
	};

public:
	RED_INLINE const DataBuffer& GetPhysicsBuffer() const { return m_physicsBuffer; }

	const SPhysicsRagdollState& GetState() const { return m_state; }

public:
	CRagdoll();

	//! Serialize the object
	virtual void OnSerialize( IFile& file );
	
#if !defined(NO_EDITOR)
	virtual void OnResourceSavedInEditor();
#endif

public:
	//! Create resource from factory definition
	static CRagdoll* Create( const FactoryInfo& data );

};

////////////////////////////////////////////////////////////////////////////////////////

BEGIN_CLASS_RTTI( CRagdoll );
	PARENT_CLASS( CResource );
	PROPERTY_EDIT_NAME( m_state.m_windScaler, TXT( "windScaler" ), TXT("Wind scaler") );
	PROPERTY_EDIT_NAME( m_state.m_densityScaler, TXT( "densityScaler" ), TXT("Density scaler") );
	PROPERTY_EDIT_NAME( m_state.m_autoStopDelay, TXT("autoStopDelay" ), TXT("Delay before Auto-stop")  );
	PROPERTY_EDIT_NAME( m_state.m_autoStopTime, TXT( "autoStopTime" ), TXT("Auto-stop after time [s]") );
	PROPERTY_EDIT_NAME( m_state.m_autoStopSpeed, TXT( "autoStopSpeed" ), TXT("Auto-stop min speed")  );
	PROPERTY_EDIT_NAME( m_state.m_resetDampingAfterStop, TXT( "resetDampingAfterStop" ), TXT("Reset Damping after stop")  );
	PROPERTY_EDIT_NAME( m_state.m_forceWakeUpOnAttach, TXT( "forceWakeUpOnAttach" ), TXT("Ragdoll will be wakeuped when attached")  );
	PROPERTY_CUSTOM_EDIT_NAME( m_state.m_customDynamicGroup, TXT( "customDynamicGroup" ), TXT( "Custom dynamic group" ), TXT("PhysicalCollisionTypeSelector") );
	PROPERTY_EDIT_NAME( m_state.m_disableConstrainsTwistAxis, TXT( "disableConstrainsTwistAxis" ), TXT("") );
	PROPERTY_EDIT_NAME( m_state.m_disableConstrainsSwing1Axis, TXT("disableConstrainsSwing1Axis"), TXT("") );
	PROPERTY_EDIT_NAME( m_state.m_disableConstrainsSwing2Axis, TXT("disableConstrainsSwing2Axis"), TXT("") );
	PROPERTY_EDIT_RANGE_NAME( m_state.m_jointBounce, TXT("jointBounce"), TXT("Bounce override for all joints in ragdoll resource. Range 0.f to 1.f"), 0.f, 1.f );
	PROPERTY_EDIT_NAME( m_state.m_modifyTwistLower, TXT("modifyTwistLower"), TXT("") );
	PROPERTY_EDIT_NAME( m_state.m_modifyTwistUpper, TXT("modifyTwistUpper"), TXT("") );
	PROPERTY_EDIT_NAME( m_state.m_modifySwingY, TXT("modifySwingY"), TXT("") );
	PROPERTY_EDIT_NAME( m_state.m_modifySwingZ, TXT("modifySwingZ"), TXT("") );
	PROPERTY_EDIT_NAME( m_state.m_projectionIterations, TXT("projectionIterations"), TXT("") );

END_CLASS_RTTI();