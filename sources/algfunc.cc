//
// FILE: algfunc.cc -- Solution algorithm functions for GCL
//
// $Id$
//

#include "gsm.h"
#include "portion.h"
#include "gsmfunc.h"

#include "rational.h"

#include "gwatch.h"
#include "mixedsol.h"
#include "behavsol.h"
#include "nfg.h"
#include "efg.h"


extern Portion *ArrayToList(const gArray<int> &A);
extern Portion *ArrayToList(const gArray<double> &A);
extern Portion *ArrayToList(const gArray<gRational> &A);
extern gVector<double>* ListToVector_Float(ListPortion* list);
extern gVector<gRational>* ListToVector_Rational(ListPortion* list);
extern gMatrix<double>* ListToMatrix_Float(ListPortion* list);
extern gMatrix<gRational>* ListToMatrix_Rational(ListPortion* list);


//
// Useful utilities for creation of lists of profiles
//
template <class T> class Mixed_ListPortion : public ListValPortion   {
  public:
    Mixed_ListPortion(const gList<MixedSolution<T> > &);
    virtual ~Mixed_ListPortion()   { }
};

Mixed_ListPortion<double>::Mixed_ListPortion(const gList<MixedSolution<double> > &list)
{
  _DataType = porMIXED_FLOAT;
  for (int i = 1; i <= list.Length(); i++)
    Append(new MixedValPortion(new MixedSolution<double>(list[i])));
}

Mixed_ListPortion<gRational>::Mixed_ListPortion(const gList<MixedSolution<gRational> > &list)
{
  _DataType = porMIXED_RATIONAL;
  for (int i = 1; i <= list.Length(); i++)
    Append(new MixedValPortion(new MixedSolution<gRational>(list[i])));
}


template <class T> class Behav_ListPortion : public ListValPortion   {
  public:
    Behav_ListPortion(const gList<BehavSolution<T> > &);
    virtual ~Behav_ListPortion()   { }
};

Behav_ListPortion<double>::Behav_ListPortion(
			   const gList<BehavSolution<double> > &list)
{
  _DataType = porBEHAV_FLOAT;
  for (int i = 1; i <= list.Length(); i++)
    Append(new BehavValPortion(new BehavSolution<double>(list[i])));
}

Behav_ListPortion<gRational>::Behav_ListPortion(
			      const gList<BehavSolution<gRational> > &list)
{
  _DataType = porBEHAV_RATIONAL;
  for (int i = 1; i <= list.Length(); i++)
    Append(new BehavValPortion(new BehavSolution<gRational>(list[i])));
}


//-------------
// AgentForm
//-------------

Portion *GSM_AgentForm_Float(Portion **param)
{
  Efg<double> &E = * (Efg<double>*) ((EfgPortion*) param[0])->Value();
  gWatch watch;

  Nfg<double> *N = MakeAfg(E);
  
  ((FloatPortion *) param[1])->Value() = watch.Elapsed();
  
  if (N)
    return new NfgValPortion(N);
  else
    return new ErrorPortion("Conversion to agent form failed");
}

Portion *GSM_AgentForm_Rational(Portion **param)
{
  Efg<gRational> &E = * (Efg<gRational>*) ((EfgPortion*) param[0])->Value();
  gWatch watch;

  Nfg<gRational> *N = MakeAfg(E);
  
  ((FloatPortion *) param[1])->Value() = watch.Elapsed();

  if (N)
    return new NfgValPortion(N);
  else
    return new ErrorPortion("Conversion to agent form failed");
}

//------------
// Behav
//------------

Portion *GSM_Behav_Float(Portion **param)
{
  MixedSolution<double> &mp = * (MixedSolution<double>*) ((MixedPortion*) param[0])->Value();
  Efg<double> &E = * (Efg<double>*) ((EfgPortion*) param[1])->Value();
  Nfg<double> &N = *mp.BelongsTo(); 

  BehavSolution<double>* bp;

  if(AssociatedNfg(&E) != &N)  
    return new ErrorPortion("Normal and extensive form games not associated");
  else
  {
    bp = new BehavSolution<double>(E);
    MixedToBehav(N, mp, E, *bp);
  }

  Portion* por = new BehavValPortion(bp);
  por->SetGame(param[1]->Game(), param[1]->GameIsEfg());
  return por;
}

Portion *GSM_Behav_Rational(Portion **param)
{
  MixedSolution<gRational> &mp = 
    * (MixedSolution<gRational>*) ((MixedPortion*) param[0])->Value();
  Efg<gRational> &E = * (Efg<gRational>*) ((EfgPortion*) param[1])->Value();
  Nfg<gRational> &N = *mp.BelongsTo(); 

  BehavSolution<gRational>* bp;

  if(AssociatedNfg(&E) != &N)  
    return new ErrorPortion("Normal and extensive form games not associated");
  else
  {
    bp = new BehavSolution<gRational>(E);
    MixedToBehav(N, mp, E, *bp);
  }

  Portion* por = new BehavValPortion(bp);
  por->SetGame(param[1]->Game(), param[1]->GameIsEfg());
  return por;
}

//------------------
// EnumMixedSolve
//------------------

#include "enum.h"

Portion *GSM_EnumMixed_Support(Portion **param)
{
  NFSupport* S = ((NfSupportPortion*) param[0])->Value();
  BaseNfg* N = (BaseNfg*) &(S->BelongsTo());
  Portion* por = 0;

  EnumParams EP;

  EP.stopAfter = ((IntPortion *) param[1])->Value();

  EP.tracefile = &((OutputPortion *) param[4])->Value();
  EP.trace = ((IntPortion *) param[5])->Value();

  switch(N->Type())
  {
  case DOUBLE:
    {
      EnumModule<double> EM(* (Nfg<double>*) N, EP, *S);
      EM.Enum();
      ((IntPortion *) param[2])->Value() = EM.NumPivots();
      ((FloatPortion *) param[3])->Value() = EM.Time();
      por = new Mixed_ListPortion<double>(EM.GetSolutions());
    }
    break;
  case RATIONAL:
    {
      EnumModule<gRational> EM(* (Nfg<gRational>*) N, EP, *S);
      EM.Enum();
      ((IntPortion *) param[2])->Value() = EM.NumPivots();
      ((FloatPortion *) param[3])->Value() = EM.Time();
      por = new Mixed_ListPortion<gRational>(EM.GetSolutions());
    }
    break;
  default:
    assert(0);
  }

  assert(por != 0);
  por->SetGame(param[0]->Game(), param[0]->GameIsEfg());
  return por;
}

#include "enumsub.h"


Portion *GSM_EnumMixed_EfgFloat(Portion **param)
{
  Efg<double> &E = * (Efg<double>*) ((EfgPortion*) param[0])->Value();

  if (!((BoolPortion *) param[1])->Value())
    return new ErrorPortion("algorithm not implemented for extensive forms");

  EnumParams EP;
  EP.stopAfter = ((IntPortion *) param[2])->Value();

  EP.tracefile = &((OutputPortion *) param[5])->Value();
  EP.trace = ((IntPortion *) param[6])->Value();
  
  EnumBySubgame<double> EM(E, EP);
  
  EM.Solve();
  gList<BehavSolution<double> > solns(EM.GetSolutions());

  ((IntPortion *) param[3])->Value() = EM.NumPivots();
  ((FloatPortion *) param[4])->Value() = EM.Time();

  Portion* por = new Behav_ListPortion<double>(solns);
  por->SetGame(param[0]->Game(), param[0]->GameIsEfg());
  return por;
}

Portion *GSM_EnumMixed_EfgRational(Portion **param)
{
  Efg<gRational> &E = * (Efg<gRational>*) ((EfgPortion*) param[0])->Value();

  if (!((BoolPortion *) param[1])->Value())
    return new ErrorPortion("algorithm not implemented for extensive forms");

  EnumParams EP;
  EP.stopAfter = ((IntPortion *) param[2])->Value();
  
  EP.tracefile = &((OutputPortion *) param[5])->Value();
  EP.trace = ((IntPortion *) param[6])->Value();

  EnumBySubgame<gRational> EM(E, EP);
  
  EM.Solve();

  gList<BehavSolution<gRational> > solns(EM.GetSolutions());

  ((IntPortion *) param[3])->Value() = EM.NumPivots();
  ((FloatPortion *) param[4])->Value() = EM.Time();

  Portion* por = new Behav_ListPortion<gRational>(solns);
  por->SetGame(param[0]->Game(), param[0]->GameIsEfg());
  return por;
}

//-----------------
// EnumPureSolve
//-----------------

#include "nfgpure.h"

