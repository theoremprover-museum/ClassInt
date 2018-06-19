
#include <iostream>

#pragma hdrstop

#include "term.h"


//---------------------------------------------------------------------------

VarTable var_table;
FuncArray func_array;
int Term::max_depth = -1;

unsigned long Term::meta_number = 0;
unsigned long Term::skolem_number;

//---------------------------------------------------------------------------
size_t new_temp_var(size_t vars)
{
  if(vars == var_table.size()) {
    int i = 0;

    while(true) {
      string name = string(++i,'t');
      if(find(var_table.begin(),var_table.end(),name) == var_table.end()) {
        var_table.push_back(name);
        break;
      }
    }

  }

  return vars;
}

//---------------------------------------------------------------------------
size_t find_func(const string &name)
{
  size_t end = func_array.size();
  for(size_t i=0; i<end; i++)
    if(func_array[i].name==name) return i;
  return end;
}

//---------------------------------------------------------------------------
/*
size_t new_temp_func(size_t funcs,size_t arity)
{
  if(funcs == func_array.size()) {
    int i = 0;
    string name;

    do
      name = string(++i,'f');
    while(find_func(name)!=func_array.size());

    func_array.push_back(func_struct(name,arity));
  }

  return funcs;
}*/

//---------------------------------------------------------------------------
bool Func::equalTo(const Term& t,CmpInfo &ci) const
{
  const Func* f = dynamic_cast<const Func*>(&t);

  if(f==0 || f_index != f->f_index) return false;

  return terms->equalTo(*f->terms,ci);
}

bool TermList::equalTo(const TermList& t,CmpInfo &ci) const
{
  TermList::const_iterator i1 = begin(), end1 = end(),
                           i2 = t.begin(), end2 = t.end();

  while(i1!=end1 && i2!=end2 && (*i1)->equalTo(**i2,ci)) ++i1, ++i2;

  return i1==end1 && i2==end2;
}

bool Variable::equalTo(const Term& t,CmpInfo &ci) const
{
  if(type!=t.type) return false;

  return v_index == dynamic_cast<const Variable&>(t).v_index;
}

bool QVariable::equalTo(const Term& t,CmpInfo &ci) const
{
  if(type!=t.type) return false;

  return q_pos == dynamic_cast<const QVariable&>(t).q_pos;
}

bool GlobalMeta::equalTo(const Term& t,CmpInfo &ci) const
{
  if(type!=t.type) return false;

  const GlobalMeta& gm = dynamic_cast<const GlobalMeta&>(t);

  if(!ci.cmpgm) return n == gm.n;

  if(vars != gm.vars) return false;

  MetaMap::iterator i = ci.gmm.find(n);

  if(i == ci.gmm.end()) {
    ci.gmm.insert(pair<int,int>(n,gm.n));
    return true;
  } else {
    return i->second == gm.n;
  }
}

bool LocalMeta::equalTo(const Term& t,CmpInfo &ci) const
{
  if(type!=t.type) return false;

  const LocalMeta& lm = dynamic_cast<const LocalMeta&>(t);

  if(vars != lm.vars) return false;

  MetaMap::iterator i = ci.lmm.find(n);

  if(i == ci.lmm.end()) {
    ci.lmm.insert(pair<int,int>(n,lm.n));
    return true;
  } else {
    return i->second == lm.n;
  }
}

//---------------------------------------------------------------------------
bool Func::lessThan(const Term& t) const
{
  //if(t.type==Term::TERM_GLOBAL_META ||
  //   t.type==Term::TERM_LOCAL_META) return false;

  if(type<t.type) return true;
  if(type>t.type) return false;

  const Func& f = dynamic_cast<const Func&>(t);

  if(f_index < f.f_index) return true;
  if(f_index > f.f_index) return false;

  return terms->lessThan(*f.terms);
}

bool TermList::lessThan(const TermList& t) const
{
  TermList::const_iterator i1 = begin(), end1 = end(),
                           i2 = t.begin(), end2 = t.end();

  while(i1!=end1 && i2!=end2) {

    if((*i1)->lessThan(**i2)) return true;
    if((*i2)->lessThan(**i1)) return false;

    ++i1, ++i2;
  }

  return i1==end1 && i2!=end2;
}

bool Variable::lessThan(const Term& t) const
{
  //if(t.type==Term::TERM_GLOBAL_META ||
  //   t.type==Term::TERM_LOCAL_META) return false;

  if(type<t.type) return true;
  if(type>t.type) return false;

  const Variable& v = dynamic_cast<const Variable&>(t);

  return v_index < v.v_index;
}

