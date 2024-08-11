/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "component.h"
#include "entityDismemberment.h"
#include "../core/sortedset.h"

class CMeshComponent;
class CMeshTypeComponent;
class IRenderResource;
class CRenderFrame;
class CWorld;

//////////////////////////////////////////////////////////////////////////

// Control dismemberment in an entity. Can turn on a single wound, which causes a part of the entity to be cut away and a "fill mesh" to be
// added to fill the resulting hole.
//
// Should only be used for skinned entities. Technically it will work on static meshes as well, but the general assumption is that things
// will be skinned.
//
// The fill mesh is created as a new CMeshComponent inside the entity using default settings except CastShadows turned on.
//
// Visible wound is not persistent.
//
// NOTE : Only a single dismemberment component should exist in any given entity. The editor is set up to just work with the first one it
// finds, and having multiple could cause conflicts if they all set a visible wound (only the clipping from one would happen).
class CDismembermentComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CDismembermentComponent, CComponent, 0 );


	static const Uint32	INVALID_WOUND_INDEX = ( Uint32 )-1;

protected:

	const CDismembermentWound*					m_visibleWound;
	CName										m_visibleWoundName;

	THandle< CMeshComponent >					m_fillMeshComponent;
	IRenderResource*							m_decal;

	TDynArray< THandle< CMeshTypeComponent > >	m_affectedComponents;

public:
	CDismembermentComponent();
	~CDismembermentComponent();


	// debug
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag ) override;


	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;

	virtual bool UsesAutoUpdateTransform() override { return false; }

	// Is the given wound enabled, based on the entity's appearance. Does not check if the wound exists.
	Bool IsWoundEnabled( const CName& woundName ) const;


	// Get the names of all wounds allowed by the entity's current appearance.
	void GetWoundNames( TDynArray< CName >& outNames, EWoundTypeFlags woundTypeFlags = WTF_All ) const;

	// Get the currently visible wound. Note that in the editor, adding/removing wounds could invalidate this pointer. In the actual game
	// the wound list shouldn't change, so the pointer should be good as long as the component exists.
	RED_FORCE_INLINE const CDismembermentWound* GetVisibleWound() const { return m_visibleWound; }

	// Get the name of the currently visible wound.
	RED_FORCE_INLINE CName GetVisibleWoundName() const { return m_visibleWoundName; }

	// Make a wound visible. Only a single wound can be visible, so if something is already on it will be turned off first. It is
	// assumed that the entity won't change much after setting a wound (components will not be added, appearances changed, anything
	// like that). Setting a wound that doesn't exist will clear any active.
	void SetVisibleWound( const CName& name );

	// Set no wound visible. Clipping ellipse is turned off and fill mesh is destroyed.
	void ClearVisibleWound();

	// Get the CMeshComponent that was created for the visible wound's fill mesh. Note that this is destroyed if the wound is cleared.
	RED_FORCE_INLINE CMeshComponent* GetFillMeshComponent() const { return m_fillMeshComponent; }

	const CDismembermentWound* FindWoundByName( const CName& name ) const;
#ifndef NO_EDITOR
	// Non-const wounds are only available in the editor. Once we're in the game, wounds should be immutable.
	CDismembermentWound* FindWoundByName( const CName& name );
#endif


	// Get the nearest wound for the given position and cut "normal" (both in MODEL space)
	const CDismembermentWound* GetNearestWound( const Vector& positionMS, const Vector& directionMS, EWoundTypeFlags woundTypeFlags ) const;

	// Get the nearest wound for the given bone index and cut "normal" (in WORLD space)
	const CDismembermentWound* GetNearestWound( Int32 bondeIndex, const Vector& directionWS, EWoundTypeFlags woundTypeFlags ) const;

	// Spawn entity associated with wound
	Bool SpawnWoundEntity( const CDismembermentWound* wound, Bool dropEquipment = false, const Vector& direction = Vector::ZEROS, Uint32 playedEffectsMask = 0 );

	//void ExplodeSpawnedBodyPart( CEntity* spawnedEntity ) const;

	// Create particles associated with wound
	Bool CreateWoundParticles( const CName& woundName ) const;

