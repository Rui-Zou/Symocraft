#include "world/chunk_manager.h"
#include "world/chunk.h"
#include "core/constants.h"
#include "renderer/renderer.h"

namespace SymoCraft{

    static robin_hood::unordered_node_map<glm::ivec2, Chunk> chunks;

    namespace ChunkManager {
        Block GetBlock(const glm::vec3 &worldPosition) {
            Chunk *chunk = GetChunk(worldPosition);

            if (!chunk)
                return BlockConstants::NULL_BLOCK;

            return chunk->GetWorldBlock(worldPosition);
        }

        void SetBlock(const glm::vec3 &worldPosition, uint16 block_id) {
            glm::ivec2 chunkCoords = World::ToChunkCoords(worldPosition);
            Chunk *chunk = GetChunk(worldPosition);

            if (!chunk) {
                if (worldPosition.y >= 0 && worldPosition.y < 256) {
                    // Assume it's a chunk that's out of bounds
                    AmoLogger_Warning("Tried to set invalid block at position<%2.3f, %2.3f, %2.3f>!", worldPosition.x,
                                      worldPosition.y, worldPosition.z);
                }
                return;
            }

            chunk->SetWorldBlock(worldPosition, block_id);
        }

        void RemoveBLock(const glm::vec3 &worldPosition) {
            Chunk *chunk = GetChunk(worldPosition);

            if (!chunk) {
                if (worldPosition.y >= 0 && worldPosition.y < 256) {
                    // Assume it's a chunk that's out of bounds
                    AmoLogger_Warning("Tried to set invalid block at position<%2.3f, %2.3f, %2.3f>!", worldPosition.x,
                                      worldPosition.y, worldPosition.z);
                }
                return;
            }

            chunk->RemoveWorldBlock(worldPosition);
        }

        Chunk *GetChunk(const glm::vec3 &worldPosition)
        {
            glm::ivec2 chunkCoords = World::ToChunkCoords(worldPosition);
            return GetChunk(chunkCoords);
        }

        Chunk* GetChunk(const glm::ivec2& chunkCoords)
        {
            const robin_hood::unordered_map<glm::ivec2, Chunk>::iterator& iter = chunks.find(chunkCoords);
            if (iter != chunks.end())
                return &iter->second;
            else
                return nullptr;
        }

        robin_hood::unordered_node_map<glm::ivec2, Chunk>& GetAllChunks()
        {
            return chunks;
        }

        void CreateChunk(const glm::ivec2 &chunk_coord)
        {
            static uint16 chunk_index;
            Chunk *chunk = GetChunk(chunk_coord);
            //If the chunk hasn't been instantiated, create the chunk
            if (!chunk)
            {
                Chunk new_chunk{};
                new_chunk.m_local_blocks = (Block *)AmoMemory_Allocate(sizeof(Block) * k_chunk_length * k_chunk_width * k_chunk_height);
                new_chunk.m_chunk_coord = chunk_coord;
                new_chunk.m_vertex_data = (BlockVertex3D *) AmoMemory_Allocate(sizeof(BlockVertex3D) * World::max_vertices_per_chunk);
                new_chunk.m_vertex_count = 0;
                new_chunk.m_draw_command.first = chunk_index++ * sizeof(BlockVertex3D) * World::max_vertices_per_chunk;
                new_chunk.m_draw_command.baseInstance = 0;
                new_chunk.m_draw_command.instanceCount = 1;
                new_chunk.front_neighbor = GetChunk(chunk_coord + INormals2::Front);
                new_chunk.back_neighbor = GetChunk(chunk_coord + INormals2::Back);
                new_chunk.left_neighbor = GetChunk(chunk_coord + INormals2::Left);
                new_chunk.right_neighbor = GetChunk(chunk_coord + INormals2::Right);
                new_chunk.state = ChunkState::ToBeUpdated;

                chunks[new_chunk.m_chunk_coord] = new_chunk;
            }
        }

        void RearrangeChunkNeighborPointers()
        {
            for (auto& pair : chunks)
            {
                Chunk& chunk = pair.second;
                auto iter1 = chunks.find(chunk.m_chunk_coord + INormals2::Front);
                chunk.front_neighbor = iter1 == chunks.end() ? nullptr : &iter1->second;
                auto iter2 = chunks.find(chunk.m_chunk_coord + INormals2::Back);
                chunk.back_neighbor = iter2 == chunks.end() ? nullptr : &iter2->second;
                auto iter3 = chunks.find(chunk.m_chunk_coord + INormals2::Left);
                chunk.left_neighbor = iter3 == chunks.end() ? nullptr : &iter3->second;
                auto iter4 = chunks.find(chunk.m_chunk_coord + INormals2::Right);
                chunk.right_neighbor = iter4 == chunks.end() ? nullptr : &iter4->second;

                if(chunk.front_neighbor == nullptr || chunk.back_neighbor == nullptr ||
                   chunk.left_neighbor == nullptr || chunk.right_neighbor == nullptr)
                    chunk.m_is_fringe_chunk = true;
            }
        }

        void UpdateAllChunks()
        {
            for(auto &pair : chunks)
                if (pair.second.state == ChunkState::Updated || pair.second.m_is_fringe_chunk)
                    continue;
                else if(pair.second.state == ChunkState::ToBeUpdated)
                    pair.second.GenerateRenderData();
                else
                    AmoLogger_Info("Unknown state of chunk updated\n");
        }

        void LoadAllChunks()
        {
            for(auto &pair : chunks)
                if(pair.second.m_vertex_count == 0 || pair.second.m_is_fringe_chunk)
                    continue;
                else
                    chunk_batch.AddVertex(pair.second.m_vertex_data, pair.second.m_vertex_count);
        }

        void FreeAllChunks()
        {
            for(auto &pair : chunks)
                // if (pair.second.m_vertex_count != 0)
                    pair.second.Free();

        }
    }
}
