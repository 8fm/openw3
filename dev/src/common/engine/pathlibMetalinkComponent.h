/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/refCountPointer.h"

#include "pathlibComponent.h"


namespace PathLib
{

typedef Uint8 MetalinkClassId;

////////////////////////////////////////////////////////////////////////////
// IMetalinkSetup
class IMetalinkSetup
{
protected:
	~IMetalinkSetup()														{}	// never destroy directly, always through factory

public:
	enum EMetalinkPathfollowFlags
	{
		METALINK_PATHFOLLOW_DEFUALT											= 0,

		METALINK_PATHFOLLOW_ALLOW_PATHOPTIMIZATION							= FLAG( 0 ),
		METALINK_PATHFOLLOW_GHOST_LINK										= FLAG( 1 ),
	};

	struct RuntimeData
	{
		SComponentMapping			m_mapping;
		IComponent::SafePtr			m_component;

		IMetalinkComponent*			GetComponent( CAgent* pathAgent );
		CComponent*					GetEngineComponent( CAgent* pathAgent );
	};

	typedef TRefCountPointer< IMetalinkSetup > Ptr;

	virtual MetalinkClassId		GetClassId() const = 0;

	virtual Bool				AgentPathfollowUpdate( RuntimeData& r, CAgent* pathAgent, const Vector3& interactionPoint, const Vector3& destinationPoint );
	virtual Bool				AgentPathfollowOver( RuntimeData& r, CAgent* pathAgent, const Vector3& interactionPoint, const Vector3& destinationPoint );
	virtual Bool				AgentPathfollowIgnore( CAgent* pathAgent );
	virtual Uint32				GetMetalinkPathfollowFlags() const;

	// smart pointer interface
	// most of that classes will be just global objects that are not spawned&despawned in runtime
	virtual void				AddRef();									
	virtual void				Release();

	virtual Bool				ReadFromBuffer( CSimpleBufferReader& reader );
	virtual void				WriteToBuffer( CSimpleBufferWriter& writer ) const;
};

class CGenericMetalinkSetup : public IMetalinkSetup
{
public:
	MetalinkClassId				GetClassId() const override;

	static IMetalinkSetup::Ptr	Create();
};

class CMetalinkSetupFactory
{
public:
	typedef MetalinkClassId ClassFactoryId;

	class IClassFactory
	{
	protected:
		typedef CMetalinkSetupFactory::ClassFactoryId ClassFactoryId;
	public:
		IClassFactory( ClassFactoryId id, ClassFactoryId knownIdLimit );

		virtual IMetalinkSetup*		Request()								= 0;
		virtual void				Release( IMetalinkSetup* setup )		= 0;
	};

	template < class T >
	class TClassFactory_GlobalObject : public IClassFactory
	{
	protected:
		T							s_instance;

	public:
		TClassFactory_GlobalObject( ClassFactoryId id, ClassFactoryId knownIdLimit )
			: IClassFactory( id, knownIdLimit )								{}

		virtual IMetalinkSetup*	Request() override							{ return &s_instance; }
		virtual void			Release( IMetalinkSetup* setup ) override	{}
	};

protected:
	TDynArray< IClassFactory* >		m_classes;

public:
	CMetalinkSetupFactory();
	~CMetalinkSetupFactory();

	IClassFactory*					GetClassFactory( ClassFactoryId classId );

	void							RegisterClassFactory( IClassFactory* classFactory, ClassFactoryId classId, ClassFactoryId classIdLimit );

	IMetalinkSetup*					NewFromBuffer( CSimpleBufferReader& reader );
	static void						SaveToBuffer( CSimpleBufferWriter& writer, IMetalinkSetup* metalinSetup );

	static CMetalinkSetupFactory&	GetInstance();
};

////////////////////////////////////////////////////////////////////////////
// IMetalinkComponent is interface for engine components to create and use metalinks (used by explorations, doors)
class IMetalinkComponent : public IComponent
{
	typedef IComponent Super;
protected:
	typedef CMetalinkConfiguration GraphConfiguration;
public:

	Bool						RuntimeProcessing( CComponentRuntimeProcessingContext& context, const CProcessingEvent& e ) override;
	Bool						ToolAsyncProcessing( CObstaclesLayerAsyncProcessing* processingJob, const CProcessingEvent& e, IGenerationManagerBase::CAsyncTask::CSynchronousSection& section ) override;

	Bool						IsNoticableInGame( CProcessingEvent::EType eventType ) const override;
	Bool						IsNoticableInEditor( CProcessingEvent::EType eventType ) const override;

	IMetalinkComponent*			AsMetalinkComponent() override;

	virtual Bool				ConfigureGraph( GraphConfiguration& configuration, CPathLibWorld& pathlib )									= 0;
	virtual IMetalinkSetup::Ptr	CreateMetalinkSetup() const																					= 0;
	virtual IAIQueueMetalinkInterface* GetAIQueueInterface();
};



};				// namespace PathLib

