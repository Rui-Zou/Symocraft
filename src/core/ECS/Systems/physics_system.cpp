//
// Created by Amo on 2022/6/18.
//
#include "core/ECS/Systems/physics_system.h"
#include "core/ECS/component.h"
#include "core/ECS/registry.h"
#include "core/application.h"
#include "world/chunk.h"
#include "renderer/renderer.h"
#include "core/constants.h"

namespace SymoCraft::Physics
    {
        // The main component of our physics system is collision
        // The remaining part of it is to deal with collision
        enum class Direction : uint8
        {
            NONE = 0,
            TOP,
            BOTTOM,
            FRONT,
            BACK,
            LEFT,
            RIGHT
        };

        bool compare(float x, float y, float epsilon = std::numeric_limits<float>::min())
        {
            return abs(x - y) <= epsilon * std::max(1.0f, std::max(abs(x), abs(y)));
        }

        bool compare(const glm::vec3& vec1, const glm::vec3& vec2, float epsilon = std::numeric_limits<float>::min())
        {
            return compare(vec1.x, vec2.x, epsilon) && compare(vec1.y, vec2.y, epsilon) && compare(vec1.z, vec2.z, epsilon);
        }

        struct CollisionInfo
        {
            glm::vec3 overlap_part;     // overlap part of two entity
            Direction force;            // collision force direction
            bool did_collision;     // did they really collision?
        };

        // ----------------------------------------------------------------------------------------------------------
        // some useful uniform variables in physics system
        static glm::vec3 uniform_gravity = glm::vec3(0.0, 20.0, 0.0);
        static glm::vec3 terminal_velocity = glm::vec3(50.0f, 50.0f, 50.0f);
        static const float kPhysicsUpdateRate = 1.0f / 120.0f; // 120Hz

        // ----------------------------------------------------------------------------------------------------------
        // some useful uniform function in physics system
        static void ResolveStaticCollision(ECS::EntityId entity, RigidBody &rb, Transform &transform, HitBox &hit_box);
        static CollisionInfo StaticCollisionInformation(const RigidBody& rb1, const HitBox &hb1, const Transform& tr1
                                                        , const HitBox &hb2, const Transform& tr2);
        static bool IsColliding(const HitBox &hb1, const Transform &tr1
                                , const HitBox &hb2, const Transform &tr2);

        static float PenetrationAmount(const glm::vec3 &hb1_negative, const glm::vec3 &hb1_positive
                                       ,const glm::vec3 &hb2_negative, const glm::vec3 &hb2_positive
                                       , const glm::vec3 &axis);

        //static Interval GetInterval(const HitBox &box, const Transform &transform, const glm::vec3 &axis);
        static void GetQuadrantResult(const Transform &tr1, const Transform &tr2, const HitBox &hb2_expanded
                                      , CollisionInfo* res ,Direction x_face, Direction y_face, Direction z_face);
        //static glm::vec3 ClosetPointOnRay(const glm::vec3 ray_origin, const glm::vec3 ray_direction
         //                                 , float ray_max_distance, glm::vec3 &point);


        // ----------------------------------------------------------------------------------------------------------
        // Physics system core

        void Update(ECS::Registry& registry)
        {
            static float accumulated_delta_time = 0.0f;
            accumulated_delta_time += Application::delta_time;

            while (accumulated_delta_time >= kPhysicsUpdateRate)
            {
                accumulated_delta_time -= kPhysicsUpdateRate;

                for (ECS::EntityId entity : registry.View<Transform, RigidBody, HitBox>())
                {

                    auto &rb = registry.GetComponent<RigidBody>(entity);
                    auto &transform = registry.GetComponent<Transform>(entity);
                    auto &hit_box = registry.GetComponent<HitBox>(entity);

                    transform.position += rb.velocity * kPhysicsUpdateRate;
                    rb.velocity += rb.acceleration * kPhysicsUpdateRate;
                    if (rb.use_gravity)
                    {
                        rb.velocity -= uniform_gravity * kPhysicsUpdateRate;
                    }
                    rb.velocity = glm::clamp(rb.velocity, -terminal_velocity, terminal_velocity);

                    if (rb.is_sensor)
                    {
                        // not designed yet
                        continue;
                    }

                    ResolveStaticCollision(entity, rb, transform, hit_box);

                }
            }
        }


        static bool DoRayTracing(const glm::vec3 &origin, const glm::vec3 &normal_direction,
                              float max_distance, bool draw,
                              const glm::vec3 &block_corner, const glm::vec3& step, RaycastStaticResult* out)
        {
            glm::vec3 block_center = block_corner - (glm::vec3(0.5f) * step);
            int block_id = ChunkManager::GetBlock(block_center).block_id;

            if (block_id != BlockConstants::NULL_BLOCK.block_id && block_id != BlockConstants::AIR_BLOCK.block_id)
            {
                HitBox current_box{};
                current_box.offset = glm::vec3();
                current_box.size = glm::vec3(1.0f, 1.0f, 1.0f);
                Transform currentTransform{};
                currentTransform.position = block_center;

                Block block = ChunkManager::GetBlock(currentTransform.position);
                BlockFormat blockFormat = get_block(block.block_id);

                if (blockFormat.m_is_solid)
                {
                    glm::vec3 min = currentTransform.position - (current_box.size * 0.5f) + current_box.offset;
                    glm::vec3 max = currentTransform.position + (current_box.size * 0.5f) + current_box.offset;
                    float t1 = (min.x - origin.x) / (compare(normal_direction.x, 0.0f) ? 0.00001f : normal_direction.x);
                    float t2 = (max.x - origin.x) / (compare(normal_direction.x, 0.0f) ? 0.00001f : normal_direction.x);
                    float t3 = (min.y - origin.y) / (compare(normal_direction.y, 0.0f) ? 0.00001f : normal_direction.y);
                    float t4 = (max.y - origin.y) / (compare(normal_direction.y, 0.0f) ? 0.00001f : normal_direction.y);
                    float t5 = (min.z - origin.z) / (compare(normal_direction.z, 0.0f) ? 0.00001f : normal_direction.z);
                    float t6 = (max.z - origin.z) / (compare(normal_direction.z, 0.0f) ? 0.00001f : normal_direction.z);

                    float tmin = glm::max(glm::max(glm::min(t1, t2), glm::min(t3, t4)), glm::min(t5, t6));
                    float tmax = glm::min(glm::min(glm::max(t1, t2), glm::max(t3, t4)), glm::max(t5, t6));
                    if (tmax < 0 || tmin > tmax)
                    {
                        // No intersection
                        return false;
                    }

                    // intersection exists
                    float depth = 0.0f;
                    if (tmin < 0.0f)
                        depth = tmax;
                    else
                        depth = tmin;

                    out->point = origin + normal_direction * depth;
                    out->hit = true;
                    out->block_center = currentTransform.position + current_box.offset;
                    out->block_size = current_box.size;
                    out->hit_normal = out->point - out->block_center;
                    float max_component = glm::max(glm::abs(out->hit_normal.x), glm::max(glm::abs(out->hit_normal.y), glm::abs(out->hit_normal.z)));
                    out->hit_normal = glm::abs(out->hit_normal.x) == max_component
                                      ? glm::vec3(1, 0, 0) * glm::sign(out->hit_normal.x)
                                      : glm::abs(out->hit_normal.y) == max_component
                                        ? glm::vec3(0, 1, 0) * glm::sign(out->hit_normal.y)
                                        : glm::vec3(0, 0, 1) * glm::sign(out->hit_normal.z);
                    return true;
                }
            }

            return false;
        }
        // Ray casting from player on the block


        RaycastStaticResult RayCastStatic(const glm::vec3 &origin, const glm::vec3 &normal_direction
                , float max_distance, bool draw)
        {
            RaycastStaticResult result{};
            result.hit = false;

            if (compare(normal_direction, glm::vec3(0, 0, 0)))
            {
                // invalid normal direction, return null result
                return result;
            }

            /*
			if (draw)
			{
				Renderer::drawLine(origin, origin + normal_direction * max_distance,);
			}
            */
            // NOTE: the reference paper: http://www.cse.yorku.ca/~amana/research/grid.pdf
            glm::vec3 rayEnd = origin + normal_direction * max_distance;

            // Do some fancy math to figure out which voxel is the next voxel
            glm::vec3 block_center = glm::ceil(origin);
            // step is the sign of normal direction
            glm::vec3 step = glm::sign(normal_direction);
            // Max amount we can step in any direction of the ray, and remain in the voxel
            glm::vec3 block_center_to_origin_sign = glm::sign(block_center - origin);
            glm::vec3 revised_normal_direction = glm::vec3(
                    normal_direction.x == 0.0f ? 1e-10 * block_center_to_origin_sign.x : normal_direction.x,
                    normal_direction.y == 0.0f ? 1e-10 * block_center_to_origin_sign.y : normal_direction.y,
                    normal_direction.z == 0.0f ? 1e-10 * block_center_to_origin_sign.z : normal_direction.z);
            glm::vec3 t_delta = ((block_center + step) - origin) / revised_normal_direction;
            // If any number is 0, then we max the delta so that we don't get a false positive
            if (t_delta.x == 0.0f) t_delta.x = 1e10;
            if (t_delta.y == 0.0f) t_delta.y = 1e10;
            if (t_delta.z == 0.0f) t_delta.z = 1e10;
            glm::vec3 t_max = t_delta;
            float min_t_value;
            do
            {
                t_delta = (block_center - origin) / revised_normal_direction;
                t_max = t_delta;
                min_t_value = FLT_MAX;
                if (t_max.x < t_max.y)
                {
                    if (t_max.x < t_max.z)
                    {
                        block_center.x += step.x;
                        // Check if we actually hit the block
                        if (DoRayTracing(origin, normal_direction, max_distance, draw, block_center, step, &result))
                            return result;
                        min_t_value = t_max.x;
                    }
                    else
                    {
                        block_center.z += step.z;
                        if (DoRayTracing(origin, normal_direction, max_distance, draw, block_center, step, &result))
                            return result;
                        min_t_value = t_max.z;
                    }
                }
                else
                {
                    if (t_max.y < t_max.z)
                    {
                        block_center.y += step.y;
                        if (DoRayTracing(origin, normal_direction, max_distance, draw, block_center, step, &result))
                            return result;
                        min_t_value = t_max.y;
                    }
                    else
                    {
                        block_center.z += step.z;
                        if (DoRayTracing(origin, normal_direction, max_distance, draw, block_center, step, &result))
                            return result;
                        min_t_value = t_max.z;
                    }
                }
            } while (min_t_value < max_distance);

            return result;
        }

        // ----------------------------------------------------------------------------------------------------------
        // Functions Implementation

        // Resolve static collision
        // Parameters: entity id, rigid body, transform, hit box
        static void ResolveStaticCollision(ECS::EntityId entity, RigidBody &rb, Transform &transform, HitBox &hit_box)
        {
            // Get all face coordinate of the hit box
            // ceil : return the nearest integer >= expression
            auto right_x = (int32) glm::ceil(transform.position.x + hit_box.size.x * 0.5f);
            auto left_x = (int32) glm::ceil(transform.position.x - hit_box.size.x * 0.5f);
            auto front_z = (int32) glm::ceil(transform.position.z + hit_box.size.z * 0.5f);
            auto back_z = (int32) glm::ceil(transform.position.z - hit_box.size.z * 0.5f);
            auto top_y = (int32) glm::ceil(transform.position.y + hit_box.size.y * 0.5f) ;
            auto bottom_y = (int32) glm::ceil(transform.position.y - hit_box.size.y * 0.5f);

            HitBox default_block_box{};
            default_block_box.size = glm::vec3(1.0f, 1.0f, 1.0f);
            Transform block_transform{};
            block_transform.front = glm::vec3(1, 0, 0);
            block_transform.up = glm::vec3(0, 1, 0);
            block_transform.right = glm::vec3(0, 0, 1);
            block_transform.yaw = 0;
            block_transform.pitch = 0;
            block_transform.scale = glm::vec3(1, 1, 1);

            bool did_collision = false;
            for (int32 y = top_y; y >= bottom_y; y--)
                for  (int32 x = left_x; x <= right_x; x++)
                    for (int32 z = back_z; z <= front_z; z++)
                    {
                        glm::vec3 box_pos = glm::vec3(x - 0.5f, y - 0.5f, z - 0.5f);
                        Block block = ChunkManager::GetBlock(box_pos);
                        BlockFormat block_format = get_block(block.block_id);

                        block_transform.position = box_pos;

                        if (block_format.m_is_solid && IsColliding(hit_box, transform, default_block_box, block_transform))
                        {
                            CollisionInfo collision_info
                            = StaticCollisionInformation(rb, hit_box, transform, default_block_box, block_transform);

                            float dot_product = glm::dot(glm::normalize(collision_info.overlap_part), glm::normalize(rb.velocity));
                            if (dot_product < 0)
                            {
                                // We're already moving out of the collision, don't do anything
                                continue;
                            }
                            transform.position -= collision_info.overlap_part;
                            switch (collision_info.force)
                            {
                                case Direction::BOTTOM:
                                case Direction::TOP:
                                    rb.acceleration.y = 0;
                                    rb.velocity.y = 0;
                                    break;
                                case Direction::RIGHT:
                                case Direction::LEFT:
                                    rb.velocity.x = 0;
                                    rb.acceleration.x = 0;
                                    break;
                                case Direction::FRONT:
                                case Direction::BACK:
                                    rb.velocity.z = 0;
                                    rb.acceleration.z = 0;
                                    break;
                                default:
                                    break;
                            }
                            rb.on_ground = rb.on_ground || collision_info.force == Direction::BOTTOM;
                            did_collision = true;
                        }
                    }

            if (!did_collision && rb.on_ground && rb.velocity.y > 0)
            {
                // If we're not colliding with any object it's impossible to be on the ground
                rb.on_ground = false;
            }
        }

        static bool IsColliding(const HitBox &hb1, const Transform &tr1
                , const HitBox &hb2, const Transform &tr2)
        {
            glm::vec3 test_axes[3] = {
                    glm::vec3(1, 0, 0),
                    glm::vec3(0, 1, 0),
                    glm::vec3(0, 0, 1) };

            // negative axis face's position
            glm::vec3 hb1_negative = tr1.position - (hb1.size * 0.5f);
            glm::vec3 hb2_negative = tr2.position - (hb2.size * 0.5f);

            // positive axis face's position
            glm::vec3 hb1_positive = tr1.position + (hb1.size * 0.5f);
            glm::vec3 hb2_positive = tr2.position + (hb2.size * 0.5f);

            // check each face if there is an overlapping

            /*for (int i = 0; i < 3; i++)
            {
                float f1_positive = glm::dot(hb1_positive, test_axes[i]);
                float f2_negative = glm::dot(hb2_negative, test_axes[i]);
                if (f2_negative - f1_positive <= 0.001f)
                    return true;

                float f1_negative = glm::dot(hb1_negative, test_axes[i]);
                float f2_positive = glm::dot(hb2_positive, test_axes[i]);
                if (f1_negative - f2_positive <= 0.001f)
                    return true;
            }
            return false;
            */

            for (auto test_axe : test_axes)
            {
                float penetration = PenetrationAmount(hb1_negative, hb1_positive
                                                      , hb2_negative, hb2_positive
                                                      , test_axe);
                if (glm::abs(penetration) <= 0.001f)
                    return false;
            }
            return true;
        }

        static float PenetrationAmount(const glm::vec3 &hb1_negative, const glm::vec3 &hb1_positive
                                    ,const glm::vec3 &hb2_negative, const glm::vec3 &hb2_positive
                                    , const glm::vec3 &axis)
        {
            if (axis == glm::vec3(1, 0, 0))
            {
                if ((hb2_negative.x <= hb1_positive.x) && (hb1_negative.x <= hb2_positive.x))
                {
                    // We have penetration
                    return hb2_negative.x - hb1_positive.x;
                }
            }
            else if (axis == glm::vec3(0, 1, 0))
            {
                if ((hb2_negative.y <= hb1_positive.y) && (hb1_negative.y <= hb2_positive.y))
                {
                    // We have penetration
                    return hb2_positive.y - hb1_negative.y;
                }
            }
            else if (axis == glm::vec3(0, 0, 1))
            {
                if ((hb2_negative.z <= hb1_positive.z) && (hb1_negative.z <= hb2_positive.z))
                {
                    // We have penetration
                    return hb2_negative.z - hb1_positive.z;
                }
            }

            return 0.0f;
        }



        static CollisionInfo StaticCollisionInformation(const RigidBody& rb1, const HitBox &hb1, const Transform& tr1
                , const HitBox &hb2, const Transform& tr2)
        {
            CollisionInfo res{};
            res.did_collision = true;
            HitBox hb2_expanded = hb2;
            hb2_expanded.size += hb1.size;

            // Figure out which quadrant the collision is and resolve it
            glm::vec3 hb1_to_hb2 = tr1.position - tr2.position;

            if (hb1_to_hb2.x > 0 && hb1_to_hb2.y > 0 && hb1_to_hb2.z > 0)
            {
                // We are in the top-right-front quadrant
                GetQuadrantResult(tr1, tr2, hb2_expanded, &res, Direction::LEFT, Direction::BOTTOM, Direction::BACK);
            }
            else if (hb1_to_hb2.x > 0 && hb1_to_hb2.y > 0 && hb1_to_hb2.z <= 0)
            {
                // We are in the top-right-back quadrant
                GetQuadrantResult(tr1, tr2, hb2_expanded, &res, Direction::LEFT, Direction::BOTTOM, Direction::FRONT);
            }
            else if (hb1_to_hb2.x > 0 && hb1_to_hb2.y <= 0 && hb1_to_hb2.z > 0)
            {
                // We are in the bottom-right-front quadrant
                GetQuadrantResult(tr1, tr2, hb2_expanded, &res, Direction::LEFT, Direction::TOP, Direction::BACK);
            }
            else if (hb1_to_hb2.x > 0 && hb1_to_hb2.y <= 0 && hb1_to_hb2.z <= 0)
            {
                // We are in the bottom-right-back quadrant
                GetQuadrantResult(tr1, tr2, hb2_expanded, &res, Direction::LEFT, Direction::TOP, Direction::FRONT);
            }
            else if (hb1_to_hb2.x <= 0 && hb1_to_hb2.y > 0 && hb1_to_hb2.z > 0)
            {
                // We are in the top-left-front quadrant
                GetQuadrantResult(tr1, tr2, hb2_expanded, &res, Direction::RIGHT, Direction::BOTTOM, Direction::BACK);
            }
            else if (hb1_to_hb2.x <= 0 && hb1_to_hb2.y > 0 && hb1_to_hb2.z <= 0)
            {
                // We are in the top-left-back quadrant
                GetQuadrantResult(tr1, tr2, hb2_expanded, &res, Direction::RIGHT, Direction::BOTTOM, Direction::FRONT);
            }
            else if (hb1_to_hb2.x <= 0 && hb1_to_hb2.y <= 0 && hb1_to_hb2.z > 0)
            {
                // We are in the bottom-left-front quadrant
                GetQuadrantResult(tr1, tr2, hb2_expanded, &res, Direction::RIGHT, Direction::TOP, Direction::BACK);
            }
            else if (hb1_to_hb2.x <= 0 && hb1_to_hb2.y <= 0 && hb1_to_hb2.z <= 0)
            {
                // We are in the bottom-left-back quadrant
                GetQuadrantResult(tr1, tr2, hb2_expanded, &res, Direction::RIGHT, Direction::TOP, Direction::FRONT);
            }
            else
            {
                AmoLogger_Error("Could not evaluate physics calculation.");
                res.did_collision = false;
            }
            return res;
        }

        static float GetDirection(Direction face)
        {
            switch (face)
            {
                case Direction::BACK:
                    return 1;
                case Direction::FRONT:
                    return -1;
                case Direction::RIGHT:
                    return -1;
                case Direction::LEFT:
                    return 1;
                case Direction::TOP:
                    return -1;
                case Direction::BOTTOM:
                    return 1;
                case Direction::NONE:
                    AmoLogger_Error("Could not get direction from face!");
            }
            return 0.0001f;
        }

        static void GetQuadrantResult(const Transform &tr1, const Transform &tr2, const HitBox &hb2_expanded
                , CollisionInfo* res ,Direction x_face, Direction y_face, Direction z_face)
        {
            float x_direction = GetDirection(x_face);
            float y_direction = GetDirection(y_face);
            float z_direction = GetDirection(z_face);

            glm::vec3 hb2_expanded_size_by_direction = {
                    hb2_expanded.size.x * x_direction,
                    hb2_expanded.size.y * y_direction,
                    hb2_expanded.size.z * z_direction
            };
            glm::vec3 quadrant = (hb2_expanded_size_by_direction * 0.5f) + tr2.position;
            glm::vec3 delta = tr1.position - quadrant;
            glm::vec3 abs_delta = glm::abs(delta);

            if (abs_delta.x < abs_delta.y && abs_delta.x < abs_delta.z)
            {
                // We are colliding with the right-face
                res->overlap_part = delta.x * glm::vec3(1, 0, 0);
                res->force = x_face;
            }
            else if (abs_delta.y < abs_delta.x && abs_delta.y < abs_delta.z)
            {
                // We are colliding with the top-face
                res->overlap_part = delta.y * glm::vec3(0, 1, 0);
                res->force = y_face;
            }
            else
            {
                // We are colliding with the front-face
                res->overlap_part = delta.z * glm::vec3(0, 0, 1);
                res->force = z_face;
            }
        }
/*
        static Interval GetInterval(const HitBox& box, const Transform& transform, const glm::vec3& axis)
        {
            glm::vec3 vertices[8];
            glm::vec3 center = transform.position;
            glm::vec3 half_size = box.size * 0.5f;
            vertices[0] = center + glm::vec3(0, 0, 1) * half_size.z + glm::vec3(0, 1, 0) * half_size.y + glm::vec3(1, 0, 0) * half_size.x; // ForwardTopRight
            vertices[1] = center + glm::vec3(0, 0, 1) * half_size.z + glm::vec3(0, 1, 0) * half_size.y - glm::vec3(1, 0, 0) * half_size.x; // ForwardTopLeft
            vertices[2] = center + glm::vec3(0, 0, 1) * half_size.z - glm::vec3(0, 1, 0) * half_size.y + glm::vec3(1, 0, 0) * half_size.x; // ForwardBottomRight
            vertices[3] = center + glm::vec3(0, 0, 1) * half_size.z - glm::vec3(0, 1, 0) * half_size.y - glm::vec3(1, 0, 0) * half_size.x; // ForwardBottomLeft
            vertices[4] = center - glm::vec3(0, 0, 1) * half_size.z + glm::vec3(0, 1, 0) * half_size.y + glm::vec3(1, 0, 0) * half_size.x; // BackTopRight
            vertices[5] = center - glm::vec3(0, 0, 1) * half_size.z + glm::vec3(0, 1, 0) * half_size.y - glm::vec3(1, 0, 0) * half_size.x; // BackTopLeft
            vertices[6] = center - glm::vec3(0, 0, 1) * half_size.z - glm::vec3(0, 1, 0) * half_size.y + glm::vec3(1, 0, 0) * half_size.x; // BackBottomRight
            vertices[7] = center - glm::vec3(0, 0, 1) * half_size.z - glm::vec3(0, 1, 0) * half_size.y - glm::vec3(1, 0, 0) * half_size.x; // BackBottomLeft

            Interval result;
            result._min = result._max = glm::dot(axis, vertices[0]);
            for (int i = 1; i < 8; i++)
            {
                float projection = glm::dot(axis, vertices[i]);
                result._min = glm::min(result._min, projection);
                result._max = glm::max(result._max, projection);
            }

            return result;
        }

        static glm::vec3 ClosetPointOnRay(const glm::vec3 ray_origin, const glm::vec3 ray_direction
                , float ray_max_distance, glm::vec3 &point)
        {
            glm::vec3 origin_to_point = point - ray_origin;
            glm::vec3 ray_segment = ray_direction * ray_max_distance;

            // ray_ms = ray_max_distance .2
            float ray_magnitude_squared = ray_max_distance * ray_max_distance;
            float dot_product = glm::dot(origin_to_point, ray_segment);
            float distance = dot_product / ray_magnitude_squared; //The normalized "distance" from a(point) to your closest point

            if (distance < 0)     //Check if P projection is over vectorAB
            {
                return ray_origin;
            }
            else if (distance > 1)
            {
                return ray_origin + (ray_direction * ray_max_distance);
            }

            return ray_origin + ray_direction * distance * ray_max_distance;
        }

*/
    }