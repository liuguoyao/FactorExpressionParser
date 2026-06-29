#include "Parser.h"
#include <stdexcept>

Parser::Parser(Lexer& lexer) : lexer_(lexer) {}

std::unique_ptr<Expr> Parser::parse() {
    auto expr = parse_expression(LOWEST);
    if (lexer_.peek().type != TokenType::END) {
        throw std::runtime_error("unexpected token '" + lexer_.peek().text +
                                 "' at position " + std::to_string(lexer_.peek().pos));
    }
    return expr;
}

static Parser::Precedence lbp_of(TokenType type) {
    switch (type) {
    case TokenType::QUESTION: return Parser::TERNARY;
    case TokenType::LT: case TokenType::GT:
    case TokenType::LE: case TokenType::GE:
    case TokenType::EQ: case TokenType::NE: return Parser::COMPARISON;
    case TokenType::PLUS: case TokenType::MINUS: return Parser::SUM;
    case TokenType::STAR: case TokenType::SLASH: return Parser::PRODUCT;
    case TokenType::CARET: return Parser::POWER;
    case TokenType::LPAREN: return Parser::CALL;
    default: return Parser::LOWEST;
    }
}

Parser::Precedence Parser::current_lbp() const {
    switch (lexer_.peek().type) {
    case TokenType::QUESTION: return TERNARY;
    case TokenType::LT:
    case TokenType::GT:
    case TokenType::LE:
    case TokenType::GE:
    case TokenType::EQ:
    case TokenType::NE:       return COMPARISON;
    case TokenType::PLUS:
    case TokenType::MINUS:    return SUM;
    case TokenType::STAR:
    case TokenType::SLASH:    return PRODUCT;
    case TokenType::CARET:    return POWER;
    case TokenType::LPAREN:   return CALL;
    default:                  return LOWEST;
    }
}

std::unique_ptr<Expr> Parser::parse_expression(Precedence min_prec) {
    auto left = parse_nud();
    if (!left) {
        throw std::runtime_error("expected expression at position " +
                                 std::to_string(lexer_.peek().pos));
    }

    while (min_prec < current_lbp()) {
        Token tok = lexer_.consume();

        switch (tok.type) {
        case TokenType::PLUS:
        case TokenType::MINUS:
        case TokenType::STAR:
        case TokenType::SLASH:
        case TokenType::CARET:
        case TokenType::LT:
        case TokenType::GT:
        case TokenType::LE:
        case TokenType::GE:
        case TokenType::EQ:
        case TokenType::NE: {
            Precedence bp = lbp_of(tok.type);
            // left-assoc: right side parses with bp (stops at same-precedence ops)
            // right-assoc (^): right side parses with bp-1 (allows same-precedence ops)
            Precedence next = (tok.type == TokenType::CARET)
                                  ? static_cast<Precedence>(bp - 1)
                                  : bp;
            auto right = parse_expression(next);
            left.reset(new BinaryExpr(tok.text, std::move(left), std::move(right)));
            break;
        }
        case TokenType::QUESTION: {
            auto then_expr = parse_expression(LOWEST);
            if (!lexer_.consume_if(TokenType::COLON)) {
                throw std::runtime_error("expected ':' in ternary expression at position " +
                                         std::to_string(lexer_.peek().pos));
            }
            auto else_expr = parse_expression(TERNARY); // right-assoc
            left.reset(new TernaryExpr(std::move(left), std::move(then_expr),
                                       std::move(else_expr)));
            break;
        }
        case TokenType::LPAREN: {
            auto args = parse_args();
            // The identifier is already in `left` as IdentExpr
            if (auto* id = dynamic_cast<IdentExpr*>(left.get())) {
                std::string name = id->name;
                left.reset(new CallExpr(name, std::move(args)));
            } else {
                throw std::runtime_error("cannot call non-identifier at position " +
                                         std::to_string(tok.pos));
            }
            break;
        }
        default:
            // Should not reach here due to lbp check
            break;
        }
    }

    return left;
}

std::unique_ptr<Expr> Parser::parse_nud() {
    Token tok = lexer_.consume();

    switch (tok.type) {
    case TokenType::NUMBER:
        return std::unique_ptr<Expr>(new NumberExpr(tok.value));

    case TokenType::IDENTIFIER:
        return std::unique_ptr<Expr>(new IdentExpr(tok.text));

    case TokenType::PLUS:
        // Unary plus: +expr
        return std::unique_ptr<Expr>(
            new UnaryExpr("+", parse_expression(PREFIX)));

    case TokenType::MINUS:
        // Unary minus: -expr
        return std::unique_ptr<Expr>(
            new UnaryExpr("-", parse_expression(PREFIX)));

    case TokenType::LPAREN: {
        auto expr = parse_expression(LOWEST);
        if (!lexer_.consume_if(TokenType::RPAREN)) {
            throw std::runtime_error("expected ')' at position " +
                                     std::to_string(lexer_.peek().pos));
        }
        return expr;
    }

    case TokenType::END:
        throw std::runtime_error("unexpected end of expression");

    default:
        throw std::runtime_error("unexpected token '" + tok.text +
                                 "' at position " + std::to_string(tok.pos));
    }
}

std::vector<std::unique_ptr<Expr>> Parser::parse_args() {
    std::vector<std::unique_ptr<Expr>> args;
    if (lexer_.peek().type == TokenType::RPAREN) {
        lexer_.consume();
        return args;
    }
    args.push_back(parse_expression(LOWEST));
    while (lexer_.consume_if(TokenType::COMMA)) {
        args.push_back(parse_expression(LOWEST));
    }
    if (!lexer_.consume_if(TokenType::RPAREN)) {
        throw std::runtime_error("expected ')' at position " +
                                 std::to_string(lexer_.peek().pos));
    }
    return args;
}
