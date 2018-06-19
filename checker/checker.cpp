#include <iostream>
#include "checker.h"

bool Checker::find(const Expr &ex,const ExprItList &il) const
{
  ExprItList::const_iterator i, end = il.end();

  for(i = il.begin(); i != end; ++i)
    if(ex == *(*i)->second) return true;

  return false;
}

bool Checker::check(const Expr *ex,const ExprItList &il) const
{
  const AndExpr *ae;
  const OrExpr *oe;
  const ImpliesExpr *ie, *ie2;

  Term::reset_meta(0);    /* TODO : why reset_meta()? */

  ExprItList::const_iterator i, end = il.end();

  // true
  if(ex->op()==Expr::OP_TRUE) return true;

  // ex x st true
  const ExQuantor *exq = dynamic_cast<const ExQuantor*>(ex);
  while(exq) {
    if(exq->expr().op()==Expr::OP_TRUE) return true;
    exq = dynamic_cast<const ExQuantor*>(&exq->expr());
  }

  // false / F
  if(find(FalseConst(),il)) return true;

  // p[x] / ex x st p[x]
  for(i = il.begin(); i != end; ++i) {
    const Expr *e = ex->clone();
    while((exq = dynamic_cast<const ExQuantor*>(e)) != 0) {
      e = exq->pt_subst(GlobalMeta(Term::new_meta(),vars.size()));
      delete exq;
      if(e->unify(*(*i)->second,LocalSubst(),GlobalSubst())) break;
    }
    delete e;

    if(exq!=0) return true;
  }

  // for x holds p[x] / p[x]
  for(i = il.begin(); i != end; ++i) {
    const ForQuantor *fq;
    const Expr *e = (*i)->second->clone();
    while((fq = dynamic_cast<const ForQuantor*>(e)) != 0) {
      e = fq->pt_subst(GlobalMeta(Term::new_meta(),vars.size()));
      delete fq;
      if(e->unify(*ex,LocalSubst(),GlobalSubst())) break;
    }
    delete e;

    if(fq!=0) return true;
  }

  // p[t1],t1=t2 / p[t2]
  for(i = il.begin(); i != end; ++i) {
    const Equality *eq = dynamic_cast<const Equality*>((*i)->second);
    if(eq != 0) {
      Expr *e = ex->t2q(eq->right());
      Expr *e1 = e->q2t(eq->left());
      delete e;

      bool b = false;

      for(ExprItList::const_iterator j = il.begin(); j != end && !b; ++j) {
        e = (*j)->second->t2q(eq->right());
        Expr *e2 = e->q2t(eq->left());
        delete e;

        b = *e1 == *e2;

        delete e2;
      }

      delete e1;

      if(b) return true;
    }
  }

  // t = t
  const Equality *eq = dynamic_cast<const Equality*>(ex);
  if(eq != 0 && eq->left()==eq->right()) return true;

  // t1 = t2 / t(t1) = t(t2)
  if(ex->op() == Expr::OP_EQUALITY) {

    for(i = il.begin(); i != end; ++i) {
      eq = dynamic_cast<const Equality*>((*i)->second);
      if(eq != 0) {

        Expr *e = ex->t2q(eq->left());
        Expr *e2 = e->q2t(eq->right());
        delete e;

        const Equality &equ = dynamic_cast<const Equality&>(*e2);

        if(equ.left() == equ.right()) {
          delete e2;
          return true;
        } else
          delete e2;

      }
    }
  }

  // F / F
  if(find(*ex,il)) return true;

  // F,G / F & G
  ae = dynamic_cast<const AndExpr *>(ex);
  if(ae != 0 && find(ae->left(),il) && find(ae->right(),il)) return true;

  // F / F or G, G or F
  oe = dynamic_cast<const OrExpr *>(ex);
  if(oe != 0 && (find(oe->left(),il) || find(oe->right(),il))) return true;

  // F and G / F,G
  for(i = il.begin(); i != end; ++i) {
    ae = dynamic_cast<const AndExpr *>((*i)->second);
    if(ae != 0 && (*ex == ae->left() || *ex == ae->right())) return true;
  }

  // F implies G, F / G
  for(i = il.begin(); i != end; ++i) {
    ie = dynamic_cast<const ImpliesExpr *>((*i)->second);
    if(ie != 0 && *ex == ie->right() && find(ie->left(),il)) return true;
  }

  // not F or G, F / G
  //for(i = il.begin(); i != end; ++i) {
  //  oe = dynamic_cast<const OrExpr *>((*i)->second);
  //  if(oe != 0 && *ex == oe->right()) {
  //    ie = dynamic_cast<const ImpliesExpr *>(&oe->left());
  //    if(ie != 0 && ie->right().op()==Expr::OP_FALSE && find(ie->left(),il)) return true;
  //  }
  //}

  // F or G, not F / G
  for(i = il.begin(); i != end; ++i) {
    oe = dynamic_cast<const OrExpr *>((*i)->second);
    if(oe != 0 &&
       *ex == oe->right() &&
       find(ImpliesExpr(oe->left(),FalseConst()),il)) return true;
  }

  // F iff G / F implies G, G implies F
  //ie = dynamic_cast<const ImpliesExpr *>(ex);
  //if(ie != 0)
  //  for(i = il.begin(); i != end; ++i) {
  //    iffe = dynamic_cast<const IFFExpr *>((*i)->second);
  //    if(iffe != 0 &&
  //       (ie->left() == iffe->left() && ie->right() == iffe->right() ||
  //       ie->left() == iffe->right() && ie->right() == iffe->left())) return true;
  //  }

  // F implies G, G implies F / G iff F
  //iffe = dynamic_cast<const IFFExpr *>(ex);
  //if(iffe != 0)
  //  for(i = il.begin(); i != end; ++i) {
  //    ie = dynamic_cast<const ImpliesExpr *>((*i)->second);
  //    if(ie != 0 &&
  //       ie->left() == iffe->left() && ie->right() == iffe->right())
  //      for(ExprItList::const_iterator j = il.begin(); j != end; ++j) {
  //        ie2 = dynamic_cast<const ImpliesExpr *>((*j)->second);
  //        if(ie2 != 0 &&
  //           ie2->left() == iffe->right() && ie2->right() == iffe->left()) return true;
  //      }
  //  }

  // A or B, A implies C, B implies C / C
  for(i = il.begin(); i != end; ++i) {
    ie = dynamic_cast<const ImpliesExpr *>((*i)->second);
    if(ie != 0 && ie->right() == *ex)
      for(ExprItList::const_iterator j = il.begin(); j != end; ++j) {
        ie2 = dynamic_cast<const ImpliesExpr *>((*j)->second);
        if(ie2 != 0 && ie2->right() == *ex && find(OrExpr(ie->left(),ie2->left()),il)) return true;
      }
  }

  return false;
}

