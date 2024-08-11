
/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef RED_FINAL_BUILD
#define USE_ANIMATION_LOGGER
#endif

#ifdef USE_ANIMATION_LOGGER

enum EAnimationLogType
{
	EALT_MissingAnim,
	EALT_RequestedAnim,
	EALT_SampledAnim
};

class AnimationLogger
{	
	struct AnimationData
	{
		String		m_resourcePath;
		Int32		m_numSoundEvents;

		AnimationData() : m_numSoundEvents( -1 ) {}
	};

	typedef THashMap< CName, AnimationData > TLoggerMap;

	Red::Threads::CMutex	m_mutex;

	String		m_path;
	TLoggerMap	m_missingAnimMap;
	TLoggerMap	m_requestedAnimMap;
	TLoggerMap	m_sampledAnimMap;

public:
	AnimationLogger();

	void Log( const CSkeletalAnimation* animation, EAnimationLogType logType );
	void Log( const CName& animation, const CSkeletalAnimationContainer* cont, EAnimationLogType logType );

private:
	void LoadFile();

	Bool AddToMap( const CName& str, const AnimationData& data, EAnimationLogType logType );

	void WriteToFile( const CName& animation, const AnimationData& data, EAnimationLogType logType );
};

typedef TSingleton< AnimationLogger > SAnimationLogger;

#endif
