#pragma once

#include "../../common/engine/localizableObject.h"
#include "storySceneControlPart.h"
#include "../engine/entityTemplate.h"
#include "storySceneSectionVariant.h"

class CStorySceneGraph;
class CStorySceneSection;
class CStorySceneGraphBlock;
class CStorySceneSectionBlock;
class CStorySceneCutsceneSectionBlock;
class CStorySceneInputBlock;
class CStorySceneOutputBlock;
class CStorySceneScriptingBlock;
class CStorySceneFlowConditionBlock;
class CStorySceneFlowSwitchBlock;
class CAbstractStorySceneLine;
class CStorySceneControlPart;
class CStorySceneInput;
class IStorySceneItem;
class CStorySceneActor;
class CStorySceneProp;
class CStorySceneEffect;
class CStorySceneLight;

//////////////////////////////////////////////////////////////////////////

// What is that??? Editor???
// Editor events
RED_DECLARE_NAME( SceneSectionRemoved );
RED_DECLARE_NAME( SceneSectionAdded );
RED_DECLARE_NAME( ScenePartBlockSocketsChanged );
RED_DECLARE_NAME( SceneSectionNameChanged );
RED_DECLARE_NAME( SceneSectionElementAdded );
RED_DECLARE_NAME( SceneSectionLinksChanged );
RED_DECLARE_NAME( SceneChoiceLineChanged );
RED_DECLARE_NAME( SceneChoiceLineLinkChanged );
RED_DECLARE_NAME( SceneSettingChanged );

//////////////////////////////////////////////////////////////////////////

// TODO - find better place for this
struct StorySceneDefinition
{
	DECLARE_RTTI_STRUCT( StorySceneDefinition );

	TSoftHandle< CStoryScene >	m_scene;
	CName						m_input;
};

