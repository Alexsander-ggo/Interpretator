#include "lexer.h"

using namespace std;

namespace parse {

    bool operator==(const Token& lhs, const Token& rhs) {
        using namespace token_type;

        if (lhs.index() != rhs.index()) {
            return false;
        }
        if (lhs.Is<Char>()) {
            return lhs.As<Char>().value == rhs.As<Char>().value;
        }
        if (lhs.Is<Number>()) {
            return lhs.As<Number>().value == rhs.As<Number>().value;
        }
        if (lhs.Is<String>()) {
            return lhs.As<String>().value == rhs.As<String>().value;
        }
        if (lhs.Is<Id>()) {
            return lhs.As<Id>().value == rhs.As<Id>().value;
        }
        return true;
    }

    bool operator!=(const Token& lhs, const Token& rhs) {
        return !(lhs == rhs);
    }

    std::ostream& operator<<(std::ostream& os, const Token& rhs) {
        using namespace token_type;

    #define VALUED_OUTPUT(type) \
        if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

        VALUED_OUTPUT(Number);
        VALUED_OUTPUT(Id);
        VALUED_OUTPUT(String);
        VALUED_OUTPUT(Char);

    #undef VALUED_OUTPUT

    #define UNVALUED_OUTPUT(type) \
        if (rhs.Is<type>()) return os << #type;

        UNVALUED_OUTPUT(Class);
        UNVALUED_OUTPUT(Return);
        UNVALUED_OUTPUT(If);
        UNVALUED_OUTPUT(Else);
        UNVALUED_OUTPUT(Def);
        UNVALUED_OUTPUT(Newline);
        UNVALUED_OUTPUT(Print);
        UNVALUED_OUTPUT(Indent);
        UNVALUED_OUTPUT(Dedent);
        UNVALUED_OUTPUT(And);
        UNVALUED_OUTPUT(Or);
        UNVALUED_OUTPUT(Not);
        UNVALUED_OUTPUT(Eq);
        UNVALUED_OUTPUT(NotEq);
        UNVALUED_OUTPUT(LessOrEq);
        UNVALUED_OUTPUT(GreaterOrEq);
        UNVALUED_OUTPUT(None);
        UNVALUED_OUTPUT(True);
        UNVALUED_OUTPUT(False);
        UNVALUED_OUTPUT(Eof);

    #undef UNVALUED_OUTPUT

        return os << "Unknown token :("sv;
    }

    Lexer::Lexer(istream& input) {
        ParseTextOnTokens(input);
    }

    const Token& Lexer::CurrentToken() const {
        return current_token_;
    }

    Token Lexer::NextToken() {
        if (current_index_ < tokens_.size()) {
            current_token_ = tokens_.at(current_index_++);
        }
        return current_token_;
    }
    
    void Lexer::ParseTextOnTokens(istream& input) {
        string line;
        while (getline(input, line)) {
            if (!StringIsEmpty(line)) {
                stringstream stream(line);
                ParseString(stream);
                LoadEndl();
            }
        }
        LoadDedent();
        LoadEof();
        NextToken();
    }
    
    void Lexer::ParseString(istream& input) {
        LoadTab(input);
        LoadTokens(input);
    }
    
    void Lexer::LoadTab(istream& input) {
        if (input.peek() == ' ') {
            size_t i = 0;
            do { 
                ++i;
                input.ignore();
            } while (input.peek() == ' ');
            if (i > indent_size_) {
                LoadIndent(i);
            } else {
                LoadDedent(i);
            }
        } else {
            LoadDedent();
        }
    }
    
     void Lexer::LoadTokens(istream& input) {
        while (!input.eof()) {
            if (input.peek() == ' ') {
                input.ignore();
            }
            if (input.peek() == '#') {
                break;
            }
            if (isdigit(input.peek())) {
                LoadNumber(input);
            }
            if (input.peek() == '\"' || input.peek() == '\'') {
                LoadString(input);
            }
            if (input.peek() == '_' || isalpha(input.peek())) {
                LoadWord(input);
            }
            if (IsChar(input.peek())) {
                if (special_operators_.count(input.peek()) != 0) {
                    LoadSign(input);
                } else {
                    LoadChar(input.get());
                }
            }
        }
     }
    
    void Lexer::LoadNumber(istream& input) {
        string number = ReadString(input, [] (char c) {
            return isdigit(c);
        });
        tokens_.push_back(Token(token_type::Number{stoi(number)}));
    }
    
    void Lexer::LoadString(istream& input) {
        char end = input.get();
        string dest = ReadString(input, [end] (char c) {
            return c != end;
        });
        tokens_.push_back(Token(token_type::String{dest}));
    }
    
    void Lexer::LoadWord(istream& input) {
        string word = ReadString(input, [] (char c) {
            return c == '_' || isalnum(c);
        });
        if (key_words_token_.count(word) != 0) {
            tokens_.push_back(key_words_token_.at(word));
        } else {
            tokens_.push_back(Token(token_type::Id{word}));
        }
    }
    
    void Lexer::LoadSign(istream& input) {
        char first = input.get();
        if (input.peek() == '=') {
            char second = input.get();
            string sign({first, second});
            tokens_.push_back(special_operators_token_.at(sign));
        } else {
            LoadChar(first);
        }
    }
    
    void Lexer::LoadEndl() {
        if (!tokens_.empty() && !tokens_.back().Is<token_type::Newline>()) {
            tokens_.push_back(Token(token_type::Newline{}));
        }
    }
    
    void Lexer::LoadDedent(size_t current_indent) {
        while (indent_size_ > current_indent) {
            indent_size_ -= 2;
            tokens_.push_back(Token(token_type::Dedent{}));
        }
    }
    
    void Lexer::LoadIndent(size_t current_indent) {
        while (indent_size_ < current_indent) {
            indent_size_ += 2;
            tokens_.push_back(Token(token_type::Indent{}));
        }
    }
    
    void Lexer::LoadChar(char c) {
        tokens_.push_back(Token(token_type::Char{c}));
    }
    
    void Lexer::LoadEof() {
        tokens_.push_back(Token(token_type::Eof{}));
    }
    
    bool Lexer::IsChar(char c) {
        return c == '.' || c == ',' || c == '(' || c == '+' || c == ')' || c == '-' || c == '*' || c == '/' || c == ':'
            || c == '@' || c == '%' || c == '$' || c == '^' || c == '&' || c == ';' || c == '?' || c == '=' || c == '<'
            || c == '>' || c == '!' || c == '{' || c == '}' || c == '[' || c == ']';
    }
    
    bool Lexer::StringIsEmpty(std::string_view str) {
        for (char c : str) {
            if (c != ' ') {
                if (c == '#') {
                    break;
                }
                return false;
            } 
        }
        return true;
    }
    
} 