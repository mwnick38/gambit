//#
//# FILE: nliap.cc -- Normal form Liapunov module
//#
//# $Id$
//#

#define MAXIT 10
#define TOL (T)(.00000001)

#include <math.h>
#include "nliap.h"
#include "gfunct.h"
#include "rational.h"
#include "grandom.h"

//-------------------------------------------------------------------------
//                     NFLiapParams<T>: Member functions
//-------------------------------------------------------------------------

template <class T> NFLiapParams<T>::NFLiapParams(void)
{ }

//-------------------------------------------------------------------------
//                      NFLiapFunc<T>: Class definition
//-------------------------------------------------------------------------

template <class T>
class NFLiapFunc : public LiapFunc<T>, public gBC2FunctMin<T>   {
  private:
    int niters, nevals;
    const NormalForm<T> &N;
    gPVector<T> p, pp;
//    NFLiapParams<T> &params;

    T Value(const gVector<T> &x);
    int Deriv(const gVector<T> &p, gVector<T> &d);
    int Hess(const gVector<T> &p, gMatrix<T> &d);

    T LiapDerivValue(int i, int j, const gPVector<T> &p) const;

  public:
    NFLiapFunc(const NormalForm<T> &NF, const LiapParams<T> &P); 
    virtual ~NFLiapFunc();

    void Randomize(void);
    int Optimize(int &iter, T &value);
    void Output(gOutput &f) const;

    int NumIters(void) const;
    int NumEvals(void) const;

    const gPVector<T> &GetProfile(void) const;
};

template <class T>NFLiapFunc<T>
::NFLiapFunc(const NormalForm<T> &NF, const LiapParams<T> &P)
  : gBC2FunctMin<T>(NF.ProfileLength(),P.tolOpt,P.maxitsOpt,
		    P.tolBrent,P.maxitsBrent), N(NF), p(NF.Dimensionality()),
		    pp(NF.Dimensionality()),  niters(0), nevals(0)
{
  SetPlev(P.plev);
  N.Centroid(pp);
}

template <class T> NFLiapFunc<T>::~NFLiapFunc()
{ }


template <class T> const gPVector<T> &NFLiapFunc<T>::GetProfile(void) const
{
  return pp;
}

template <class T> void NFLiapFunc<T>::Output(gOutput &f) const
{
  f << "\np = " << pp;
 }

template <class T> int NFLiapFunc<T>::NumIters(void) const
{
  return niters;
}

template <class T> int NFLiapFunc<T>::NumEvals(void) const
{
  return nevals;
}

template <class T> int NFLiapFunc<T>::Optimize(int &iter, T &value)
{
//  if (params.plev >= 3 && params.outfile)
//    *params.outfile << "\np= " << pp;
//  T val = (T) 0;
//  int iter = 0;
  return   DFP(pp, iter, value);
//  if (params.plev > 0 && params.outfile)
//    *params.outfile << "\np= " << pp << " f = " << val;
//  return (val < (T) ((T) 1 / (T) 100000));
}

template <class T> void NFLiapFunc<T>::Randomize(void)
{
  T sum, tmp;

  for (int i = 1; i <= N.NumPlayers(); i++)  {
    sum = (T) 0;
    for (int j = 1; j < N.NumStrats(i); j++)  {
      do
	tmp = (T) Uniform();
      while (tmp + sum > (T) 1);
      pp(i,j) = tmp;
      sum += tmp;
    }
    pp(i,j) = (T) 1 - sum;
  }
}

