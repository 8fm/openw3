#pragma once

#include "spawnTreeCompositeMember.h"

class CSpawnTreeQuestPhase : public CSpawnTreeNode
{
	DECLARE_ENGINE_CLASS( CSpawnTreeQuestPhase, CSpawnTreeNode, 0 );
public:
	void						GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const override;
	Bool						IsSpawnableByDefault() const override;
	Color						GetBlockColor() const override;
	String						GetBlockCaption() const override;
	String						GetEditorFriendlyName() const override;
	String						GetBitmapName() const override;
	Bool						CanBeHidden() const  override;
	void						GetContextMenuDebugOptions( CSpawnTreeInstance& instanceBuffer, TDynArray< String >& outOptions ) override;
	void						RunDebugOption( CSpawnTreeInstance& instanceBuffer, Int32 option ) override;

#ifndef NO_EDITOR
	void						OnCreatedInEditor() override;
#endif
};


BEGIN_CLASS_RTTI( CSpawnTreeQuestPhase );
	PARENT_CLASS( CSpawnTreeNode );
END_CLASS_RTTI();

class CSpawnTreeQuestNode : public ISpawnTreeBranch
{
	DECLARE_ENGINE_CLASS( CSpawnTreeQuestNode, ISpawnTreeBranch, 0 );

	typedef TListOperations< TDynArray< CSpawnTreeQuestPhase* >, CSpawnTreeQuestPhase > ListOperations;

protected:
	TDynArray< CSpawnTreeQuestPhase* >			m_spawnPhases;

	TInstanceVar< Uint16 >						i_currentSpawnPhaseIdx;
	
	void						DeactivateCurrentSpawnPhase( CSpawnTreeInstance& instance ) const;
public:
	CSpawnTreeQuestNode()														{}

	void						UpdateLogic( CSpawnTreeInstance& instance ) override;
	void						Activate( CSpawnTreeInstance& instance ) override;
	void						Deactivate( CSpawnTreeInstance& instance ) override;

	Bool						ActivateSpawnPhase( CSpawnTreeInstance& instance, CName phaseName );
	Bool						AmIDefaultPhase( const CSpawnTreeQuestPhase* phaseNode );

	Bool						SetSpawnPhase( CSpawnTreeInstance& instance, CName phaseName ) override;
	void						GetSpawnPhases( TDynArray< CName >& outPhaseNames ) override;
	

	void						GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const override;
	Bool						CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const override;
	Bool						CanAddChild() const override;
	ISpawnTreeBaseNode*			GetChildMember( Uint32 i ) const override;
	Uint32						GetChildMembersCount() const override;
	void						AddChild( IEdSpawnTreeNode* child ) override;
	void						RemoveChild( IEdSpawnTreeNode* node ) override;
	Bool						UpdateChildrenOrder() override;
	Color						GetBlockColor() const override;
	String						GetEditorFriendlyName() const override;
	String						GetBitmapName() const override;
	Bool						IsSpawnableByDefault() const override;

	// Instance buffer interface
	void						OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void						OnInitData( CSpawnTreeInstance& instance, CSpawnTreeInitializationContext& context ) override;

	// Saving state
	Bool						IsNodeStateSaving( CSpawnTreeInstance& instance ) const override;
	void						SaveNodeState( CSpawnTreeInstance& instance, IGameSaver* writer ) const override;
	Bool						LoadNodeState( CSpawnTreeInstance& instance, IGameLoader* reader ) const override;

#ifndef NO_EDITOR
	void						OnCreatedInEditor() override;
#endif
};

BEGIN_CLASS_RTTI( CSpawnTreeQuestNode );
	PARENT_CLASS( ISpawnTreeBranch );
	PROPERTY( m_spawnPhases );
END_CLASS_RTTI();

class CSpawnTreeTimetableEntry : public CSpawnTreeEntryList
{
	DECLARE_ENGINE_CLASS( CSpawnTreeTimetableEntry, CSpawnTreeEntryList, 0 );
protected:
	GameTime					m_begin;
	GameTime					m_end;

	Bool						MatchTime( GameTime time ) const;
	Bool						TestConditions( CSpawnTreeInstance& instance ) const override;
public:
	CSpawnTreeTimetableEntry();

	Bool						IsSpawnableByDefault() const override;
	//Color						GetBlockColor() const override;
	String						GetBlockCaption() const override;
	String						GetEditorFriendlyName() const override;
};

BEGIN_CLASS_RTTI( CSpawnTreeTimetableEntry );
	PARENT_CLASS( CSpawnTreeEntryList );
	PROPERTY_CUSTOM_EDIT( m_begin, TXT( "Starting daytime" ), TXT( "DayTimeEditor" ) );
	PROPERTY_CUSTOM_EDIT( m_end, TXT( "Ending daytime" ), TXT( "DayTimeEditor" ) );
END_CLASS_RTTI();

//class CSpawnTreeTimetable : public CSpawnTreeNode
//{
//	DECLARE_ENGINE_CLASS( CSpawnTreeTimetable, CSpawnTreeNode, 0 );
//
//public:
//	CClass*						GetRootClassForChildren() const override;
//	String						GetEditorFriendlyName() const override;
//};
//
//BEGIN_CLASS_RTTI( CSpawnTreeTimetable );
//	PARENT_CLASS( CSpawnTreeNode );
//END_CLASS_RTTI();

