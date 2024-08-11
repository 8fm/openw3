
#pragma once

#include "storySceneDebug.h"
#include "sceneLog.h"
#include "storySceneInstanceBuffer.h"

#include "../core/enumBuilder.h"
#include "../core/math.h"

#include "../engine/behaviorIncludes.h"
#include "../engine/cameraDirector.h"

class CSkeleton;
class CComponent;
class CStorySceneLinkElement;

//////////////////////////////////////////////////////////////////////////

#ifndef NO_EDITOR

#define SS_EDITOR_CODE( x ) x
#define SS_EDITOR_CODE_BEGIN
#define SS_EDITOR_CODE_END

#else

#define SS_EDITOR_CODE( x )
#define SS_EDITOR_CODE_BEGIN
#define SS_EDITOR_CODE_END

#endif

//////////////////////////////////////////////////////////////////////////

enum EDialogMimicsLayer
{
	DML_Eyes,
	DML_Pose,
	DML_Animation,
	DML_Override,
	DML_Last
};

//////////////////////////////////////////////////////////////////////////

enum EDialogLookAtType
{
	DLT_Static,
	DLT_Dynamic,
	DLT_StaticPoint,
};

BEGIN_ENUM_RTTI( EDialogLookAtType );
	ENUM_OPTION( DLT_Static );
	ENUM_OPTION( DLT_Dynamic );
	ENUM_OPTION( DLT_StaticPoint );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EStorySceneSignalType
{
	SSST_Accept,
	SSST_Highlight,
	SSST_Skip,
};

BEGIN_ENUM_RTTI( EStorySceneSignalType );
	ENUM_OPTION( SSST_Accept );
	ENUM_OPTION( SSST_Highlight );
	ENUM_OPTION( SSST_Skip );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EStorySceneLineType
{
	SSLT_Normal,
	SSLT_Oneliner,
	SSLT_Subtitle,
};

//////////////////////////////////////////////////////////////////////////

enum EDialogResetClothAndDanglesType
{
	DRCDT_None,
	DRCDT_Reset,
	DRCDT_ResetAndRelax,
};

BEGIN_ENUM_RTTI( EDialogResetClothAndDanglesType );
	ENUM_OPTION( DRCDT_None );
	ENUM_OPTION( DRCDT_Reset );
	ENUM_OPTION( DRCDT_ResetAndRelax );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EStorySceneAnimationType
{
	AAST_Normal,
	AAST_Override,
	AAST_Additive,
};

BEGIN_ENUM_RTTI( EStorySceneAnimationType );
	ENUM_OPTION( AAST_Normal );
	ENUM_OPTION( AAST_Override );
	ENUM_OPTION( AAST_Additive );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

enum EStorySceneMimicsKeyType
{
	SSMKT_OverrideAll,
	SSMKT_OverridePose,
	SSMKT_OverrideAnimation,
	SSMKT_AdditiveAll,
};

BEGIN_ENUM_RTTI( EStorySceneMimicsKeyType );
	ENUM_OPTION( SSMKT_OverrideAll );
	ENUM_OPTION( SSMKT_OverridePose );
	ENUM_OPTION( SSMKT_OverrideAnimation );
	ENUM_OPTION( SSMKT_AdditiveAll );
END_ENUM_RTTI();

enum EStoryScenePoseKeyType
{
	SSPKT_AdditiveAll,
	SSPKT_AdditiveIdle,
};

BEGIN_ENUM_RTTI( EStoryScenePoseKeyType );
	ENUM_OPTION( SSPKT_AdditiveAll );
	ENUM_OPTION( SSPKT_AdditiveIdle );
END_ENUM_RTTI();


enum EDialogActorType
{
	AT_ACTOR	= FLAG( 1 ),
	AT_PROP		= FLAG( 2 ),
	AT_EFFECT	= FLAG( 3 ),
	AT_LIGHT	= FLAG( 4 )
};

//////////////////////////////////////////////////////////////////////////

// DIALOG_TOMSIN_TODO - do wyjebania
struct SceneActorPose
{
	CName	m_state;
	CName	m_personality;

	SceneActorPose()
		: m_state( CName::NONE )
		, m_personality( CName::NONE )
	{
	}

