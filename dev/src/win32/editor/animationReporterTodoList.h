/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

enum ETODOViewSort
{
	TDVS_Prio,
	TDVS_Type,
	TDVS_Category,
};

enum EAnimationReporterTaskPrio
{
	ARTP_Low,
	ARTP_Medium,
	ARTP_High,
};

enum EAnimationReporterTaskType
{
	ARTT_Info,
	ARTT_Check,
	ARTT_Warn,
	ARTT_Error,
};

enum EAnimationReporterTaskCategory
{
	ARTC_None,
	ARTC_Animset,
	ARTC_Animation,
	ARTC_Behavior,
	ARTC_JobTree,
	ARTC_AnimComp,
	ARTC_ApComp,
	ARTC_Scripts,
};

enum EAnimationReporterTask
{
	ART_MissingResource,
	ART_EmptySlot,
	ART_DuplicatedAnimation,
	ART_DuplicatedBehaviorSlots,
	ART_AnimNotCompressed,
	ART_ScriptPlaySlotAnim,
};

class EdAnimationReporterTodoTask
{
protected:
	EAnimationReporterTaskPrio		m_prio;
	EAnimationReporterTaskType		m_type;
	EAnimationReporterTaskCategory	m_cat;
	Uint32							m_count;

public:
	EdAnimationReporterTodoTask() {}

	EdAnimationReporterTodoTask( EAnimationReporterTaskPrio prio, EAnimationReporterTaskType type, EAnimationReporterTaskCategory cat )
		: m_prio( prio )
		, m_type( type )
		, m_cat( cat )
		, m_count( 1 )
	{

	}

	wxString GetPrioDesc() const
	{
		switch ( m_prio )
		{
		case ARTP_Low:
			return wxT("Low");
		case ARTP_Medium:
			return wxT("Medium");
		case ARTP_High:
			return wxT("High");
		default:
			return wxT("Invalid");
		}
	}

	wxString GetTypeDesc() const
	{
		switch ( m_type )
		{
		case ARTT_Info:
			return wxT("Info");
		case ARTT_Check:
			return wxT("Check");
		case ARTT_Warn:
			return wxT("Warn");
		case ARTT_Error:
			return wxT("Error");
		default:
			return wxT("Invalid");
		}
	}

	EAnimationReporterTaskType GetTaskType() const
	{
		return m_type;
	}

	Uint32 GetCount() const
	{
		return m_count;
	}

	void AddCount()
	{
		m_count++;
	}

	wxString GetCategoryDesc() const
	{
		switch ( m_cat )
		{
		case ARTC_Animset:
			return wxT("Animset");
		case ARTC_Animation:
			return wxT("Animation");
		case ARTC_Behavior:
			return wxT("Behavior");
		case ARTC_JobTree:
			return wxT("JobTree");
		case ARTC_AnimComp:
			return wxT("AnimatedComp");
		case ARTC_ApComp:
			return wxT("ActionPointComp");
		case ARTC_Scripts:
			return wxT("Scripts");
		default:
			return wxT("Invalid");
		}
	}

	Bool IsEqual( const EdAnimationReporterTodoTask* task ) const
	{
		return m_type == task->m_type && m_prio == task->m_prio && m_cat == task->m_cat && 
			GetType() == task->GetType() && GetDesc() == task->GetDesc() && GetLink() == task->GetLink();
	}

public:
	static int CmpFuncByPrio( const void* elem0, const void* elem1 )
	{
		const EdAnimationReporterTodoTask* record0 = *(const EdAnimationReporterTodoTask**)elem0;
		const EdAnimationReporterTodoTask* record1 = *(const EdAnimationReporterTodoTask**)elem1;
		if ( record0->m_prio < record1->m_prio ) return 1;
		if ( record0->m_prio > record1->m_prio ) return -1;
		return 0;
	}

	static int CmpFuncByCat( const void* elem0, const void* elem1 )
	{
		const EdAnimationReporterTodoTask* record0 = *(const EdAnimationReporterTodoTask**)elem0;
		const EdAnimationReporterTodoTask* record1 = *(const EdAnimationReporterTodoTask**)elem1;
		if ( record0->m_cat < record1->m_cat ) return 1;
		if ( record0->m_cat > record1->m_cat ) return -1;
		return 0;
	}