BEGIN_CLASS_RTTI( StorySceneDefinition )
	PROPERTY_EDIT( m_scene, TXT( "Scene File" ) );
	PROPERTY_EDIT( m_input, TXT( "Scene Input" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CStoryScene;

#ifndef NO_EDITOR

class IStorySceneModifier
{
public:
	//...
};

#endif

class CStoryScene : public CResource, public ILocalizableObject
{
	DECLARE_ENGINE_RESOURCE_CLASS( CStoryScene, CResource, "w2scene", "Scene script" );

private:
	TDynArray< CStorySceneSection* >			m_sections;				//!< Sections in this scene
	TDynArray< CStorySceneControlPart* >		m_controlParts;			//!< All control parts in this scene

	#ifndef NO_EDITOR_GRAPH_SUPPORT
	CStorySceneGraph*							m_graph;				//!< Scene graph
	String										m_layerPreset;
	#endif

	#ifndef NO_EDITOR
	IStorySceneModifier*						m_modifier;
	#endif

	//TDynArray< InstanceDataLayout >				m_dataLayouts;

	Uint32										m_elementIDCounter;		//!< Counter for generating unique element IDs
	Uint32										m_sceneId;
	Uint32										m_sectionIDCounter;
	Bool										m_mayActorsStartWorking;
	
	Bool										m_surpassWaterRendering;	//!< Surpass global water rendering

	TDynArray< CStorySceneActor* >				m_sceneTemplates;		// TODO: merge all these structs, 1 ID to lookup
	TDynArray< CStorySceneProp* >				m_sceneProps;
	TDynArray< CStorySceneEffect* >				m_sceneEffects;
	TDynArray< CStorySceneLight* >				m_sceneLights;

	// New dialogset approach
	TDynArray< CStorySceneDialogsetInstance* >	m_dialogsetInstances;
	TDynArray< StorySceneCameraDefinition >		m_cameraDefinitions;

	TDynArray< CName > m_banksDependency;

	TDynArray< CName > m_soundEventsOnEnd;	//Sound events to trigger when the scene ends 
	TDynArray< CName > m_soundEventsOnSkip;	//Sound events to trigger when the scene is skipped 

	Bool			   m_blockMusicTriggers;
	String			   m_soundListenerOverride;
	Bool			   m_muteSpeechUnderWater;
	
public:
	struct CTemplateWithAppearance
	{
		THandle<CEntityTemplate>	m_template;
		CName						m_appearance;
	};

public:
	//! Get story scene graph
	#ifndef NO_EDITOR_GRAPH_SUPPORT
	RED_INLINE CStorySceneGraph* GetGraph() { return m_graph; }
	RED_INLINE const CStorySceneGraph* GetGraph() const { return m_graph; }
	#endif

	RED_INLINE Uint32 GetSceneIdNumber() const { return m_sceneId; }
	RED_INLINE String GetSceneName() const { return GetFriendlyName(); }

	RED_INLINE const TDynArray< CStorySceneActor* >& GetSceneActorsDefinitions() const { return m_sceneTemplates; }
	RED_INLINE const TDynArray< CStorySceneProp* >& GetScenePropDefinitions() const { return m_sceneProps; }
	RED_INLINE const TDynArray< CStorySceneEffect* >& GetSceneEffectDefinitions() const { return m_sceneEffects; }
	RED_INLINE const TDynArray< CStorySceneLight* >& GetSceneLightDefinitions() const { return m_sceneLights; }

	RED_INLINE const TDynArray< CName >& GetBanksDependency() const { return m_banksDependency; }

	RED_INLINE const TDynArray< CName>& GetSoundEventsOnEnd() const { return m_soundEventsOnEnd; }
	RED_INLINE const TDynArray< CName>& GetSoundEventsOnSkip() const { return m_soundEventsOnSkip; }

	//Sound settings

	//Returns true if this scene wants to block music triggers
	RED_INLINE const Bool ShouldBlockMusicTriggers() const { return m_blockMusicTriggers; }
	RED_INLINE const String GetListenerOverride() const { return m_soundListenerOverride; }
	RED_INLINE const Bool ShouldMuteSpeechUnderWater() const { return m_muteSpeechUnderWater; }


	CStorySceneActor* GetSceneActorDefinition( const CName& id );
	CStorySceneEffect* GetSceneEffectDefinition( const CName& id );
	CStorySceneLight* GetSceneLightDefinition( const CName& id );
	IStorySceneItem* GetSceneItem( const CName& id );			// TODO: later this general function should replace all the specific ones above

	const CStorySceneActor* GetSceneActorDefinition( const CName& id ) const;
	const CStorySceneEffect* GetSceneEffectDefinition( const CName& id ) const;
	const CStorySceneLight* GetSceneLightDefinition( const CName& id ) const;
	const IStorySceneItem* GetSceneItem( const CName& id ) const;			// TODO: later this general function should replace all the specific ones above

	RED_INLINE Bool MayActorsStartWorking() const { return m_mayActorsStartWorking; }
	RED_INLINE Bool IsWaterRenderingSurpassed() const { return m_surpassWaterRendering; }

	void GetAppearancesForVoicetag( CName voicetag, TDynArray< CTemplateWithAppearance > & templates ) const;
	Bool GetActorSpawnDefinition( const CName& voicetag, THandle<CEntityTemplate>& actorTemplate, CName& appearanceName, Bool& isSearchedByVoicetag ) const;
	const CStorySceneProp* GetPropDefinition( CName id ) const;
	Bool GetTaglistForVoicetag( const CName& voicetag, TagList& tags ) const;
	const CStorySceneActor* GetActorDescriptionForVoicetag( const CName& voicetag ) const;
	void CollectVoicetags( TDynArray< CName >& voicetags, Bool includeNonSpeaking = false ) const;

	const CStorySceneDialogsetInstance* GetDialogsetByName( const CName& dialogsetName ) const;
	RED_INLINE const TDynArray< CStorySceneDialogsetInstance* > & GetDialogsetInstances() const { return m_dialogsetInstances; }
	void GetDialogsetInstancesNames( TDynArray< CName >& names ) const;

	Bool CheckSectionsIdsCollision( const CStoryScene* s ) const;

#ifndef NO_EDITOR
	void RegenerateId();
	Bool IsDialogsetUsed( CName dialogset ) const;
	CName GetFirstDialogsetNameAtSection_Old( const CStorySceneSection* section ) const;
	const CStorySceneDialogsetInstance*	GetFirstDialogsetAtSection_Old( const CStorySceneSection* section ) const;

	CStorySceneDialogsetInstance* GetFirstDialogsetAtSection( const CStorySceneSection* section );
	const CStorySceneDialogsetInstance* GetFirstDialogsetAtSection( const CStorySceneSection* section ) const;
	CStorySceneDialogsetInstance* GetDialogsetByName( const CName& dialogsetName );
	void AddDialogsetInstance( CStorySceneDialogsetInstance* dialogsetInstance );
	void RemoveDialogsetInstance( const CName& dialogsetName );
	void AddActorDefinition( CStorySceneActor* actorDef );
	void AddLightDefinition( CStorySceneLight* lightDef );
#ifndef NO_EDITOR_GRAPH_SUPPORT
	String GetLayerPreset()								{ return m_layerPreset; }
	void SetLayerPreset( const String& presets )		{ m_layerPreset = presets; }
#endif
#endif
	

public:
	CStoryScene();
	
	//! Resource was pasted from clipboard
	virtual void OnPaste( Bool wasCopied );

	//! Object was loaded from file
	virtual void OnPostLoad();

#ifndef NO_RESOURCE_COOKING
	virtual void OnCook( class ICookerFramework& cooker ) override;
#endif

	//! Before save file
	virtual void OnPreDependencyMap( IFile& mapper );

	virtual void OnPreSave();

	//! OnSavingFile
	virtual void OnSave();

	virtual void OnSerialize( IFile& file );

	virtual Bool OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue ) override;

	void CleanupSceneGraph();



public:
	//! Get the number of sections in this scene
	Uint32 GetNumberOfSections() const;

	//! Get the n-th section from the scene
	CStorySceneSection* GetSection( Uint32 index );
	const CStorySceneSection* GetSection( Uint32 index ) const;

	//! Get the index of the given section
	Int32 GetSectionIndex( CStorySceneSection* section ) const;

	void AddSectionAtPosition( CStorySceneSection* section, Int32 index );

	//! Find section with given name
	const CStorySceneSection* FindSection( const String& sectionName ) const;
	const CStorySceneInput*	FindInput( const String& inputName ) const;

	void DeleteCustomCamera( const String& name );						// Deprecated

	// New camera approach
	const StorySceneCameraDefinition* GetCameraDefinition( const CName& cameraName ) const;
	StorySceneCameraDefinition* GetCameraDefinition( const CName& cameraName );

	RED_INLINE const TDynArray< StorySceneCameraDefinition >& GetCameraDefinitions() const { return m_cameraDefinitions; }
#ifndef NO_EDITOR
	RED_INLINE TDynArray< StorySceneCameraDefinition >& GetCameraDefinitions() { return m_cameraDefinitions; }
#endif

	void AddCameraDefinition( const StorySceneCameraDefinition& cameraDefinition );
	Bool DeleteCameraDefinition( const CName& cameraName );
	void DeleteAllCameraDefinitions();


#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Remove section from the scene
	void RemoveSection( Uint32 index );
	void RemoveSection( CStorySceneSection* section );
	void RemoveControlPart( CStorySceneControlPart* controlPart );     
	void AddControlPart( CStorySceneControlPart* controlPart );   

	CStorySceneGraphBlock* CreateAndAddSceneBlock( CClass* blockClass, CClass* controlPartClass );
	RED_INLINE TDynArray< CStorySceneControlPart* >& GetControlParts() { return m_controlParts; }
#endif

	//! Generate unique scene element ID
	String GenerateUniqueElementID();

	Uint32 GenerateUniqueSectionID();

#ifndef NO_RESOURCE_IMPORT
	void GenerateSceneId();
	void RefreshAllVoiceFileNames();
	void OnPropertyPostChange( IProperty* property );
#endif

	// Please do not use it - TControlPartIterator is better option.
	// Get all lines from all sections in the story
	void GetLines( TDynArray< CAbstractStorySceneLine* >& lines ) const;

	template< class T > // Please do not use it - TControlPartIterator is better option.
	void CollectControlParts( TDynArray< T* >& controlParts ) const
	{	
		for ( TDynArray< CStorySceneControlPart* >::const_iterator controlPartIter = m_controlParts.Begin();
			controlPartIter != m_controlParts.End(); ++controlPartIter )
		{
			T* controlPart = Cast< T >( *controlPartIter );
			if ( controlPart != NULL )
			{
				controlParts.PushBackUnique( controlPart );
			}
		}
	}

	template< class T > // Please do not use it - TControlPartIterator is better option.
	void CollectControlPartsNames( TDynArray< String >& controlParts ) const
	{	
		for ( TDynArray< CStorySceneControlPart* >::const_iterator controlPartIter = m_controlParts.Begin();
			controlPartIter != m_controlParts.End(); ++controlPartIter )
		{
			T* controlPart = Cast< T >( *controlPartIter );
			if ( controlPart )
			{
				controlParts.PushBack( controlPart->GetName() );
			}
		}
	}

	template< class T > // Please do not use it - TControlPartIterator is better option.
	void CollectControlParts( TDynArray< CObject* >& controlParts ) const
	{
		for ( TDynArray< CStorySceneControlPart* >::const_iterator controlPartIter = m_controlParts.Begin();
			controlPartIter != m_controlParts.End(); ++controlPartIter )
		{
			controlParts.PushBackUnique( *controlPartIter );
		}
	}

	class ControlPartIterator
	{
	protected:
		TDynArray< CStorySceneControlPart* >&	m_list;
		Int32									m_index;
		const CClass*							m_class;

	public:
		ControlPartIterator( CStoryScene* scene, const CClass* c );

		operator Bool () const						{ return IsValid(); }
		void operator++ ()							{ Next(); }
		CStorySceneControlPart* operator*() const	{ return m_list[ m_index ]; }

	protected:
		Bool IsValid() const;
		void Next();

	private:
		const ControlPartIterator& operator=( const ControlPartIterator& );
	};

	template< class T >
	class TControlPartIterator : private ControlPartIterator
	{
	public:
		TControlPartIterator( CStoryScene* scene ) : ControlPartIterator( scene, ClassID< T >() ) {}
		operator Bool() const	{ return ControlPartIterator::operator Bool(); }
		void operator++ ()		{ ControlPartIterator::Next(); }
		T* operator*()			{ return static_cast< T* >( ControlPartIterator::operator *() ); }
	};

	template< class T >
	class TConstControlPartIterator : private ControlPartIterator
	{
	public:
		TConstControlPartIterator( const CStoryScene* scene ) : ControlPartIterator( const_cast< CStoryScene* >( scene ), ClassID< T >() ) {}
		operator Bool() const	{ return ControlPartIterator::operator Bool(); }
		void operator++ ()		{ ControlPartIterator::Next(); }
		const T* operator*()	{ return static_cast< T* >( ControlPartIterator::operator *() ); }
	};

#ifndef NO_EDITOR
	TDynArray< String > GetInputNames() const;
	TDynArray< String > GetOutputNames() const;
#endif

	void CollectAllRequiredPositionTags( TDynArray< CName >& positionTags );

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! Create graph for new story scene
	void InitializeSceneGraphs();
#endif

public:
	virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) override/*const*/;

private:
	//! Compile data layout
	void CompileDataLayout();

	void OnStorySceneModified();

public:
	InstanceBuffer* CreateInstanceBuffer( CObject* parent, const CStorySceneSection* section, CStorySceneSectionVariantId variantId, InstanceDataLayout& temp ) const;
	void InitInstanceBuffer( InstanceBuffer* data, const CStorySceneSection* section, CStorySceneSectionVariantId variantId ) const;
	void ReleaseInstanceBuffer( InstanceBuffer* data, const CStorySceneSection* section, CStorySceneSectionVariantId variantId ) const;
	void DestroyInstanceBuffer( InstanceBuffer* data ) const;

public:
#ifndef NO_EDITOR
	void ConnectSceneModifier( IStorySceneModifier* m );
#endif

	Bool OnLinkElementMarkModifiedPre( const CStorySceneLinkElement* element );
	void OnLinkElementMarkModifiedPost( const CStorySceneLinkElement* element );

public: // DIALOG_TOMSIN_TODO - zadne skrypty nie sa potrzebne tu!
	void funcGetCustomBehaviorForVoicetag( CScriptStackFrame& stack, void* result );
	void funcGetCustomAnimsetForVoicetag( CScriptStackFrame& stack, void* result );
	void funcGetRequiredPositionTags( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CStoryScene )
	PARENT_CLASS( CResource )
	PROPERTY( m_controlParts );
	PROPERTY( m_sections );
	#ifndef NO_EDITOR_GRAPH_SUPPORT
	PROPERTY_NOT_COOKED( m_graph );
	PROPERTY_NOT_COOKED( m_layerPreset );
	#endif
	PROPERTY( m_elementIDCounter );
	PROPERTY( m_sectionIDCounter );
	PROPERTY_EDIT( m_sceneId, TXT( "Scene ID" ) )
	PROPERTY_INLINED( m_sceneTemplates, TXT( "Scene Actors" ) );
	PROPERTY_INLINED( m_sceneProps, TXT( "Scene Props" ) );
	PROPERTY_INLINED( m_sceneEffects, TXT( "Scene Effects" ) );
	PROPERTY_INLINED( m_sceneLights, TXT( "Scene Lights" ) );
	PROPERTY_EDIT( m_mayActorsStartWorking, TXT( "May actors do theirs job during scene?" ) );
	PROPERTY_EDIT(  m_surpassWaterRendering, TXT( "Surpass global water rendering, default = false" ) );
	PROPERTY_INLINED( m_dialogsetInstances, TXT( "Dialogset instances" ) );
	PROPERTY_INLINED( m_cameraDefinitions, TXT( "Camera definitions" ) );
	PROPERTY_CUSTOM_EDIT_ARRAY( m_banksDependency, TXT( "" ), TXT( "SoundBankEditor" ) );
	PROPERTY_EDIT( m_blockMusicTriggers, TXT( "Should music triggers be blocked during this scene"));
	PROPERTY_EDIT( m_muteSpeechUnderWater, TXT( "Should mute all VO when inderwater during this scene"));
	PROPERTY_EDIT( m_soundListenerOverride, TXT("Specify the listener override if we want one for the scene"));
	PROPERTY_CUSTOM_EDIT_ARRAY( m_soundEventsOnEnd, TXT("Events to be triggered when this scene ends"), TXT( "AudioEventBrowser"));
	PROPERTY_CUSTOM_EDIT_ARRAY( m_soundEventsOnSkip, TXT("Events to be triggered when this scene is skipped"), TXT( "AudioEventBrowser"));
	NATIVE_FUNCTION( "GetCustomBehavior", funcGetCustomBehaviorForVoicetag );
	NATIVE_FUNCTION( "GetCustomAnimset", funcGetCustomAnimsetForVoicetag );
	NATIVE_FUNCTION( "GetRequiredPositionTags", funcGetRequiredPositionTags );
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class StorySceneSectionIterator
{
	
};

class StorySceneSectionConstIterator
{

};

template< class T >
class StorySceneSectionElementIterator
{

};

template< class T >
class StorySceneSectionElementConstIterator
{

};

template< class T >
class StorySceneElementIterator
{

};

template< class T >
class StorySceneElementConstIterator
{

};

//////////////////////////////////////////////////////////////////////////
