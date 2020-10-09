#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <stdexcept>
#include <string>

enum Token
{
    Eof,
    Number,
    Add,
    Sub,
    Mul,
    Div,
    Lpar,
    Rpar,
    Semic,
    Others
};

// 外部変数
char ch;     // 記号
Token token; // トークン
int value;   // 数値

void ReadNextChar(void)
{
    ch = getchar();
}

char GetCurrentChar(void)
{
    return ch;
}

int ParseInteger()
{
    std::string value;
    // 整数文字が連続する部分を読み取る
    while (std::isdigit(GetCurrentChar()))
    {
        value += GetCurrentChar();
        ReadNextChar();
    }
    return atoi(value.c_str());
}

// トークンの切り分け
// エラー時には std::runtime_error を投げる
void AnalyzeNextToken(void)
{
    // 空白の読み飛ばし
    while (std::isspace(GetCurrentChar()))
    {
        ReadNextChar();
    }

    if (std::isdigit(GetCurrentChar()))
    {
        token = Number;
        value = ParseInteger();
    }
    else
    {
        const auto ch = GetCurrentChar();
        switch (ch)
        {
        case '+':
            token = Add;
            ReadNextChar();
            break;
        case '-':
            token = Sub;
            ReadNextChar();
            break;
        case '*':
            token = Mul;
            ReadNextChar();
            break;
        case '/':
            token = Div;
            ReadNextChar();
            break;
        case '(':
            token = Lpar;
            ReadNextChar();
            break;
        case ')':
            token = Rpar;
            ReadNextChar();
            break;
        case ';':
            token = Semic;
            ReadNextChar();
            break;
        case EOF:
            token = Eof;
            break;
        default:
        {
            std::string error = std::string("次のトークンは不正です, ") + std::to_string(ch);
            throw std::runtime_error(error);
        }
        }
    }
}

// 構文解析
int expression(void);
int term(void);
int factor(void);

// 式
int expression(void)
{
    int val = term();
    while (true)
    {
        switch (token)
        {
        case Add:
            AnalyzeNextToken();
            val += term();
            break;
        case Sub:
            AnalyzeNextToken();
            val -= term();
            break;
        default:
            return val;
        }
    }
}

// 項
int term(void)
{
    int val = factor();
    while (true)
    {
        switch (token)
        {
        case Mul:
            AnalyzeNextToken();
            val *= factor();
            break;
        case Div:
            AnalyzeNextToken();
            val /= factor();
            break;
        default:
            return val;
        }
    }
}

// 因子
// エラー時には std::runtime_error を投げる
int factor(void)
{
    switch (token)
    {
    case Token::Lpar:
    {
        AnalyzeNextToken();
        int val = expression();
        if (token == Rpar)
        {
            AnalyzeNextToken();
        }
        else
        {
            throw std::runtime_error("')' expected");
        }
        return val;
    }
    case Token::Number:
    {
        AnalyzeNextToken();
        return value;
    }
    case Token::Add:
    {
        AnalyzeNextToken();
        return factor();
    }
    case Token::Sub:
    {
        AnalyzeNextToken();
        return -factor();
    }
    default:
    {
        throw std::runtime_error("unexpected token");
    }
    }
}

// エラー時には std::runtime_error を投げる
void toplevel(void)
{
    int val = expression();
    if (token == Semic)
    {
        printf("=> %d\nCalc> ", val);
    }
    else
    {
        std::runtime_error("invalid token");
    }
}

int main(void)
{
    printf("Calc> ");
    ReadNextChar();

    try
    {
        while (true)
        {
            AnalyzeNextToken();
            if (token == Eof)
            {
                break;
            }

            toplevel();
        }
        return EXIT_SUCCESS;
    }
    catch (const std::runtime_error &e)
    {
        fprintf(stderr, "%s\n", e.what());
        return EXIT_FAILURE;
    }
}