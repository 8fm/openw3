#pragma once

#include "storySceneGraphBlock.h"
#include "storySceneFlowCondition.h"

class CStorySceneFlowConditionBlock : public CStorySceneGraphBlock, public I2dArrayPropertyOwner
{
	DECLARE_ENGINE_CLASS( CStorySceneFlowConditionBlock, CStorySceneGraphBlock, 0 )

private:
	CStorySceneFlowCondition* m_condition;
	String m_description;

public:
	//! Get flow condition represented by this block
	RED_INLINE CStorySceneFlowCondition* GetCondition() const { return m_condition; }
	void SetCondition( CStorySceneFlowCondition* condition);
	
	//! Get scene control part ( section, control flow ) that is represented by this block
	virtual CStorySceneControlPart* GetControlPart() const { return m_condition; }
	virtual void SetControlPart( CStorySceneControlPart* part ) { m_condition = Cast< CStorySceneFlowCondition >( part ); }

public:
	CStorySceneFlowConditionBlock() : m_condition(NULL) {}

	//! Serialization
	virtual void OnSerialize( IFile& file );

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Loaded
	virtual void OnPostLoad();

	//! Recreate layout of the block
	virtual void OnRebuildSockets();

	//! Return block shape to draw
	virtual EGraphBlockShape GetBlockShape() const;

	//! Determine block color that is used to fill the client area
	virtual Color GetClientColor() const;

	//! Get block caption
	virtual String GetCaption() const;

	virtual void OnDestroyed();

#endif

public:
	virtual void Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties );

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );
#endif
};

BEGIN_CLASS_RTTI( CStorySceneFlowConditionBlock );
	PARENT_CLASS( CStorySceneGraphBlock );
	PROPERTY_EDIT( m_description, TXT( "Description of a flow condition" ) );
	PROPERTY_INLINED_RO( m_condition, TXT( "Property of a flow condition structure" ) );
END_CLASS_RTTI();