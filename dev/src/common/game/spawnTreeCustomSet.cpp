#include "build.h"
#include "spawnTreeCustomSet.h"

#include "../engine/pathlibSimpleBuffers.h"

#include "spawnTreeNodeListOperations.inl"
#include "../core/instanceDataLayoutCompiler.h"
#include "../core/gameSave.h"
#include "../engine/gameTimeManager.h"

IMPLEMENT_ENGINE_CLASS( CSpawnTreeQuestPhase );
IMPLEMENT_ENGINE_CLASS( CSpawnTreeQuestNode );
IMPLEMENT_ENGINE_CLASS( CSpawnTreeTimetableEntry );
//IMPLEMENT_ENGINE_CLASS( CSpawnTreeTimetable );


//////////////////////////////////////////////////////////////////////////
// CSpawnTreeQuestPhase
//////////////////////////////////////////////////////////////////////////
void CSpawnTreeQuestPhase::GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const
{
	rootClasses.PushBack( ISpawnTreeBranch::GetStaticClass() );
}
Bool CSpawnTreeQuestPhase::IsSpawnableByDefault() const
{
	return false;
}
Color CSpawnTreeQuestPhase::GetBlockColor() const
{
	return Color::BROWN;
}
String CSpawnTreeQuestPhase::GetBlockCaption() const
{
	Bool isDefaultPhase = false;

	CObject* parent = GetParent();
	if ( parent && parent->IsA< CSpawnTreeQuestNode >() )
	{
		CSpawnTreeQuestNode* questNode = static_cast< CSpawnTreeQuestNode* >( parent );
		isDefaultPhase = questNode->AmIDefaultPhase( this );
	}

	if ( m_nodeName.Empty() && !isDefaultPhase)
	{
		static const String STR( TXT("Phase UNNAMED") );
		return STR; 
	}
	else
	{
		return String::Printf( TXT("%sPhase %s"), isDefaultPhase ? TXT("Default ") : TXT(""), m_nodeName.AsString().AsChar() );
	}
}
String CSpawnTreeQuestPhase::GetEditorFriendlyName() const
{
	static const String STR( TXT("Phase") );
	return STR; 
}
String CSpawnTreeQuestPhase::GetBitmapName() const
{
	return TXT("IMG_SPAWNTREE_PHASE");
}
Bool CSpawnTreeQuestPhase::CanBeHidden() const
{
	return true;
}
void CSpawnTreeQuestPhase::GetContextMenuDebugOptions( CSpawnTreeInstance& instanceBuffer, TDynArray< String >& outOptions )
{
	if ( !IsActive( instanceBuffer ) )
	{
		outOptions.PushBack( TXT("Debug - Activate phase") );
	}
}
void CSpawnTreeQuestPhase::RunDebugOption( CSpawnTreeInstance& instanceBuffer, Int32 option )
{
	CSpawnTreeQuestNode* questNode = Cast< CSpawnTreeQuestNode >( GetParent() );
	if ( !questNode )
	{
		return;
	}
	questNode->ActivateSpawnPhase( instanceBuffer, m_nodeName );
}

#ifndef NO_EDITOR
void CSpawnTreeQuestPhase::OnCreatedInEditor()
{
	TBaseClass::OnCreatedInEditor();

	// TODO
}
#endif 

