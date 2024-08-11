
#pragma once

class CEdSceneEditor;

//////////////////////////////////////////////////////////////////////////

struct CEdSceneHelperShapeSettings
{
	Color		m_color;
	Color		m_wireFrameColor;
	Box			m_box;
	Float		m_boneScale;
	Float		m_selectionColorDiv;
	Bool		m_overlayMode;
	Bool		m_nameOnlyWhenSelected;
	Bool		m_isBox;
	Bool		m_isBone;
	Bool		m_isInteractive;

	CEdSceneHelperShapeSettings()
		: m_color( 200, 200, 0, 255 )
		, m_wireFrameColor( 255, 255, 0, 255 )
		, m_overlayMode( true )
		, m_nameOnlyWhenSelected( false )
		, m_isBox( true )
		, m_isBone( false )
		, m_isInteractive( false )
		, m_selectionColorDiv( 3.f )
		, m_boneScale( 1.f )

	{
		m_box = Box( Vector::ZEROS, 0.03f );
	}
};

struct SSceneHelperReferenceFrameSettings
{
	enum EReferenceFrame
	{
		RF_WorldFrame,
		RF_SceneFrame,
		RF_ActorFrame,
		RF_BoneFrame,
		RF_SlotFrame
	};

	EReferenceFrame m_frameType;
	union 
	{
		struct  //Local frame
		{
			THandle< CAnimatedComponent >	m_parentComp;
			CName							m_boneName;
			Uint32							m_attachmentFlags;
		};
		struct  //Actor frame
		{
			THandle< CActor >				m_parentActor;
		};			
		struct  
		{
			THandle< CEntity >				m_slotOwner;
			CName							m_slotName;
		};
	};

	SSceneHelperReferenceFrameSettings( EReferenceFrame frame = RF_SceneFrame )
		: m_frameType( frame)
	{}
};

//////////////////////////////////////////////////////////////////////////

class IEdSceneHelperEntityCallbackObject
{
public:
	virtual void GetTransform( Vector& outPos, EulerAngles& outRot ) {}

	virtual void OnSetPositionFromPreview( const Vector& prevPos, Vector& newPos ) {}
	virtual void OnSetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot ) {}
	virtual void OnSetScaleFromPreview( const Vector& prevScale, Vector& newScale ) {}

	virtual Bool HasId() const { return false; }
	virtual CGUID GetId() const { return CGUID::ZERO; }
};

//////////////////////////////////////////////////////////////////////////

class CEdSceneHelperComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CEdSceneHelperComponent, CComponent, 0 );

private:
	Bool						m_visible;
	CEdSceneHelperShapeSettings m_settings;
	EngineTransform				m_refTransform;

public:
	CEdSceneHelperComponent();

	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;

	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag ) override;

	virtual Bool IsManualCreationAllowed() const { return false; }

	void SetColor( const Color& color )			{ m_settings.m_color = color; }
	
	void SetRawRefPlacement( const Vector* pos, const EulerAngles* rot, const Vector* s );

public:
	void InitShape( const CEdSceneHelperShapeSettings* s );

	void SetOverlayMode( Bool flag )			{ m_settings.m_overlayMode = flag; }
	void SetVisible( Bool flag );
	void ToggleInteractive();
};

BEGIN_CLASS_RTTI( CEdSceneHelperComponent )
	PARENT_CLASS( CComponent )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CEdSceneHelperEntity : public CEntity
{
	DECLARE_ENGINE_CLASS( CEdSceneHelperEntity, CEntity, 0 );

	IEdSceneHelperEntityCallbackObject*		m_callback;
	CEdSceneEditor*							m_editorCallback;
	Vector									m_offsetWS;		
	SSceneHelperReferenceFrameSettings		m_frameSettings;
	Bool									m_visible;
public:
	CEdSceneHelperEntity();
	~CEdSceneHelperEntity();

	void Init( IEdSceneHelperEntityCallbackObject* callback, CEdSceneEditor* editorCallback );
	void SetFrameSettings( const SSceneHelperReferenceFrameSettings & set);
	Matrix CalculateL2WForLocalFrame() const;
	Matrix CalculateL2WForSlotFrame() const;
	Matrix CalculateToLocalSpaceMat() const;
	Bool GetId( CGUID& id ) const;

	void SelectHelper();
	void DeselectHelper();
	void Move( const Vector& pos, const EulerAngles& rot );

	void RepositionHelper();

	void SetRawRefPlacement( const Vector* pos, const EulerAngles* rot, const Vector* s );
	void SetColor( const Color& color );

	virtual void SetPosition( const Vector& position ) override;
	virtual void SetRotation( const EulerAngles& rotation ) override;
	virtual void SetScale( const Vector& scale ) override;
	virtual void OnTick( Float timeDelta ) override;
};

