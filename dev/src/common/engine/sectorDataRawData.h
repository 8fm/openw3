#pragma once

/// NOTE: Hand crafted data & code. Modify with care.
namespace SectorData
{
	static const Uint32 DATA_VERSION = 7;

	// Object flags
	enum EPackedObjectFlag
	{
		ePackedObjectFlag_W3_EntityBase						= FLAG( 0 ), // W3 HACK FLAG: it's a part of the building that has an entity proxy (but that's not the proxy itself)
		ePackedObjectFlag_W3_EntityProxy					= FLAG( 1 ), // W3 HACK FLAG: it's the entity proxy
		ePackedObjectFlag_Important							= FLAG( 2 ), // This object is important, try to stream it as fast as possible
		ePackedObjectFlag_Secondary							= FLAG( 3 ), // This object is secondary - we don't have to kill ourselves to stream it in if we don't have time
	};

	// General flags
	// DO NOT REORDER
	enum EPackedFlag
	{
		// meshes
		ePackedFlag_Mesh_CastingShadows						= FLAG( 0 ),
		ePackedFlag_Mesh_HasCollision						= FLAG( 1 ),
		ePackedFlag_Mesh_Visible							= FLAG( 2 ),
		ePackedFlag_Mesh_CameraTransformTranslate			= FLAG( 3 ),
		ePackedFlag_Mesh_CameraTransformRotate				= FLAG( 4 ),
		ePackedFlag_Mesh_CastingShadowsWhenNotVisible		= FLAG( 5 ),
		ePackedFlag_Mesh_ForceNoAutohide					= FLAG( 6 ),
		ePackedFlag_Mesh_FadeOnCameraCollision				= FLAG( 7 ),
		ePackedFlag_Mesh_CastingShadowsFromLocalLightsOnly	= FLAG( 8 ),
		ePackedFlag_Mesh_NoDissolves						= FLAG( 9 ),
		ePackedFlag_Mesh_PartOfEntityProxy					= FLAG( 10 ),
		ePackedFlag_Mesh_RootEntityProxy					= FLAG( 11 ),
		ePackedFlag_Mesh_AllowAutoHideOverride				= FLAG( 12 ),

		// dimmers
		ePackedFlag_Dimmer_AreaMarker						= FLAG( 0 ),

		// decals
		ePackedFlag_Decal_VerticalFlip						= FLAG( 0 ),
		ePackedFlag_Decal_HorizontalFlip					= FLAG( 1 ),

		// lights
		ePackedFlag_Light_AllowDistanceFade					= FLAG( 0 ),
		ePackedFlag_Light_CacheStaticShadows				= FLAG( 1 ),
		ePackedFlag_Light_HasProjectionTexture				= FLAG( 2 ),

		// particles
		ePackedFlag_Particles_Visible						= FLAG( 0 ),
		ePackedFlag_Particles_ForceNoAutohide				= FLAG( 1 )
	};

	// General object types
	enum EObjectType
	{
		eObject_Invalid=0,
		eObject_Mesh,
		eObject_Collision,
		eObject_Decal,
		eObject_Dimmer,
		eObject_PointLight,
		eObject_SpotLight,
		eObject_RigidBody,
		eObject_Cloth,
		eObject_Destruction,
		eObject_Particles,
	};

	// Bounding box indices
	enum EBox
	{
		eBox_MinX=0,
		eBox_MinY=1,
		eBox_MinZ=2,
		eBox_MaxX=3,
		eBox_MaxY=4,
		eBox_MaxZ=5,
	};

#pragma pack(push,4)
	/// Object handle
	struct PackedObject
	{
		// Object type, flags and, streaming radius
		Uint32		m_type:8;
		Uint32		m_flags:8;
		Uint32		m_radius:16;

		// Offset to data in the data stream
		Uint32		m_offset;

		// ID of the assigned sector
		Uint32		m_sectorID;

		// Streaming reference position (NOTE: does NOT have to correspond to actual object's position)
		Vector3		m_pos;