bool QVariable::lessThan(const Term& t) const
{
  //if(t.type==Term::TERM_GLOBAL_META ||
  //   t.type==Term::TERM_LOCAL_META) return false;

  if(type<t.type) return true;
  if(type>t.type) return false;

  const QVariable& v = dynamic_cast<const QVariable&>(t);

  return q_pos < v.q_pos;
}

bool GlobalMeta::lessThan(const Term& t) const
{
  if(type<t.type) return true;
  if(type>t.type) return false;

  const GlobalMeta& m = dynamic_cast<const GlobalMeta&>(t);

  return n < m.n;
}

bool LocalMeta::lessThan(const Term& t) const
{
  if(type<t.type) return true;
  if(type>t.type) return false;

  const LocalMeta& m = dynamic_cast<const LocalMeta&>(t);

  return n < m.n;
}

//---------------------------------------------------------------------------
void Func::out(ostream& os,QrArray &q) const
{
  if(f_index<func_array.size())
    os << func_array[f_index].name;
  else
    os << "Skolem" << f_index;
  os << '(';
  terms->out(os,q);
  os << ')';
}

void TermList::out(ostream& os,QrArray &q) const
{
  TermList::const_iterator i = begin(), endlist = end();

  while(i!=endlist) {
    (*i)->out(os,q);
    if(++i != endlist) os << ',';
  }
}

//---------------------------------------------------------------------------
bool Func::unify(const Term &t,LocalSubst &ls,GlobalSubst &gs) const
{
  if(t.type==TERM_GLOBAL_META) {
    const Term *term = dynamic_cast<const GlobalMeta &>(t).find_subst(gs);
    if(term) return unify(*term,ls,gs);
  }

  if(t.type==TERM_LOCAL_META) {
    const Term *term = dynamic_cast<const LocalMeta &>(t).find_subst(ls);
    if(term) return unify(*term,ls,gs);
  }

  switch(t.type) {
    case TERM_FUNC: {
      const Func &f = dynamic_cast<const Func &>(t);
      return f_index == f.f_index && terms->unify(*f.terms,ls,gs);
    }
    case TERM_VAR:
    case TERM_QVAR:
      return false;
    case TERM_GLOBAL_META: {
      const GlobalMeta &gm = dynamic_cast<const GlobalMeta &>(t);

      if( contains_qvar() ||
          contains_meta(gm,ls,gs) ||
          gm.max_vars()<max_vars() )
        return false;
      else {
        gs[gm] = this;
        return true;
      }
    }
    case TERM_LOCAL_META: {
      const LocalMeta &lm = dynamic_cast<const LocalMeta &>(t);

      if( contains_qvar() ||
          contains_meta(lm,ls,gs) /*||
          lm.max_vars()<max_vars()*/ )
        return false;
      else {
        ls[lm] = this;
        return true;
      }
    }
  }

  return false;
}

bool Variable::unify(const Term &t,LocalSubst &ls,GlobalSubst &gs) const
{
  if(t.type==TERM_GLOBAL_META) {
    const Term *term = dynamic_cast<const GlobalMeta &>(t).find_subst(gs);
    if(term) return unify(*term,ls,gs);
  }

  if(t.type==TERM_LOCAL_META) {
    const Term *term = dynamic_cast<const LocalMeta &>(t).find_subst(ls);
    if(term) return unify(*term,ls,gs);
  }

  switch(t.type) {
    case TERM_FUNC:
    case TERM_QVAR:
      return false;

    case TERM_VAR:
      return v_index == dynamic_cast<const Variable &>(t).v_index;

    case TERM_LOCAL_META: {
      const LocalMeta &lm = dynamic_cast<const LocalMeta &>(t);

      if(true /*|| lm.max_vars()>=max_vars()*/) {
        ls[lm] = this;
        return true;
      } else {
        return false;
      }
    }

    case TERM_GLOBAL_META: {
      const GlobalMeta &gm = dynamic_cast<const GlobalMeta &>(t);

      if(gm.max_vars()>=max_vars()) {
        gs[gm] = this;
        return true;
      } else {
        return false;
      }
    }
  }

  return false;
}

bool QVariable::unify(const Term &t,LocalSubst &ls,GlobalSubst &gs) const
{
  if(t.type==TERM_QVAR)
    return q_pos == dynamic_cast<const QVariable &>(t).q_pos;
  else
    return false;
}

