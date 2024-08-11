#pragma once

////////////////////////////////////////////////////////////////////////
// Special rtti declarations for behavior tree nodes that automates
// editor support a little

#define DECLARE_BEHTREE_NODE_INTERNAL( _rttiHeader, _class, _baseClass, _instanceClass, _readableCName )	\
	_rttiHeader;																							\
	protected:																								\
		static struct _class##Info																			\
		{																									\
			typedef _class _MyClass;																		\
			_class##Info();																					\
		}			sm_Info;																				\
	public:																									\
		typedef _instanceClass Instance;																	\
		friend Instance;																				\
		static CName StaticNodeName() { return CNAME( _readableCName ); }									\
		virtual CName GetNodeName() const;																	\
	private:


#define DECLARE_BEHTREE_ABSTRACT_NODE( _class, _baseClass, _instanceClass, _readableCName )					\
	DECLARE_BEHTREE_NODE_INTERNAL( DECLARE_ENGINE_ABSTRACT_CLASS( _class, _baseClass ), _class, _baseClass, _instanceClass, _readableCName )

#define DECLARE_BEHTREE_NODE( _class, _baseClass, _instanceClass, _readableCName )							\
	DECLARE_BEHTREE_NODE_INTERNAL( DECLARE_ENGINE_CLASS( _class, _baseClass, 0 ), _class, _baseClass, _instanceClass, _readableCName )

////////////////////////////////////////////////////////////////////////
struct CBehNodesManager
{
protected:
	typedef CName (*FUNC_NODE_NAME) ();

	struct SBehTreeClassInfo
	{
		FUNC_NODE_NAME	m_niceName;
		Bool			m_isInternalClass;
		SBehTreeClassInfo& operator=( const SBehTreeClassInfo& c )		{ m_niceName = c.m_niceName; return *this; }
	};
	TArrayMap< CName, SBehTreeClassInfo >	m_classes;

	static CBehNodesManager sm_Instance;

	void RegisterInternal( FUNC_NODE_NAME niceName, CClass* classId, Bool internalClass );

public:
	CName GetClassName( CClass* classId ) const
	{
		auto it = m_classes.Find( classId->GetName() );
		if ( it != m_classes.End() )
		{
			return it->m_second.m_niceName();
		}
		return CName::NONE;
	}
	Bool IsClassInternal( CClass* classId ) const
	{
		auto it = m_classes.Find( classId->GetName() );
		if ( it != m_classes.End() )
		{
			return it->m_second.m_isInternalClass;
		}
		return false;
	}
	template< class T >
	RED_INLINE void Register( T* info )
	{
		RegisterInternal( &T::_MyClass::StaticNodeName, T::_MyClass::GetStaticClass(), T::_MyClass::IsInternalNode() );
	}
	static CBehNodesManager& GetInstance()								{ return sm_Instance; }

};

