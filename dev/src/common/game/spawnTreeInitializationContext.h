/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CSpawnTreeInitializationContext
{
public:
	typedef SCompiledInitializer TopInitializer;
	typedef TStaticArray< TopInitializer, 32 > TopInitializersList;

	struct SPopData
	{
		TopInitializersList							m_erasedInitializers;
		TopInitializersList							m_addedInitializers;
	};
protected:
	TopInitializersList								m_topInitializers;
public:
	CSpawnTreeInitializationContext()													{}
	~CSpawnTreeInitializationContext()													{}

	void											PushInitialInitializer( ISpawnTreeInitializer* initializer, CSpawnTreeInstance* instance  ) { m_topInitializers.PushBack( TopInitializer( initializer, instance ) ); }

	void											PushTopInitializers( const TDynArray< ISpawnTreeInitializer* >& initializers, CSpawnTreeInstance* instance, SPopData& undo );
	void											PopTopInitializers( const SPopData& undo );

	const TopInitializersList&						GetTopInitializers()				{ return m_topInitializers; }
};