bool LocalMeta::unify(const Term &t,LocalSubst &ls,GlobalSubst &gs) const
{
  const Term *term;

  term = find_subst(ls);
  if(term) return term->unify(t,ls,gs);

  if(t.type==TERM_GLOBAL_META) {
    term = dynamic_cast<const GlobalMeta &>(t).find_subst(gs);
    if(term) return unify(*term,ls,gs);
  }

  if(t.type==TERM_LOCAL_META) {
    term = dynamic_cast<const LocalMeta &>(t).find_subst(ls);
    if(term) return unify(*term,ls,gs);
  }

  switch(t.type) {
    case TERM_FUNC:
      if( t.contains_qvar() ||
          t.contains_meta(*this,ls,gs) /*||
          max_vars()<t.max_vars()*/ )
        return false;
      else {
        ls[*this] = &t;
        return true;
      }

    case TERM_VAR:
      if(true /*|| max_vars()>=t.max_vars()*/) {
        ls[*this] = &t;
        return true;
      } else
        return false;

    case TERM_QVAR:
      return false;

    case TERM_LOCAL_META: {
      const LocalMeta &lm = dynamic_cast<const LocalMeta &>(t);

      if(n!=lm.n)
        if(true /*|| vars>=lm.vars*/)
          ls[*this] = &t;
        else
          ls[lm] = this;

      return true;
    }

    case TERM_GLOBAL_META: {
      const GlobalMeta &gm = dynamic_cast<const GlobalMeta &>(t);

      if(true /*|| vars>=gm.vars*/)
        ls[*this] = &t;
      else
        gs[gm] = this;

      return true;
    }
  }

  return false;
}

bool GlobalMeta::unify(const Term &t,LocalSubst &ls,GlobalSubst &gs) const
{
  const Term *term;

  term = find_subst(gs);
  if(term) return term->unify(t,ls,gs);

  if(t.type==TERM_GLOBAL_META) {
    term = dynamic_cast<const GlobalMeta &>(t).find_subst(gs);
    if(term) return unify(*term,ls,gs);
  }

  if(t.type==TERM_LOCAL_META) {
    term = dynamic_cast<const LocalMeta &>(t).find_subst(ls);
    if(term) return unify(*term,ls,gs);
  }

  switch(t.type) {
    case TERM_FUNC:
      if( t.contains_qvar() ||
          t.contains_meta(*this,ls,gs) ||
          max_vars()<t.max_vars() )
        return false;
      else {
        gs[*this] = &t;
        return true;
      }

    case TERM_VAR:
      if(max_vars()>=t.max_vars()) {
        gs[*this] = &t;
        return true;
      } else
        return false;

    case TERM_QVAR:
      return false;

    case TERM_LOCAL_META: {
      const LocalMeta &lm = dynamic_cast<const LocalMeta &>(t);

      if(true || vars>lm.vars)
        gs[*this] = &t;
      else
        ls[lm] = this;

      return true;
    }

    case TERM_GLOBAL_META: {
      const GlobalMeta &gm = dynamic_cast<const GlobalMeta &>(t);

      if(n!=gm.n)
        if(vars>=gm.vars)
          gs[*this] = &t;
        else
          gs[gm] = this;

      return true;
    }
  }

  return false;
}

bool TermList::unify(const TermList &t,LocalSubst &ls,GlobalSubst &gs) const
{
  const_iterator i1 = begin(), end1 = end(),
                 i2 = t.begin(), end2 = t.end();

  for(;i1 != end1 && i2 != end2; ++i1, ++i2) {
    if( !(**i1).unify(**i2,ls,gs) ) return false;
  }

  return i1 == end1 && i2 == end2;
}

//---------------------------------------------------------------------------
bool Func::is_super(const Term &t,LocalSubst &ls) const
{
  if(t.type==TERM_FUNC) {
    const Func &f = dynamic_cast<const Func &>(t);
    return f_index == f.f_index && terms->is_super(*f.terms,ls);
  } else
    return false;
}

bool Variable::is_super(const Term &t,LocalSubst &ls) const
{
  if(type!=t.type) return false;

  return v_index==dynamic_cast<const Variable&>(t).v_index;
}

bool LocalMeta::is_super(const Term &t,LocalSubst &ls) const
{
  const Term *term = find_subst(ls);
  if(term)
    return term->is_super(t,ls);
  else {
    const LocalMeta *lm = dynamic_cast<const LocalMeta *>(&t);
    if(!lm || lm->n!=n)
      if(t.contains_qvar() || t.contains_gmeta() || vars<t.max_vars())
        return false;
      else {
        ls[*this] = &t;
        return true;
      }
    else
      return true;
  }
}

bool GlobalMeta::is_super(const Term &t,LocalSubst &ls) const
{
  if(type!=t.type) return false;

  return n==dynamic_cast<const GlobalMeta&>(t).n;
}

