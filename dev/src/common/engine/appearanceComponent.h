#pragma once

#include "attachment.h"
#include "component.h"

class EntitySpawnInfo;
class CEntityTemplate;
struct CEntityAppearance;

struct SAppearanceAttachment
{
	DECLARE_RTTI_STRUCT( SAppearanceAttachment )

public:
	SAppearanceAttachment();
	~SAppearanceAttachment();

	IAttachment*	m_attachment;
	CName			m_parentClass, m_parentName;
	CName			m_childClass, m_childName;

	static Bool CustomSerializer( IFile& file, void* data );
};

BEGIN_CLASS_RTTI( SAppearanceAttachment )
	CUSTOM_SERIALIZER( SAppearanceAttachment::CustomSerializer );
	PROPERTY( m_parentClass );
	PROPERTY( m_parentName );
	PROPERTY( m_childClass );
	PROPERTY( m_childName );
END_CLASS_RTTI();

struct SAppearanceAttachments
{
	DECLARE_RTTI_STRUCT( SAppearanceAttachments );

	CName								m_appearance;
	TDynArray< SAppearanceAttachment >	m_attachments;

	void RemoveAppearanceAttachment( IAttachment* attachmentToRemove );

	static const CName& GetTypeName() { return RED_NAME( SAppearanceAttachments ); };
};

BEGIN_CLASS_RTTI( SAppearanceAttachments )
	PROPERTY( m_appearance );
	PROPERTY( m_attachments );
END_CLASS_RTTI();

class CAppearanceComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CAppearanceComponent, CComponent, 0 )

	CName		m_currentAppearance;
	CName		m_forcedAppearance;
	CName		m_initialAppearance;
	Bool		m_initialAppearanceApplied;
	Bool		m_initialAppearanceLoaded;
	Bool		m_usesRobe;

	// Contains information about an appearance template that is currently
	// applied and the components that were created from it
	struct SDynamicTemplate
	{
		CEntityTemplate*					m_template;
		TDynArray< THandle<CComponent> >	m_components;
	};

	TDynArray<SDynamicTemplate>							m_dynamicTemplates;				//!< Currently active templates and their components
	TDynArray<SAppearanceAttachments>					m_appearanceAttachments;
	SAttachmentReplacements								m_attachmentReplacements;
	TDynArray< THandle< IAttachment > >					m_createdTempAttachments;

public:
	CAppearanceComponent();

	// Get current appearance name
	RED_INLINE RED_MOCKABLE const CName& GetAppearance() const { return m_currentAppearance; }

	// Get forced appearance name
	RED_INLINE const CName& GetForcedAppearance() const { return m_forcedAppearance; }

	// Get whether the current appearance uses a robe
	RED_INLINE const Bool GetUsesRobe() const { return m_usesRobe; }

#ifndef NO_EDITOR
	// Set forced appearance name (editor only)
	void SetForcedAppearance( CName forcedAppearance );
#endif

	//! Set the attachment replacements
	void SetAttachmentReplacements( const SAttachmentReplacements& replacements );

	//! Apply appearance from spawn info
	void ApplyInitialAppearance( const EntitySpawnInfo& spawnInfo );

	//! Apply appearance on entity
	void ApplyAppearance( const CEntityAppearance &appearance );

	//! Apply appearance with given name on entity
	void ApplyAppearance( const CName &appearanceName );

	void ApplyAppearance( const EntitySpawnInfo& spawnInfo );

	//! Remove the current appearance without applying a new one
	void RemoveCurrentAppearance( Bool resetAppearanceName = true );

	// Called when component is initialized before being attached
	virtual void OnInitialized();

	// Property was read from file that is no longer in the object, returns true if data was handled
	virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue ) override;

	virtual void IncludeAppearanceTemplate( CEntityTemplate* entityTemplate, TDynArray< CComponent* >& addedComponents );
	virtual void ExcludeAppearanceTemplate( CEntityTemplate* entityTemplate );
	void PostProcessAddedComponents( TDynArray< CComponent* >& addedComponents );

	//! Flag that tells that this components needs to be initialized before everything else
	virtual Bool ShouldInitializeBeforeOtherComponents() const { return true; }

	//! Fills the passed array with the components of the currently active appearance
	void GetCurrentAppearanceComponents( TDynArray< CComponent* >& appearanceComponents );

	//! Fills the passed array with the templates of the currently active appearance
	void GetCurrentAppearanceTemplates( TDynArray< CEntityTemplate* >& appearanceTemplates );

	//! Create streaming components stored in the templates of the active appearance
	void CreateStreamingComponentsForActiveAppearance( TDynArray< CComponent* >& createdComponents );

	//! Checks if the given tag is defined in any of the currently active appearance template entity objects
	Bool HasTagInAppearanceEntities( const CName& tag ) const;

	//! Use instance properties to save per-world-instance appearance
	virtual void GetInstancePropertyNames( TDynArray< CName >& instancePropertyNames ) const override;

	static CAppearanceComponent* GetAppearanceComponent( const CEntity* entity );

	void UpdateCurrentAppearanceAttachments();
	void RemoveAppearanceAttachment( IAttachment* attachment );
	void RemoveAppearanceAttachments( const CName& appearanceName );

	void OnSerialize( IFile& file ) override;

	virtual bool UsesAutoUpdateTransform() override { return false; }

	virtual void OnSaveGameplayState( IGameSaver* saver );
	virtual void OnLoadGameplayState( IGameLoader* loader );

	RED_INLINE Bool CheckShouldSave() const { return m_initialAppearance != m_currentAppearance; }

	// OnFinalize
	virtual void OnFinalize() override;
	virtual void OnDetachFromEntityTemplate() override;

protected:
	void funcApplyAppearance( CScriptStackFrame& stack, void* result );
	void funcGetAppearance( CScriptStackFrame& stack, void* result );
	void funcIncludeAppearanceTemplate( CScriptStackFrame& stack, void* result );
	void funcExcludeAppearanceTemplate( CScriptStackFrame& stack, void* result );

private:
	void AddAppearanceAttachment( IAttachment* source );
	Int32 GetAppearanceAttachmentsIndex( const CName& appearanceName ) const;
	Bool IsComponentPartOfActiveAppearance( CComponent* component ) const;
	const SAppearanceAttachments* GetAppearanceAttachmentData( const CName& appearanceName ) const;
};

BEGIN_CLASS_RTTI( CAppearanceComponent );
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT( m_forcedAppearance, TXT("Forced initial appearance name (leave empty to select a random one from the ticked Appearances)") );
	PROPERTY( m_attachmentReplacements );
	PROPERTY( m_appearanceAttachments );
	NATIVE_FUNCTION( "ApplyAppearance", funcApplyAppearance );
	NATIVE_FUNCTION( "GetAppearance", funcGetAppearance );
	NATIVE_FUNCTION( "IncludeAppearanceTemplate", funcIncludeAppearanceTemplate );
	NATIVE_FUNCTION( "ExcludeAppearanceTemplate", funcExcludeAppearanceTemplate );
END_CLASS_RTTI()
