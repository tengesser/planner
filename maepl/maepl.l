%{
#include <cstdio>
#include <iostream>

#define YY_DECL extern "C" int yylex()

#include "maepl.tab.h"  // to get the token types that we return

%}

character [a-zA-Z]
digit		  [0-9]
dash      (_|\-)
name      {character}({character}|{digit}|{dash})*

%%
(\r\n|\n|\r)      { yylineno++;	}
[;][^\r\n]*[\r\n] { yylineno++;	}
[;][^\n]*[\n]     { yylineno++;	}
[;][^\r]*[\r]     { yylineno++;	}
[ \t]             ;
("("|"["|"{")     { return LBRACK; }
(")"|"]"|"}")     { return RBRACK; }
\-                { return MINUS; }
\=                { return EQUAL; }

(?i:define)               { return DEFINE; }
(?i:domain)               { return DOMAIN; }
(?i:problem)              { return PROBLEM; }

(?i::action)              { return ACTION; }
(?i::byagent)             { return BYAGENT; }
(?i::constants)           { return CONSTS; }
(?i::domain)              { return MYDOMAIN; }
(?i::extends)             { return EXTENDS; }
(?i::effect)              { return EFFECT; }
(?i::event-nondesignated) { return EVNDES; }
(?i::event-designated)    { return EVDES; }
(?i::goal)                { return GOAL; }
(?i::init)                { return INIT; }
(?i::objects)             { return OBJECTS; }
(?i::observability)       { return OBSRV; }
(?i::parameters)          { return PARAMS; }
(?i::precondition)        { return PRECD; }
(?i::predicates)          { return PREDS; }
(?i::types)               { return TYPES; }
(?i::world-designated)    { return WDES; }
(?i::world-nondesignated) { return WNDES; }

(?i:partition)            { return PART; }

(?i::requirements)               { yylval.sval = yytext; return REQS; }
(?i::strips)                     { yylval.sval = yytext; return REQT; }
(?i::typing)                     { yylval.sval = yytext; return REQT; }
(?i::disjunctive-preconditions)  { yylval.sval = yytext; return REQT; }
(?i::equality)                   { yylval.sval = yytext; return REQT; }
(?i::existential-preconditions)  { yylval.sval = yytext; return REQT; }
(?i::universal-preconditions)    { yylval.sval = yytext; return REQT; }
(?i::quantified-preconditions)   { yylval.sval = yytext; return REQT; }
(?i::conditional-effects)        { yylval.sval = yytext; return REQT; }
(?i::adl)                        { yylval.sval = yytext; return REQT; }
(?i::action-expansions)          { yylval.sval = yytext; return REQF; }
(?i::foreach-expansions)         { yylval.sval = yytext; return REQF; }
(?i::dag-expansions)             { yylval.sval = yytext; return REQF; }
(?i::domain-axioms)              { yylval.sval = yytext; return REQF; }
(?i::subgoal-through-axioms)     { yylval.sval = yytext; return REQF; }
(?i::safety-constraints)         { yylval.sval = yytext; return REQF; }
(?i::expression-evaluation)      { yylval.sval = yytext; return REQF; }
(?i::fluents)                    { yylval.sval = yytext; return REQF; }
(?i::open-world)                 { yylval.sval = yytext; return REQF; }
(?i::ucpop)                      { yylval.sval = yytext; return REQF; }


(?i:not)                  { return NOT; }
(?i:and)                  { return AND; }
(?i:knows)                { return KNOWS; }
(?i:common-knowledge)     { return COMMK; }
(?i:or)                   { return OR; }
(?i:imply)                { return IMPLY; }
(?i:either)               { return EITHER; }
(?i:exists)               { return EXISTS; }
(?i:forall)               { return FORALL; }
(?i:when)                 { return WHEN; }


\?{name}  { yylval.sval = strdup(yytext); 
            for (int i = 0; yylval.sval[i]; i++)
              yylval.sval[i] = tolower(yylval.sval[i]);
            return VAR; }
{name}    { yylval.sval = strdup(yytext);
            for (int i = 0; yylval.sval[i]; i++)
              yylval.sval[i] = tolower(yylval.sval[i]);
            return NAME; }

. { std::cerr << "Error in line " << yylineno << ". "
              << "Mystery token " << yytext << std::endl; }

%%
