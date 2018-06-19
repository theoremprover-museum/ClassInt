#ifndef TERM_H
#define TERM_H

#include <string>
#include <vector>
#include <list>
#include <map>
#include <ostream>
#include <assert.h>

///-------------------------------------------------------------------------

using namespace std;

template <class T> class Ptr {
public:
  T* value;
  Ptr(const Ptr& ptr) {value = ptr.value->clone();}
  Ptr(const T& val) {value = val.clone();}
  Ptr(T* val) {value = val;}
  T& operator *() const {return *value;}
  T* operator ->() const {return value;}
  ~Ptr() { delete value; }
};

typedef vector<string> VarTable;
typedef VarTable::iterator VarIterator;

typedef vector<string> QrArray;

typedef map<int,int> MetaMap;

struct CmpInfo {
  MetaMap lmm,gmm;
  bool cmpgm;
  CmpInfo(bool compare_gmeta=false): cmpgm(compare_gmeta) {}
};

struct func_struct {
  string name;
  size_t arity;
  func_struct(const string &funcname,size_t funcarity)
    : name(funcname), arity(funcarity) {}
};

typedef vector<func_struct> FuncArray;
typedef FuncArray::iterator FuncIterator;

extern VarTable var_table;
extern FuncArray func_array;

typedef vector<const class Term*> TermVec;

template<class Meta> struct meta_less
{
  bool operator()(const Meta &m1,const Meta &m2)
  {
    return m1.number()<m2.number();
  }
};

typedef map<class LocalMeta,const Term*,meta_less<LocalMeta> > LocalSubst;
typedef map<class GlobalMeta,const Term*,meta_less<GlobalMeta> > GlobalSubst;

extern size_t new_temp_var(size_t vars);
extern size_t find_func(const string &name);
//extern size_t new_temp_func(size_t funcs,size_t arity);

///-------------------------------------------------------------------------
class Term
{
public:
  static unsigned long meta_number;
  static unsigned long skolem_number;

  static int max_depth;

  enum TERM_TYPE {
    TERM_VOID,
    TERM_FUNC,
    TERM_VAR,
    TERM_QVAR,
    TERM_LOCAL_META,
    TERM_GLOBAL_META
  };

  const TERM_TYPE type;

  Term(TERM_TYPE t): type(t) {}

  virtual Term* clone() const = 0;

  virtual bool equalTo(const Term& t,CmpInfo &ci) const = 0;
  virtual bool lessThan(const Term& t) const = 0;

  bool operator ==(const Term& t) const { return equalTo(t,CmpInfo()); }
  bool operator <(const Term& t) const { return lessThan(t); }

  virtual const string& name() const = 0;
  virtual void out(ostream& os,QrArray &q) const = 0;

  virtual Term* p_all_subst(const Term &t) const { return clone(); }
  virtual Term* q2t(const Term &t) const { return clone(); }
  virtual Term* t2q(const Term &t) const { return clone(); }
  virtual bool check_qt(const Term &t,TermVec &tv) const { return type==t.type; }
  virtual Term* p_subst(const LocalSubst &ls,const GlobalSubst &gs) const = 0;
  virtual Term* l2g(LocalSubst&) const { return clone(); }

  virtual bool unify(const Term &t,LocalSubst &ls,GlobalSubst &gs) const = 0;
  virtual bool is_super(const Term &t,LocalSubst &ls) const = 0;

  virtual bool contains_qvar() const = 0;
  virtual bool contains_gmeta() const = 0;
  virtual bool contains_meta
    (const LocalMeta &lm,const LocalSubst &ls,const GlobalSubst &gs) const = 0;
  virtual bool contains_meta
    (const GlobalMeta &gm,const LocalSubst &ls,const GlobalSubst &gs) const = 0;
  virtual size_t max_vars() const = 0;
  virtual int max_lmeta() const = 0;
  virtual int redo_lmeta(MetaMap &m,int base) const { return base; }

  virtual int t_depth() const { return 0; }

  virtual ~Term() {}

  static unsigned long new_meta() { return meta_number++; }
  static unsigned long new_skolem() { return skolem_number++; }
  static void reset_meta(int meta_num) { meta_number=meta_num; }
  static void reset_skolem() { skolem_number=func_array.size(); }
  static Term* default_term(unsigned int level,size_t vars);

