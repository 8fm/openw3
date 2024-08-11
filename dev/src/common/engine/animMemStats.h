
#pragma once

struct AnimMemStats
{
	Uint32	m_animBufferNonStreamable;
	Uint32	m_animBufferStreamableLoaded;
	Uint32	m_animBufferStreamableWhole;
	Uint32	m_animSource; // size of source data
	Uint32	m_animCachedBuffer;
	Uint32	m_motionExtraction;
	Uint32	m_compressedPose;
	Uint32	m_compressedPoseData;
};
