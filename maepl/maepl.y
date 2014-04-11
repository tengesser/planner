%{
#include <iostream>

#include "maepl.h"

// stuff from flex that bison needs to know about:
extern "C" int yylex();
extern "C" int yyparse();
extern "C" FILE *yyin;

int dom = 0;
int prob = 0;

PlanningTask* t;

void yywarn();
void yyerror();
void yyerror(const char *s);
%}

%union {
  int ival;
	char *sval;
  void *ptr;
}

%start maepl

%glr-parser
%expect 8

// syntax stuff
%token <sval> NAME
%token <sval> VAR
%token LBRACK
%token RBRACK
%token MINUS
%token ERROR
%token EQUAL

// general stuff
%token DEFINE
%token DOMAIN
%token EXTENDS
%token MYDOMAIN
%token TYPES
%token CONSTS
%token PREDS
%token REQS
%token EITHER
%token EXISTS
%token FORALL

// requirement stuff
%token <sval> REQT
%token <sval> REQF

// problem stuff
%token PROBLEM
%token OBJECTS
%token INIT
%token WDES
%token WNDES
%token GOAL

// action stuff
%token ACTION
%token PARAMS
%token BYAGENT
%token PRECD
%token EFFECT
%token EVDES
%token EVNDES
%token OBSRV
%token WHEN
%token PART

// goal description stuff
%token NOT
%token AND
%token KNOWS
%token COMMK
%token OR
%token IMPLY

%type<ptr> atomicformula;
%type<ptr> goaldescription;
%type<ptr> goaldescriptionlist;
%type<ptr> effectlist;
%type<ptr> literal;
%type<ptr> literallist;
%type<ptr> effect;
%type<ptr> partition;

%%

/*********************************************************************
*  MAEPL                                                             *
*********************************************************************/

  // maepl - definition of domains and problems
  maepl:
  | domaindef maepl                    /* nothing to do here */
  | problemdef maepl                   /* nothing to do here */
  ; 

  // an epistemic planning domain
  domaindef:
    LBRACK DEFINE LBRACK DOMAIN NAME RBRACK
      { dom +=1; if (!t->newDomain($5)) yyerror(); } 
      mbextsdef mbreqsdef mbtypesdef mbconstsdef predsdef actionlist
    RBRACK
  ;

  // an epistemic planning problem
  problemdef:
    LBRACK DEFINE LBRACK PROBLEM NAME RBRACK 
      { prob +=1; if (!t->newProblem($5)) yyerror(); }
      domainndef mbobjectsdef mbinitdef worlddeflist brobslist goaldef
      { t->setDefaultObservability(); }
    RBRACK
  ;

  // definition of superdomains, or nothing
  mbextsdef:
  | extsdef                            /* nothing to do here */
  ;

  // definition of requirements, or nothing
  mbreqsdef:
  | reqsdef                            /* nothing to do here */
  ;

  // definition of requirements
  reqsdef:
    LBRACK REQS reqlist RBRACK         /* nothing to do here */
  ;

  // handling of single requirement tokens
  reqlist:
  | reqlist REQT                       /* nothing to do here */
  | reqlist REQF  
    { std::cerr << "WARNING: " << $2 << " not supported!" << std::endl; }
  ;
  
  // definition of types, or nothing
  mbtypesdef:
  | typesdef            { t->setDomainTypes(); }
  ;

  // definition of constants, or nothing
  mbconstsdef:
  | constsdef           { if (!t->setDomainObjects()) yyerror(); }
  ;
  
  // definition of objects, or nothing
  mbobjectsdef:
  | objectsdef                         /* nothing to do here */
  ;

  // definition of global initial values, or nothing
  mbinitdef:
  | initdef                            /* nothing to do here */
  ;

/*********************************************************************
*  Planning Domains                                                  *
*********************************************************************/

  extsdef:
    LBRACK EXTENDS namelist RBRACK
        { if (!t->setSuperDomains()) yyerror(); }
  ;

  // definitions of variable types
  typesdef:
    LBRACK TYPES typednamelist RBRACK  /* nothing to do here */
  ;

  // definition of single (task-independent) variables
  constsdef:
    LBRACK CONSTS typednamelist RBRACK /* nothing to do here */
  ;

  // predicates definitions
  predsdef:
    LBRACK PREDS typedpredlist RBRACK  /* nothing to do here */
  ;

  // list of available actions
  actionlist:
  | actionlist actiondef               /* nothing to do here */
  ;


