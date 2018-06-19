//---------------------------------------------------------------------------

#include <istream.h>
#include <strstream>
#include <string.h>

#pragma hdrstop

#include "main.h"
#include "lexer.h"
#include "parser.h"

#include <windows.h>

ofstream *upper_os;
ofstream *lower_os;
ofstream *subst_os;

//---------------------------------------------------------------------------
void error(const char *msg,int linenumber)
{
  if(linenumber==-1)
    cout << "Error: " << msg << endl;
  else
    cout << "Error in line " << linenumber << ": " << msg << endl;

  exit(1);
}

//---------------------------------------------------------------------------
void warning(const char *msg,int linenumber)
{
  if(linenumber==-1)
    cout << "Error: " << msg << endl;
  else
    cout << "Error in line " << linenumber << ": " << msg << endl;
}

//---------------------------------------------------------------------------
void usage()
{
  cout <<
#ifdef INTUITIONISTIC
    "Automatic intuitionistic theorem verifier with natural deduction\n"
    "INT 0.9  Copyright (c) 2005 U.Vtorushin, O.Okhotnikov, A.Zamyatin\n"
#else
    "Automatic theorem verifier with natural deduction\n"
    "CLASS 0.9  Copyright (c) 2005 U.Vtorushin, O.Okhotnikov, A.Zamyatin\n"
#endif
    "\n"
    "The stdin is used for input\n\n";
}

//---------------------------------------------------------------------------
#pragma argsused
int main(int argc, char* argv[])
{
  upper_os = new ofstream("class_upper.cls");
  lower_os = new ofstream("class_lower.cls");
  subst_os = new ofstream("class_subst.cls");

  istream *is = 0;

  for(int i=1;i<argc;i++)
    if(strncmp(argv[i],"-f:",3)==0) {
      if(strlen(argv[i])<4) error("File name is not specified");
      if(is) error("-f and -e can't be used together");
      is = new ifstream(argv[i]+3);
      if(!is->good())
        error((string("Failed to open ")+(argv[i]+3)).c_str());

    } else if(strncmp(argv[i],"-e",3)==0) {
      i++;
      if(i>=argc) error("Expression is not specified");
      if(is) error("-f and -e can't be used together");
      is = new istrstream(argv[i]);

    } else if(strncmp(argv[i],"-mtd:",5)==0) {
      if(strlen(argv[i])<4) error("Invalid max term depth");
      sscanf(argv[i]+5,"%d",&Term::max_depth);
      if(Term::max_depth<0) error("Invalid max term depth");

    } else if( strncmp(argv[i],"-?",2)==0 || strncmp(argv[i],"/?",2)==0 ) {
      usage();
      exit(0);

    } else
      error((string("Invalid command line parameter: ")+argv[i]).c_str());

  if(Term::max_depth<0) Term::max_depth = 3;

  if(!is) is = &cin;

  Lexer lex(*is);

  Parser parser(lex);

  long tick = GetTickCount();
  /*

  Expr *expr = parser.expr(true);
  if(lex.token().code!=Lexer::TOK_COMMA)
    error("expression syntax",lex.currentline());
  Expr *ex = parser.expr(true);
  if(lex.token().code!=Lexer::TOK_EOF)
    error("expression syntax",lex.currentline());

  cout << "Equal: " << (*expr==*ex) << endl;
  cout << "Less Than: " << (*expr<*ex) << endl;
  cout << "Reversed Less Than: " << (*ex<*expr) << endl;
  cout << "  Str1: " << *expr << endl;
  cout << "  Str2: " << *ex << endl;
  cout << "Term depth: " << expr->t_depth() << ", " << ex->t_depth() << endl;

  //------------
  //Expr *c1 = expr->convert(), *c2 = ex->convert();

  //cout << "Converted:" << endl
  //     << "str1: " << *c1 << endl
  //     << "str2: " << *c2 << endl;

  //delete c1;
  //delete c2;
  //------------

  SubstList s1,s2;
  if(expr->unify(*ex,s1,s2)) {
    cout << "Substitutions list 1:\n";
    SubstList::iterator i = s1.begin(),
                        end = s1.end();

    for(;i != end; ++i)
      cout << "  meta" << i->first << '=' << *i->second.first << " (" << 1+!i->second.second << ")\n";

    cout << "Substitutions list 2:\n";
    i = s2.begin(), end = s2.end();

    for(;i != end; ++i)
      cout << "  meta" << i->first << '=' << *i->second.first << " (" << 1+!i->second.second << ")\n";

    Expr *unexpr1 = expr->p_subst(true,s1,s2),
         *unexpr2 = ex->p_subst(false,s1,s2);

    cout << "Unified formula:\n";
    cout << "  formula 1: " << *unexpr1 << '\n';
    cout << "  formula 2: " << *unexpr2 << '\n';

    delete unexpr1;
    delete unexpr2;

  } else
    cerr << "UNIFICATION FAILED\n";

  delete expr;
  delete ex;

  */

  //try {

  //prover(parser,cout);

  //cout << "Less Than: " << (FalseConst()<TrueConst()) << endl;
  //return 0;

  parser.parse_text(true);

  //}
  //catch(char *s) {
  //  error(s);
  //}

  cout << endl
       << GetTickCount() - tick << " ms\n";

  //while(lex->next_token().code>0)
  //  cout << lex->token().str << endl;

  if(is != &cin) delete is;

  delete upper_os;
  delete lower_os;
  delete subst_os;

  return 0;
}
//---------------------------------------------------------------------------