Portion *GSM_EnumPure_Support(Portion **param)
{
  NFSupport* S = ((NfSupportPortion*) param[0])->Value();
  BaseNfg* N = (BaseNfg*) &(S->BelongsTo());
  Portion* por;

  gWatch watch;

  switch(N->Type())
  {
  case DOUBLE:
    {
      gList<MixedSolution<double> > solns;
      FindPureNash(* (Nfg<double>*) N, solns);
      por = new Mixed_ListPortion<double>(solns);
    }
    break;
  case RATIONAL:
    {
      gList<MixedSolution<gRational> > solns;
      FindPureNash(* (Nfg<gRational>*) N, solns);
      por = new Mixed_ListPortion<gRational>(solns);
    }
    break;
  default:
    assert(0);
  }

  ((FloatPortion *) param[2])->Value() = watch.Elapsed();
  
  por->SetGame(param[0]->Game(), param[0]->GameIsEfg());
  return por;
}

#include "efgpure.h"
#include "psnesub.h"

Portion *GSM_EnumPure_EfgFloat(Portion **param)
{
  Efg<double> &E = * (Efg<double>*) ((EfgPortion*) param[0])->Value();
  gList<BehavSolution<double> > solns;

  if (((BoolPortion *) param[1])->Value())   {
    PureNashBySubgame<double> M(E);
    M.Solve();
    solns = M.GetSolutions();
    ((FloatPortion *) param[3])->Value() = M.Time();
  }
  else  {
    EfgPSNEBySubgame<double> M(E);
    M.Solve();
    solns = M.GetSolutions();
    ((FloatPortion *) param[3])->Value() = M.Time();
  }

  Portion* por = new Behav_ListPortion<double>(solns);
  por->SetGame(param[0]->Game(), param[0]->GameIsEfg());
  return por;
}

Portion *GSM_EnumPure_EfgRational(Portion **param)
{
  Efg<gRational> &E = * (Efg<gRational>*) ((EfgPortion*) param[0])->Value();
  gList<BehavSolution<gRational> > solns;
  
  if (((BoolPortion *) param[1])->Value())  {
    PureNashBySubgame<gRational> M(E);
    M.Solve();
    solns = M.GetSolutions();
    ((FloatPortion *) param[3])->Value() = M.Time();
  }
  else  {
    EfgPSNEBySubgame<gRational> M(E);
    M.Solve();
    solns = M.GetSolutions();
    ((FloatPortion *) param[3])->Value() = M.Time();
  } 
 
  Portion* por = new Behav_ListPortion<gRational>(solns);
  por->SetGame(param[0]->Game(), param[0]->GameIsEfg());
  return por;
}

//------------------
// GobitGridSolve
//------------------

#include "grid.h"

Portion *GSM_GobitGrid_Support(Portion **param)
{
  NFSupport& S = * ((NfSupportPortion*) param[0])->Value();
  BaseNfg* N = (BaseNfg*) &(S.BelongsTo());
  Portion* por = 0;

  GridParams<double> GP;

  if(((TextPortion*) param[1])->Value() != "")
    GP.pxifile = new gFileOutput(((TextPortion*) param[1])->Value());
  else
    GP.pxifile = &gnull;
  GP.minLam = ((FloatPortion *) param[2])->Value();
  GP.maxLam = ((FloatPortion *) param[3])->Value();
  GP.delLam = ((FloatPortion *) param[4])->Value();
  GP.powLam = ((IntPortion *) param[5])->Value();
  GP.delp = ((FloatPortion *) param[6])->Value();
  GP.tol = ((FloatPortion *) param[7])->Value();

  switch(N->Type())
  {
  case DOUBLE:
    {
      GridSolveModule<double> GM(* (Nfg<double>*) N, GP, S);
      GM.GridSolve();
      // ((IntPortion *) param[8])->Value() = GM.NumEvals();
      // ((FloatPortion *) param[9])->Value() = GM.Time();
      gList<MixedSolution<double> > solns;
      por = new Mixed_ListPortion<double>(solns);
    }
    break;
  case RATIONAL:
    return new ErrorPortion("The rational version of GobitGridSolve is not implemented");
    break;
  default:
    assert(0);
  }

  assert(por != 0);
  por->SetGame(param[0]->Game(), param[0]->GameIsEfg());
  return por;
}

//---------------
// GobitSolve
//---------------

#include "ngobit.h"
#include "egobit.h"

Portion *GSM_Gobit_Start(Portion **param)
{
  if (param[0]->Spec().Type == porMIXED_FLOAT)  {
    MixedSolution<double> &start = 
      * (MixedSolution<double> *) ((MixedPortion *) param[0])->Value();
    Nfg<double> &N = *start.BelongsTo();
  
    NFGobitParams NP;
    if (((TextPortion *) param[1])->Value() != "")
      NP.pxifile = new gFileOutput(((TextPortion *) param[1])->Value());
    else
      NP.pxifile = &gnull;
    NP.minLam = ((FloatPortion *) param[2])->Value();
    NP.maxLam = ((FloatPortion *) param[3])->Value();
    NP.delLam = ((FloatPortion *) param[4])->Value();
    NP.powLam = ((IntPortion *) param[5])->Value();
    NP.fullGraph = ((BoolPortion *) param[6])->Value();

    NP.maxitsN = ((IntPortion *) param[7])->Value();
    NP.tolN = ((FloatPortion *) param[8])->Value();
    NP.maxits1 = ((IntPortion *) param[9])->Value();
    NP.tol1 = ((FloatPortion *) param[10])->Value();

    gWatch watch;
    gList<MixedSolution<double> > solutions;
    Gobit(N, NP, start, solutions, 
	  ((IntPortion *) param[12])->Value(),
	  ((IntPortion *) param[13])->Value());

    ((FloatPortion *) param[11])->Value() = watch.Elapsed();

    Portion *por = new Mixed_ListPortion<double>(solutions);
    por->SetGame(param[0]->Game(), param[0]->GameIsEfg());
    return por;
  }
  else  {     // BEHAV_FLOAT  
    BehavSolution<double>& start = 
      * (BehavSolution<double> *) ((BehavPortion *) param[0])->Value();
    Efg<double> &E = *(start.BelongsTo());
  
    EFGobitParams EP;
    if(((TextPortion*) param[1])->Value() != "")
      EP.pxifile = new gFileOutput(((TextPortion*) param[1])->Value());
    else
      EP.pxifile = &gnull;
    EP.minLam = ((FloatPortion *) param[2])->Value();
    EP.maxLam = ((FloatPortion *) param[3])->Value();
    EP.delLam = ((FloatPortion *) param[4])->Value();
    EP.powLam = ((IntPortion *) param[5])->Value();
    EP.fullGraph = ((BoolPortion *) param[6])->Value();
    
    EP.maxitsN = ((IntPortion *) param[7])->Value();
    EP.tolN = ((FloatPortion *) param[8])->Value();
    EP.maxits1 = ((IntPortion *) param[9])->Value();
    EP.tol1 = ((FloatPortion *) param[10])->Value();
  
    EP.tracefile = &((OutputPortion *) param[14])->Value();
    EP.trace = ((IntPortion *) param[15])->Value();
    
    gWatch watch;
    
    gList<BehavSolution<double> > solutions;
    Gobit(E, EP, start, solutions,
	  ((IntPortion *) param[12])->Value(),
	  ((IntPortion *) param[13])->Value());
    
    ((FloatPortion *) param[11])->Value() = watch.Elapsed();
    
    Portion * por = new Behav_ListPortion<double>(solutions);
    por->SetGame(param[0]->Game(), param[0]->GameIsEfg());
    return por;
  }
}


//------------
// LcpSolve
//------------

#include "lemke.h"

Portion *GSM_Lcp_NFSupport(Portion **param)
{
  NFSupport& S = * ((NfSupportPortion*) param[0])->Value();
  BaseNfg* N = (BaseNfg*) &(S.BelongsTo());
  Portion* por = 0;

  LemkeParams LP;
  LP.stopAfter = ((IntPortion *) param[1])->Value();

  LP.tracefile = &((OutputPortion *) param[4])->Value();
  LP.trace = ((IntPortion *) param[5])->Value();

  switch(N->Type())
  {
  case DOUBLE:
    {
      LemkeModule<double> LS(* (Nfg<double>*) N, LP, S);
      LS.Lemke();
      ((IntPortion *) param[2])->Value() = LS.NumPivots();
      ((FloatPortion *) param[3])->Value() = LS.Time();
      por = new Mixed_ListPortion<double>(LS.GetSolutions());
    }
    break;
  case RATIONAL:
    {
      LemkeModule<gRational> LS(* (Nfg<gRational>*) N, LP, S);
      LS.Lemke();
      ((IntPortion *) param[2])->Value() = LS.NumPivots();
      ((FloatPortion *) param[3])->Value() = LS.Time();
      por = new Mixed_ListPortion<gRational>(LS.GetSolutions());
    }
    break;
  default:
    assert(0);
  }
  assert(por != 0);
  por->SetGame(param[0]->Game(), param[0]->GameIsEfg());
  return por;
}


#include "lemketab.h"

