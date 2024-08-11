/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "idBasicBlocks.h"

//------------------------------------------------------------------------------------------------------------------
// Graph block
//------------------------------------------------------------------------------------------------------------------
class CIDGraphBlockBranch : public CIDGraphBlock
{
	DECLARE_ENGINE_CLASS( CIDGraphBlockBranch, CIDGraphBlock, 0 );

protected:
	TDynArray< IDConditionList* >	m_outputs;

	enum
	{
		OUTPUT_Max = 10
	};

public:
	CIDGraphBlockBranch() {}

	//! Get the name of the block
	virtual CName GetDefaultName() const { return CNAME( Branch ); } 

	//! Rebuild block layout
	virtual void OnRebuildSockets();

	//! Called in the editor when property had been changed
	virtual void OnPropertyPostChange( IProperty* prop );

	//! Get title bar color
	virtual Color GetTitleColor() const;

	//! Branch block is always active (all non-regular blocks are)
	virtual void SetActive( CIDTopicInstance* topicInstance, Bool activate ) const {} 
	virtual Bool IsActivated( const CIDTopicInstance* topicInstance ) const { return true; } 

	//! Graph evaluation
	virtual const CIDGraphBlock* ActivateInput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* input, Bool evaluate = true ) const;
	virtual const CIDGraphBlock* Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const;
	virtual const CIDGraphBlock* ActivateOutput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* output, Bool evaluate = true ) const;
	virtual Bool IsRegular() const { return false; }

	//! Update() method is fidderent than Evaluate() - it is called separately in the thread
	const CIDGraphBlock* Update( CIDTopicInstance* topicInstance ) const;

	//! Get block shape
	virtual EGraphBlockShape GetBlockShape() const;

	//! InstanceBuffer stuff
	virtual void OnInitInstance( InstanceBuffer& data ) const;
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

protected:
	void ValidateOutputs();
	String GetOutputNameFor( Uint32 outputIndex ) const;
};

BEGIN_CLASS_RTTI( CIDGraphBlockBranch )
	PARENT_CLASS( CIDGraphBlock )
	PROPERTY_INLINED( m_outputs, TXT("") )
END_CLASS_RTTI()