		// Get object data
		template< typename T >
		const T* GetData( const TDynArray< Uint8, MC_SectorData >& dataStream ) const
		{
			return (const T*) OffsetPtr( dataStream.TypedData(), m_offset );
		}

		// Get object data
		template< typename T >
		T* GetData( TDynArray< Uint8, MC_SectorData >& dataStream ) const
		{
			return (T*) OffsetPtr( dataStream.TypedData(), m_offset );
		}
	};

	// Packed object matrix
	struct PackedMatrix
	{
		Float						m_data[12];

		RED_FORCE_INLINE const Matrix Unpack() const
		{
			Matrix ret;
			ret.V[0] = Vector( m_data[0], m_data[1], m_data[2], 0.0f );
			ret.V[1] = Vector( m_data[3], m_data[4], m_data[5], 0.0f );
			ret.V[2] = Vector( m_data[6], m_data[7], m_data[8], 0.0f );
			ret.V[3] = Vector( m_data[9], m_data[10], m_data[11], 1.0f );
			return ret;
		}

		RED_FORCE_INLINE void Pack( const Matrix& m )
		{
			m_data[0] = m.V[0].X;
			m_data[1] = m.V[0].Y;
			m_data[2] = m.V[0].Z;
			m_data[3] = m.V[1].X;
			m_data[4] = m.V[1].Y;
			m_data[5] = m.V[1].Z;
			m_data[6] = m.V[2].X;
			m_data[7] = m.V[2].Y;
			m_data[8] = m.V[2].Z;
			m_data[9] = m.V[3].X;
			m_data[10] = m.V[3].Y;
			m_data[11] = m.V[3].Z;
		}

		RED_FORCE_INLINE Vector GetTranslation() const
		{
			return Vector( m_data[9], m_data[10], m_data[11] );
		}
	};

	// Packed reference to resource
	struct PackedResourceRef
	{
		Uint16						m_resourceIndex;			//!< Index of resource in the resource table, 0 - NULL

		RED_FORCE_INLINE PackedResourceRef()
			: m_resourceIndex( 0 )
		{}

		RED_FORCE_INLINE PackedResourceRef( const Uint32 index )
		{
			RED_FATAL_ASSERT( index <= 65535, "Out of range resource index" );
			m_resourceIndex = index;
		}

		template< typename T >
		RED_FORCE_INLINE void Remap( const T& remapTable )
		{
			m_resourceIndex = remapTable[ m_resourceIndex ];
		}
	};

	// Packed object data
	struct PackedBase
	{		
		PackedMatrix				m_localToWorld;				//!< Full transformation
		Uint16						m_radius;					//!< Object streaming radius
		Uint16						m_flags;					//!< Generic flags
		Uint32						m_occlusionId;				//!< Static occlusion id

		RED_FORCE_INLINE const Bool HasFlag( const EPackedFlag flag ) const
		{
			return 0 != ( m_flags & flag );
		}
	};

	// Packed mesh data
	struct PackedMesh : public PackedBase
	{
		PackedResourceRef			m_mesh;						//!< Mesh resource to use
		Uint16						m_forcedAutoHide;			//!< Forced custom auto hide on mesh (if 0, not used)
		Uint8						m_lightChannels;			//!< Light channels
		Int8						m_forcedLODLevel;			//!< Forced LOD level
		Int8						m_shadowBias;				//!< Shadow rendering bias
		Uint8						m_renderingPlane;			//!< Rendering plane
	};

	// Packed collision object data
	struct PackedCollision : public PackedBase
	{
		PackedResourceRef			m_mesh;						//!< Mesh resource to use
		Uint16						m_pad00;					//!< General padding
		Uint64						m_collisionMask;			//!< Collision type to use - see the CPhysicalCollision constructor
		Uint64						m_collisionGroup;			//!< Collision group to use - see the CPhysicalCollision constructor
	};

