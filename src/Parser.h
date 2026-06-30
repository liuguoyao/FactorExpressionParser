#ifndef FEP_PARSER_H
#define FEP_PARSER_H

#include "Lexer.h"
#include "ast.h"
#include <memory>

class Parser {
public:
    explicit Parser(Lexer& lexer);

    /// Parse a complete expression.
    std::unique_ptr<Expr> parse();

    enum Precedence : int {
        LOWEST      = 0,
        TERNARY     = 1,  // ? :
        COMPARISON  = 2,  // < > <= >= == !=
        SUM         = 3,  // + -
        PRODUCT     = 4,  // * /
        POWER       = 5,  // ^  (right-assoc)
        PREFIX      = 6,  // unary + -
        CALL        = 7,  // function call ( )
    };

    /// Return the left-binding power for the current token when used as infix.
    Precedence current_lbp() const;

    /// Consume the current token and parse it in prefix position.
    std::unique_ptr<Expr> parse_nud();

    /// Parse an expression, stopping at tokens with bp < min_prec.
    std::unique_ptr<Expr> parse_expression(Precedence min_prec);

    /// Parse a parenthesised list of arguments: (arg1, arg2, ...)
    std::vector<std::unique_ptr<Expr>> parse_args();

private:
    Lexer& lexer_;
};

#endif // FEP_PARSER_H
