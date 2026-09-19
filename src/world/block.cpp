#include "world/block.h"

namespace SymoCraft{
    static robin_hood::unordered_flat_map<uint16, BlockFormat> block_format_map;
    static robin_hood::unordered_flat_map<std::string , uint16> name_to_id_map;

    uint16 get_block_id(std::string_view block_name)
    {
        const auto& iter = name_to_id_map.find(block_name.data());
        if (iter == name_to_id_map.end())
        {
            return 0;
        }
        return iter->second;
    }

    const BlockFormat& get_block(int block_id)
    {
        if (block_format_map.contains(block_id))
        {
            return block_format_map[block_id];
        }
        return block_format_map[0];
    }


    const BlockFormat& get_block(std::string_view name)
    {
        int blockId = get_block_id(name);
        return block_format_map[blockId];
    }

    void LoadBlocks(std::string_view block_format_config)
    {
        YAML::Node block_formats = YAML::LoadFile(block_format_config.data());

        block_format_map[0] = {255, 255, 255,
                               true, false, false,
                               false, 0};

        for (auto block : block_formats)
        {
            // AmoLogger_Notice( !block.second["id"].IsDefined(), "All blocks must have a block id defined. Block '%s' does not have an id.", block.first.as<std::string>().c_str() );
            int id = block.second["id"].as<int>();
            name_to_id_map[block.first.as<std::string>()] = id;

            auto side_texture =  block.second["side"].as<uint16>();
            auto top_texture= block.second["top"].as<uint16>();
            auto bottom_texture = block.second["bottom"].as<uint16>();
            bool isTransparent = block.second["isTransparent"].as<bool>();
            bool isSolid = block.second["isSolid"].as<bool>();
            bool isBlendable = block.second["IsBlendable"].IsDefined() && block.second["IsBlendable"].as<bool>();
            bool isLightSource = block.second["IsLightSource"].IsDefined() && block.second["IsLightSource"].as<bool>();
            int16 lightLevel = block.second["m_light_level"].IsDefined() ? block.second["m_light_level"].as<int16>() : 0;

            if (block_format_map.contains(id))
            {
                AmoLogger_Warning("Block format detected a duplicate block id '%d'. Do you have two blocks with id '%d'?", id, id);
            }

            block_format_map[id] = BlockFormat{
                    top_texture, side_texture, bottom_texture,
                    isTransparent, isSolid, isBlendable,
                    isLightSource, lightLevel };
        }
    }
}
