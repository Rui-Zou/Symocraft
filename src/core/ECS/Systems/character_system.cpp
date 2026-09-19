//
// Created by Amo on 2022/6/18.
//

#include "core/ECS/Systems/character_system.h"
#include "core.h"
#include "core/ECS/component.h"
#include "core/ECS/registry.h"
#include "core/ECS/Systems/physics_system.h"
#include "core/application.h"
#include "camera/camera.h"

namespace SymoCraft::Character::Player
        {

            void Update(ECS::Registry &registry)
            {
                for (ECS::EntityId entity : registry.View<Transform, CharacterComponent
                                                ,  Physics::RigidBody>())
                {
                    auto &transform = registry.GetComponent<Transform>(entity);
                    auto &character_com = registry.GetComponent<CharacterComponent>(entity);
                    auto &rigid_body = registry.GetComponent<Physics::RigidBody>(entity);

                    float speed = (character_com.is_running) ? character_com.run_speed : character_com.base_speed;
                    rigid_body.velocity.x = 0;
                    if (!rigid_body.use_gravity)
                        rigid_body.velocity.y = 0;
                    rigid_body.velocity.z = 0;

                    float rotation = glm::radians(transform.yaw);
                    glm::vec3 front = glm::vec3(glm::cos(rotation), 0, glm::sin(rotation));
                    glm::vec3 right = glm::vec3(-front.z, 0, front.x);
                    if (character_com.movement_axis.x != 0)
                    {
                        rigid_body.velocity.x = front.x * character_com.movement_axis.x;
                        rigid_body.velocity.z = front.z * character_com.movement_axis.x;
                    }
                    if (!rigid_body.use_gravity && character_com.movement_axis.y != 0)
                        rigid_body.velocity.y += character_com.movement_axis.y;
                    if (character_com.movement_axis.z != 0)
                    {
                        rigid_body.velocity.x += right.x * character_com.movement_axis.z;
                        rigid_body.velocity.z += right.z * character_com.movement_axis.z;
                    }

                    if (glm::abs(rigid_body.velocity.x) > 0 || glm::abs(rigid_body.velocity.z) > 0)
                    {
                        float denominator = glm::inversesqrt(rigid_body.velocity.x * rigid_body.velocity.x
                                                            + rigid_body.velocity.z * rigid_body.velocity.z);
                        if (!rigid_body.use_gravity && glm::abs(rigid_body.velocity.y) > 0)
                        {
                            denominator = glm::inversesqrt(
                                    rigid_body.velocity.x * rigid_body.velocity.x
                                    + rigid_body.velocity.z * rigid_body.velocity.z
                                    + rigid_body.velocity.y * rigid_body.velocity.y);
                            rigid_body.velocity.y *= denominator * speed;
                        }
                        rigid_body.velocity.x *= denominator * speed;
                        rigid_body.velocity.z *= denominator * speed;
                    }


                    if (character_com.apply_jump_force)
                    {
                        rigid_body.velocity.y = character_com.jump_force;
                        character_com.apply_jump_force = false;
                        character_com.is_jumping = true;
                    }

                    // At the jump peak, we want to start falling fast
                    if (character_com.is_jumping && rigid_body.velocity.y <= 0)
                    {
                        rigid_body.acceleration.y = character_com.down_jump_force;
                        character_com.is_jumping = false;
                    }

                    if (registry.HasComponent<PlayerComponent>(entity))
                    {
                        auto &player_com = registry.GetComponent<PlayerComponent>(entity);
                        auto camera_entity = Application::GetCamera()->entity_id;

                        if (camera_entity != ECS::null_entity && registry.HasComponent<Transform>(camera_entity))
                        {
                            auto& camera_transform = registry.GetComponent<Transform>(camera_entity);
                            camera_transform.position = transform.position + player_com.camera_offset;
                            camera_transform.yaw = transform.yaw;
                            camera_transform.pitch = transform.pitch;
                        }
                        else
                        {
                            AmoLogger_Warning("Camera is Null!");
                        }
                    }
                }
            }
        }