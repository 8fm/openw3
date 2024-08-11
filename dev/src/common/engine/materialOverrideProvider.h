/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once


/// Material override context
class SMaterialOverrideContext
{
public:
	IMaterial*	m_originalMaterial;
	Uint32		m_chunkIndex;

public:
	SMaterialOverrideContext ();
};


/// Material override provider
class IMaterialOverrideProvider
{
public:
	virtual ~IMaterialOverrideProvider () 
	{
		// empty
	}

	/// Overrides given material (returns provided material in case of no override).
	virtual IMaterial* OverrideMaterial( const SMaterialOverrideContext &context )=0;
};


/// Material override provider for mesh components
class CMaterialOverrideProviderMeshComponent : public IMaterialOverrideProvider
{
public:
	/// Tests wheather mesh component qualifies for override
	static Bool Qualifies( const CMeshComponent &meshComponent );

public:
	const CMeshComponent *m_meshComponent;

public:
	/// Ctor
	CMaterialOverrideProviderMeshComponent ( const CMeshComponent *meshComponent );

	/// Overrides given material (returns provided material in case of no override)
	virtual IMaterial* OverrideMaterial( const SMaterialOverrideContext &context );
};