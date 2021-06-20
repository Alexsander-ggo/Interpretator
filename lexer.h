#pragma once

#include <iosfwd>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace parse {
    
    using namespace std::literals;

    namespace token_type {

        // ------------------------Number-----------------------

        struct Number {  
            int            value;   
        };

        // ------------------------Id-----------------------

        struct Id {             
            std::string    value;  
        };

        // ------------------------Char-----------------------

        struct Char {    
            char           value;  
        };

        // ------------------------String-----------------------

        struct String {  
            std::string    value;
        };

        // ------------------------Lexemes-----------------------

        struct Class {};   
        struct Return {};   
        struct If {};       
        struct Else {};     
        struct Def {};      
        struct Newline {};  
        struct Print {};    
        struct Indent {};  
        struct Dedent {};  
        struct Eof {};     
        struct And {};     
        struct Or {};      
        struct Not {};     
        struct Eq {};      
        struct NotEq {};   
        struct LessOrEq {};     
        struct GreaterOrEq {};  
        struct None {};         
        struct True {};         
        struct False {};       
    }  // namespace token_type

    // ------------------------TokenBase-----------------------

    using TokenBase 
        = std::variant<token_type::Number, token_type::Id, token_type::Char, token_type::String,
                       token_type::Class, token_type::Return, token_type::If, token_type::Else,
                       token_type::Def, token_type::Newline, token_type::Print, token_type::Indent,
                       token_type::Dedent, token_type::And, token_type::Or, token_type::Not,
                       token_type::Eq, token_type::NotEq, token_type::LessOrEq, token_type::GreaterOrEq,
                       token_type::None, token_type::True, token_type::False, token_type::Eof>;

    // ------------------------Token-----------------------

    struct Token : TokenBase {
        using TokenBase::TokenBase;

        template <typename T>
        [[nodiscard]] bool                         Is() const; 

        template <typename T>
        [[nodiscard]] const T&                     As() const; 

        template <typename T>
        [[nodiscard]] const T*                     TryAs() const; 
    };

    template <typename T>
    bool Token::Is() const {
        return std::holds_alternative<T>(*this);
    }

    template <typename T>
    const T& Token::As() const {
        return std::get<T>(*this);
    }

    template <typename T>
    const T* Token::TryAs() const {
        return std::get_if<T>(this);
    }

    bool operator==(const Token& lhs, const Token& rhs);
    bool operator!=(const Token& lhs, const Token& rhs);

    std::ostream& operator<<(std::ostream& os, const Token& rhs);

    // ------------------------LexerError-----------------------

    class LexerError : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };

    // ------------------------Lexer-----------------------

    class Lexer {
    public:
        explicit                                          Lexer(std::istream& input); 

        [[nodiscard]] const Token&                        CurrentToken() const;

        Token                                             NextToken();

        template <typename T>
        const T&                                          Expect() const; 

        template <typename T, typename U>
        void                                              Expect(const U& value) const; 

        template <typename T>
        const T&                                          ExpectNext(); 

        template <typename T, typename U>
        void                                              ExpectNext(const U& value); 

    private:
        Token                                             current_token_;
        std::vector<Token>                                tokens_;
        size_t                                            current_index_ = 0;
        const std::unordered_map<std::string, Token>      key_words_token_ = {
            {"class"s, token_type::Class{}}, {"return"s, token_type::Return{}}, {"if"s, token_type::If{}},
            {"else"s, token_type::Else{}}, {"def"s, token_type::Def{}}, {"print"s, token_type::Print{}},
            {"and"s, token_type::And{}}, {"or"s, token_type::Or{}}, {"not"s, token_type::Not{}},
            {"None"s, token_type::None{}}, {"True"s, token_type::True{}}, {"False"s, token_type::False{}}
        };
        const std::unordered_map<std::string, Token>      special_operators_token_ = {
            {"=="s, token_type::Eq{}}, {"!="s, token_type::NotEq{}}, 
            {"<="s, token_type::LessOrEq{}}, {">="s, token_type::GreaterOrEq{}}
        };
        const std::unordered_set<char>                    special_operators_ = {'=', '!', '<', '>'};
        size_t                                            indent_size_ = 0;
    
        const std::unordered_set<char>                    special_symbols_ = {'\'', '\"', 'n', 't'};
   
    private:
        void                                              ParseTextOnTokens(std::istream& input);
    
        void                                              ParseString(std::istream& input);
    
        void                                              LoadTab(std::istream& input);
    
        void                                              LoadTokens(std::istream& input);
    
        void                                              LoadNumber(std::istream& input);
    
        void                                              LoadString(std::istream& input);
    
        void                                              LoadWord(std::istream& input);
    
        void                                              LoadSign(std::istream& input);
    
        void                                              LoadEndl();
    
        void                                              LoadDedent(size_t current_indent = 0);
    
        void                                              LoadIndent(size_t current_indent = 0);
    
        void                                              LoadChar(char c);
    
        void                                              LoadEof();
    
        bool                                              IsChar(char c);
    
        bool                                              StringIsEmpty(std::string_view str);
    
        template <typename Fn>
        std::string                                       ReadString(std::istream& input, Fn pred); 
    };

    template <typename T>
    const T& Lexer::Expect() const {
        if (!current_token_.Is<T>()) {
            throw LexerError("The token is not of the appropriate type"s);
        }
        return current_token_.As<T>();
    }

    template <typename T, typename U>
    void Lexer::Expect(const U& value) const {
        Expect<T>();
        if (current_token_.As<T>().value != value) {
            throw LexerError("A token with an invalid value"s);
        }
    }

    template <typename T>
    const T& Lexer::ExpectNext() {
        NextToken();
        return Expect<T>();
    }

    template <typename T, typename U>
    void Lexer::ExpectNext(const U& value) {
        NextToken();
        Expect<T, U>(value);
    }

    template <typename Fn>
    std::string Lexer::ReadString(std::istream& input, Fn pred) {
        std::string res;
        char c;
        while (!input.eof()) {
            if (input >> std::noskipws >> c) {
                if (!pred(c)) {
                    break;
                }
                else if (c == '\\') {
                    char ch = input.get();
                    if (special_symbols_.count(ch) != 0) {
                        if (ch == 'n') {
                            ch = '\n';
                        }
                        else if (ch == 't') {
                            ch = '\t';
                        }
                        res.push_back(ch);
                    }
                }
                else {
                    res.push_back(c);
                }
            }
        }
        if (!pred(c) && c != '\'' && c != '\"') {
            input.putback(c);
        }
        return res;
    }

}