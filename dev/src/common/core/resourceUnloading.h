/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef __RESOURCE_UNLOADING_H__
#define __RESOURCE_UNLOADING_H__

class CDiskFile;
class CResource;
class CObjectReachability;
class CFastObjectList;

/// Resource unloading queue
class CResourceUnloader
{
public:
	CResourceUnloader();
	~CResourceUnloader();

	// Enable/Disable automatic resource purge
	void EnableAutomaticPurge( const Bool isEnabled );

	// Internal update
	void Update();

	// Force resources to be purged NOW, slow (as slow as the old GC)
	void ForcePurge();

private:
	Uint32					m_currentFrame;		// current frame counter
	Uint32					m_nextAutoPurge;	// next purge frame

	CObjectReachability*	m_reachability;		// reachability analyzer
	CFastObjectList*		m_unreachables;		// unreachable objects

	typedef TDynArray< CDiskFile* > TPurgatoryObjects;
	TPurgatoryObjects	m_purgatory;

	void Purge( const Bool full );
	void ValidatePurge( TDynArray< CDiskFile* >& purgeList ) const;
	void ReportLeaks() const;
};

typedef TSingleton< CResourceUnloader > SResourceUnloader;

#endif

