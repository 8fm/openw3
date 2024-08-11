/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "animationReporterTodoList.h"
#include "../../common/game/jobTree.h"

//////////////////////////////////////////////////////////////////////////

template < typename T >
struct EdAnimReportResource
{
	String				m_name;
	String				m_path;
	String				m_absPath;

	TSoftHandle< T >	m_resource;

	EdAnimReportResource() {}

	const T* GetResource() const { return m_resource.Get(); }

	void Serialize( IFile& file )
	{
		file << m_name;
		file << m_path;

		m_resource = LoadResource< T >( m_path );

		String depotPath;
		GDepot->GetAbsolutePath( depotPath );
		m_absPath = depotPath + m_path;
	}
};

struct EdAnimReportAnimation;

struct EdAnimReportBehavior : public EdAnimReportResource< CBehaviorGraph >
{
	EdAnimReportBehavior();
	EdAnimReportBehavior( CBehaviorGraph* graph, EdAnimationReporterTodoList& todo );

	Uint32				m_size;
	Uint32				m_blockNums;
	TDynArray< CName >	m_usedAnimations;

	void Serialize( IFile& file );

	static int CmpFuncBySize( const void* elem0, const void* elem1 )
	{
		const EdAnimReportBehavior* record0 = *(const EdAnimReportBehavior**)elem0;
		const EdAnimReportBehavior* record1 = *(const EdAnimReportBehavior**)elem1;
		if ( record0->m_size < record1->m_size ) return 1;
		if ( record0->m_size > record1->m_size ) return -1;
		return 0;
	}

	static int CmpFuncByBlockNums( const void* elem0, const void* elem1 )
	{
		const EdAnimReportBehavior* record0 = *(const EdAnimReportBehavior**)elem0;
		const EdAnimReportBehavior* record1 = *(const EdAnimReportBehavior**)elem1;
		if ( record0->m_blockNums < record1->m_blockNums ) return 1;
		if ( record0->m_blockNums > record1->m_blockNums ) return -1;
		return 0;
	}

	static int CmpFuncByAnimsNums( const void* elem0, const void* elem1 )
	{
		const EdAnimReportBehavior* record0 = *(const EdAnimReportBehavior**)elem0;
		const EdAnimReportBehavior* record1 = *(const EdAnimReportBehavior**)elem1;
		if ( record0->m_usedAnimations.Size() < record1->m_usedAnimations.Size() ) return 1;
		if ( record0->m_usedAnimations.Size() > record1->m_usedAnimations.Size() ) return -1;
		return 0;
	}

	static int CmpFuncByNames( const void* elem0, const void* elem1 )
	{
		const EdAnimReportBehavior* record0 = *(const EdAnimReportBehavior**)elem0;
		const EdAnimReportBehavior* record1 = *(const EdAnimReportBehavior**)elem1;
		if ( record0->m_name < record1->m_name ) return -1;
		if ( record0->m_name > record1->m_name ) return 1;
		return 0;
	}
};

struct EdAnimReportAnimset : public EdAnimReportResource< CSkeletalAnimationSet >
{
	EdAnimReportAnimset();
	EdAnimReportAnimset( CSkeletalAnimationSet* animset, EdAnimationReporterTodoList& todo );
	~EdAnimReportAnimset();

	TDynArray< EdAnimReportAnimation* >			m_animations;

	Uint32	m_animNum;
	Uint32	m_animUsedNum;
	Uint32	m_animUnusedNum;
	Uint32	m_animWithoutPoseNum;
	Uint32	m_animWithoutBoxNum;
	Uint32	m_animInvalidPoses;
	Uint32	m_animInvalid;
	Uint32	m_animSize;

	void Serialize( IFile& file);

	void CollectAnimations( EdAnimationReporterTodoList* todo );

	static int CmpFuncByAnimSize( const void* elem0, const void* elem1 )
	{
		const EdAnimReportAnimset* record0 = *(const EdAnimReportAnimset**)elem0;
		const EdAnimReportAnimset* record1 = *(const EdAnimReportAnimset**)elem1;
		if ( record0->m_animSize < record1->m_animSize ) return 1;
		if ( record0->m_animSize > record1->m_animSize ) return -1;
		return 0;
	}

