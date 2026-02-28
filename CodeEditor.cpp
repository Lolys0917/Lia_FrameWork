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

void CursorJump(CodeEditorState& ed, float viewHeight)
{
    float lineHeight = ImGui::GetTextLineHeight();

    int line = 0;
    for (int i = 0; i < ed.cursor && i < ed.text.size(); i++)
        if (ed.text[i] == '\n')
            line++;

    float cursorY = line * lineHeight;

    if (cursorY < ed.scrollY)
        ed.scrollY = cursorY;

    if (cursorY > ed.scrollY + viewHeight - lineHeight)
        ed.scrollY = cursorY - viewHeight + lineHeight;
}

void CodeEditorInput(CodeEditorState& ed)
{
    ImGuiIO& io = ImGui::GetIO();

    bool movedCursor = false;

    for (int i = 0; i < io.InputQueueCharacters.Size; i++)
    {
        unsigned int c = io.InputQueueCharacters[i];

        if (c == 8)
        {
            if (ed.cursor > 0)
            {
                ed.text.erase(ed.cursor - 1, 1);
                ed.cursor--;
                movedCursor = true;
            }
        }
        else if (c == 13)
        {
            ed.text.insert(ed.cursor, "\n");
            ed.cursor++;
            movedCursor = true;
        }
        else if (c >= 32)
        {
            ed.text.insert(ed.cursor, 1, (char)c);
            ed.cursor++;
            movedCursor = true;
        }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
    {
        ed.cursor = std::max(0, ed.cursor - 1);
        movedCursor = true;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
    {
        ed.cursor = std::min((int)ed.text.size(), ed.cursor + 1);
        movedCursor = true;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
    {
        int line, col;
        GetCursorLineColumn(ed.text, ed.cursor, line, col);

        if (line > 0)
        {
            int prevStart = GetLineStart(ed.text, line - 1);
            int prevLen = GetLineLength(ed.text, prevStart);

            ed.cursor = prevStart + std::min(col, prevLen);
            movedCursor = true;
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
            movedCursor = true;
        }
    }

    // マウスホイール
    if (io.MouseWheel != 0.0f)
    {
        ed.scrollY -= io.MouseWheel * ImGui::GetTextLineHeight() * 3;
        ed.scrollY = std::max(0.0f, ed.scrollY);
    }

    // クリックカーソル移動
    if (ImGui::IsMouseClicked(0))
    {
        ImVec2 mouse = io.MousePos;
        ImVec2 start = ImGui::GetCursorScreenPos();

        float lineHeight = ImGui::GetTextLineHeight();

        int clickLine = (mouse.y - start.y + ed.scrollY) / lineHeight;
        int clickCol = (mouse.x - start.x) / ImGui::CalcTextSize(" ").x;

        int lineStart = GetLineStart(ed.text, clickLine);
        int lineLen = GetLineLength(ed.text, lineStart);

        ed.cursor = lineStart + std::min(clickCol, lineLen);
    }

    // カーソル移動したらスクロール補正
    if (movedCursor)
    {
        CursorJump(ed, ImGui::GetContentRegionAvail().y);
    }
}

void CodeEditorDraw(CodeEditorState& ed)
{
    static std::vector<HLToken> tokens;

    Tokenize(ed.text, tokens);

    ImDrawList* draw = ImGui::GetWindowDrawList();

    ImVec2 start = ImGui::GetCursorScreenPos();

    float x = start.x;
    float y = start.y;

    float lineHeight = ImGui::GetTextLineHeight();

    // カーソル描画
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

    //画面スクロール調整
	//１．下限で自動スクロールするように調整
    //２．上限で自動スクロールするように調整
	//※エディタの描画範囲をポインタが超えたときにスクロールするように調整
	//※動作はビジュアルスタジオのエディタを参考にする
	//※カーソル位置は常にエディタの描画範囲内に収まるようにする
    
    float cursorLocalY = line * lineHeight;

    float viewTop = ed.scrollY;
    float viewBottom = ed.scrollY + ImGui::GetContentRegionAvail().y;

    //if (cursorLocalY < viewTop)
    //{
    //    ed.scrollY = cursorLocalY;
    //}

    //if (cursorLocalY > viewBottom - lineHeight)
    //{
    //    ed.scrollY = cursorLocalY - ImGui::GetContentRegionAvail().y + lineHeight;
    //}

    float cx = start.x + col * ImGui::CalcTextSize(" ").x;
    float cy = start.y + line * lineHeight - ed.scrollY;

    draw->AddLine(
        ImVec2(cx, cy),
        ImVec2(cx, cy + lineHeight),
        IM_COL32(255, 255, 255, 255),
        2.0f
    );
    // テキスト描画
    lineHeight = ImGui::GetTextLineHeight();

    y = start.y - ed.scrollY;
    x = start.x;

    for (auto& t : tokens)
    {
        draw->AddText(ImVec2(x, y), t.color, t.text.c_str());

        x += ImGui::CalcTextSize(t.text.c_str()).x;

        if (t.text == "\n")
        {
            x = start.x;
            y += lineHeight;
        }
    }
}

void SetCodeText(std::string inText)
{
    CodeText = inText;
}
std::string GetCodeText()
{
    return CodeText;
}