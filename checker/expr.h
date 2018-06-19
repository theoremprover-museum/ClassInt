#ifndef EXPR_H
#define EXPR_H

#include <utility>
#include <map>
//#include <stlport/hash_map>
#include <list>
#include <string>
#include <ostream>
#include <assert.h>

#pragma hdrstop

#include "term.h"

typedef /*hash_*/map<string,size_t> PredArray;
typedef PredArray::iterator PredIterator;

extern PredArray pred_array;

///-------------------------------------------------------------------------
class Expr {
public:
  enum OPERATION {
    OP__VOID,
    OP_EX,
    OP_FOR,
    OP_IFF,
    OP_IMPLIES,
    OP_OR,
    OP_AND,
    OP_NOT,
    OP_TRUE,
    OP_FALSE,
    OP_PRED,
    OP_EQUALITY,
    OP__MAX
  };

private:
  Expr& operator =(const Expr&);

public:
  virtual bool equalTo(const Expr& e,CmpInfo &ci) const = 0;
  virtual bool lessThan(const Expr& e) const = 0;

  virtual Expr* clone() const = 0;
  virtual void out(ostream& os,QrArray &q,OPERATION upper_op=OP__VOID) const = 0;

  bool operator ==(const Expr& e) const { return equalTo(e,CmpInfo()); }
  bool operator <(const Expr& e) const { return lessThan(e); }

  virtual bool unify(const Expr &e,LocalSubst &ls,GlobalSubst &gs) const = 0;
  //bool is_super(const Expr &e,LocalSubst &ls) const = 0;
  bool is_super(const Expr &e) const;

  virtual Expr* p_all_subst(const Term &t) const = 0;
  virtual Expr* q2t(const Term &t) const = 0;
  virtual Expr* t2q(const Term &t) const = 0;  // t!=qvar && t!=pvar
  virtual bool check_qt(const Expr &e,TermVec &tv) const = 0;
  virtual Expr* p_subst(const LocalSubst &ls,const GlobalSubst &gs) const = 0;
  virtual int t_depth() const { return 0; }
  virtual int max_lmeta() const = 0;
  virtual Expr* l2g(LocalSubst&) const = 0;
  virtual int redo_lmeta(MetaMap &m,int base) const = 0;
  virtual OPERATION op() const = 0;

  virtual ~Expr() {}
};

///-------------------------------------------------------------------------
class DummyExpr: public Expr {
  const OPERATION opc;

public:
  DummyExpr(const DummyExpr &de): opc(de.op()) {}
  DummyExpr(OPERATION op): opc(op) { assert(opc==OP__VOID || opc==OP__MAX); }

  bool equalTo(const Expr& e,CmpInfo &ci) const { return op() == e.op(); }
  bool lessThan(const Expr& e) const { return op() < e.op(); }

  Expr* clone() const { return new DummyExpr(opc); }
  void out(ostream& os,QrArray &q,OPERATION upper_op=OP__VOID) const
    { os << "Dummy"; };
  Expr* p_all_subst(const Term &t) const { assert(false); return 0; }
  Expr* q2t(const Term &t) const { assert(false); return 0; }
  Expr* t2q(const Term &t) const { assert(false); return 0; }
  bool check_qt(const Expr &e,TermVec &tv) const { assert(false); return 0; }
  bool unify(const Expr&,LocalSubst &ls,GlobalSubst &gs) const
    { assert(false); return false; };
  //bool is_super(const Expr &e,LocalSubst &ls) const
  //  { assert(false); return false; };
  Expr* p_subst(const LocalSubst &ls,const GlobalSubst &gs) const
    { return clone(); }
  int max_lmeta() const { assert(false); return 0; }
  int redo_lmeta(MetaMap &m,int base) const { assert(false); return 0; }
  Expr* l2g(LocalSubst&) const { assert(false); return 0; }
  OPERATION op() const {return opc;}

  ~DummyExpr() {}
};

///-------------------------------------------------------------------------
class UnaryExpr: public Expr {
  Expr *ex;

public:
  UnaryExpr(const UnaryExpr &e): ex(e.ex->clone()) {}
  UnaryExpr(const Expr &e): ex(e.clone()) {}
  UnaryExpr(Expr *e): ex(e) {}

