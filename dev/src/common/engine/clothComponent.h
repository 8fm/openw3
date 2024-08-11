/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "apexClothWrapper.h"
#include "meshTypeComponent.h"
#include "meshTypeResource.h"

enum ETriggerShape
{
	TS_None,
	TS_Sphere,
	TS_Box,
};


BEGIN_ENUM_RTTI( ETriggerShape );
ENUM_OPTION( TS_None );
ENUM_OPTION( TS_Sphere );
ENUM_OPTION( TS_Box );
END_ENUM_RTTI();

struct SClothParameters 
{
	THandle< CApexResource > m_resource;

	Matrix m_pose;

	EDispatcherSelection	m_dispacherSelection;
	Bool					m_recomputeNormals;
	Bool					m_correctSimulationNormals;
	Bool					m_slowStart;
	Bool					m_useStiffSolver;
	Float					m_pressure;
	Float					m_simulationMaxDistance;
	Float					m_distanceWeight;
	Float					m_bias;
	Float					m_benefitsBias;
	Float					m_maxDistanceBlendTime;
	Uint32					m_uvChannelForTangentUpdate;
	Bool					m_maxDistanceScaleMultipliable;
	Float					m_maxDistanceScaleScale;
	Float					m_collisionResponseCoefficient;
	Bool					m_allowAdaptiveTargetFrequency;
	Float					m_windScaler;
	Bool					m_isClothSkinned;

#ifndef NO_EDITOR
	Bool m_usePreviewAsset;
#endif

	SClothParameters();

};

class CClothComponent : public CMeshTypeComponent, public IPhysicalCollisionTriggerCallback
{
	DECLARE_ENGINE_CLASS( CClothComponent, CMeshTypeComponent, 0 )

protected:
	SClothParameters			m_parameters;
	ETriggerShape				m_triggerType;
	Vector						m_triggerDimensions;
	TDynArray< CName >			m_triggeringCollisionGroupNames;
	Matrix						m_triggerLocalOffset;
	CPhysicsWrapperInterface*	m_triggerWrapper;

#ifdef USE_APEX
	CApexClothWrapper*			m_clothWrapper;
#endif

	Float						m_shadowDistanceOverride;

	Bool						m_shouldUpdateWetness;
	Bool						m_teleportRequested;
#ifndef NO_EDITOR_FRAGMENTS
	Bool						m_isSkinnningUpdating;
#endif //NO_EDITOR_FRAGMENTS

	// [HACK]
	// This hack stores last available skinning matrices in order to pass it to the
	// apex object, when skinning is not updated (basically cloth component is not in view frustum).
	// Unfortunately, apex API does not allow to update only position and keep last skinning - that is what we need here.
	// Potential problem with this hack is that m_cachedLastSkinningMatrices pointer can be invalidated in 
	// CMeshSkinningAttachement (skinning buffers can be recreated), but to my best knowledge this is not the
	// case in this context. 
	Matrix* m_cachedLastSkinningMatrices;
	Uint32 m_cachedLastSkinningMatricesCount;

public:
	CClothComponent();

	Float GetShadowDistance( Uint8& outRenderMask ) const;

	virtual CMeshTypeResource* GetMeshTypeResource() const override;

	virtual Bool IsDynamicGeometryComponent() const { return true; }

	void OnParentAttachmentAdded( IAttachment* attachment ) override;
	void OnParentAttachmentBroken( IAttachment* attachment ) override;

	virtual void OnAttached( CWorld* world ) override;
	virtual void OnItemEntityAttached( const CEntity* par ) override;
	virtual void OnDetached( CWorld* world ) override;

	virtual Bool CanAttachToRenderScene() const;

	const ISkeletonDataProvider* GetProvider() const;

	virtual void OnUpdateSkinning(const ISkeletonDataProvider* provider, IRenderSkinningData* renderSkinningData, const Box& boxMS, const Matrix& l2w, SMeshSkinningUpdateContext& skinningContext ) override;
	virtual void OnUpdateTransformWithoutSkinning( const Box& boxMS, const Matrix& l2w, SMeshSkinningUpdateContext& skinningContext ) override;

	// Update component transform
	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;

	//Set resource
	virtual void SetResource( CResource* resource ) override;
	virtual void GetResource( TDynArray< const CResource* >& resources ) const override;

	virtual void onTriggerEntered( const STriggeringInfo& info );
	virtual void onTriggerExited( const STriggeringInfo& info );

#ifndef NO_EDITOR_FRAGMENTS
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag ) override;
#endif

	Uint32 SelectVertex( const Vector& worldPos, const Vector& worldDir, Vector& hitPos );
	void MoveVertex( Uint32 vertexIndex, const Vector& worldPos );	
	void FreeVertex( Uint32 vertexIndex );

	// Turn on/off the cloth simulation. If turned off, normal skinning will be used if available.
	void SetSimulated( Bool simulated );
	void FreezeCloth( Bool shouldFreeze = false );
	void SetMaxDistanceScale( Float ratio );
	void SetMaxDistanceBlendTime( Float ratio );
	void RequestTeleport() { m_teleportRequested = true; }
	virtual void OnResetClothAndDangleSimulation() override;

#ifdef USE_APEX
	RED_INLINE Bool HasResource() const { return m_parameters.m_resource.Get() != NULL; }

	class CPhysicsWrapperInterface* GetPhysicsRigidBodyWrapper() const { return m_triggerWrapper; }
	class CApexClothWrapper* GetClothWrapper() const { return m_clothWrapper; }
	virtual class IPhysicalCollisionTriggerCallback* QueryPhysicalCollisionTriggerCallback() { return this; }

	const TDynArray< String >&				GetApexMaterialNames() const;
	const CMeshTypeResource::TMaterials&	GetMaterials() const;
