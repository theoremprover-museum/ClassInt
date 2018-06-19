#ifndef PARSER_H
#define PARSER_H

#include <utility>
#include "lexer.h"
#include "expr.h"
#include "checker.h"

class Parser {
  Lexer& lex;
  QrArray q;
  ExprArray exprs;
  Checker *ch;
  int max_gmetas;

  ExQuantor* Parser::ex_quantor();
  ForQuantor* Parser::for_quantor();

  bool is_formula(const Lexer::TOKEN &tok) const;

  Expr *equals();
  Expr *implies();
  Expr *or();
  Expr *and();
  Expr *not();
  Expr *quantor();
  Expr *atom();
  Predicate *predicate();
  TermList *parse_terms();
  Term* parse_term();

public:
  Parser(Lexer &lexer);

  Lexer& lexer() {return lex;};
  const Lexer::TOKEN& token() {return lex.token();};

  void Parser::parse_text(bool get_new_token);

  void article();
  void environment();
  void text();
  const Expr* text_item();
  pair<string,const Expr*> proposition();
  void assumption();
  void exist_assumption();
  void conclusion();
  void exemplification();
  void generalization();
  void consideration();
  void reasoning();
  void variable_list(VarItList &vil);
  const Expr *formula();
  void justification(const Expr *);
  void simple_justification(const Expr *e);
  void label_list(ExprItList &);

  int get_max_gmetas() const { return max_gmetas; }

  Expr *expr(bool get_new_token);

};

#endif
