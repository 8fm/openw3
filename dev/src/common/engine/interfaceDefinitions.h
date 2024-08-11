
#pragma once

//////////////////////////////////////////////////////////////////////////
/*
	//////////////////////////////////////////////////////////////////////////
	// Define your interfaces
	
	.h
	class InterfaceX
	{
		RED_REGISTER_INTERFACE( InterfaceX );

	public:
		virtual void funcX() = 0;
	};

	class InterfaceY
	{
		RED_REGISTER_INTERFACE( InterfaceY );

	public:
		virtual void funcY() = 0;
	};
	
	
	.cpp
	RED_IMPLEMENT_INTERFACE( InterfaceX )
	RED_IMPLEMENT_INTERFACE( InterfaceY )



	//////////////////////////////////////////////////////////////////////////
	// Add RED_DECL_INTERFACE_SUPPORT macro to base class for final queries
	
	class BaseClass
	{
		RED_DECL_INTERFACE_SUPPORT()
	};



	//////////////////////////////////////////////////////////////////////////
	// Define your classes
	
	class A : public BaseClass
			, public InterfaceX
	{
		RED_DECL_INTERFACE_SUPPORT_1( InterfaceX )

	public:
		virtual void funcX() {}
	};

	class B : public BaseClass
			, public InterfaceY
	{
		RED_DECL_INTERFACE_SUPPORT_1( InterfaceY )

	public:
		virtual void funcY() {}
	};

	class C : public BaseClass
			, public InterfaceX
			, public InterfaceY
	{
		RED_DECL_INTERFACE_SUPPORT_2( InterfaceX, InterfaceY )

	public:
		virtual void funcX() {}
		virtual void funcY() {}
	};

	// Use query pattern for your final code
	for ( Uint32 i=0; i<m_objects.Size(); ++i )
	{
		BaseClass* baseObj = m_objects[ i ];

		if ( InterfaceX* inter = baseObj->QueryInterface< InterfaceX >() )
		{
			inter->funcX();
		}
		if ( InterfaceY* inter = baseObj->QueryInterface< InterfaceY >() )
		{
			inter->funcY();
		}
	};


	//////////////////////////////////////////////////////////////////////////
	// You can add interfaces if your base class already has few interfaces defined

	class DA	: public A
				, public InterfaceY
	{
		RED_ADD_INTERFACE_SUPPORT_1( InterfaceY )

	public:
		virtual void funcY() {}
	};

*/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#define RED_INTERFACE_ID const CName&

#define RED_REGISTER_INTERFACE( _class )\
public:\
	static RED_INTERFACE_ID _GetId();\
private:

#define RED_IMPLEMENT_INTERFACE( _class )\
	RED_DEFINE_NAME( _class )\
	RED_INTERFACE_ID _class::_GetId() { return CNAME( _class ); }

// No define needed
#define RED_DECL_CLASS_INTERFACE()\
public:\
	template < class _class > _class* QueryInterface()\
	{\
		return static_cast< _class* >( this->VQueryInterface( _class::_GetId() ) );\
	}

#define RED_ADD_CLASS_INTERFACE()\
	RED_DECL_CLASS_INTERFACE()

#define RED_DECL_IMPL_SINGLE_INTERFACE_START( _classA )\
if ( _classA::_GetId() == interfaceId )\
{\
	return static_cast< _classA* >( this );\
}

#define RED_DECL_IMPL_SINGLE_INTERFACE( _classA )\
else if ( _classA::_GetId() == interfaceId )\
{\
	return static_cast< _classA* >( this );\
}

// TODO - We can not have template <> _classA* QueryInterface() function here because of Clang :(
#define RED_DECL_SINGLE_INTERFACE( _classA )
/*#define RED_DECL_SINGLE_INTERFACE( _classA )\
template <> _classA* QueryInterface()\
{\
	return static_cast< _classA* >( this );\
}*/

#define RED_ADD_SINGLE_INTERFACE( _classA )\
	RED_DECL_SINGLE_INTERFACE( _classA )

//////////////////////////////////////////////////////////////////////////

#define RED_INTERFACE_RETURN_NULL return NULL;
#define RED_INTERFACE_RETURN_BASE return TBaseClass::VQueryInterface( interfaceId ); // Here should be something like __super but it's only for MS. Now we use TBaseClass but it can be an issue because we can use interfaces with not CObject classes

// 1
#define RED_DECL_IMPL_INTERFACE_1_R( _classA, ret )\
protected:\
	virtual void* VQueryInterface( RED_INTERFACE_ID interfaceId )\
	{\
		RED_DECL_IMPL_SINGLE_INTERFACE_START( _classA )\
		ret\
	}\
private:

