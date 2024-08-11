
#pragma once

struct SBgCounter
{
	static const	Uint32 BUCKET_COUNT = 10;
	static			Uint32 STATIC_COUNTER;
	static			Uint32 CURRENT_BUCKET_ID;

	SBgCounter() : m_id( STATIC_COUNTER++ % BUCKET_COUNT ) { }
	
	RED_INLINE static void UpdateTick( Uint64 tickNumber ) { CURRENT_BUCKET_ID = tickNumber % BUCKET_COUNT; }
	RED_INLINE Bool ShouldUpdateThisTick() { return m_id == CURRENT_BUCKET_ID; }

	Uint32 m_id;
};