private:

	// Sync spawned entity position to the base entity pose and transform
	Bool SyncSpawnedEntity( const SDismembermentWoundSingleSpawn& wound, CEntity* spawnedEntity ) const;

	// Calculate space that can mimic entity init rotation in world space.
	// All three axes are orthogonal to the specified bone matrix, moreover:
	// Z axis is "the most" up vector, X is "the most" right and Y "the most" forward
	Matrix FixBoneRotationWS( const Matrix& boneMatrixWS ) const;

	// Create particles using wound projection vector (Y axis) as reference position and attached to the nearest bone
	Bool CreateWoundParticles( const CDismembermentWound* wound ) const;

	// Create particles using slot as reference position and attachment info
	Bool CreateWoundParticles( const CDismembermentWound* wound, const EntitySlot* slot ) const;

	// Get index if nearest bone in reference pose
	Int32 FindNearestBone( const Vector& referencePositionMS ) const;

public:

	// Play sound events
	Bool PlaySoundEvents( const CDismembermentWound* wound ) const;

	// Force the active wound to be set. Clears the wound and then sets it again.
	void ForceUpdateWound();

	// Force the active wound's transform to be set.
	void ForceUpdateWoundTransform();

protected:
	// Collect which components should be affected by the given wound, and clear out any previously affected components that are no longer.
	void PreCreate( const CDismembermentWound* wound );
	// Update affected components with the current wound.
	void PostCreate();

	void CreateDecalOnAffectedComponents();

	void ClearUnaffectedComponents( const TDynArray< CMeshTypeComponent* >& newAffectedComponents );
	void UpdateAffectedComponents();

	Bool IsComponentAffected( CMeshTypeComponent* mtc, const CDismembermentWound* wound, const Matrix& entityToWound ) const;

	// Find components in the entity that are affected by the given wound. This is just any visible components whose bounds
	// intersect with the wound's bounds. Conservative test, so that there may be more components than necessary, but it shouldn't
	// miss any. Optionally apply scale and offset in the wound's space.
	void CollectAffectedComponents( const CDismembermentWound* wound, const Vector& scale, const Vector& offset, TDynArray< CMeshTypeComponent* >& components ) const;


protected:

	void funcIsWoundDefined( CScriptStackFrame& stack, void* result );
	void funcSetVisibleWound( CScriptStackFrame& stack, void* result );
	void funcClearVisibleWound( CScriptStackFrame& stack, void* result );
	void funcGetVisibleWoundName( CScriptStackFrame& stack, void* result );
	void funcCreateWoundParticles( CScriptStackFrame& stack, void* result );
	void funcGetNearestWoundName( CScriptStackFrame& stack, void* result );
	void funcGetNearestWoundNameForBone( CScriptStackFrame& stack, void* result );
	void funcGetWoundsNames( CScriptStackFrame& stack, void* result );
	void funcIsExplosionWound( CScriptStackFrame& stack, void* result );
	void funcIsFrostWound( CScriptStackFrame& stack, void* result );
	void funcGetMainCurveName( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CDismembermentComponent );
	PARENT_CLASS( CComponent );
	NATIVE_FUNCTION( "IsWoundDefined", funcIsWoundDefined );
	NATIVE_FUNCTION( "SetVisibleWound", funcSetVisibleWound );
	NATIVE_FUNCTION( "ClearVisibleWound", funcClearVisibleWound );
	NATIVE_FUNCTION( "GetVisibleWoundName", funcGetVisibleWoundName );
	NATIVE_FUNCTION( "CreateWoundParticles", funcCreateWoundParticles );
	NATIVE_FUNCTION( "GetNearestWoundName", funcGetNearestWoundName );
	NATIVE_FUNCTION( "GetNearestWoundNameForBone", funcGetNearestWoundNameForBone );
	NATIVE_FUNCTION( "GetWoundsNames", funcGetWoundsNames );
	NATIVE_FUNCTION( "IsExplosionWound", funcIsExplosionWound );
	NATIVE_FUNCTION( "IsFrostWound", funcIsFrostWound );
	NATIVE_FUNCTION( "GetMainCurveName", funcGetMainCurveName );
END_CLASS_RTTI();
