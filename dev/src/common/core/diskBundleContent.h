/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "object.h"
#include "resource.h"

/// Loaded content of the disk bundle
/// Wrapper class so we don't have to add every resource to the root set
class CDiskBundleContent : public CObject
{
	DECLARE_ENGINE_CLASS( CDiskBundleContent, CObject, 0 );

public:
	CDiskBundleContent();
	~CDiskBundleContent();

	// get resource list
	RED_INLINE const TDynArray< THandle< CResource > >& GetResources() const { return m_resources; }

	// set resource list
	void SetResources( TDynArray< THandle< CResource > >& resources );

	// extract resources of given type
	void ExtractResources( const CClass* resourceClass, TDynArray< THandle< CResource > >& outResources ) const;

private:
	virtual void OnSerialize( IFile& file ) override;
	TDynArray< THandle< CResource > >		m_resources;
};

BEGIN_CLASS_RTTI( CDiskBundleContent );
	PARENT_CLASS( CObject );
END_CLASS_RTTI()