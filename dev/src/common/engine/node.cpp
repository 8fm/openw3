/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "node.h"

#include "renderFrame.h"
#include "viewport.h"
#include "hardAttachment.h"
#include "game.h"
#include "componentIterator.h"
#include "boundedComponent.h"
#include "world.h"
#include "tagManager.h"
#include "layer.h"
#include "layerInfo.h"
#include "appearanceComponent.h"
#include "../core/taskBatch.h"
#include "../core/scriptStackFrame.h"
#include "entity.h"
#include "baseEngine.h"

#ifdef DEBUG_UPDATE_TRANSFORM
#include "camera.h"
#endif

INodeAttachListener::~INodeAttachListener()
{
#ifndef RED_FINAL_BUILD
	if ( !m_listenedNodes.Empty() )
	{
		TDynArray< THandle< CNode > > nodes = m_listenedNodes;
		for ( Uint32 i = 0; i < nodes.Size(); ++i )
		{
			CNode* node = nodes[ i ].Get();
			ASSERT( !node, TXT("INodeAttachListener was not fully unregistered!") );
			if ( node )
			{
				node->UnregisterNodeAttachListener( this );
			}
			else
			{
				m_listenedNodes.RemoveFast( nodes[ i ] );
			}
		}

		ASSERT( m_listenedNodes.Empty() );
		m_listenedNodes.ClearFast();
	}
#endif
}


//////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CNode )

RED_DISABLE_WARNING_MSC( 4355 )

CNode::CNode()
	: m_transformParent( NULL )
#ifdef DEBUG_UPDATE_TRANSFORM
	, m_debugUpdateID( -1 )
	, m_debugUpdateForceID( -1 )
#endif
{
#ifndef RED_FINAL_BUILD
	// Make the default local to world matrix INVALID - this way we will capture all bullshit stuff
	// We use signaling NaN (SNaN) to trigger breakpoint every time we use this number in a calculation
	for ( Uint32 i=0; i<16; ++i )
	{
		((Uint32*) &m_localToWorld)[i] = 0x7FBFFFFF;
	}
#endif
}

/*CTask* CNode::GetNewUpdateTransformJob( CTaskBatch& taskBatch )
{
	return new ( CTask::Root ) CJobImmediateUpdateTransform( taskBatch.GetSyncToken(), this );
}*/

void CNode::OnPropertyPostChange( IProperty* property )
{
	// Propagate to base class
	TBaseClass::OnPropertyPostChange( property );

	// Node name changed
	if ( property->GetName() == RED_NAME( name ) )
	{
		EDITOR_QUEUE_EVENT( CNAME( NodeNameChanged ), CreateEventData( this ) );
	}
	else
	// Position/Rotation/Scale changed
	if ( property->GetName() == RED_NAME( transform ) )
	{
        ScheduleUpdateTransformNode();
#ifndef NO_EDITOR
		EditorOnTransformChanged();
#endif
	}
}

struct OldNodeTransformData
{
	Vector			m_position;
	EulerAngles		m_rotation;
	Vector			m_scale;

	void Reset()
	{
		m_position = Vector::ZERO_3D_POINT;
		m_rotation = EulerAngles::ZEROS;
		m_scale = Vector::ONES;
	}

	EngineTransform TranslateTransform() const
	{
		return EngineTransform( m_position, m_rotation, m_scale );
	}
};

static OldNodeTransformData GOldNodeTransformData;

void CNode::OnSerialize( IFile& file )
{
	CGUID oldGuid = m_guid;

	// Reset
	GOldNodeTransformData.Reset();

	// Pass to base class
	TBaseClass::OnSerialize( file );

	// Assume the GUID has changed
	if( file.IsReader() )
	{
		OnGUIDChanged( oldGuid );
	}

	file << m_parentAttachments;
	file << m_childAttachments;

	if( !file.IsGarbageCollector() )
	{
		// support for old local to world shit
		if ( file.GetVersion() < VER_REMOVED_SAVING_OF_LOCAL_TO_WORLD )
		{
			Matrix oldLocalToWorld;
			oldLocalToWorld.SetIdentity();
			file << oldLocalToWorld;
		}
	}
}

void CNode::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	// Broken attachment clean-up
	{
		TList< IAttachment* >::iterator currAttachment = m_parentAttachments.Begin();
		TList< IAttachment* >::iterator lastAttachment = m_parentAttachments.End();
		while ( currAttachment != lastAttachment )
		{
			IAttachment* attachment = *currAttachment;
			if ( attachment == NULL || attachment->GetParent() == NULL )
			{
				if ( !GIsCooker )
				{
					ERR_ENGINE( TXT("Broken parrent attachment in '%ls'"), GetFriendlyName().AsChar() );
				}

				currAttachment = m_parentAttachments.Erase( currAttachment );
				if( attachment )
				{
					attachment->SetParent( nullptr );
					attachment->m_parent = nullptr;
					attachment->m_child = nullptr;
				}
			}
			else
			{
				++currAttachment;
			}
		}
	}
	{
		TList< IAttachment* >::iterator currAttachment = m_childAttachments.Begin();
		TList< IAttachment* >::iterator lastAttachment = m_childAttachments.End();
		while ( currAttachment != lastAttachment )
		{
			IAttachment* attachment = *currAttachment;
			if ( attachment == NULL || attachment->GetChild() == NULL )
			{
				if ( !GIsCooker )
				{
					ERR_ENGINE( TXT("Broken child attachment in '%ls' ( %s is NULL )"), GetFriendlyName().AsChar(), ( *currAttachment == NULL ) ? TXT("attachment") : TXT("child") );
				}

				currAttachment = m_childAttachments.Erase( currAttachment );
				if( attachment )
				{
					attachment->SetParent( nullptr );
					attachment->m_parent = nullptr;
					attachment->m_child = nullptr;
				}
			}
			else
			{
				++currAttachment;
			}
		}
	}

	CheckUpdateTransformMode();
}