	SceneActorPose( CName state, CName personality )
		: m_state( state )
		, m_personality( personality )
	{
	}
};

//////////////////////////////////////////////////////////////////////////

struct SStorySceneActorAnimationState_HashBase
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_Gameplay );

	CName	m_status;
	CName	m_emotionalState;
	CName	m_poseType;

	CName	m_mimicsEmotionalState;

	SStorySceneActorAnimationState_HashBase( CName status = CName::NONE, CName emotionalState = CName::NONE, CName poseType = CName::NONE, CName mimicsEmotionalState = CName::NONE )
		: m_status( status )
		, m_emotionalState( emotionalState )
		, m_poseType( poseType )
		, m_mimicsEmotionalState( mimicsEmotionalState )
	{}

	RED_FORCE_INLINE Uint32 CalcHash() const
	{
		ASSERT( !( sizeof( SStorySceneActorAnimationState_HashBase ) & 3 ) );
		return Red::System::CalculateHash32FromUint32Array( ( const Uint32* ) this, sizeof( SStorySceneActorAnimationState_HashBase ) >> 2 );
	}
};

struct SStorySceneActorAnimationState : SStorySceneActorAnimationState_HashBase
{
	CName	m_mimicsLayerEyes;
	CName	m_mimicsLayerPose;
	CName	m_mimicsLayerAnimation;

	SStorySceneActorAnimationState( CName status = CName::NONE, CName emotionalState = CName::NONE, CName poseType = CName::NONE, CName mimicsEmotionalState = CName::NONE )
		: SStorySceneActorAnimationState_HashBase( status, emotionalState, poseType, mimicsEmotionalState )
	{}

	Bool operator==( const SStorySceneActorAnimationState& rhs ) const
	{
		return IsBodyEqual( rhs ) && IsMimicEqual( rhs );
	}

	Bool operator<( const SStorySceneActorAnimationState& rhs ) const
	{
		if( m_status != rhs.m_status )
		{
			return m_status < rhs.m_status;
		}			
		if( m_emotionalState != rhs.m_emotionalState )
		{
			return m_emotionalState < rhs.m_emotionalState;
		}
		if( m_poseType != rhs.m_poseType )
		{
			return m_poseType < rhs.m_poseType;
		}
		return m_mimicsEmotionalState < rhs.m_mimicsEmotionalState;
	}
 
	Bool IsBodyEqual( const SStorySceneActorAnimationState& rhs ) const
	{
		return ( m_status == rhs.m_status || rhs.m_status == CName::NONE ) && 
			   ( m_emotionalState == rhs.m_emotionalState || rhs.m_emotionalState == CName::NONE ) &&
			   ( m_poseType == rhs.m_poseType || rhs.m_poseType == CName::NONE);
	}

	Bool IsMimicEqual( const SStorySceneActorAnimationState& rhs ) const
	{
		return	m_mimicsLayerEyes == rhs.m_mimicsLayerEyes &&
				m_mimicsLayerPose == rhs.m_mimicsLayerPose &&
				m_mimicsLayerAnimation == rhs.m_mimicsLayerAnimation;
	}

	Bool IsBodySet() const
	{
		return m_status != CName::NONE && m_emotionalState != CName::NONE && m_poseType != CName::NONE;
	}

	Bool IsMimicSet() const
	{
		return m_mimicsEmotionalState != CName::NONE;
	}

	Bool IsSomethingSet() const
	{
		return m_status || m_emotionalState || m_poseType || m_mimicsEmotionalState || m_mimicsLayerEyes || m_mimicsLayerPose || m_mimicsLayerAnimation;
	}

	void CopyBodyData( const SStorySceneActorAnimationState& rhs )
	{
		m_status = rhs.m_status;
		m_emotionalState = rhs.m_emotionalState;
		m_poseType = rhs.m_poseType;
	}

	void CopyMimicsData( const SStorySceneActorAnimationState& rhs )
	{
		m_mimicsEmotionalState = rhs.m_mimicsEmotionalState;
		m_mimicsLayerEyes = rhs.m_mimicsLayerEyes;
		m_mimicsLayerPose = rhs.m_mimicsLayerPose;
		m_mimicsLayerAnimation = rhs.m_mimicsLayerAnimation;
	}

