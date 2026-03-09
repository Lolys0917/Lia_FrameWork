#define SYSTEM_EXPORTS
#include "Manager.h"
#include "SystemAPI.h"

Char2Vector g_Messages_Box;
BoolVector g_Messages_Box_Active;

void InitDebugManager()
{
	Char2_Init(&g_Messages_Box);
	VecBool_Init(&g_Messages_Box_Active);
}

extern "C" SYSTEM_API void MessageBoxText(const char* text, const char* caption)
{
	//メッセージボックスの内容の保存＆表示
	Char2_PushBack(&g_Messages_Box, { text, caption });
	VecBool_PushBack(&g_Messages_Box_Active, true);
}

void DrawMessageBox()
{
	for (size_t i = 0; i < g_Messages_Box.size; i++)
	{
		if (VecBool_Get(&g_Messages_Box_Active, i))
		{
			Char2 msg = Char2_Get(&g_Messages_Box, i);
			MessageBoxA(nullptr, msg.First, msg.End, MB_OK);
			VecBool_Set(&g_Messages_Box_Active, i, false); // 一度表示したら非アクティブにする
		}
	}
}