#include <string>
#include <vector>
#include <filesystem>

/*
# bash の構文を BNF っぽく定義してみる
* 右辺には正規表現を用いる
* 文字列のクォーテーションには対応しない
* `<JOB>       = <CMD>{'|'<CMD>}*{'>'<STR>}?'\n'`
* `<CMD>       = <STR>{' '<STR>}*`
* `<STR>       = [^ ]+`
*/

// 解析される文字列を表す
class StringToBeParsed final
{
public:
    StringToBeParsed(const char *s) : m_string(s){};

    // 次の文字に移動し、移動した結果を返す
    // すでに文字列末尾に到達していて、移動できない場合は '\n' を返す
    // m_string が空文字の場合は '\n' を返す
    char NextChar()
    {
        // m_currentPos == m_string.size() のときに文字列末尾が判別できる
        if (m_currentPos < m_string.size())
        {
            m_currentPos++;
        }
        return CurrentChar();
    };

    // すでに文字列末尾に到達していて、移動できない場合は '\n' を返す
    // m_string が空文字の場合は '\n' を返す
    char CurrentChar() const
    {
        if (m_string.empty() || m_currentPos == m_string.size())
        {
            return '\n';
        }
        return m_string.at(m_currentPos);
    };

public:
    const std::string m_string;

private:
    size_t m_currentPos = 0;
};

struct Command final
{
    std::vector<std::string> args;
};

struct Job final
{
    // コマンドが複数存在する場合は、パイプで連結する
    // リダイレクトが指定されていない場合、最後のコマンド結果は標準出力に出力する
    std::vector<Command> commands;

    // リダイレクトが指定されている場合に設定される
    // リダイレクトが指定されていない場合は空になる
    std::filesystem::path redirectFilename;
};

enum Token
{
    Pipe,
    Redirect,
    StrSeparator,
    Str,
    End,
};

Token ToToken(const char c)
{
    switch (c)
    {
    case '|':
    {
        return Token::Pipe;
    }
    case '>':
    {
        return Token::Redirect;
    }
    case ' ':
    {
        return Token::StrSeparator;
    }
    case '\n':
    {
        return Token::End;
    }
    default:
    {
        return Token::Str;
    }
    }
}

// p の現在の解析位置から <STR> を取得する
// <STR> を取得できた場合、p の解析地点も移動する
std::string ParseStr(StringToBeParsed &p)
{
    std::string str;
    while (true)
    {
        const auto c = p.CurrentChar();
        if (ToToken(c) != Token::Str)
        {
            return str;
        }

        str += c;
        p.NextChar();
    }
}

Command NextCmd(StringToBeParsed &p)
{
    Command cmd;

    // <STR> をすべて読み込む
    while (true)
    {
        // 連続するスペースを飛ばす
        while (ToToken(p.CurrentChar()) == Token::StrSeparator)
        {
            p.NextChar();
        }

        const auto str = ParseStr(p);
        if (!str.empty())
        {
            cmd.args.push_back(str);
        }

        // <STR> の次のトークンを調べる
        // スペースが存在するなら次の <STR> が存在するかもしれないので続行する
        if (ToToken(p.CurrentChar()) == Token::StrSeparator)
        {
            // 次の <STR> の初めの文字に移動させる
            p.NextChar();
        }
        else
        {
            return cmd;
        }
    }
}

Job ParseJob(StringToBeParsed &p)
{
    Job job;

    // <CMD> をすべて読み込む
    while (true)
    {
        // 連続するスペースを飛ばす
        while (ToToken(p.CurrentChar()) == Token::StrSeparator)
        {
            p.NextChar();
        }

        Command cmd(NextCmd(p));
        if (!cmd.args.empty())
        {
            job.commands.push_back(cmd);
        }

        // NextCmd は スペース+<STR> が連続する箇所を読み取るので、NextCmd 後に出現するトークンはスペース以外になる
        // そのためスペースを飛ばす処理は不要になる

        // <CMD> の次のトークンを調べる
        // '|' が存在するなら次の <CMD> が存在するかもしれないので続行する
        if (ToToken(p.CurrentChar()) == Token::Pipe)
        {
            // 次の <CMD> の初めの文字に移動させる
            p.NextChar();
        }
        else
        {
            break;
        }
    }

    // リダイレクトを読み込む
    if (ToToken(p.CurrentChar()) == Token::Redirect)
    {
        // '>' の次の文字に移動させる
        p.NextChar();

        // 連続するスペースを飛ばす
        while (ToToken(p.CurrentChar()) == Token::StrSeparator)
        {
            p.NextChar();
        }

        job.redirectFilename = ParseStr(p);
    }

    return job;
}

void TestParseJob(const char *in, const std::vector<std::vector<std::string>> expectCommands, const std::filesystem::path expectRedirectFilename)
{
    StringToBeParsed str(in);
    const auto testeeJob = ParseJob(str);

    // コマンド一覧をテストする
    std::vector<std::vector<std::string>> testeeCommands;
    for (const auto &cmd : testeeJob.commands)
    {
        std::vector<std::string> args;
        for (const auto &arg : cmd.args)
        {
            args.push_back(arg);
        }
        testeeCommands.push_back(args);
    }
    if (testeeCommands != expectCommands)
    {
        fprintf(stderr, "コマンドテスト失敗, %s\n", in);
        return;
    }

    // リダイレクトをテストする
    if (testeeJob.redirectFilename != expectRedirectFilename)
    {
        fprintf(stderr, "リダイレクトテスト失敗, %s\n", in);
        return;
    }

    // OK
    printf("テスト成功, %s\n", in);
}

int main()
{
    TestParseJob("cmd1 aaa    bbb     | cmd2 |cmd3|cmd4 xxx>out.txt", {{"cmd1", "aaa", "bbb"}, {"cmd2"}, {"cmd3"}, {"cmd4", "xxx"}}, "out.txt");
    TestParseJob(" cmd1 > out.txt", {{"cmd1"}}, "out.txt");
    return EXIT_SUCCESS;
}