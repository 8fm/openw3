/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animationReporterRecords.h"
#include "../../common/game/jobTree.h"
#include "../../common/core/depot.h"
#include "../../common/core/diskFile.h"
#include "../../common/engine/behaviorGraph.h"
#include "../../common/engine/skeleton.h"

//////////////////////////////////////////////////////////////////////////

EdAnimReportAnimation::EdAnimReportAnimation()
{

}

EdAnimReportAnimation::EdAnimReportAnimation( const EdAnimReportAnimset* parent, const CSkeletalAnimationSetEntry* animEntry, EdAnimationReporterTodoList* todo )
{
	m_parent = parent;
	m_animation = animEntry;
	m_name = animEntry->GetName();

	m_duration = animEntry->GetAnimation() ? animEntry->GetAnimation()->GetDuration() : 0.f;
	m_animSize = animEntry->GetAnimation() ? animEntry->GetAnimation()->GetSizeOfAnimBuffer() : 0;
	
	m_motionExSize = 0;//animEntry->GetAnimation() ? animEntry->GetAnimation()->GetSizeOfMotionExtraction() : 0;
	m_motionExNum = 0;//animEntry->GetAnimation() ? animEntry->GetAnimation()->GetNumOfMotionExtractionSamples() : 0;

	m_used = 0;
	m_isValid = true;
}

Bool EdAnimReportAnimation::TestSample( const CSkeletalAnimationSetEntry* animEntry, SBehaviorGraphOutput& pose )
{
	m_isValid = false;

	CSkeletalAnimation* anim = animEntry->GetAnimation();
	if ( anim )
	{
		m_isValid = anim->Sample( 0.f, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks );
	}

	return m_isValid;
}

//////////////////////////////////////////////////////////////////////////

EdAnimReportAnimset::EdAnimReportAnimset()
{
	m_animNum = 0;
	m_animUsedNum = 0;
	m_animUnusedNum = 0;
	m_animSize = 0;
	m_animWithoutPoseNum = 0;
	m_animWithoutBoxNum = 0;
	m_animInvalidPoses = 0;
	m_animInvalid = 0;
}

EdAnimReportAnimset::EdAnimReportAnimset( CSkeletalAnimationSet* animset, EdAnimationReporterTodoList& todo )
{
	m_resource = animset;
	m_animNum = animset->GetNumAnimations();
	m_animUsedNum = 0;
	m_animUnusedNum = 0;
	m_animWithoutPoseNum = 0;
	m_animWithoutBoxNum = 0;
	m_animInvalidPoses = 0;
	m_animInvalid = 0;

	m_name = animset->GetFile() ? animset->GetFile()->GetFileName() : TXT("<unknown>");
	m_path = animset->GetDepotPath();

	m_animSize = 0;

	CollectAnimations( &todo );

	String depotPath;
	GDepot->GetAbsolutePath( depotPath );
	m_absPath = depotPath + m_path;
}

void EdAnimReportAnimset::Serialize( IFile& file)
{
	EdAnimReportResource< CSkeletalAnimationSet >::Serialize( file );

	file << m_animNum;
	file << m_animUsedNum;
	file << m_animUnusedNum;
	file << m_animSize;

	if ( file.IsWriter() )
	{
		Uint32 animSize = m_animations.Size();
		file << animSize;

		for ( Uint32 i=0; i<animSize; ++i )
		{
			EdAnimReportAnimation* anim = m_animations[ i ];

			file << anim->m_name;
			file << anim->m_used;
		}
	}
	else
	{
		CollectAnimations( NULL );

		Uint32 animSize = 0;
		file << animSize;

		for ( Uint32 i=0; i<animSize; ++i )
		{
			CName animName;
			Int32 used;

			file << animName;
			file << used;

			for ( Uint32 j=0; j<m_animations.Size(); ++j )
			{
				EdAnimReportAnimation* anim = m_animations[ j ];
				if ( anim->m_name == animName )
				{
					anim->m_used = used;
					break;
				}
			}
		}
	}
}

