
#include "build.h"
#include "Bezier_Polynomials.h"

namespace QuadraticBezierNamespace
{

		Polynomial::Polynomial(int Degree)
		{
			m_degree=Degree; 
			m_constants=new float[Count()]; 
			for(int i=0;i<Count();i++)
			{m_constants[i]=0.0f;}
		}

		Polynomial::~Polynomial()
		{
			delete [] m_constants;
		}

		void Polynomial::FindRoots(float* roots, int & numroots)
		{
			if(m_degree==0){FindRoots0(roots,numroots);return;}
			if(m_degree==1){FindRoots1(roots,numroots);return;}
			if(m_degree==2){FindRoots2(roots,numroots);return;}
			if(m_degree==3){FindRoots3(roots,numroots);return;}
			if(m_degree==4){FindRoots4(roots,numroots);return;}
		}
		Polynomial* Polynomial::Derivative()
		{
			Polynomial* derivative=new Polynomial(m_degree-1);
			for(int i=0;i<derivative->Count();i++)
			{
				derivative->m_constants[i]=m_constants[i]*(m_degree-i);
			}
			return derivative;
		}
		void Polynomial::FindRoots0(float* roots, int & numroots)
		{
			numroots=0;
		}
		void Polynomial::FindRoots1(float* roots, int & numroots)
		{
			float a=m_constants[0];
			float b=m_constants[1];
			numroots=1;
			roots[0] = -b/a;
		}
		void Polynomial::FindRoots2(float* roots, int & numroots)
		{
			float a=m_constants[0];
			float b=m_constants[1];
			float c=m_constants[2];
			float Delta = (b*b) - 4 * a * c;
			if(Delta<0){numroots=0;return;}
			if(Delta==0)
			{
				numroots=1;
				roots[0]=(-b+sqrt(Delta))/(2*a);
				return;
			}
			numroots=2;
			roots[0]=(-b+sqrt(Delta))/(2*a);
			roots[1]=(-b-sqrt(Delta))/(2*a);
		}
		void Polynomial::FindRoots3(float* roots, int & numroots)
		{
			float a=m_constants[0];
			float b=m_constants[1];
			float c=m_constants[2];
			float d=m_constants[3];
			float B=b/a;
			float C=c/a;
			float D=d/a;
			float m = 4.5f*((B*C)-(3*D))-(B*B*B);
			float n = (B*B)-3*C;
			float Delta = (m*m)-(n*n*n);
			float r=m+sqrt(Delta);
			float s=m-sqrt(Delta);
			r=Sign(r)*pow( abs(r), 1.0f/3.0f );
			s=Sign(s)*pow( abs(s), 1.0f/3.0f );
			if(Delta>0)
			{
				numroots=1;
				roots[0]=(1.f/3.f)*(-B+r+s);
				return;
			}
			if(Delta==0)
			{
				if(m==0)
				{
					numroots=1;
					roots[0]=(1.0f/3.0f)*(-B+2*Sign(m)*pow(abs(m),(1.0f/3.0f)));
					return;
				}
				numroots=2;
				roots[0]=(1.0f/3.0f)*(-B+2*Sign(m)*pow(abs(m),(1.0f/3.0f)));
				roots[1]=(1.0f/3.0f)*(-B-Sign(m)*pow(abs(m),(1.0f/3.0f)));
				return;
			}
			float fi = acos( m/(n*sqrt(n)) );
			numroots=3;
			roots[0]=(1.0f/3.0f)*( -B + 2*sqrt(n)*cos(  (fi+0*2*M_PI)/3.0f  )  );
			roots[1]=(1.0f/3.0f)*( -B + 2*sqrt(n)*cos(  (fi+1*2*M_PI)/3.0f  )  );
			roots[2]=(1.0f/3.0f)*( -B + 2*sqrt(n)*cos(  (fi+2*2*M_PI)/3.0f  )  );
		}
		void Polynomial::FindRoots4(float* roots, int & numroots)
		{
			float k=m_constants[0];
			float a=m_constants[1];
			float b=m_constants[2];
			float c=m_constants[3];
			float d=m_constants[4];
			float A = a/k;
			float B=-b/k;
			float C=c/k;
			float D=d/k;
			float m = 4.5f*(A*B*C + 3*A*A*D + 3*C*C + 8*B*D)-(B*B*B);
			float n = 3*(4*D-A*C) + (B*B);
			float Delta = (m*m)-(n*n*n);
			float r=m+Sign(Delta)*sqrt(fabs(Delta));
			float s=m-Sign(Delta)*sqrt(fabs(Delta));
			r=Sign(r)*pow( abs(r), 1.0f/3.0f );
			s=Sign(s)*pow( abs(s), 1.0f/3.0f );
			if(Delta>0)
			{
				float x = (1.f/3.f)*(-B+r+s);
				float pierw =	((A*A)/4.0f) + B + x;
				float t1 = (-A/4.0f)+(1.0f/2.0f)*sqrt( pierw );
				float t2 = (-A/4.0f)-(1.0f/2.0f)*sqrt( pierw );
				GetRoots4(x, m, n, A, B, C, pierw,t1,t2, roots, numroots);
				return;
			}
			if(Delta==0)
			{
				if(m==0)
				{
					float x = (1.0f/3.0f)*(-B+2*Sign(m)*pow(abs(m),(1.0f/3.0f)));
					float pierw =	((A*A)/4.0f) + B + x;
					float t1 = (-A/4.0f)+(1.0f/2.0f)*sqrt( pierw );
					float t2 = (-A/4.0f)-(1.0f/2.0f)*sqrt( pierw );
					GetRoots4(x, m, n, A, B, C, pierw,t1,t2, roots, numroots);
					return;
				}
				float x = (1.0f/3.0f)*(-B+2*Sign(m)*pow(abs(m),(1.0f/3.0f)));
				float x2 =(1.0f/3.0f)*(-B-Sign(m)*pow(abs(m),(1.0f/3.0f)));
				float pierw =	((A*A)/4.0f) + B + x;
				if(pierw==0){	pierw = ((A*A)/4.0f) + B + x2;	}
				float t1 = (-A/4.0f)+(1.0f/2.0f)*sqrt( pierw );
				float t2 = (-A/4.0f)-(1.0f/2.0f)*sqrt( pierw );
				GetRoots4(x, m, n, A, B, C, pierw,t1,t2, roots, numroots);
				return;
			}
			float fi = acos( m/(n*sqrt(n)) );
			float x1 = (1.0f/3.0f)*( -B + 2*sqrt(n)*cos(  (fi+0*2*M_PI)/3.0f  )  );
			float x2=  (1.0f/3.0f)*( -B + 2*sqrt(n)*cos(  (fi+1*2*M_PI)/3.0f  )  );
			float x3=  (1.0f/3.0f)*( -B + 2*sqrt(n)*cos(  (fi+2*2*M_PI)/3.0f  )  );
			float x=x1;
			float pierw =	((A*A)/4.0f) + B + x1; x=x1;
			if(pierw==0){	pierw = ((A*A)/4.0f) + B + x2; x=x2;	}
			if(pierw==0){	pierw = ((A*A)/4.0f) + B + x3; x=x3;	}
			float t1 = (-A/4.0f)+(1.0f/2.0f)*sqrt( pierw );
			float t2 = (-A/4.0f)-(1.0f/2.0f)*sqrt( pierw );
			GetRoots4(x, m, n, A, B, C, pierw,t1,t2, roots, numroots);
			return;
		}
		void Polynomial::GetRoots4(float x, float m, float n, float A, float B, float C, float pierw, float t1, float t2, float* roots, int & numroots )
		{
			if(m==0.0f && n==0.0f && pierw==0.0f)
			{
				numroots=1;
				roots[0]=-A/4.0f;
				return;
			}
			float sq1 = (t1*t1)-( (2*t1*x + C)/(4*t1+A) );
			float sq2 = (t2*t2)-( (2*t2*x + C)/(4*t2+A) );
			if(sq1==0)
			{
				numroots=3;
				roots[0]=t1;
				roots[1]=t2+sqrt(sq2);
				roots[2]=t2-sqrt(sq2);
				return;
			}
			if(sq2==0)
			{
				numroots=3;
				roots[0]=t2;
				roots[1]=t1+sqrt(sq1);
				roots[2]=t1-sqrt(sq1);
				return;
			}
			if(sq1==0 && sq2==0)
			{
				numroots=2;
				roots[0]=t1;
				roots[1]=t2;
				return;
			}
			numroots=0;
			if(sq1>0)
			{
				numroots+=2;
				roots[0]=t1+sqrt(sq1);
				roots[1]=t1-sqrt(sq1);
			}
			if(sq2>0)
			{
				numroots+=2;
				roots[numroots-2]=t2+sqrt(sq2);
				roots[numroots-1]=t2-sqrt(sq2);
			}
		}
}