//////////////////////////////////////////////////////////////////////////
// CEncounterQuestNode
//////////////////////////////////////////////////////////////////////////
void CSpawnTreeQuestNode::UpdateLogic( CSpawnTreeInstance& instance )
{
	TBaseClass::UpdateLogic( instance );

	if ( IsActive( instance ) )
	{
		if ( instance[ i_currentSpawnPhaseIdx ] < m_spawnPhases.Size() )
		{
			m_spawnPhases[ instance[ i_currentSpawnPhaseIdx ] ]->UpdateLogic( instance );
		}
	}
}
void CSpawnTreeQuestNode::Activate( CSpawnTreeInstance& instance )
{
	instance[ i_active ] = true;
}
void CSpawnTreeQuestNode::Deactivate( CSpawnTreeInstance& instance )
{
	instance[ i_active ] = false;

	DeactivateCurrentSpawnPhase( instance );
}
Bool CSpawnTreeQuestNode::ActivateSpawnPhase( CSpawnTreeInstance& instance, CName phaseName )
{
	Uint16 newSpawnPhase = 0xffff;
	for ( Uint16 i = 0, n = Uint16( m_spawnPhases.Size() ); i < n; ++i )
	{
		if ( m_spawnPhases[ i ]->GetName() == phaseName )
		{
			newSpawnPhase = i;
			break;
		}
	}
	if ( newSpawnPhase != 0xffff && newSpawnPhase != instance[ i_currentSpawnPhaseIdx ] )
	{
		DeactivateCurrentSpawnPhase( instance );
		instance[ i_currentSpawnPhaseIdx ] = newSpawnPhase;
		return true;
	}
	return false;
}
void CSpawnTreeQuestNode::DeactivateCurrentSpawnPhase( CSpawnTreeInstance& instance ) const
{
	// deactivate current spawn phase
	Uint16 currentSpawnPhase = instance[ i_currentSpawnPhaseIdx ];
	if ( currentSpawnPhase < m_spawnPhases.Size() && m_spawnPhases[ currentSpawnPhase ]->IsActive( instance ) )
	{
		m_spawnPhases[ currentSpawnPhase ]->Deactivate( instance );
	}
}
Bool CSpawnTreeQuestNode::AmIDefaultPhase( const CSpawnTreeQuestPhase* phaseNode )
{
	return phaseNode == m_spawnPhases[ 0 ];
}
Bool CSpawnTreeQuestNode::SetSpawnPhase( CSpawnTreeInstance& instance, CName phaseName )
{
	Bool b = ActivateSpawnPhase( instance, phaseName );

	return TBaseClass::SetSpawnPhase( instance, phaseName ) || b;
}
void CSpawnTreeQuestNode::GetSpawnPhases( TDynArray< CName >& outPhaseNames )
{
	for( Uint32 i = 0, n = m_spawnPhases.Size(); i != n; ++i )
	{
		if ( m_spawnPhases[ i ] )
		{
			CName phaseName = m_spawnPhases[ i ]->GetName();
			if ( !phaseName.Empty() )
			{
				outPhaseNames.PushBackUnique( phaseName );
			}
		}
	}
	TBaseClass::GetSpawnPhases( outPhaseNames );
}

void CSpawnTreeQuestNode::GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const
{
	ListOperations::GetRootClassForChildren( rootClasses );
}
Bool CSpawnTreeQuestNode::CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const
{
	return true;
}
Bool CSpawnTreeQuestNode::CanAddChild() const
{
	return true;
}
ISpawnTreeBaseNode* CSpawnTreeQuestNode::GetChildMember( Uint32 i ) const
{
	return m_spawnPhases[ i ];
}
Uint32 CSpawnTreeQuestNode::GetChildMembersCount() const
{
	return m_spawnPhases.Size();
}
void CSpawnTreeQuestNode::AddChild( IEdSpawnTreeNode* child )
{
	ListOperations::AddChild( m_spawnPhases, child );
}
void CSpawnTreeQuestNode::RemoveChild( IEdSpawnTreeNode* node )
{
	ListOperations::RemoveChild( m_spawnPhases, node );
}
Bool CSpawnTreeQuestNode::UpdateChildrenOrder()
{
	Bool dirty = ListOperations::UpdateChildrenOrder( m_spawnPhases );
	return TBaseClass::UpdateChildrenOrder() || dirty;
}
Color CSpawnTreeQuestNode::GetBlockColor() const
{
	return Color::BROWN;
}
String CSpawnTreeQuestNode::GetEditorFriendlyName() const
{
	static const String STR( TXT("SelectPhase") );
	return STR;
}
String CSpawnTreeQuestNode::GetBitmapName() const
{
	return TXT("IMG_SPAWNTREE_PHASE_SETTER");
}
Bool CSpawnTreeQuestNode::IsSpawnableByDefault() const
{
	return false;
}
void CSpawnTreeQuestNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_currentSpawnPhaseIdx;
}
void CSpawnTreeQuestNode::OnInitData( CSpawnTreeInstance& instance, CSpawnTreeInitializationContext& context )
{
	TBaseClass::OnInitData( instance, context );

	instance[ i_currentSpawnPhaseIdx ] = m_spawnPhases.Empty() ?  0xffff : 0;
}

