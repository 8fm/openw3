/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

namespace BoolExpression
{
	class IExp
	{
	public:
		virtual ~IExp(){}
		virtual Bool Evaluate() = 0;
		virtual void Destroy() = 0;
	};

	class COrExp : public IExp
	{
	public:
		COrExp( IExp* left, IExp* right ) : m_left( left ), m_right( right ) {}
		virtual Bool Evaluate() { return m_left->Evaluate() || m_right->Evaluate(); }
		virtual void Destroy()
		{
			m_left->Destroy();
			m_right->Destroy();
			delete this;
		}

	private:
		IExp* m_left;
		IExp* m_right;
	};

	class CAndExp : public IExp
	{
	public:
		CAndExp( IExp* left, IExp* right ) : m_left( left ), m_right( right ) {}
		virtual Bool Evaluate() { return m_left->Evaluate() && m_right->Evaluate(); }
		virtual void Destroy()
		{
			m_left->Destroy();
			m_right->Destroy();
			delete this;
		}

	private:
		IExp* m_left;
		IExp* m_right;
	};

	class CNotExp : public IExp
	{
	public:
		CNotExp( IExp* exp ) : m_exp( exp ) {}
		virtual Bool Evaluate() { return !m_exp->Evaluate(); }
		virtual void Destroy()
		{
			m_exp->Destroy();
			delete this;
		}

	private:
		IExp* m_exp;
	};
}
