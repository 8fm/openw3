/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/engineTransform.h"
#include "../core/hashset.h"
#include "../core/task.h"
#include "../core/list.h"

#include "showFlags.h"

//#define DEBUG_UPDATE_TRANSFORM

// Forward class declarations
class IAttachment;
class CEntity;
class CHardAttachment;
class ISkeletonDataProvider;
class ISkeletonDataConsumer;
class ISlotProvider;
class IAnimatedObjectInterface;
class AttachmentSpawnInfo;
class CNode;
class CRenderFrame;
class CComponent;
struct STaskBatchSyncToken;
class CTaskBatch;
struct SUpdateTransformContext;

// Flags for node
BEGIN_FLAGS_INHERIT( ENodeFlags, EObjectFlags )
	NF_Destroyed					= FLAGS( ENodeFlags, 0 ),		// Node was destroyed
	NF_Selected						= FLAGS( ENodeFlags, 1 ),		// Node is selected in the editor
	NF_Initialized					= FLAGS( ENodeFlags, 2 ),		// Node is initialized
	NF_UseParentTransform			= FLAGS( ENodeFlags, 3 ),		// Node uses parent's update transform (m_localToWorld)
	NF_MarkUpdateTransform			= FLAGS( ENodeFlags, 4 ),		// Node was marked for update transform
	NF_Attached						= FLAGS( ENodeFlags, 5 ),		// Node is attached to world
	NF_Attaching					= FLAGS( ENodeFlags, 6 ),		// Node is being attached to world
	NF_Detaching					= FLAGS( ENodeFlags, 7 ),		// Node is being detached from world
	NF_ScheduledUpdateTransform		= FLAGS( ENodeFlags, 8 ),		// Node was scheduled for update transform
	NF_IncludedFromTemplate			= FLAGS( ENodeFlags, 9 ),		// Node was included from other template (f.e. appearance)
	NF_PostAttachSpawnCalled		= FLAGS( ENodeFlags, 10 ),		// Internal post attach event was called
	NF_HideInGame					= FLAGS( ENodeFlags, 11 ),		// Should this node be hidden when in game ?
	NF_WasAttachedInGame			= FLAGS( ENodeFlags, 12 ),		// Node was attached while in game
	NF_WasInstancedFromTemplate		= FLAGS( ENodeFlags, 13 ),		// This node was instanced from template  
	NF_SuspendRendering				= FLAGS( ENodeFlags, 14 ),		// Don't render contents of this node
	NF_ShouldSave					= FLAGS( ENodeFlags, 15 ),		// Should save this node
END_FLAGS( ENodeFlags, 16 )

//////////////////////////////////////////////////////////////////////////
// NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE
//
// INodeAttachListener should not be used in general at this point. It
// exists to support one particular case, but is not intended for the
// general public.
// DO NOT USE IT!
//    -- TG
//////////////////////////////////////////////////////////////////////////
/// Allows another class to receive notifications when a node is attached to or detached from another node.
class INodeAttachListener
{
	friend class CNode;

#ifndef RED_FINAL_BUILD
private:
	// In a non-final build, the listener can track what nodes it has registered with, so that we can later check that
	// everything is unregistered correctly. Failure to unregister before being deleted could result in crashes later on
	// when a node makes some change. Would likely be difficult to track down, so we have this.
	TDynArray< THandle< CNode > > m_listenedNodes;
#endif

public:
	virtual ~INodeAttachListener();

	virtual void OnNotifyNodeChildAttached( CNode* parent, CNode* child, IAttachment* attachment ) = 0;
	virtual void OnNotifyNodeChildDetached( CNode* parent, CNode* child ) = 0;
	virtual void OnNotifyNodeParentAttached( CNode* parent, CNode* child, IAttachment* attachment ) = 0;
	virtual void OnNotifyNodeParentDetached( CNode* parent, CNode* child ) = 0;
};

/////////////////////////////////////////////////////////////////////////////

/****************************************************/
/* Base class for entity/component hierarchy		*/
/****************************************************/
class CNode : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CNode, CObject )

	friend class CLayer;
	friend class CEntity;
	friend class CSelectionManager;

