#include "AssetManager.h"

#include <fmt/format.h>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "generators/GenerateShaderLookup.h"
#include "generators/IAssetGenerator.h"

namespace assets_system
{
    void trimEnd(std::string& string, const char* ofchars)
    {
        string.erase(string.find_last_not_of(ofchars) + 1);
    }

    void removeAllEmptyDirs(const std::string& path)
    {
        // This will remove empty directories in multiple passes
    restart:
        for (auto it = std::filesystem::recursive_directory_iterator(path); it != std::filesystem::recursive_directory_iterator(); ++it)
            if (it->is_directory() && std::filesystem::is_empty(it->path()))
            {
                std::filesystem::remove(it->path());
                goto restart;
            }
    }

    std::string joinCSV(const std::vector<std::string>& line)
    {
        std::string result;
        for (const std::string& value : line)
            result += value + ',';
        trimEnd(result, ",");
        return result;
    }

    std::vector<std::string> splitCSV(const std::string& line)
    {
        bool inQuotes = false;
        std::string value;
        std::vector<std::string> result;
        for (const char c : line)
        {
            if (c == '"')
                inQuotes = !inQuotes;
            else if (c == ',' && !inQuotes)
            {
                result.push_back(value);
                value.clear();
            } else
                value += c;
        }
        if (result.size() > 0 || value.size() > 0)
            result.push_back(value);
        return result;
    }

    bool readCSVLine(std::ifstream& file, std::vector<std::string>& outResult)
    {
        std::string line;
        if (!std::getline(file, line))
            return false;
        trimEnd(line, "\r\n");

        outResult = splitCSV(line);

        return true;
    }

    void writeCSVLine(std::ofstream& file, const std::vector<std::string>& row)
    {
        file << joinCSV(row) << "\n";
    }

    std::vector<std::string> AssetManager::GenerateAsset(const std::string& assetPath)
    {
        const std::filesystem::path readPath(assetPath);
        std::filesystem::create_directories(GEN_ASSET_DIR + readPath.parent_path().string());

        std::ifstream assetFile(ASSET_DIR + assetPath, std::ios::binary | std::ios::ate);

        if (!assetFile.is_open())
            throw std::ios::failure("Could not open file");

        size_t fileSize = assetFile.tellg();
        std::vector<std::byte> buffer(fileSize);

        assetFile.seekg(0);
        assetFile.read(reinterpret_cast<char*>(buffer.data()), fileSize);
        assetFile.close();

        std::string extension = readPath.extension().string();
        auto it = assetGenerators.find(std::hash<std::string> {}(extension.c_str()));

        return it == assetGenerators.end() ? std::vector<std::string>() : it->second->GenerateAssets(assetPath, std::move(buffer));
    }

    void AssetManager::WriteAssetLookup(const std::unordered_map<uint32_t, std::vector<std::string>>& assetNames)
    {
        constexpr auto ASSET_LOOKUP = PROJECT_ROOT"/src/assets-system/assets-system/lookup";
        for (const auto& [id, names] : assetNames)
        {
            if (names.size() == 0)
                continue;

            std::string result = "#pragma once\n#include \"assets-system/AssetID.h\"\n\nnamespace assets_system::lookup\n{\n";
            for (uint32_t idx = 0; idx < names.size(); ++idx)
            {
                std::string field = PrettyNameOfAsset(names[idx]);
                result += fmt::format("    constexpr AssetID {} = {{ {}, {} }};\n", field, id, idx);
            }
            result += "}";

            // write each assets generated files to a seperate lookup file, this minimises rebuilding when an asset is moved / created / destroyed
            std::string filename = fmt::format("{}/Asset{}.h", ASSET_LOOKUP, id);

            std::ifstream reader(filename);
            if (reader.is_open())
            {
                std::string existingContent((std::istreambuf_iterator(reader)), std::istreambuf_iterator<char>());
                reader.close();

                if (existingContent == result)
                    continue;
            }

            // only update lookup when necessary to prevent triggering a rebuild.
            std::ofstream writer(filename);
            if (!writer.is_open())
                throw std::ios::failure("Could not open file");

            writer.write(result.c_str(), result.size());
        }
        for (const auto& entry : std::filesystem::directory_iterator(ASSET_LOOKUP))
        {
            if (!entry.is_regular_file())
                continue;
            std::string filename = entry.path().filename().string();

            if (filename.starts_with("Asset") && filename.ends_with(".h"))
            {
                size_t offset = std::size("Asset") - 1;
                size_t count = filename.find_first_of('.') - offset;
                int id = std::stoi(filename.substr(offset, count));
                if (!assetNames.contains(id))
                    std::filesystem::remove(entry.path());
            }
        }
    }

    void AssetManager::AddAssetGenerator(const char* extension, const char* assetType, IAssetGenerator* delegate)
    {
        const size_t hash = std::hash<std::string> {}(extension);
        assetTypes.insert_or_assign(hash, assetType);
        assetGenerators.insert_or_assign(hash, delegate);
    }