void EdAnimReportAnimset::CollectAnimations( EdAnimationReporterTodoList* todo )
{
	const CSkeletalAnimationSet* animset = GetResource();
	if ( !animset )
	{
		return;
	}

	const CSkeleton* skeleton = animset->GetSkeleton();
	SBehaviorGraphOutput* pose = nullptr;
	if( skeleton ) 
	{ 
		pose = new SBehaviorGraphOutput();
		pose->Init( skeleton->GetBonesNum(), skeleton->GetTracksNum() );
	}

	const TDynArray< CSkeletalAnimationSetEntry* >& anims = animset->GetAnimations();
	m_animNum = anims.Size();

	for ( Uint32 i=0; i<m_animNum; ++i )
	{
		const CSkeletalAnimationSetEntry* animEntry = anims[ i ];
		if ( !animEntry )
		{
			if ( todo )
			{
				todo->AddTask( new EdAnimationReporterEmptySlot( animset->GetDepotPath(), String::EMPTY, ARTC_Animset ) );
			}
			continue;
		}

		if ( !animEntry->GetAnimation() )
		{
			if ( todo )
			{
				todo->AddTask( new EdAnimationReporterMissingResource( animset->GetDepotPath(), String::EMPTY, ARTC_Animset ) );
			}
			continue;
		}

		if ( !animEntry->GetAnimation()->IsCompressed() )
		{
			if ( todo )
			{
				todo->AddTask( new EdAnimationReporterAnimNotCompressed( animset->GetDepotPath(), animEntry->GetName() ) );
			}
			continue;
		}

		EdAnimReportAnimation* newAnimRecord = new EdAnimReportAnimation( this, animEntry, todo );
		if ( pose && !newAnimRecord->TestSample( animEntry, *pose ) )
		{
			m_animInvalid++;
		}

		m_animSize += newAnimRecord->m_animSize;

		if ( animEntry->GetAnimation() )
		{
			if ( !animEntry->GetAnimation()->HasBoundingBox() )
			{
				m_animWithoutBoxNum++;
			}

			if ( !animEntry->GetAnimation()->HasCompressedPose() )
			{
				m_animWithoutPoseNum++;
			}
			else if ( pose && !animEntry->GetAnimation()->SampleCompressedPoseWithoutTouch( pose->m_numBones, pose->m_outputPose, pose->m_numFloatTracks, pose->m_floatTracks, skeleton ) )
			{
				m_animInvalidPoses++;
			}
		}

		m_animations.PushBack( newAnimRecord );
	}

	delete pose;
}

EdAnimReportAnimset::~EdAnimReportAnimset()
{
	m_animations.ClearPtr();
}

//////////////////////////////////////////////////////////////////////////

EdAnimReportBehavior::EdAnimReportBehavior()
{

}

EdAnimReportBehavior::EdAnimReportBehavior( CBehaviorGraph* graph, EdAnimationReporterTodoList& todo )
{
	m_resource = graph;

	m_name = graph->GetFile() ? graph->GetFile()->GetFileName() : TXT("<unknown>");
	m_path = graph->GetDepotPath();

	m_size = graph->GetFile() ? GFileManager->GetFileSize( graph->GetFile()->GetAbsolutePath() ) : 0;
	m_blockNums = graph->GetNumberOfNodes();

	graph->EnumUsedAnimations( m_usedAnimations );

	String depotPath;
	GDepot->GetAbsolutePath( depotPath );
	m_absPath = depotPath + m_path;
}

void EdAnimReportBehavior::Serialize( IFile& file )
{
	EdAnimReportResource< CBehaviorGraph >::Serialize( file );

	file << m_size;
	file << m_blockNums;
	file << m_usedAnimations;
}

//////////////////////////////////////////////////////////////////////////

EdAnimReportJobTree::EdAnimReportJobTree()
{

}

EdAnimReportJobTree::EdAnimReportJobTree( CJobTree* jobTree, EdAnimationReporterTodoList& todo )
{
	m_resource = jobTree;

	m_name = jobTree->GetFile() ? jobTree->GetFile()->GetFileName() : TXT("<unknown>");
	m_path = jobTree->GetDepotPath();

	String depotPath;
	GDepot->GetAbsolutePath( depotPath );
	m_absPath = depotPath + m_path;
}

void EdAnimReportJobTree::Serialize( IFile& file )
{
	EdAnimReportResource< CJobTree >::Serialize( file );
}

//////////////////////////////////////////////////////////////////////////

EdAnimReportAnimEvents::EdAnimReportAnimEvents()
{

}

EdAnimReportAnimEvents::EdAnimReportAnimEvents( CExtAnimEventsFile* animEvents, EdAnimationReporterTodoList& todo )
{
	m_resource = animEvents;

	m_name = animEvents->GetFile() ? animEvents->GetFile()->GetFileName() : TXT("<unknown>");
	m_path = animEvents->GetDepotPath();

	String depotPath;
	GDepot->GetAbsolutePath( depotPath );
	m_absPath = depotPath + m_path;
}
