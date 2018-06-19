#ifndef MAIN_H
#define MAIN_H

#include <fstream.h>

using namespace std;

void error(const char *msg,int linenumber=-1);
void warning(const char *msg,int linenumber=-1);

extern ofstream *upper_os;
extern ofstream *lower_os;
extern ofstream *subst_os;

#endif