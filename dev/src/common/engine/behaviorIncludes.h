/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "../engine/animMath.h"
#include "../core/classBuilder.h"
#include "../core/math.h"
#include "../core/instanceBuffer.h"
#include "../core/enumBuilder.h"
#include "../core/engineTransform.h"

class CAnimatedComponent;
class CBehaviorGraph;
class CSkeleton;
struct SBehaviorGraphOutput;

//////////////////////////////////////////////////////////////////////////

#ifndef NO_LOG

#define BEH_LOG( format, ... )					RED_LOG( Behavior, format, ## __VA_ARGS__ )
#define BEH_WARN( format, ... )					RED_LOG_WARNING( Behavior, format, ## __VA_ARGS__ )
#define BEH_ERROR( format, ... )				RED_LOG_ERROR( Behavior, format, ## __VA_ARGS__ )

#else

#define BEH_LOG( format, ... )	
#define BEH_WARN( format, ... )	
#define BEH_ERROR( format, ... )

#endif

//////////////////////////////////////////////////////////////////////////

#define LOOK_AT_POSE

//////////////////////////////////////////////////////////////////////////

#define MIMIC_POSE_AREAS_NUM 16
#define MIMIC_POSE_BONES_NUM 0

//////////////////////////////////////////////////////////////////////////

enum EBehaviorLod : CEnum::TValueType
{
	BL_Lod0,	// Normal
	BL_Lod1,	// First lod, don't sample extra bones ( fingers, additives, overrides, etc. )
	BL_Lod2,	// Sample only necesary nodes
	BL_Lod3,	// Only motion extraction
	BL_NoLod,
};

BEGIN_ENUM_RTTI( EBehaviorLod );
	ENUM_OPTION( BL_Lod0 );
	ENUM_OPTION( BL_Lod1 );
	ENUM_OPTION( BL_Lod2 );
	ENUM_OPTION( BL_Lod3 );
	ENUM_OPTION( BL_NoLod );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EBlendType : CEnum::TValueType
{
	BT_BlendIn,
	BT_BlendOut
};

//////////////////////////////////////////////////////////////////////////

enum EAdditiveType : CEnum::TValueType
{
	AT_Local,
	AT_Ref,
	AT_TPose,
	AT_Animation,
};

BEGIN_ENUM_RTTI( EAdditiveType );
	ENUM_OPTION( AT_Local );
	ENUM_OPTION( AT_Ref );
	ENUM_OPTION( AT_TPose );
	ENUM_OPTION( AT_Animation );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

enum ECompressedPoseBlend : CEnum::TValueType
{
	CPBT_None,
	CPBT_Fast,
	CPBT_Normal,
	CPBT_Slow,
};

BEGIN_ENUM_RTTI( ECompressedPoseBlend );
	ENUM_OPTION( CPBT_None );
	ENUM_OPTION( CPBT_Fast );
	ENUM_OPTION( CPBT_Normal );
	ENUM_OPTION( CPBT_Slow );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

enum ESkeletalAnimationPoseType
{
	SAPT_Invalid,
	SAPT_Stand,
	SAPT_Sit,
	SAPT_Lie,
};

BEGIN_ENUM_RTTI( ESkeletalAnimationPoseType );
	ENUM_OPTION( SAPT_Invalid );
	ENUM_OPTION( SAPT_Stand );
	ENUM_OPTION( SAPT_Sit );
	ENUM_OPTION( SAPT_Lie );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

enum ESkeletalAnimationStreamingType : CEnum::TValueType
{
	SAST_Standard,
	SAST_Prestreamed,
	SAST_Persistent,
};

BEGIN_ENUM_RTTI( ESkeletalAnimationStreamingType );
	ENUM_OPTION( SAST_Standard );
	ENUM_OPTION( SAST_Prestreamed );
	ENUM_OPTION( SAST_Persistent );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EPoseDeformationLevel
{				// Example:
	PDL_Small,	// Idle
	PDL_Medium,	// Weapon draw, drink - one arm, leg is moving
	PDL_High,	// Combat hits
};