#define RED_DECL_IMPL_INTERFACE_1( _classA )\
	RED_DECL_IMPL_INTERFACE_1_R( _classA, RED_INTERFACE_RETURN_NULL )

#define RED_ADD_IMPL_INTERFACE_1( _classA )\
	RED_DECL_IMPL_INTERFACE_1_R( _classA, RED_INTERFACE_RETURN_BASE )

// 2
#define RED_DECL_IMPL_INTERFACE_2_R( _classA, _classB, ret )\
protected:\
	virtual void* VQueryInterface( RED_INTERFACE_ID interfaceId )\
	{\
		RED_DECL_IMPL_SINGLE_INTERFACE_START( _classA )\
		RED_DECL_IMPL_SINGLE_INTERFACE( _classB )\
		ret\
	}\
private:

#define RED_DECL_IMPL_INTERFACE_2( _classA, _classB )\
	RED_DECL_IMPL_INTERFACE_2_R( _classA, _classB, RED_INTERFACE_RETURN_NULL );

#define RED_ADD_IMPL_INTERFACE_2( _classA, _classB )\
	RED_DECL_IMPL_INTERFACE_2_R( _classA, _classB, RED_INTERFACE_RETURN_BASE );

// 3
#define RED_DECL_IMPL_INTERFACE_3_R( _classA, _classB, _classC, ret )\
protected:\
	virtual void* VQueryInterface( RED_INTERFACE_ID interfaceId )\
	{\
		RED_DECL_IMPL_SINGLE_INTERFACE_START( _classA )\
		RED_DECL_IMPL_SINGLE_INTERFACE( _classB )\
		RED_DECL_IMPL_SINGLE_INTERFACE( _classC )\
		ret\
	}\
private:

#define RED_DECL_IMPL_INTERFACE_3( _classA, _classB, _classC )\
	RED_DECL_IMPL_INTERFACE_3_R( _classA, _classB, _classC, RED_INTERFACE_RETURN_NULL );

#define RED_ADD_IMPL_INTERFACE_3( _classA, _classB, _classC )\
	RED_DECL_IMPL_INTERFACE_3_R( _classA, _classB, _classC, RED_INTERFACE_RETURN_BASE );

// 4
#define RED_DECL_IMPL_INTERFACE_4_R( _classA, _classB, _classC, _classD,  ret )\
protected:\
	virtual void* VQueryInterface( RED_INTERFACE_ID interfaceId )\
	{\
		RED_DECL_IMPL_SINGLE_INTERFACE_START( _classA )\
		RED_DECL_IMPL_SINGLE_INTERFACE( _classB )\
		RED_DECL_IMPL_SINGLE_INTERFACE( _classC )\
		RED_DECL_IMPL_SINGLE_INTERFACE( _classD )\
		ret\
	}\
private:

#define RED_DECL_IMPL_INTERFACE_4( _classA, _classB, _classC, _classD )\
	RED_DECL_IMPL_INTERFACE_4_R( _classA, _classB, _classC, _classD, RED_INTERFACE_RETURN_NULL );

#define RED_ADD_IMPL_INTERFACE_4( _classA, _classB, _classC, _classD )\
	RED_DECL_IMPL_INTERFACE_4_R( _classA, _classB, _classC, _classD, RED_INTERFACE_RETURN_BASE );

// 5
#define RED_DECL_IMPL_INTERFACE_5_R( _classA, _classB, _classC, _classD, _classE, ret )\
protected:\
	virtual void* VQueryInterface( RED_INTERFACE_ID interfaceId )\
	{\
		RED_DECL_IMPL_SINGLE_INTERFACE_START( _classA )\
		RED_DECL_IMPL_SINGLE_INTERFACE( _classB )\
		RED_DECL_IMPL_SINGLE_INTERFACE( _classC )\
		RED_DECL_IMPL_SINGLE_INTERFACE( _classD )\
		RED_DECL_IMPL_SINGLE_INTERFACE( _classE )\
		ret\
	}\
private:

#define RED_DECL_IMPL_INTERFACE_5( _classA, _classB, _classC, _classD, _classE )\
	RED_DECL_IMPL_INTERFACE_5_R( _classA, _classB, _classC, _classD, _classE, RED_INTERFACE_RETURN_NULL );

#define RED_ADD_IMPL_INTERFACE_5( _classA, _classB, _classC, _classD, _classE )\
	RED_DECL_IMPL_INTERFACE_5_R( _classA, _classB, _classC, _classD, _classE, RED_INTERFACE_RETURN_BASE );

//////////////////////////////////////////////////////////////////////////

// 0
#define RED_DECL_INTERFACE_SUPPORT()\
public:\
template < class _class > _class* QueryInterface()\
{\
	return static_cast< _class* >( VQueryInterface( _class::_GetId() ) );\
}\
protected:\
virtual void* VQueryInterface( RED_INTERFACE_ID interfaceId )\
{\
	return nullptr;\
}\
private:

