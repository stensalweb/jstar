#ifndef AST_H
#define AST_H

#include "blconf.h"
#include "linkedlist.h"

#include <stdbool.h>
#include <stdlib.h>

typedef enum Operator {
    PLUS,
    MINUS,
    MULT,
    DIV,
    MOD,
    EQ,
    NEQ,
    AND,
    OR,
    NOT,
    GT,
    GE,
    LT,
    LE,
    IS,
    LENGTH,
    STRINGOP
} Operator;

typedef enum ExprType {
    BINARY,
    UNARY,
    ASSIGN,
    NUM_LIT,
    BOOL_LIT,
    STR_LIT,
    VAR_LIT,
    NULL_LIT,
    EXPR_LST,
    CALL_EXPR,
    EXP_EXPR,
    SUPER_LIT,
    ACCESS_EXPR,
    ARR_LIT,
    TUPLE_LIT,
    ARR_ACC,
    TERNARY,
    COMP_ASSIGN,
    ANON_FUNC,
} ExprType;

typedef struct Identifier {
    size_t length;
    const char *name;
} Identifier;

BLANG_API Identifier *newIdentifier(size_t length, const char *name);
BLANG_API bool identifierEquals(Identifier *id1, Identifier *id2);

typedef struct Expr Expr;
typedef struct Stmt Stmt;

struct Expr {
    int line;
    ExprType type;
    union {
        struct {
            Operator op;
            Expr *left, *right;
        } bin;
        struct {
            Operator op;
            Expr *operand;
        } unary;
        struct {
            Expr *lval, *rval;
        } assign;
        struct {
            Operator op;
            Expr *lval, *rval;
        } compundAssign;
        struct {
            size_t length;
            const char *str;
        } str;
        struct {
            Identifier id;
        } var;
        struct {
            LinkedList *lst;
        } exprList;
        struct {
            Expr *callee, *args;
        } callExpr;
        struct {
            Expr *base, *exp;
        } expExpr;
        struct {
            Expr *left;
            Identifier id;
        } accessExpr;
        struct {
            Expr *left;
            Expr *index;
        } arrAccExpr;
        struct {
            Expr *exprs;
        } arr;
        struct {
            Expr *exprs;
        } tuple;
        struct {
            Expr *cond;
            Expr *thenExpr;
            Expr *elseExpr;
        } ternary;
        struct {
            Stmt *func;
        } anonFunc;
        double num;
        bool boolean;
    };
};

BLANG_API Expr *newBinary(int line, Operator op, Expr *l, Expr *r);
BLANG_API Expr *newAssign(int line, Expr *lval, Expr *rval);
BLANG_API Expr *newUnary(int line, Operator op, Expr *operand);
BLANG_API Expr *newNullLiteral(int line);
BLANG_API Expr *newSuperLiteral(int line);
BLANG_API Expr *newNumLiteral(int line, double num);
BLANG_API Expr *newBoolLiteral(int line, bool boolean);
BLANG_API Expr *newArrayAccExpr(int line, Expr *left, Expr *index);
BLANG_API Expr *newStrLiteral(int line, const char *str, size_t len);
BLANG_API Expr *newVarLiteral(int line, const char *str, size_t len);
BLANG_API Expr *newArrLiteral(int line, Expr *exprs);
BLANG_API Expr *newTupleLiteral(int line, Expr *exprs);
BLANG_API Expr *newExprList(int line, LinkedList *exprs);
BLANG_API Expr *newCallExpr(int line, Expr *callee, LinkedList *args);
BLANG_API Expr *newExpExpr(int line, Expr *base, Expr *exp);
BLANG_API Expr *newAccessExpr(int line, Expr *left, const char *name, size_t length);
BLANG_API Expr *newTernary(int line, Expr *cond, Expr *thenExpr, Expr *elseExpr);
BLANG_API Expr *newCompoundAssing(int line, Operator op, Expr *lval, Expr *rval);
BLANG_API Expr *newAnonymousFunc(int line, bool vararg, LinkedList *args, LinkedList *defArgs, Stmt *body);