Portion* GSM_Lcp_ListFloat(Portion** param)
{
  gMatrix<double>* a = ListToMatrix_Float((ListPortion*) param[0]);
  gVector<double>* b = ListToVector_Float((ListPortion*) param[1]);
  
  LTableau<double>* tab = new LTableau<double>(*a, *b);
  tab->LemkePath(0);
  gVector<double> vector;
  tab->BasisVector(vector);
  Portion* result = ArrayToList(vector);
  delete tab;
  delete a;
  delete b;
  
  return result;
}

Portion* GSM_Lcp_ListRational(Portion** param)
{
  gMatrix<gRational>* a = ListToMatrix_Rational((ListPortion*) param[0]);
  gVector<gRational>* b = ListToVector_Rational((ListPortion*) param[1]);
  
  LTableau<gRational>* tab = new LTableau<gRational>(*a, *b);
  tab->LemkePath(0);
  gVector<gRational> vector;
  tab->BasisVector(vector);
  Portion* result = ArrayToList(vector);
  delete tab;
  delete a;
  delete b;
  
  return result;
}



#include "seqform.h"
#include "lemkesub.h"

Portion *GSM_Lcp_EfSupport(Portion **param)
{
  EFSupport& S = *((EfSupportPortion*) param[0])->Value();
  Portion* por;

  SeqFormParams SP;
  SP.stopAfter = ((IntPortion *) param[2])->Value();  
  SP.tracefile = &((OutputPortion *) param[5])->Value();
  SP.trace = ((IntPortion *) param[6])->Value();

  switch(S.BelongsTo().Type())
  {
  case DOUBLE:
    {
      // getting E from S.BelongsTo() doesn't work for some reason...
      Efg<double>* E = (Efg<double>*) &S.BelongsTo();
      gList<BehavSolution<double> > solns;
      
      SeqFormModule<double> SM(*E, SP, S);
      SM.Lemke();
      
      solns = SM.GetSolutions();
      por = new Behav_ListPortion<double>(solns);
      
      ((IntPortion *) param[3])->Value() = SM.NumPivots();
      ((FloatPortion *) param[4])->Value() = SM.Time();
    }
    break;
  case RATIONAL:
    {
      // getting E from S.BelongsTo() doesn't work for some reason...
      Efg<gRational>* E = (Efg<gRational>*) &S.BelongsTo();
      gList<BehavSolution<gRational> > solns;
      
      SeqFormModule<gRational> SM(*E, SP, S);
      SM.Lemke();
      
      solns = SM.GetSolutions();
      por = new Behav_ListPortion<gRational>(solns);
      
      ((IntPortion *) param[3])->Value() = SM.NumPivots();
      ((FloatPortion *) param[4])->Value() = SM.Time();
    }
    break;
  default:
    assert(0);
  }

  por->SetGame(param[0]->Game(), param[0]->GameIsEfg());
  return por;
}


//-------------
// LiapSolve
//-------------

#include "liapsub.h"
#include "eliap.h"

Portion *GSM_Liap_BehavFloat(Portion **param)
{
  BehavSolution<double> &start = 
    * (BehavSolution<double> *) ((BehavPortion *) param[0])->Value();
  Efg<double> &E = *(start.BelongsTo());
  
  if (((BoolPortion *) param[1])->Value())   {
    return new ErrorPortion("This function not implemented yet");
  }
  else  {
    EFLiapParams LP;

    LP.stopAfter = ((IntPortion *) param[2])->Value();
    LP.nTries = ((IntPortion *) param[3])->Value();

    LP.maxitsN = ((IntPortion *) param[4])->Value();
    LP.tolN = ((FloatPortion *) param[5])->Value();
    LP.maxits1 = ((IntPortion *) param[6])->Value();
    LP.tol1 = ((FloatPortion *) param[7])->Value();
    
    LP.tracefile = &((OutputPortion *) param[10])->Value();
    LP.trace = ((IntPortion *) param[11])->Value();

    long niters;
    gWatch watch;
    gList<BehavSolution<double> > solutions;
    Liap(E, LP, start, solutions,
	 ((IntPortion *) param[8])->Value(),
	 niters);

    ((FloatPortion *) param[7])->Value() = watch.Elapsed();

    Portion *por = new Behav_ListPortion<double>(solutions);
    por->SetGame(param[0]->Game(), param[0]->GameIsEfg());
    return por;
  }
}

#include "nliap.h"

Portion *GSM_Liap_MixedFloat(Portion **param)
{
  MixedSolution<double> &start = 
    * (MixedSolution<double> *) ((MixedPortion *) param[0])->Value();
  Nfg<double> &N = *start.BelongsTo();

  NFLiapParams params;
  
  params.stopAfter = ((IntPortion *) param[1])->Value();
  params.nTries = ((IntPortion *) param[2])->Value();

  params.maxitsN = ((IntPortion *) param[3])->Value();
  params.tolN = ((FloatPortion *) param[4])->Value();
  params.maxits1 = ((IntPortion *) param[5])->Value();
  params.tol1 = ((FloatPortion *) param[6])->Value();
 
  long niters;
  gWatch watch;
  gList<MixedSolution<double> > solutions;
  Liap(N, params, start, solutions,
       ((IntPortion *) param[8])->Value(),
       niters);

  ((FloatPortion *) param[7])->Value() = watch.Elapsed();

  Portion *por = new Mixed_ListPortion<double>(solutions);
  por->SetGame(param[0]->Game(), param[0]->GameIsEfg());
  return por;
}

//------------
// LpSolve
//------------

#include "nfgcsum.h"

Portion *GSM_Lp_Support(Portion **param)
{
  NFSupport& S = * ((NfSupportPortion*) param[0])->Value();
  BaseNfg* N = (BaseNfg*) &(S.BelongsTo());
  Portion* por = 0;

  if (N->NumPlayers() > 2 || !N->IsConstSum())
    return new ErrorPortion("Only valid for two-person zero-sum games");

  ZSumParams ZP;

  ZP.tracefile = &((OutputPortion *) param[3])->Value();
  ZP.trace = ((IntPortion *) param[4])->Value();

  switch(N->Type())  {
  case DOUBLE:
    {
      ZSumModule<double> ZM(* (Nfg<double>*) N, ZP, S);
      ZM.ZSum();
      ((IntPortion *) param[1])->Value() = ZM.NumPivots();
      ((FloatPortion *) param[2])->Value() = ZM.Time();
      gList<MixedSolution<double> > solns;
      ZM.GetSolutions(solns);  por = new Mixed_ListPortion<double>(solns);
    }
    break;
  case RATIONAL:
    {
      ZSumModule<gRational> ZM(*(Nfg<gRational>*) N, ZP, S);
      ZM.ZSum();
      ((IntPortion *) param[1])->Value() = ZM.NumPivots();
      ((FloatPortion *) param[2])->Value() = ZM.Time();
      gList<MixedSolution<gRational> > solns;
      ZM.GetSolutions(solns);  por = new Mixed_ListPortion<gRational>(solns);
    }
    break;
  default:
    assert(0);
  }

  assert(por != 0);
  por->SetGame(param[0]->Game(), param[0]->GameIsEfg());
  return por;
}


#include "lpsolve.h"

Portion* GSM_Lp_ListFloat(Portion** param)
{
  gMatrix<double>* a = ListToMatrix_Float((ListPortion*) param[0]);
  gVector<double>* b = ListToVector_Float((ListPortion*) param[1]);
  gVector<double>* c = ListToVector_Float((ListPortion*) param[2]);
  if(!a || !b || !c)
    return 0;
  int nequals = ((IntPortion*) param[3])->Value();
  bool isFeasible;
  bool isBounded;
  
  LPSolve<double>* s = new LPSolve<double>(*a, *b, *c, nequals);
  Portion* result = ArrayToList(s->OptimumVector());
  isFeasible = s->IsFeasible();
  isBounded = s->IsBounded();
  delete s;
  delete a;
  delete b;
  delete c;
  
  ((BoolPortion*) param[4])->Value() = isFeasible;
  ((BoolPortion*) param[5])->Value() = isBounded;
  return result;
}

Portion* GSM_Lp_ListRational(Portion** param)
{
  gMatrix<gRational>* a = ListToMatrix_Rational((ListPortion*) param[0]);
  gVector<gRational>* b = ListToVector_Rational((ListPortion*) param[1]);
  gVector<gRational>* c = ListToVector_Rational((ListPortion*) param[2]);
  if(!a || !b || !c)
    return 0;
  int nequals = ((IntPortion*) param[3])->Value();
  bool isFeasible;
  bool isBounded;
  
  LPSolve<gRational>* s = new LPSolve<gRational>(*a, *b, *c, nequals);
  Portion* result = ArrayToList(s->OptimumVector());
  isFeasible = s->IsFeasible();
  isBounded = s->IsBounded();
  delete s;
  delete a;
  delete b;
  delete c;
  
  ((BoolPortion*) param[4])->Value() = isFeasible;
  ((BoolPortion*) param[5])->Value() = isBounded;
  return result;
}




