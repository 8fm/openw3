#include "build.h"
#include "r4HumanoidCombatComponent.h"

#include "..\..\common\engine\mesh.h"
#include "..\..\common\engine\meshTypeComponent.h"
#include "..\..\common\engine\component.h"
#include "..\..\common\engine\skeleton.h"
#include "..\..\common\core\gatheredResource.h"
#include "..\..\common\core\handleMap.h"
#include "..\..\common\core\indexed2dArray.h"

IMPLEMENT_ENGINE_CLASS( CR4HumanoidCombatComponent );
IMPLEMENT_ENGINE_CLASS( SSoundInfoMapping );

CGatheredResource resSoundInfos( TXT("gameplay\\globals\\sound_info.redicsv"), RGF_Startup );

const SSoundInfoMapping SSoundInfoMapping::EMPTY = SSoundInfoMapping();
CSoundInfoMappingHelper* CSoundInfoMappingHelper::st_instance = nullptr;

CSoundInfoMappingHelper::CSoundInfoMappingHelper()
	: m_firstBoneIndex( 0 )
	, m_lastBoneIndex( 0 )
{
	m_array = resSoundInfos.LoadAndGet< CIndexed2dArray >();

	if( m_array )
	{
		// Cache bone indexes
		CreateBoneIndexesCaching();	
	}	
}

void CSoundInfoMappingHelper::CreateBoneIndexesCaching()
{
	for(Int32 i = 1; ; ++i)
	{
		String boneHeader = String::EMPTY;
		boneHeader = String::Printf( TXT("BoneName%d"), i);
		const Int32 index = m_array->GetColumnIndex( boneHeader );
		if( index != -1 )
		{
			if( m_firstBoneIndex == -1)
			{
				m_firstBoneIndex = index;
			}
			m_lastBoneIndex = index + 1;
		}
		else
		{
			break;
		}
	}
}

CSoundInfoMappingHelper* CSoundInfoMappingHelper::GetInstance()
{
	if( !st_instance )
	{
		st_instance = new CSoundInfoMappingHelper();
	}
	return st_instance; 
}

Bool CSoundInfoMappingHelper::FillMapping(const CActor* parent, CName soundInfoName, SSoundInfoMapping& newMapping) const
{
	Bool retValue = false;
	if( soundInfoName != CName::NONE )
	{
		const Int32 rowIndex = m_array->GetRowIndex( soundInfoName );
		if( rowIndex != -1 )
		{
			const CAnimatedComponent* ac = parent->GetRootAnimatedComponent();
			if( ac ) 
			{
				const CSkeleton * skele = ac->GetSkeleton();
				if( skele )
				{
					Int32 isDefault = 0;
					retValue = true;
					if( C2dArray::ConvertValue( m_array->GetValue( TXT( "IsDefault" ), rowIndex), isDefault) )
					{
						newMapping.m_isDefault = isDefault > 0 ? true : false;
					}

					// Now read the bones mapped to this piece of "sound mesh"
					newMapping.m_boneIndexes.Clear();
					for(Int32 i = m_firstBoneIndex; i < m_lastBoneIndex; ++i)
					{
						const String& boneNameStr = m_array->GetValue( i , rowIndex);

						if( boneNameStr == String::EMPTY )
						{
							// No BoneNameX, possibly the soundInfo has less bones mapped
							// Nevertheless, we'll check all the bones just in case someone decided to make a break in the middle...
							continue;
						}
						const CName boneName = CName( boneNameStr );

						// We got another bone name, add to the mapping
						Int32 boneIndex = skele->FindBoneByName( boneName );
						if( boneIndex != -1 )
						{
							//newMapping.m_boneIndexes.Set( boneIndex, boneName );
							newMapping.m_boneIndexes.PushBackUnique( boneIndex );
						}
					}
				}
			}
		}
		else if(soundInfoName == CName(TXT("TorsoArmor")))
		{
			newMapping.m_isDefault = true;
			const CAnimatedComponent* ac = parent->GetRootAnimatedComponent();
			if( ac ) 
			{
				const CSkeleton * skele = ac->GetSkeleton();
				if( skele )
				{

					retValue = true;

					CName boneName = CName( TXT("torso3") );

					// We got another bone name, add to the mapping
					Int32 boneIndex = skele->FindBoneByName( boneName );
					if( boneIndex != -1 )
					{
						//newMapping.m_boneIndexes.Set( boneIndex, boneName );
						newMapping.m_boneIndexes.PushBackUnique( boneIndex );
					}

					boneName = CName(TXT("torso"));
					boneIndex = skele->FindBoneByName( boneName );
					if( boneIndex != -1 )
					{
						//newMapping.m_boneIndexes.Set( boneIndex, boneName );
						newMapping.m_boneIndexes.PushBackUnique( boneIndex );
					}
				}
			}
		}
	}



	return retValue;
}


