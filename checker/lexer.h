#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <iostream>
#include <istream>
#include <map>

using namespace std;

class Lexer {

public:
  enum TOKEN_CODE {
    TOK_BAD=-1,
    TOK_EOF=0,
    TOK_COMMENT,
    TOK_ID,
    TOK_ENVIRON,
    TOK_TEXT,
//    TOK_CORK,                // |-
    TOK_EX,
    TOK_FOR,
    TOK_ST,
    TOK_HOLDS,
//    TOK_FROM,
    TOK_BY,
    TOK_PROOF,
    TOK_QED,
    TOK_BEGIN,
    TOK_END,
    TOK_ASSUME,
    TOK_THUS,
    TOK_CONSIDER,
    TOK_LET,
    TOK_GIVEN,
    TOK_TAKE,
    TOK_SUCH,
    TOK_THAT,
    TOK_NOT,
    TOK_AND,
    TOK_OR,
    TOK_IMPLIES,
    TOK_IFF,
    TOK_TRUE,
    TOK_FALSE,
    TOK_THESIS,
    TOK_LPAR,
    TOK_RPAR,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_EQUAL,
    TOK_COMMA,
    TOK_SEMICOLON,
    TOK_COLON,
    TOK_NUMBER,
  };

  struct TOKEN {
    TOKEN_CODE code;
    string str;
  };

private:
  TOKEN tok,ahead;
  bool ahead_valid;
  string tok_str;
  int linenumber;

  istream input;

  typedef map<string,TOKEN_CODE> Tok_Map;
  Tok_Map tok_map;

public:

  Lexer(istream &is);

  const TOKEN& next_token();
  const TOKEN& next_token_wc();
  const TOKEN& look_ahead();
  const TOKEN& token() const {return tok;}
  int currentline() const {return linenumber;}

};

#endif