	static int CmpFuncByType( const void* elem0, const void* elem1 )
	{
		const EdAnimationReporterTodoTask* record0 = *(const EdAnimationReporterTodoTask**)elem0;
		const EdAnimationReporterTodoTask* record1 = *(const EdAnimationReporterTodoTask**)elem1;
		if ( record0->m_type < record1->m_type ) return 1;
		if ( record0->m_type > record1->m_type ) return -1;
		return 0;
	}

public:
	virtual void Serialize( IFile& file )
	{
		if ( file.IsWriter() )
		{
			Uint32 temp;

			temp = (Uint32)m_prio;
			file << temp;

			temp = (Uint32)m_type;
			file << temp;

			temp = (Uint32)m_cat;
			file << temp;
		}
		else
		{
			Uint32 temp = 0;

			file << temp;
			m_prio = (EAnimationReporterTaskPrio)temp;

			file << temp;
			m_type = (EAnimationReporterTaskType)temp;

			file << temp;
			m_cat = (EAnimationReporterTaskCategory)temp;
		}

		file << m_count;
	}

	virtual EAnimationReporterTask GetType() const = 0;
	virtual wxString GetDesc() const = 0;
	virtual wxString GetLink() const = 0;
};

//////////////////////////////////////////////////////////////////////////

class EdAnimationReporterTodoList
{
	TDynArray< EdAnimationReporterTodoTask* > m_tasks;

public:
	EdAnimationReporterTodoList();
	~EdAnimationReporterTodoList();

	void AddTask( EdAnimationReporterTodoTask* task );
	void Clear();

	void Sort( ETODOViewSort sortType );

	void Serialize( IFile& file );

public:
	Uint32 GetTaskNum() const;
	const EdAnimationReporterTodoTask* GetTask( Uint32 i ) const;

public:
	Uint32 GetErrorsNum() const;
	Uint32 GetWarnsNum() const;
	Uint32 GetChecksNum() const;
	Uint32 GetInfosNum() const;

private:
	EdAnimationReporterTodoTask* CreateTask( Uint32 type ) const;
	Uint32 GetNumByType( EAnimationReporterTaskType type ) const;
};

//////////////////////////////////////////////////////////////////////////

class EdAnimationReporterMissingResource : public EdAnimationReporterTodoTask
{
	String m_path;
	String m_exDesc;

public:
	EdAnimationReporterMissingResource() {}

	EdAnimationReporterMissingResource( const String& path, const String& exDesc, EAnimationReporterTaskCategory cat )
		: EdAnimationReporterTodoTask( ARTP_Medium, ARTT_Error, cat )
		, m_path( path )
		, m_exDesc( exDesc )
	{
		
	}

	virtual EAnimationReporterTask GetType() const
	{
		return ART_MissingResource;
	}

	virtual void Serialize( IFile& file )
	{
		EdAnimationReporterTodoTask::Serialize( file );

		file << m_path;
		file << m_exDesc;
	}

	virtual wxString GetDesc() const
	{
		if ( m_exDesc.Empty() )
		{
			return wxT("Missing resource");
		}
		else
		{
			return wxString::Format( wxT("Missing resource - %s"), m_exDesc.AsChar() );
		}
	}

	virtual wxString GetLink() const
	{
		return m_path.AsChar();
	}
};

//////////////////////////////////////////////////////////////////////////

class EdAnimationReporterEmptySlot : public EdAnimationReporterTodoTask
{
	String m_path;
	String m_exDesc;

public:
	EdAnimationReporterEmptySlot() {}

	EdAnimationReporterEmptySlot( const String& path, const String& exDesc, EAnimationReporterTaskCategory cat )
		: EdAnimationReporterTodoTask( ARTP_Low, ARTT_Warn, cat )
		, m_path( path )
		, m_exDesc( exDesc )
	{
		
	}

	virtual EAnimationReporterTask GetType() const
	{
		return ART_EmptySlot;
	}

	virtual void Serialize( IFile& file )
	{
		EdAnimationReporterTodoTask::Serialize( file );

		file << m_path;
		file << m_exDesc;
	}

	virtual wxString GetDesc() const
	{
		if ( m_exDesc.Empty() )
		{
			return wxT("Empty slot");
		}
		else
		{
			return wxString::Format( wxT("Empty slot - %s"), m_exDesc.AsChar() );
		}
	}

	virtual wxString GetLink() const
	{
		return m_path.AsChar();
	}
};

//////////////////////////////////////////////////////////////////////////

class EdAnimationReporterDuplicatedAnimation : public EdAnimationReporterTodoTask
{
	TDynArray< String >	m_animsets;
	CName				m_animation;

public:
	EdAnimationReporterDuplicatedAnimation() {}

	EdAnimationReporterDuplicatedAnimation( TDynArray< String >& animsets, const CName& animation )
		: EdAnimationReporterTodoTask( ARTP_High, ARTT_Error, ARTC_Animation )
		, m_animsets( animsets )
		, m_animation( animation )
	{

	}

