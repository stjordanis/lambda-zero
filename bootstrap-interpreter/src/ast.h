typedef enum {SYMBOL, LAMBDA, APPLICATION, NATURAL, BUILTIN} NodeType;

// ====================================
// Functions to get a value from a node
// ====================================

static inline NodeType getNodeType(Node* node) {return (NodeType)getType(node);}
static inline String getLexeme(Node* node) {return getTag(node).lexeme;}
static inline Node* getParameter(Node* lambda) {return getLeft(lambda);}
static inline Node* getBody(Node* lambda) {return getRight(lambda);}

static inline unsigned long long getDebruijnIndex(Node* reference) {
    assert(getValue(reference) > 0);
    return (unsigned long long)(getValue(reference) - 1);
}

static inline unsigned long long getGlobalIndex(Node* reference) {
    assert(getValue(reference) < 0);
    return (unsigned long long)(-getValue(reference) - 1);
}

// =============================================
// Functions to test if a node is a certain type
// =============================================

static inline bool isSameLexeme(Node* a, Node* b) {
    return isSameString(getLexeme(a), getLexeme(b));
}

static inline bool isThisLexeme(Node* node, const char* lexeme) {
    return isThisString(getLexeme(node), lexeme);
}

static inline bool isThisLeaf(Node* leaf, const char* lexeme) {
    return isLeaf(leaf) && isThisLexeme(leaf, lexeme);
}

static inline bool isSymbol(Node* node) {return getNodeType(node) == SYMBOL;}
static inline bool isLambda(Node* node) {return getNodeType(node) == LAMBDA;}
static inline bool isNatural(Node* node) {return getNodeType(node) == NATURAL;}
static inline bool isBuiltin(Node* node) {return getNodeType(node) == BUILTIN;}

static inline bool isApplication(Node* node) {
    return getNodeType(node) == APPLICATION;
}

static inline bool isGlobalReference(Node* node) {
    return isSymbol(node) && getValue(node) < 0;
}

static inline bool isEOF(Node* node) {return isThisLeaf(node, "\0");}
static inline bool isBlank(Node* node) {return isThisLeaf(node, "_");}

static inline bool isIdentity(Node* node) {
    return isLambda(node) && isSymbol(getBody(node)) &&
        isSameLexeme(getParameter(node), getBody(node));
}

static inline bool isSection(Node* node) {
    return isThisLexeme(node, "_._") ||
        isThisLexeme(node, "_.") || isThisLexeme(node, "._");
}

// ================================
// Functions to construct new nodes
// ================================


static inline Node* newSymbol(Tag tag, void* rules) {
    return newLeaf(tag, SYMBOL, 0, rules);
}

static inline Node* newName(Tag tag) {return newSymbol(tag, NULL);}

static inline Node* newBlank(Tag tag) {return newName(renameTag(tag, "_"));}

static inline Node* newBlankReference(Tag tag, unsigned long long debruijn) {
    return newLeaf(renameTag(tag, "_"), SYMBOL, (long long)debruijn, NULL);
}

static inline Node* newNatural(Tag tag, long long n) {
    return newLeaf(tag, NATURAL, n, NULL);
}

static inline Node* newBuiltin(Tag tag, long long n) {
    return newLeaf(tag, BUILTIN, n, NULL);
}

static inline Node* newLambda(Tag tag, Node* parameter, Node* body) {
    // we could make the left child of a lambda VOID, but storing a parameter
    // lets us decouple the location of the lambda from the parameter name,
    // which is useful in cases like string literals where we need to point
    // to the string literal for error messages, but we prefer not to make the
    // parameter name be the string literal
    assert(isSymbol(parameter) && getValue(parameter) == 0);
    return newBranch(tag, LAMBDA, parameter, body);
}

static inline Node* newApplication(Tag tag, Node* left, Node* right) {
    return newBranch(tag, APPLICATION, left, right);
}

static inline Node* newNil(Tag tag) {return newName(renameTag(tag, "[]"));}

static inline Node* prepend(Tag tag, Node* item, Node* list) {
    Node* operator = newName(renameTag(tag, "::"));
    return newApplication(tag, newApplication(tag, operator, item), list);
}

static inline Node* newBoolean(Tag tag, bool value) {
    return newLambda(tag, newBlank(tag), newLambda(tag, newBlank(tag),
        value ? newBlankReference(tag, 1) : newBlankReference(tag, 2)));
}

// ====================================
// Builtins
// ====================================

enum BuiltinCode {PLUS, MINUS, TIMES, DIVIDE, MODULUS,
      EQUAL, NOTEQUAL, LESSTHAN, GREATERTHAN, LESSTHANOREQUAL,
      GREATERTHANOREQUAL, ERROR, UNDEFINED, EXIT, PUT, GET};

static inline Node* convertOperator(Tag tag) {
    // names in builtins must line up with codes in BuiltinCode, except
    // EXIT, PUT, GET which don't have accessible names
    static const char* const builtins[] =
        {"+", "-", "*", "//", "%", "=", "!=", "<", ">", "<=", ">=", "error"};
    for (unsigned int i = 0; i < sizeof(builtins)/sizeof(char*); ++i)
        if (isThisString(tag.lexeme, builtins[i]))
            return newBuiltin(tag, i);
    return newName(tag);
}

static inline Node* newPrinter(Tag tag) {
    Node* put = newBuiltin(renameTag(tag, "put"), PUT);
    Node* fold = newName(renameTag(tag, "fold"));
    Node* unit = newName(renameTag(tag, "()"));
    Node* blank = newBlankReference(tag, 1);

    Node* body = newApplication(tag, newApplication(tag, newApplication(tag,
        fold, blank), put), unit);
    return newLambda(tag, newBlank(tag), body);
}
