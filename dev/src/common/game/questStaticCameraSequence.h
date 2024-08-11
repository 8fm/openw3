/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "questCamera.h"

class CQuestStaticCameraSequenceBlock : public CQuestCameraBlock
{
	DECLARE_ENGINE_CLASS( CQuestStaticCameraSequenceBlock, CQuestCameraBlock, 0 )

	typedef TDynArray< THandle< CStaticCamera > > TCameraArray;

private:
	TDynArray< CName >		m_cameras;
	Float					m_maxWaitTimePerCamera;

private:
	TInstanceVar< Float >			i_startTime;
	TInstanceVar< TGenericPtr >		i_listener;
	TInstanceVar< Float >			i_duration;
	TInstanceVar< Int32 >			i_cameraIterator;
	TInstanceVar< TCameraArray >	i_cameras;

public:
	CQuestStaticCameraSequenceBlock() : m_maxWaitTimePerCamera( 10.f ) { m_name = TXT("Static Camera Sequence"); }

protected:

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }
	virtual Color GetClientColor() const { return Color( 200, 128, 200 ); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const;

	virtual void OnExecute( InstanceBuffer& data ) const;
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual void OnDeactivate( InstanceBuffer& data ) const;

public:
	void OnStaticCameraStopped( InstanceBuffer& data, const CStaticCamera* camera ) const;

private:
	void CreateListener( InstanceBuffer& data ) const;
	void DeleteListener( InstanceBuffer& data ) const;
	CQuestStaticCameraBlockListener* GetListener( InstanceBuffer& data ) const;
	Bool CheckCameraListener( InstanceBuffer& data, const CStaticCamera* camera ) const;

	CStaticCamera* FindCamera( const CName& tag ) const;
	CStaticCamera* GetCurrCamera( InstanceBuffer& data ) const;
	Bool CollectCameras( InstanceBuffer& data ) const;

	Bool IsSequenceFinished( InstanceBuffer& data ) const;
	Bool PlayNextCamera( InstanceBuffer& data ) const;
};

BEGIN_CLASS_RTTI( CQuestStaticCameraSequenceBlock )
	PARENT_CLASS( CQuestCameraBlock )
	PROPERTY_EDIT( m_cameras, TXT( "Cameras" ) )
	PROPERTY_EDIT( m_maxWaitTimePerCamera, TXT("") )
END_CLASS_RTTI()