/*********************************************************************
*  Planning Problems                                                 *
*********************************************************************/

  // domain to use
  domainndef:
    LBRACK MYDOMAIN NAME RBRACK
    { if (!t->setProblemDomain($3)) yyerror(); }
  ;

  // objects, typed or untyped
  objectsdef:
    LBRACK OBJECTS typednamelist RBRACK  
    { if (!t->setProblemObjects()) yyerror(); }
  ;

  // literals that are initially true in every world
  initdef:
    LBRACK INIT literallist RBRACK  
     { t->setProblemInits("", true, (Formula*)$3); }
  ;

  // list of wolrds 
  worlddeflist:
  | worlddef worlddeflist              /* nothing to do here */
  ;

  // worlds: designated / nondesignated
  worlddef:
    LBRACK WDES NAME literallist RBRACK
     { t->setProblemInits($3, true, (Formula*)$4); }
  | LBRACK WNDES NAME literallist RBRACK
     { t->setProblemInits($3, false, (Formula*)$4); }
  ;

  // goal specification
  goaldef:
    LBRACK GOAL goaldescription RBRACK
     { t->setProblemGoal((Formula*)$3); }
  ;


/*********************************************************************
*  Predicates                                                        *
*********************************************************************/

  // list of typed predicates
  typedpredlist:
  | typedpredlist typedpred            /* nothing to do here */
  ;

  // typed predicates
  typedpred:
    LBRACK NAME typedvarlist RBRACK
    { if (!t->setDomainPred($2)) yyerror(); }
  ;

/*********************************************************************
*  Actions                                                           *
*********************************************************************/

  // action definition
  actiondef:
    LBRACK ACTION NAME PARAMS LBRACK typedvarlist RBRACK
    { if (!t->setDomainAction($3)) yyerror(); }
             mbbyagent actiondefbody RBRACK
    { t->setDefaultObservability(); }
  ;

  // maybe definition of executing agent
  mbbyagent:
  | byagent
  ;

  // definition of executing agent
  byagent:
    BYAGENT NAME { if (!t->setActionAgent($2)) yyerror("FOO?"); }
  | BYAGENT VAR  { if (!t->setActionAgent($2)) yyerror("BAR?"); }
  ;

  // action definition body: one or multiple events
  actiondefbody:
    preceffect { t->setDomainEvent("default-event", true); }
  | eventlist obslist                  /* nothing to do here */
  ;

  // list of multiple events
  eventlist:
  | eventlist event                    /* nothing to do here */
  ;

  // event syntax
  // TODO: alternative non-determinism syntax - one-of?
  event:
    LBRACK EVDES NAME preceffect RBRACK 
    { if (!t->setDomainEvent($3, true)) yyerror();  }
  | LBRACK EVNDES NAME preceffect RBRACK
    { if (!t->setDomainEvent($3, false)) yyerror(); }
  ;

  // precondition and effects
  // TODO: make precs / effs optional?
  preceffect:
    PRECD goaldescription { t->setEventGoalDescription((Formula*)$2); }
    EFFECT effect         { t->setEventEffect((Formula*)$5); }
  ;

/*********************************************************************
*  observability for worlds / events                                 *
*********************************************************************/

  // observability specification list (for problem parsing)
  brobslist:
  | brobslist brobs                    /* nothing to do here */
  ;

  // observability specification list (for domain parsing)
  obslist:
  | obslist obs                        /* nothing to do here */
  ;

  // observability specification (for problem parsing)
  brobs:
    LBRACK obs RBRACK                  /* nothing to do here */
  ;

  // observability specification (for domain & problem parsing)
  obs:
    OBSRV NAME namevarlist
    { if (!t->setObservability(t->getNameList(), 
                               t->genTemplatePartition($2)))
            yyerror(); }
  | OBSRV LBRACK PART partition RBRACK namevarlist
    { if (!t->setObservability(t->getNameList(), (Partition*)$4))
            yyerror(); }
  ;

  partition:
    { $$ = newPartition(); }
  | partition LBRACK namelist RBRACK
    { if (($$ = ((Partition*)$1)->setClass(t->getNameList())) == NULL)
          yyerror(); }
  ;