	void ToString( String& str ) const
	{
		str = String::Printf( TXT("Status: %s; Emo state: %s; Pose: %s; Mimics emo state: %s"), m_status.AsString().AsChar(), m_emotionalState.AsString().AsChar(), m_poseType.AsString().AsChar(), m_mimicsEmotionalState.AsString().AsChar() );
	}
};

#ifndef NO_EDITOR
struct SStorySceneEventGeneratorAnimationState
{
	SStorySceneEventGeneratorAnimationState()
		: m_animSpeedRange( 1.f, 1.f ), m_animWeightRange( 1.f, 1.f )
	{}
	TDynArray<CName> m_tags;
	TDynArray<SStorySceneActorAnimationState> m_useOnlyWithManStates;
	TDynArray<SStorySceneActorAnimationState> m_useOnlyWithWomanStates;
	TPair<Float, Float>	m_animSpeedRange;
	TPair<Float, Float>	m_animWeightRange;
};

#endif

//////////////////////////////////////////////////////////////////////////

class IDialogBodyAnimationFilterInterface
{
public:
	virtual const CName& GetBodyFilterStatus() const = 0;
	virtual const CName& GetBodyFilterEmotionalState() const = 0;
	virtual const CName& GetBodyFilterPoseName() const = 0;
	virtual const CName& GetBodyFilterTypeName() const = 0;
	virtual const CName& GetBodyFilterActor() const = 0;
};

class IDialogMimicsAnimationFilterInterface
{
public:
	virtual const CName& GetMimicsActionFilter() const { return CName::NONE; }
	virtual const CName& GetMimicsFilterEmotionalState() const { return CName::NONE; }
	virtual const CName& GetMimicsFilterActor() const = 0;
};

//////////////////////////////////////////////////////////////////////////

#ifndef NO_EDITOR
class IDialogBodyPartOwner
{
public:
	virtual const CSkeleton* GetBodyPartSkeleton( const CComponent* c ) const;
	virtual TDynArray< SBehaviorGraphBoneInfo >* GetBodyPartBones() = 0;

	virtual void OnBodyPartsChanged() {}
};
#endif

//////////////////////////////////////////////////////////////////////////

// DIALOG_TOMSIN_TODO - do wyjebania

// Should match Flash witcher3.views.storydialog.DialogActionType
// Deprecated W2 enums left in for now as documentation until we start
// seeing real dialogs.
enum EDialogActionIcon
{
	DialogAction_NONE				= 1<<0,
	DialogAction_AXII				= 1<<1, // DEPRECATED: W2 only
	DialogAction_CONTENT_MISSING	= 1<<2, // W3 new
	DialogAction_BRIBE				= 1<<3, // W3
	DialogAction_HOUSE				= 1<<4, // W3
	DialogAction_PERSUASION			= 1<<5, // DEPRECATED: W2 only
	DialogAction_GETBACK			= 1<<6, // W3
	DialogAction_GAME_DICES			= 1<<7, // W3
	DialogAction_GAME_FIGHT			= 1<<8, // W3
	DialogAction_GAME_WRESTLE		= 1<<9, // W3	
	DialogAction_CRAFTING			= 1<<10, // DEPRECATED: W2 only
	DialogAction_SHOPPING			= 1<<11, // W3
	DialogAction_TimedChoice		= 1<<12, // W3 new
	DialogAction_EXIT				= 1<<13, // W3
	DialogAction_HAIRCUT			= 1<<14, // W3
	DialogAction_MONSTERCONTRACT	= 1<<15, // W3 new
	DialogAction_BET				= 1<<16, // W3 new
	DialogAction_STORAGE			= 1<<17, // DEPRECATED: W2 only
	DialogAction_GIFT				= 1<<18, // W3
	DialogAction_GAME_DRINK			= 1<<19, // W3
	DialogAction_GAME_DAGGER		= 1<<20, // W3 new
	DialogAction_SMITH				= 1<<21, // W3 new
	DialogAction_ARMORER			= 1<<22, // W3 new
	DialogAction_RUNESMITH			= 1<<23, // W3 new
	DialogAction_TEACHER			= 1<<24, // W3 new
	DialogAction_FAST_TRAVEL		= 1<<25, // W3 new
	DialogAction_GAME_CARDS			= 1<<26, // W3 new
	DialogAction_SHAVING			= 1<<27, // W3 new
	DialogAction_AUCTION			= 1<<28, // W3 new
	DialogAction_LEVELUP1			= 1<<29, // W3 new
	DialogAction_LEVELUP2			= 1<<30, // W3 new
	DialogAction_LEVELUP3			= 1<<31  // W3 new
};