void CNode::OnPostInstanced()
{
	SetFlag( NF_WasInstancedFromTemplate );
}

Bool CNode::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	// Parent attachments
	if ( propertyName == TXT("parentAttachments") )
	{
		TDynArray< IAttachment* > att;
		if ( readValue.AsType( att ) )
		{
			m_parentAttachments.Clear();
			m_parentAttachments.PushBack( att );
			return true;
		}
	}

	// Child attachments
	if ( propertyName == TXT("childAttachments") )
	{
		TDynArray< IAttachment* > att;
		if ( readValue.AsType( att ) )
		{
			m_childAttachments.Clear();
			m_childAttachments.PushBack( att );
			return true;
		}
	}

	// Local to world matrix
	if ( propertyName == TXT("localToWorld") )
	{
		Matrix mat;
		if ( readValue.AsType( mat ) )
		{
			m_localToWorld.Set( mat );
			return true;
		}
	}

	// World to local matrix
	if ( propertyName == TXT("worldToLocal") )
	{
		// This matrix is not stored explicitly any more
		return true;
	}

	// Component ID is not used any more
	if ( propertyName == TXT("componentID") )
	{
		// This is not used anymore
		return true;
	}

	// Position
	if ( propertyName == TXT("position") )
	{
		readValue.AsType( GOldNodeTransformData.m_position );
		return true;
	}

	// Rotation
	if ( propertyName == TXT("rotation") )
	{
		readValue.AsType( GOldNodeTransformData.m_rotation );
		return true;
	}

	// Scale
	if ( propertyName == TXT("scale") )
	{
		readValue.AsType( GOldNodeTransformData.m_scale );
		return true;
	}

	// Pass to base class
	return TBaseClass::OnPropertyMissing( propertyName, readValue );
}

void CNode::OnSelectionChanged()
{
}

void CNode::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	if( flag == SHOW_NodeTags )
	{
		String tagText = String::EMPTY;

		const TagList& tags = GetTags();
		const Uint32 tagCount = tags.Size();

		if( tagCount == 0 )
		{
			return;
		}

		for( Uint32 i=0; i<tagCount; ++i )
		{
			tagText += tags.GetTag( i ).AsString();
			if( i<tagCount-1)
			{
				tagText += TXT("\n");
			}
		}

		frame->AddDebugText( GetLocalToWorld().GetTranslation(), tagText, true, Color( 255, 255, 255, 0 ), Color( 0, 0, 0, 0 ), nullptr );
	}
}

#ifndef NO_DATA_VALIDATION
void CNode::OnCheckDataErrors( Bool isInTemplate ) const
{
}
#endif // NO_DATA_VALIDATION

String CNode::GetFriendlyName() const
{
	if ( GetName().Empty() && m_guid != CGUID::ZERO )
	{
		Char guidStr[ RED_GUID_STRING_BUFFER_SIZE ];
		m_guid.ToString( guidStr, RED_GUID_STRING_BUFFER_SIZE );

		if ( GetParent() )
		{
			return GetParent()->GetFriendlyName() + TXT("::") + guidStr;
		}

		return guidStr;
	}
	else
	{
		if ( GetParent() )
		{
			return GetParent()->GetFriendlyName() + TXT("::") + GetName();
		}

		return GetName();
	}
}

void CNode::Destroy()
{
	/* intentinoally empty */
}

Bool CNode::CalcLocalTransformMatrix( Matrix& matrix ) const
{
	// Local transform is identity, fast path
	if ( m_transform.IsIdentity() )
	{
		matrix = Matrix::IDENTITY;
		return true;
	}

	// Calculate local transform matrix
	m_transform.CalcLocalToWorld( matrix );
	return false;
}

void CNode::SetPosition( const Vector& position )
{
	m_transform.SetPosition( position );
	CheckUpdateTransformMode();
	ScheduleUpdateTransformNode();
}

void CNode::SetRotation( const EulerAngles& rotation )
{
	m_transform.SetRotation( rotation );
	CheckUpdateTransformMode();
	ScheduleUpdateTransformNode();
}

void CNode::SetScale( const Vector& scale )
{
	m_transform.SetScale( scale );
	CheckUpdateTransformMode();
	ScheduleUpdateTransformNode();
}

#ifndef NO_EDITOR
void CNode::EditorOnTransformChangeStart()
{
	// left empty
}

void CNode::EditorOnTransformChanged()
{
	// left empty
}

void CNode::EditorOnTransformChangeStop()
{
	// left empty
}

void CNode::EditorPreDeletion()
{
	// left empty
}
void CNode::EditorPostDuplication( CNode* originalNode )
{

}
#endif