BEGIN_CLASS_RTTI( CEdSceneHelperEntity )
	PARENT_CLASS( CEntity )
END_CLASS_RTTI()



class CEdSceneHelperEntityForLightEvent : public CEdSceneHelperEntity
{
	DECLARE_ENGINE_CLASS( CEdSceneHelperEntityForLightEvent, CEdSceneHelperEntity, 0 );

	CName m_lightId;

	virtual void OnGenerateEditorFragments( CRenderFrame* /*frame*/, EShowFlags ) override;
	virtual void OnDetached( CWorld* world ) override;
	virtual void OnAttached( CWorld* world ) override;

	CName GetLightId() const { return m_lightId; }
	void SetLightId( CName lightId ) { m_lightId = lightId; }
};

BEGIN_CLASS_RTTI( CEdSceneHelperEntityForLightEvent )
	PARENT_CLASS( CEdSceneHelperEntity )
END_CLASS_RTTI()


class CEdSceneHelperEntityForDurationLookat : public CEdSceneHelperEntity
{
	DECLARE_ENGINE_CLASS( CEdSceneHelperEntityForDurationLookat, CEdSceneHelperEntity, 0 );


};

BEGIN_CLASS_RTTI( CEdSceneHelperEntityForDurationLookat )
	PARENT_CLASS( CEdSceneHelperEntity )
END_CLASS_RTTI() 


//////////////////////////////////////////////////////////////////////////

class CEdSceneHelperEntitiesCtrl
{
	CEdSceneEditor*									m_mediator;
	TDynArray< THandle< CEdSceneHelperEntity > >	m_helpers;
	Bool											m_isHandlingSelection;
	Bool											m_enableFloatingHelpers;

public:
	CEdSceneHelperEntitiesCtrl() 
		: m_isHandlingSelection( false )
		, m_enableFloatingHelpers( true )
	{}

	~CEdSceneHelperEntitiesCtrl();

	void Init( CEdSceneEditor* mediator );
	void Destroy();

	void Refresh();

	void OnWidgetPostChange( const CNode* node ) ;

public:
	Bool Exist( const CGUID& id ) const;
	Bool Select( const CGUID& id );
	void DeselectAllHelpers();

	Bool Contains( const CGUID& id ) const;
	Bool Contains( const CComponent* c ) const;

	CEdSceneHelperEntity* CreateHelper( const CComponent* c, const CEdSceneHelperShapeSettings* s = nullptr );
	CEdSceneHelperEntity* CreateHelper( const EngineTransform& transform, const CEdSceneHelperShapeSettings* s = nullptr );
	CEdSceneHelperEntity* CreateHelper( const CGUID& id, Vector& vec, const CEdSceneHelperShapeSettings* s = nullptr );
	CEdSceneHelperEntity* CreateHelper( const CGUID& id, EngineTransform& transform, const CEdSceneHelperShapeSettings* s = nullptr );
	CEdSceneHelperEntity* CreateHelper( const CGUID& id, EngineTransform& transform, Bool& dirtyFlag, const CEdSceneHelperShapeSettings* s = nullptr );
	CEdSceneHelperEntity* CreateHelperForLightEvent( const CGUID& id, EngineTransform& transform, const CEdSceneHelperShapeSettings* s = nullptr );
	CEdSceneHelperEntity* CreateHelperForDurationLookatEvent( const CGUID& id, Vector& pos1, Vector& pos2, const CEdSceneHelperShapeSettings* s = nullptr );

	void EnableFloatingHelpers( Bool val ){ m_enableFloatingHelpers = val; }
	Bool FloatingHelpersEnabled() const { return m_enableFloatingHelpers; }

	void DestroyHelpers( const CEntity* entity );
	void DestroyHelpers( const CGUID& id );

	void UpdateRawPlacement( const CGUID& id, const EngineTransform& t );
	void UpdateRawPlacement( const CGUID& id, const EngineTransform& t, const EngineTransform& refT );

	CEdSceneHelperEntity* FindHelperById( const CGUID& id );