BLANG_API void freeExpr(Expr *e);

typedef enum StmtType {
    IF,
    FOR,
    WHILE,
    FOREACH,
    BLOCK,
    RETURN_STMT,
    EXPR,
    VARDECL,
    FUNCDECL,
    NATIVEDECL,
    CLASSDECL,
    IMPORT,
    TRY_STMT,
    EXCEPT_STMT,
    RAISE_STMT,
    CONTINUE_STMT,
    BREAK_STMT
} StmtType;

struct Stmt {
    int line;
    StmtType type;
    union {
        struct {
            Expr *cond;
            Stmt *thenStmt, *elseStmt;
        } ifStmt;
        struct {
            Stmt *init;
            Expr *cond, *act;
            Stmt *body;
        } forStmt;
        struct {
            Stmt *var;
            Expr *iterable;
            Stmt *body;
        } forEach;
        struct {
            Expr *cond;
            Stmt *body;
        } whileStmt;
        struct {
            Expr *e;
        } returnStmt;
        struct {
            LinkedList *stmts;
        } blockStmt;
        struct {
            bool isUnpack;
            LinkedList *ids;
            Expr *init;
        } varDecl;
        struct {
            Identifier id;
            LinkedList *formalArgs, *defArgs;
            bool isVararg;
            Stmt *body;
        } funcDecl;
        struct {
            Identifier id;
            LinkedList *formalArgs, *defArgs;
            bool isVararg;
        } nativeDecl;
        struct {
            Identifier id;
            Expr *sup;
            LinkedList *methods;
        } classDecl;
        struct {
            LinkedList *modules;
            Identifier as;
            LinkedList *impNames;
        } importStmt;
        struct {
            Stmt *block;
            LinkedList *excs;
            Stmt *ensure;
        } tryStmt;
        struct {
            Expr *cls;
            Identifier var;
            Stmt *block;
        } excStmt;
        struct {
            Expr *exc;
        } raiseStmt;
        Expr *exprStmt;
    };
};

BLANG_API Stmt *newFuncDecl(int line, bool vararg, size_t length, const char *id, LinkedList *args, LinkedList *defArgs, Stmt *body);
BLANG_API Stmt *newNativeDecl(int line, bool vararg, size_t length, const char *id, LinkedList *args, LinkedList *defArgs);
BLANG_API Stmt *newImportStmt(int line, LinkedList *modules, LinkedList *impNames, const char *as, size_t asLength);
BLANG_API Stmt *newClassDecl(int line, size_t clength, const char *cid, Expr *sup, LinkedList *methods);
BLANG_API Stmt *newExceptStmt(int line, Expr *cls, size_t vlen, const char *var, Stmt *block);
BLANG_API Stmt *newForStmt(int line, Stmt *init, Expr *cond, Expr *act, Stmt *body);
BLANG_API Stmt *newVarDecl(int line, bool isUnpack, LinkedList *ids, Expr *init);
BLANG_API Stmt *newTryStmt(int line, Stmt *blck, LinkedList *excs, Stmt *ensure);
BLANG_API Stmt *newIfStmt(int line, Expr *cond, Stmt *thenStmt, Stmt *elseStmt);
BLANG_API Stmt *newForEach(int line, Stmt *varDecl, Expr *iter, Stmt *body);
BLANG_API Stmt *newWhileStmt(int line, Expr *cond, Stmt *body);
BLANG_API Stmt *newBlockStmt(int line, LinkedList *list);
BLANG_API Stmt *newReturnStmt(int line, Expr *e);
BLANG_API Stmt *newRaiseStmt(int line, Expr *e);
BLANG_API Stmt *newExprStmt(int line, Expr *e);
BLANG_API Stmt *newContinueStmt(int line);
BLANG_API Stmt *newBreakStmt(int line);

BLANG_API void freeStmt(Stmt *s);

#endif
