/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class ISpawnTreeInitializer;
class CSpawnTreeInstance;

class ISpawnTreeInitializersIterator : public Red::System::NonCopyable
{
public:
	virtual Bool		Next( const ISpawnTreeInitializer*& outInitializer, CSpawnTreeInstance*& instanceBuffer ) = 0;
	virtual void		Reset() = 0;
};