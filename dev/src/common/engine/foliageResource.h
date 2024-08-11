/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _ENGINE_FOLIAGE_RESOURCE_H_
#define _ENGINE_FOLIAGE_RESOURCE_H_

#include "../core/resource.h"
#include "foliageInstance.h"

class CWorldPartitionNode;
class CSRTBaseTree;

typedef THandle< CSRTBaseTree >	BaseTreeHandle;

typedef std::function< Bool ( const SFoliageInstance & instance ) > FoliageInstanceFilter;
inline Bool AllIntances( const SFoliageInstance & ) { return true; }

struct SFoliageInstanceGroup
{
	SFoliageInstanceGroup();
	
	SFoliageInstanceGroup( const SFoliageInstanceGroup& rhs );

	SFoliageInstanceGroup( SFoliageInstanceGroup&& rhs );

	SFoliageInstanceGroup& operator = ( const SFoliageInstanceGroup& rhs );

	SFoliageInstanceGroup& operator = ( SFoliageInstanceGroup&& rhs );

	void Serialize( IFile & file );
	void SwapWith( SFoliageInstanceGroup& other );

	typedef FoliageInstanceContainer Instances;
	BaseTreeHandle baseTree;
	Instances instances;
};

IFile & operator<<( IFile & file, SFoliageInstanceGroup & group ); 

class CFoliageResource : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CFoliageResource, CResource, "flyr", "Foliage Resource" );

public:
	
	typedef TDynArray< BaseTreeHandle, MC_FoliageInstances > BaseTreeContainer;
	typedef TDynArray< SFoliageInstanceGroup, MC_FoliageInstances > InstanceGroupContainer;

	CFoliageResource();
	virtual ~CFoliageResource();

#ifndef NO_EDITOR_RESOURCE_SAVE
	virtual void OnResourceSavedInEditor() override;
#endif 

	virtual void OnPreSave() override;
	virtual void OnSerialize( IFile& file ) override;
	virtual void OnPostLoad() override;

	Bool CanInsertInstance( const SFoliageInstance & instance ) const;
	
	Bool InsertInstance( BaseTreeHandle baseTree, const SFoliageInstance & instance );
	void RemoveInstance( BaseTreeHandle baseTree, const SFoliageInstance & instance );

	//! If newBaseTree is nullptr, removes oldBaseTree and all its instances. Returns number of replaced/removed instances.
	Uint32 ReplaceBaseTree( const CSRTBaseTree* oldBaseTree, const CSRTBaseTree* newBaseTree, FoliageInstanceFilter filter = AllIntances, const Float* resetScale = nullptr );

	RED_MOCKABLE const InstanceGroupContainer & GetAllTreeInstances() const;
	RED_MOCKABLE const InstanceGroupContainer & GetAllGrassInstances() const;
	RED_MOCKABLE const BaseTreeContainer & GetAllBaseTree() const;

	void GetInstancesFromArea( const CSRTBaseTree* baseTree, const Box & box, FoliageInstanceContainer& instances );
	void GetInstancesFromArea( const CSRTBaseTree* baseTree, const Vector & center, Float radius, FoliageInstanceContainer& instances );

	void SetGridBox( const Box & box );
	const Box & GetGridBox() const;
	const Box & GetInstancesBoundingBox() const;

private:
	void ExtractInstances( FoliageInstanceContainer& source, FoliageInstanceContainer& dest, FoliageInstanceFilter filter );

	SFoliageInstanceGroup & AcquireFoliageInstanceGroup( BaseTreeHandle baseTree );
 	void PopulateBaseTreeContainer( const InstanceGroupContainer & container );
	void RecomputeBBox();
	void RemoveBaseTree( const CSRTBaseTree * baseTree, InstanceGroupContainer & container );
	void SanitizeInstances();
	void RebuildInstanceGroupContainer();
	void RemoveInstanceOutsideBox();

	Uint32 m_version;
	Box	m_bbox;	
	Box m_gridbbox;

	InstanceGroupContainer m_treeInstanceContainer;
	InstanceGroupContainer m_grassInstanceContainer;
	BaseTreeContainer m_baseTreeContainer;
};

BEGIN_CLASS_RTTI( CFoliageResource );
PARENT_CLASS( CResource );
PROPERTY( m_bbox );
PROPERTY( m_gridbbox);
PROPERTY( m_version ); 
END_CLASS_RTTI();

#endif