// 1
#define RED_DECL_INTERFACE_SUPPORT_1( _classA )\
	RED_DECL_CLASS_INTERFACE()\
	RED_DECL_SINGLE_INTERFACE( _classA )\
	RED_DECL_IMPL_INTERFACE_1( _classA )\
private:

#define RED_ADD_INTERFACE_SUPPORT_1( _classA )\
	RED_ADD_CLASS_INTERFACE()\
	RED_ADD_SINGLE_INTERFACE( _classA )\
	RED_ADD_IMPL_INTERFACE_1( _classA )\
private:

// 2
#define RED_DECL_INTERFACE_SUPPORT_2( _classA, _classB )\
	RED_DECL_CLASS_INTERFACE()\
	RED_DECL_SINGLE_INTERFACE( _classA )\
	RED_DECL_SINGLE_INTERFACE( _classB )\
	RED_DECL_IMPL_INTERFACE_2( _classA, _classB )\
private:

#define RED_ADD_INTERFACE_SUPPORT_2( _classA, _classB )\
	RED_ADD_CLASS_INTERFACE()\
	RED_ADD_SINGLE_INTERFACE( _classA )\
	RED_ADD_SINGLE_INTERFACE( _classB )\
	RED_ADD_IMPL_INTERFACE_2( _classA, _classB )\
private:

// 3
#define RED_DECL_INTERFACE_SUPPORT_3( _classA, _classB, _classC )\
	RED_DECL_CLASS_INTERFACE()\
	RED_DECL_SINGLE_INTERFACE( _classA )\
	RED_DECL_SINGLE_INTERFACE( _classB )\
	RED_DECL_SINGLE_INTERFACE( _classC )\
	RED_DECL_IMPL_INTERFACE_3( _classA, _classB, _classC )\
private:

#define RED_ADD_INTERFACE_SUPPORT_3( _classA, _classB, _classC )\
	RED_ADD_CLASS_INTERFACE()\
	RED_ADD_SINGLE_INTERFACE( _classA )\
	RED_ADD_SINGLE_INTERFACE( _classB )\
	RED_ADD_SINGLE_INTERFACE( _classC )\
	RED_ADD_IMPL_INTERFACE_3( _classA, _classB, _classC )\
private:

// 4
#define RED_DECL_INTERFACE_SUPPORT_4( _classA, _classB, _classC, _classD )\
	RED_DECL_CLASS_INTERFACE()\
	RED_DECL_SINGLE_INTERFACE( _classA )\
	RED_DECL_SINGLE_INTERFACE( _classB )\
	RED_DECL_SINGLE_INTERFACE( _classC )\
	RED_DECL_SINGLE_INTERFACE( _classD )\
	RED_DECL_IMPL_INTERFACE_4( _classA, _classB, _classC, _classD )\
private:

#define RED_ADD_INTERFACE_SUPPORT_4( _classA, _classB, _classC, _classD )\
	RED_ADD_CLASS_INTERFACE()\
	RED_ADD_SINGLE_INTERFACE( _classA )\
	RED_ADD_SINGLE_INTERFACE( _classB )\
	RED_ADD_SINGLE_INTERFACE( _classC )\
	RED_ADD_SINGLE_INTERFACE( _classD )\
	RED_ADD_IMPL_INTERFACE_4( _classA, _classB, _classC, _classD )\
private:

// 4
#define RED_DECL_INTERFACE_SUPPORT_5( _classA, _classB, _classC, _classD, _classE )\
	RED_DECL_CLASS_INTERFACE()\
	RED_DECL_SINGLE_INTERFACE( _classA )\
	RED_DECL_SINGLE_INTERFACE( _classB )\
	RED_DECL_SINGLE_INTERFACE( _classC )\
	RED_DECL_SINGLE_INTERFACE( _classD )\
	RED_DECL_SINGLE_INTERFACE( _classE )\
	RED_DECL_IMPL_INTERFACE_5( _classA, _classB, _classC, _classD, _classE )\
private:

#define RED_ADD_INTERFACE_SUPPORT_5( _classA, _classB, _classC, _classD, _classE )\
	RED_ADD_CLASS_INTERFACE()\
	RED_ADD_SINGLE_INTERFACE( _classA )\
	RED_ADD_SINGLE_INTERFACE( _classB )\
	RED_ADD_SINGLE_INTERFACE( _classC )\
	RED_ADD_SINGLE_INTERFACE( _classD )\
	RED_ADD_SINGLE_INTERFACE( _classE )\
	RED_ADD_IMPL_INTERFACE_5( _classA, _classB, _classC, _classD, _classE )\
private:

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////