Bool CNode::IsHiddenInGame() const
{
	// Node is hidden in game
 	if ( m_objectFlags & NF_HideInGame )
	{
		return true;
	}

	// Component case
	const CComponent* component = AsComponent();
	if ( component )
	{
		const CEntity* entity = component->GetEntity();
		if ( entity )
		{
			return entity->IsHiddenInGame();
		}
	}

	// Check transform parent
	if ( m_transformParent && m_transformParent->GetParent() )
	{
		return m_transformParent->GetParent()->IsHiddenInGame();
	}

	// Not hidden
	return false;
}

Bool CNode::IsRenderingSuspended() const
{
	// Node is hidden in game
	if ( m_objectFlags & NF_SuspendRendering )
	{
		return true;
	}

	// Check transform parent
	if ( m_transformParent && m_transformParent->GetParent() )
	{
		return m_transformParent->GetParent()->IsRenderingSuspended();
	}

	// Component case
	const CComponent* component = AsComponent();
	if ( component )
	{
		const CEntity* entity = component->GetEntity();
		if ( entity )
		{
			return entity->IsRenderingSuspended();
		}
	}

	// Not hidden
	return false;
}

void CNode::SetTags( const TagList& tagList )
{
	CWorld* world = ( GetLayer() != nullptr ) ? GetLayer()->GetWorld() : nullptr;

	// do not change tags to the same values
	// NOTE: this DOES NOT WORK properly for some reason - it still requires investigating why
	//	if ( !TagList::MatchAll( tagList, m_tags ) )
	{
		// Unregister node
		if( world != nullptr && world->GetTagManager() != nullptr )
		{
			world->GetTagManager()->RemoveNode( this, m_tags );
	#ifndef NO_EDITOR
			world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_NodeTags );
	#endif	// NO_EDITOR
		}

		// Change tags
		m_tags = tagList;

		// Register with new tags
		if ( world != nullptr && world->GetTagManager() != nullptr )
		{
			world->GetTagManager()->AddNode( this, m_tags );
			if( m_tags.Size() != 0 )
			{
	#ifndef NO_EDITOR
				world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_NodeTags );
	#endif	// NO_EDITOR
			}
		}
	}
}

void CNode::SetTags( TagList&& tagList )
{
	if ( IsDestroyed() )
	{
		return; // Fail SetTags() if node is already destroyed to prevent re-registering back to TagManager
	}

	CWorld* world = ( GetLayer() != nullptr ) ? GetLayer()->GetWorld() : nullptr;

	// do not change tags to the same values
	// NOTE: this DOES NOT WORK properly for some reason - it still requires investigating why
	//if ( !TagList::MatchAll( tagList, m_tags ) )
	{
		// Unregister node
		if( world != nullptr && world->GetTagManager() != nullptr )
		{
			world->GetTagManager()->RemoveNode( this, m_tags );
	#ifndef NO_EDITOR
			world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_NodeTags );
	#endif	// NO_EDITOR
		}

		// Change tags
		m_tags = Move( tagList );

		// Register with new tags
		if ( world != nullptr && world->GetTagManager() != nullptr )
		{
			world->GetTagManager()->AddNode( this, m_tags );
			if( m_tags.Size() != 0 )
			{
	#ifndef NO_EDITOR
				world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_NodeTags );
	#endif	// NO_EDITOR
			}
		}
	}
}

void CNode::Select( Bool state )
{
}

IAttachment* CNode::Attach( CNode* child, const AttachmentSpawnInfo& info )
{
	ASSERT( info.GetAttachmentClass() );
	ASSERT( child );

	// Create attachment
	IAttachment* attachment = CreateObject< IAttachment >( info.GetAttachmentClass(), this );
	if ( !attachment )
	{
		// Unable to create attachment object
		WARN_ENGINE( TXT("Unable to spawn attachment object") );
		return NULL;
	}

	// Initialize attachment
	if ( !attachment->Init( this, child, &info ) )
	{
		// Unable to initialize attachment
		WARN_ENGINE( TXT("Unable to initialize attachment object") );
		return NULL;
	}

	// Created
	return attachment;
}

IAttachment* CNode::Attach( CNode* child, CClass* attachmentClass )
{
	ASSERT( attachmentClass );
	ASSERT( !attachmentClass->IsAbstract() );
	ASSERT( attachmentClass->IsBasedOn( ClassID< IAttachment >() ) );
	ASSERT( child );

	// Create attachment
	IAttachment* attachment = CreateObject< IAttachment >( attachmentClass, this );
	if ( !attachment )
	{
		// Unable to create attachment object
		WARN_ENGINE( TXT("Unable to spawn attachment object") );
		return NULL;
	}

	// Initialize attachment
	if ( !attachment->Init( this, child, NULL ) )
	{
		// Discard the failed attachment
		attachment->Discard();

		// Unable to initialize attachment
		WARN_ENGINE( TXT("Unable to initialize attachment object") );
		return NULL;
	}

	// Created
	return attachment;
}

IAttachment* CNode::Attach( CNode* child, IAttachment* attachment )
{
	if ( !attachment )
	{
		// Unable to create attachment object
		WARN_ENGINE( TXT("Null attachment object given") );
		return NULL;
	}

	// Initialize attachment
	if ( !attachment->Init( this, child, NULL ) )
	{
		// Unable to initialize attachment
		WARN_ENGINE( TXT("Unable to initialize attachment object") );
		return NULL;
	}

	// Created
	return attachment;
}