  friend bool next(Term*& t,unsigned int level,size_t vars);

};

///-------------------------------------------------------------------------
class TermList: public list<Term*>
{

public:

  TermList(const TermList &t);
  TermList(const LocalSubst &ls);
  TermList() {}

  bool equalTo(const TermList& t,CmpInfo &ci) const;
  bool lessThan(const TermList& t) const;

  bool unify(const TermList &t,LocalSubst &ls,GlobalSubst &gs) const;
  bool is_super(const TermList &t,LocalSubst &ls) const;

  TermList *p_subst(const LocalSubst &ls,const GlobalSubst &gs) const;
  TermList *l2g(LocalSubst&) const;

  int t_depth() const;

  TermList *p_all_subst(const Term &t) const;
  TermList *q2t(const Term &t) const;
  TermList *t2q(const Term &t) const;
  bool check_qt(const TermList &t,TermVec &tv) const;
  size_t max_vars() const;
  int max_lmeta() const;

  bool contains_qvar() const;
  bool contains_gmeta() const;
  bool contains_meta(const LocalMeta &lm,
                     const LocalSubst &ls,const GlobalSubst &gs) const;
  bool contains_meta(const GlobalMeta &gm,
                     const LocalSubst &ls,const GlobalSubst &gs) const;

  int redo_lmeta(MetaMap &m,int base) const;

  //friend ostream& operator <<(ostream& os,const TermList& tl);
  void out(ostream& os,QrArray &q) const;

  ~TermList();
};

///-------------------------------------------------------------------------
class Func: public Term
{
  TermList *terms;

  bool equalTo(const Term& t,CmpInfo &ci) const;
  bool lessThan(const Term& t) const;

public:
  size_t f_index;       /* TODO : !!!!!!!!!!!!! */
  Func(const Func &f): Term(TERM_FUNC)
  {
    f_index=f.f_index;
    terms=new TermList(*f.terms);
  }

  Func(size_t func_index,const TermList &t): Term(TERM_FUNC)
  {
    //assert(func_array[func_index].arity == t.size());
    f_index = func_index;
    terms = new TermList(t);
  }

  Func(size_t func_index,TermList *t): Term(TERM_FUNC)
  {
    //assert(func_array[func_index].arity == t->size());
    f_index = func_index;
    terms = t;
  }

  Func& operator =(const Func &f)
  {
    delete terms;

    f_index=f.f_index;
    terms=new TermList(*f.terms);

    return *this;
  }

  const string& name() const { return func_array[f_index].name; }
  void out(ostream& os,QrArray &q) const;

  Func* clone() const { return new Func(*this); }
  Term* p_all_subst(const Term &t) const
  {
    return new Func(f_index,terms->p_all_subst(t));
  }
  Term* q2t(const Term &t) const { return new Func(f_index,terms->q2t(t)); }
  Term* t2q(const Term &t) const { return new Func(f_index,terms->t2q(t)); }

  bool check_qt(const Term &t,TermVec &tv) const
  {
    if(type!=t.type) return false;
    const Func &f = dynamic_cast<const Func&>(t);
    return f_index==f.f_index && terms->check_qt(*f.terms,tv);
  }

  Term* p_subst(const LocalSubst &ls,const GlobalSubst &gs) const
  {
    return new Func(f_index,terms->p_subst(ls,gs));
  }

  Term* l2g(LocalSubst &ls) const
  {
    return new Func(f_index,terms->l2g(ls));
  }

  bool unify(const Term &t,LocalSubst &ls,GlobalSubst &gs) const;
  bool is_super(const Term &t,LocalSubst &ls) const;

  bool contains_qvar() const { return terms->contains_qvar(); }
  bool contains_gmeta() const { return terms->contains_gmeta(); }

  bool contains_meta(const LocalMeta &lm,
                     const LocalSubst &ls,const GlobalSubst &gs) const
  {
    return terms->contains_meta(lm,ls,gs);
  }

  bool contains_meta(const GlobalMeta &gm,
                     const LocalSubst &ls,const GlobalSubst &gs) const
  {
    return terms->contains_meta(gm,ls,gs);
  }