protected:
	TList< IAttachment* >			m_parentAttachments;	//!< Attachments that control us -				4 byte
	TList< IAttachment* >			m_childAttachments;		//!< Attachments that we control -				4 byte
	Matrix							m_localToWorld;			//!< The cached local to world matrix - 4*4*4 =	64 byte
	EngineTransform					m_transform;			//!< Node transform -							4 byte
	CHardAttachment*				m_transformParent;		//!< Parent transform attachment -				4 byte
	TagList							m_tags;					//!< Node tags -								4 byte

private:
	CGUID							m_guid;					//!< GUID -								4*4 =	16 byte

#ifdef DEBUG_UPDATE_TRANSFORM
public:
	Uint64							m_debugUpdateID;
	Uint64							m_debugUpdateForceID;
#endif

public:
	CNode();

	// Returns true if node is destroyed
	RED_INLINE Bool IsDestroyed() const { return 0 != ( m_objectFlags & NF_Destroyed ); }

	// Returns true if node is not destroyed
	RED_INLINE Bool IsNotDestroyed() const { return 0 == ( m_objectFlags & NF_Destroyed ); }

	// Returns true if node is selected in editor
	RED_INLINE Bool IsSelected() const { return 0 != ( m_objectFlags & NF_Selected ); }

	// Is entity initialized ?
	RED_INLINE Bool IsInitialized() const { return 0 != ( m_objectFlags & NF_Initialized ); }

	// Returns true if node is attached to world
	RED_INLINE Bool IsAttached() const { return 0 != ( m_objectFlags & NF_Attached ); }

	// Are we attaching to world right now ?
	RED_INLINE Bool IsAttaching() const { return 0 != ( m_objectFlags & NF_Attaching ); }

	// Are we detaching from world right now ?
	RED_INLINE Bool IsDetaching() const { return 0 != ( m_objectFlags & NF_Detaching ); }

	// Should this node be saved in a game save ?
	RED_INLINE Bool ShouldSave() const { return 0 != ( m_objectFlags & NF_ShouldSave ); }

	// Is node included from a template
	RED_INLINE Bool IsIncludedFromTemplate() const { return 0 != ( m_objectFlags & NF_IncludedFromTemplate ); }

	// Node uses parent's update transform (m_localToWorld)
	RED_INLINE Bool UseParentUpdateTransform() const { return 0 != ( m_objectFlags & NF_UseParentTransform ); }

	// Node uses fast update transform method
	//RED_INLINE Bool UseFastUpdateTransform() const { return 0 != ( m_objectFlags & NF_UseFastUpdateTransform ); }

	// Get node name
	virtual const String & GetName() const { return String::EMPTY; }

	// Checks if node has specific tag
	Bool HasTag( const CName tag ) const;

	// Get node tag list
	RED_INLINE const TagList& GetTags() const { return m_tags; }

	// Get node transform
	RED_INLINE const EngineTransform& GetTransform() const { return m_transform; }

	// Get local position
	RED_INLINE const Vector& GetPosition() const { return m_transform.GetPosition(); }

	// Get local rotation
	RED_INLINE const EulerAngles& GetRotation() const { return m_transform.GetRotation(); }

	// Get component scale
	RED_INLINE const Vector& GetScale() const { return m_transform.GetScale(); }

	// Get matrix that transforms from local space to world space
	RED_INLINE const Matrix& GetLocalToWorld() const { return m_localToWorld; }

	// Get matrix that transforms from local space to world space
	RED_INLINE void GetLocalToWorld( Matrix& mat ) const { mat = m_localToWorld; }

	// Get matrix that transforms from world space to local space
	RED_INLINE void GetWorldToLocal( Matrix& mat ) const { mat = m_localToWorld.FullInverted(); }

	// Get world space position 
	RED_INLINE Vector GetWorldPosition() const { return m_localToWorld.GetTranslation(); }

	// Get world space position 
	RED_INLINE const Vector& GetWorldPositionRef() const { return m_localToWorld.GetTranslationRef(); }

	// Get world rotation
	RED_INLINE EulerAngles GetWorldRotation() const { return m_localToWorld.ToEulerAnglesFull(); }

	// Get world Yaw
	RED_INLINE Float GetWorldYaw() const { return m_localToWorld.GetYaw(); }

	// Get world Forward vector
	RED_INLINE Vector GetWorldForward() const { return m_localToWorld.V[1].Normalized3(); }
	
	// Get world Right vector
	RED_INLINE Vector GetWorldRight() const { return m_localToWorld.V[0].Normalized3(); }
	
	// Get world Up vector
	RED_INLINE Vector GetWorldUp() const { return m_localToWorld.V[2].Normalized3(); }

	// Get parent attachments
	RED_INLINE const TList< IAttachment* >& GetParentAttachments() const { return m_parentAttachments; }

	// Get child attachments
	RED_INLINE const TList< IAttachment* >& GetChildAttachments() const { return m_childAttachments; }

	// Check whether a node is a child of this node, attached with the given type of attachment
	Bool HasChild( const CNode* node, const CClass* attachmentClass ) const;

	// Get hard attachment that acts as transform parent
	RED_INLINE CHardAttachment* GetTransformParent() const { return m_transformParent; }

	// Get object GUID
	RED_INLINE const CGUID& GetGUID() const { return m_guid; }

	// Set object GUID
	RED_INLINE void SetGUID( const CGUID& guid ) { const CGUID oldGuid = m_guid; m_guid = guid; OnGUIDChanged( oldGuid ); }

	// Get update transform job for this node
	//CTask* GetNewUpdateTransformJob( CTaskBatch& taskSync );

	// Gets node bounds (used for example in GameplayStorage)
	virtual void GetStorageBounds( Box& box ) const { box.Min = box.Max = GetWorldPositionRef(); }

