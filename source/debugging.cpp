#include "debugging.h"

#include "windows_lean_and_mean.h"

void debug::WaitForDebugger()
{
#ifndef NDEBUG
    if (!(GetKeyState(VK_CONTROL) & 0x8000))
        return;

    while (!IsDebuggerPresent())
        Sleep(10);

    Sleep(2000);
#endif
}