  virtual bool equalTo(const Expr& e,CmpInfo &ci) const;
  virtual bool lessThan(const Expr& expr) const;

  const Expr& expr() const { return *ex; }

  bool check_qt(const Expr &e,TermVec &tv) const
  {
    if(op()!=e.op()) return false;
    const UnaryExpr &ue = dynamic_cast<const UnaryExpr &>(e);
    return ex->check_qt(*ue.ex,tv);
  }

  bool unify(const Expr &e,LocalSubst &ls,GlobalSubst &gs) const;
  //bool is_super(const Expr &e,LocalSubst &ls) const;
  int t_depth() const { return ex->t_depth(); }
  int max_lmeta() const { return ex->max_lmeta(); }
  int redo_lmeta(MetaMap &m,int base) const { return ex->redo_lmeta(m,base); }

  ~UnaryExpr() { delete ex; }
};

///-------------------------------------------------------------------------
class Quantor: public UnaryExpr {
  string qvar;

//protected:
public:
  Quantor(const Quantor &q): qvar(q.qvar), UnaryExpr(q.expr()) {}
  Quantor(const string& var,const Expr &e): qvar(var), UnaryExpr(e) {}
  Quantor(const string& var,Expr *e): qvar(var), UnaryExpr(e) {}

  bool equalTo(const Expr& expr,CmpInfo &ci) const;
  bool lessThan(const Expr& expr) const;

  Expr* q2meta(size_t vars) const
  {
    return expr().q2t(LocalMeta(expr().max_lmeta()+1,vars));
  }

  Expr* pt_subst(const Term &t) const { return expr().q2t(t); }

  const string& var() const { return qvar; }
};

///-------------------------------------------------------------------------
class ExQuantor: public Quantor {
public:
  ExQuantor(const Quantor &q): Quantor(q) {}
  ExQuantor(const string& var,const Expr &e): Quantor(var,e) {}
  ExQuantor(const string& var,Expr *e): Quantor(var,e) {}

  ExQuantor* clone() const { return new ExQuantor(*this); }

  void out(ostream& os,QrArray &q,OPERATION upper_op=OP__VOID) const;
  OPERATION op() const { return OP_EX; }
  Expr* p_all_subst(const Term &t) const
  {
    return new ExQuantor(var(),expr().p_all_subst(t));
  }
  Expr* q2t(const Term &t) const { return new ExQuantor(var(),expr().q2t(t)); }
  Expr* t2q(const Term &t) const { return new ExQuantor(var(),expr().t2q(t)); }

  Expr* p_subst(const LocalSubst &ls,const GlobalSubst &gs) const
  {
    return new ExQuantor(var(),expr().p_subst(ls,gs));
  }

  Expr* l2g(LocalSubst &ls) const
  {
    return new ExQuantor(var(),expr().l2g(ls));
  }
};

///-------------------------------------------------------------------------
class ForQuantor: public Quantor {
public:
  ForQuantor(const Quantor &q): Quantor(q) {}
  ForQuantor(const string& var,const Expr &e): Quantor(var,e) {}
  ForQuantor(const string& var,Expr *e): Quantor(var,e) {}

  ForQuantor* clone() const { return new ForQuantor(*this); }

  void out(ostream& os,QrArray &q,OPERATION upper_op=OP__VOID) const;
  OPERATION op() const { return OP_FOR; }
  Expr* p_all_subst(const Term &t) const
  {
    return new ForQuantor(var(),expr().p_all_subst(t));
  }
  Expr* q2t(const Term &t) const { return new ForQuantor(var(),expr().q2t(t)); }
  Expr* t2q(const Term &t) const { return new ForQuantor(var(),expr().t2q(t)); }

  Expr* p_subst(const LocalSubst &ls,const GlobalSubst &gs) const
  {
    return new ForQuantor(var(),expr().p_subst(ls,gs));
  }

  Expr* l2g(LocalSubst &ls) const
  {
    return new ForQuantor(var(),expr().l2g(ls));
  }
};

///-------------------------------------------------------------------------
class Negation: public UnaryExpr {
public:
  Negation(const UnaryExpr &e): UnaryExpr(e) {}
  Negation(const Expr &e): UnaryExpr(e) {}
  Negation(Expr *e): UnaryExpr(e) {}

