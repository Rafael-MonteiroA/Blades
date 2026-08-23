#include "compiler/token.hpp"

#include <sstream>

namespace blades
{

const char* to_string(TokenType type)
{
    switch (type)
    {
        case TokenType::LeftParen:    return "LeftParen";
        case TokenType::RightParen:   return "RightParen";
        case TokenType::LeftBrace:    return "LeftBrace";
        case TokenType::RightBrace:   return "RightBrace";
        case TokenType::LeftBracket:  return "LeftBracket";
        case TokenType::RightBracket: return "RightBracket";
        case TokenType::Comma:        return "Comma";
        case TokenType::Dot:          return "Dot";
        case TokenType::Minus:        return "Minus";
        case TokenType::Plus:         return "Plus";
        case TokenType::Semicolon:    return "Semicolon";
        case TokenType::Slash:        return "Slash";
        case TokenType::Star:         return "Star";
        case TokenType::Colon:        return "Colon";
        case TokenType::Bang:         return "Bang";
        case TokenType::BangEqual:    return "BangEqual";
        case TokenType::Equal:        return "Equal";
        case TokenType::EqualEqual:   return "EqualEqual";
        case TokenType::Greater:      return "Greater";
        case TokenType::GreaterEqual: return "GreaterEqual";
        case TokenType::Less:         return "Less";
        case TokenType::LessEqual:    return "LessEqual";
        case TokenType::Arrow:        return "Arrow";
        case TokenType::Identifier:   return "Identifier";
        case TokenType::String:       return "String";
        case TokenType::Integer:      return "Integer";
        case TokenType::Float:        return "Float";
        case TokenType::Fn:           return "Fn";
        case TokenType::Let:          return "Let";
        case TokenType::Return:       return "Return";
        case TokenType::Struct:       return "Struct";
        case TokenType::If:           return "If";
        case TokenType::Else:         return "Else";
        case TokenType::True:         return "True";
        case TokenType::False:        return "False";
        case TokenType::For:          return "For";
        case TokenType::While:        return "While";
        case TokenType::And:          return "And";
        case TokenType::Or:           return "Or";
        case TokenType::Eof:          return "Eof";
        case TokenType::Error:        return "Error";
    }
    return "Unknown";
}

std::string Token::to_string() const
{
    std::ostringstream oss;
    oss << blades::to_string(type) << " '" << lexeme << "' at " << span.to_string();
    return oss.str();
}

} // namespace blades
