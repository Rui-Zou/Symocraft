//
// Created by Amo on 2022/7/5.
//

#ifndef SYMOCRAFT_PLAYERCONTROLLER_H
#define SYMOCRAFT_PLAYERCONTROLLER_H
#include "core.h"

namespace SymoCraft
{
    namespace Application
    {
        extern const float kBlockPlaceDebounceTime;
        extern float block_place_debounce;
        extern int new_block_id;
    }
    namespace ECS
    {
        class Registry;
    }

    class Window;
    namespace PlayerController
    {
        static const uint16 kBlockInventor[10] = {2, 3, 4, 5, 6, 7, 10, 11};
        void DoRayCast(ECS::Registry &registry, Window &window);
        void DisplayCurrentBlockName();
    }
}

#endif //SYMOCRAFT_PLAYERCONTROLLER_H
