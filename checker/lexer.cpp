//---------------------------------------------------------------------------

#pragma hdrstop

#include "lexer.h"
#include <assert>

//---------------------------------------------------------------------------

Lexer::Lexer(istream &is): input(is.rdbuf())
{
  struct MapInitTok {
    char *str;
    TOKEN_CODE code;
  } maptoks[] = {
    {":",       TOK_COLON},
    {"::",      TOK_COMMENT},
    {"environ", TOK_ENVIRON},
    {"text",    TOK_TEXT},
    {"|-",      TOK_CORK},
    {"ex",      TOK_EX},
    {"for",     TOK_FOR},
    {"st",      TOK_ST},
    {"holds",   TOK_HOLDS},
    {"from",    TOK_FROM},
    {"by",      TOK_BY},
    {"proof",   TOK_PROOF},
    {"qed",     TOK_QED},
    {"begin",   TOK_BEGIN},
    {"end",     TOK_END},
    {"assume",  TOK_ASSUME},
    {"thus",    TOK_THUS},
    {"let",     TOK_LET},
    {"given",   TOK_GIVEN},
    {"consider",TOK_CONSIDER},
    {"take",    TOK_TAKE},
    {"such",    TOK_SUCH},
    {"that",    TOK_THAT},
    {"not",     TOK_NOT},
    {"or",      TOK_OR},
    {"implies", TOK_IMPLIES},
    {"iff",     TOK_IFF},
    {"true",    TOK_TRUE},
    {"false",   TOK_FALSE},
    {"contradiction", TOK_FALSE},
    {"thesis",  TOK_THESIS},
    {"&",       TOK_AND},
    {"(",       TOK_LPAR},
    {")",       TOK_RPAR},
    {"[",       TOK_LBRACKET},
    {"]",       TOK_RBRACKET},
    {"=",       TOK_EQUAL},
    {",",       TOK_COMMA},
    {";",       TOK_SEMICOLON},
  };

  const int MapToksCnt = sizeof(maptoks) / sizeof(maptoks[0]);

  for(int i=0;i<MapToksCnt;i++)
    tok_map[maptoks[i].str] = maptoks[i].code;

  linenumber = 1;
  ahead_valid = false;
}

const Lexer::TOKEN& Lexer::look_ahead()
{
  assert(!ahead_valid);

  TOKEN old = tok;
  //istream::pos_type pos = input.tellg();
  ahead = next_token();
  tok = old;
  //input.seekg(pos);
  ahead_valid = true;
  return ahead;
}

const Lexer::TOKEN& Lexer::next_token()
{
  while(next_token_wc().code==TOK_COMMENT);
  return tok;
}

const Lexer::TOKEN& Lexer::next_token_wc()
{
  if(ahead_valid) {
    ahead_valid = false;
    return tok = ahead;
  }

  char c=' ';

  tok.str.clear();

  while(input.get(c).good() && (c==' ' || c=='\n' || c=='\t')) {
    if(c=='\n') ++linenumber;
  }

  if(!input.good()) {
    tok.code = TOK_EOF;
    return tok;
  }

  tok.str = c;

  if(isalpha(c)) {
    while(input.get(c).good() && (isalpha(c) || isdigit(c))) tok.str+=c;

    if(!input.eof()) input.putback(c);

    tok.code = tok_map[tok.str];
    if(!tok.code) tok.code = TOK_ID;
    return tok;
  }

  if(isdigit(c)) {
    while(input.get(c).good() && isdigit(c)) tok.str+=c;

    if(!input.eof()) input.putback(c);

    tok.code = TOK_NUMBER;
    return tok;
  }

  while(input.get(c).good() && tok_map.find(tok.str+c)!=tok_map.end())
    tok.str+=c;

  if(!input.eof()) input.putback(c);

  tok.code = tok_map[tok.str];
  if(!tok.code) tok.code = TOK_BAD;

  if(tok.code==TOK_COMMENT) {
    while(input.get(c).good() && c!='\n') tok.str+=c;
    ++linenumber;
  }

  return tok;

}

