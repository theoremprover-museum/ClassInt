//---------------------------------------------------------------------------

#pragma hdrstop

#include "parser.h"
#include "main.h"

//---------------------------------------------------------------------------

Parser::Parser(Lexer &lexer): lex(lexer)
{
  ch = 0;
  max_gmetas = -1;
}

void Parser::parse_text(bool get_new_token)
{
  if(get_new_token) lex.next_token();
  article();
}

void Parser::article()
{
  ch = new Checker(exprs,var_table);

  if(lex.token().code==Lexer::TOK_ENVIRON) {
    lex.next_token();
    environment();
    if(lex.token().code==Lexer::TOK_TEXT) {
      lex.next_token();
      text();
      if(lex.token().code!=Lexer::TOK_EOF)
        error("syntax error",lex.currentline());
    } else
      error("'text' expected",lex.currentline());
  } else
    error("'environ' expected",lex.currentline());

  delete ch;
}

void Parser::environment()
{
  while(true) {
    if(lex.token().code==Lexer::TOK_NUMBER || is_formula(lex.token())) {
      pair<string,const Expr*> p = proposition();

      ch->add_formula(p.first,p.second);

      if(lex.token().code==Lexer::TOK_SEMICOLON)
        lex.next_token();
      else
        break;
    } else
      break;
  }
}

void Parser::text()
{
  while(true) {
    if(lex.token().code==Lexer::TOK_NUMBER ||
       lex.token().code==Lexer::TOK_BEGIN ||
       is_formula(lex.token()))
    {
      text_item();
      if(lex.token().code==Lexer::TOK_SEMICOLON)
        lex.next_token();
      else
        break;
    } else
      break;
  }
}

const Expr* Parser::text_item()
{
  string label;

  if(lex.token().code==Lexer::TOK_NUMBER) {
    label = lex.token().str;
    if(lex.next_token().code==Lexer::TOK_COLON)
      lex.next_token();
    else
      error("label ':' expected",lex.currentline());
  }

  if(lex.token().code==Lexer::TOK_BEGIN) {
    lex.next_token();

    Checker *ocheck = ch;
    ch = new Checker(exprs,var_table);
    reasoning();

    if(lex.token().code==Lexer::TOK_END)
      lex.next_token();
    else
      error("'end' expected",lex.currentline());

    if(!ch->check_thesis())
      warning("invalid thesis",lex.currentline());

    Expr *e = ch->calc_thesis();
    if(e == 0) {
      warning("local variable in thesis",lex.currentline());
      e = new TrueConst();
    }

    delete ch;
    ch = ocheck;

    if(!ch->add_formula(label,e))
      error("duplicate label",lex.currentline());

    return e;

  } else if(is_formula(lex.token())) {
    const Expr *e = formula();
    justification(e);

    if(!ch->add_formula(label,e))
      error("duplicate label",lex.currentline());

    return e;
  } else
    error("'begin' or formula expected",lex.currentline());

  return 0;
}

pair<string,const Expr*> Parser::proposition()
{
  if(is_formula(lex.token()))
    return pair<string,const Expr*>("",formula());

  else if(lex.token().code==Lexer::TOK_NUMBER) {
    string label = lex.token().str;
    lex.next_token();
    if(lex.token().code==Lexer::TOK_COLON) {
      lex.next_token();
      return pair<string,const Expr*>(label,formula());

    } else
      error("label ':' expected",lex.currentline());
  } else
    error("number or formula expected",lex.currentline());

  return pair<string,const Expr*>();
}

bool Parser::is_formula(const Lexer::TOKEN& tok) const
{
  return tok.code==Lexer::TOK_TRUE ||
         tok.code==Lexer::TOK_FALSE ||
         tok.code==Lexer::TOK_THESIS ||
         tok.code==Lexer::TOK_EX ||
         tok.code==Lexer::TOK_FOR ||
         tok.code==Lexer::TOK_LPAR ||
         tok.code==Lexer::TOK_NOT ||
         tok.code==Lexer::TOK_ID;
}

const Expr* Parser::formula()
{
  if(!is_formula(lex.token()))
    error("syntax error",lex.currentline());

  return quantor();
}

void Parser::justification(const Expr *e)
{
  if(lex.token().code==Lexer::TOK_PROOF) {
    lex.next_token();

    Checker *ocheck = ch;
    ch = new Checker(exprs,var_table,e->clone());
    reasoning();

    if(lex.token().code==Lexer::TOK_QED)
      lex.next_token();
    else
      error("'qed' expected",lex.currentline());

    if(!ch->check_thesis())
      warning("inference structure doesn't match thesis",lex.currentline());

    delete ch;
    ch = ocheck;

  } else
    simple_justification(e);
}

