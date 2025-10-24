#include<iostream>
#include<string>
#include<vector>
#include<unordered_map>
#include<sstream>
using namespace std;
enum TokenType
{
    INT, VOID, IF, ELSE, WHILE, BREAK, CONTINUE, RETURN, IDENT, INTCONST, ASSIGN,
    PLUS, MINUS, MULTIPLY, DIVIDE, MODULO, LESS, GREATER, LESSEQUAL, GREATEREQUAL,
    EQUAL, NOTEQUAL, AND, OR, NOT, LP, RP, LB, RB, COMMA, SEMICOLON, END
};
unordered_map<TokenType, string> token_name = {
    {INT, "'int'"},
    {VOID, "'void'"},
    {IF, "'if'"},
    {ELSE, "'else'"},
    {WHILE, "'while'"},
    {BREAK, "'break'"},
    {CONTINUE, "'continue'"},
    {RETURN, "'return'"},
    {IDENT, "Ident"},
    {INTCONST, "IntConst"},
    {ASSIGN, "'='"},
    {PLUS, "'+'"},
    {MINUS, "'-'"},
    {MULTIPLY, "'*'"},
    {DIVIDE, "'/'"},
    {MODULO, "'%'"},
    {LESS, "'<'"},
    {GREATER, "'>'"},
    {LESSEQUAL, "'<='"},
    {GREATEREQUAL, "'>='"},
    {EQUAL, "'=='"},
    {NOTEQUAL, "'!='"},
    {AND, "'&&'"},
    {OR, "'||'"},
    {NOT, "'!'"},
    {LP, "'('"},
    {RP, "')'"},
    {LB, "'{'"},
    {RB, "'}'"},
    {COMMA, "','"},
    {SEMICOLON, "';'"}
};
unordered_map<string, TokenType> keywords = {
     {"int", INT},
     {"void", VOID},
     {"if", IF},
     {"else", ELSE},
     {"while", WHILE},
     {"break", BREAK},
     {"continue", CONTINUE},
     {"return", RETURN}
};
class Token 
{
public:
    TokenType type;
    string value;
    int line;
    Token(TokenType type, string value,int line)
    {
        this->type = type;
        this->value = value;
        this->line = line;
    }
};
class Lexer
{
public:
    string input;
    int line;
    int pos;
    char current_char;
    Lexer(string input)
    {
        this->input = input;
        this->pos = 0;
        this->line = 1;
        this->current_char = input[0];
    }
    void front()
    {
        this->pos++;
        if (pos > input.length() - 1) {
            current_char = '\0';
        }
        else {
            current_char = input[pos];
        }
    }
    char next_char()
    {
        if (pos > input.length() - 1) {
            return '\0';
        }
        else {
            return input[pos + 1];
        }
    }
    void skip_freespace()
    {
        while (current_char != '\0' && isspace(current_char))
        {
            if (current_char == '\n')
            {
                line++;
            }
            front();
        }
    }
    void skip_comment()
    {
        if (current_char == '/')
        {
            char next = next_char();
            if (next == '/')
            {
                front();
                front();
                while (current_char != '\0' && current_char != '\n')
                {
                    front();
                }
                if (current_char == '\n')
                {
                    line++;
                    front();
                }
            }
            else if (next == '*')
            {
                front();
                front();
                while (current_char != '\0')
                {
                    if (current_char == '*' && next_char() == '/')
                    {
                        front();
                        front();
                        break;
                    }
                    if (current_char == '\n')
                    {
                        line++;
                    }
                    front();
                }
            }
        }
    }
    Token get_number()
    {
        string res;
        int line=this->line;
        while (current_char != '\0' && isdigit(current_char))
        {
            res += current_char;
            front();
        }
        Token num=Token(INTCONST,res,line);
        return num;
    }
    Token get_TagOrKeyword()
    {
        string res;
        int line=this->line;
        if (isalpha(current_char) || current_char == '_')
        {
            res += current_char;
            front();
        }
        while ((isalnum(current_char) || current_char == '_')&&current_char!='\0')
        {
            res += current_char;
            front();
        }
        if (keywords.find(res) != keywords.end())
        {
            Token keyword=Token(keywords[res],res,line);
            return keyword;
        }
        Token tag=Token(IDENT,res,line);
        return tag;
    }
    Token get_token()
    { 
        while (current_char != '\0')
        {
            if (isspace(current_char))
            {
                skip_freespace();
                continue;
            }
            if (current_char == '/' && (next_char() == '/' || next_char() == '*'))
            {
                skip_comment();
                continue;
            }
            if (isalpha(current_char) || current_char == '_')
            {
                return get_TagOrKeyword();
            }
            if (isdigit(current_char))
            {
                return get_number();
            }
            int line=this->line;
            switch (current_char)
            { 
            case '=':
                if (next_char() == '=')
                {
                    front();
                    front();
                    return Token(EQUAL,"==",line);
                }
                front();
                return Token(ASSIGN,"=",line);
            case '+':
                front();
                return Token(PLUS,"+",line);
            case '-':
                front();
                return Token(MINUS,"-",line);
            case '*':
                front();
                return Token(MULTIPLY,"*",line);
            case '/':
                front();
                return Token(DIVIDE,"/",line);
            case '%':
                front();
                return Token(MODULO,"%",line);
            case '<':
                if (next_char() == '=')
                {
                    front();
                    front();
                    return Token(LESSEQUAL, "<=", line);
                }
                front();
                return Token(LESS, "<", line);
            case '>':
                if (next_char() == '=')
                {
                    front();
                    front();
                    return Token(GREATEREQUAL, ">=", line);
                }
                front();
                return Token(GREATER, ">", line);
            case '!':
                if (next_char() == '=')
                { 
                    front();
                    front();
                    return Token(NOTEQUAL, "!=", line);
                }
                front();
                return Token(NOT, "!", line);
            case '&':
                if (next_char() == '&')
                {
                    front();
                    front();
                    return Token(AND, "&&", line);
                }
                front();
                return Token(AND, "&", line);
            case '|':
                if (next_char() == '|')
                {
                    front();
                    front();
                    return Token(OR, "||", line);
                }
                front();
                return Token(OR, "|", line);
            case '(':
                front();
                return Token(LP,"(",line);
            case ')':
                front();
                return Token(RP,")",line);
            case '{':
                front();
                return Token(LB,"{",line);
            case '}':
                front();
                return Token(RB,"}",line);
            case ',':
                front();
                return Token(COMMA,",",line);
            case ';':
                front();
                return Token(SEMICOLON,";",line);
            default:
                front();
                continue;
            }
        }
        return Token(END, "", line);
    }
    vector<Token> save_token() 
    { 
        vector<Token> res;
        Token token=get_token();
        while(token.type!=END)
        {
            res.push_back(token);
            token=get_token();
        }
        return res;
    }
};

int main()
{
    /*string a = "int";
    TokenType t= find_key(a, keywords);
    cout << token_name[t] << endl;*/
    stringstream ss;
    ss << cin.rdbuf();
    string input = ss.str();
    /*cout<<"input:"<<input<<endl;*/
    Lexer lexer(input);
    vector<Token> tokens=lexer.save_token();
    for (int i = 0; i < tokens.size(); i++)
    {
        cout<<i<<":"<<token_name[tokens[i].type]<<":\""<<tokens[i].value<<"\""<<endl;
    }
    return 0;
}