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
class VariableExprAST : ExprAST {
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
