/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "idBasicBlocks.h"
#include "idLine.h"

//------------------------------------------------------------------------------------------------------------------
// Graph block
//------------------------------------------------------------------------------------------------------------------
class CIDGraphBlockConnector : public CIDGraphBlock
{
	DECLARE_ENGINE_CLASS( CIDGraphBlockConnector, CIDGraphBlock, 0 );

protected:
	CName						m_speaker;
	CName						m_category;	

	TInstanceVar< TGenericPtr >	i_request;

public:
	CIDGraphBlockConnector() {}

	//! Get the name of the block
	virtual CName GetDefaultName() const { return CNAME( Connector ); }

	//! Rebuild block layout
	virtual void OnRebuildSockets();
	virtual void OnInitInstance( InstanceBuffer& data ) const;
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual const CIDGraphBlock* ActivateInput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* input, Bool evaluate = true ) const;
	virtual const CIDGraphBlock* Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const;
	virtual const CIDGraphBlock* ActivateOutput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* output, Bool evaluate = true ) const;

	//! Get client color
	virtual Color GetClientColor() const;
};

BEGIN_CLASS_RTTI( CIDGraphBlockConnector )
	PARENT_CLASS( CIDGraphBlock )
	PROPERTY_CUSTOM_EDIT( m_speaker, TXT("InterlocutorID of an existing actor"), TXT("InterlocutorIDList") )
	PROPERTY_EDIT( m_category, TXT("Connector category" ) )
END_CLASS_RTTI()