void Parser::simple_justification(const Expr *e)
{
  if(lex.token().code==Lexer::TOK_BY) {
    lex.next_token();
    ExprItList il;
    label_list(il);

    if(!ch->check(e,il))
      warning("inference is not accepted",lex.currentline());

  } else {              // without justification - check for tautology
    if(!ch->check(e,ExprItList()))
      warning("inference is not accepted",lex.currentline());
  }
}

void Parser::label_list(ExprItList &il)
{
  while(true) {
    if(lex.token().code==Lexer::TOK_NUMBER) {
      if(!ch->add_label(il,lex.token().str))
        warning("undefined label",lex.currentline());

      lex.next_token();
      if(lex.token().code==Lexer::TOK_COMMA)
        lex.next_token();
      else
        break;
    } else
      error("label number expected",lex.currentline());
  }
}

void Parser::assumption()
{
  if(lex.token().code==Lexer::TOK_ASSUME) {
    lex.next_token();
    pair<string,const Expr*> p = proposition();
    if(!ch->add_formula(p.first,p.second))
      error("duplicate label",lex.currentline());
    if(!ch->assumption(p.second))
      warning("assumption doesn't match thesis",lex.currentline());
  }
}

void Parser::exist_assumption()
{
  if(lex.token().code==Lexer::TOK_GIVEN) {
    lex.next_token();

    VarItList vil;
    variable_list(vil);

    if(lex.token().code==Lexer::TOK_SUCH) {
      if(lex.next_token().code==Lexer::TOK_THAT) {
        lex.next_token();
        pair<string,const Expr*> p = proposition();

        if(!ch->exist_assumption(vil,p.second))
          warning("existential assumption doesn't match thesis",lex.currentline());

        ch->add_formula(p.first,p.second);

      } else
        error("'that' expected",lex.currentline());
    } else {

      if(!ch->exist_assumption(vil,&TrueConst()))
        warning("existential assumption doesn't match thesis",lex.currentline());

    }
  }
}

void Parser::conclusion()
{
  if(lex.token().code==Lexer::TOK_THUS) {
    lex.next_token();
    if(!ch->conclusion(text_item()))
      warning("conclusion doesn't match thesis",lex.currentline());
  }
}

void Parser::exemplification()
{
  if(lex.token().code==Lexer::TOK_TAKE) {
    lex.next_token();
    TermList *t = parse_terms();

    TermList::iterator i, end = t->end();

    for(i = t->begin(); i != end; ++i)
      if(!ch->exemplification(&**i))
        warning("'take' disagrees with current thesis",lex.currentline());

    delete t;
  }
}

void Parser::variable_list(VarItList &vil)
{
  while(true) {
    if(lex.token().code==Lexer::TOK_ID) {
      if(!ch->add_variable(vil,lex.token().str))
        warning("Duplicate variable",lex.currentline());
      if(lex.next_token().code==Lexer::TOK_COMMA)
        lex.next_token();
      else
        break;
    } else
      error("variable name expected",lex.currentline());
  }
}

void Parser::generalization()
{
  if(lex.token().code==Lexer::TOK_LET) {
    lex.next_token();

    VarItList vil;
    variable_list(vil);

    VarItList::iterator i, end = vil.end();

    for(i=vil.begin(); i!=end; ++i)
      if(!ch->generalization(*i))
        warning("generalization disagrees with current thesis",lex.currentline());

    if(lex.token().code==Lexer::TOK_SUCH) {
      if(lex.next_token().code==Lexer::TOK_THAT) {
        lex.next_token();
        pair<string,const Expr*> p = proposition();
        if(!ch->add_formula(p.first,p.second))
          error("duplicate label",lex.currentline());
        if(!ch->assumption(p.second))
          warning("generalization formula disagrees with current thesis",lex.currentline());
      } else
        error("'that' expected",lex.currentline());
    }
  }
}

void Parser::consideration()
{
  if(lex.token().code==Lexer::TOK_CONSIDER) {
    lex.next_token();

    VarItList vil;
    variable_list(vil);

    if(lex.token().code==Lexer::TOK_SUCH) {
      if(lex.next_token().code==Lexer::TOK_THAT) {
        lex.next_token();
        pair<string,const Expr*> p = proposition();

        ExprItList il;
        if(lex.token().code==Lexer::TOK_BY) {
          lex.next_token();
          label_list(il);
        }

        if(!ch->consideration(vil,p.second,il))
          warning("not accepted",lex.currentline());

        ch->add_formula(p.first,p.second);

      } else
        error("'that' expected",lex.currentline());
    }
  }
}