BEGIN_ENUM_RTTI( EPoseDeformationLevel );
	ENUM_OPTION( PDL_Small );
	ENUM_OPTION( PDL_Medium );
	ENUM_OPTION( PDL_High );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EInterpolationType
{
	IT_Linear,
	IT_Bezier
};

BEGIN_ENUM_RTTI( EInterpolationType );
	ENUM_OPTION( IT_Linear );
	ENUM_OPTION( IT_Bezier );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EAxis
{
	A_X = 0,
	A_Y,
	A_Z,
	A_NX,
	A_NY,
	A_NZ
};

BEGIN_ENUM_RTTI( EAxis );
	ENUM_OPTION( A_X );
	ENUM_OPTION( A_Y );
	ENUM_OPTION( A_Z );
	ENUM_OPTION( A_NX );
	ENUM_OPTION( A_NY );
	ENUM_OPTION( A_NZ );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EBehaviorTransitionBlendMotion
{
	BTBM_Blending,
	BTBM_Source,
	BTBM_Destination,
};

BEGIN_ENUM_RTTI( EBehaviorTransitionBlendMotion );
	ENUM_OPTION( BTBM_Blending );
	ENUM_OPTION( BTBM_Source );
	ENUM_OPTION( BTBM_Destination );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EAnimationFps
{
	AF_5,
	AF_10,
	AF_15,
	AF_30,
};

BEGIN_ENUM_RTTI( EAnimationFps );
	ENUM_OPTION( AF_5 );
	ENUM_OPTION( AF_10 );
	ENUM_OPTION( AF_15 );
	ENUM_OPTION( AF_30 );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EAnimationType
{
	EAT_Normal,
	EAT_Additive,
	EAT_Override,
};

//////////////////////////////////////////////////////////////////////////

struct SAnimationState
{
	CName	m_animation;
	Float	m_prevTime;
	Float	m_currTime;

	SAnimationState() : m_prevTime( 0.f ), m_currTime( 0.f ) {}

	void Reset()
	{
		m_animation = CName::NONE;
		m_currTime = 0.f;
		m_prevTime = 0.f;
	}
};

//////////////////////////////////////////////////////////////////////////

// A provider of a behavior graph property
class IBehaviorGraphProperty
{
public:
	virtual ~IBehaviorGraphProperty() {}

	virtual CBehaviorGraph* GetParentGraph() = 0;
};

//////////////////////////////////////////////////////////////////////////

// Behavior graph node interface for multi selection bones in custom editor
struct SBehaviorGraphBoneInfo
{
	String	m_boneName;
	Float	m_weight;
	Int32	m_num;

	DECLARE_RTTI_STRUCT( SBehaviorGraphBoneInfo );
};

BEGIN_CLASS_RTTI( SBehaviorGraphBoneInfo );
	PROPERTY_EDIT_NAME( m_boneName, TXT("m_boneName"), TXT("Bone name") );
	PROPERTY_EDIT_NAME( m_weight, TXT("m_weight"), TXT("Weight for bones") );
	PROPERTY(m_num);
END_CLASS_RTTI();

struct SBehaviorGraphTrackInfo
{
	SBehaviorGraphTrackInfo() : m_weight( 1.f ) {}

	String	m_trackName;
	Float	m_weight;

	DECLARE_RTTI_STRUCT( SBehaviorGraphTrackInfo );
};

BEGIN_CLASS_RTTI( SBehaviorGraphTrackInfo );
	PROPERTY_CUSTOM_EDIT( m_trackName, TXT("Track name"), TXT("BehaviorTrackSelection") );
	PROPERTY_EDIT( m_weight, TXT("Weight for track") );
END_CLASS_RTTI();

class IBehaviorGraphBonesPropertyOwner
{
public:
	virtual CSkeleton* GetBonesSkeleton( CAnimatedComponent* component ) const;
	virtual TDynArray<SBehaviorGraphBoneInfo>* GetBonesProperty() { return NULL; }
};