    bool AssetManager::RefreshAssets(bool refreshAll)
    {
        constexpr auto ASSET_INDEX = PROJECT_ROOT"/assets-meta/assets_index.csv";
        constexpr auto GEN_ASSET_INDEX = PROJECT_ROOT"/assets-meta/gen_assets_index.csv";

        // read assets index to see what needs updating
        std::ifstream assetReader(ASSET_INDEX);

        if (!assetReader.is_open())
            return false;

        std::unordered_map<uint32_t, std::vector<std::string>> genAssetMap;

        std::vector<std::string> assetHeader;
        readCSVLine(assetReader, assetHeader);
        assert(assetHeader[ID] == "id" && assetHeader[PATH] == "path" && assetHeader[DIRTY] == "dirty");

        std::vector<std::string> metadata;
        readCSVLine(assetReader, metadata);
        assert(metadata[PATH] == "__metadata__");

        const bool anyDirty = std::stoi(metadata[DIRTY]);

        if (!refreshAll && !anyDirty)
            return false;

        std::vector<std::string> row;

        std::vector<std::vector<std::string>> data;
        while (readCSVLine(assetReader, row))
        {
            if (row.size() == 0)
                continue;
            data.push_back(row);
            genAssetMap[std::stoi(row[ID])] = {};
        }

        assetReader.close();

        // read generated assets, delete any that are no longer associated with an id
        std::ifstream genAssetReader(GEN_ASSET_INDEX);

        std::vector<std::string> genAssetHeader;
        if (genAssetReader.is_open())
        {
            readCSVLine(genAssetReader, genAssetHeader);
            assert(genAssetHeader[ID] == "id" && genAssetHeader[PATH] == "paths");

            while (readCSVLine(genAssetReader, row))
            {
                if (row.size() == 0)
                    continue;

                uint32_t id = std::stoi(row[ID]);

                auto oldGenAssetsPaths = splitCSV(row[PATH]);

                if (genAssetMap.contains(id))
                {
                    genAssetMap[id] = std::move(oldGenAssetsPaths);
                    continue;
                }

                for (const std::string& oldGenAssetPath : oldGenAssetsPaths)
                {
                    std::string filename = GEN_ASSET_DIR + oldGenAssetPath;
                    std::remove(filename.c_str());
                }
            }
            genAssetReader.close();
        }

        bool anyShadersDirty = refreshAll;

        // regenerate dirty generated assets
        for (auto& entry : data)
        {
            if (!refreshAll && !std::stoi(entry[DIRTY]))
                continue;

            uint32_t id = std::stoi(entry[ID]);

            auto it = genAssetMap.find(id);
            if (it != genAssetMap.end())
                for (const std::string& oldGenAssetPath : it->second)
                {
                    std::string filename = GEN_ASSET_DIR + oldGenAssetPath;
                    std::remove(filename.c_str());
                }

            anyShadersDirty |= ".hlsl" == std::filesystem::path(entry[PATH]).extension();

            genAssetMap[id] = GenerateAsset(entry[PATH]);
            entry[DIRTY] = "0";
        }
        metadata[DIRTY] = "0";

        removeAllEmptyDirs(GEN_ASSET_DIR);
        WriteAssetLookup(genAssetMap);

        // a bit messy, but this is a special case. no point making a general system for managers to do post processing
        if (anyShadersDirty)
            GenerateShaderMetadata(GEN_ASSET_DIR, ".hlsl.asset", PROJECT_ROOT"/src/engine/rendering/pass-system/shader_descriptors.h");

        // mark all assets not dirty
        std::ofstream assetWriter(ASSET_INDEX);

        if (!assetWriter.is_open())
            throw std::ios::failure("Could not open file");

        writeCSVLine(assetWriter, assetHeader);
        writeCSVLine(assetWriter, metadata);
        for (const auto& entry : data)
            writeCSVLine(assetWriter, entry);

        assetWriter.close();

        // update generated assets index with new data
        std::ofstream genAssetWriter(GEN_ASSET_INDEX);

        if (!genAssetWriter.is_open())
            throw std::ios::failure("Could not open file");

        writeCSVLine(genAssetWriter, genAssetHeader);
        for (const auto& [id, paths] : genAssetMap)
            genAssetWriter << fmt::format("{},\"{}\"\n", std::to_string(id), joinCSV(paths));

        genAssetWriter.close();;

        return true;
    }

    AssetFile AssetManager::LoadAsset(AssetID assetId)
    {
        constexpr auto GEN_ASSET_INDEX = PROJECT_ROOT"/assets-meta/gen_assets_index.csv";

        std::ifstream genAssetReader(GEN_ASSET_INDEX);

        if (!genAssetReader.is_open())
            return AssetFile::Invalid();

        std::vector<std::string> row;
        readCSVLine(genAssetReader, row);
        assert(row[ID] == "id" && row[PATH] == "paths");

        while (readCSVLine(genAssetReader, row))
        {
            if (row.size() == 0)
                continue;

            uint32_t id = std::stoi(row[ID]);

            if (id != assetId.id)
                continue;

            auto assetPaths = splitCSV(row[PATH]);

            if (assetPaths.size() <= assetId.idx)
                return AssetFile::Invalid();

            const std::string assetFilename = GEN_ASSET_DIR + assetPaths[assetId.idx];
            AssetFile result = AssetFile::Load(assetFilename.c_str());

            return result;
        }
        return AssetFile::Invalid();
    }

    std::string AssetManager::PrettyNameOfAsset(const std::string& relativeAssetPath)
    {
        std::filesystem::path path(relativeAssetPath);

        if (path.extension() == ".asset")
            path = path.stem();

        const std::string extension = path.extension().string();
        const auto it = assetTypes.find(std::hash<std::string> {}(extension));

        std::string type = it == assetTypes.end() ? "____" : it->second;

        std::string field = fmt::format("{}_{}", type, path.stem().string());
        for (size_t i = 0; i < field.size(); ++i)
            if (!std::isalnum(field[i]))
                field[i] = '_';

        return field;
    }
}