void Parser::reasoning()
{
  while(true) {
    if(is_formula(lex.token()))
      text_item();
    else
      switch(lex.token().code) {
        case Lexer::TOK_NUMBER:
        case Lexer::TOK_BEGIN: text_item(); break;
        case Lexer::TOK_ASSUME: assumption(); break;
        case Lexer::TOK_GIVEN: exist_assumption(); break;
        case Lexer::TOK_THUS: conclusion(); break;
        case Lexer::TOK_CONSIDER: consideration(); break;
        case Lexer::TOK_LET: generalization(); break;
        case Lexer::TOK_TAKE: exemplification(); break;
        default: return;
      }
    if(lex.token().code==Lexer::TOK_SEMICOLON)
      lex.next_token();
    else
      break;
  }
}

//------------------- Expressions -------------------

Expr* Parser::expr(bool get_new_token)
{
  if(get_new_token) lex.next_token();
  q.clear();
  return quantor();
}

Expr* Parser::quantor()
{
  Expr *ex;
  string var;

  switch(lex.token().code) {
    case Lexer::TOK_EX:  return ex_quantor();
    case Lexer::TOK_FOR: return for_quantor();
    default:             return equals();
  }
}

ExQuantor* Parser::ex_quantor()
{
  ExQuantor *ex;
  string var;

  if(lex.next_token().code!=Lexer::TOK_ID)
    error("variable expected",lex.currentline());

  var = lex.token().str;
  if(find(q.begin(),q.end(),var) != q.end())
    error("quantor variable duplicate",lex.currentline());

  q.push_back(var);

  switch(lex.next_token().code) {
    case Lexer::TOK_COMMA:
      ex = new ExQuantor(var,ex_quantor());
      break;

    case Lexer::TOK_ST:
      lex.next_token();
      ex = new ExQuantor(var,quantor());
      break;

    default:
      error("'st' expected",lex.currentline());
  }

  q.pop_back();

  return ex;
}

ForQuantor* Parser::for_quantor()
{
  ForQuantor *ex;
  string var;

  if(lex.next_token().code!=Lexer::TOK_ID)
    error("variable expected",lex.currentline());

  var = lex.token().str;
  if(find(q.begin(),q.end(),var) != q.end())
    error("quantor variable duplicate",lex.currentline());

  q.push_back(var);

  switch(lex.next_token().code) {
    case Lexer::TOK_COMMA:
      ex = new ForQuantor(var,for_quantor());
      break;

    case Lexer::TOK_ST: {
      lex.next_token();
      Expr *e1 = quantor();
      if(lex.token().code!=Lexer::TOK_HOLDS)
        error("'holds' expected",lex.currentline());
      lex.next_token();
      Expr *e2 = quantor();
      ex = new ForQuantor(var,new ImpliesExpr(e1,e2));
      break;
    }

    case Lexer::TOK_HOLDS:
      lex.next_token();
      ex = new ForQuantor(var,quantor());
      break;

    default:
      error("'st' or 'holds' expected",lex.currentline());
  }

  q.pop_back();

  return ex;
}

Expr* Parser::equals()
{
  Expr *expr = implies();

  if(lex.token().code==Lexer::TOK_IFF) {
    lex.next_token();
    Expr *expr2 = implies();
    //expr = new IFFExpr(expr,expr2);
    expr = new AndExpr(new ImpliesExpr(expr,expr2),
                       new ImpliesExpr(expr2->clone(),expr->clone()));
  }

  return expr;
}

Expr* Parser::implies()
{
  Expr *expr = or();

  if(lex.token().code==Lexer::TOK_IMPLIES) {
    lex.next_token();
    expr = new ImpliesExpr(expr,or());
  }

  return expr;
}

Expr* Parser::or()
{
  Expr *expr = and();

  while(lex.token().code==Lexer::TOK_OR) {
    lex.next_token();
    expr = new OrExpr(expr,and());
  }

  return expr;
}

Expr* Parser::and()
{
  Expr *expr = not();

  if(lex.token().code==Lexer::TOK_AND) {
    lex.next_token();
    expr = new AndExpr(expr,and());
  }

  return expr;
}

Expr* Parser::not()
{
  if(lex.token().code==Lexer::TOK_NOT) {
    lex.next_token();
    return new ImpliesExpr(not(),new FalseConst());
  } else {
    return atom();
  }
}

