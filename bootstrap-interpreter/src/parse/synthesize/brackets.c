#include <string.h>
#include "shared/lib/tree.h"
#include "parse/shared/errors.h"
#include "parse/shared/ast.h"
#include "symbols.h"

// isEOF returns true for both start of file and end of file nodes
bool isEOF(Node* n) {
    String lexeme = getTag(n).lexeme;
    return isOperator(n) && (lexeme.length == 0 || lexeme.start[0] == '\0');
}

static unsigned int getCommaListLength(Node* node) {
    return !isCommaPair(node) ? 1 : 1 + getCommaListLength(getLeft(node));
}

static Node* applyToCommaList(Tag tag, Node* base, Node* arguments) {
    if (!isCommaPair(arguments))
        return Juxtaposition(tag, base, arguments);
    return Juxtaposition(tag, applyToCommaList(tag, base,
        getLeft(arguments)), getRight(arguments));
}

static Node* newSpineName(Node* node, const char* name, unsigned int length) {
    unsigned int maxLength = (unsigned int)strlen(name);
    syntaxErrorIf(length > maxLength, "too many arguments", node);
    return Name(newTag(newString(name, length), getTag(node).location));
}

static Node* newTuple(Node* open, Node* commaList) {
    const char* lexeme = ",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,";
    Node* name = newSpineName(open, lexeme, getCommaListLength(commaList) - 1);
    return applyToCommaList(getTag(open), name, commaList);
}

Node* reduceOpenParenthesis(Node* open, Node* before, Node* contents) {
    Tag tag = getTag(open);
    if (contents == NULL) {
        Node* unit = FixedName(tag, "()");
        return before == NULL ? unit : Juxtaposition(tag, before, unit);
    }
    if (isDefinition(contents))
        syntaxError("missing scope for definition", contents);
    if (before != NULL)
        return applyToCommaList(tag, before, contents);
    if (isCommaPair(contents))
        return newTuple(open, contents);
    if (isArrow(contents))
        setVariety(contents, LOCKEDARROW);
    if (isJuxtaposition(contents))
        setTag(contents, tag);
    return contents;
}

Node* reduceOpenSquareBracket(Node* open, Node* before, Node* contents) {
    Tag tag = getTag(open);
    if (contents == NULL) {
        syntaxErrorIf(before != NULL, "missing argument to", open);
        return Nil(tag);
    }
    if (before != NULL) {
        const char* lexeme = "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[";
        Node* name = newSpineName(open, lexeme, getCommaListLength(contents));
        Node* base = Juxtaposition(tag, name, before);
        return applyToCommaList(tag, base, contents);
    }
    Node* list = Nil(tag);
    if (!isCommaPair(contents))
        return prepend(tag, contents, list);
    for(; isCommaPair(contents); contents = getLeft(contents))
        list = prepend(tag, getRight(contents), list);
    return prepend(tag, contents, list);
}

Node* reduceOpenBrace(Node* open, Node* before, Node* patterns) {
    syntaxErrorIf(before != NULL, "invalid operand before", open);
    if (patterns == NULL)
        return SetBuilder(renameTag(getTag(open), "{}"), VOID);
    return SetBuilder(getTag(open), newTuple(open, patterns));
}

Node* reduceOpenFile(Node* open, Node* before, Node* contents) {
    syntaxErrorIf(before != NULL, "invalid operand before", open);
    syntaxErrorIf(!isEOF(open), "missing close for", open);
    syntaxErrorIf(isCommaPair(contents), "comma not inside brackets", contents);
    return contents;
}