#ifndef NO_EDITOR
	// Set the local to world matrix from transform directly (used by the editor after copypaste and clone operations)
	RED_INLINE void SetLocalToWorldFromTransformDirectly() { CalcLocalTransformMatrix( m_localToWorld ); }
#endif

public:
	// Get layer this node was created in
	virtual CLayer* GetLayer() const = 0;

	//! Get object friendly name
	virtual String GetFriendlyName() const;

	//! Format unique node name
	virtual String GetUniqueName() const;

	//! Destroy this node and remove it from layer
	virtual void Destroy();

	//! Serialization
	virtual void OnSerialize( IFile& file );

	// Called after object is loaded
	virtual void OnPostLoad();

	// Called after object is instanced (from template)
	virtual void OnPostInstanced();

	// Entity was attached to world
	virtual void OnAttached( CWorld* world );

	// Entity was detached from world
	virtual void OnDetached( CWorld* world );

	// Game started
	virtual void OnGameStarted() {}

	//! Missing property
	virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue );

	//! Node selection changed
	virtual void OnSelectionChanged();

	// Generate editor rendering fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

#ifndef NO_DATA_VALIDATION
	// Check data, can be called either for node on the level or for node in the template
	virtual void OnCheckDataErrors( Bool isInTemplate ) const;
#endif // NO_DATA_VALIDATION

public:
	// Property was changed
	virtual void OnPropertyPostChange( IProperty* property );

	// New parent attachment was added
	virtual void OnParentAttachmentAdded( IAttachment* attachment );

	// New child attachment was added
	virtual void OnChildAttachmentAdded( IAttachment* attachment );

	// Parent attachment was broken
	virtual void OnParentAttachmentBroken( IAttachment* attachment );

	// Child attachment was broken
	virtual void OnChildAttachmentBroken( IAttachment* attachment );

	// Removes NULL parent attachments (deserialization process will nullify attachments to nonexistence nodes, so sometimes we need to remove this attachments)
	virtual void RemoveNullParentAttachments();

	// Removes NULL child attachments
	virtual void RemoveNullChildAttachments();

	// Create attachment to given component
	virtual IAttachment* Attach( CNode* child, const AttachmentSpawnInfo& info );

	// Create attachment to given component
	virtual IAttachment* Attach( CNode* child, CClass* attachmentClass );

	// Re-attach using an existing attachment. The attachment should not already be attached to anything.
	virtual IAttachment* Attach( CNode* child, IAttachment* attachment );

	// Change selection state of this component
	void Select( Bool state );

	// Set node tag list
	void SetTags( const TagList& tagList );
	void SetTags( TagList&& tagList );

	// Gets bounds for gameplay purposes
	virtual void GetGameplayBounds( Box& box ) const { box.Min = box.Max = GetWorldPositionRef(); }

	// Returns true if node or any parent of the node is marked as hidden in game
	Bool IsHiddenInGame() const;
	Bool IsRenderingSuspended() const;

	// Get the entity that owns this node, not recursive, direct entity access
