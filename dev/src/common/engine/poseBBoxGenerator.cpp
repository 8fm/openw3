
#include "build.h"
#include "behaviorGraphOutput.h"
#include "poseBBoxGenerator.h"
#include "skeleton.h"
#include "animatedIterators.h"

IMPLEMENT_ENGINE_CLASS( CPoseBBoxGenerator );

CPoseBBoxGenerator::CPoseBBoxGenerator()
{

}

void CPoseBBoxGenerator::OnPropertyPostChange( IProperty* property )
{
	if ( property->GetName() == TXT("boneNames") )
	{
		// Remap
		const CSkeleton* skeleton = SafeCast< CSkeleton >( GetParent() );

		const Uint32 size = m_boneNames.Size();

		if ( m_boneIndex.Size() != size )
		{
			m_boneIndex.Resize( size );
		}

		//dex++: switched to skeleton function
		if ( NULL != skeleton )
		{
			for ( Uint32 i=0; i<size; ++i )
			{
				m_boneIndex[ i ] =  skeleton->FindBoneByName( m_boneNames[i].AsChar() );
			}
		}
		else
		{
			Red::System::MemoryZero( m_boneIndex.Data(), sizeof(m_boneIndex) );
		}
		//dex--
	}
}

Bool CPoseBBoxGenerator::GenerateBBox( const CSkeletalAnimation* animation, Box& out ) const
{
	if ( m_boneIndex.Size() == 0 )
	{
		return false;
	}

	//const CSkeleton* skeleton = SafeCast< CSkeleton >( GetParent() );
#ifdef USE_HAVOK_ANIMATION
	const hkaSkeleton* hkSkeleton = skeleton->GetHavokSkeleton();

	if ( !hkSkeleton )
	{
		return false;
	}
		// Clear
	out.Clear();

	// Create pose
	SBehaviorGraphOutput pose( hkSkeleton->m_numBones, hkSkeleton->m_numFloatSlots );

	// MS buffor
	TDynArray< Matrix > bonesMS;
	bonesMS.Resize( pose.m_numBones );

	const Float timeSample = 0.033f;
	const Float duration = animation->GetDuration();
	Float t = 0.f;

	// Sample animation over time
	while ( t <= duration )
	{
		Bool ret = animation->Sample( t, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks );
		if ( !ret )
		{
			return false;
		}

		pose.GetBonesModelSpace( skeleton, bonesMS );

		const Uint32 boneIndexSize = m_boneIndex.Size();
		for ( Uint32 i=0; i<boneIndexSize; ++i )
		{
			const Uint32 bone = m_boneIndex[i];
			out.AddPoint( bonesMS[ bone ].GetTranslation() );
		}

		t += timeSample;
	}

	// Done
	if ( !out.IsEmpty() )
	{
		out.Extrude( 0.25f );
	}
#else
	return false;
#endif
	return true;
}

Bool CPoseBBoxGenerator::IsEmpty() const
{
	return m_boneNames.Empty();
}

void CPoseBBoxGenerator::Fill( const CSkeleton* skeleton )
{
	m_boneIndex.Clear();
	m_boneNames.Clear();

	for ( BoneIterator it( skeleton ); it; ++it )
	{
		m_boneNames.PushBack( ANSI_TO_UNICODE( it.GetName() ) );
		m_boneIndex.PushBack( it.GetIndex() );
	}
}