Expr* Parser::atom()
{
  Expr *expr;
  const Expr *ex;
  TermList *terms;
  string name;

  switch(lex.token().code) {
    case Lexer::TOK_ID:

      if(lex.look_ahead().code==Lexer::TOK_LBRACKET)
        expr = predicate();
      else {
        terms = new TermList();
        terms->push_back(parse_term());
        if(lex.token().code!=Lexer::TOK_EQUAL)
          error("expression syntax",lex.currentline());
        lex.next_token();
        terms->push_back(parse_term());
        expr = new Equality(terms);
      }

      break;

    case Lexer::TOK_LPAR:
      lex.next_token();
      expr = quantor();
      if(lex.token().code!=Lexer::TOK_RPAR)
        error("expression syntax",lex.currentline());
      lex.next_token();
      break;

    case Lexer::TOK_TRUE:
      expr = new TrueConst();
      lex.next_token();
      break;

    case Lexer::TOK_FALSE:
      expr = new FalseConst();
      lex.next_token();
      break;

    case Lexer::TOK_THESIS:
      ex = ch->remain_thesis();
      if(ex == 0) {
        warning("'thesis' is not allowed here",lex.currentline());
        expr = new TrueConst();
      } else
        expr = ex->clone();

      lex.next_token();
      break;

    case Lexer::TOK_EX:
      expr = ex_quantor();
      break;

    case Lexer::TOK_FOR:
      expr = for_quantor();
      break;

    default:
      error("formula expected",lex.currentline());
  }

  return expr;
}

Predicate* Parser::predicate()
{
  if(lex.token().code != Lexer::TOK_ID)
    error("term expected",lex.currentline());

  string name = lex.token().str;

  if(lex.next_token().code!=Lexer::TOK_LBRACKET)
    error("predicate '[' expected",lex.currentline());

  Predicate *pred;
  PredIterator pi;
  TermList *terms;

  if(lex.next_token().code!=Lexer::TOK_RBRACKET) {
    terms = parse_terms();

    if(lex.token().code!=Lexer::TOK_RBRACKET)
      error("predicate ']' expected",lex.currentline());
  } else
    terms = new TermList;

  pi = pred_array.find(name);
  if(pi != pred_array.end()) {
    if(terms->size() != pi->second)
      error("invalid predicate arity",lex.currentline());  /* TODO : informate error */
  } else {
    pi = pred_array.insert(pair<string,size_t>(name,terms->size())).first;
  }

  pred = new Predicate(pi,terms);

  lex.next_token();

  return pred;
}

TermList* Parser::parse_terms()
{

  TermList *t = new TermList;

  while(true) {
    t->push_back(parse_term());
    if(lex.token().code==Lexer::TOK_COMMA)
      lex.next_token();
    else
      break;
  }

  return t;
}

Term* Parser::parse_term()
{

  if(lex.token().code != Lexer::TOK_ID)
    error("term expected",lex.currentline());

  string name = lex.token().str;

  Term *t;

  if(name.substr(0,5)=="gmeta") {      /* TODO : metas parsed here */
    t = new GlobalMeta(name[5]-'0',0);
    max_gmetas = max(max_gmetas,name[5]-'0');
    lex.next_token();
    return t;
  }

  if(name.substr(0,5)=="lmeta") {
    t = new LocalMeta(name[5]-'0',0);
    lex.next_token();
    return t;
  }

  if(lex.next_token().code!=Lexer::TOK_LPAR) {

    QrArray::iterator qi = find(q.begin(),q.end(),name);
    if(qi == q.end()) {
      VarIterator vi = find(var_table.begin(),var_table.end(),name);
      if(vi == var_table.end()) {   /* TODO : IMHO bad work on global tables */
        warning("Undefined variable",lex.currentline());
        ch->add_variable(VarItList(),name);
        vi = find(var_table.begin(),var_table.end(),name);
      }
      t = new Variable(vi-var_table.begin(),0);
    } else
      t = new QVariable(qi-q.begin());

  } else {

    TermList *terms;

    if(lex.next_token().code!=Lexer::TOK_RPAR) {
      terms = parse_terms();

      if(lex.token().code!=Lexer::TOK_RPAR)
        error(") expected",lex.currentline());
    } else
      terms = new TermList;

    size_t fi = find_func(name);
    if(fi != func_array.size()) {
      if(terms->size() != func_array[fi].arity)
        error("invalid function arity",lex.currentline());  /* TODO : informate error */
    } else {
      fi = func_array.size();
      func_array.push_back(func_struct(name,terms->size()));
    }

    t = new Func(fi,terms);

    lex.next_token();

  }

  return t;
}