//	virtual CEntity* GetEntity() const=0;

	// Set local component position
	virtual void SetPosition( const Vector& position );

	// Set local component rotation
	virtual void SetRotation( const EulerAngles& rotation );

	// Set local component scale
	virtual void SetScale( const Vector& scale );

public:
	// Should we update transform this node automatically when parent is update transformed /
	virtual bool UsesAutoUpdateTransform();

	// Has this node transform update scheduled
	RED_INLINE Bool HasScheduledUpdateTransform() const { return HasFlag( NF_ScheduledUpdateTransform ); }

	void ScheduleUpdateTransformNode( CLayer* layer = nullptr );
	void RefreshMarkUpdateTransformChain( CLayer* layer );
	void UpdateTransformNode( SUpdateTransformContext& context, Bool parentScheduledUpdateTransform = false );
	virtual void OnUpdateTransformNode( SUpdateTransformContext& context, const Matrix& prevLocalToWorld );
	void ForceUpdateTransformNode( SUpdateTransformContext& context );
	void ForceUpdateTransformNodeAndCommitChanges();
	void ForceUpdateBoundsNode();

	// Calculate local transform matrix, returns true if matrix is identity
	Bool CalcLocalTransformMatrix( Matrix& matrix ) const;

	void HACK_UpdateLocalToWorld();
	virtual Bool HACK_ForceUpdateTransformWithGlobalPose( const Matrix& matrixWorldSpace ); // See void CPhysicsSimpleWrapper::PostSimulationUpdateTransform

	void UpdateTransformChildrenNodes( SUpdateTransformContext& context, Bool parentScheduledUpdateTransform );

protected:
	// Update local to world only
	void UpdateLocalToWorld();
	CLayer* CheckLayer( CLayer* l );
	void MarkUpdateTransformChain( CLayer* layer );

public:
	// Editor only stuff
#ifndef NO_EDITOR
	
	// Called when node is going to move in editor
	virtual void EditorOnTransformChangeStart();

	// Called when node is moved in editor
	virtual void EditorOnTransformChanged();

	// Called when node is going to stop moving in editor
	virtual void EditorOnTransformChangeStop();

	// Called pre deletion from editor
	virtual void EditorPreDeletion();

	// Called after an object is duplicated (SHIFT + DRAG)
	virtual void EditorPostDuplication( CNode* originalNode );

	// Called by the mass action dialog to check if a node-specific condition is valid
	virtual Bool CheckMassActionCondition( const Char* condition ) const { return false; }

	// Called by the mass action dialog to perform a node-specific action, expected to return true on success
	virtual Bool PerformMassAction( const Char* action ) { return false; }
#endif

public:
	//! Get skeleton data provider interface
	virtual const ISkeletonDataProvider* QuerySkeletonDataProvider() const { return NULL; }

	//! Get skeleton data consumer interface
	virtual const ISkeletonDataConsumer* QuerySkeletonDataConsumer() const { return NULL; }

	//! Get slot provider interface
	virtual ISlotProvider* QuerySlotProvider() { return NULL; }

	// Query animated object interface
	virtual IAnimatedObjectInterface* QueryAnimatedObjectInterface() { return NULL; }

	//! Get collision listener to be used by phantom components
	virtual class IPhysicalCollisionTriggerCallback* QueryPhysicalCollisionTriggerCallback() { return NULL; }

public:
	// Cast to component ( faster than Cast<> )
	virtual CComponent* AsComponent() { return NULL; }

	// Cast to component - const version ( faster than Cast<> )
	virtual const CComponent* AsComponent() const { return NULL; }

	// Cast to entity ( faster than Cast<> )
	virtual CEntity* AsEntity() { return NULL; }

	// Cast to entity - const version ( faster than Cast<> )
	virtual const CEntity* AsEntity() const { return NULL; }

	virtual Bool ShouldGenerateEditorFragments( CRenderFrame* frame ) const;

	// Refresh visibility - do it locally and for all attached components & entities
	typedef THashSet< CNode* > NodeProcessingContext;
	virtual void RefreshNodeVisibilityFlag( NodeProcessingContext& context, Bool force ) = 0;

	// Called by streaming system to create heavyweight engine representation; returns false until succeeded
	virtual Bool OnCreateEngineRepresentation() { return true; }
	// Called by streaming system to destroy heavyweight engine representation
	virtual void OnDestroyEngineRepresentation() {}