  Negation* clone() const { return new Negation(*this); }

  void out(ostream& os,QrArray &q,OPERATION upper_op=OP__VOID) const;
  OPERATION op() const { return OP_NOT; }
  Expr* p_all_subst(const Term &t) const
  {
    return new Negation(expr().p_all_subst(t));
  }
  Expr* q2t(const Term &t) const { return new Negation(expr().q2t(t)); }
  Expr* t2q(const Term &t) const { return new Negation(expr().t2q(t)); }
  Expr* p_subst(const LocalSubst &ls,const GlobalSubst &gs) const
  {
    return new Negation(expr().p_subst(ls,gs));
  }

  Expr* l2g(LocalSubst &ls) const
  {
    return new Negation(expr().l2g(ls));
  }
};

///-------------------------------------------------------------------------
class BinaryExpr: public Expr {
  Expr *l,*r;

public:
  BinaryExpr(const BinaryExpr &e): l(e.l->clone()), r(e.r->clone()) {}
  BinaryExpr(const Expr &L,const Expr &R): l(L.clone()), r(R.clone()) {}
  BinaryExpr(Expr *L,Expr *R): l(L), r(R) {}

  bool equalTo(const Expr& e,CmpInfo &ci) const;
  bool lessThan(const Expr& expr) const;

  const Expr& left() const {return *l;}
  const Expr& right() const {return *r;}

  bool unify(const Expr &e,LocalSubst &ls,GlobalSubst &gs) const;
  //bool is_super(const Expr &e,LocalSubst &ls) const;

  bool check_qt(const Expr &e,TermVec &tv) const
  {
    if(op()!=e.op()) return false;
    const BinaryExpr &be = dynamic_cast<const BinaryExpr &>(e);
    return l->check_qt(*be.l,tv) && r->check_qt(*be.r,tv);
  }

  int t_depth() const { return max(l->t_depth(),r->t_depth()); }
  int max_lmeta() const { return max(l->max_lmeta(),r->max_lmeta()); }

  int redo_lmeta(MetaMap &m,int base) const
  {
    return max(l->redo_lmeta(m,base),r->redo_lmeta(m,base));
  }

  ~BinaryExpr() { delete l; delete r; }
};

///-------------------------------------------------------------------------
class ImpliesExpr: public BinaryExpr {
public:
  ImpliesExpr(const BinaryExpr &e): BinaryExpr(e) {}
  ImpliesExpr(const Expr &l,const Expr &r): BinaryExpr(l,r) {}
  ImpliesExpr(Expr *l,Expr *r): BinaryExpr(l,r) {}

  ImpliesExpr* clone() const { return new ImpliesExpr(*this); }

  void out(ostream& os,QrArray &q,OPERATION upper_op=OP__VOID) const;

  OPERATION op() const { return OP_IMPLIES; }
  Expr* p_all_subst(const Term &t) const
  {
    return new ImpliesExpr(left().p_all_subst(t),right().p_all_subst(t));
  }
  Expr* q2t(const Term &t) const
  {
    return new ImpliesExpr(left().q2t(t),right().q2t(t));
  }
  Expr* t2q(const Term &t) const
  {
    return new ImpliesExpr(left().t2q(t),right().t2q(t));
  }
  Expr* p_subst(const LocalSubst &ls,const GlobalSubst &gs) const
  {
    return new ImpliesExpr(left().p_subst(ls,gs),right().p_subst(ls,gs));
  }
  Expr* l2g(LocalSubst &ls) const
  {
    return new ImpliesExpr(left().l2g(ls),right().l2g(ls));
  }
};

///-------------------------------------------------------------------------
class OrExpr: public BinaryExpr {
public:
  OrExpr(const BinaryExpr &e): BinaryExpr(e) {}
  OrExpr(const Expr &l,const Expr &r): BinaryExpr(l,r) {}
  OrExpr(Expr *l,Expr *r): BinaryExpr(l,r) {}

  OrExpr* clone() const { return new OrExpr(*this); }

  void out(ostream& os,QrArray &q,OPERATION upper_op=OP__VOID) const;