bool QVariable::is_super(const Term &t,LocalSubst &ls) const
{
  if(type!=t.type) return false;

  return q_pos==dynamic_cast<const QVariable&>(t).q_pos;
}

bool TermList::is_super(const TermList &t,LocalSubst &ls) const
{
  const_iterator i1 = begin(), end1 = end(),
                 i2 = t.begin(), end2 = t.end();

  for(;i1 != end1 && i2 != end2; ++i1, ++i2)
    if( !(**i1).is_super(**i2,ls) ) return false;

  return i1 == end1 && i2 == end2;
}

//---------------------------------------------------------------------------
bool TermList::contains_qvar() const
{
  const_iterator i = begin(), end1 = end();

  for(;i != end1; ++i)
    if( (*i)->contains_qvar() ) return true;

  return false;
}

//---------------------------------------------------------------------------
bool TermList::contains_gmeta() const
{
  const_iterator i = begin(), end1 = end();

  for(;i != end1; ++i)
    if( (*i)->contains_gmeta() ) return true;

  return false;
}

//---------------------------------------------------------------------------
bool GlobalMeta::contains_meta(const LocalMeta &lm,
                               const LocalSubst &ls,const GlobalSubst &gs) const
{
  const Term *t = find_subst(gs);
  if(t)
    return t->contains_meta(lm,ls,gs);
  else
    return false;
}

bool GlobalMeta::contains_meta(const GlobalMeta &gm,
                               const LocalSubst &ls,const GlobalSubst &gs) const
{
  if(n==gm.n) return true;

  const Term *t = find_subst(gs);
  if(t)
    return t->contains_meta(gm,ls,gs);
  else
    return false;
}

bool LocalMeta::contains_meta(const LocalMeta &lm,
                              const LocalSubst &ls,const GlobalSubst &gs) const
{
  if(n==lm.n) return true;

  const Term *t = find_subst(ls);
  if(t)
    return t->contains_meta(lm,ls,gs);
  else
    return false;
}

bool LocalMeta::contains_meta(const GlobalMeta &gm,
                              const LocalSubst &ls,const GlobalSubst &gs) const
{
  const Term *t = find_subst(ls);
  if(t)
    return t->contains_meta(gm,ls,gs);
  else
    return false;
}

//---------------------------------------------------------------------------
bool TermList::contains_meta(const LocalMeta &lm,
                             const LocalSubst &ls,const GlobalSubst &gs) const
{
  const_iterator i = begin(), endlist = end();

  for(;i != endlist; ++i)
    if( (*i)->contains_meta(lm,ls,gs) ) return true;

  return false;
}

bool TermList::contains_meta(const GlobalMeta &gm,
                             const LocalSubst &ls,const GlobalSubst &gs) const
{
  const_iterator i = begin(), endlist = end();

  for(;i != endlist; ++i)
    if( (*i)->contains_meta(gm,ls,gs) ) return true;

  return false;
}

//---------------------------------------------------------------------------
const Term* LocalMeta::find_subst(const LocalSubst &ls) const
{
  LocalSubst::const_iterator i = ls.find(*this);
  if(i == ls.end())
    return 0;
  else
    return i->second;
}

const Term* GlobalMeta::find_subst(const GlobalSubst &gs) const
{
  GlobalSubst::const_iterator i = gs.find(*this);
  if(i == gs.end())
    return 0;
  else
    return i->second;
}

//---------------------------------------------------------------------------
TermList *TermList::p_all_subst(const Term &t) const
{
  TermList *newterm = new TermList;

  const_iterator i = begin(), endlist = end();

  for(; i != endlist; ++i)
    newterm->push_back((*i)->p_all_subst(t));

  return newterm;
}

//---------------------------------------------------------------------------
TermList* TermList::l2g(LocalSubst &ls) const
{
  TermList *newterm = new TermList;

  const_iterator i = begin(), endlist = end();

  for(; i != endlist; ++i)
    newterm->push_back((*i)->l2g(ls));

  return newterm;
}

//---------------------------------------------------------------------------
TermList *TermList::q2t(const Term &t) const
{
  TermList *newterm = new TermList;

  const_iterator i = begin(), endlist = end();

  for(; i != endlist; ++i)
    newterm->push_back((*i)->q2t(t));

  return newterm;
}

//---------------------------------------------------------------------------
TermList *TermList::t2q(const Term &t) const
{
  TermList *newterm = new TermList;

  const_iterator i = begin(), endlist = end();

  for(; i != endlist; ++i) {
    if(**i == t)
      newterm->push_back(new QVariable(0));
    else
      newterm->push_back((*i)->t2q(t));
  }

  return newterm;
}

