#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// Lexer returns tokens (0-255 in the ascii set) if it isn't one of the known things below

#include <cctype>
enum Token {
	// EOF
	tok_eof = -1,

	// First Class Commands
	tok_def = -2,
	tok_extern = -3,

	// primary
	tok_identifier = -4,
	tok_number = -5,

};

static std::string IdentifierStr; // Filled in if tok_identifier
static double NumeralVal; // Filled in if tok_number
						  // oops all doubles lol

static int getToken() {
	static int LastChar = ' ';

	// Ignore Whites
	while (isspace(LastChar))
		LastChar = getchar();
	
	if (isalpha(LastChar)) {
		IdentifierStr = LastChar;
		while (isalnum(LastChar = getchar()))
			IdentifierStr += LastChar;
		if (IdentifierStr == "def")
			return tok_def;
		if (IdentifierStr == "extern")
			return tok_extern;
		return tok_identifier;
	}
	if (isdigit(LastChar || LastChar == '.')) {
		std::string NumStr;
		do {
			NumStr += LastChar;
			LastChar = getchar();
		} while (isdigit(LastChar || LastChar == '.'));

		NumeralVal = strtod(NumStr.c_str(), 0);
		return tok_number;
	}
	if (LastChar == '#') {
		do {
		LastChar = getchar();
		} while (isdigit(LastChar) || LastChar == '.');
		if (LastChar != EOF)
			return getToken();
	}
	if (LastChar == EOF)
		return tok_eof;
	int ThisChar = LastChar;
	LastChar = getchar();
	return ThisChar;
}

// ExpAST - Base Class for All Expression Nodes in the AST

namespace {

class ExprAST {
public:
    virtual ~ExprAST() = default;
};

// NumberExprAST - Expression Class for Numeric Literals like "1.0"
class NumberExprAST : public ExprAST {
    double Val;
public:
    NumberExprAST(double Val) : Val(Val) {}
};

// VariableExprAST - Expression Class for Referencing a Variable, like "a"
class VariableExprAST : public ExprAST {
    std::string Name;
public:
    VariableExprAST(const std::string &Name) : Name(Name) {}
};

// BinaryExprAST - Expression Class for a Binary Operator, like "+"
class BinaryExprAST: public ExprAST {
    char Op;
    std::unique_ptr<ExprAST> LHS, RHS;
public:
    BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
        : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

// CallExprAST - Expression Class for Function Calls
class CallExprAST : public ExprAST {
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;
public:
    CallExprAST(const std::string &Callee,
                std::vector<std::unique_ptr<ExprAST>> Args)
        : Callee(Callee), Args(std::move(Args)) {}
};

// PrototypeAST - This Class Represents the "Prototype" for a Function
// expressing it's name, argument signature (names and total number of args)
class PrototypeAST {
    std::string Name;
    std::vector<std::string> Args;
public:
    PrototypeAST(const std::string &Name, std::vector<std::string> Args)
        : Name(Name), Args(std::move(Args)) {}
    const std::string &getName() const { return Name; }
};

// FunctionAST - This Class Represents a Function Definition itself
class FunctionAST {
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;
public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                std::unique_ptr<ExprAST> Body)
        : Proto(std::move(Proto)), Body(std::move(Body)) {}
};

}

// Parser - Now that we have all the AST componets  defined, we need to take
// tokens from our lexer and build an AST out of them

// Buffer for one token and fn to get next token from lexer
static int CurrentToken;
static int getNextToken() {
    return CurrentToken = getToken();
}

// LogError* - Helpers to make errors more readable
std::unique_ptr<ExprAST> LogError(const char *Str) {
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
    LogError(Str);
    return nullptr;
}

static std::unique_ptr<ExprAST> ParseExpression();

// Now lets do some basic expression parsing
// Numerical Expressions first
static std::unique_ptr<ExprAST> ParseNumberExpr() {
    auto Result = std::make_unique<NumberExprAST>(NumeralVal);
    getNextToken(); // This will be a number or a syntax erro
    return std::move(Result);
}

