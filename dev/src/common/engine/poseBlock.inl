/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */
#pragma once

RED_INLINE Bool CPoseBlock::IsPoseAvailable() const
{
	return m_poseAvailable.GetValue() != 0;
}

RED_INLINE Bool CPoseBlock::IsAllPoseAvailable() const
{
	return m_poseAvailable.GetValue() == m_poseCount;
}

RED_INLINE Uint32 CPoseBlock::GetPoseCount() const
{
	return m_poseCount;
}

RED_INLINE Uint32 CPoseBlock::GetPoseAvailableCount() const
{
	return m_poseAvailable.GetValue();
};

RED_INLINE void CPoseBlock::SignalPoseAvailable()
{
	m_poseAvailable.Increment();
}

