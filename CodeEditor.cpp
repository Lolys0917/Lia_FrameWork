#define NOMINMAX

#include "GUI.h"
#include "Main.h"

std::string CodeText;

bool Keyword(const std::string& s)
{
    static const char* kw[] =
    {
        "int","float","double","void","return",
        "if","else","for","while",
        "class","struct","static","const"
    };

    for (auto& k : kw)
        if (s == k) return true;

    return false;
}

void Tokenize(const std::string& src, std::vector<HLToken>& out)
{
    out.clear();

    std::string buf;

    for (char c : src)
    {
        if (isalnum(c) || c == '_')
        {
            buf += c;
        }
        else
        {
            if (!buf.empty())
            {
                HLToken t;

                t.text = buf;

                if (Keyword(buf))
                    t.color = IM_COL32(80, 160, 255, 255);
                else
                    t.color = IM_COL32(220, 220, 220, 255);

                out.push_back(t);

                buf.clear();
            }

            HLToken t;

            t.text = std::string(1, c);
            t.color = IM_COL32(200, 200, 200, 255);

            out.push_back(t);
        }
    }

    if (!buf.empty())
    {
        HLToken t;

        t.text = buf;
        t.color = Keyword(buf)
            ? IM_COL32(80, 160, 255, 255)
            : IM_COL32(220, 220, 220, 255);

        out.push_back(t);
    }
}

void GetCursorLineColumn(const std::string& text, int cursor, int& line, int& col)
{
    line = 0;
    col = 0;

    for (int i = 0; i < cursor && i < text.size(); i++)
    {
        if (text[i] == '\n')
        {
            line++;
            col = 0;
        }
        else
        {
            col++;
        }
    }
}
int GetLineStart(const std::string& text, int line)
{
    int current = 0;

    if (line == 0) return 0;

    for (int i = 0; i < text.size(); i++)
    {
        if (text[i] == '\n')
        {
            current++;

            if (current == line)
                return i + 1;
        }
    }

    return text.size();
}
int GetLineLength(const std::string& text, int start)
{
    int len = 0;

    for (int i = start; i < text.size(); i++)
    {
        if (text[i] == '\n')
            break;

        len++;
    }

    return len;
}

void CodeEditorInput(CodeEditorState& ed)
{
    ImGuiIO& io = ImGui::GetIO();

    for (int i = 0; i < io.InputQueueCharacters.Size; i++)
    {
        unsigned int c = io.InputQueueCharacters[i];

        if (c == 8) // backspace
        {
            if (ed.cursor > 0)
            {
                ed.text.erase(ed.cursor - 1, 1);
                ed.cursor--;
            }
        }
        else if (c == 13)
        {
            ed.text.insert(ed.cursor, "\n");
            ed.cursor++;
        }
        else if (c >= 32)
        {
            ed.text.insert(ed.cursor, 1, (char)c);
            ed.cursor++;
        }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
        ed.cursor = std::max(0, ed.cursor - 1);

    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
        ed.cursor = std::min((int)ed.text.size(), ed.cursor + 1);

    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
    {
        int line, col;
        GetCursorLineColumn(ed.text, ed.cursor, line, col);

        if (line > 0)
        {
            int prevStart = GetLineStart(ed.text, line - 1);
            int prevLen = GetLineLength(ed.text, prevStart);

            ed.cursor = prevStart + std::min(col, prevLen);
        }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
    {
        int line, col;
        GetCursorLineColumn(ed.text, ed.cursor, line, col);

        int nextStart = GetLineStart(ed.text, line + 1);

        if (nextStart < ed.text.size())
        {
            int nextLen = GetLineLength(ed.text, nextStart);

            ed.cursor = nextStart + std::min(col, nextLen);
        }
    }
}

void CodeEditorDraw(CodeEditorState& ed)
{
    static std::vector<HLToken> tokens;

    Tokenize(ed.text, tokens);

    ImDrawList* draw = ImGui::GetWindowDrawList();

    ImVec2 start = ImGui::GetCursorScreenPos();

    float x = start.x + 5;
    float y = start.y + 5;

    float lineHeight = ImGui::GetTextLineHeight();

    for (auto& t : tokens)
    {
        draw->AddText(ImVec2(x, y), t.color, t.text.c_str());

        x += ImGui::CalcTextSize(t.text.c_str()).x;

        if (t.text == "\n")
        {
            x = start.x + 5;
            y += lineHeight;
        }
    }

    // ÉJÅ[É\Éãï`âÊ
    int line = 0;
    int col = 0;

    for (int i = 0; i < ed.cursor && i < ed.text.size(); i++)
    {
        if (ed.text[i] == '\n')
        {
            line++;
            col = 0;
        }
        else
        {
            col++;
        }
    }

    float cx = start.x + col * ImGui::CalcTextSize(" ").x;
    float cy = start.y + line * lineHeight;

    draw->AddLine(
        ImVec2(cx, cy),
        ImVec2(cx, cy + lineHeight),
        IM_COL32(255, 255, 255, 255),
        1.0f
    );
}

void SetCodeText(std::string inText)
{
    CodeText = inText;
}
std::string GetCodeText()
{
    return CodeText;
}