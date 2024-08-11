#pragma once

enum EPhysicsWrappers
{
	EPW_INTERVAL = 128,
	EPW_Simple = 0,
	EPW_Tile = EPW_Simple + EPW_INTERVAL,
	EPW_Chained = EPW_Tile + EPW_INTERVAL,
	EPW_Jointed = EPW_Chained + EPW_INTERVAL,
	EPW_Character = EPW_Jointed + EPW_INTERVAL,
	EPW_Partice = EPW_Character + EPW_INTERVAL,
	EPW_ApexDestruction = EPW_Partice + EPW_INTERVAL,
	EPW_ApexCloth = EPW_ApexDestruction + EPW_INTERVAL,
	EPW_Destruction = EPW_ApexCloth + EPW_INTERVAL,
	EPW_COUNT = EPW_Destruction + EPW_INTERVAL,
	EPW_MAX = EPW_COUNT + EPW_INTERVAL
};

#define DECLARE_PHYSICS_WRAPPER(a,b,c,d)																								\
template<> TWrappersPool< a, SWrapperContext>* CPhysicsWorld::GetWrappersPool< a, SWrapperContext >()									\
{																																		\
	char exist = m_wrapperPools[ EPW_COUNT + b / EPW_INTERVAL ];																			\
	TWrappersPool< a, SWrapperContext >* pool = ( TWrappersPool< a, SWrapperContext >* )&m_wrapperPools[ b ];							\
	if( !exist )																														\
	{																																	\
		pool = ::new( pool ) TWrappersPool< a, SWrapperContext >( 2048, c, d );															\
		m_wrapperPools[ EPW_COUNT + b / EPW_INTERVAL ] = 1;																				\
	}																																	\
	return pool;																														\
}																												