Bool CNode::HasChild( const CNode* node, const CClass* attachmentClass ) const
{
	for ( TList< IAttachment* >::const_iterator it = m_childAttachments.Begin(); it != m_childAttachments.End(); ++it )
	{
		const IAttachment* att = *it;
		if ( att->GetChild() == node && att->IsA( attachmentClass ) )
		{
			return true;
		}
	}

	return false;
}

void CNode::CheckUpdateTransformMode()
{
	//if ( AsComponent() ) // x2 Clear flag is faster then vistual call
	//{
		if ( !m_transformParent && AsComponent() ) // So we test AsComponent here
		{
			if ( m_transform.IsIdentity() )
			{
				SetFlag( NF_UseParentTransform );
				//ClearFlag( NF_UseFastUpdateTransform );
			}
			else
			{
				ClearFlag( NF_UseParentTransform );
				//SetFlag( NF_UseFastUpdateTransform );
			}
		}
		else
		{
			ClearFlag( NF_UseParentTransform );
			//ClearFlag( NF_UseFastUpdateTransform );
		}
	//}
}

void CNode::OnParentAttachmentAdded( IAttachment* attachment )
{
	// Added parent hard attachment - a old "parent transform"
	if ( attachment->IsA< CHardAttachment >() )
	{
		// Duplicated parent transform !
		ASSERT( !m_transformParent );

		// Set transform parent
		m_transformParent = SafeCast< CHardAttachment >( attachment );
	}

	// Add to list of attachments
	ASSERT( attachment );
	m_parentAttachments.PushBack( attachment );

	// Make a temp copy of the listeners array, so listeners can be (un/)registered while processing, without affecting what
	// we're doing here.
	// It's assumed that this isn't going to be happening often, and that the listeners list should be small enough that the
	// extra copy and checks won't be too expensive.
	TDynArray< INodeAttachListener* > tempListeners = m_attachListeners;
	const Uint32 numListeners = tempListeners.Size();
	for ( Uint32 i = 0; i < numListeners; ++i )
	{
		INodeAttachListener* listener = tempListeners[ i ];

		// Make sure this listener hasn't been unregistered since we started out. If it was unregistered, maybe it's been destroyed?
		if ( m_attachListeners.Exist( listener ) )
		{
			listener->OnNotifyNodeParentAttached( attachment->GetParent(), this, attachment );
		}
	}

	CheckUpdateTransformMode();
}

void CNode::OnChildAttachmentAdded( IAttachment* attachment )
{
	// Add to list of attachments
	ASSERT( attachment );
	m_childAttachments.PushBack( attachment );

	// As above, make a temp copy of the listeners array, so listeners can be (un/)registered while processing, without affecting what
	// we're doing here.
	TDynArray< INodeAttachListener* > tempListeners = m_attachListeners;
	const Uint32 numListeners = tempListeners.Size();
	for ( Uint32 i = 0; i < numListeners; ++i )
	{
		INodeAttachListener* listener = tempListeners[ i ];

		if ( m_attachListeners.Exist( listener ) )
		{
			listener->OnNotifyNodeChildAttached( this, attachment->GetChild(), attachment );
		}
	}
}

void CNode::OnParentAttachmentBroken( IAttachment* attachment )
{
	// Parent transform was removed
	if ( attachment == m_transformParent )
	{
		m_transformParent = NULL;
	}

	// Remove from list of attachments
	ASSERT( attachment );
	m_parentAttachments.Remove( attachment );

	// As above, make a temp copy of the listeners array, so listeners can be (un/)registered while processing, without affecting what
	// we're doing here.
	TDynArray< INodeAttachListener* > tempListeners = m_attachListeners;
	const Uint32 numListeners = tempListeners.Size();
	for ( Uint32 i = 0; i < numListeners; ++i )
	{
		INodeAttachListener* listener = tempListeners[ i ];

		if ( m_attachListeners.Exist( listener ) )
		{
			listener->OnNotifyNodeParentDetached( attachment->GetParent(), this );
		}
	}

	CheckUpdateTransformMode();
}

void CNode::OnChildAttachmentBroken( IAttachment* attachment )
{
	// Remove from list of attachments
	ASSERT( attachment );
	m_childAttachments.Remove( attachment );

	// As above, make a temp copy of the listeners array, so listeners can be (un/)registered while processing, without affecting what
	// we're doing here.
	TDynArray< INodeAttachListener* > tempListeners = m_attachListeners;
	const Uint32 numListeners = tempListeners.Size();
	for ( Uint32 i = 0; i < numListeners; ++i )
	{
		INodeAttachListener* listener = tempListeners[ i ];

		if ( m_attachListeners.Exist( listener ) )
		{
			listener->OnNotifyNodeChildDetached( this, attachment->GetChild() );
		}
	}
}

void CNode::RemoveNullParentAttachments()
{
	m_parentAttachments.Remove( NULL );
}

void CNode::RemoveNullChildAttachments()
{
	m_childAttachments.Remove( NULL );
}

String CNode::GetUniqueName() const
{
	CLayer* layer = GetLayer();
	String ret;
	if( layer && layer->GetLayerInfo() )
		ret = layer->GetLayerInfo()->GetShortName() + TXT( "/" );
	Vector pos = GetWorldPosition();
	return ret + GetName() + TXT( "@" ) + 
		ToString( pos.X ) + TXT( "," ) +
		ToString( pos.Y ) + TXT( "," ) +
		ToString( pos.Z );
}


