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