struct SBoneTransform
{
	DECLARE_RTTI_STRUCT( SBoneTransform );

	CName				m_bone;
	EngineTransform		m_transform;
};

BEGIN_CLASS_RTTI( SBoneTransform );
	PROPERTY_EDIT( m_bone, TXT("") );
	PROPERTY_EDIT( m_transform, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CInstancePropertiesBuilder
{
public:
	typedef TPair< String, String > tDesc;
	TDynArray< tDesc > m_data;

	void AddClassCaption( CClass* _class );

	void AddData( const String& name, const String& data, Uint32 depth );

	void ParseData( IRTTIType* type, const void* data, const String& name, Uint32 depth );

	template< class T > void AddProperty( InstanceBuffer& instance, const TInstanceVar< T >& var, const String& name )
	{
		const T& value = instance[ var ];

		IRTTIType* type = SRTTI::GetInstance().FindType( ::GetTypeName<T>() );

		if ( 0 == Red::System::StringCompare( name.AsChar(), TXT("i_"), 2 ) )
		{
			ParseData( type, &value, name.AsChar() + 2, 0 );
		}
		else
		{
			ParseData( type, &value, name.AsChar(), 0 );
		}
	}
};

#define INST_PROP_INIT builder.AddClassCaption( this->GetStaticClass() )
#define INST_PROP( _var ) builder.AddProperty( instance, _var, TXT( #_var ) )
#define INST_PROP_NAME( _var, _name ) builder.AddProperty( instance, _var, _name )

//////////////////////////////////////////////////////////////////////////

namespace BehaviorDebugUtils 
{
	void LogMsg( const String& desc );

	void LogVector( const Vector& vec );
	void LogVector( const String& desc, const Vector& vec );

	void LogMatrix( const Matrix& mat );
	void LogMatrix( const String& desc, const Matrix& mat );
}

//////////////////////////////////////////////////////////////////////////

namespace BehaviorUtils 
{
	//////////////////////////////////////////////////////////////////////////
	RED_INLINE Vector ConvertToVector(const RedVector4& rhs)
	{
		return reinterpret_cast<const Vector&>(rhs);
	}

	//////////////////////////////////////////////////////////////////////////
	template< class T >
	RED_INLINE T TClamp( const T& x, const T& min, const T& max )
	{
		return Clamp< T >( x, min, max );
	}

	template <> 
	RED_INLINE Vector TClamp< Vector >( const Vector& x, const Vector& min, const Vector& max )		
	{
		Vector result = Vector::Max4( x, min );
		result = Vector::Min4( result, max );
		return result;
	}

	template <> 
	RED_INLINE EulerAngles TClamp< EulerAngles >( const EulerAngles& x, const EulerAngles& min, const EulerAngles& max )		
	{
		EulerAngles result;
		result.Roll = Clamp( x.Roll, min.Roll, max.Roll );
		result.Pitch = Clamp( x.Pitch, min.Pitch, max.Pitch );
		result.Yaw = Clamp( x.Yaw, min.Yaw, max.Yaw );
		return result;
	}

	template < typename T >
	RED_INLINE T TValueDiffrence( const T& a, const T& b )
	{
		return a - b;
	}

	template <>
	RED_INLINE EulerAngles TValueDiffrence< EulerAngles >( const EulerAngles& a, const EulerAngles& b )
	{
		return EulerAngles::AngleDistance( b, a );
	}

	RED_INLINE Float BezierInterpolation( Float t )
	{
		return Clamp( -2.f * t*t*t + 3.f * t*t, 0.f, 1.f );
	}

	RED_INLINE Float Smoothstep2Interpolation( Float t )
	{
		return Clamp( t*t*t*(t*(t*6.f - 15.f) + 10.f), 0.f, 1.f );
	}

	RED_INLINE Float CosInterpolation( Float t )
	{
		return 1.f - 0.5f * ( cosf( M_PI * Clamp( t, 0.f, 1.f ) ) + 1.f );
	}

	RED_INLINE Float SmoothLinearToZero( Float t )
	{
		const Float t3 = t*t*t;
		const Float t4 = t*t*t*t;
		const Float t5 = t*t*t*t*t;

		return 3.f*t5 - 7.f*t4 + 4.f*t3 + t;
	}

	RED_INLINE Float SmoothZeroToLinear( Float t )
	{
		return -SmoothLinearToZero( t ) + 1.f;
	}

	template < typename T >
	RED_INLINE void SmoothCriticalDamp( T& currVal, T& vel, const Float dt, const T& targetVal, const Float smoothTime )
	{
		if ( smoothTime > 0.f )
		{
			const Float omega = 2.f / smoothTime;
			const Float x = omega * dt;
			const Float exp = 1.0f / ( 1.0f + x + 0.48f*x*x + 0.235f*x*x*x );
			const T diff = TValueDiffrence< T >( currVal, targetVal );
			const T temp = ( vel + diff * omega ) * dt;
			vel = ( vel - temp * omega ) * exp;
			currVal = targetVal + ( diff + temp ) * exp;
		}
		else if ( dt > 0.0f )
		{
			vel = TValueDiffrence< T >( targetVal, currVal ) / dt;
			currVal = targetVal;
		}
		else
		{
			vel -= vel;
			currVal = targetVal;
		}
	}

	template < typename T >
	RED_INLINE void SmoothCriticalDamp_MV( T& currVal, T& vel, const Float dt, const T& targetVal, const T& maxVel, const Float smoothTime )
	{
		if ( smoothTime > 0.f )
		{
			const Float omega = 2.f / smoothTime;
			const Float x = omega * dt;
			const Float exp = 1.0f / ( 1.0f + x + 0.48f*x*x + 0.235f*x*x*x );
			const T diff = TValueDiffrence< T >( currVal, targetVal );
			const T temp = ( vel + diff * omega ) * dt;
			vel = ( vel - temp * omega ) * exp;
			vel = TClamp< T >( vel, -maxVel, maxVel );
			currVal = targetVal + ( diff + temp ) * exp;
		}
		else if ( dt > 0.0f )
		{
			vel = TValueDiffrence< T >( targetVal, currVal ) / dt;
			currVal = targetVal;
		}
		else
		{
			vel -= vel;
			currVal = targetVal;
		}
	}

	template < typename T >
	RED_INLINE void SmoothCriticalDamp_MD( T& currVal, T& vel, const Float dt, const T& targetVal, const T& maxDiff, const Float smoothTime )
	{
		if ( smoothTime > 0.f )
		{
			const Float omega = 2.f / smoothTime;
			const Float x = omega * dt;
			const Float exp = 1.0f / ( 1.0f + x + 0.48f*x*x + 0.235f*x*x*x );
			T diff = TValueDiffrence< T >( currVal, targetVal );
			diff = TClamp< T >( diff, -maxDiff, maxDiff );
			const T temp = ( vel + diff * omega ) * dt;
			vel = ( vel - temp * omega ) * exp;
			currVal = targetVal + ( diff + temp ) * exp;
		}
		else if ( dt > 0.0f )
		{
			vel = TValueDiffrence< T >( targetVal, currVal ) / dt;
			currVal = targetVal;
		}
		else
		{
			vel -= vel;
			currVal = targetVal;
		}
	}

	template < typename T >
	RED_INLINE void SmoothCriticalDamp_MVMD( T& currVal, T& vel, const Float dt, const T& targetVal, const T& maxVel, const T& maxDiff, const Float smoothTime )
	{
		if ( smoothTime > 0.f )
		{
			const Float omega = 2.f / smoothTime;
			const Float x = omega * dt;
			const Float exp = 1.0f / ( 1.0f + x + 0.48f*x*x + 0.235f*x*x*x );
			T diff = TValueDiffrence< T >( currVal, targetVal );
			diff = TClamp< T >( diff, -maxDiff, maxDiff );
			const T temp = ( vel + diff * omega ) * dt;
			vel = ( vel - temp * omega ) * exp;
			vel = TClamp< T >( vel, -maxVel, maxVel );
			currVal = targetVal + ( diff + temp ) * exp;
		}
		else if ( dt > 0.0f )
		{
			vel = TValueDiffrence< T >( targetVal, currVal ) / dt;
			currVal = targetVal;
		}
		else
		{
			vel -= vel;
			currVal = targetVal;
		}
	}

	template < typename T >
	RED_INLINE void SmoothExponential( T& currVal, const Float dt, const T& targetVal, const Float smoothTime )
	{
		const Float lambda = dt / smoothTime;
		const T diff = TValueDiffrence< T >( currVal, targetVal );
		currVal = targetVal + diff / ( 1.0f + lambda + 0.5f*lambda*lambda );
	}

	template < typename T >
	RED_INLINE void SmoothExponential( T& currVal, const Float dt, const T& targetVal, const T& maxDiff, const Float smoothTime )
	{
		const Float lambda = dt / smoothTime;
		T diff = TValueDiffrence< T >( currVal, targetVal );
		diff = TClamp< T >( diff, -maxDiff, maxDiff );
		currVal = targetVal + diff / ( 1.0f + lambda + 0.5f*lambda*lambda );
	}

	RED_INLINE Float Interpolate( EInterpolationType type, Float w )
	{
		return type == IT_Bezier ? BezierInterpolation( w ) : w;
	}

	Float RandF( const Float val );

	Float RandF( const Float minVal, Float maxVal );

	RED_INLINE Vector VectorFromAxis( EAxis enumAxis )
	{
		static Vector axis[6] = { Vector::EX, Vector::EY, Vector::EZ, -Vector::EX, -Vector::EY, -Vector::EZ };
		return axis[ enumAxis ];
	}

#ifdef USE_HAVOK_ANIMATION
	RED_INLINE hkVector4 hkVectorFromAxis( EAxis enumAxis )
	{
		static hkVector4 axis[6] = { 
			hkVector4( 1.f, 0.f, 0.f ), 
			hkVector4( 0.f, 1.f, 0.f ), 
			hkVector4( 0.f, 0.f, 1.f ), 
			hkVector4( -1.f, 0.f, 0.f ), 
			hkVector4( 0.f, -1.f, 0.f ), 
			hkVector4( 0.f, 0.f, -1.f ) };
		return axis[ enumAxis ];
	}

	RED_INLINE void hkVectorToTransform( const hkVector4& vec, hkQsTransform& out )
	{
		out.m_rotation.setIdentity();
		out.m_scale.setAll( 1.f );
		out.m_translation = vec;
	}

	RED_INLINE void hkQuaternionToTransform( const hkQuaternion& quat, hkQsTransform& out )
	{
		out.m_rotation = quat;
		out.m_scale.setAll( 1.f );
		out.m_translation.setAll( 0.f );
	}
#else
	//TODO: Chris - Can we refactor this Tomsin?
	RED_INLINE RedVector4 RedVectorFromAxis( EAxis enumAxis )
	{
		static RedVector4 axis[6] = { 
			RedVector4( 1.f, 0.f, 0.f ), 
			RedVector4( 0.f, 1.f, 0.f ), 
			RedVector4( 0.f, 0.f, 1.f ), 
			RedVector4( -1.f, 0.f, 0.f ), 
			RedVector4( 0.f, -1.f, 0.f ), 
			RedVector4( 0.f, 0.f, -1.f ) };
		return axis[ enumAxis ];
	}

	RED_INLINE void hkVectorToTransform( const RedVector4& vec, RedQsTransform& out )
	{
		out.Rotation.SetIdentity();
		out.Scale.SetOnes();
		out.Translation = vec;
	}

	RED_INLINE void hkQuaternionToTransform( const RedQuaternion& quat, RedQsTransform& out )
	{
		out.Rotation = quat;
		out.Scale.SetOnes();
		out.Translation.SetZeros();
	}
#endif
	
	RED_INLINE Int32 ConvertFloatToInt( Float value )
	{
		return (Int32)MRound( value );
	}

	RED_INLINE EulerAngles VectorToAngles( const Vector& vec )
	{
		return EulerAngles( vec.A[0], vec.A[1], vec.A[2] );
	}

	RED_INLINE Float GetTimeFromCompressedBlend( ECompressedPoseBlend type )
	{
		switch ( type )
		{
		case CPBT_None:
			return 0.f;
		case CPBT_Fast:
			return 0.1f;
		case CPBT_Normal:
			return 0.2f;
		case CPBT_Slow:
			return 0.4f;
		default:
			return 0.2f;
		}
	}

	void RotateBoneMS( const AnimQsTransform& parent, AnimQsTransform& child, Float angle, const AnimVector4& axis );
	void RotateBoneLS( AnimQsTransform& bone, Float angle, const AnimVector4& axis );
	void RotateBoneRefS( const AnimQsTransform& ref, AnimQsTransform& bone, Float angle, const AnimVector4& axis );
	void LogQuaternion( const String& label, const AnimQuaternion& quat );
	void CartesianFromSpherical( AnimVector4& sphericalInCartesianOut );
	void SphericalFromCartesian( AnimVector4& cartesianInSphericalOut );

	namespace BlendingUtils
	{
		void BlendAdditive( SBehaviorGraphOutput& output, const SBehaviorGraphOutput& pose, const SBehaviorGraphOutput& additive, Float weight, EAdditiveType type );
		void BlendAdditive_Local( SBehaviorGraphOutput& output, const SBehaviorGraphOutput& pose, const SBehaviorGraphOutput& additive, Float weight );
		void BlendAdditive_Ref( SBehaviorGraphOutput& output, const SBehaviorGraphOutput& pose, const SBehaviorGraphOutput& additive, Float weight );

		void BlendPosesNormal( SBehaviorGraphOutput& output, const SBehaviorGraphOutput& a, const SBehaviorGraphOutput& b, Float alpha );
		void BlendPosesNormal( SBehaviorGraphOutput& poseA, const SBehaviorGraphOutput& poseB, Float weight );
		void BlendPosesNormal( AnimQsTransform* bonesA, const AnimQsTransform* bonesB, Uint32 num, Float weight );

		void BlendPartialPosesNormal( AnimQsTransform* bonesA, const AnimQsTransform* bonesB, Uint32 num, Float weight, const TDynArray< Int32 >& bones );

		void BlendPartialPosesOverride( AnimQsTransform* bonesA, AnimQsTransform* bonesB, Uint32 num, Float weight, const TDynArray< Int32 >& bones );
		void BlendPartialPosesOverride( AnimQsTransform* bonesA, AnimQsTransform* bonesB, Uint32 num, Float weight, const TDynArray< Int32 >& bones, const TDynArray< Float >& boneWeights );

		void SetPoseZero( AnimQsTransform* bones, Uint32 num );
		void SetPoseIdentity( AnimQsTransform* bones, Uint32 num );
		void RenormalizePose( AnimQsTransform* bones, Uint32 num, Float accWeight );
		void RenormalizePoseRotations( AnimQsTransform* bones, Uint32 num );

		void SetTracksZero( Float* tracks, Uint32 num );
		void RenormalizeTracks( Float* tracks, Uint32 num, Float accWeight );
		void BlendTracksNormal( Float* tracksA, const Float* tracksB, Uint32 num, Float weight );

		void SetPoseZero( SBehaviorGraphOutput& pose );
		void SetPoseIdentity( SBehaviorGraphOutput& pose );
		void RenormalizePose( SBehaviorGraphOutput& pose, Float accWeight, Bool renormalizeTracks = true );
		void RenormalizePoseRotations( SBehaviorGraphOutput& pose );
	}
}