  size_t max_vars() const { return terms->max_vars(); }
  int max_lmeta() const { return terms->max_lmeta(); }

  int redo_lmeta(MetaMap &m,int base) const
  {
    return terms->redo_lmeta(m,base);
  }

  int t_depth() const { return 1+terms->t_depth(); }

  ~Func() { delete terms; }

  static Func* default_func(unsigned int level,size_t beg,size_t vars);

  friend bool next(Term*& t,unsigned int level,size_t vars);

};

///-------------------------------------------------------------------------
class Variable: public Term
{
  size_t v_index;

  bool equalTo(const Term& t,CmpInfo &ci) const;
  bool lessThan(const Term& t) const;

public:
  //const int level;

  Variable(const Variable &v): Term(TERM_VAR) { v_index = v.v_index; }
  Variable(size_t var_index, int Level): Term(TERM_VAR) { v_index = var_index; }
  Variable* clone() const { return new Variable(*this); }

  const string& name() const { return var_table[v_index]; }
  void out(ostream& os,QrArray &q) const { os << var_table[v_index]; }

  Term* p_all_subst(const Term &t) const { return clone(); }
  Term* p_subst(const LocalSubst &ls,const GlobalSubst &gs) const
    { return clone(); }

  bool unify(const Term &t,LocalSubst &ls,GlobalSubst &gs) const;
  bool is_super(const Term &t,LocalSubst &ls) const;

  bool contains_qvar() const { return false; }

  bool contains_gmeta() const { return false; }

  bool contains_meta(const LocalMeta &lm,
                     const LocalSubst &ls,const GlobalSubst &gs) const
    { return false; }

  bool contains_meta(const GlobalMeta &gm,
                     const LocalSubst &ls,const GlobalSubst &gs) const
    { return false; }

  size_t max_vars() const { return v_index+1; }
  int max_lmeta() const { return -1; }

  static Variable* default_var(size_t beg,size_t vars);
  friend bool next(Term*& t,unsigned int level,size_t vars);

};

///-------------------------------------------------------------------------
class GlobalMeta: public Term
{
  int n;

  bool equalTo(const Term& t,CmpInfo &ci) const;
  bool lessThan(const Term& t) const;

public:
  const size_t vars;

  GlobalMeta(const GlobalMeta &gm)
    : Term(TERM_GLOBAL_META), vars(gm.vars), n(gm.n) {}
  GlobalMeta(int N,size_t Vars)
    : Term(TERM_GLOBAL_META), vars(Vars), n(N) {}
  GlobalMeta* clone() const { return new GlobalMeta(*this); }

  const string& name() const { throw "attempt to get name of local metavar"; }
  void out(ostream& os,QrArray &q) const { os << "gmeta" << n; }

  Term* p_all_subst(const Term &t) const { return t.clone(); }

  bool check_qt(const Term &t,TermVec &tv) const
  {
    throw "check_qt() is not supported on pvar";
  }

  Term* p_subst(const LocalSubst &ls,const GlobalSubst &gs) const
  {
    const Term *t = find_subst(gs);
    if(t)
      return t->p_subst(ls,gs);
    else
      return clone();
  }

  int number() const { return n; }

  bool unify(const Term &t,LocalSubst &ls,GlobalSubst &gs) const;
  bool is_super(const Term &t,LocalSubst &ls) const;

  bool contains_qvar() const { return false; }
  bool contains_gmeta() const { return true; }
  bool contains_meta(const LocalMeta &lm,
                     const LocalSubst &ls,const GlobalSubst &gs) const;
  bool contains_meta(const GlobalMeta &gm,
                     const LocalSubst &ls,const GlobalSubst &gs) const;
  size_t max_vars() const { return vars; }
  int max_lmeta() const { return -1; }

  const Term* find_subst(const GlobalSubst &s) const;

};

///-------------------------------------------------------------------------
class LocalMeta: public Term
{
  mutable int n;

  bool equalTo(const Term& t,CmpInfo &ci) const;
  bool lessThan(const Term& t) const;

public:
  const size_t vars;

