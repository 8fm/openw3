/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animationLogger.h"

#ifdef USE_ANIMATION_LOGGER

#include "skeletalAnimation.h"
#include "animatedComponent.h"
#include "skeletalAnimationContainer.h"
#include "skeleton.h"

AnimationLogger::AnimationLogger()
{
	RED_FATAL_ASSERT( SIsMainThread(), "AnimationLogger can only be called from main thread" );

	m_path = GFileManager->GetBaseDirectory() + TXT("log_anim.txt");

	m_missingAnimMap.Reserve( 1000 );
	m_requestedAnimMap.Reserve( 1000 );
	m_sampledAnimMap.Reserve( 1000 );

	LoadFile();
}

namespace
{
	Int32 GetNumSoundEvents( const CSkeletalAnimation* animation )
	{
		static CClass* c = SRTTI::GetInstance().FindClass( CName( TXT("CExtAnimSoundEvent") ) );
		if ( const CSkeletalAnimationSetEntry* entry = animation->GetEntry() )
		{
			return entry->GetNumEventsOfType( c );
		}

		return -1;
	}
}

void AnimationLogger::Log( const CSkeletalAnimation* animation, EAnimationLogType logType )
{
	Bool added = false;

	const CSkeletalAnimationSetEntry* entry = animation->GetEntry();
	ASSERT( entry );
	if ( entry )
	{
		const CSkeletalAnimationSet* animSet = entry->GetAnimSet();
		ASSERT( animSet );
		if ( animSet )
		{
			AnimationData data;
			data.m_resourcePath = animSet->GetDepotPath();
			data.m_numSoundEvents = GetNumSoundEvents( animation );

			CName animationName = animation->GetName();

			if ( AddToMap( animationName, data, logType ) )
			{
				WriteToFile( animationName, data, logType );
			}

			added = true;
		}
	}

	if ( !added )
	{
		Log( animation->GetName(), nullptr, logType );
	}
}

void AnimationLogger::Log( const CName& animation, const CSkeletalAnimationContainer* cont, EAnimationLogType logType )
{
	String path( TXT("<no path>") );

	if ( cont && cont->GetParent() && cont->GetParent()->IsA< CAnimatedComponent >() )
	{
		const CAnimatedComponent* ac = static_cast< const CAnimatedComponent* >( cont->GetParent() );
		if ( const CSkeleton* skeleton = ac->GetSkeleton() )
		{
			path = skeleton->GetDepotPath();
		}
	}

	AnimationData data;
	data.m_resourcePath = path;
	data.m_numSoundEvents = -1;

	if ( AddToMap( animation, data, logType ) )
	{
		WriteToFile( animation, data, logType );
	}
}

Bool AnimationLogger::AddToMap( const CName& animation, const AnimationData& data, EAnimationLogType logType )
{	
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );

	switch ( logType )
	{
		case EALT_MissingAnim:
			if( m_missingAnimMap.Insert( animation, data ) ) return true;
			break;

		case EALT_RequestedAnim:
			if( m_requestedAnimMap.Insert( animation, data ) ) return true;
			break;

		case EALT_SampledAnim:
			if( m_sampledAnimMap.Insert( animation, data ) ) return true;
			break;

		default:
			ASSERT( 0 );
			break;
	}
	return false;
}

void AnimationLogger::WriteToFile( const CName& str, const AnimationData& data, EAnimationLogType logType )
{
#ifndef RED_PLATFORM_ORBIS
	FILE* f = _wfopen( m_path.AsChar(), TXT( "a" ) );
#else
	FILE* f = fopen( UNICODE_TO_ANSI( m_path.AsChar() ), "a" );
#endif
	if ( f )
	{
		String lType = TXT("SAMPLED");
		
		switch ( logType )
		{
		case EALT_MissingAnim:
			lType = TXT("MISSING");			
			break;

		case EALT_RequestedAnim:
			lType = TXT("REQUESTED");
			break;

		default:			
			break;
		}

		fwprintf( f, TXT( "%s;%s;%s;%d\n" ), lType.AsChar(), str.AsString().AsChar(), data.m_resourcePath.AsChar(), data.m_numSoundEvents );
		fclose( f );
	}
}


void AnimationLogger::LoadFile()
{
	RED_FATAL_ASSERT( SIsMainThread(), "AnimationLogger can only be called from main thread" );

#ifndef RED_PLATFORM_ORBIS
	FILE *f = _wfopen( m_path.AsChar(), TXT("r") );
#else
	FILE *f = fopen( UNICODE_TO_ANSI( m_path.AsChar() ), "r" );
#endif

	if ( f )
	{
		Char buf[ 1024 ];

		while ( fgetws( buf, 1024, f ) )
		{
			String dataStr = String::Printf( TXT("%s"), buf );
			dataStr.Trim();

			TDynArray< String > records = dataStr.Split( TXT(";") );
			const Uint32 numRecords = records.Size();

			if ( numRecords > 0 )
			{
				const String& type = numRecords > 0 ? records[ 0 ] : String::EMPTY;
				const CName animation = numRecords > 1 ? CName( records[ 1 ] ) : CName::NONE;

				AnimationData data;
				data.m_resourcePath = numRecords > 2 ? records[ 2 ] : String::EMPTY;
				if ( numRecords > 3 )
				{
					FromString( records[ 3 ], data.m_numSoundEvents );
				}

				if ( Red::System::StringCompare( type.AsChar(), TXT("REQUESTED") ) == 0 ) 
				{
					AddToMap( animation, data, EALT_RequestedAnim );
				}
				else if ( Red::System::StringCompare( type.AsChar(), TXT("MISSING") ) == 0 ) 
				{
					AddToMap( animation, data, EALT_MissingAnim );
				}
				else 
				{
					AddToMap( animation, data, EALT_SampledAnim );
				}
			}
		}

		fclose( f );
	}
}

#endif
