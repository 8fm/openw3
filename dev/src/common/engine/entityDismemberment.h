/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "dynamicDecal.h"
#include "entityTemplateParams.h"
#include "dropPhysicsComponent.h"

class CEntityTemplate;

enum EWoundTypeFlags
{
	WTF_None		= 0,
	WTF_Cut			= FLAG( 0 ),
	WTF_Explosion	= FLAG( 1 ),
	WTF_Frost		= FLAG( 2 ),
	WTF_All			= WTF_Cut | WTF_Explosion | WTF_Frost,
};

BEGIN_ENUM_RTTI( EWoundTypeFlags )
	ENUM_OPTION( WTF_None );
	ENUM_OPTION( WTF_Cut );
	ENUM_OPTION( WTF_Explosion );
	ENUM_OPTION( WTF_Frost );
	ENUM_OPTION( WTF_All );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EDismembermentEffectTypeFlag
{ 
	DETF_Base		= FLAG( 0 ), 
	DETF_Igni		= FLAG( 1 ), 
	DETF_Aaard		= FLAG( 2 ), 
	DETF_Yrden		= FLAG( 3 ), 
	DETF_Quen		= FLAG( 4 ), 
	DETF_Mutation6  = FLAG( 5 ),
};

BEGIN_BITFIELD_RTTI( EDismembermentEffectTypeFlag, 1 );
	BITFIELD_OPTION( DETF_Base );
	BITFIELD_OPTION( DETF_Igni );
	BITFIELD_OPTION( DETF_Aaard );
	BITFIELD_OPTION( DETF_Yrden );
	BITFIELD_OPTION( DETF_Quen );
	BITFIELD_OPTION( DETF_Mutation6 );
END_BITFIELD_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SDismembermentEffect
{
	DECLARE_RTTI_STRUCT( SDismembermentEffect );

	CName	m_name;
	Uint32	m_typeMask;
};