//---------------------------------------------------------------------------
bool TermList::check_qt(const TermList &t,TermVec &tv) const
{
  assert(size() == t.size());

  const_iterator i1 = begin(),
                 i2 = t.begin(),
                 end1 = end();

  for(;i1 != end1; ++i1, ++i2)
    if( !(*i1)->check_qt(**i2,tv) ) return false;

  return true;
}

//---------------------------------------------------------------------------
size_t TermList::max_vars() const
{
  size_t r = 0;

  const_iterator i = begin(), endlist = end();

  for(;i != endlist; ++i)
    r = max(r,(*i)->max_vars());

  return r;
}

//---------------------------------------------------------------------------
int TermList::max_lmeta() const
{
  int n = -1;

  const_iterator i = begin(), endlist = end();

  for(;i != endlist; ++i)
    n = max(n,(*i)->max_lmeta());

  return n;
}

//---------------------------------------------------------------------------
int TermList::redo_lmeta(MetaMap &m,int base) const
{
  int n = base;

  const_iterator i = begin(), endlist = end();

  for(;i != endlist; ++i)
    n = max(n,(*i)->redo_lmeta(m,base));

  return n;
}

//---------------------------------------------------------------------------
TermList *TermList::p_subst(const LocalSubst &ls,const GlobalSubst &gs) const
{
  TermList *newterm = new TermList;

  const_iterator i = begin(), endlist = end();

  for(; i != endlist; ++i)
    newterm->push_back( (*i)->p_subst(ls,gs) );

  return newterm;
}

//---------------------------------------------------------------------------
int TermList::t_depth() const
{
  int m=0;

  const_iterator i = begin(), endlist = end();

  for(; i != endlist; ++i)
    m = max(m,(*i)->t_depth());

  return m;
}

//---------------------------------------------------------------------------
TermList::TermList(const TermList &t)
{
  const_iterator i = t.begin(), end_ = t.end();

  for(; i != end_; ++i) push_back((*i)->clone());
}

TermList::TermList(const LocalSubst &ls)
{
  LocalSubst::const_iterator i = ls.begin(), end_ = ls.end();

  for(; i != end_; ++i) push_back(i->first.clone());
}

TermList::~TermList()
{
  const_iterator i = begin(), end_ = end();

  for(; i != end_; ++i) delete *i;
}

//---------------------------------------------------------------------------
Term* Term::default_term(unsigned int level,size_t vars)
{
  Term *t;

  if((t = Variable::default_var(0,vars)) != 0) return t;

  return Func::default_func(level,0,vars);
}

Func* Func::default_func(unsigned int level,size_t beg,size_t vars)
{
  if(level==0) {
    size_t i = beg, end = func_array.size();

    while(i!=end) {                                   /* TODO : use algorithm */
      if(func_array[i].arity==0) return new Func(i,new TermList());
      ++i;
    }

    return 0;
  } else {
    if(beg == func_array.size())
      return 0;

    Term *t = Term::default_term(level-1,vars);
    if(t) {
      TermList *terms = new TermList();
      for(int i=func_array[beg].arity;i>0;i--) terms->push_back(t->clone());
      delete t;
      return new Func(beg,terms);
    }
    return 0;
  }

}

Variable* Variable::default_var(size_t beg,size_t vars)
{
  if(var_table.empty() || beg==vars)
    return 0;
  else
    return new Variable(beg,0);  /* TODO : incorrect level for default_var */
}

//---------------------------------------------------------------------------
bool next(Term*& t,unsigned int level,size_t vars)
{
  Func* f = dynamic_cast<Func*>(t);

  if(f) {
    TermList::iterator ti = f->terms->begin(),
                       end = f->terms->end();

    while(ti != end && !next(*ti,level-1,vars)) ++ti;

    if(ti == end) {
      size_t fi = f->f_index;
      delete t;
      t = Func::default_func(level,++fi,vars);
      if(t == 0) {
        t = Term::default_term(level,vars);
        return false;
      } else
        return true;
    }

    return true;
  }

  Variable* v = dynamic_cast<Variable*>(t);

  assert(v != 0);

  size_t vi = v->v_index;
  delete t;
  t = Variable::default_var(vi+1,vars);

  if(t == 0) {
    t = Func::default_func(level,0,vars);

    if(t == 0) {
      t = Term::default_term(level,vars);
      return false;
    }
  }

  return true;
}

void generate(unsigned int level,size_t vars)
{
  Term *t = Term::default_term(level,vars);
  int n=0;

  while(t) {
    n++;
    t->out(cout,QrArray());
    cout << endl;
    if(!next(t,level,vars)) {delete t; t = 0;}
  }

  cout << endl
       << n << " terms generated" << endl;

}


