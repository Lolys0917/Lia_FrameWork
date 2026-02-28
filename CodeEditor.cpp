#define NOMINMAX

#include "GUI.h"
#include "Main.h"

std::string CodeText;
float lineNumberWidth = 40;

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
    //画面スクロール調整
    //１．下限で自動スクロールするように調整
    //２．上限で自動スクロールするように調整
    //※エディタの描画範囲をポインタが超えたときにスクロールするように調整
    //※動作はビジュアルスタジオのエディタを参考にする
    //※カーソル位置は常にエディタの描画範囲内に収まるようにする

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
            if (ed.selectStart != ed.selectEnd)
            {
                int startPos = std::min(ed.selectStart, ed.selectEnd);
                int end = std::max(ed.selectStart, ed.selectEnd);

                ed.text.erase(startPos, end - startPos);
                ed.cursor = startPos;

                ed.selectStart = -1;
                ed.selectEnd = -1;
            }
            else if (ed.cursor > 0)
            {
                ed.text.erase(ed.cursor - 1, 1);
                ed.cursor--;
            }

            movedCursor = true;
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
	// ※Shift + マウスホイールで横スクロール
    // 下に行き過ぎないようにする
    float lineHeight = ImGui::GetTextLineHeight();

    // 総行数
    int totalLines = 1;
    for (char c : ed.text)
        if (c == '\n')
            totalLines++;

    float contentHeight = totalLines * lineHeight;
    float viewHeight = ImGui::GetContentRegionAvail().y;

    float maxScroll = std::max(0.0f, contentHeight - viewHeight);

    if (io.MouseWheel != 0.0f)
    {
        ed.scrollY -= io.MouseWheel * ImGui::GetTextLineHeight() * 3;
        ed.scrollY = std::clamp(ed.scrollY, 0.0f, maxScroll);
    }
    if (io.MouseWheel != 0.0f)
    {
        if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
        {
            ed.scrollX -= io.MouseWheel * 40;
            ed.scrollX = std::max(0.0f, ed.scrollX);
        }
        else
        {
            ed.scrollY -= io.MouseWheel * ImGui::GetTextLineHeight() * 3;
            ed.scrollY = std::max(0.0f, ed.scrollY);
        }
    }
	// マウスポインタ座標計算
    ImVec2 mouse = io.MousePos;
    ImVec2 start = ImGui::GetCursorScreenPos();
    int clickCol =
        (mouse.x - start.x - lineNumberWidth + ed.scrollX)
        / ImGui::CalcTextSize(" ").x;
    // クリックカーソル移動
    if (ImGui::IsMouseClicked(0))
    {

        float lineHeight = ImGui::GetTextLineHeight();

        int clickLine = (mouse.y - start.y + ed.scrollY) / lineHeight;

        int lineStart = GetLineStart(ed.text, clickLine);
        int lineLen = GetLineLength(ed.text, lineStart);

        ed.cursor = lineStart + std::min(clickCol, lineLen);
    }

    // カーソル移動したらスクロール補正
    if (movedCursor)
    {
        CursorJump(ed, ImGui::GetContentRegionAvail().y);
    }

    //マウス範囲選択を追加
    if (ImGui::IsMouseClicked(0))
    {
        ed.selecting = true;
        ed.selectStart = ed.cursor;
        ed.selectEnd = ed.cursor;
    }
    else if (ed.selecting)
    {
        float lineHeight = ImGui::GetTextLineHeight();
        int clickLine = (mouse.y - start.y + ed.scrollY) / lineHeight;
        int lineStart = GetLineStart(ed.text, clickLine);
        int lineLen = GetLineLength(ed.text, lineStart);
        ed.selectEnd = lineStart + std::min(clickCol, lineLen);
	}
    if (ImGui::IsMouseReleased(0))
    {
        ed.selecting = false;
    }
    
	//Shift + 矢印キーで選択範囲を拡大縮小
    if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
    {
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
        {
            if (ed.selectStart == -1)
                ed.selectStart = ed.cursor;
            ed.selectEnd = ed.cursor;
            movedCursor = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
        {
            if (ed.selectStart == -1)
                ed.selectStart = ed.cursor;
            ed.selectEnd = ed.cursor;
            movedCursor = true;
        }
	}
    else
    {
        //カーソルが動いたら選択範囲を削除
        if (movedCursor)
        {
            ed.selectStart = -1;
            ed.selectEnd = -1;
        }
    }

    //デリーと
    if (ImGui::IsKeyPressed(ImGuiKey_Delete))
    {
        if (ed.selectStart != ed.selectEnd)
        {
            int startPos = std::min(ed.selectStart, ed.selectEnd);
            int end = std::max(ed.selectStart, ed.selectEnd);

            ed.text.erase(startPos, end - startPos);
            ed.cursor = startPos;
        }
        else if (ed.cursor < ed.text.size())
        {
            ed.text.erase(ed.cursor, 1);
        }

        ed.selectStart = -1;
        ed.selectEnd = -1;

        movedCursor = true;
    }

	//コピー
    if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_C))
    {
        if (ed.selectStart != ed.selectEnd)
        {
            int startPos = std::min(ed.selectStart, ed.selectEnd);
            int end = std::max(ed.selectStart, ed.selectEnd);
            std::string selectedText = ed.text.substr(startPos, end - startPos);
            ImGui::SetClipboardText(selectedText.c_str());
        }
	}
    //ペースト
	if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::
        IsKeyPressed(ImGuiKey_V))
    {
        const char* clipboard = ImGui::GetClipboardText();
        if (clipboard)
        {
            ed.text.insert(ed.cursor, clipboard);
            ed.cursor += strlen(clipboard);
            CursorJump(ed, ImGui::GetContentRegionAvail().y);
        }
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

    int totalLines = 1;
    for (char c : ed.text)
        if (c == '\n')
            totalLines++;

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

    float cx = start.x + col * ImGui::CalcTextSize(" ").x;
    float cy = start.y + line * lineHeight - ed.scrollY;

	//選択範囲描画※背景色変化
    //選択範囲の開始位置と終了位置を取得
    //選択範囲内のテキストをループして、背景色を変える矩形を描画
	//矩形の位置は、テキストの行と列から計算する
    if( ed.selectStart != ed.selectEnd )
    {
        int startPos = std::min(ed.selectStart, ed.selectEnd);
        int end = std::max(ed.selectStart, ed.selectEnd);
        int index = 0;
        int line = 0;
        int col = 0;
        for (char c : ed.text)
        {
            if (index >= startPos && index < end)
            {
                float x = start.x + (col * ImGui::CalcTextSize(" ").x);
                float y = start.y + line * lineHeight - ed.scrollY;
                draw->AddRectFilled(
                    ImVec2(x + lineNumberWidth - ed.scrollX, y),
                    ImVec2(x + lineNumberWidth - ed.scrollX + ImGui::CalcTextSize(" ").x, y + lineHeight),
                    IM_COL32(30, 60, 100, 120)
                );
            }
            if (c == '\n')
            {
                line++;
                col = 0;
            }
            else
            {
                col++;
            }
            index++;
        }
	}

    int lineIndex = 0;
    float lineY = start.y - ed.scrollY;

    for (int i = 0; i < totalLines; i++)
    {
        char buf[16];
        sprintf(buf, "%d", i + 1);

        draw->AddText(
            ImVec2(start.x, lineY),
            IM_COL32(150, 150, 150, 255),
            buf
        );

        lineY += lineHeight;
    }

	//かーそる描画
    draw->AddLine(
        ImVec2(cx + lineNumberWidth - ed.scrollX, cy),
        ImVec2(cx + lineNumberWidth - ed.scrollX, cy + lineHeight),
        IM_COL32(255, 255, 255, 255),
        2.0f
    );

    // テキスト描画
    lineHeight = ImGui::GetTextLineHeight();

    y = start.y - ed.scrollY;
    x = start.x;

    for (auto& t : tokens)
    {
        draw->AddText(ImVec2(x + lineNumberWidth - ed.scrollX, y), t.color, t.text.c_str());

        x += ImGui::CalcTextSize(t.text.c_str()).x;

        if (t.text == "\n")
        {
            x = start.x;
            y += lineHeight;
        }
    }

    float contentHeight = totalLines * lineHeight;
    float viewHeight = ImGui::GetContentRegionAvail().y;

    float scrollbarHeight = viewHeight * (viewHeight / contentHeight);

    float scrollRatio = ed.scrollY / contentHeight;

    float scrollbarY = start.y + scrollRatio * viewHeight;

    draw->AddRectFilled(
        ImVec2(start.x + ImGui::GetContentRegionAvail().x - 6, scrollbarY),
        ImVec2(start.x + ImGui::GetContentRegionAvail().x, scrollbarY + scrollbarHeight),
        IM_COL32(120, 120, 120, 200)
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