Bool CNode::RegisterNodeAttachListener( INodeAttachListener* listener )
{
	ASSERT( listener, TXT("Registering NULL listener") );
	if ( !listener ) return false;

	if ( m_attachListeners.PushBackUnique( listener ) )
	{
#ifndef RED_FINAL_BUILD
		listener->m_listenedNodes.PushBack( this );
#endif
		return true;
	}

	HALT( "Trying to register an INodeAttachListener that is already registered to this node" );

	return false;
}

Bool CNode::UnregisterNodeAttachListener( INodeAttachListener* listener )
{
	ASSERT( listener, TXT("Unregistering NULL listener") );
	if ( !listener ) return false;

	if ( m_attachListeners.RemoveFast( listener ) )
	{
#ifndef RED_FINAL_BUILD
		listener->m_listenedNodes.RemoveFast( this );
#endif
		return true;
	}

	HALT( "Trying to unregister an INodeAttachListener that is not registered to this node" );

	return false;
}


void CNode::funcGetName( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRING( GetName() );
}

void CNode::funcGetLocalPosition( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, GetPosition() );
}

void CNode::funcGetLocalRotation( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( EulerAngles, GetRotation() );
}

void CNode::funcGetLocalScale( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, GetScale() );
}

void CNode::funcGetLocalToWorld( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( Matrix, GetLocalToWorld() );
}

void CNode::funcGetWorldPosition( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, GetWorldPosition() );
}

void CNode::funcGetWorldRotation( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( EulerAngles, GetWorldRotation() );
}

void CNode::funcGetWorldForward( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, GetWorldForward());
}

void CNode::funcGetWorldRight( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, GetWorldRight());
}

void CNode::funcGetWorldUp( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, GetWorldUp());
}

void CNode::funcGetHeading( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_FLOAT( GetWorldYaw() );
}

void CNode::funcGetHeadingVector( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, m_localToWorld.GetAxisY() );
}

Bool CNode::HasTag( const CName tag ) const
{
	// Check if this node has a tag
	if ( m_tags.HasTag( tag ) ) 
	{
		return true;
	}

	// For entity, also check if the entity's template and includes have a tag
	// (NOTE: if this proves to be slow it might be a good idea to cache the
	// tags in a m_runtimeTags or something like that to avoid scanning the
	// whole include hierarchy all the time)
	const CEntity* entity = AsEntity();
	if ( entity != nullptr && entity->GetEntityTemplate() != nullptr )
	{
		struct  {
			Bool Scan( CEntityTemplate* tpl, const CName tag )
			{
				// Check this template
				if ( tpl->GetEntityObject() && tpl->GetEntityObject()->HasTag( tag ) )
				{
					return true;
				}

				// Check the includes
				const auto& includes = tpl->GetIncludes();
				for ( const auto& subtpl : includes )
				{
					if ( subtpl != nullptr && Scan( subtpl, tag ) )
					{
						return true;
					}
				}

				// No tag
				return false;
			}
		} local;

		// Check if the entity template's own includes have the tag
		if ( local.Scan( entity->GetEntityTemplate(), tag ) )
		{
			return true;
		}

		// Check if any of the appearance templates have the tag
		CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( entity );
		if ( appearanceComponent != nullptr && appearanceComponent->HasTagInAppearanceEntities( tag ) )
		{
			return true;
		}
	}

	// No tag
	return false;
}

void CNode::funcHasTag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tagName, CName::NONE );
	FINISH_PARAMETERS;

	Bool res = m_tags.HasTag( tagName );
	RETURN_BOOL( res );
}

void CNode::funcGetTags( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( TagList::TTagList, GetTags().GetTags() );
}

void CNode::funcSetTags( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TDynArray< CName >, tags, TDynArray< CName >() );
	FINISH_PARAMETERS;
	
	SetTags( tags );
}

void CNode::funcGetTagsString( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRING( GetTags().ToString() );
}

void CNode::funcSetPosition( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, position, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;
	SetPosition( position );
}

Bool CNode::ShouldGenerateEditorFragments( CRenderFrame* frame ) const
{
	Float dist = frame->GetFrameInfo().GetRenderingDebugOption( VDCommon_MaxRenderingDistance );
	Vector currentPos = GetLocalToWorld().GetTranslation();
	Float distFromCam = frame->GetFrameInfo().m_camera.GetPosition().DistanceSquaredTo( currentPos );
	if ( distFromCam < dist )
	{
		return true;
	}
	else
	{
		return false;
	}
}

void CNode::OnAttached( CWorld* world )
{
#ifndef NO_EDITOR
	if( world != nullptr )
	{
		if( m_tags.Size() != 0 )
		{
			world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_NodeTags );
		}
	}
#endif	// NO_EDITOR
}

void CNode::OnDetached( CWorld* world )
{
#ifndef NO_EDITOR
	if( world != nullptr )
	{
		world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_NodeTags );
	}	