#include "csumsub.h"
#include "efgcsum.h"

Portion *GSM_Lp_EfgFloat(Portion **param)
{
  Efg<double> &E = * (Efg<double> *) ((EfgPortion*) param[0])->Value();
  
  if (E.NumPlayers() > 2 || !E.IsConstSum())
    return new ErrorPortion("Only valid for two-person zero-sum games");

  gList<BehavSolution<double> > solns;
  
  if (((BoolPortion *) param[1])->Value())   {
    ZSumParams ZP;

    ZP.tracefile = &((OutputPortion *) param[4])->Value();
    ZP.trace = ((IntPortion *) param[5])->Value();

    ZSumBySubgame<double> ZM(E, ZP);

    ZM.Solve();

    solns = ZM.GetSolutions();

    ((IntPortion *) param[2])->Value() = ZM.NumPivots();
    ((FloatPortion *) param[3])->Value() = ZM.Time();
  }
  else  {
    CSSeqFormParams ZP;

    ZP.tracefile = &((OutputPortion *) param[4])->Value();
    ZP.trace = ((IntPortion *) param[5])->Value();

    CSSeqFormBySubgame<double> ZM(E, ZP);

    ZM.Solve();

    solns = ZM.GetSolutions();

    ((IntPortion *) param[2])->Value() = ZM.NumPivots();
    ((FloatPortion *) param[3])->Value() = ZM.Time();
  }

  Portion* por = new Behav_ListPortion<double>(solns);
  por->SetGame(param[0]->Game(), param[0]->GameIsEfg());
  return por;
}

Portion *GSM_Lp_EfgRational(Portion **param)
{
  Efg<gRational> &E = * (Efg<gRational> *) ((EfgPortion*) param[0])->Value();
  
  if (E.NumPlayers() > 2 || !E.IsConstSum())
    return new ErrorPortion("Only valid for two-person zero-sum games");

  gList<BehavSolution<gRational> > solns;

  if (((BoolPortion *) param[1])->Value())   {
    ZSumParams ZP;

    ZP.tracefile = &((OutputPortion *) param[4])->Value();
    ZP.trace = ((IntPortion *) param[5])->Value();

    ZSumBySubgame<gRational> ZM(E, ZP);

    ZM.Solve();

    solns = ZM.GetSolutions();

    ((IntPortion *) param[2])->Value() = ZM.NumPivots();
    ((FloatPortion *) param[3])->Value() = ZM.Time();
  }
  else  {
    CSSeqFormParams ZP;

    ZP.tracefile = &((OutputPortion *) param[4])->Value();
    ZP.trace = ((IntPortion *) param[5])->Value();

    CSSeqFormBySubgame<gRational> ZM(E, ZP);

    ZM.Solve();

    solns = ZM.GetSolutions();

    ((IntPortion *) param[2])->Value() = ZM.NumPivots();
    ((FloatPortion *) param[3])->Value() = ZM.Time();
  }

  Portion* por = new Behav_ListPortion<gRational>(solns);
  por->SetGame(param[0]->Game(), param[0]->GameIsEfg());
  return por;
}

//---------
// Nfg
//---------

Portion *GSM_Nfg_Float(Portion **param)
{
  Efg<double> &E = * (Efg<double>*) ((EfgPortion*) param[0])->Value();
  gWatch watch;

  Nfg<double> *N = MakeReducedNfg(E, EFSupport(E));
  
  ((FloatPortion *) param[1])->Value() = watch.Elapsed();
  
  if (N)
    return new NfgValPortion(N);
  else
    return new ErrorPortion("Conversion to reduced nfg failed");
}

Portion *GSM_Nfg_Rational(Portion **param)
{
  Efg<gRational> &E = * (Efg<gRational>*) ((EfgPortion*) param[0])->Value();
  gWatch watch;

  Nfg<gRational> *N = MakeReducedNfg(E, EFSupport(E));
  
  ((FloatPortion *) param[1])->Value() = watch.Elapsed();

  if (N)
    return new NfgValPortion(N);
  else
    return new ErrorPortion("Conversion to reduced nfg failed");
}


//----------
// Payoff
//----------

Portion* GSM_Payoff_BehavFloat(Portion** param)
{
  int i;
  Portion* por = new ListValPortion;
  BehavSolution<double>* bp = 
    (BehavSolution<double>*) ((BehavPortion*) param[0])->Value();
  for(i = 1; i <= bp->BelongsTo()->PlayerList().Length(); i++)
  {
    ((ListValPortion*) por)->Append(new FloatValPortion(bp->Payoff(i)));
  }
  return por;
}

 Portion* GSM_Payoff_BehavRational(Portion** param)
{
  int i;
  Portion* por = new ListValPortion;
  BehavSolution<gRational>* bp = 
    (BehavSolution<gRational>*) ((BehavPortion*) param[0])->Value();
  for(i = 1; i <= bp->BelongsTo()->PlayerList().Length(); i++)
  {
    ((ListValPortion*) por)->Append(new RationalValPortion(bp->Payoff(i)));
  }
  return por;
}


Portion* GSM_Payoff_MixedFloat(Portion** param)
{
  int i;
  Portion* por = new ListValPortion;
  MixedSolution<double>* bp = 
    (MixedSolution<double>*) ((MixedPortion*) param[0])->Value();
  for(i = 1; i <= bp->BelongsTo()->NumPlayers(); i++)
  {
    ((ListValPortion*) por)->Append(new FloatValPortion(bp->Payoff(i)));
  }
  return por;
}

Portion* GSM_Payoff_MixedRational(Portion** param)
{
  int i;
  Portion* por = new ListValPortion;
  MixedSolution<gRational>* bp = 
    (MixedSolution<gRational>*) ((MixedPortion*) param[0])->Value();
  for(i = 1; i <= bp->BelongsTo()->NumPlayers(); i++)
  {
    ((ListValPortion*) por)->Append(new RationalValPortion(bp->Payoff(i)));
  }
  return por;
}


Portion* GSM_Payoff_NfgFloat(Portion** param)
{
  int i;
  Portion* p;
  Portion* por = new ListValPortion;
  Nfg<double>* nfg = (Nfg<double>*) ((NfgPortion*) param[0])->Value();
  gArray<int> Solution(((ListPortion*) param[1])->Length());
  
  if(((ListPortion*) param[1])->Length() != nfg->NumPlayers())
    return new ErrorPortion("Invalid number of players specified in \"list\"");
  
  for(i = 1; i <= nfg->NumPlayers() ; i++)
  {
    p = ((ListPortion*) param[1])->SubscriptCopy(i);
    assert(p->Spec().Type == porINTEGER);
    Solution[i] = ((IntPortion*) p)->Value();
    delete p;
  }
  for(i = 1; i <= nfg->NumPlayers(); i++)
  {
    ((ListValPortion*) por)->
      Append(new FloatValPortion(nfg->Payoff(i, Solution)));
  }
  return por;
}

Portion* GSM_Payoff_NfgRational(Portion** param)
{
  int i;
  Portion* p;
  Portion* por = new ListValPortion;
  Nfg<gRational>* nfg = (Nfg<gRational>*) ((NfgPortion*) param[0])->Value();
  gArray<int> Solution(((ListPortion*) param[1])->Length());
  
  if(((ListPortion*) param[1])->Length() != nfg->NumPlayers())
    return new ErrorPortion("Invalid number of players specified in \"list\"");
  
  for(i = 1; i <= nfg->NumPlayers() ; i++)
  {
    p = ((ListPortion*) param[1])->SubscriptCopy(i);
    assert(p->Spec().Type == porINTEGER);
    Solution[i] = ((IntPortion*) p)->Value();
    delete p;
  }
  for(i = 1; i <= nfg->NumPlayers(); i++)
  {
    ((ListValPortion*) por)->
      Append(new RationalValPortion(nfg->Payoff(i, Solution)));
  }
  return por;
}




//----------------
// SimpDivSolve
//----------------

#include "simpdiv.h"

Portion *GSM_Simpdiv_Support(Portion **param)
{
  NFSupport& S = * ((NfSupportPortion*) param[0])->Value();
  BaseNfg* N = (BaseNfg*) &(S.BelongsTo());
  Portion* por = 0;

  SimpdivParams SP;
  SP.stopAfter = ((IntPortion *) param[1])->Value();
  SP.nRestarts = ((IntPortion *) param[2])->Value();
  SP.leashLength = ((IntPortion *) param[3])->Value();

  SP.tracefile = &((OutputPortion *) param[6])->Value();
  SP.trace = ((IntPortion *) param[7])->Value();

  switch(N->Type())
  {
  case DOUBLE:
    {
      SimpdivModule<double> SM(* (Nfg<double>*) N, SP, S);
      SM.Simpdiv();
      ((IntPortion *) param[4])->Value() = SM.NumEvals();
      ((FloatPortion *) param[5])->Value() = SM.Time();
      por = new Mixed_ListPortion<double>(SM.GetSolutions());
    }
    break;
  case RATIONAL:
    {
      SimpdivModule<gRational> SM(* (Nfg<gRational>*) N, SP, S);
      SM.Simpdiv();
      ((IntPortion *) param[4])->Value() = SM.NumEvals();
      ((FloatPortion *) param[5])->Value() = SM.Time();
      por = new Mixed_ListPortion<gRational>(SM.GetSolutions());
    }
    break;
  default:
    assert(0);
  }

  assert(por != 0);
  por->SetGame(param[0]->Game(), param[0]->GameIsEfg());
  return por;
}

