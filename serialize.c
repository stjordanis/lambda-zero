#include <assert.h>
#include "lib/lltoa.h"
#include "lib/tree.h"
#include "lib/array.h"
#include "ast.h"
#include "closure.h"
#include "lex.h"
#include "serialize.h"

static inline bool isDefinition(Node* node) {
    return isApplication(node) && isThisToken(node, "=");
}

static inline bool isNewline(Node* node) {
    return isLeafNode(node) && isThisToken(node, "\n");
}

void debug(const char* message) {
    fputs(message, stderr);
}

void debugLine(void) {
    debug("======================================");
    debug("======================================\n");
}

void serializeInteger(long long n, FILE* stream) {
    char buffer[3 * sizeof(long long)];
    fputs(lltoa(n, buffer, 10), stream);
}

void debugInteger(long long n) {
    serializeInteger(n, stderr);
}

void serializeAST(Node* node, FILE* stream) {
    if (node == NULL) {
        fputs("NULL", stream);     // for debugging
    } else if (isBranchNode(node)) {
        fputs("(", stream);
        serializeAST(getLeft(node), stream);
        fputs(isDefinition(node) ? " = " : " ", stream);
        serializeAST(getRight(node), stream);
        fputs(")", stream);
    } else if (isParameter(node)) {
        printToken(node, stream);
        fputs(" ->", stream);
    } else if (isInteger(node)) {
        // builtins create integers, so not all integers will exist in input
        serializeInteger(getInteger(node), stream);
    } else if (isNewline(node)) {
        fputs("\\n", stream);
    } else if (isReference(node)) {
        printToken(node, stream);
        fputs("#", stream);
        serializeInteger((long long)getDebruijnIndex(node), stream);
    } else if (isGlobal(node)) {
        printToken(node, stream);
        fputs("#G", stream);
    } else {
        printToken(node, stream);
    }
}

void debugAST(Node* node) {
    serializeAST(node, stderr);
}

void debugStack(Stack* stack, Node* (*select)(Node*)) {
    debug("[");
    for (Iterator* it = iterate(stack); !end(it); it = next(it)) {
        debugAST(select == NULL ? cursor(it) : select(cursor(it)));
        if (!end(next(it)))
            debug(", ");
    }
    debug("]");
}

void serializeNode(Node* node, Node* locals, const Array* globals,
        unsigned int depth, FILE* stream) {
    if (isApplication(node)) {
        fputs("(", stream);
        serializeNode(getLeft(node), locals, globals, depth, stream);
        fputs(" ", stream);
        serializeNode(getRight(node), locals, globals, depth, stream);
        fputs(")", stream);
    } else if (isLambda(node)) {
        fputs("(", stream);
        printToken(getParameter(node), stream);
        fputs(" -> ", stream);
        serializeNode(getBody(node), locals, globals, depth + 1, stream);
        fputs(")", stream);
    } else if (isReference(node)) {
        unsigned long long debruijn = getDebruijnIndex(node);
        if (debruijn > depth) {
            Closure* next = getListElement(locals, debruijn - depth - 1);
            serializeNode(getTerm(next), getLocals(next), globals, 0, stream);
        } else {
            printToken(node, stream);
        }
    } else if (isInteger(node)) {
        serializeInteger(getInteger(node), stream);
    } else if (isBuiltin(node)) {
        printToken(node, stream);
    } else if (isGlobal(node)) {
        Node* value = elementAt(globals, getGlobalIndex(node));
        serializeNode(value, locals, globals, 0, stream);
    } else {
        assert(false);
    }
}

void serialize(Closure* closure, const Array* globals, FILE* stream) {
    serializeNode(getTerm(closure), getLocals(closure), globals, 0, stream);
}