BEGIN_CLASS_RTTI( SDismembermentEffect );
	PROPERTY_EDIT( m_name,												TXT( "Name of the effect to play" ) );
	PROPERTY_BITFIELD_EDIT( m_typeMask, EDismembermentEffectTypeFlag,	TXT( "Effect type combined flags" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SDismembermentWoundDecal
{
	DECLARE_RTTI_STRUCT( SDismembermentWoundDecal );

	SDynamicDecalMaterialInfo		m_materialInfo;

	Vector2							m_scale;			// Scale of the decal in its local projection space.
	Float							m_depthScale;		// Scale of the decal along projection direction.

	Vector2							m_offset;			// Offset from the wound's center.

	Float							m_depthFadePower;	// Exponent applied to fade the decal out by depth.
	Float							m_normalFadeBias;	// Offset affecting when normal-fading starts.
	Float							m_normalFadeScale;	// Scale affecting how quick normal-fading fades out.
	Bool							m_doubleSided;		// Whether decal should be applied to back facing surfaces.

	ERenderDynamicDecalProjection	m_projectionMode;

	Bool							m_applyToFillMesh;	// Whether the decal should be applied to the wound's fill mesh.

	SDismembermentWoundDecal()
		: m_scale( 1.0f, 1.0f )
		, m_depthScale( 1.0f )
		, m_offset( 0.0f, 0.0f )
		, m_depthFadePower( 0.0f )
		, m_normalFadeBias( 0.0f )
		, m_normalFadeScale( 1.0f )
		, m_doubleSided( true )
		, m_projectionMode( RDDP_Ortho )
		, m_applyToFillMesh( true )
	{}
};

BEGIN_CLASS_RTTI( SDismembermentWoundDecal );
	PROPERTY_EDIT( m_materialInfo			, TXT("Material used by the decal") );
	PROPERTY_EDIT( m_scale					, TXT("Scaling applied to the decal's local width/height, so it can be a different size/shape from the wound itself") );
	PROPERTY_EDIT( m_depthScale				, TXT("Scaling applied along the decal's length, so it can be visible beyond the end of the wound") );
	PROPERTY_EDIT( m_offset					, TXT("Offset applied to the decal's local right/up position, so it doesn't need to be centered on the wound") );
	PROPERTY_EDIT_RANGE( m_depthFadePower	, TXT("Higher values cause the decal to fade out smoothly towards the ends of the wound. Lower creates a sharper cutoff"), 0.0f, 100.0f );
	PROPERTY_EDIT_RANGE( m_normalFadeBias	, TXT("Bias applied when fading out based on the surface normal. Higher values make the decal more visible when the surface points away from projection direction"), 0.0f, 1.0f );
	PROPERTY_EDIT_RANGE( m_normalFadeScale	, TXT("Scale applied when fading out based on surface normal. Higher values make the fade sharper."), 0.0f, 1.0f );
	PROPERTY_EDIT( m_doubleSided			, TXT("Whether the decal is applied to surfaces facing away from the projection") );
	PROPERTY_EDIT( m_projectionMode			, TXT("How the decal texture coordinates are calculated. Ortho: orthographic projection, decal keeps the same size and shape. Sphere: decal shrinks towards ends, making an ellipsoidal volume.") );
	PROPERTY_EDIT( m_applyToFillMesh		, TXT("Whether the decal is applied to the wound's fill mesh") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SDismembermentWoundSingleSpawn
{
	DECLARE_RTTI_STRUCT( SDismembermentWoundSingleSpawn );

	//!< Entity template which can be used to spawn a new entity for the severed.
	//!< part. DismembermentComponent does not use this directly,
	THandle< CEntityTemplate >	m_spawnedEntity; 

	//!< Bone that will be used as spawn transform for the entity.
	CName					m_spawnEntityBoneName;

	//!< Name of the curve (defined in CDropPhysicsComponent) that will be used
	//!< to define spawned entity tajectory.
	CName					m_spawnedEntityCurveName;

	//!< Tag of the equipment mesh that should be dropped when the wound is active.
	CName					m_droppedEquipmentTag;

	//!< Sound events played while wound is shown
	TDynArray< StringAnsi >	m_soundEvents;

	//!< Despawn wound entity when base character's entity is being despawned
	Bool					m_despawnAlongWithBase;

	//!< Sync entity's pose to the base character's pose (false by default for comaptibiltiy with old assets)
	Bool					m_syncPose;

	//!< The way we want to fix bones hierarchy in case of disjoint parts are present after disabling parts of the ragdoll
	EFixBonesHierarchyType	m_fixBaseBonesHierarchyType;
	EFixBonesHierarchyType	m_fixSpawnedBonesHierarchyType;

	//!< List of effects' names that should be (optionally) played on spawned entity
	TDynArray< CName >		m_effectsNames;

	//!< List of additional effects that can be played depending on requested effect type
	TDynArray< SDismembermentEffect >	m_additionalEffects;

	SDismembermentWoundSingleSpawn()
		: m_spawnedEntity( nullptr )
		, m_despawnAlongWithBase( true )
		, m_syncPose( true )
		, m_fixBaseBonesHierarchyType( FBHTAddMissingBones )
		, m_fixSpawnedBonesHierarchyType( FBHTAddMissingBones )
	{
	}

	RED_FORCE_INLINE CEntityTemplate*				GetSpawnedEntity()			const { return m_spawnedEntity.Get(); }
	RED_FORCE_INLINE CName							GetSpawnEntityBoneName()	const { return m_spawnEntityBoneName; }
	RED_FORCE_INLINE CName							GetSpawnedEntityCurveName() const { return m_spawnedEntityCurveName; }
	RED_FORCE_INLINE CName							GetDroppedEquipmentTag()	const { return m_droppedEquipmentTag; }
	RED_FORCE_INLINE const TDynArray< StringAnsi >& GetSoundEvents()			const { return m_soundEvents; }

	void CollectEffects( Uint32 effectsMask, TDynArray< CName > & effectsNames ) const;
};

BEGIN_CLASS_RTTI( SDismembermentWoundSingleSpawn );
	PROPERTY_EDIT( m_spawnedEntity			, TXT("An entity that should be spawned with this wound") );
	PROPERTY_EDIT( m_spawnEntityBoneName	, TXT("Bone that will be used as spawn transform for the entity") );
	PROPERTY_EDIT( m_spawnedEntityCurveName	, TXT("Curve (defined in CDropPhysicsCompnent) that will be used as entity trajectory") );
	PROPERTY_EDIT( m_droppedEquipmentTag	, TXT("Tag of the equipment mesh that should be dropped when the wound is active") );
	PROPERTY_EDIT_ARRAY( m_soundEvents		, TXT("List of sound events." ) );
	PROPERTY_EDIT( m_despawnAlongWithBase	, TXT("Despawn wound entity when base character's entity is being despawned" ) );
	PROPERTY_EDIT( m_syncPose				, TXT("Sync entity's pose to the base character's pose (false by default for comaptibiltiy with old assets)" ) );
	PROPERTY_EDIT( m_fixBaseBonesHierarchyType		, TXT("The way we want to fix bones hierarchy in case of disjoint parts are present after disabling parts of the ragdoll" ) );
	PROPERTY_EDIT( m_fixSpawnedBonesHierarchyType	, TXT("The way we want to fix bones hierarchy in case of disjoint parts are present after disabling parts of the ragdoll" ) );
	PROPERTY_EDIT_ARRAY( m_effectsNames		, TXT("List of effects' names that should be (optionally) played on spawned entity" ) );
	PROPERTY_EDIT_ARRAY( m_additionalEffects, TXT("List of additional effects that can be played depending on requested effect type" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CDismembermentWound : public ISerializable
{
	DECLARE_RTTI_SIMPLE_CLASS( CDismembermentWound );
	friend class CEntityDismemberment;
	friend class CDismembermentComponent;

public:

	typedef TDynArray< SDismembermentWoundSingleSpawn > SingleSpawnArray;

private:
	CName						m_name;						//!< Name of the wound, used for selecting which is visible.
	EngineTransform				m_transform;				//!< Transform for the clipping ellipse.
	CName						m_excludeTag;				//!< Any components with this tag will not be clipped.

	TDynArray< CName >			m_disabledOnAppearances;	//!< This wound is disabled on these appearances.

	THandle< CMesh >			m_fillMesh;					//!< Mesh used to fill the resulting hole. Will be skinned if possible.

	SingleSpawnArray			m_singleSpawnArray;

	TSoftHandle<CParticleSystem> m_particles;				//!< Particles spawned while wound is shown - spawned as a separate entity
	TSoftHandle<CParticleSystem> m_attachedParticles;		//!< Particles spawned while wound is shown - will be attached to the nearest bone
	Bool						m_isExplosionWound;			//!< Is wound caused by explosion.
	Bool						m_isFrostWound;				//!< Is wound caused by frost.


	CName						m_mainEntityCurveName;

	SDismembermentWoundDecal	m_decal;

	CDismembermentWound( const CName& name );

#ifndef NO_EDITOR
	Bool SetTransform( const Vector* pos, const EulerAngles* rot, const Vector* scale );
#endif

public:
	CDismembermentWound();

	RED_FORCE_INLINE const CName&				GetName() const { return m_name; }
	RED_FORCE_INLINE const EngineTransform&		GetTransform() const { return m_transform; }
	RED_FORCE_INLINE const CName&				GetExcludeTag() const { return m_excludeTag; }
	RED_FORCE_INLINE CMesh*						GetFillMesh() const { return m_fillMesh.Get(); }
	RED_FORCE_INLINE const SingleSpawnArray&	GetSpawnArray() const { return m_singleSpawnArray; }
	RED_FORCE_INLINE TSoftHandle<CParticleSystem> GetParticles() const { return m_particles; }
	RED_FORCE_INLINE TSoftHandle<CParticleSystem> GetAttachedParticles() const { return m_attachedParticles; }
	RED_FORCE_INLINE Bool						IsExplosionWound() const { return m_isExplosionWound; }
	RED_FORCE_INLINE Bool						IsFrostWound() const { return m_isFrostWound; }
	RED_FORCE_INLINE const CName&				GetMainEntityCurveName() const { return m_mainEntityCurveName; }

	EWoundTypeFlags								GetTypeFlags() const
	{
		Uint32 flags = 0;
		if ( m_isExplosionWound )
		{
			flags |= WTF_Explosion;
		}
		if ( m_isFrostWound )
		{
			flags |= WTF_Frost;
		}
		return flags != 0 ? static_cast< EWoundTypeFlags >( flags ) : WTF_Cut;
	}


	Bool IsAvailableForAppearance( const CName& appearanceName ) const
	{
		return !m_disabledOnAppearances.Exist( appearanceName );
	}

	void AllowForAppearance( const CName& appearanceName, Bool isAllowed )
	{
		if ( isAllowed )
		{
			m_disabledOnAppearances.Remove( appearanceName );
		}
		else
		{
			m_disabledOnAppearances.PushBackUnique( appearanceName );
		}
	}

	// 
	RED_INLINE bool operator ==( const CDismembermentWound& other ) const { return m_name == other.m_name; }
	RED_INLINE bool operator !=( const CDismembermentWound& other ) const { return m_name != other.m_name; }
};

BEGIN_CLASS_RTTI( CDismembermentWound );
	PARENT_CLASS( ISerializable );
	PROPERTY_EDIT( m_name, TXT("Wound name") );
	PROPERTY( m_disabledOnAppearances );
	PROPERTY_EDIT( m_transform			, TXT("Position/Orientation/Scale of the wound's cutting ellipse. This is in the bind pose, so for optimal editing you should force T Pose. Use \"Show Wounds\" button to edit visually.") );
	PROPERTY_EDIT( m_excludeTag			, TXT("Components with this tag will not be clipped by this wound") );
	PROPERTY_EDIT( m_fillMesh			, TXT("A mesh used to fill the wound's hole") );
	PROPERTY_EDIT( m_singleSpawnArray	, TXT( "" ) );
	PROPERTY_EDIT( m_particles			, TXT("Particles spawned while wound is shown - spawned as a separate entity") );
	PROPERTY_EDIT( m_attachedParticles	, TXT("Particles spawned while wound is shown - will attached to the nearest bone") );
	PROPERTY_EDIT( m_isExplosionWound	, TXT("Is wound caused by explosion") );
	PROPERTY_EDIT( m_isFrostWound		, TXT("Is wound caused by frost." ) );
	PROPERTY_EDIT( m_decal				, TXT("Details about the decal applied over the wound.") );
	PROPERTY_EDIT( m_mainEntityCurveName, TXT("Curve (defined in CDropPhysicsCompnent) that will be used as entity main trajectory") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
// Used to disable a wound for a specific appearance.

struct SDismembermentWoundFilter
{
	DECLARE_RTTI_STRUCT( SDismembermentWoundFilter );
	CName m_wound;
	CName m_appearance;

	SDismembermentWoundFilter() {}
	SDismembermentWoundFilter( const CName& woundName, const CName& appearanceName )
		: m_wound( woundName )
		, m_appearance( appearanceName )
	{}

	Bool operator ==( const SDismembermentWoundFilter& other ) const { return m_wound == other.m_wound && m_appearance == other.m_appearance; }
	Bool operator !=( const SDismembermentWoundFilter& other ) const { return !operator ==( other ); }
};

BEGIN_CLASS_RTTI( SDismembermentWoundFilter );
	PROPERTY_EDIT( m_wound		, TXT( "Wound" ) );
	PROPERTY_EDIT( m_appearance , TXT( "Appearance" ));
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CEntityDismemberment : public CEntityTemplateParam
{
	DECLARE_ENGINE_CLASS( CEntityDismemberment, CEntityTemplateParam, 0 );

private:
	typedef TDynArray< CName > AppearanceArray;


	TDynArray< CDismembermentWound* >			m_wounds;

	TDynArray< SDismembermentWoundFilter >		m_disabledWounds;			//!< Wounds disabled for certain appearances.

public:
	CEntityDismemberment();
	~CEntityDismemberment();


	const TDynArray< CDismembermentWound* >& GetWounds() const { return m_wounds; }
	TDynArray< CDismembermentWound* >& GetWounds() { return m_wounds; }

	Bool HasWound( const CName& name ) const;

#ifndef NO_EDITOR
	CDismembermentWound* FindWoundByName( const CName& name );
#endif
	const CDismembermentWound* FindWoundByName( const CName& name ) const;

#ifndef NO_EDITOR
	Bool AddWound( const CName& name );
	Bool AddWound( CDismembermentWound* wound );

	Bool RemoveWound( const CName& name );
	Bool SetWoundTransform( const CName& name, const Vector* pos, const EulerAngles* rot, const Vector* scale );


	void SetWoundDisabledForAppearance( const CName& woundName, const CName& appearanceName, Bool disabled );
#endif

	Bool IsWoundDisabledForAppearance( const CName& woundName, const CName& appearanceName ) const;


#ifndef NO_EDITOR
	// Only for the editor.
	TDynArray< SDismembermentWoundFilter >& GetDisabledWounds() { return m_disabledWounds; }
#endif


	//////////////////////////////////////////////////////////////////////////
	// Some static utilities for doing operations across multiple CEntityDismemberments, depending on entity template inclusions.
	//
	// TODO : These aren't as efficient as they could be, so maybe keep it in mind for optimizing later?

	// Check if the wound is disabled for the entity's current appearance. If entity has no appearances (or no template), wounds are
	// never disabled.
	static Bool IsWoundDisabledRecursive( CEntity* entity, const CName& woundName );

	// Check if the wound is disabled for the entity's current appearance, by an included entity template.
	static Bool IsWoundDisabledByIncludeRecursive( CEntity* entity, const CName& woundName );

	static void GetEnabledWoundNamesRecursive( CEntity* entity, TDynArray< CName >& outNames, EWoundTypeFlags woundTypeFlags = WTF_All );
	static void GetEnabledWoundsRecursive( CEntity* entity, TDynArray< const CDismembermentWound* >& outWounds, EWoundTypeFlags woundTypeFlags = WTF_All );


	static const CDismembermentWound* FindWoundByNameRecursive( CEntityTemplate* entityTemplate, const CName& woundName );
#ifndef NO_EDITOR
	static CDismembermentWound* FindNonConstWoundByNameRecursive( CEntityTemplate* entityTemplate, const CName& woundName );
#endif

	static void GetAllWoundNamesRecursive( CEntityTemplate* entityTemplate, TDynArray< CName >& outNames );
};

BEGIN_CLASS_RTTI( CEntityDismemberment );
	PARENT_CLASS( CEntityTemplateParam );
	PROPERTY_INLINED( m_wounds, TXT("Wounds") );
	PROPERTY_INLINED( m_disabledWounds, TXT("Disabled wounds") );
END_CLASS_RTTI();