#include <MemoryAllocator/AmoBase.h>
#include "core/application.h"

int main()
{
#ifdef _DEBUG
    AmoBase::AmoMemory_Init(true, 1024);
#endif
    SymoCraft::Application::Init();
    SymoCraft::Application::Run();
    SymoCraft::Application::Free();

    AmoBase::AmoMemory_MemoryLeaksDetected();
    return 0;
}