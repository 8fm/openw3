/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once 
#include "../engine/animMath.h"
#include "../core/weakPtr.h"
#include "../core/sharedPtr.h"

class CSkeleton;
class CSkeletalAnimation;
class CRenderSkinningData;
class CMesh;

namespace SwarmMassiveAnimation
{
	/**************************************************************************************************************/
	struct CMassiveAnimationInstance
	{
		DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_MassiveAnimation );

		CMassiveAnimationInstance()
		{
			/* Intentionally Empty */
		}

		CMassiveAnimationInstance( Float timeOffset, CRenderSkinningData* skinningData )
			: m_timeOffset( timeOffset )
			, m_lastFrameIndex( 0 )
			, m_skinningData( skinningData )
		{
			/* Intentionally Empty */
		}

		Float								m_timeOffset;
		Uint32								m_lastFrameIndex;
		CRenderSkinningData*				m_skinningData;			//!< skinning data
	};

	/**************************************************************************************************************/
	class CMassiveAnimation
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_Animation, MC_MassiveAnimation );
	public:

		static CMassiveAnimation* Create( CMeshTypeResource* mesh, CSkeleton* skeleton, CSkeletalAnimationSetEntry* animation );

		CMassiveAnimation();
		CMassiveAnimation( CMeshTypeResource* mesh, CSkeleton* skeleton, CSkeletalAnimationSetEntry* animation );
		~CMassiveAnimation();

		//!
		void UpdateInstance(Float animTime, Uint32 animationInstanceId, Uint32 frameIndex);

		//!
		Uint32 GetBeginningInstanceId( Float animTime ) const;

		//!
		const CRenderSkinningData* GetSkinningDataForInstance( Uint32 animationInstanceId ) const;

	private:
		Float GetAnimationInstanceTime(const CMassiveAnimationInstance* instance, Float animTime) const;
		void GetBonesModelSpace( TDynArray<Matrix>& bonesMS, TDynArray< AnimQsTransform >& animatedBones );

		void Initialize();
		void InitializeInstances();
		void CreateBoneMapping();

	private:
		CMeshTypeResource*						m_mesh;								//!< source mesh
		CSkeleton*								m_skeleton;							//!< source skeleton
		CSkeletalAnimationSetEntry*				m_animation;						//!< animation
		TDynArray< Int16, MC_MassiveAnimation >	m_boneMapping;						//!< Mapping of skinned mesh bones to source skeleton bones
		static const Uint8						MAX_INSTANCE_COUNT = 10;			//!< We can have up to 10 instances per animation
		Uint8									m_instanceCount;					//!< How many instances are there for this animation
		CMassiveAnimationInstance				m_instances[MAX_INSTANCE_COUNT];	//!< Instances

	};

	/**************************************************************************************************************/
	class CMassiveAnimationSet
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_Animation, MC_MassiveAnimation );
	public:

		CMassiveAnimationSet();
		CMassiveAnimationSet( CName name );
		~CMassiveAnimationSet();

		//!
		CMassiveAnimation* GetAnimation( CName animation );

		//!
		CMassiveAnimation* GetDefaultAnimation();

		//! Add animation to set
		Bool AddMassiveAnimation( CName name, CMassiveAnimation* massiveAnimation );

		RED_INLINE CName& GetName() { return m_name; }

	private:
		CName m_name;
		THashMap< CName, CMassiveAnimation*, DefaultHashFunc<CName>, DefaultEqualFunc<CName>, MC_MassiveAnimation >	m_animations;		//!< Animations within that set

	};

	typedef Red::TWeakPtr< CMassiveAnimationSet > MassiveAnimationSetWeakPtr;
	typedef Red::TSharedPtr< CMassiveAnimationSet > MassiveAnimationSetSharedPtr;

	/**************************************************************************************************************/
	class CMassiveAnimationRegister
	{
	public:
		MassiveAnimationSetSharedPtr GetMassiveAnimationSet( CMeshTypeResource* mesh, CSkeleton* skeleton, CSkeletalAnimationSet* animationSet );

	private:
		MassiveAnimationSetSharedPtr CreateMassiveAnimationSet( CName name, CMeshTypeResource* mesh, CSkeleton* skeleton, CSkeletalAnimationSet* animationSet );

	private:
		THashMap< CName, MassiveAnimationSetWeakPtr, DefaultHashFunc<CName>, DefaultEqualFunc<CName>, MC_MassiveAnimation > m_animationSets;

	};

	typedef TSingleton<CMassiveAnimationRegister> GMassiveAnimationRegister;

}	// MassiveAnimation