BEGIN_ENUM_RTTI( EDialogActionIcon )
	ENUM_OPTION( DialogAction_NONE )
	ENUM_OPTION( DialogAction_AXII )
	ENUM_OPTION( DialogAction_CONTENT_MISSING )
	ENUM_OPTION( DialogAction_BRIBE )
	ENUM_OPTION( DialogAction_HOUSE )
	ENUM_OPTION( DialogAction_PERSUASION )
	ENUM_OPTION( DialogAction_GETBACK )
	ENUM_OPTION( DialogAction_GAME_DICES )
	ENUM_OPTION( DialogAction_GAME_FIGHT )
	ENUM_OPTION( DialogAction_GAME_WRESTLE )
	ENUM_OPTION( DialogAction_CRAFTING )
	ENUM_OPTION( DialogAction_SHOPPING )
	ENUM_OPTION( DialogAction_EXIT )
	ENUM_OPTION( DialogAction_HAIRCUT )
	ENUM_OPTION( DialogAction_MONSTERCONTRACT )
	ENUM_OPTION( DialogAction_BET )
	ENUM_OPTION( DialogAction_STORAGE )
	ENUM_OPTION( DialogAction_GIFT )
	ENUM_OPTION( DialogAction_GAME_DRINK )
	ENUM_OPTION( DialogAction_GAME_DAGGER )
	ENUM_OPTION( DialogAction_SMITH )
	ENUM_OPTION( DialogAction_ARMORER )
	ENUM_OPTION( DialogAction_RUNESMITH )
	ENUM_OPTION( DialogAction_TEACHER )
	ENUM_OPTION( DialogAction_FAST_TRAVEL )
	ENUM_OPTION( DialogAction_GAME_CARDS )
	ENUM_OPTION( DialogAction_SHAVING )
	ENUM_OPTION( DialogAction_TimedChoice )
	ENUM_OPTION( DialogAction_AUCTION )
	ENUM_OPTION( DialogAction_LEVELUP1 )
	ENUM_OPTION( DialogAction_LEVELUP2 )
	ENUM_OPTION( DialogAction_LEVELUP3 )
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////
class CStorySceneChoice;

struct SSceneChoice
{
	DECLARE_RTTI_STRUCT( SSceneChoice );

	const CStorySceneLinkElement*	link;
	const CStorySceneChoice*		m_choiceToReturnTo;
	String							m_description;
	Uint32							m_order;
	Bool							m_emphasised;
	Bool							m_previouslyChoosen;
	Bool							m_disabled;
	EDialogActionIcon				m_dialogAction;
	CName							m_playGoChunk;
	Bool							m_injectedChoice;

	SSceneChoice()
		: link( nullptr )
		, m_choiceToReturnTo( nullptr )
		, m_description( String::EMPTY )
		, m_order( 0 )
		, m_emphasised( false )
		, m_previouslyChoosen( false )
		, m_disabled( false )
		, m_dialogAction( DialogAction_NONE )
		, m_injectedChoice( false )
	{
	}

	SSceneChoice( const CStorySceneLinkElement* _link, const String& _description, Uint32 order = 0, Bool previouslyChoosen = false, Bool emphasised = false )
		: link( _link )
		, m_choiceToReturnTo( nullptr )
		, m_description( _description )
		, m_order( order )
		, m_previouslyChoosen( previouslyChoosen )
		, m_emphasised( emphasised )
	{}

	RED_INLINE Bool operator== ( const SSceneChoice& other ) const
	{
		return link == other.link && m_description == other.m_description && m_order == other.m_order;
	}