#include "simpsub.h"

Portion *GSM_Simpdiv_EfgFloat(Portion **param)
{
  Efg<double> &E = * (Efg<double> *) ((EfgPortion*) param[0])->Value();
  
  if (!((BoolPortion *) param[1])->Value())  
    return new ErrorPortion("algorithm not implemented for extensive forms");

  SimpdivParams SP;
  SP.stopAfter = ((IntPortion *) param[2])->Value();
  SP.nRestarts = ((IntPortion *) param[3])->Value();
  SP.leashLength = ((IntPortion *) param[4])->Value();

  SP.tracefile = &((OutputPortion *) param[7])->Value();
  SP.trace = ((IntPortion *) param[8])->Value();

  SimpdivBySubgame<double> SM(E, SP);
  SM.Solve();

  gList<BehavSolution<double> > solns(SM.GetSolutions());

  ((IntPortion *) param[5])->Value() = SM.NumEvals();
  ((FloatPortion *) param[6])->Value() = SM.Time();
  
  Portion* por = new Behav_ListPortion<double>(solns);
  por->SetGame(param[0]->Game(), param[0]->GameIsEfg());
  return por;
}

Portion *GSM_Simpdiv_EfgRational(Portion **param)
{
  Efg<gRational> &E = * (Efg<gRational> *) ((EfgPortion*) param[0])->Value();
  
  if (!((BoolPortion *) param[1])->Value())  
    return new ErrorPortion("algorithm not implemented for extensive forms");

  SimpdivParams SP;
  SP.stopAfter = ((IntPortion *) param[2])->Value();
  SP.nRestarts = ((IntPortion *) param[3])->Value();
  SP.leashLength = ((IntPortion *) param[4])->Value();

  SP.tracefile = &((OutputPortion *) param[7])->Value();
  SP.trace = ((IntPortion *) param[8])->Value();

  SimpdivBySubgame<gRational> SM(E, SP);

  SM.Solve();

  gList<BehavSolution<gRational> > solns(SM.GetSolutions());

  ((IntPortion *) param[5])->Value() = SM.NumEvals();
  ((FloatPortion *) param[6])->Value() = SM.Time();
  
  Portion* por = new Behav_ListPortion<gRational>(solns);
  por->SetGame(param[0]->Game(), param[0]->GameIsEfg());
  return por;
}


