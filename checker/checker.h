#ifndef CHECKER_H
#define CHECKER_H

#include <map>
#include "term.h"
#include "expr.h"

using namespace std;

typedef map<string,const Expr *> ExprArray;
typedef vector<ExprArray::iterator> ExprItList;
typedef vector<size_t> VarItList;

class Checker {
public:
  enum ItemType {IT_LET,IT_ASSUME,IT_THUS};

private:
  Expr *thesis;
  ExprArray &exprs;
  VarTable &vars;
  ExprItList new_formulae;
  size_t oldvars;
  vector<const Expr*> to_delete;
  typedef vector<pair<ExprArray::iterator,const Expr *> > ChangesVec;
  ChangesVec changes;

  struct ThesisItem {
    const Expr* expr;
    size_t vi;
    ItemType item_type;
    ThesisItem(ItemType it,const Expr *ex,size_t var_index=0): item_type(it),expr(ex),vi(var_index) {}
  };
  typedef vector<ThesisItem> ThesisVec;
  ThesisVec thesis_vec;

  bool find(const Expr &ex,const ExprItList &il) const;
  Expr *calc_thesis(ThesisVec::const_iterator) const;
  Checker();

public:
  Checker(ExprArray &exps,VarTable &var_table,Expr *Thesis = 0)
    : exprs(exps), vars(var_table), thesis(Thesis)
  {
    oldvars = vars.size();
  }

  bool check_thesis() const { return thesis == 0 || thesis->op()==Expr::OP_TRUE; }
  Expr* calc_thesis() const;
  const Expr* remain_thesis() const { return thesis; }

  bool assumption(const Expr *ex);
  bool conclusion(const Expr *ex);
  bool consideration(const VarItList &vil,const Expr *e,const ExprItList &il);
  bool exist_assumption(const VarItList &vil,const Expr *e);
  bool generalization(size_t vi);
  bool exemplification(const Term *t);
  bool check(const Expr *ex,const ExprItList &ll) const;
  bool add_variable(VarItList &il,const string &var);
  bool add_label(ExprItList &il,const string &label) const;
  bool add_formula(const string &label,const Expr *ex);

  ~Checker();
};

#endif
