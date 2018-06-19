//---------------------------------------------------------------------------

#include <typeinfo>
#include <iostream>

#pragma hdrstop

#include "expr.h"

//---------------------------------------------------------------------------

PredArray pred_array;

//---------------------------------------------------------------------------
bool UnaryExpr::equalTo(const Expr& e,CmpInfo &ci) const
{
  if(op()!=e.op()) return false;

  return ex->equalTo(dynamic_cast<const UnaryExpr&>(e).expr(),ci);
}

bool Quantor::equalTo(const Expr& e,CmpInfo &ci) const
{
  if(op()!=e.op()) return false;

  const Quantor& q = dynamic_cast<const Quantor&>(e);

  bool eq = expr().equalTo(q.expr(),ci);

  return eq;
}

bool BinaryExpr::equalTo(const Expr& e,CmpInfo &ci) const
{
  if(op()!=e.op()) return false;

  const BinaryExpr &expr = dynamic_cast<const BinaryExpr&>(e);

  return l->equalTo(*expr.l,ci) &&
         r->equalTo(*expr.r,ci);
}

bool Predicate::equalTo(const Expr& e,CmpInfo &ci) const
{
  if(op()!=e.op()) return false;

  const Predicate& p = dynamic_cast<const Predicate&>(e);

  if(iter != p.iter) return false;

  return terms->equalTo(*p.terms,ci);
}

bool Equality::equalTo(const Expr& e,CmpInfo &ci) const
{
  if(op()!=e.op()) return false;

  const Equality& p = dynamic_cast<const Equality&>(e);

  return terms->equalTo(*p.terms,ci);
}

//---------------------------------------------------------------------------
bool UnaryExpr::lessThan(const Expr& e) const
{
  if(op()<e.op()) return true;
  if(op()>e.op()) return false;

  return expr().lessThan(dynamic_cast<const UnaryExpr&>(e).expr());
}

bool Quantor::lessThan(const Expr& e) const
{
  if(op()<e.op()) return true;
  if(op()>e.op()) return false;

  return expr().lessThan(dynamic_cast<const Quantor&>(e).expr());
}

bool BinaryExpr::lessThan(const Expr& e) const
{
  if(op()<e.op()) return true;
  if(op()>e.op()) return false;

  const BinaryExpr &q = dynamic_cast<const BinaryExpr&>(e);

  if(left().lessThan(q.left())) return true;
  if(q.left().lessThan(left())) return false;

  return right().lessThan(q.right());
}

bool Predicate::lessThan(const Expr& e) const
{
  if(op()<e.op()) return true;
  if(op()>e.op()) return false;

  const Predicate& p = dynamic_cast<const Predicate&>(e);

  if(iter->first < p.iter->first) return true;
  if(iter->first > p.iter->first) return false;

  return terms->lessThan(*p.terms);
}

bool Equality::lessThan(const Expr& e) const
{
  if(op()<e.op()) return true;
  if(op()>e.op()) return false;

  const Equality& p = dynamic_cast<const Equality&>(e);

  return terms->lessThan(*p.terms);
}

//---------------------------------------------------------------------------
void ExQuantor::out(ostream& os,QrArray &q,OPERATION upper_op) const
{
  if(upper_op>=op()) os << '(';

  int n = 1;
  os << "ex " << var();
  q.push_back(var());

  const ExQuantor *f = this, *e;
  while(e=f,f=dynamic_cast<const ExQuantor*>(&f->expr()))
  {
    q.push_back(f->var());
    os << "," << f->var();
    n++;
  }

  os << " st "; e->expr().out(os,q,op());

  while(n--) q.pop_back();

  if(upper_op>=op()) os << ')';
}

void ForQuantor::out(ostream& os,QrArray &q,OPERATION upper_op) const
{
  if(upper_op>=op()) os << '(';

  int n = 1;
  os << "for " << var();
  q.push_back(var());

  const ForQuantor *f = this, *e;
  while(e=f,f=dynamic_cast<const ForQuantor*>(&f->expr()))
  {
    q.push_back(f->var());
    os << "," << f->var();
    n++;
  }

  os << " holds "; e->expr().out(os,q,op());

  while(n--) q.pop_back();
  
  if(upper_op>=op()) os << ')';
}