	// Packed dimmer data - matches CDimmerComponent
	struct PackedDimmer : public PackedBase
	{
		Float						m_ambientLevel;				//!< TODO: Consider quantizing and packing
		Float						m_marginFactor;				//!< TODO: Consider quantizing and packing
		Uint8						m_dimmerType;				//!< Type of the dimmer object
		Uint8						m_pad00;
		Uint8						m_pad01;
		Uint8						m_pad02;
	};

	// Packed decal data - matches CDecalComponent
	struct PackedDecal : public PackedBase
	{
		PackedResourceRef			m_diffuseTexture;
		Uint8						m_pad00;
		Uint8						m_pad01;
		Color						m_specularColor;
		Float						m_normalThreshold;			//!< TODO: Consider quantizing and packing
		Float						m_specularity;				//!< TODO: Consider quantizing and packing
		Float						m_fadeTime;
	};

	// Packed light data
	struct PackedLight : public PackedBase
	{
		Color						m_color;					//!< Light color
		Float						m_radius;					//!< Light radius (TODO: consider quantizing and packing)
		Float						m_brightness;				//!< Light brightness (TODO: consider quantizing and packing)
		Float						m_attenuation;				//!< Light attenuation (TODO: consider quantizing and packing)
		Float						m_autoHideRange;			//!< Auto hide range (TODO: consider quantizing and packing)
		Float						m_shadowFadeDistance;		//!< Distance from light at which the shadow should start to fade away (if 0 then disabled) (TODO: consider quantizing and packing)
		Float						m_shadowFadeRange;			//!< Shadow fading range (TODO: consider quantizing and packing)
		Float						m_shadowBlendFactor;		//!< Shadow blend factor (TODO: consider quantizing and packing)
		Float						m_lightFlickering[3];		//!< Light flickering
		Uint8						m_shadowCastingMode;		//!< How the shadows are casted
		Uint8						m_dynamicShadowsFaceMask;	//!< PL: dynamic shadows masking
		Uint8						m_envColorGroup;			//!< Environment color group
		Uint8						m_pad00;
		Uint32						m_lightUsageMask;			//!< Light flags determining in which situations light is used
	};

	// Packed light data
	struct PackedParticles : public PackedBase
	{
		PackedResourceRef			m_particleSystem;			//!< Particle system resource to use
		Uint16						m_pad00;
		Uint8						m_lightChannels;			//!< Light channels
		Uint8						m_renderingPlane;			//!< Rendering plane
		Uint8						m_envAutoHideGroup;			//!< Environment auto hide group
		Uint8						m_transparencySortGroup;	//!< Transparency sort group
		Float						m_globalEmissionScale;		//!< Global emission scale for this particle system
	};

	// Packed spot light data
	struct PackedSpotLight : public PackedLight
	{
		Float						m_innerAngle;				//!< Inner cone angle (TODO: consider quantizing and packing)
		Float						m_outerAngle;				//!< Outer cone angle (TODO: consider quantizing and packing)
		Float						m_softness;					//!< Cone softness (TODO: consider quantizing and packing)
		Float						m_projectionTextureAngle;	//!< Projection texture angle (TODO: consider quantizing and packing)
		Float						m_projectionTexureUBias;	//!< Projection texture U bias (TODO: consider quantizing and packing) 
		Float						m_projectionTexureVBias;	//!< Projection texture V bias (TODO: consider quantizing and packing)
		PackedResourceRef			m_projectionTexture;		//!< Projection texture
		Uint16						m_pad00;
	};

	// Packed rigid body data
	struct PackedRigidBody : public PackedMesh
	{
		Float						m_linearDamping;			//!< TODO: Consider quantizing and packing
		Float						m_angularDamping;			//!< TODO: Consider quantizing and packing
		Float						m_linearVelocityClamp;		//!< TODO: Consider quantizing and packing
		Uint64						m_collisionMask;			//!< Collision type to use - see the CPhysicalCollision constructor
		Uint64						m_collisionGroup;			//!< Collision group to use - see the CPhysicalCollision constructor
		Uint8						m_motionType;
		Uint8						m_pad00[3];
	};
#pragma pack(pop)


} /// Sector data