	RED_INLINE Bool operator< ( const SSceneChoice& other ) const
	{
		return m_order < other.m_order;
	}

	RED_INLINE Bool operator<= ( const SSceneChoice& other ) const
	{
		return m_order <= other.m_order;
	}

	RED_INLINE Bool operator> ( const SSceneChoice& other ) const
	{
		return m_order > other.m_order;
	}
};

BEGIN_CLASS_RTTI( SSceneChoice );
	PROPERTY( m_description );
	PROPERTY( m_emphasised );
	PROPERTY( m_previouslyChoosen );
	PROPERTY( m_disabled );
	PROPERTY( m_dialogAction );
	PROPERTY( m_playGoChunk );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

RED_WARNING_PUSH();
RED_DISABLE_WARNING_MSC( 4324 );

struct StorySceneCameraState
{
	Bool								m_valid;
	Vector								m_position;
	EulerAngles							m_rotation;
	Vector								m_rawPosition; // position and rotation without adjustments
	EulerAngles							m_rawRotation; //
	Matrix								m_localToWorld;
	Float								m_fov;
	SDofParams							m_dof;
	Int32								m_cameraUniqueID;

	StorySceneCameraState() 
		: m_valid( false )
		, m_position( Vector::ZERO_3D_POINT )
		, m_rotation( EulerAngles::ZEROS )
		, m_rawPosition( Vector::ZERO_3D_POINT )
		, m_rawRotation( EulerAngles::ZEROS )
		, m_localToWorld( Matrix::IDENTITY )
		, m_fov( 50.f )
	{}
};
RED_WARNING_POP();


//////////////////////////////////////////////////////////////////////////

#define SCENE_DELETE( x ) delete (x);
//#define SCENE_DELETE( x ) ;

class IStorySceneObject
{
public:
	void *operator new( size_t size );
	void operator delete( void *ptr );
};

//////////////////////////////////////////////////////////////////////////

void CallMyPlaceForBreakpoint( const Char* msgFile, const Uint32 lineNum, const Char* msg );

#ifdef DEBUG_SCENES_2
#define SCENE_ASSERT( expression, ... ) RED_ASSERT( expression )
#define SCENE_ASSERT__FIXME_LATER( expression, ... )
#else
#define SCENE_ASSERT( expression, ... ) if ( !( expression ) ) CallMyPlaceForBreakpoint( MACRO_TXT( __FILE__ ), __LINE__, TXT( #expression ) );
#define SCENE_ASSERT__FIXME_LATER( expression, ... )
#endif

#define SCENE_VERIFY( x ) RED_VERIFY( x )

//////////////////////////////////////////////////////////////////////////

enum ELightTrackingType
{
	LTT_Normal,
	LTT_Reverse,
};

BEGIN_ENUM_RTTI( ELightTrackingType )
	ENUM_OPTION( LTT_Normal )
	ENUM_OPTION( LTT_Reverse )
END_ENUM_RTTI()


struct SStorySceneLightTrackingInfo
{
	DECLARE_RTTI_STRUCT( SStorySceneLightTrackingInfo );

	Bool			m_enable;
	SSimpleCurve	m_radius;
	SSimpleCurve    m_angleOffset;
	ELightTrackingType m_trackingType;

	SStorySceneLightTrackingInfo()
		: m_enable( false )
	{
		m_angleOffset.Reset( SCT_Float, 15.f, 0.f );
		m_angleOffset.AddPoint( 0.1f, SSimpleCurvePoint::BuildValue( Color::WHITE, 0.f ) );

		m_radius.Reset( SCT_Float, 0.5f, 0.f );
		m_radius.AddPoint( 0.1f, SSimpleCurvePoint::BuildValue( Color::WHITE, 1.f ) );
	}

	Bool Empty() const { return m_enable; }
};

BEGIN_CLASS_RTTI( SStorySceneLightTrackingInfo )
	PROPERTY_EDIT( m_enable, TXT( "" ) );
	PROPERTY_EDIT( m_trackingType, TXT("") )
	PROPERTY_EDIT( m_radius, TXT( "" ) );
	PROPERTY_EDIT( m_angleOffset, TXT( "" ) );
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