	virtual EAnimationReporterTask GetType() const
	{
		return ART_DuplicatedAnimation;
	}

	virtual void Serialize( IFile& file )
	{
		EdAnimationReporterTodoTask::Serialize( file );

		file << m_animsets;
		file << m_animation;
	}

	virtual wxString GetDesc() const
	{
		wxString str = wxString::Format( wxT("Duplicated animation '%s' in %d animsets: "), m_animation.AsString().AsChar(), m_animsets.Size() );

		if ( m_animsets.Size() > 1 )
		{
			for ( Uint32 i=0; i<m_animsets.Size(); ++i )
			{
				str += wxString::Format( wxT("'%s' "), m_animsets[ i ].AsChar() );
			}
		}

		return str;
	}

	virtual wxString GetLink() const
	{
		return m_animsets.Size() > 0 ? m_animsets[ 0 ].AsChar() : TXT("Invalid");
	}
};

//////////////////////////////////////////////////////////////////////////

class EdAnimationReporterDuplicatedBehaviorSlots : public EdAnimationReporterTodoTask
{
	String				m_path;
	String				m_component;
	CName				m_slotName;
	String				m_graphPath;

public:
	EdAnimationReporterDuplicatedBehaviorSlots() {}

	EdAnimationReporterDuplicatedBehaviorSlots( const String& path, const String& component, const CName& slotName, const String& graphPath )
		: EdAnimationReporterTodoTask( ARTP_High, ARTT_Error, ARTC_AnimComp )
		, m_path( path )
		, m_component( component )
		, m_slotName( slotName )
		, m_graphPath( graphPath )
	{

	}

	virtual EAnimationReporterTask GetType() const
	{
		return ART_DuplicatedBehaviorSlots;
	}

	virtual void Serialize( IFile& file )
	{
		EdAnimationReporterTodoTask::Serialize( file );

		file << m_path;
		file << m_component;
		file << m_slotName;
		file << m_graphPath;
	}

	virtual wxString GetDesc() const
	{
		return wxString::Format( wxT("Duplicated behavior slot '%s - %s' for resource '%s', component '%s'"), 
			m_slotName.AsString().AsChar(), m_graphPath.AsChar(), m_path.AsChar(), m_component.AsChar() );
	}

	virtual wxString GetLink() const
	{
		return m_path.AsChar();
	}
};

//////////////////////////////////////////////////////////////////////////

class EdAnimationReporterAnimNotCompressed : public EdAnimationReporterTodoTask
{
	String				m_path;
	CName				m_animaName;

public:
	EdAnimationReporterAnimNotCompressed() {}

	EdAnimationReporterAnimNotCompressed( const String& path, const CName& animaName )
		: EdAnimationReporterTodoTask( ARTP_High, ARTT_Error, ARTC_Animation )
		, m_path( path )
		, m_animaName( animaName )
	{

	}

	virtual EAnimationReporterTask GetType() const
	{
		return ART_AnimNotCompressed;
	}

	virtual void Serialize( IFile& file )
	{
		EdAnimationReporterTodoTask::Serialize( file );

		file << m_path;
		file << m_animaName;
	}

	virtual wxString GetDesc() const
	{
		return wxString::Format( wxT("Animation '%s' is not compressed, animset '%s'"), 
			m_animaName.AsString().AsChar(), m_path.AsChar() );
	}

	virtual wxString GetLink() const
	{
		return m_path.AsChar();
	}
};

//////////////////////////////////////////////////////////////////////////

class EdAnimationReporterScriptPlaySlotAnim : public EdAnimationReporterTodoTask
{
	String				m_scriptPath;

public:
	EdAnimationReporterScriptPlaySlotAnim() {}

	EdAnimationReporterScriptPlaySlotAnim( const String& scriptPath )
		: EdAnimationReporterTodoTask( ARTP_High, ARTT_Error, ARTC_Scripts )
		, m_scriptPath( scriptPath )
	{

	}

	virtual EAnimationReporterTask GetType() const
	{
		return ART_ScriptPlaySlotAnim;
	}

	virtual void Serialize( IFile& file )
	{
		EdAnimationReporterTodoTask::Serialize( file );

		file << m_scriptPath;
	}

	virtual wxString GetDesc() const
	{
		return wxString::Format( wxT("Play slot animation is not used correctly in script '%s'"), m_scriptPath.AsChar() );
	}

	virtual wxString GetLink() const
	{
		return m_scriptPath.AsChar();
	}
};