  OPERATION op() const { return OP_OR; }
  Expr* p_all_subst(const Term &t) const
  {
    return new OrExpr(left().p_all_subst(t),right().p_all_subst(t));
  }
  Expr* q2t(const Term &t) const
  {
    return new OrExpr(left().q2t(t),right().q2t(t));
  }
  Expr* t2q(const Term &t) const
  {
    return new OrExpr(left().t2q(t),right().t2q(t));
  }
  Expr* p_subst(const LocalSubst &ls,const GlobalSubst &gs) const
  {
    return new OrExpr(left().p_subst(ls,gs),right().p_subst(ls,gs));
  }
  Expr* l2g(LocalSubst &ls) const
  {
    return new OrExpr(left().l2g(ls),right().l2g(ls));
  }
};

///-------------------------------------------------------------------------
class AndExpr: public BinaryExpr {
public:
  AndExpr(const BinaryExpr &e): BinaryExpr(e) {}
  AndExpr(const Expr &l,const Expr &r): BinaryExpr(l,r) {}
  AndExpr(Expr *l,Expr *r): BinaryExpr(l,r) {}

  AndExpr* clone() const { return new AndExpr(*this); }

  void out(ostream& os,QrArray &q,OPERATION upper_op=OP__VOID) const;

  OPERATION op() const { return OP_AND; }
  Expr* p_all_subst(const Term &t) const
  {
    return new AndExpr(left().p_all_subst(t),right().p_all_subst(t));
  }
  Expr* q2t(const Term &t) const
  {
    return new AndExpr(left().q2t(t),right().q2t(t));
  }
  Expr* t2q(const Term &t) const
  {
    return new AndExpr(left().t2q(t),right().t2q(t));
  }
  Expr* p_subst(const LocalSubst &ls,const GlobalSubst &gs) const
  {
    return new AndExpr(left().p_subst(ls,gs),right().p_subst(ls,gs));
  }
  Expr* l2g(LocalSubst &ls) const
  {
    return new AndExpr(left().l2g(ls),right().l2g(ls));
  }
};

///-------------------------------------------------------------------------
class Constant: public Expr {
public:

  bool equalTo(const Expr& e,CmpInfo &ci) const { return op()==e.op(); }
  bool lessThan(const Expr& e) const { return op()<e.op(); }

  Expr* p_all_subst(const Term &t) const { return clone(); }
  Expr* q2t(const Term &t) const { return clone(); }
  Expr* t2q(const Term &t) const { return clone(); }
  Expr* p_subst(const LocalSubst &ls,const GlobalSubst &gs) const
  {
    return clone();
  };

  Expr* l2g(LocalSubst &ls) const { return clone(); }

  bool unify(const Expr &e,LocalSubst &ls,GlobalSubst &gs) const
  {
    return op()==e.op();
  }

  //bool is_super(const Expr &e,LocalSubst &ls) const
  //{
  //  return op()==e.op();
  //}

  bool check_qt(const Expr &e,TermVec &tv) const { return op()==e.op(); }
  int max_lmeta() const { return -1; }
  int redo_lmeta(MetaMap &m,int base) const { return base; }
};

///-------------------------------------------------------------------------
class TrueConst: public Constant {
public:
  TrueConst* clone() const { return new TrueConst(); }
  void out(ostream& os,QrArray &q,OPERATION upper_op=OP__VOID) const { os << "true"; }
  OPERATION op() const { return OP_TRUE; }
  /* TODO : use "const OP op" as public member instead */
};

///-------------------------------------------------------------------------
class FalseConst: public Constant {
public:
  FalseConst* clone() const { return new FalseConst(); }
  void out(ostream& os,QrArray &q,OPERATION upper_op=OP__VOID) const { os << "false"; }
  OPERATION op() const { return OP_FALSE; }
};

///-------------------------------------------------------------------------
class Predicate: public Expr {
  PredIterator iter;
  TermList *terms;

protected:
  Predicate() {}

public:
  Predicate(const Predicate &pred)
  {
    iter = pred.iter;
    terms = new TermList(*pred.terms);
  }

  Predicate(const PredIterator &it,const TermList &t)
  {
    iter = it;
    terms = new TermList(t);
  }

