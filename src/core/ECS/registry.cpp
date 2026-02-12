//
// Created by Amo on 2022/6/17.
//

#include "core/ECS/registry.h"
#include "core.h"

namespace SymoCraft::ECS
    {
        typedef uint32 EntityIndex;
        typedef uint32 EntityType;
        typedef uint64 EntityId;
        typedef uint32 ComponentIndex;

        // ----------------------------------------------------------------------
        // Registry Implement

        void Registry::Free()
        {
            for (Internal::ComponentContainer& container : component_set)
                container.Free();
        }

        void Registry::Clear()
        {
            entities.clear();
            Free();
            component_set.clear();
            component_set = std::vector<Internal::ComponentContainer>();
            debug_component_names.clear();
            free_entities.clear();
        }

        EntityId Registry::CreateEntity()
        {
            if (!free_entities.empty())
            {
                EntityIndex new_index = free_entities.back();
                free_entities.pop_back();
                EntityId new_id = Internal::CreateEntityId(new_index
                        , Internal::GetEntityVersion(entities[new_index]));
                entities[new_index] = new_id;
                return new_id;
            }
            entities.emplace_back(Internal::CreateEntityId((uint32)entities.size(), 0));
            return entities.back();
        }

        int Registry::NumComponents(EntityId entity) const
        {
            int num_components = 0;
            for (int component_id = 0; component_id < component_set.size(); component_id++)
                if (HasComponentByType(entity, component_id))
                    num_components++;

            return num_components;
        }

        bool Registry::IsEntityValid(EntityId entity) const
        {
            return Internal::GetEntityIndex(entity) < entities.size() && Internal::GetEntityIndex(entity) != UINT32_MAX;
        }

        void Registry::DestroyEntity(EntityId entity)
        {
            RemoveAllComponent(entity);
            EntityId new_id = Internal::CreateEntityId(UINT32_MAX, Internal::GetEntityVersion(entity) + 1);
            entities[Internal::GetEntityIndex(entity)] = new_id;
            free_entities.push_back(Internal::GetEntityIndex(entity));
        }


        bool Registry::HasComponentByType(EntityId entity, int32 component_type) const
        {
            if (!IsEntityValid(entity))
                return false;

            if (component_type >= component_set.size() || component_type < 0)
            {
                AmoLogger_Warning("Tried to check if an entity had component '%d'"
                                ", but a component of type '%d' does not exist in the registry "
                                "which only has '%d' components."
                                , component_type, component_type, component_set.size());
                return false;
            }

            return component_set[component_type].IsComponentExist(entity);
        }


        uint8* Registry::GetComponentByType(EntityId entity, int32 component_type) const
        {
            if (!IsEntityValid(entity))
            {
                AmoLogger_Error("Cannot check if invalid entity %d has a component."
                                , entity);
                return nullptr;
            }

            if (component_type >= component_set.size() || component_type < 0)
            {
                AmoLogger_Warning("Tried to check if an entity had component '%d', "
                                  "but a component of type '%d' does not exist in the registry."
                                  , component_type, component_type);
                return nullptr;
            }

            return component_set[component_type].Get(Internal::GetEntityIndex(entity));
        }

        uint8* Registry::AddOrGetComponentByType(EntityId entity, int32 component_id)
        {
            if (!IsEntityValid(entity))
            {
                AmoLogger_Error("Cannot check if invalid entity %d has a component."
                                , Internal::GetEntityIndex(entity));
                return nullptr;
            }

            if (component_id >= component_set.size() || component_id < 0)
            {
                AmoLogger_Warning("Tried to check if an entity had component '%d'"
                                  ", but a component of type '%d' does not exist in the registry"
                                  , component_id, component_id);
                return nullptr;
            }

            return component_set[component_id].AddOrGet(entity);
        }

        void Registry::RemoveAllComponent(EntityId entity)
        {
            if (!IsEntityValid(entity))
            {
                AmoLogger_Error("Tried to remove invalid entity %d 's component."
                                , entity);
                return;
            }

            EntityIndex entity_index = Internal::GetEntityIndex(entity);
            if (entity_index >= entities.size())
            {
                AmoLogger_Error("Tried to remove all components from invalid entity '%d'"
                , entity);
                return;
            }

            for (int i = 0; i < component_set.size(); i++)
                if (component_set[i].IsComponentExist(entity))
                    component_set[i].Remove(entity);
        }

        RawMemory Registry::Serialize()
        {
            /*
             * uint32 number of entities
             * Begin looping entities
             * uint64 entity id
             * int32 number of components
             * Begin looping components
             * int32 component id
             * Copy component into this entity id or create this entity if it does not exist
            */

            RawMemory memory;
            size_t entity_id_data_size = sizeof(uint32) + (sizeof(uint16) * entities.size());
            memory.Init(entity_id_data_size);

            auto num_of_entities = (uint32)entities.size();
            memory.Write<uint32>(&num_of_entities);
            for (ECS::EntityId entity : entities)
            {
                memory.Write<EntityId>(&entity);
                int32 num_components = this->NumComponents(entity);
                memory.Write<int32>(&num_components);
                for (int i = 0; i <  component_set.size(); i++)
                {
                    if (HasComponentByType(entity, i))
                    {
                        memory.Write<int32>(&i);
                        size_t component_size = component_set[i].GetComponentSize();
                        uint8* component_data = component_set[i].Get(Internal::GetEntityIndex(entity));
                        memory.WriteDangerous(component_data, component_size);
                    }
                }
            }

            memory.ShrinkToFit();
            return memory;
        }

        void Registry::Deserialize(RawMemory &memory)
        {
            memory.ResetReadWriteCursor();

            uint32 num_entities;
            memory.Read<uint32>(&num_entities);
            entities.resize(num_entities);
            for (uint32 entity_counter = 0; entity_counter < num_entities; entity_counter++)
            {
                EntityId entity;
                memory.Read<EntityId>(&entity);
                entities[Internal::GetEntityIndex(entity)] = entity;
                int32 num_components;
                memory.Read<int32>(&num_components);
                AmoLogger_Assert(num_components >= 0, "Deserialized bad data.");
                for (int component_counter = 0; component_counter < num_components; component_counter++)
                {
                    int32 component_id;
                    memory.Read<int32>(&component_id);
                    uint8 *component_data = AddOrGetComponentByType(entity, component_id);
                    size_t component_size = component_set[component_id].GetComponentSize();
                    memory.ReadDangerous(component_data, component_size);
                }
            }
            AmoLogger_Info("Deserialized %d entities.", num_entities);
        }

        // ----------------------------------------------------------------------
        // Iterator implementation
        Iterator::Iterator(Registry &reg, EntityIndex index, std::bitset<Internal::kMaxNumComponents> com_need,
                           bool all)
                           : registry(reg), entity_index(index), components_need(com_need),
                           _is_searching_all(all)
        {/*empty*/}

        EntityIndex Iterator::operator*() const
        {
            return registry.entities[entity_index];
        }

        bool Iterator::operator==(Iterator &other) const
        {
            return entity_index==other.entity_index;
        }


        bool Iterator::operator!=(Iterator &other) const
        {
            return entity_index!=other.entity_index;
        }

        Iterator& Iterator::operator++()
        {
            do
            {
                entity_index++;
            } while (entity_index < registry.entities.size() && !IsIndexValid());
            return *this;
        }

        bool Iterator::IsIndexValid()
        {
            return entity_index >=0 && registry.IsEntityValid(registry.entities[entity_index]) &&
                    (
                            _is_searching_all ||
                            RegistryViewer<>::HasRequiredComponents(registry, components_need
                                                                  , registry.entities[entity_index])
                            );
        }
    }