struct SStorySceneAttachmentInfo
{
	DECLARE_RTTI_STRUCT( SStorySceneAttachmentInfo );

	CName			m_attachTo;
	CName			m_parentSlotName;
	Bool			m_freePositionAxisX;
	Bool			m_freePositionAxisY;
	Bool			m_freePositionAxisZ;
	Bool			m_freeRotation;

	SStorySceneAttachmentInfo()
		: m_freePositionAxisX( false )
		, m_freePositionAxisY( false )
		, m_freePositionAxisZ( false )
		, m_freeRotation( true )
	{}

	Uint32 GetAttachmentFlags() const;
	Bool Empty() const { return m_attachTo == CName::NONE || m_parentSlotName == CName::NONE; }
};

BEGIN_CLASS_RTTI( SStorySceneAttachmentInfo )
	PROPERTY_CUSTOM_EDIT( m_attachTo, TXT( "" ), TXT("DialogVoicePropTag") );
	PROPERTY_EDIT( m_parentSlotName, TXT( "" ) );
	PROPERTY_EDIT( m_freePositionAxisX, TXT( "" ) );
	PROPERTY_EDIT( m_freePositionAxisY, TXT( "" ) );
	PROPERTY_EDIT( m_freePositionAxisZ, TXT( "" ) );
	PROPERTY_EDIT( m_freeRotation, TXT( "" ) );
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CStorySceneAnimationContainer
{
	struct Record
	{
		CName				m_id;
		TDynArray< CName >	m_bodyAnimations;
		TDynArray< CName >	m_mimicAnimations;
		TDynArray< SStorySceneActorAnimationState > m_bodyIdles;
		TDynArray< SStorySceneActorAnimationState > m_mimicIdles;

		Int32				m_bodyAnimationsPreloadIdx;
		Int32				m_mimicAnimationsPreloadIdx;
		Int32				m_bodyIdlesPreloadIdx;
		Int32				m_mimicIdlesPreloadIdx;

		Record();

		void AddBodyAnimation( const CName& animation );
		void AddMimicAnimation( const CName& animation );

		void AddBodyIdle( const SStorySceneActorAnimationState& state );
		void AddMimicIdle( const SStorySceneActorAnimationState& state );
	};

	TDynArray< Record >							m_data;
	TDynArray< THandle< CSkeletalAnimation > >	m_anims;
	Bool										m_valid;
	const CSkeletalAnimation*					m_lastUnstreamedAnim;
	Float										m_lastUnstreamedAnimDuration;
	TSet< const CSkeletalAnimation* >			m_corruptedAnims;

public:
	CStorySceneAnimationContainer();
	~CStorySceneAnimationContainer();

	void AddBodyAnimation( const CName& actorId, const CName& animation );
	void AddMimicAnimation( const CName& actorId, const CName& animation );

	void AddBodyIdle( const CName& actorId, const CName& status, const CName& emotionalState, const CName& poseName );
	void AddMimicIdle( const CName& actorId, const CName& mimicsLayer_Eyes, const CName& mimicsLayer_Pose, const CName& mimicsLayer_Animation );

	struct SPreloadedAnimFunc
	{
		virtual void OnAnimationPreloaded( const CEntity* e, const CSkeletalAnimationSetEntry* a ) = 0;
	};

	void PreloadNewAnimations( const CStoryScenePlayer* p, SPreloadedAnimFunc* func = nullptr );
	void UnloadAllAnimations();

	Bool WaitForAllAnimations() const;

	Bool HasAnimation( const CSkeletalAnimationSetEntry* anim ) const;

private:
	Record* FindRecord( const CName& id );

public:
	class Iterator
	{
		Uint32									m_index;
		CStorySceneAnimationContainer&			m_container;

	public:
		Iterator( CStorySceneAnimationContainer& c );
		operator Bool () const;
		void operator++ ();
		const CSkeletalAnimation* operator*() const;
	};

private:
	const CStorySceneAnimationContainer& operator=( const CStorySceneAnimationContainer& );
};

//////////////////////////////////////////////////////////////////////////