#endif	// NO_EDITOR
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
static void funcFindClosestNode( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, position, Vector::ZEROS );
	GET_PARAMETER_REF( TDynArray< THandle< CNode > >, nodes, TDynArray< THandle< CNode > >() );
	FINISH_PARAMETERS;

	const Uint32 size = nodes.Size();
	if( size == 0 )
	{
		RETURN_OBJECT( NULL );
	}
	else if( size == 1 )
	{
		RETURN_OBJECT( nodes[0].Get() );
	}
	else
	{
		Float bestDistSquared = NumericLimits< Float >::Max();
		Uint32 bestIdx = 0;
		for( Uint32 i=0; i<size; i++ )
		{
			CNode* node = nodes[i].Get();
			Float distSquared;
			if( node )
				distSquared = node->GetWorldPosition().DistanceSquaredTo( position );
			else
				distSquared = NumericLimits< Float >::Max();

			if( distSquared < bestDistSquared )
			{
				bestDistSquared = distSquared;
				bestIdx = i;
			}
		}

		RETURN_OBJECT( nodes[bestIdx].Get() );
	}
}

static void funcSortNodesByDistance( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, position, Vector::ZEROS );
	GET_PARAMETER_REF( TDynArray< THandle< CNode > >, nodes, TDynArray< THandle< CNode > >() );
	FINISH_PARAMETERS;

	if( nodes.Size() <= 1 )
		return;

	struct NodeDistance
	{
		THandle< CNode >	m_node;
		Float				m_distSquared;
	};

	TDynArray< NodeDistance > distanceArray( nodes.Size() );
	for( Uint32 i=0; i<nodes.Size(); i++ )
	{
		distanceArray[i].m_node = nodes[i];
		CNode* node = nodes[i].Get();
		if( node )
			distanceArray[i].m_distSquared = node->GetWorldPosition().DistanceSquaredTo( position );
		else
			distanceArray[i].m_distSquared = NumericLimits< Float >::Max();
	}

	struct Local
	{
		static Bool Pred( const NodeDistance& nd0, const NodeDistance& nd1 )
		{			
			return nd0.m_distSquared < nd1.m_distSquared;
		}		
	};

	Sort( distanceArray.Begin(), distanceArray.End(), Local::Pred );
	for( Uint32 i=0; i<nodes.Size(); i++ )
	{
		nodes[i] = distanceArray[i].m_node;
	}
}

