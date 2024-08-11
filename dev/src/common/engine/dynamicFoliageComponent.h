/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "component.h"
#include "../core/softHandleProcessor.h"
#include "../core/sharedPtr.h"
#include "foliageDynamicInstanceService.h"

class CSRTBaseTree;

class CDynamicFoliageComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CDynamicFoliageComponent, CComponent, 0 )

public:
	CDynamicFoliageComponent(void);
	~CDynamicFoliageComponent(void);

	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	// Called when component is detached from world
	virtual void OnDetached( CWorld* world );
	virtual void OnDestroyed();

#ifndef NO_EDITOR
	virtual void EditorOnTransformChanged( ) override;
	virtual void EditorOnTransformChangeStop() override;
#endif

	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag ) override;

	//! Returns the minimum streaming distance for this component
	virtual Uint32 GetMinimumStreamingDistance() const override;

private:
	void CreateFoliageInstance();
	void DestroyFoliageInstance();
	void UpdateInstancePosition();

	mutable TSoftHandle< CSRTBaseTree > m_baseTree;
	CFoliageDynamicInstanceService m_foliageController;

	Uint32 m_minimumStreamingDistance;

#ifndef NO_EDITOR
	Vector m_instancePosition;		//!< To keep track of position only in editor
	CSRTBaseTree* m_oldBaseTree;	//!< For swap functionality only in editor
#endif

};

BEGIN_CLASS_RTTI( CDynamicFoliageComponent );
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT( m_baseTree, TXT("Base Tree"))
	PROPERTY_EDIT( m_minimumStreamingDistance, TXT( "Minimum streaming distance" ) );
END_CLASS_RTTI();