/*********************************************************************
*  Formulae & Effects                                                *
*********************************************************************/

  // a formula
  goaldescription:
      atomicformula
        { $$ = $1; }
    | LBRACK AND goaldescriptionlist RBRACK
        { $$ = ((Formula*)$3)->setType(FAND); }
    | LBRACK KNOWS NAME goaldescription RBRACK
        { $$ = newFormula()->addSub((Formula*)$4)
             ->setName($3)->setType(FKNOWS); }
    | LBRACK KNOWS VAR goaldescription RBRACK
        { $$ = newFormula()->addSub((Formula*)$4)
             ->setName($3)->setType(FKNOWS); }
    | LBRACK COMMK goaldescription RBRACK
        { $$ = newFormula()->addSub((Formula*)$3)
             ->setType(FCOMMK); }
    | LBRACK OR goaldescriptionlist RBRACK
        { $$ = ((Formula*)$3)->setType(FOR); }
    | LBRACK NOT goaldescription RBRACK
        { $$ = newFormula()->addSub((Formula*)$3)
             ->setType(FNOT); }
    | LBRACK IMPLY goaldescription goaldescription RBRACK
        { $$ = newFormula()->addSub((Formula*)$3)
             ->addSub((Formula*)$4)->setType(FIMPLY); }
    | LBRACK EQUAL namevarlist RBRACK
        { $$ = newFormula()->setType(FEQUALS)
             ->setArgs(t->getNameList()); }
    | LBRACK EXISTS LBRACK typedvarlist RBRACK
        { bool w; $<ptr>$ = t->beginQuantifiedFormula(FEXISTS, w);
          if (w) yywarn(); }
        goaldescription RBRACK
        { $$ = ((Formula*)$<ptr>6)->addSub((Formula*)$7); 
          t->finishQuantifiedFormula(); }
    | LBRACK FORALL LBRACK typedvarlist RBRACK
        { bool w; $<ptr>$ = t->beginQuantifiedFormula(FFORALL, w);
          if (w) yywarn(); }
        goaldescription RBRACK
        { $$ = ((Formula*)$<ptr>6)->addSub((Formula*)$7);
          t->finishQuantifiedFormula(); }
  ;

  // an effect (action & events)
  effect:
    literal
      { $$ = $1; }
  | LBRACK AND effectlist RBRACK
      { $$ = ((Formula*)$3)->setType(FAND); }
  | LBRACK WHEN goaldescription effect RBRACK
      { $$ = newCond((Formula*)$3, (Formula*)$4); }
  | LBRACK FORALL LBRACK typedvarlist RBRACK
      { bool w; $<ptr>$ = t->beginQuantifiedFormula(FFORALL, w);
        if (w) yywarn(); }
    effect RBRACK
      { $$ = ((Formula*)$<ptr>6)->addSub((Formula*)$7);
        t->finishQuantifiedFormula();}
  ;

  // list of goaldescriptionS
  goaldescriptionlist:
    { $$ = newFormula(); }
  | goaldescriptionlist goaldescription
    { $$ = ((Formula*)$1)->addSub((Formula*)$2); }
  ;

  // list of effects
  effectlist:
    { $$ = newFormula(); }
  | effectlist effect
    { $$ = ((Formula*)$1)->addSub((Formula*)$2); }
  ;

  // list of literals
  literallist:
    { $$ = newFormula()->setType(LLIST); }
  | literallist literal
    { $$ = ((Formula*)$1)->addSub((Formula*)$2); }
  ;

  // a literal
  literal:
    atomicformula { $$ = $1; }
  | LBRACK NOT atomicformula RBRACK
      { $$ = newFormula()->setType(FNOT)->addSub((Formula*)$3); }
  ;

  // an atomic formula
  atomicformula:
    LBRACK NAME namevarlist RBRACK
      { $$ = newFormula()->setType(FATOM)->setName($2)
             ->setArgs(t->getNameList()); 
        if (!t->checkAtom((Formula*)$$)) yyerror();}
  ;

/*********************************************************************
*  names, types & variables                                          *
*********************************************************************/

  // typed list of names allowing inheritance
  typednamelist:
    namelist  { t->finishList(); }
  | namelist MINUS type typednamelist  /* nothing to do here */
  ;

  // typed list of variables
  typedvarlist:
    varlist  { t->finishList(); }                      
  | varlist MINUS type typedvarlist    /* nothing to do here */
  ;

  // type
  type:
    NAME  { t->setParseParent(); t->inputName($1); t->setParseChild(); }
  | LBRACK EITHER
       { t->setParseParent(); } namelist { t->setParseChild(); }
    RBRACK
  ;

  // untyped list of names / vars
  namevarlist:
  | namevarlist NAME  { t->inputName($2); }
  | namevarlist VAR   { t->inputName($2); }
  ;

  // untyped list of names
  namelist:
  | namelist NAME { t->inputName($2); }
  ;

  // untyped list of variables
  varlist:
  | varlist VAR   { t->inputName($2); }
  ;
  
%%


bool bparse(char* filename, PlanningTask * task) {

  // open file
	FILE *myfile;
  myfile = fopen(filename, "r");
    
  // make sure it is valid:
  if (!myfile)
  {
		  std::cerr << "ERROR: I can't open the file "
                << filename << "." << std::endl;
      return false;
  }

	// set flex to read from it instead of defaulting to STDIN:
	yyin =  myfile;

  extern int yylineno;
  yylineno = 1;

  // build planning tasks here
  t = task;
	
	// parse through the input until there is no more:
	do {
		yyparse();
	} while (!feof(yyin));
	
  return true;
}

void yywarn()
{
  extern int yylineno;	// defined and maintained in lex.c
  std::cerr << " on line " << yylineno << std::endl;
}

void yyerror()
{
  yywarn();
  exit(1);
}

void yyerror(const char * s)
{
  extern char *yytext;	// defined and maintained in lex.c
  std::cerr << "ERROR: " << s << " at symbol \"" << yytext;
  yywarn();
  exit(1);
}