CR4HumanoidCombatComponent::CR4HumanoidCombatComponent( ) 
	: m_pelvisBone( CNAME( pelvis ) )
	, m_default( )
	, m_firstBoneInSkeletonIndex( -1 )
	, m_lastBoneInSkeletonIndex( -1 )
{
}

CR4HumanoidCombatComponent::~CR4HumanoidCombatComponent( )
{
}

const SSoundInfoMapping& CR4HumanoidCombatComponent::GetSoundInfoMapping(Int32 boneIndex) const
{
	for( auto it = m_mappings.Begin(), end = m_mappings.End(); it != end; ++it )
	{
		const SSoundInfoMapping& map = (*it).m_second;

		const auto boneIndexPtr = map.m_boneIndexes.FindPtr( boneIndex );
		if( boneIndexPtr != nullptr )
		{
			return map;
		}
	}
	return SSoundInfoMapping::EMPTY;
}

Bool CR4HumanoidCombatComponent::InsertNewMapping(const CName& soundInfoName, const CName& soundType, const CName& soundSize)
{
	const CActor * parent = Cast< CActor >( GetParent() );
	SSoundInfoMapping newMapping;
	Bool retVal = false;
	if( parent )
	{
		const CSoundInfoMappingHelper* soundInfosHelper = CSoundInfoMappingHelper::GetInstance();
		
		// Create a new mapping and get base info		
		if( soundInfosHelper->FillMapping(parent, soundInfoName, newMapping) )
		{
			newMapping.m_soundSizeIdentification = soundSize;
			newMapping.m_soundTypeIdentification = soundType;

			m_mappings.Set( soundInfoName, newMapping );
			if( newMapping.m_isDefault )
			{
				m_default = newMapping;				
			}
			retVal = true;
		}
	}

	return retVal;
}

void CR4HumanoidCombatComponent::CacheSkeletonBoneIndexes(const CActor * parent)
{
	m_firstBoneInSkeletonIndex = 0;
	m_lastBoneInSkeletonIndex = 0;
	if( parent )
	{
		const CAnimatedComponent* ac = parent->GetRootAnimatedComponent();
		if( ac ) 
		{
			const CSkeleton * skele = ac->GetSkeleton();
			if( skele )
			{
				m_firstBoneInSkeletonIndex = 0;
				m_lastBoneInSkeletonIndex = skele->GetBonesNum();

				if ( m_lastBoneInSkeletonIndex > 0 )
				{
					if(skele->HasLod_1())
					{
						m_lastBoneInSkeletonIndex = skele->GetLodBoneNum_1();
					}
					const Int32 pelvisIndex = skele->FindBoneByName( m_pelvisBone );
					m_firstBoneInSkeletonIndex = pelvisIndex > 0 ? pelvisIndex : 0;
				}
			}
		}
	}
}