Bool CSpawnTreeQuestNode::IsNodeStateSaving( CSpawnTreeInstance& instance ) const
{
	if ( instance[ i_currentSpawnPhaseIdx ] != 0 )
	{
		return true;
	}
	return TBaseClass::IsNodeStateSaving( instance );
}
void CSpawnTreeQuestNode::SaveNodeState( CSpawnTreeInstance& instance, IGameSaver* writer ) const
{
	writer->WriteValue< Uint16 >( CNAME( spawnPhaseIndex ), instance[ i_currentSpawnPhaseIdx ] );

	TBaseClass::SaveNodeState( instance, writer );
}
Bool CSpawnTreeQuestNode::LoadNodeState( CSpawnTreeInstance& instance, IGameLoader* reader ) const
{
	reader->ReadValue< Uint16 >( CNAME( spawnPhaseIndex ), instance[ i_currentSpawnPhaseIdx ] );

	return TBaseClass::LoadNodeState( instance, reader );
}

#ifndef NO_EDITOR
void CSpawnTreeQuestNode::OnCreatedInEditor()
{
	TBaseClass::OnCreatedInEditor();

	CSpawnTreeQuestPhase* defaultQuestPhase = ::CreateObject< CSpawnTreeQuestPhase >( this );
	defaultQuestPhase->GenerateId();
	m_spawnPhases.PushBack( defaultQuestPhase );

	defaultQuestPhase->OnCreatedInEditor();
}
#endif

//////////////////////////////////////////////////////////////////////////
// CEncounterTimetableEntry
//////////////////////////////////////////////////////////////////////////
CSpawnTreeTimetableEntry::CSpawnTreeTimetableEntry()
{
	m_begin = GameTime( 0, 0, 0, 0 );
	m_end = GameTime( 0, 0, 0, 0 );
}

Bool CSpawnTreeTimetableEntry::MatchTime( GameTime time ) const
{
	Int32 checkSeconds = time.m_seconds % GameTime::DAY.m_seconds;
	if ( m_begin.m_seconds >= m_end.m_seconds )
	{
		return checkSeconds >= m_begin.m_seconds || checkSeconds < m_end.m_seconds;
	}
	else
	{
		return checkSeconds >= m_begin.m_seconds && checkSeconds < m_end.m_seconds;
	}
}

Bool CSpawnTreeTimetableEntry::TestConditions( CSpawnTreeInstance& instance ) const
{
	if ( !TBaseClass::TestConditions( instance ) )
	{
		return false;
	}
	GameTime time = GGame->GetTimeManager()->GetTime();
	if ( !MatchTime( time ) )
	{
		return false;
	}
	return true;
}
Bool CSpawnTreeTimetableEntry::IsSpawnableByDefault() const
{
	return true;
}
//Color CSpawnTreeTimetableEntry::GetBlockColor() const
//{
//	return Color( 128, 255, 40 );
//}

String CSpawnTreeTimetableEntry::GetBlockCaption() const
{
	if ( m_begin == m_end )
	{
		return String( TXT("SpawnEntries" ) );
	}
	else
	{
		Uint32 beginHours = m_begin.Hours();
		Uint32 beginMinutes = m_begin.Minutes();
		Uint32 endHours = m_end.Hours();
		Uint32 endMinutes = m_end.Minutes();
		if ( m_end.m_seconds >= GameTime::DAY.m_seconds )
		{
			endHours = 24;
			endMinutes = 0;
		}

		return String::Printf( TXT("SpawnEntries (%2d:%2d to %2d:%2d)"), beginHours, beginMinutes, endHours, endMinutes );
	}
}

String CSpawnTreeTimetableEntry::GetEditorFriendlyName() const
{
	static const String STR( TXT("SpawnEntries") );
	return STR; 
}

//////////////////////////////////////////////////////////////////////////
// CEncounterTimetable
//////////////////////////////////////////////////////////////////////////
//CClass* CSpawnTreeTimetable::GetRootClassForChildren() const
//{
//	return CSpawnTreeTimetableEntry::GetStaticClass();
//}
//String CSpawnTreeTimetable::GetEditorFriendlyName() const
//{
//	static const String STR( TXT("Timetable") );
//	return STR;
//}