protected:
	void CheckUpdateTransformMode();
private:
	virtual void OnGUIDChanged( const CGUID& oldGuid ) { RED_UNUSED( oldGuid ); }

	//////////////////////////////////////////////////////////////////////////
	// NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE
	//
	// As noted above for INodeAttachListener, this is not intended for the
	// general public.
	// DO NOT USE IT!
	//    -- TG
	//////////////////////////////////////////////////////////////////////////
public:
	Bool RegisterNodeAttachListener( INodeAttachListener* listener );
	Bool UnregisterNodeAttachListener( INodeAttachListener* listener );
private:
	TDynArray< INodeAttachListener* >	m_attachListeners;

	//////////////////////////////////////////////////////////////////////////

private:
	void funcGetName( CScriptStackFrame& frame, void* result );
	void funcGetLocalPosition( CScriptStackFrame& frame, void* result );
	void funcGetLocalRotation( CScriptStackFrame& frame, void* result );
	void funcGetLocalScale( CScriptStackFrame& frame, void* result );
	void funcGetLocalToWorld( CScriptStackFrame& frame, void* result );
	void funcGetWorldPosition( CScriptStackFrame& frame, void* result );
	void funcGetWorldRotation( CScriptStackFrame& frame, void* result );
	void funcGetWorldForward( CScriptStackFrame& frame, void* result );
	void funcGetWorldRight( CScriptStackFrame& frame, void* result );
	void funcGetWorldUp( CScriptStackFrame& frame, void* result );
	void funcGetHeading( CScriptStackFrame& frame, void* result );
	void funcGetHeadingVector( CScriptStackFrame& frame, void* result );
	void funcHasTag( CScriptStackFrame& frame, void* result );
	void funcGetTags( CScriptStackFrame& frame, void* result );
	void funcSetTags( CScriptStackFrame& frame, void* result );
	void funcGetTagsString( CScriptStackFrame& frame, void* result );
	void funcSetPosition( CScriptStackFrame& frame, void* result );
};

BEGIN_ABSTRACT_CLASS_RTTI( CNode );
	PARENT_CLASS( CObject );	
	PROPERTY_CUSTOM_EDIT( m_tags, TXT("Node tags"), TXT("TagListEditor") );
	PROPERTY_EDIT( m_transform, TXT("Local node transform") );
	PROPERTY( m_transformParent );
	PROPERTY( m_guid );
	NATIVE_FUNCTION( "GetName", funcGetName );
	NATIVE_FUNCTION( "GetLocalPosition", funcGetLocalPosition );
	NATIVE_FUNCTION( "GetLocalRotation", funcGetLocalRotation );
	NATIVE_FUNCTION( "GetLocalScale", funcGetLocalScale );
	NATIVE_FUNCTION( "GetLocalToWorld", funcGetLocalToWorld );
	NATIVE_FUNCTION( "GetWorldPosition", funcGetWorldPosition );
	NATIVE_FUNCTION( "GetWorldRotation", funcGetWorldRotation );
	NATIVE_FUNCTION( "GetWorldForward", funcGetWorldForward );
	NATIVE_FUNCTION( "GetWorldRight", funcGetWorldRight );
	NATIVE_FUNCTION( "GetWorldUp", funcGetWorldUp );
	NATIVE_FUNCTION( "GetHeading", funcGetHeading );
	NATIVE_FUNCTION( "GetHeadingVector", funcGetHeadingVector );
	NATIVE_FUNCTION( "HasTag", funcHasTag );
	NATIVE_FUNCTION( "GetTags", funcGetTags );
	NATIVE_FUNCTION( "SetTags", funcSetTags );
	NATIVE_FUNCTION( "GetTagsString", funcGetTagsString );
END_CLASS_RTTI();