	static int CmpFuncByAnimNum( const void* elem0, const void* elem1 )
	{
		const EdAnimReportAnimset* record0 = *(const EdAnimReportAnimset**)elem0;
		const EdAnimReportAnimset* record1 = *(const EdAnimReportAnimset**)elem1;
		if ( record0->m_animNum < record1->m_animNum ) return 1;
		if ( record0->m_animNum > record1->m_animNum ) return -1;
		return 0;
	}

	static int CmpFuncByNames( const void* elem0, const void* elem1 )
	{
		const EdAnimReportAnimset* record0 = *(const EdAnimReportAnimset**)elem0;
		const EdAnimReportAnimset* record1 = *(const EdAnimReportAnimset**)elem1;
		if ( record0->m_name < record1->m_name ) return 1;
		if ( record0->m_name > record1->m_name ) return -1;
		return 0;
	}

	static int CmpFuncByAnimUsed( const void* elem0, const void* elem1 )
	{
		const EdAnimReportAnimset* record0 = *(const EdAnimReportAnimset**)elem0;
		const EdAnimReportAnimset* record1 = *(const EdAnimReportAnimset**)elem1;
		if ( record0->m_animUsedNum < record1->m_animUsedNum ) return 1;
		if ( record0->m_animUsedNum > record1->m_animUsedNum ) return -1;
		return 0;
	}

	static int CmpFuncByAnimUnused( const void* elem0, const void* elem1 )
	{
		const EdAnimReportAnimset* record0 = *(const EdAnimReportAnimset**)elem0;
		const EdAnimReportAnimset* record1 = *(const EdAnimReportAnimset**)elem1;
		if ( record0->m_animUnusedNum < record1->m_animUnusedNum ) return 1;
		if ( record0->m_animUnusedNum > record1->m_animUnusedNum ) return -1;
		return 0;
	}

	static int CmpFuncByAnimWithoutPose( const void* elem0, const void* elem1 )
	{
		const EdAnimReportAnimset* record0 = *(const EdAnimReportAnimset**)elem0;
		const EdAnimReportAnimset* record1 = *(const EdAnimReportAnimset**)elem1;
		if ( record0->m_animWithoutPoseNum < record1->m_animWithoutPoseNum ) return 1;
		if ( record0->m_animWithoutPoseNum > record1->m_animWithoutPoseNum ) return -1;
		return 0;
	}

	static int CmpFuncByAnimWithoutBox( const void* elem0, const void* elem1 )
	{
		const EdAnimReportAnimset* record0 = *(const EdAnimReportAnimset**)elem0;
		const EdAnimReportAnimset* record1 = *(const EdAnimReportAnimset**)elem1;
		if ( record0->m_animWithoutBoxNum < record1->m_animWithoutBoxNum ) return 1;
		if ( record0->m_animWithoutBoxNum > record1->m_animWithoutBoxNum ) return -1;
		return 0;
	}

	static int CmpFuncByInvalidPoses( const void* elem0, const void* elem1 )
	{
		const EdAnimReportAnimset* record0 = *(const EdAnimReportAnimset**)elem0;
		const EdAnimReportAnimset* record1 = *(const EdAnimReportAnimset**)elem1;
		if ( record0->m_animInvalidPoses < record1->m_animInvalidPoses ) return 1;
		if ( record0->m_animInvalidPoses > record1->m_animInvalidPoses ) return -1;
		return 0;
	}

	static int CmpFuncByInvalidAnims( const void* elem0, const void* elem1 )
	{
		const EdAnimReportAnimset* record0 = *(const EdAnimReportAnimset**)elem0;
		const EdAnimReportAnimset* record1 = *(const EdAnimReportAnimset**)elem1;
		if ( record0->m_animInvalid < record1->m_animInvalid ) return 1;
		if ( record0->m_animInvalid > record1->m_animInvalid ) return -1;
		return 0;
	}
};