void CR4HumanoidCombatComponent::funcUpdateSoundInfo( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	const CActor * parent = Cast< CActor >( GetParent() );

	if( parent )
	{
		const CAnimatedComponent* ac = parent->GetRootAnimatedComponent();
		if( ac ) 
		{
			TDynArray< CComponent* > macComponents;
			ac->GetAllChildAttachedComponents(macComponents);

			// Clear all created mappings
			m_mappings.Clear();
			
			// Iterate through components and update mappings from Meshes.
			for (auto it = macComponents.Begin(), endIt = macComponents.End(); it != endIt; ++it )
			{
				const CMeshTypeComponent* component = Cast< CMeshTypeComponent >((*it));

				if( !component )
				{
					continue;
				}

				const CMeshTypeResource* mtrPtr = component->GetMeshTypeResource();

				if( !mtrPtr )
				{
					continue;
				}

				const CMesh* mesh = Cast< const CMesh >( mtrPtr);
				if( !mesh )
				{
					continue;
				}
				const SMeshSoundInfo* meshSI = mesh->GetMeshSoundInfo();
				if( meshSI )
				{
					const CName& soundInfoName = meshSI->m_soundBoneMappingInfo;

					InsertNewMapping(soundInfoName,  meshSI->m_soundTypeIdentification, meshSI->m_soundSizeIdentification );
				}

			}
		}
	}
	RETURN_VOID();
}

void CR4HumanoidCombatComponent::funcGetSoundTypeIdentificationForBone( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Int32, boneIndex, -1 );
	FINISH_PARAMETERS;

	CName retValue = CName::NONE;
	
	// We see if we have a mapping that uses that bone
	const SSoundInfoMapping& maping = GetSoundInfoMapping(boneIndex);
	if( maping.m_soundTypeIdentification != CName::NONE )
	{
		retValue = maping.m_soundTypeIdentification;
	}
	// If we don't, we try to give the value from the default mapping
	else 
	{
		retValue = m_default.m_soundTypeIdentification;
	}

	RETURN_NAME( retValue );
}

void CR4HumanoidCombatComponent::funcGetBoneClosestToEdge( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Vector, b, Vector::ZEROS  );
	GET_PARAMETER_OPT( Bool, preciseSearch, false  );
	FINISH_PARAMETERS;

	Int32 boneIndex = -1;
	Float minDist = FLT_MAX;

	const CActor * parent = Cast< CActor >( GetParent() );

	if( parent )
	{
		// If we have mappings and we don't want precise (sic!) result,
		// we check only bones from them to get best result
		if( m_mappings.Size() > 0 && !preciseSearch )
		{
			for( auto it = m_mappings.Begin(), end = m_mappings.End(); it != end; ++it )
			{
				const SSoundInfoMapping& map = (*it).m_second;
				const auto& boneIndexes = map.m_boneIndexes;
				const Int32 biSize = boneIndexes.Size();
				for( Int32 j = 0 ; j < biSize; ++j )
				{
					Matrix boneWorldMatrix( Matrix::IDENTITY );
					const Int32& currBone = boneIndexes[j];
					parent->GetSubObjectWorldMatrix( currBone, boneWorldMatrix );
					const Vector& pt = boneWorldMatrix.GetTranslationRef();
					Float dist =  pt.DistanceToEdge( a, b );

					if(dist < minDist)
					{
						minDist = dist;
						boneIndex = currBone;
					}
				}
			}		
		}
		else
		{
			// First time using precise search, we cache boneIndexes range
			if( m_firstBoneInSkeletonIndex < 0)
			{
				CacheSkeletonBoneIndexes( parent );
			}

			if ( m_lastBoneInSkeletonIndex > 0 )
			{
				for(Int32 i = m_firstBoneInSkeletonIndex; i < m_lastBoneInSkeletonIndex; i++) 
				{
					Matrix boneWorldMatrix( Matrix::IDENTITY );	
					parent->GetSubObjectWorldMatrix( i, boneWorldMatrix );
					const Vector& pt = boneWorldMatrix.GetTranslationRef();
					Float dist =  pt.DistanceToEdge( a, b );

					if(dist < minDist)
					{
						minDist = dist;
						boneIndex = i;
					}
				}
			}
		}
	}
	RETURN_INT( boneIndex );
}

void CR4HumanoidCombatComponent::funcGetDefaultSoundInfoMapping( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_STRUCT( SSoundInfoMapping , m_default );
}