#endif	

	const SClothParameters&	GetParameters() const { return m_parameters; }

	void ActivateTrigger();

#ifndef NO_EDITOR
	virtual void EditorOnTransformChanged() override;

	void ForceLODLevel( Int32 lodOverride );
	void OnPropertyPostChange( IProperty* property );
	/// Set whether to use a preview asset, or the saved asset. Defaults to false (use saved asset)
	void SetUsePreview( Bool usePreview );

	void CopyParameters( class CPhantomComponent* component );
#endif

	virtual void OnCutsceneStarted();
	virtual void OnCutsceneEnded();
	virtual void OnCinematicStorySceneStarted();
	virtual void OnCinematicStorySceneEnded();

protected:
	void funcSetSimulated( CScriptStackFrame& stack, void* result );
	void funcSetMaxDistanceScale( CScriptStackFrame& stack, void* result );
	void funcSetFrozen( CScriptStackFrame& stack, void* result );

private:
	void CheckWetnessSupport( const CEntity* par );

	RED_INLINE void ClearCachedSkinningMatrices() { m_cachedLastSkinningMatrices = nullptr; m_cachedLastSkinningMatricesCount = 0; }
};

BEGIN_CLASS_RTTI( CClothComponent );
	PARENT_CLASS( CMeshTypeComponent );
	PROPERTY_EDIT_NAME( m_parameters.m_resource, TXT( "resource" ), TXT( "" ) )
	PROPERTY_EDIT_NAME( m_parameters.m_dispacherSelection, TXT( "dispacher selection" ), TXT( "" ) )
	PROPERTY_EDIT_NAME( m_parameters.m_recomputeNormals, TXT( "recomputeNormals" ), TXT( "Fully recomputes the normals on the final mesh. This usually leads to better looking results, but is more expensive to compute. Default is off. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_correctSimulationNormals, TXT( "correctSimulationNormals" ), TXT( "The MaxDistance=0 vertices can have a perturbed simulation normal. This usually happens only for meshes where the MaxDistance=0 vertices are somewhere in the middle separating a simulated and non-simulated region. The normal for those vertices will be computed only by the simulated triangles which can lead to wrong results. This solution will use the normals from the original simulation mesh and skin them with respect to the local pose. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_slowStart, TXT( "slowStart" ), TXT( "Prevents from having full max distance right from the start. The first time a NxClothingActor starts to be simulated is with full max distance. This prevents starting with full max distance and instead blending in as it will do the second time. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_useStiffSolver, TXT( "useStiffSolver" ), TXT( "Disables stiff (semi-implicit) solver for vertical fibers. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_pressure, TXT( "pressure" ), TXT( "Set pressure of cloth, only works on closed meshes. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_simulationMaxDistance, TXT( "lodWeights.maxDistance" ), TXT( "" ) )
	PROPERTY_EDIT_NAME( m_parameters.m_distanceWeight, TXT( "lodWeights.distanceWeight" ), TXT( "" ) )
	PROPERTY_EDIT_NAME( m_parameters.m_bias, TXT( "lodWeights.bias" ), TXT( "" ) )
	PROPERTY_EDIT_NAME( m_parameters.m_benefitsBias, TXT( "lodWeights.benefitsBias" ), TXT( "" ) )
	PROPERTY_EDIT_NAME( m_parameters.m_maxDistanceBlendTime, TXT( "maxDistanceBlendTime" ), TXT( " Time in seconds how long it takes to go from zero maxDistance to full maxDistance. Note: This also influences how quickly different physical LoDs can be switched " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_uvChannelForTangentUpdate, TXT( "uvChannelForTangentUpdate" ), TXT( "  This UV channel is used for updating tangent space.	Tangent update is done based on one UV channel. This allows selection of what UV channel is being used. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_maxDistanceScaleMultipliable, TXT( "maxDistanceScale.Multipliable" ), TXT( "" ) )
	PROPERTY_EDIT_NAME( m_parameters.m_maxDistanceScaleScale, TXT( "maxDistanceScale.Scale" ), TXT( "" ) )
	PROPERTY_EDIT_NAME( m_parameters.m_collisionResponseCoefficient, TXT( "collisionResponseCoefficient" ), TXT( "Defines a factor for the impulse transfer from cloth to colliding rigid bodies. This is only needed if the twoway interaction flag is set in the clothing asset. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_allowAdaptiveTargetFrequency, TXT( "allowAdaptiveTargetFrequency" ), TXT( "Slightly modifies gravity to avoid high frequency jittering due to variable time steps. " ) )
	PROPERTY_EDIT_NAME( m_parameters.m_windScaler, TXT( "windScaler" ), TXT( "Wind vector will be scalled by this value" ) )
	PROPERTY_CUSTOM_EDIT( m_triggeringCollisionGroupNames, TXT( "Defines which collision groups will trigger" ), TXT("PhysicalCollisionGroupSelector") );
	PROPERTY_EDIT( m_triggerType, TXT( "Phantom shape" ) );
	PROPERTY_EDIT( m_triggerDimensions, TXT( "Shape size ( applicable for a sphere and a box )" ) );
	PROPERTY_EDIT( m_triggerLocalOffset.V[ 3 ], TXT( "Local Offset" ) );
	PROPERTY_EDIT( m_shadowDistanceOverride, TXT("If < 0, get a default from the resource.") );
	NATIVE_FUNCTION( "SetSimulated", funcSetSimulated );
	NATIVE_FUNCTION( "SetMaxDistanceScale", funcSetMaxDistanceScale );
	NATIVE_FUNCTION( "SetFrozen", funcSetFrozen );
END_CLASS_RTTI();