  Predicate(const PredIterator &it,TermList *t)
  {
    iter = it;
    terms = t;
  }

  bool equalTo(const Expr& e,CmpInfo &ci) const;
  bool lessThan(const Expr& expr) const;

  virtual Predicate* clone() const { return new Predicate(*this); }

  virtual void out(ostream& os,QrArray &q,OPERATION upper_op=OP__VOID) const;
  virtual OPERATION op() const { return OP_PRED; }
  virtual Expr* p_all_subst(const Term &t) const
  {
    return new Predicate(iter,terms->p_all_subst(t));
  }
  virtual Expr* q2t(const Term &t) const { return new Predicate(iter,terms->q2t(t)); }
  virtual Expr* t2q(const Term &t) const { return new Predicate(iter,terms->t2q(t)); }
  virtual Expr* p_subst(const LocalSubst &ls,const GlobalSubst &gs) const
  {
    return new Predicate(iter,terms->p_subst(ls,gs));
  }

  bool unify(const Expr &e,LocalSubst &ls,GlobalSubst &gs) const;
  //bool is_super(const Expr &e,LocalSubst &ls) const;

  bool check_qt(const Expr &e,TermVec &tv) const
  {
    if(op()!=e.op()) return false;
    const Predicate &p = dynamic_cast<const Predicate &>(e);
    return iter==p.iter && terms->check_qt(*p.terms,tv);
  }

  int t_depth() const { return terms->t_depth(); }
  int max_lmeta() const { return terms->max_lmeta(); }

  int redo_lmeta(MetaMap &m,int base) const
  {
    return terms->redo_lmeta(m,base);
  }

  Expr* l2g(LocalSubst &ls) const
  {
    return new Predicate(iter,terms->l2g(ls));
  }

  ~Predicate() { delete terms; }

  friend class Equality;
};

///-------------------------------------------------------------------------
class Equality: public Predicate {
public:
  Equality(const Equality &eq)
  {
    iter = 0;//pred_array.end();
    terms = new TermList(*eq.terms);
  }

  Equality(const Term &t1,const Term &t2)
  {
    iter = 0;//pred_array.end();
    terms = new TermList();
    terms->push_back(t1.clone());
    terms->push_back(t2.clone());
  }

  Equality(Term *t1,Term *t2)
  {
    iter = 0;//pred_array.end();
    terms = new TermList();
    terms->push_back(t1);
    terms->push_back(t2);
  }

  Equality(TermList *t)
  {
    iter = 0;//pred_array.end();
    assert(t->size()==2);
    terms = t;
  }

  bool equalTo(const Expr& e,CmpInfo &ci) const;
  bool lessThan(const Expr& expr) const;

  Equality* clone() const { return new Equality(*this); }

  const Term& left() const { return *terms->front(); }
  const Term& right() const { return *terms->back(); }

  void out(ostream& os,QrArray &q,OPERATION upper_op=OP__VOID) const;
  OPERATION op() const { return OP_EQUALITY; }
  Expr* p_all_subst(const Term &t) const
  {
    return new Equality(terms->p_all_subst(t));
  }
  Expr* q2t(const Term &t) const { return new Equality(terms->q2t(t)); }
  Expr* t2q(const Term &t) const { return new Equality(terms->t2q(t)); }
  Expr* p_subst(const LocalSubst &ls,const GlobalSubst &gs) const
  {
    return new Equality(terms->p_subst(ls,gs));
  }
};

///-------------------------------------------------------------------------
template<class T> struct EPtrLess {
  bool operator ()(const T* x,const T* y) { return *x < *y; }
};

///-------------------------------------------------------------------------
inline ostream& operator <<(ostream& os,const Expr& e)
{
  e.out(os,QrArray());
  return os;
}

//---------------------------------------------------------------------------
template<class QuantorType>    /* TODO : может стоит сделать членами ForQuantor, ExQuantor */
  const Expr* remove_prefix(const Expr *expr,size_t vars)
{
  const Expr *e = expr;
  const QuantorType *fq = dynamic_cast<const QuantorType *>(expr);
  while(fq) {
    e = fq->q2meta(vars);
    if(fq != expr) delete fq;
    fq = dynamic_cast<const QuantorType *>(e);
  }
  return e;
}


#endif