// Parens Expression
static std::unique_ptr<ExprAST> ParseParenExpr() {
    getNextToken(); // consoom one (
    auto V = ParseExpression();
    if (!V)
        return nullptr;
    if (CurrentToken != ')')
        return LogError("expected '(' ya dingus");
    getNextToken(); // now consoom the )
    return V;
}

// Identifier Expressions
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
    std::string IdentifierName = IdentifierStr;

    getNextToken(); // consoom identifier

    if (CurrentToken != '(') // Simple Variable Ref
        return std::make_unique<VariableExprAST>(IdentifierName);

    // Call
    getNextToken(); // consoom (
    std::vector<std::unique_ptr<ExprAST>> Args;
    if (CurrentToken != ')') {
        while (true) {
            if (auto Arg = ParseExpression())
                Args.push_back(std::move(Arg));
            else
             return nullptr;

            if (CurrentToken == ')')
                break;

            if (CurrentToken != ',')
                return LogError("Expected ')' or ',' in argument list");
            getNextToken();
        }
    }
    getNextToken(); // consoom )

    return std::make_unique<CallExprAST>(IdentifierName, std::move(Args));
}

// Primary Expressions
static std::unique_ptr<ExprAST> ParsePrimary() {
    switch (CurrentToken) {
        default:
            return LogError("Unknown token when expecting an expression");
        case tok_identifier:
            return ParseIdentifierExpr();
        case tok_number:
            return ParseNumberExpr();
        case '(':
            return ParseParenExpr();
    }
}

// Binary Expression parsing
static std::map<char, int> BinopPrecedence;

// Get precedence of the pending binary operator token
static int GetTokPrecedence(){
    if (!isascii(CurrentToken))
        return -1;
    // Make sure it's an existent binary operator
    int TokPrec = BinopPrecedence[CurrentToken];
    if (TokPrec <= 0) return -1;
    return TokPrec;
}

// Bin Op RHS
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS) {
    while (true) {
        int TokPrec = GetTokPrecedence();
    // If this binop binds at least as tightly as the current, consoom it
    // Move on if otherwise
    if (TokPrec < ExprPrec)
        return LHS;
    
    int BinOp = CurrentToken;
    getNextToken(); // consoom binop

    // Parse the primary expression after the binary operator
    auto RHS = ParsePrimary();
    if (!RHS)
        return nullptr;
    int NextPrec = GetTokPrecedence();
    if (TokPrec < NextPrec) {
        RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
        if (!RHS)
            return nullptr;
    }
    // Merge LHS/RHS
    LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
    }
}


// Expression
//   ::= primary binary operators
static std::unique_ptr<ExprAST> ParseExpression() {
   auto LHS = ParsePrimary();
   if (!LHS)
       return nullptr;
   return ParseBinOpRHS(0, std::move(LHS));
}

// Prototype
//  ::= id '(' id* ')'
static std::unique_ptr<PrototypeAST> ParsePrototype() {
    if (CurrentToken != tok_identifier)
        return LogErrorP("Expected function name in prototype");
    std::string FnName = IdentifierStr;
    getNextToken();
    if (CurrentToken != '(')
        return LogErrorP("Expected '(' in prototype");
    // Read the list of argument names
    std::vector<std::string> ArgNames;
    while (getNextToken() == tok_identifier)
        ArgNames.push_back(IdentifierStr);
    if (CurrentToken != ')')
        return LogErrorP("Expected ')' in prototype");
    // success
    getNextToken(); // consoom )
    return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

static std::unique_ptr<FunctionAST> ParseDefinition() {
    getNextToken(); // consoom def
    auto Proto = ParsePrototype();
    if (!Proto) return nullptr;
    if (auto E = ParseExpression()) return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    return nullptr;
}

static std::unique_ptr<PrototypeAST> ParseExtern() {
    getNextToken(); // consoom extern
    return ParsePrototype();
}

static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
    if (auto E = ParseExpression()) {
        // Create an anon prototype
        auto Proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

int main() {
    // Start with binary operators
    // 1 is lowest precedence
    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40; // highest
    BinopPrecedence['/'] = 40;
}