struct EdAnimReportAnimation
{
	EdAnimReportAnimation();
	EdAnimReportAnimation( const EdAnimReportAnimset* parent, const CSkeletalAnimationSetEntry* animEntry, EdAnimationReporterTodoList* todo );

	const CSkeletalAnimationSetEntry* m_animation;

	const EdAnimReportAnimset*	m_parent;

	Int32							m_used;
	Bool						m_isValid;

	CName						m_name;
	Uint32						m_animSize;
	Float						m_duration;
	Uint32						m_motionExSize;
	Uint32						m_motionExNum;

	static int CmpFuncByMotionExNum( const void* elem0, const void* elem1 )
	{
		const EdAnimReportAnimation* record0 = *(const EdAnimReportAnimation**)elem0;
		const EdAnimReportAnimation* record1 = *(const EdAnimReportAnimation**)elem1;
		if ( record0->m_motionExNum < record1->m_motionExNum ) return 1;
		if ( record0->m_motionExNum > record1->m_motionExNum ) return -1;
		return 0;
	}

	static int CmpFuncByAnimSize( const void* elem0, const void* elem1 )
	{
		const EdAnimReportAnimation* record0 = *(const EdAnimReportAnimation**)elem0;
		const EdAnimReportAnimation* record1 = *(const EdAnimReportAnimation**)elem1;
		if ( record0->m_animSize < record1->m_animSize ) return 1;
		if ( record0->m_animSize > record1->m_animSize ) return -1;
		return 0;
	}

	static int CmpFuncByMotionExSize( const void* elem0, const void* elem1 )
	{
		const EdAnimReportAnimation* record0 = *(const EdAnimReportAnimation**)elem0;
		const EdAnimReportAnimation* record1 = *(const EdAnimReportAnimation**)elem1;
		if ( record0->m_motionExSize < record1->m_motionExSize ) return 1;
		if ( record0->m_motionExSize > record1->m_motionExSize ) return -1;
		return 0;
	}

	static int CmpFuncByDuration( const void* elem0, const void* elem1 )
	{
		const EdAnimReportAnimation* record0 = *(const EdAnimReportAnimation**)elem0;
		const EdAnimReportAnimation* record1 = *(const EdAnimReportAnimation**)elem1;
		if ( record0->m_duration < record1->m_duration ) return 1;
		if ( record0->m_duration > record1->m_duration ) return -1;
		return 0;
	}

	static int CmpFuncByNames( const void* elem0, const void* elem1 )
	{
		const EdAnimReportAnimation* record0 = *(const EdAnimReportAnimation**)elem0;
		const EdAnimReportAnimation* record1 = *(const EdAnimReportAnimation**)elem1;
		if ( record0->m_name < record1->m_name ) return -1;
		if ( record0->m_name > record1->m_name ) return 1;
		return 0;
	}

	static int CmpFuncByUsedNum( const void* elem0, const void* elem1 )
	{
		const EdAnimReportAnimation* record0 = *(const EdAnimReportAnimation**)elem0;
		const EdAnimReportAnimation* record1 = *(const EdAnimReportAnimation**)elem1;
		if ( record0->m_used < record1->m_used ) return -1;
		if ( record0->m_used > record1->m_used ) return 1;
		return 0;
	}

	Bool TestSample( const CSkeletalAnimationSetEntry* animEntry, SBehaviorGraphOutput& pose );
};

struct EdAnimReportJobTree : public EdAnimReportResource< CJobTree >
{
	EdAnimReportJobTree();
	EdAnimReportJobTree( CJobTree* jobTree, EdAnimationReporterTodoList& todo );

	void Serialize( IFile& file );
};

struct EdAnimReportAnimEvents : public EdAnimReportResource< CExtAnimEventsFile >
{
	EdAnimReportAnimEvents();
	EdAnimReportAnimEvents( CExtAnimEventsFile* animEvents, EdAnimationReporterTodoList& todo );
};