void ImpliesExpr::out(ostream& os,QrArray &q,OPERATION upper_op) const
{
  if(right().op()==OP_FALSE)
  {
    if(upper_op>=OP_NOT) os << '(';
    os << "not "; left().out(os,q,OP_NOT);
    if(upper_op>=OP_NOT) os << ')';
  } else {
    if(upper_op>=op()) os << '(';
    left().out(os,q,op()); os << " implies "; right().out(os,q,op());
    if(upper_op>=op()) os << ')';
  }
}

void OrExpr::out(ostream& os,QrArray &q,OPERATION upper_op) const
{
  if(upper_op>=op()) os << '(';
  left().out(os,q,op()); os << " or "; right().out(os,q,op());
  if(upper_op>=op()) os << ')';
}

void AndExpr::out(ostream& os,QrArray &q,OPERATION upper_op) const
{
  const ImpliesExpr *iel = dynamic_cast<const ImpliesExpr *>(&left()),
                    *ier = dynamic_cast<const ImpliesExpr *>(&right());

  if(iel && ier && iel->left()==ier->right() && iel->right()==ier->left())
  {
    if(upper_op>=OP_IFF) os << '(';
    iel->left().out(os,q,OP_IFF); os << " iff "; iel->right().out(os,q,OP_IFF);
    if(upper_op>=OP_IFF) os << ')';
  } else {
    if(upper_op>=op()) os << '(';
    left().out(os,q,op()); os << " & "; right().out(os,q,op());
    if(upper_op>=op()) os << ')';
  }
}

void Negation::out(ostream& os,QrArray &q,OPERATION upper_op) const
{
  if(upper_op>=op()) os << '(';
  os << "not "; expr().out(os,q,op());
  if(upper_op>=op()) os << ')';
}

void Predicate::out(ostream& os,QrArray &q,OPERATION upper_op) const
{
  os << iter->first << '[';
  terms->out(os,q);
  os << ']';
}

void Equality::out(ostream& os,QrArray &q,OPERATION upper_op) const
{
  terms->front()->out(os,q); os << " = "; terms->back()->out(os,q);
}

//---------------------------------------------------------------------------
bool UnaryExpr::unify(const Expr &e,LocalSubst &ls,GlobalSubst &gs) const
{
  if(op()!=e.op()) return false;

  const UnaryExpr &ue = dynamic_cast<const UnaryExpr &>(e);

  return ex->unify(*ue.ex,ls,gs);
}

bool BinaryExpr::unify(const Expr &e,LocalSubst &ls,GlobalSubst &gs) const
{
  if(op()!=e.op()) return false;

  const BinaryExpr &be = dynamic_cast<const BinaryExpr &>(e);

  return l->unify(*be.l,ls,gs) && r->unify(*be.r,ls,gs);
}

bool Predicate::unify(const Expr &e,LocalSubst &ls,GlobalSubst &gs) const
{
  const Predicate *p = dynamic_cast<const Predicate *>(&e);

  return p!=0 && iter == p->iter && terms->unify(*p->terms,ls,gs);
}

//---------------------------------------------------------------------------
bool Expr::is_super(const Expr &e) const
{
  LocalSubst ls;
  GlobalSubst gs;

  if(!unify(e,ls,gs) || !gs.empty()) return false;

  GlobalSubst::iterator i = gs.begin(), end = gs.end();
  for(; i != end; ++i)
    if(i->second->contains_gmeta()) return false;

  Expr *te = p_subst(ls,gs);

  bool b = e == *te;

  delete te;

  return b;
}

//---------------------------------------------------------------------------
/*
bool UnaryExpr::is_super(const Expr &e,LocalSubst &ls) const
{
  if(op()!=e.op()) return false;

  const UnaryExpr &ue = dynamic_cast<const UnaryExpr &>(e);

  return ex->is_super(*ue.ex,ls);
}

bool BinaryExpr::is_super(const Expr &e,LocalSubst &ls) const
{
  if(op()!=e.op()) return false;

  const BinaryExpr &be = dynamic_cast<const BinaryExpr &>(e);

  return l->is_super(*be.l,ls) && r->is_super(*be.r,ls);
}

bool Predicate::is_super(const Expr &e,LocalSubst &ls) const
{
  const Predicate *p = dynamic_cast<const Predicate *>(&e);

  return p!=0 && iter == p->iter && terms->is_super(*p->terms,ls);
}
*/