void Init_algfunc(GSM *gsm)
{
  FuncDescObj *FuncObj;

  FuncObj = new FuncDescObj("AgentForm", 2);
  FuncObj->SetFuncInfo(0, FuncInfoType(GSM_AgentForm_Float, porNFG_FLOAT, 2));
  FuncObj->SetParamInfo(0, 0, ParamInfoType("efg", porEFG_FLOAT));
  FuncObj->SetParamInfo(0, 1, ParamInfoType("time", porFLOAT,
					    new FloatValPortion(0), BYREF));
  
  FuncObj->SetFuncInfo(1, FuncInfoType(GSM_AgentForm_Rational, porNFG_RATIONAL, 2));
  FuncObj->SetParamInfo(1, 0, ParamInfoType("efg", porEFG_RATIONAL));
  FuncObj->SetParamInfo(1, 1, ParamInfoType("time", porFLOAT,
					    new FloatValPortion(0), BYREF));
  gsm->AddFunction(FuncObj);



  FuncObj = new FuncDescObj("Behav", 2);
  FuncObj->SetFuncInfo(0, FuncInfoType(GSM_Behav_Float, porBEHAV_FLOAT, 2));
  FuncObj->SetParamInfo(0, 0, ParamInfoType("mixed", porMIXED_FLOAT));
  FuncObj->SetParamInfo(0, 1, ParamInfoType("efg", porEFG_FLOAT));

  FuncObj->SetFuncInfo(1, FuncInfoType(GSM_Behav_Rational, 
				       porBEHAV_RATIONAL, 2));
  FuncObj->SetParamInfo(1, 0, ParamInfoType("mixed", porMIXED_RATIONAL));
  FuncObj->SetParamInfo(1, 1, ParamInfoType("efg", porEFG_RATIONAL));
  gsm->AddFunction(FuncObj);



  FuncObj = new FuncDescObj("EnumMixedSolve", 3);
  FuncObj->SetFuncInfo(0, FuncInfoType(GSM_EnumMixed_Support, 
				       PortionSpec(porMIXED, 1), 6));
  FuncObj->SetParamInfo(0, 0, ParamInfoType("support", porNF_SUPPORT));
  FuncObj->SetParamInfo(0, 1, ParamInfoType("stopAfter", porINTEGER,
					    new IntValPortion(0)));
  FuncObj->SetParamInfo(0, 2, ParamInfoType("nPivots", porINTEGER,
					    new IntValPortion(0), BYREF));
  FuncObj->SetParamInfo(0, 3, ParamInfoType("time", porFLOAT,
					    new FloatValPortion(0.0), BYREF));
  FuncObj->SetParamInfo(0, 4, ParamInfoType("traceFile", porOUTPUT,
					    new OutputRefPortion(gnull), 
					    BYREF));
  FuncObj->SetParamInfo(0, 5, ParamInfoType("traceLevel", porINTEGER,
					    new IntValPortion(0)));

  FuncObj->SetFuncInfo(1, FuncInfoType(GSM_EnumMixed_EfgFloat, 
				       PortionSpec(porBEHAV_FLOAT, 1), 7));
  FuncObj->SetParamInfo(1, 0, ParamInfoType("efg", porEFG_FLOAT));
  FuncObj->SetParamInfo(1, 1, ParamInfoType("asNfg", porBOOL,
					    new BoolValPortion(false)));
  FuncObj->SetParamInfo(1, 2, ParamInfoType("stopAfter", porINTEGER,
					    new IntValPortion(0)));
  FuncObj->SetParamInfo(1, 3, ParamInfoType("nPivots", porINTEGER,
					    new IntValPortion(0), BYREF));
  FuncObj->SetParamInfo(1, 4, ParamInfoType("time", porFLOAT,
					    new FloatValPortion(0.0), BYREF));
  FuncObj->SetParamInfo(1, 5, ParamInfoType("traceFile", porOUTPUT,
					    new OutputRefPortion(gnull), 
					    BYREF));
  FuncObj->SetParamInfo(1, 6, ParamInfoType("traceLevel", porINTEGER,
					    new IntValPortion(0)));

  FuncObj->SetFuncInfo(2, FuncInfoType(GSM_EnumMixed_EfgRational, 
				       PortionSpec(porBEHAV_RATIONAL, 1), 7));
  FuncObj->SetParamInfo(2, 0, ParamInfoType("efg", porEFG_RATIONAL));
  FuncObj->SetParamInfo(2, 1, ParamInfoType("asNfg", porBOOL,
					    new BoolValPortion(false)));
  FuncObj->SetParamInfo(2, 2, ParamInfoType("stopAfter", porINTEGER,
					    new IntValPortion(0)));
  FuncObj->SetParamInfo(2, 3, ParamInfoType("nPivots", porINTEGER,
					    new IntValPortion(0), BYREF));
  FuncObj->SetParamInfo(2, 4, ParamInfoType("time", porFLOAT,
					    new FloatValPortion(0.0), BYREF));
  FuncObj->SetParamInfo(2, 5, ParamInfoType("traceFile", porOUTPUT,
					    new OutputRefPortion(gnull), 
					    BYREF));
  FuncObj->SetParamInfo(2, 6, ParamInfoType("traceLevel", porINTEGER,
					    new IntValPortion(0)));
  gsm->AddFunction(FuncObj);


  FuncObj = new FuncDescObj("EnumPureSolve", 3);
  FuncObj->SetFuncInfo(0, FuncInfoType(GSM_EnumPure_Support, 
				       PortionSpec(porMIXED, 1), 5));
  FuncObj->SetParamInfo(0, 0, ParamInfoType("support", porNF_SUPPORT));
  FuncObj->SetParamInfo(0, 1, ParamInfoType("stopAfter", porINTEGER,
					    new IntValPortion(0)));
  FuncObj->SetParamInfo(0, 2, ParamInfoType("time", porFLOAT,
					    new FloatValPortion(0.0), BYREF));
  FuncObj->SetParamInfo(0, 3, ParamInfoType("traceFile", porOUTPUT,
					    new OutputRefPortion(gnull), 
					    BYREF));
  FuncObj->SetParamInfo(0, 4, ParamInfoType("traceLevel", porINTEGER,
					    new IntValPortion(0)));

  FuncObj->SetFuncInfo(1, FuncInfoType(GSM_EnumPure_EfgFloat, 
				       PortionSpec(porBEHAV_FLOAT, 1), 6));
  FuncObj->SetParamInfo(1, 0, ParamInfoType("efg", porEFG_FLOAT));
  FuncObj->SetParamInfo(1, 1, ParamInfoType("asNfg", porBOOL,
					    new BoolValPortion(false)));
  FuncObj->SetParamInfo(1, 2, ParamInfoType("stopAfter", porINTEGER,
					    new IntValPortion(0)));
  FuncObj->SetParamInfo(1, 3, ParamInfoType("time", porFLOAT,
					    new FloatValPortion(0.0), BYREF));
  FuncObj->SetParamInfo(1, 4, ParamInfoType("traceFile", porOUTPUT,
					    new OutputRefPortion(gnull), 
					    BYREF));
  FuncObj->SetParamInfo(1, 5, ParamInfoType("traceLevel", porINTEGER,
					    new IntValPortion(0)));

  FuncObj->SetFuncInfo(2, FuncInfoType(GSM_EnumPure_EfgRational, 
				       PortionSpec(porBEHAV_RATIONAL, 1), 6));
  FuncObj->SetParamInfo(2, 0, ParamInfoType("efg", porEFG_RATIONAL));
  FuncObj->SetParamInfo(2, 1, ParamInfoType("asNfg", porBOOL,
					    new BoolValPortion(false)));
  FuncObj->SetParamInfo(2, 2, ParamInfoType("stopAfter", porINTEGER,
					    new IntValPortion(0)));
  FuncObj->SetParamInfo(2, 3, ParamInfoType("time", porFLOAT,
					    new FloatValPortion(0.0), BYREF));
  FuncObj->SetParamInfo(2, 4, ParamInfoType("traceFile", porOUTPUT,
					    new OutputRefPortion(gnull),
					    BYREF));
  FuncObj->SetParamInfo(2, 5, ParamInfoType("traceLevel", porINTEGER,
					    new IntValPortion(0)));
  gsm->AddFunction(FuncObj);


  FuncObj = new FuncDescObj("GobitGridSolve", 1);
  FuncObj->SetFuncInfo(0, FuncInfoType(GSM_GobitGrid_Support, 
				       PortionSpec(porMIXED, 1), 12));
  FuncObj->SetParamInfo(0, 0, ParamInfoType("support", porNF_SUPPORT));
  FuncObj->SetParamInfo(0, 1, ParamInfoType("pxifile", porTEXT,
					    new TextValPortion("")));
  FuncObj->SetParamInfo(0, 2, ParamInfoType("minLam", porFLOAT,
					    new FloatValPortion(0.01)));
  FuncObj->SetParamInfo(0, 3, ParamInfoType("maxLam", porFLOAT,
					    new FloatValPortion(30.0)));
  FuncObj->SetParamInfo(0, 4, ParamInfoType("delLam", porFLOAT,
					    new FloatValPortion(0.01)));
  FuncObj->SetParamInfo(0, 5, ParamInfoType("powLam", porINTEGER,
					    new IntValPortion(1)));
  FuncObj->SetParamInfo(0, 6, ParamInfoType("delp", porFLOAT,
					    new FloatValPortion(.01)));
  FuncObj->SetParamInfo(0, 7, ParamInfoType("tol", porFLOAT,
					    new FloatValPortion(.01)));
  FuncObj->SetParamInfo(0, 8, ParamInfoType("nEvals", porINTEGER,
					    new IntValPortion(0), BYREF));
  FuncObj->SetParamInfo(0, 9, ParamInfoType("time", porFLOAT,
					    new FloatValPortion(0.0), BYREF));
  FuncObj->SetParamInfo(0, 10, ParamInfoType("traceFile", porOUTPUT,
					     new OutputRefPortion(gnull),
					     BYREF));
  FuncObj->SetParamInfo(0, 11, ParamInfoType("traceLevel", porINTEGER,
					     new IntValPortion(0)));

  gsm->AddFunction(FuncObj);


  FuncObj = new FuncDescObj("GobitSolve", 1);
  FuncObj->SetFuncInfo(0, FuncInfoType(GSM_Gobit_Start, 
				       PortionSpec(porMIXED_FLOAT |
						   porBEHAV_FLOAT, 1), 16));
  FuncObj->SetParamInfo(0, 0, ParamInfoType("start",
					    porMIXED_FLOAT | porBEHAV_FLOAT));
  FuncObj->SetParamInfo(0, 1, ParamInfoType("pxifile", porTEXT,
					    new TextValPortion("")));
  FuncObj->SetParamInfo(0, 2, ParamInfoType("minLam", porFLOAT,
					    new FloatValPortion(0.01)));
  FuncObj->SetParamInfo(0, 3, ParamInfoType("maxLam", porFLOAT,
					    new FloatValPortion(30.0)));
  FuncObj->SetParamInfo(0, 4, ParamInfoType("delLam", porFLOAT,
					    new FloatValPortion(0.01)));
  FuncObj->SetParamInfo(0, 5, ParamInfoType("powLam", porINTEGER,
					    new IntValPortion(1)));
  FuncObj->SetParamInfo(0, 6, ParamInfoType("fullGraph", porBOOL,
					    new BoolValPortion(false)));
  FuncObj->SetParamInfo(0, 7, ParamInfoType("maxitsN", porINTEGER,
					    new IntValPortion(20)));
  FuncObj->SetParamInfo(0, 8, ParamInfoType("tolN", porFLOAT,
					    new FloatValPortion(1.0e-10)));
  FuncObj->SetParamInfo(0, 9, ParamInfoType("maxits1", porINTEGER,
					    new IntValPortion(100)));
  FuncObj->SetParamInfo(0, 10, ParamInfoType("tol1", porFLOAT,
					     new FloatValPortion(2.0e-10)));
  FuncObj->SetParamInfo(0, 11, ParamInfoType("time", porFLOAT,
					     new FloatValPortion(0), BYREF));
  FuncObj->SetParamInfo(0, 12, ParamInfoType("nEvals", porINTEGER,
					     new IntValPortion(0), BYREF));
  FuncObj->SetParamInfo(0, 13, ParamInfoType("nIters", porINTEGER,
					     new IntValPortion(0), BYREF));
  FuncObj->SetParamInfo(0, 14, ParamInfoType("traceFile", porOUTPUT,
					     new OutputRefPortion(gnull), 
					     BYREF));
  FuncObj->SetParamInfo(0, 15, ParamInfoType("traceLevel", porINTEGER,
					     new IntValPortion(0)));

  gsm->AddFunction(FuncObj);


  FuncObj = new FuncDescObj("LcpSolve", 4);
  FuncObj->SetFuncInfo(0, FuncInfoType(GSM_Lcp_NFSupport, 
				       PortionSpec(porMIXED, 1), 6));
  FuncObj->SetParamInfo(0, 0, ParamInfoType("support", porNF_SUPPORT));
  FuncObj->SetParamInfo(0, 1, ParamInfoType("stopAfter", porINTEGER, 
					    new IntValPortion(0)));
  FuncObj->SetParamInfo(0, 2, ParamInfoType("nPivots", porINTEGER, 
					    new IntValPortion(0), BYREF));
  FuncObj->SetParamInfo(0, 3, ParamInfoType("time", porFLOAT, 
					    new FloatValPortion(0), BYREF));
  FuncObj->SetParamInfo(0, 4, ParamInfoType("traceFile", porOUTPUT,
					    new OutputRefPortion(gnull), 
					    BYREF));
  FuncObj->SetParamInfo(0, 5, ParamInfoType("traceLevel", porINTEGER,
					    new IntValPortion(0)));

  FuncObj->SetFuncInfo(1, FuncInfoType(GSM_Lcp_ListFloat, 
				       PortionSpec(porFLOAT, 1), 2));
  FuncObj->SetParamInfo(1, 0, ParamInfoType("a", PortionSpec(porFLOAT,2),
					    REQUIRED, BYVAL));
  FuncObj->SetParamInfo(1, 1, ParamInfoType("b", PortionSpec(porFLOAT,1),
					    REQUIRED, BYVAL));

  FuncObj->SetFuncInfo(2, FuncInfoType(GSM_Lcp_ListRational, 
				       PortionSpec(porRATIONAL, 1), 2));
  FuncObj->SetParamInfo(2, 0, ParamInfoType("a", PortionSpec(porRATIONAL,2),
					    REQUIRED, BYVAL));
  FuncObj->SetParamInfo(2, 1, ParamInfoType("b", PortionSpec(porRATIONAL,1),
					    REQUIRED, BYVAL));

  FuncObj->SetFuncInfo(3, FuncInfoType(GSM_Lcp_EfSupport, 
				       PortionSpec(porBEHAV, 1), 7));
  FuncObj->SetParamInfo(3, 0, ParamInfoType("support", porEF_SUPPORT));
  FuncObj->SetParamInfo(3, 1, ParamInfoType("asNfg", porBOOL,
					    new BoolValPortion(false)));
  FuncObj->SetParamInfo(3, 2, ParamInfoType("stopAfter", porINTEGER,
					    new IntValPortion(0)));
  FuncObj->SetParamInfo(3, 3, ParamInfoType("nPivots", porINTEGER,
					    new IntValPortion(0), BYREF));
  FuncObj->SetParamInfo(3, 4, ParamInfoType("time", porFLOAT,
					    new FloatValPortion(0.0), BYREF));
  FuncObj->SetParamInfo(3, 5, ParamInfoType("traceFile", porOUTPUT,
					    new OutputRefPortion(gnull), 
					    BYREF));
  FuncObj->SetParamInfo(3, 6, ParamInfoType("traceLevel", porINTEGER,
					    new IntValPortion(0)));
  
  gsm->AddFunction(FuncObj);


  FuncObj = new FuncDescObj("LiapSolve", 2);
  FuncObj->SetFuncInfo(0, FuncInfoType(GSM_Liap_BehavFloat, 
				       PortionSpec(porBEHAV_FLOAT, 1), 12));
  FuncObj->SetParamInfo(0, 0, ParamInfoType("start", porBEHAV_FLOAT));
  FuncObj->SetParamInfo(0, 1, ParamInfoType("asNfg", porBOOL,
					    new BoolValPortion(false)));
  FuncObj->SetParamInfo(0, 2, ParamInfoType("stopAfter", porINTEGER,
					    new IntValPortion(1)));
  FuncObj->SetParamInfo(0, 3, ParamInfoType("nTries", porINTEGER,
					    new IntValPortion(10)));
  FuncObj->SetParamInfo(0, 4, ParamInfoType("maxitsN", porINTEGER,
					    new IntValPortion(20)));
  FuncObj->SetParamInfo(0, 5, ParamInfoType("tolN", porFLOAT,
					    new FloatValPortion(1.0e-10)));
  FuncObj->SetParamInfo(0, 6, ParamInfoType("maxits1", porINTEGER,
					    new IntValPortion(100)));
  FuncObj->SetParamInfo(0, 7, ParamInfoType("tol1", porFLOAT,
					    new FloatValPortion(2.0e-10)));
  FuncObj->SetParamInfo(0, 8, ParamInfoType("time", porFLOAT,
					    new FloatValPortion(0), BYREF));
  FuncObj->SetParamInfo(0, 9, ParamInfoType("nEvals", porINTEGER,
					    new IntValPortion(0), BYREF));
  FuncObj->SetParamInfo(0, 10, ParamInfoType("traceFile", porOUTPUT,
					     new OutputRefPortion(gnull), 
					     BYREF));
  FuncObj->SetParamInfo(0, 11, ParamInfoType("traceLevel", porINTEGER,
					     new IntValPortion(0)));

  FuncObj->SetFuncInfo(1, FuncInfoType(GSM_Liap_MixedFloat, 
				       PortionSpec(porMIXED_FLOAT, 1), 11));
  FuncObj->SetParamInfo(1, 0, ParamInfoType("start", porMIXED_FLOAT));
  FuncObj->SetParamInfo(1, 1, ParamInfoType("stopAfter", porINTEGER,
					    new IntValPortion(1)));
  FuncObj->SetParamInfo(1, 2, ParamInfoType("nTries", porINTEGER,
					    new IntValPortion(10)));
  FuncObj->SetParamInfo(1, 3, ParamInfoType("maxitsN", porINTEGER,
					    new IntValPortion(20)));
  FuncObj->SetParamInfo(1, 4, ParamInfoType("tolN", porFLOAT,
					    new FloatValPortion(1.0e-10)));
  FuncObj->SetParamInfo(1, 5, ParamInfoType("maxits1", porINTEGER,
					    new IntValPortion(100)));
  FuncObj->SetParamInfo(1, 6, ParamInfoType("tol1", porFLOAT,
					    new FloatValPortion(2.0e-10)));
  FuncObj->SetParamInfo(1, 7, ParamInfoType("time", porFLOAT,
					    new FloatValPortion(0), BYREF));
  FuncObj->SetParamInfo(1, 8, ParamInfoType("nEvals", porINTEGER,
					    new IntValPortion(0), BYREF));
  FuncObj->SetParamInfo(1, 9, ParamInfoType("traceFile", porOUTPUT,
					    new OutputRefPortion(gnull), 
					    BYREF));
  FuncObj->SetParamInfo(1, 10, ParamInfoType("traceLevel", porINTEGER,
					     new IntValPortion(0)));

  gsm->AddFunction(FuncObj);


  FuncObj = new FuncDescObj("LpSolve", 5);
  FuncObj->SetFuncInfo(0, FuncInfoType(GSM_Lp_Support, 
				       PortionSpec(porMIXED, 1), 5));
  FuncObj->SetParamInfo(0, 0, ParamInfoType("support", porNF_SUPPORT));
  FuncObj->SetParamInfo(0, 1, ParamInfoType("nPivots", porINTEGER,
					    new IntValPortion(0), BYREF));
  FuncObj->SetParamInfo(0, 2, ParamInfoType("time", porFLOAT,
					    new FloatValPortion(0.0), BYREF));
  FuncObj->SetParamInfo(0, 3, ParamInfoType("traceFile", porOUTPUT,
					    new OutputRefPortion(gnull),
					    BYREF));
  FuncObj->SetParamInfo(0, 4, ParamInfoType("traceLevel", porINTEGER,
					    new IntValPortion(0)));

  FuncObj->SetFuncInfo(1, FuncInfoType(GSM_Lp_EfgFloat, 
				       PortionSpec(porBEHAV_FLOAT, 1), 6));
  FuncObj->SetParamInfo(1, 0, ParamInfoType("efg", porEFG_FLOAT));
  FuncObj->SetParamInfo(1, 1, ParamInfoType("asNfg", porBOOL,
					    new BoolValPortion(false)));
  FuncObj->SetParamInfo(1, 2, ParamInfoType("nPivots", porINTEGER,
					    new IntValPortion(0), BYREF));
  FuncObj->SetParamInfo(1, 3, ParamInfoType("time", porFLOAT,
					    new FloatValPortion(0.0), BYREF));
  FuncObj->SetParamInfo(1, 4, ParamInfoType("traceFile", porOUTPUT,
					    new OutputRefPortion(gnull), 
					    BYREF));
  FuncObj->SetParamInfo(1, 5, ParamInfoType("traceLevel", porINTEGER,
					    new IntValPortion(0)));

  FuncObj->SetFuncInfo(2, FuncInfoType(GSM_Lp_EfgRational, 
				       PortionSpec(porBEHAV_RATIONAL, 1), 6));
  FuncObj->SetParamInfo(2, 0, ParamInfoType("efg", porEFG_RATIONAL));
  FuncObj->SetParamInfo(2, 1, ParamInfoType("asNfg", porBOOL,
					    new BoolValPortion(false)));
  FuncObj->SetParamInfo(2, 2, ParamInfoType("nPivots", porINTEGER,
					    new IntValPortion(0), BYREF));
  FuncObj->SetParamInfo(2, 3, ParamInfoType("time", porFLOAT,
					    new FloatValPortion(0.0), BYREF));
  FuncObj->SetParamInfo(2, 4, ParamInfoType("traceFile", porOUTPUT,
					    new OutputRefPortion(gnull),
					    BYREF));
  FuncObj->SetParamInfo(2, 5, ParamInfoType("traceLevel", porINTEGER,
					    new IntValPortion(0)));

  FuncObj->SetFuncInfo(3, FuncInfoType(GSM_Lp_ListFloat, 
				       PortionSpec(porFLOAT, 1), 6));
  FuncObj->SetParamInfo(3, 0, ParamInfoType("a", PortionSpec(porFLOAT,2),
					    REQUIRED, BYVAL));
  FuncObj->SetParamInfo(3, 1, ParamInfoType("b", PortionSpec(porFLOAT,1),
					    REQUIRED, BYVAL));
  FuncObj->SetParamInfo(3, 2, ParamInfoType("c", PortionSpec(porFLOAT,1),
					    REQUIRED, BYVAL));
  FuncObj->SetParamInfo(3, 3, ParamInfoType("nEqualities", porINTEGER));
  FuncObj->SetParamInfo(3, 4, ParamInfoType("isFeasible", porBOOL,
					    new BoolValPortion(false), BYREF));
  FuncObj->SetParamInfo(3, 5, ParamInfoType("isBounded", porBOOL,
					    new BoolValPortion(false), BYREF));

  FuncObj->SetFuncInfo(4, FuncInfoType(GSM_Lp_ListRational, 
				       PortionSpec(porRATIONAL, 1), 6));
  FuncObj->SetParamInfo(4, 0, ParamInfoType("a", PortionSpec(porRATIONAL,2),
					    REQUIRED, BYVAL));
  FuncObj->SetParamInfo(4, 1, ParamInfoType("b", PortionSpec(porRATIONAL,1),
					    REQUIRED, BYVAL));
  FuncObj->SetParamInfo(4, 2, ParamInfoType("c", PortionSpec(porRATIONAL,1),
					    REQUIRED, BYVAL));
  FuncObj->SetParamInfo(4, 3, ParamInfoType("nEqualities", porINTEGER));
  FuncObj->SetParamInfo(4, 4, ParamInfoType("isFeasible", porBOOL,
					    new BoolValPortion(false), BYREF));
  FuncObj->SetParamInfo(4, 5, ParamInfoType("isBounded", porBOOL,
					    new BoolValPortion(false), BYREF));

  gsm->AddFunction(FuncObj);



  FuncObj = new FuncDescObj("Nfg", 2);
  FuncObj->SetFuncInfo(0, FuncInfoType(GSM_Nfg_Float, porNFG_FLOAT, 2));
  FuncObj->SetParamInfo(0, 0, ParamInfoType("efg", porEFG_FLOAT));
  FuncObj->SetParamInfo(0, 1, ParamInfoType("time", porFLOAT,
					    new FloatValPortion(0), BYREF));

  FuncObj->SetFuncInfo(1, FuncInfoType(GSM_Nfg_Rational, porNFG_RATIONAL, 2));
  FuncObj->SetParamInfo(1, 0, ParamInfoType("efg", porEFG_RATIONAL));
  FuncObj->SetParamInfo(1, 1, ParamInfoType("time", porFLOAT,
					    new FloatValPortion(0), BYREF));
  gsm->AddFunction(FuncObj);




  FuncObj = new FuncDescObj("Payoff", 6);
  FuncObj->SetFuncInfo(0, FuncInfoType(GSM_Payoff_BehavFloat, 
				       PortionSpec(porFLOAT, 1), 1));
  FuncObj->SetParamInfo(0, 0, ParamInfoType("strategy", porBEHAV_FLOAT));
  FuncObj->SetFuncInfo(1, FuncInfoType(GSM_Payoff_BehavRational,
				       PortionSpec(porRATIONAL, 1), 1));
  FuncObj->SetParamInfo(1, 0, ParamInfoType("strategy", porBEHAV_RATIONAL));

  FuncObj->SetFuncInfo(2, FuncInfoType(GSM_Payoff_MixedFloat, 
				       PortionSpec(porFLOAT, 1), 1));
  FuncObj->SetParamInfo(2, 0, ParamInfoType("strategy", porMIXED_FLOAT));
  FuncObj->SetFuncInfo(3, FuncInfoType(GSM_Payoff_MixedRational, 
				       PortionSpec(porRATIONAL, 1), 1));
  FuncObj->SetParamInfo(3, 0, ParamInfoType("strategy", porMIXED_RATIONAL));

  FuncObj->SetFuncInfo(4, FuncInfoType(GSM_Payoff_NfgFloat, 
				       PortionSpec(porFLOAT, 1), 2));
  FuncObj->SetParamInfo(4, 0, ParamInfoType("nfg", porNFG_FLOAT, 
					    REQUIRED, BYREF));
  FuncObj->SetParamInfo(4, 1, ParamInfoType("list", 
					    PortionSpec(porINTEGER,1)));
  FuncObj->SetFuncInfo(5, FuncInfoType(GSM_Payoff_NfgRational, 
				       PortionSpec(porRATIONAL, 1), 2));
  FuncObj->SetParamInfo(5, 0, ParamInfoType("nfg", porNFG_RATIONAL, 
					    REQUIRED, BYREF));
  FuncObj->SetParamInfo(5, 1, ParamInfoType("list", 
					    PortionSpec(porINTEGER,1)));
  gsm->AddFunction(FuncObj);



  FuncObj = new FuncDescObj("SimpDivSolve", 3);
  FuncObj->SetFuncInfo(0, FuncInfoType(GSM_Simpdiv_Support, 
				       PortionSpec(porMIXED, 1), 8));
  FuncObj->SetParamInfo(0, 0, ParamInfoType("support", porNF_SUPPORT));
  FuncObj->SetParamInfo(0, 1, ParamInfoType("stopAfter", porINTEGER,
					    new IntValPortion(1)));
  FuncObj->SetParamInfo(0, 2, ParamInfoType("nRestarts", porINTEGER,
					    new IntValPortion(1)));
  FuncObj->SetParamInfo(0, 3, ParamInfoType("leashLength", porINTEGER,
					    new IntValPortion(0)));
  FuncObj->SetParamInfo(0, 4, ParamInfoType("nEvals", porINTEGER,
					    new IntValPortion(0), BYREF));
  FuncObj->SetParamInfo(0, 5, ParamInfoType("time", porFLOAT,
					    new FloatValPortion(0.0), BYREF));
  FuncObj->SetParamInfo(0, 6, ParamInfoType("traceFile", porOUTPUT,
					    new OutputRefPortion(gnull), 
					    BYREF));
  FuncObj->SetParamInfo(0, 7, ParamInfoType("traceLevel", porINTEGER,
					    new IntValPortion(0)));

  FuncObj->SetFuncInfo(1, FuncInfoType(GSM_Simpdiv_EfgFloat, 
				       PortionSpec(porBEHAV_FLOAT, 1), 9));
  FuncObj->SetParamInfo(1, 0, ParamInfoType("efg", porEFG_FLOAT));
  FuncObj->SetParamInfo(1, 1, ParamInfoType("asNfg", porBOOL,
					    new BoolValPortion(false)));
  FuncObj->SetParamInfo(1, 2, ParamInfoType("stopAfter", porINTEGER,
					    new IntValPortion(1)));
  FuncObj->SetParamInfo(1, 3, ParamInfoType("nRestarts", porINTEGER,
					    new IntValPortion(1)));
  FuncObj->SetParamInfo(1, 4, ParamInfoType("leashLength", porINTEGER,
					    new IntValPortion(0)));
  FuncObj->SetParamInfo(1, 5, ParamInfoType("nEvals", porINTEGER,
					    new IntValPortion(0), BYREF));
  FuncObj->SetParamInfo(1, 6, ParamInfoType("time", porFLOAT,
					    new FloatValPortion(0.0), BYREF));
  FuncObj->SetParamInfo(1, 7, ParamInfoType("traceFile", porOUTPUT,
					    new OutputRefPortion(gnull), 
					    BYREF));
  FuncObj->SetParamInfo(1, 8, ParamInfoType("traceLevel", porINTEGER,
					    new IntValPortion(0)));

  FuncObj->SetFuncInfo(2, FuncInfoType(GSM_Simpdiv_EfgRational, 
				       PortionSpec(porBEHAV_RATIONAL, 1), 9));
  FuncObj->SetParamInfo(2, 0, ParamInfoType("efg", porEFG_RATIONAL));	
  FuncObj->SetParamInfo(2, 1, ParamInfoType("asNfg", porBOOL,
					    new BoolValPortion(false)));
  FuncObj->SetParamInfo(2, 2, ParamInfoType("stopAfter", porINTEGER,
					    new IntValPortion(1)));
  FuncObj->SetParamInfo(2, 3, ParamInfoType("nRestarts", porINTEGER,
					    new IntValPortion(1)));
  FuncObj->SetParamInfo(2, 4, ParamInfoType("leashLength", porINTEGER,
					    new IntValPortion(0)));
  FuncObj->SetParamInfo(2, 5, ParamInfoType("nEvals", porINTEGER,
					    new IntValPortion(0), BYREF));
  FuncObj->SetParamInfo(2, 6, ParamInfoType("time", porFLOAT,
					    new FloatValPortion(0.0), BYREF));
  FuncObj->SetParamInfo(2, 7, ParamInfoType("traceFile", porOUTPUT,
					    new OutputRefPortion(gnull), 
					    BYREF));
  FuncObj->SetParamInfo(2, 8, ParamInfoType("traceLevel", porINTEGER,
					    new IntValPortion(0)));
  gsm->AddFunction(FuncObj);
}


#ifdef __GNUG__
#define TEMPLATE template
#elif defined __BORLANDC__
#define TEMPLATE
#pragma option -Jgd
#endif   // __GNUG__, __BORLANDC__

TEMPLATE class Mixed_ListPortion<double>;
TEMPLATE class Mixed_ListPortion<gRational>;

TEMPLATE class Behav_ListPortion<double>;
TEMPLATE class Behav_ListPortion<gRational>;