void RegisterNodeFunctions()
{
	NATIVE_GLOBAL_FUNCTION("FindClosestNode", funcFindClosestNode )
	NATIVE_GLOBAL_FUNCTION("SortNodesByDistance", funcSortNodesByDistance )
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_TRANS_MGR
#pragma optimize("",off)

namespace
{
	Int32 DummyFunc()
	{
		Int32 i( 0 );
		i++;
		return i;
	}
}
#endif

void CNode::UpdateLocalToWorld()
{
	if ( UseParentUpdateTransform() )
	{
		RED_ASSERT( AsComponent() != nullptr );
		m_localToWorld = static_cast< CComponent* >( this )->GetEntity()->GetLocalToWorld(); // TODO - parent local to world needs to be arg for this func = no cache miss

#ifdef DEBUG_TRANS_MGR
		if ( CLayer* l = GetLayer() )
		{
			if ( CWorld* w = l->GetWorld() )
			{
				w->GetUpdateTransformManager().m_numSimple.Increment();
			}
			else
			{
				DummyFunc();
			}
		}
		else
		{
			DummyFunc();
		}
#endif
	}
	/*else if ( UseFastUpdateTransform() )
	{
		RED_ASSERT( AsComponent() != nullptr );
		RED_ASSERT( !m_transform.IsIdentity() );
		
		const Matrix& parentLocalToWorld = static_cast< CComponent* >( this )->GetEntity()->GetLocalToWorld(); // TODO - parent local to world needs to be arg for this func = no cache miss
		
		Matrix localToParent;
		m_transform.CalcLocalToWorld( localToParent );

		m_localToWorld = localToParent * parentLocalToWorld;

#ifdef DEBUG_TRANS_MGR
		if ( CLayer* l = GetLayer() )
		{
			if ( CWorld* w = l->GetWorld() )
			{
				w->GetUpdateTransformManager().m_numFast.Increment();
			}
			else
			{
				DummyFunc();
			}
		}
		else
		{
			DummyFunc();
		}
#endif
	}*/
	else
	{
#ifdef DEBUG_TRANS_MGR
		if ( CLayer* l = GetLayer() )
		{
			if ( CWorld* w = l->GetWorld() )
			{
				if ( AsComponent() )
				{
					w->GetUpdateTransformManager().m_numRest.Increment();
				}
				else
				{
					w->GetUpdateTransformManager().m_numEntities.Increment();
				}
			}
			else
			{
				DummyFunc();
			}
		}
		else
		{
			DummyFunc();
		}
#endif

		// Directly use world space transformation from component
		const Bool hasIdentity = m_transform.IsIdentity();
		
		RedMatrix4x4 &localToWorld = reinterpret_cast<RedMatrix4x4&>(m_localToWorld);

		// Assemble final transform
		if ( m_transformParent && m_transformParent->GetParent() )
		{
			// Get transformable component we are attached to and use it to calculate attachment target 
			if ( hasIdentity )
			{
				m_transformParent->CalcAttachedLocalToWorld( m_localToWorld );
			}
			else
			{
				RedMatrix4x4 localToParent, attachedLocalToWorld;
				m_transform.CalcLocalToWorld( reinterpret_cast<Matrix&>(localToParent) );
				m_transformParent->CalcAttachedLocalToWorld( reinterpret_cast<Matrix&>(attachedLocalToWorld) );
				Mul( localToWorld, localToParent, attachedLocalToWorld );
			}
		}
		else 
		{
			// Use the node to transform crap
			const CNode* parentNode = AsComponent() ? static_cast< CComponent* >( this )->GetEntity() : nullptr;
			if ( parentNode )
			{
				// Get transformable component we are attached to and use it to calculate attachment target 
				const RedMatrix4x4 &parentToWorld = reinterpret_cast<const RedMatrix4x4&>(parentNode->GetLocalToWorld());

				// Multiply with local transform
				if ( hasIdentity )
				{
					// If we don't have any local transform copy the parent matrix directly
					localToWorld = parentToWorld;
				}
				else
				{
					// Assemble final local to world matrix
					RedMatrix4x4 localToParent;
					m_transform.CalcLocalToWorld( reinterpret_cast<Matrix&>(localToParent) );
					Mul( localToWorld, localToParent, parentToWorld);
				}
			}
			else
			{
				// No parent node, use local matrix directly
				CalcLocalTransformMatrix( m_localToWorld );
			}
		}
	}
}


Bool CNode::UsesAutoUpdateTransform()
{
	return true;
}

void CNode::UpdateTransformNode( SUpdateTransformContext& context, Bool parentScheduledUpdateTransform )
{
	PC_SCOPE( NodeUpdateTransform );

	const Bool scheduledUpdateTransform = parentScheduledUpdateTransform || HasFlag( NF_ScheduledUpdateTransform );
	if ( scheduledUpdateTransform )
	{
#ifdef DEBUG_UPDATE_TRANSFORM
		const Uint64 currTickID = GEngine->GetCurrentEngineTick();
		if ( m_debugUpdateID != -1 && currTickID > m_debugUpdateForceID+1 && m_debugUpdateID == currTickID && !IsA< CCamera >() )
		{
			RED_ASSERT( m_debugUpdateID != currTickID );
		}
		m_debugUpdateID = currTickID;
#endif

		Matrix prevLocalToWorld( m_localToWorld ); // Oh nooo - look CRigidMeshComponent::OnUpdateTransform() or CParticleComponent::OnUpdateTransform()
		UpdateLocalToWorld();

		OnUpdateTransformNode( context, prevLocalToWorld );
	}

	UpdateTransformChildrenNodes( context, scheduledUpdateTransform );

	ClearFlag( NF_ScheduledUpdateTransform );
}

void CNode::UpdateTransformChildrenNodes( SUpdateTransformContext& context, Bool parentScheduledUpdateTransform )
{
	for ( TList< IAttachment* >::iterator it=m_childAttachments.Begin(); it!=m_childAttachments.End(); ++it )
	{
		IAttachment* att = *it;
		if ( CHardAttachment* attachment = att->ToHardAttachment() )
		{
			ASSERT( attachment->GetParent() == this );

			CNode* childNode = attachment->GetChild();
			if ( childNode && childNode->IsNotDestroyed() && ( ( parentScheduledUpdateTransform && childNode->UsesAutoUpdateTransform() ) || ( childNode->HasFlag( NF_MarkUpdateTransform ) || childNode->HasFlag( NF_ScheduledUpdateTransform ) ) ) )
			{
				childNode->ClearFlag( NF_MarkUpdateTransform );

				if ( CEntity* e = childNode->AsEntity() )
				{
					e->UpdateTransformEntity( context, parentScheduledUpdateTransform );
				}
				else
				{
					childNode->UpdateTransformNode( context, parentScheduledUpdateTransform );
				}

				RED_ASSERT( !childNode->HasFlag( NF_ScheduledUpdateTransform ) );
				RED_ASSERT( !childNode->HasFlag( NF_MarkUpdateTransform ) );
			}
		}
	}
}

void CNode::OnUpdateTransformNode( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{

}

CLayer* CNode::CheckLayer( CLayer* l )
{
	if ( !l )
	{
		CLayer* layer = GetLayer();
		return layer && layer->IsAttached() ? layer : nullptr;
	}
	else
	{
		return l;
	}
}

void CNode::ScheduleUpdateTransformNode( CLayer* _layer )
{
	if ( !HasFlag( NF_ScheduledUpdateTransform ) && IsAttached() )
	{
		if ( CLayer* l = CheckLayer( _layer ) )
		{
			SetFlag( NF_ScheduledUpdateTransform );

			if ( CEntity* e = AsEntity() )
			{
 				for ( CComponent* c : e->GetComponents() )
 				{
					if ( c )
					{
						if ( c->IsAttached() && c->IsRootComponent() && c->UsesAutoUpdateTransform() )
						{
							c->ScheduleUpdateTransformNode( l );
						}
					}
				}
			}

			MarkUpdateTransformChain( l );
		}
	}
	/*else if ( IsAttached() )
	{
		CEntity* e = AsEntity();
		if ( !e )
		{
			e = FindParent< CEntity >();
		}
		if ( e )
		{
			while ( e->GetTransformParent() )
			{
				e = e->GetTransformParent()->GetParent()->FindParent< CEntity >();
			}

			if ( !e )
			{
				e = nullptr;
			}
			else
			{
				if ( CLayer* l = CheckLayer( _layer ) )
				{
					static CClass* c = SRTTI::GetInstance().FindClass( CName( TXT("CItemEntity") ) );
					if ( !GetClass()->IsA( c ) )
					{
						CWorld* world = l->GetWorld();
						if ( !world->GetUpdateTransformManager().HasEntityScheduled( e ) )
						{
							e = nullptr;
						}
					}
				}
			}
		}
	}*/
}

void CNode::RefreshMarkUpdateTransformChain( CLayer* layer )
{
	RED_ASSERT( HasFlag( NF_MarkUpdateTransform ), TXT("There wasn't a previous call to MarkUpdateTransformChain!") );
	RED_ASSERT( IsAttached(), TXT("The node needs to be attached!") );
	RED_ASSERT( layer != nullptr, TXT("No layer provided") );

	ClearFlag( NF_MarkUpdateTransform );
	MarkUpdateTransformChain( layer );
}

void CNode::MarkUpdateTransformChain( CLayer* layer )
{
	if ( !HasFlag( NF_MarkUpdateTransform ) )
	{
		Bool hasParent = false;

		SetFlag( NF_MarkUpdateTransform );

		for ( TList< IAttachment* >::iterator it=m_parentAttachments.Begin(), end = m_parentAttachments.End(); it != end; ++it )
		{
#ifndef NO_EDITOR
			if ( (*it) == nullptr )
			{
				continue;
			}
#endif
			CHardAttachment* attachment = (*it)->ToHardAttachment();
			if ( attachment )
			{
				ASSERT( attachment->GetChild() == this );

				hasParent = true;

				CNode* parentNode = attachment->GetParent();
				if ( parentNode->IsAttached() && !parentNode->HasFlag( NF_MarkUpdateTransform ) && parentNode->UsesAutoUpdateTransform() )
				{
					parentNode->MarkUpdateTransformChain( layer );
				}
			}
		}

		if ( !hasParent && IsAttached() )
		{
			CWorld* world = layer->GetWorld();

			if ( CEntity* e = AsEntity() )
			{
				e->SetFlag( NF_MarkUpdateTransform );

				world->GetUpdateTransformManager().ScheduleEntity( e );
			}
			else if ( CComponent* c = AsComponent() ) // Should be 'else' but for debug we have 'else if' now
			{
				RED_ASSERT( c->IsRootComponent() );

				c->GetEntity()->MarkUpdateTransformChain( layer );
			}
			else
			{
				RED_FATAL_ASSERT( 0, "CNode::MarkUpdateTransformChain" );
			}
		}
	}
}

void CNode::ForceUpdateTransformNodeAndCommitChanges()
{
	SUpdateTransformContext context;
	ForceUpdateTransformNode( context );
	context.CommitChanges();
}

void CNode::ForceUpdateTransformNode( SUpdateTransformContext& context )
{
	CEntity* e = AsEntity();

#ifdef DEBUG_UPDATE_TRANSFORM
	if ( e )
	{
		for ( BaseComponentIterator it( e ); it; ++it )
		{
			(*it)->m_debugUpdateID = -1;
			(*it)->m_debugUpdateForceID = GEngine->GetCurrentEngineTick();
		}

		for ( TList< IAttachment* >::iterator it=m_childAttachments.Begin(); it!=m_childAttachments.End(); ++it )
		{
			IAttachment* att = *it;
			CNode* childNode = att->GetChild();
			if ( childNode )
			{
				childNode->m_debugUpdateID = -1;
				childNode->m_debugUpdateForceID = GEngine->GetCurrentEngineTick();
			}
		}
	}
	m_debugUpdateID = -1;
	m_debugUpdateForceID = GEngine->GetCurrentEngineTick();
#endif

	if ( e )
	{
		const Bool removeFromList = HasFlag( NF_ScheduledUpdateTransform ) || HasFlag( NF_MarkUpdateTransform );

		e->UpdateTransformEntity( context, true );

		if ( removeFromList && GetLayer()->GetWorld() )
		{
			e->ClearFlag( NF_ScheduledUpdateTransform );
			e->ClearFlag( NF_MarkUpdateTransform );

			GetLayer()->GetWorld()->GetUpdateTransformManager().UnscheduleEntity( e );
		}
	}
	else if ( CComponent* c = AsComponent() ) // Should be 'else' but for debug we have 'else if' now
	{
		UpdateTransformNode( context, true );
	}
	else
	{
		RED_FATAL_ASSERT( 0, "CNode::ForceUpdateTransformNode" );
	}
}

void CNode::ForceUpdateBoundsNode()
{
	if ( CEntity* e = AsEntity() )
	{
		for ( ComponentIterator< CBoundedComponent > it( e ); it; ++it )
		{
			(*it)->OnUpdateBounds();
		}
	}
	else if ( CBoundedComponent* c = Cast< CBoundedComponent >( this ) )
	{
		c->OnUpdateBounds();
	}
}

void CNode::HACK_UpdateLocalToWorld()
{
	UpdateLocalToWorld();
}

Bool CNode::HACK_ForceUpdateTransformWithGlobalPose( const Matrix& globalPose )
{
	Matrix prevLocalToWorld( m_localToWorld );

	m_transform.Init( globalPose );
	
	CalcLocalTransformMatrix( m_localToWorld );

	SUpdateTransformContext context;
	OnUpdateTransformNode( context, prevLocalToWorld );
	context.CommitChanges();

	return true; // finish update transform after this hack
}

#ifdef DEBUG_TRANS_MGR
#pragma optimize("",on)
#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