  LocalMeta(const LocalMeta &lm)
    : Term(TERM_LOCAL_META), vars(lm.vars), n(lm.n) {}
  LocalMeta(int N,size_t Vars)
    : Term(TERM_LOCAL_META), vars(Vars), n(N) {}
  LocalMeta* clone() const { return new LocalMeta(*this); }

  const string& name() const { throw "attempt to get name of local metavar"; }
  void out(ostream& os,QrArray &q) const { os << "lmeta" << n; }

  Term* p_all_subst(const Term &t) const { return t.clone(); }

  bool check_qt(const Term &t,TermVec &tv) const
  {
    throw "check_qt() is not supported on pvar";
  }

  Term* p_subst(const LocalSubst &ls,const GlobalSubst &gs) const
  {
    const Term *t = find_subst(ls);
    if(t)
      return t->p_subst(ls,gs);
    else
      return clone();
  }

  Term* l2g(LocalSubst &ls) const
  {
    const Term *t = find_subst(ls);
    if(t) return t->clone();
    Term *tt = new GlobalMeta(new_meta(),vars);
    ls[*this] = tt;
    return tt;
  }

  int number() const { return n; }

  bool unify(const Term &t,LocalSubst &ls,GlobalSubst &gs) const;
  bool is_super(const Term &t,LocalSubst &ls) const;

  bool contains_qvar() const { return false; }
  bool contains_gmeta() const { return false; }

  bool contains_meta(const LocalMeta &lm,
                     const LocalSubst &ls,const GlobalSubst &gs) const;
  bool contains_meta(const GlobalMeta &gm,
                     const LocalSubst &ls,const GlobalSubst &gs) const;
  size_t max_vars() const { return vars; }
  int max_lmeta() const { return n; }

  int redo_lmeta(MetaMap &m,int base) const
  {
    MetaMap::iterator i = m.find(n);
    if(i==m.end()) {
      int n_ = n;
      n = base+m.size();
      m.insert(pair<int,int>(n_,n));
    } else
      n = i->second;
    return n;
  }

  const Term* find_subst(const LocalSubst &ls) const;

};

///-------------------------------------------------------------------------
class QVariable: public Term
{
  unsigned int q_pos;

  bool equalTo(const Term& t,CmpInfo &ci) const;
  bool lessThan(const Term& t) const;

public:
  QVariable(const QVariable &qvar_): Term(TERM_QVAR) { q_pos=qvar_.q_pos; }
  QVariable(unsigned int qvar_pos): Term(TERM_QVAR) { q_pos=qvar_pos; }
  QVariable* clone() const { return new QVariable(*this); }

  const string& name() const { throw "attempt to get name of qvar"; }
  void out(ostream& os,QrArray &q) const { os << q[q_pos]; }

  Term* q2t(const Term &t) const
  {
    if(q_pos>0)
      return new QVariable(q_pos-1);
    else
      return t.clone();
  }

  Term* t2q(const Term &t) const
  {
    return new QVariable(q_pos+1);
  }

  bool check_qt(const Term &t,TermVec &tv) const
  {
    if(tv.size()<=q_pos || tv[q_pos]==0) {
      tv.resize(q_pos+1);    /* TODO : IMHO VERY BAD */
      tv[q_pos] = &t;
      return true;
    } else
      return *tv[q_pos] == t;
  }

  Term* p_subst(const LocalSubst &ls,const GlobalSubst &gs) const
    { return clone(); }

  bool unify(const Term &t,LocalSubst &ls,GlobalSubst &gs) const;
  bool is_super(const Term &t,LocalSubst &ls) const;

  bool contains_qvar() const { return true; }
  bool contains_gmeta() const { return false; }
  bool contains_meta(const LocalMeta &lm,
                     const LocalSubst &ls,const GlobalSubst &gs) const
    { return false; }
  bool contains_meta(const GlobalMeta &gm,
                     const LocalSubst &ls,const GlobalSubst &gs) const
    { return false; }
  size_t max_vars() const { return 0; }
  int max_lmeta() const { return -1; }

};

///-------------------------------------------------------------------------
inline ostream& operator <<(ostream& os,const Term& t)
{
  t.out(os,QrArray());
  return os;
}

extern void generate(unsigned int level);

#endif