template <class T> T NFLiapFunc<T>::Value(const gVector<T> &v)
{
  static const T BIG1 = (T) 100;
  static const T BIG2 = (T) 100;

  nevals++;

  p = v;
  gPVector<T> tmp(p), payoff(p);
  T x, result((T) 0), avg, sum;
  payoff = (T) 0;

  result = (T) 0;
  for(int i = 1; i <= N.NumPlayers(); i++) {
    tmp.CopyRow(i, payoff);
    avg = sum = (T) 0;
	// then for each strategy for that player set it to 1 and evaluate
    for (int j = 1; j <= N.NumStrats(i); j++) {
      tmp(i, j) = (T) 1;
      x = p(i, j);
      payoff(i, j) = N.Payoff(i, tmp);
      avg += x * payoff(i, j);
      sum += x;
      x= (x > ((T) 0) ? ((T) 0) : x);
      result += BIG1*x*x;         // add penalty for neg probabilities
      tmp(i,j) = (T) 0;
    }
    tmp.CopyRow(i, p);
    for(j=1;j<=N.NumStrats(i);j++) {
      x=payoff(i,j)-avg;
      x = (x > 0 ? x : 0);
      result += x*x;          // add penalty if not best response
    }
    x=sum - ((T) 1);
    result += BIG2*x*x ;          // add penalty for sum not equal to 1
  }
  return result;
}

template <class T> int NFLiapFunc<T>::Deriv(const gVector<T> &v, gVector<T> &d)
{
  p=v;
  int i1,j1,ii;
  T avg;
  
  for(i1=1,ii=1;i1<=N.NumPlayers();i1++) {
    avg=(T)0;
    for(j1=1;j1<=N.NumStrats(i1);j1++) {
      d[ii]=LiapDerivValue(i1,j1,p);
      avg+=d[ii];
      ii++;
    }
    avg/=(T)N.NumStrats(i1);
    ii-=N.NumStrats(i1);
    for(j1=1;j1<=N.NumStrats(i1);j1++) {
      d[ii]-=avg;
      ii++;
    }
  }
}

template <class T> T NFLiapFunc<T>::
LiapDerivValue(int i1, int j1, const gPVector<T> &p) const
{
  int i, j;
  T x, x1,psum;
  
  x=(T)0;
  for(i=1;i<=N.NumPlayers();i++) {
    psum=(T)0.0;
    for(j=1;j<=N.NumStrats(i);j++) {
      psum+=p(i,j);
      x1=N.Payoff(i,i,j,p)-N.Payoff(i,p);
      if(i1==i) {
	if(x1>0)x-=x1*(N.Payoff(i,i1,j1,p));
      }
      else {
	if(x1>0)x+=x1*(N.Payoff(i,i,j,i1,j1,p)-N.Payoff(i,i1,j1,p));
      }
    }
    if(i==i1)x+=psum-(T)1.0;
  }
  if(p(i1,j1)<(T)0.0)x+=p(i1,j1);
  return (T)2.0*x;
};

template <class T> int NFLiapFunc<T>::Hess(const gVector<T> &, gMatrix<T> &)
{
  return 1;
}


//------------------------------------------------------------------------
//                    NFLiapModule<T>: Member functions
//------------------------------------------------------------------------

template <class T> 
NFLiapModule<T>::NFLiapModule(const NormalForm<T> &N, NFLiapParams<T> &p)
  : LiapModule<T>(p), nf(N)
{ }

template <class T> NFLiapModule<T>::~NFLiapModule()
{ }

template <class T>
const gList<gPVector<T> > &NFLiapModule<T>::GetSolutions(void) const
{
  return solutions;
}

template <class T> LiapFunc<T> *NFLiapModule<T>::CreateFunc(void)
{
//  return new NFLiapFunc<T>(nf, (NFLiapParams<T> &) params);
  return new NFLiapFunc<T>(nf, params);
}

template <class T>
void NFLiapModule<T>::AddSolution(const LiapFunc<T> *const F)
{
  solutions.Append(((NFLiapFunc<T> *) F)->GetProfile());
}


#ifdef __GNUG__
#define TEMPLATE template
#elif defined __BORLANDC__
#define TEMPLATE
#pragma option -Jgd
#endif   // __GNUG__, __BORLANDC__

TEMPLATE class NFLiapParams<double>;
TEMPLATE class NFLiapParams<gRational>;
TEMPLATE class NFLiapModule<double>;
TEMPLATE class NFLiapModule<gRational>;
TEMPLATE class NFLiapFunc<double>;
TEMPLATE class NFLiapFunc<gRational>;