bool Checker::add_label(ExprItList &il,const string &label) const
{
  ExprArray::iterator i = exprs.find(label);
  if(i != exprs.end()) {
    il.push_back(i);
    return true;
  }

  return false;
}

bool Checker::assumption(const Expr *ex)
{
  thesis_vec.push_back(ThesisItem(IT_ASSUME,ex));

  if(thesis != 0) {
    const ImpliesExpr *ie = dynamic_cast<const ImpliesExpr *>(thesis);

    if(ie != 0 && ie->left() == *ex) {
      const Expr *t = thesis;
      thesis = ie->right().clone();
      delete t;
    } else {
#ifndef INTUITIONISTIC
      ie = dynamic_cast<const ImpliesExpr *>(ex);
      if(ie != 0 && ie->right().op() == Expr::OP_FALSE && ie->left() == *thesis) {
        delete thesis;
        thesis = new FalseConst();
      } else
#endif
        return false;
    }
  }

  return true;
}

bool Checker::exist_assumption(const VarItList &vil,const Expr *ex)
{
  VarItList::const_reverse_iterator i, end = vil.rend();

  Expr *ne = ex->clone();
  for(i = vil.rbegin(); i != end; ++i) {
    Expr *t = ne;

    ne = new ExQuantor("_",ne->t2q(Variable(*i,0)));

    delete t;
  }

  thesis_vec.push_back(ThesisItem(IT_ASSUME,ne));

  to_delete.push_back(ne);

  if(thesis == 0) return true;

  const ImpliesExpr *ie = dynamic_cast<const ImpliesExpr *>(thesis);
  if(ie == 0 || !(ie->left() == *ne)) return false;

  const Expr *t = thesis;
  thesis = ie->right().clone();
  delete t;

  return true;
}

bool Checker::conclusion(const Expr *ex)
{
  thesis_vec.push_back(ThesisItem(IT_THUS,ex));

  if(thesis != 0) {
    if(*ex == *thesis) {
      delete thesis;
      thesis = new TrueConst();
    } else {
      const AndExpr *ae = dynamic_cast<const AndExpr *>(thesis);
      if(ae != 0 && ae->left() == *ex) {
        const Expr *t = thesis;
        thesis = ae->right().clone();
        delete t;
      } else
        return false;
    }
  }

  return true;
}

