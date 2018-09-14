#include "lib/tree.h"
#include "lib/stack.h"
#include "ast.h"
#include "errors.h"
#include "operators.h"
#include "patterns.h"
#include "define.h"
#include "brackets.h"

static Node* reduceApply(Node* operator, Node* left, Node* right) {
    return newApplication(getTag(operator), left, right);
}

static Node* reduceInfix(Node* operator, Node* left, Node* right) {
    return reduceApply(operator, reduceApply(operator,
        convertOperator(operator), left), right);
}

static Node* reducePrefix(Node* operator, Node* left, Node* right) {
    (void)left;
    return reduceApply(operator, convertOperator(operator), right);
}

static Node* reduceNegate(Node* operator, Node* left, Node* right) {
    (void)left;
    Tag tag = getTag(operator);
    return newApplication(tag, newName(renameTag(tag, "negate")), right);
}

static Node* reduceReserved(Node* operator, Node* left, Node* right) {
    syntaxError("reserved operator", operator);
    return left == NULL ? right : left; // suppress unused parameter warning
}

static void shiftInfix(Stack* stack, Node* operator) {
    Node* top = peek(stack, 0);
    if (isOperator(top) && !isOpenParen(top))
        syntaxError("missing left argument to", operator);   // e.g. "5 - * 2"
    push(stack, operator);
}

static void shiftPrefix(Stack* stack, Node* operator) {
    push(stack, operator);
}

static void shiftWhitespace(Stack* stack, Node* operator) {
    // ignore whitespace after operators
    if (!isOperator(peek(stack, 0)))
        push(stack, operator);
}

void initOperators(void) {
    // default (must be first)
    addOperator("", 14, 14, IN, L, shiftInfix, reduceInfix);

    // syntactic operators
    addOperator("\0", 0, 0, CLOSE, L, shiftBracket, reduceEOF);
    addOperator("(", 22, 0, OPENCALL, L, shiftOpenCall, reduceUnmatched);
    addOperator("(", 22, 0, OPEN, L, push, reduceUnmatched);
    addOperator(")", 0, 22, CLOSE, R, shiftBracket, reduceParentheses);
    addOperator("[", 22, 0, OPENCALL, L, shiftOpenCall, reduceUnmatched);
    addOperator("[", 22, 0, OPEN, L, push, reduceUnmatched);
    addOperator("]", 0, 22, CLOSE, R, shiftBracket, reduceSquareBrackets);
    addOperator("{", 22, 0, OPEN, L, shiftOpenCurly, reduceUnmatched);
    addOperator("}", 0, 22, CLOSE, R, shiftBracket, reduceCurlyBrackets);
    addOperator("|", 1, 1, IN, N, shiftInfix, reduceReserved);
    addOperator(",", 2, 2, IN, L, shiftInfix, reduceApply);
    addOperator("\n", 3, 3, IN, R, shiftWhitespace, reduceApply);
    addOperator(":=", 3, 3, IN, R, shiftInfix, reduceDefine);
    addOperator("::=", 3, 3, IN, R, shiftInfix, reduceADTDefinition);
    addOperator(";", 4, 4, IN, L, shiftInfix, reducePatternLambda);
    addOperator("->", 5, 5, IN, R, shiftInfix, reduceLambda);

    // conditional operators
    addOperator("||", 6, 6, IN, R, shiftInfix, reduceInfix);
    addOperator("?", 7, 7, IN, R, shiftInfix, reduceInfix);
    addOperator("?||", 7, 7, IN, R, shiftInfix, reduceInfix);

    // monadic operators
    addOperator(">>=", 8, 8, IN, L, shiftInfix, reduceInfix);

    // logical operators
    addOperator("<=>", 9, 9, IN, N, shiftInfix, reduceInfix);
    addOperator("=>", 10, 10, IN, N, shiftInfix, reduceInfix);
    addOperator("\\/", 11, 11, IN, R, shiftInfix, reduceInfix);
    addOperator("/\\", 12, 12, IN, R, shiftInfix, reduceInfix);

    // comparison/test operators
    addOperator("=", 13, 13, IN, N, shiftInfix, reduceInfix);
    addOperator("!=", 13, 13, IN, N, shiftInfix, reduceInfix);
    addOperator("=*=", 13, 13, IN, N, shiftInfix, reduceInfix);
    addOperator("=?=", 13, 13, IN, N, shiftInfix, reduceInfix);
    addOperator("<", 13, 13, IN, N, shiftInfix, reduceInfix);
    addOperator(">", 13, 13, IN, N, shiftInfix, reduceInfix);
    addOperator("<=", 13, 13, IN, N, shiftInfix, reduceInfix);
    addOperator(">=", 13, 13, IN, N, shiftInfix, reduceInfix);
    addOperator("=<", 13, 13, IN, N, shiftInfix, reduceInfix);
    addOperator("~<", 13, 13, IN, N, shiftInfix, reduceInfix);
    addOperator("<:", 13, 13, IN, N, shiftInfix, reduceInfix);
    addOperator("<*=", 13, 13, IN, N, shiftInfix, reduceInfix);
    addOperator(":", 13, 13, IN, N, shiftInfix, reduceInfix);
    addOperator("!:", 13, 13, IN, N, shiftInfix, reduceInfix);
    addOperator("~", 13, 13, IN, N, shiftInfix, reduceInfix);

    // precedence 14: default
    // arithmetic/list operators
    addOperator("|:", 14, 14, IN, N, shiftInfix, reduceInfix);
    addOperator("..", 15, 15, IN, N, shiftInfix, reduceInfix);
    addOperator("::", 16, 16, IN, R, shiftInfix, reduceInfix);
    addOperator("&", 16, 16, IN, L, shiftInfix, reduceInfix);
    addOperator("+", 16, 16, IN, L, shiftInfix, reduceInfix);
    addOperator("\\./", 16, 16, IN, L, shiftInfix, reduceInfix);
    addOperator("++", 16, 16, IN, R, shiftInfix, reduceInfix);
    addOperator("-", 16, 16, IN, L, shiftInfix, reduceInfix);
    addOperator("\\", 16, 16, IN, L, shiftInfix, reduceInfix);
    addOperator("*", 17, 17, IN, L, shiftInfix, reduceInfix);
    addOperator("**", 17, 17, IN, R, shiftInfix, reduceInfix);
    addOperator("/", 17, 17, IN, L, shiftInfix, reduceInfix);
    addOperator("%", 17, 17, IN, L, shiftInfix, reduceInfix);

    // functional operators
    addOperator("<>", 19, 19, IN, R, shiftInfix, reduceInfix);

    // space operator
    addOperator(" ", 20, 20, IN, L, shiftWhitespace, reduceApply);

    // prefix operators
    addOperator("-", 21, 21, PRE, L, shiftPrefix, reduceNegate);
    addOperator("--", 21, 21, PRE, L, shiftPrefix, reducePrefix);
    addOperator("!", 21, 21, PRE, L, shiftPrefix, reducePrefix);
    addOperator("#", 21, 21, PRE, L, shiftPrefix, reducePrefix);
    addOperator("*", 21, 21, PRE, L, shiftPrefix, reduceAsterisk);

    // precedence 22: parentheses/brackets
    addOperator("^", 23, 23, IN, R, shiftInfix, reduceInfix);
    addOperator("^^", 23, 23, IN, L, shiftInfix, reduceInfix);
    addOperator(".", 24, 24, IN, L, shiftInfix, reduceInfix);
    addOperator("`", 25, 25, PRE, L, shiftPrefix, reducePrefix);
}
