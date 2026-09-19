//
// Created by Amo on 2022/7/5.
//
#include "playercontroller/playercontroller.h"
#include "core.h"
#include "core/constants.h"
#include "core/window.h"
#include "core/ECS/registry.h"
#include "core/ECS/component.h"
#include "core/ECS/Systems/physics_system.h"
#include "world/world.h"
#include "world/chunk_manager.h"
#include "renderer/renderer.h"

namespace SymoCraft
{
    namespace PlayerController
    {
        void DoRayCast( ECS::Registry &registry, Window &window)
        {
            auto controller = registry.GetComponent<Character::CharacterComponent>(World::GetPlayer());
            auto player_com = registry.GetComponent<Character::PlayerComponent>(World::GetPlayer());
            auto &transform = registry.GetComponent<Transform>(World::GetPlayer());

            RaycastStaticResult res = Physics::RayCastStatic(transform.position + player_com.camera_offset, transform.front, 3.0f);
            if (res.hit)
            {
                //printf("ray hitted\n");
                glm::vec3 block_looking_atpos = res.point - (res.hit_normal * 0.1f);
                Renderer::GenerateBlockFrameData(block_looking_atpos);

                if (glfwGetMouseButton((GLFWwindow*)window.window_ptr, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS
                    && Application::block_place_debounce <=0)
                {
                    Block new_block = { 0, 0, 0, 0};
                    new_block.block_id = kBlockInventor[Application::new_block_id];

                    if (new_block != BlockConstants::NULL_BLOCK && new_block != BlockConstants::AIR_BLOCK)
                    {
                        glm::vec3 world_pos = res.point + (res.hit_normal * 0.1f);
                        ChunkManager::SetBlock(world_pos, new_block.block_id);
                    }
                    Application::block_place_debounce = Application::kBlockPlaceDebounceTime;
                }
                else
                if (glfwGetMouseButton((GLFWwindow*)window.window_ptr , GLFW_MOUSE_BUTTON_LEFT)  == GLFW_PRESS
                    && Application::block_place_debounce <= 0)
                {
                    static int num_delete;
                    glm::vec3 worldPos = res.point - (res.hit_normal * 0.1f);
                    ChunkManager::RemoveBLock(worldPos);
                    num_delete++;
                    std::cout << "block removed " << num_delete  << std::endl;
                    Application::block_place_debounce = Application::kBlockPlaceDebounceTime;
                }
            }
        }

        void DisplayCurrentBlockName()
        {
            static std::string kBlockName[12] = {"", "Air Block", "Grass", "Sand", "Dirt", "Stone", "Oak Log", "Oak Leaves"
                                                , "Oak Planks", "Water Still", "Birch Plank", "Cobble Stone"};

            std::cout << "Current block is " << kBlockName[kBlockInventor[Application::new_block_id]] << std::endl;

        }

    }
}