bool Checker::generalization(size_t vi)
{
  thesis_vec.push_back(ThesisItem(IT_LET,0,vi));

  if(thesis != 0) {
    const ForQuantor *fq = dynamic_cast<const ForQuantor *>(thesis);

    if(fq == 0) return false;

    const Expr *t = thesis;
    thesis = fq->pt_subst(Variable(vi,0));
    delete t;
  }

  return true;
}

bool Checker::consideration(const VarItList &vil,const Expr *e,const ExprItList &il)
{                       /* TODO : optimize consideration verification */

  if(e->op()==Expr::OP_TRUE) return true;

  ExprItList::const_iterator ei, eend = il.end();

  for(ei = il.begin(); ei != eend; ++ei) {

    Expr *ex = (*ei)->second->clone();
    VarItList::const_iterator i, end = vil.end();

    for(i=vil.begin(); i!=end; ++i) {
      ExQuantor *exq = dynamic_cast<ExQuantor *>(ex);

      if(exq == 0) break;

      const Expr *t = ex;
      ex = exq->pt_subst(Variable(*i,0));
      delete t;
    }

    if(i==end && *ex==*e) {
      delete ex;
      return true;
    }

    delete ex;
  }

  return false;
}

bool Checker::add_variable(VarItList &il,const string &var)
{
  VarTable::iterator i = ::find(vars.begin(),vars.end(),var);

  if(i==vars.end()) {
    size_t n = vars.insert(vars.end(),var)-vars.begin();
    il.push_back(n);
    return true;
  } else {
    il.push_back(i-vars.begin());
    return false;
  }
}

bool Checker::exemplification(const Term *t)
{
  if(thesis == 0) return false;

  const ExQuantor * ex = dynamic_cast<const ExQuantor *>(thesis);

  if(ex == 0) return false;

  const Expr *e = thesis;

  thesis = ex->pt_subst(*t);

  delete e;

  return true;
}

bool Checker::add_formula(const string &label,const Expr *ex)
{
  if(!label.empty()) {
    ExprArray::iterator i = exprs.find(label);
    if(i==exprs.end()) {
      new_formulae.push_back(
        exprs.insert(pair<string,const Expr*>(label,ex)).first);
    } else {
      changes.push_back(pair<ExprArray::iterator,const Expr*>(i,i->second));
      i->second = ex;
    }
  } else
    to_delete.push_back(ex);

  return true;
}

Expr* Checker::calc_thesis(ThesisVec::const_iterator i) const
{
  if(i == thesis_vec.end()) return new TrueConst();

  switch(i->item_type) {
    case IT_ASSUME:
      return new ImpliesExpr(i->expr->clone(),calc_thesis(i+1));
    case IT_THUS:
      {
        ThesisVec::const_iterator j = i+1;
        if(j == thesis_vec.end())
          return i->expr->clone();
        else
          return new AndExpr(i->expr->clone(),calc_thesis(j));
      }
    case IT_LET:
      {
        Expr *e = calc_thesis(i+1);
        Expr *r = new ForQuantor(var_table[i->vi],e->t2q(Variable(i->vi,0)));
        delete e;
        return r;
      }
    default:
      assert(false);
      return 0;
  }

}

Expr* Checker::calc_thesis() const
{
  Expr *e = calc_thesis(thesis_vec.begin());
  size_t size = vars.size();
  bool err = false;

  for(size_t i=oldvars; i<size && !err; i++) {
    Expr *e_ = e->t2q(Variable(i,0));
    Expr *ne = e_->q2t(QVariable(0));
    delete e_;
    err = !(*e == *ne);
    delete ne;
  }

  if(!err) return e;

  delete e;
  return 0;
}


Checker::~Checker()
{
  delete thesis;

  {
    ChangesVec::reverse_iterator i, end = changes.rend();
    for(i = changes.rbegin(); i != end; ++i) {
      delete i->first->second;
      i->first->second = i->second;
    }
  }

  {
    ExprItList::iterator i, end = new_formulae.end();
    for(i = new_formulae.begin(); i != end; ++i) {
      delete (*i)->second;
      exprs.erase(*i);
    }
  }

  {
    vector<const Expr*>::iterator i, end = to_delete.end();
    for(i = to_delete.begin(); i != end; ++i) delete *i;
  }

  vars.resize(oldvars);

}