	void HandleSelection( CWorld* world );
	Bool IsSomethingFromScene( const CComponent* c ) const;
	Bool IsHelperFromScene( const CComponent* c ) const;
	Bool IsSomethingFromCurveEntity( const CComponent* c ) const;
	CNode* SelectObjectFromScene( const CComponent* c );
	CNode* SelectObjectFromWorld( const CComponent* c );

	void CollectSelectedHelpers( TDynArray< CEdSceneHelperEntity* >& entities );

private:
	CEdSceneHelperEntity* InternalCreateHelper( const Vector& pos, const EulerAngles& rot, const CEdSceneHelperShapeSettings* s = nullptr, CClass* customClass = nullptr );
	void DestroyAllHelpers();
	
};

//////////////////////////////////////////////////////////////////////////

class CEdSceneHelperEntityCallbackObject_VectorEvent : public IEdSceneHelperEntityCallbackObject
{
	CGUID				m_id;
	Vector&				m_vec;

public:
	CEdSceneHelperEntityCallbackObject_VectorEvent( const CGUID& id, Vector& vec );

	virtual void GetTransform( Vector& outPos, EulerAngles& outRot ) override;

	virtual void OnSetPositionFromPreview( const Vector& prevPos, Vector& newPos ) override;
	virtual void OnSetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot ) override {}
	virtual void OnSetScaleFromPreview( const Vector& prevScale, Vector& newScale ) override {}
	
	virtual Bool HasId() const override { return true;}
	virtual CGUID GetId() const override { return m_id;}
};

class CEdSceneHelperEntityCallbackObject_TransformEvent : public IEdSceneHelperEntityCallbackObject
{
protected:
	CGUID				m_id;
	EngineTransform&	m_transform;

public:
	CEdSceneHelperEntityCallbackObject_TransformEvent( const CGUID& id, EngineTransform& transform );

	virtual void GetTransform( Vector& outPos, EulerAngles& outRot ) override;

	virtual void OnSetPositionFromPreview( const Vector& prevPos, Vector& newPos ) override;
	virtual void OnSetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot ) override;
	virtual void OnSetScaleFromPreview( const Vector& prevScale, Vector& newScale ) override;

	virtual Bool HasId() const override { return true;}
	virtual CGUID GetId() const override { return m_id;}
};


class CEdSceneHelperEntityCallbackObject_LightPropEvent : public CEdSceneHelperEntityCallbackObject_TransformEvent
{
	CName m_lightId;
public:
	CEdSceneHelperEntityCallbackObject_LightPropEvent( const CGUID& id, EngineTransform& transform )
		: CEdSceneHelperEntityCallbackObject_TransformEvent( id, transform )
	{}

	void SetLightId( CName lightId ) { m_lightId = lightId; }
};

class CEdSceneHelperEntityCallbackObject_TransformWithDirtyFlagEvent : public CEdSceneHelperEntityCallbackObject_TransformEvent
{
	Bool& m_dirtyFlag;

public:
	CEdSceneHelperEntityCallbackObject_TransformWithDirtyFlagEvent( const CGUID& id, EngineTransform& transform, Bool& dirtyFlag );

	virtual void OnSetPositionFromPreview( const Vector& prevPos, Vector& newPos ) override;
	virtual void OnSetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot ) override;
	virtual void OnSetScaleFromPreview( const Vector& prevScale, Vector& newScale ) override;
};

//////////////////////////////////////////////////////////////////////////

class CEdSceneHelperEntityCallbackObject_ActorPlacementEvent : public IEdSceneHelperEntityCallbackObject
{
	//...

public:
	CEdSceneHelperEntityCallbackObject_ActorPlacementEvent() {}

	virtual void OnSetPositionFromPreview( const Vector& prevPos, Vector& newPos ) {}
	virtual void OnSetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot ) {}
	virtual void OnSetScaleFromPreview( const Vector& prevScale, const Vector& newScale ) {}
};

//////////////////////////////////////////////////////////////////////////

class CEdSceneHelperEntityCallbackObject_ActorLookAtTarget : public IEdSceneHelperEntityCallbackObject
{
	//...

public:
	CEdSceneHelperEntityCallbackObject_ActorLookAtTarget() {}

	virtual void OnSetPositionFromPreview( const Vector& prevPos, Vector& newPos ) {}
	virtual void OnSetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot ) {}
	virtual void OnSetScaleFromPreview( const Vector& prevScale, const Vector& newScale ) {}
};
