#include "ShaderAssetGenerator.h"

#include <fmt/format.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <regex>

#include "AssetManager.h"

namespace assets_system
{
    std::vector<std::byte> compile_shader(const std::string& dxcCompiler, const std::string& entry, const std::string& profile, const std::string& hlslFile)
    {
        const std::string tempSpv = PROJECT_ROOT"/tmp_shader.spv";
        const std::string tempError = PROJECT_ROOT"/tmp_shader.err";

        // Build DXC command with temp file outputs
        const std::string command = fmt::format(
            "{} -E {} -T {}_6_0 -spirv -fspv-target-env=vulkan1.3 -D COMPILING {} -Fo {} 2>{}",
            dxcCompiler, entry, profile, hlslFile, tempSpv, tempError);

        // Execute the command
        const int exitCode = std::system(command.c_str());

        if (exitCode != 0)
        {
            // Read error output
            std::string errorText;
            if (std::filesystem::exists(tempError))
            {
                std::ifstream errorFile(tempError);
                if (errorFile.is_open())
                    errorText = std::string(std::istreambuf_iterator(errorFile), std::istreambuf_iterator<char>());
            }

            std::filesystem::remove(tempSpv);
            std::filesystem::remove(tempError);

            throw std::runtime_error(fmt::format("DXC compilation failed for file: {}\n{}", hlslFile, errorText));
        }

        // Read the compiled SPIR-V binary
        if (!std::filesystem::exists(tempSpv))
        {
            std::filesystem::remove(tempError);
            throw std::runtime_error(fmt::format("SPIR-V output file not created: {}", tempSpv));
        }

        std::ifstream spvFile(tempSpv, std::ios::binary);
        if (!spvFile.is_open())
        {
            std::filesystem::remove(tempSpv);
            std::filesystem::remove(tempError);
            throw std::runtime_error(fmt::format("Failed to open compiled SPIR-V file: {}", tempSpv));
        }

        // Get file size and read entire file
        spvFile.seekg(0, std::ios::end);
        const size_t fileSize = spvFile.tellg();
        spvFile.seekg(0, std::ios::beg);

        std::vector<std::byte> result(fileSize);
        spvFile.read(reinterpret_cast<char*>(result.data()), fileSize);
        spvFile.close();

        // Clean up temp files
        std::filesystem::remove(tempSpv);
        std::filesystem::remove(tempError);

        return result;
    }

    std::vector<std::string> ShaderAssetGenerator::GenerateAssets(const std::string& assetPath, std::vector<std::byte>&& contents)
    {
        std::string fileContent;
        fileContent.resize(contents.size());
        std::memcpy(fileContent.data(), contents.data(), contents.size());

        // Define shader type mappings
        struct ShaderType
        {
            std::string pragma;
            std::string profile;
        };
        struct EntryPoint
        {
            std::string name;
            std::string profile;
        };

        std::vector<EntryPoint> detectedEntries;

        const std::vector<ShaderType> shaderTypes = {
            { "vertex", "vs" },
            { "pixel", "ps" },
            { "compute", "cs" },
            { "geometry", "gs" }
        };

        // Search for each shader type
        for (const auto& [pragma, profile] : shaderTypes)
        {
            std::string pattern = R"(#pragma\s+)" + pragma + R"(\s+([a-zA-Z_][a-zA-Z0-9_]*))";
            std::regex pragmaRegex(pattern);

            std::sregex_iterator iter(fileContent.begin(), fileContent.end(), pragmaRegex);
            std::sregex_iterator end;

            for (; iter != end; ++iter)
            {
                std::string entryPoint = (*iter)[1].str();
                detectedEntries.emplace_back(entryPoint, profile);
            }
        }

        std::filesystem::path assetFilePath = assetPath;
        std::string baseFileName = assetPath.substr(0, assetPath.size() - assetFilePath.extension().string().size());

        std::vector<std::string> spirvBinaryFiles;

        // Process each detected entry point
        for (const auto& [entry, profile] : detectedEntries)
        {
            std::string spirvPath;

            // Generate output filename based on shader type
            if (profile == "cs")
                spirvPath = fmt::format("{}_{}_{}.hlsl.asset", baseFileName, entry, profile);
            else
                spirvPath = fmt::format("{}_{}.hlsl.asset", baseFileName, profile);

            AssetFile assetFile("SHAD", 0);

            assetFile.blob = compile_shader(PROJECT_ROOT"/include/dxc/bin/dxc", entry, profile, PROJECT_ROOT"/assets/" + assetPath);

            assetFile.header["entry"] = entry;
            assetFile.header["profile"] = profile;
            assetFile.header["stem"] = assetFilePath.stem().string();

            std::string fullSprivPath = GEN_ASSET_DIR + spirvPath;
            assetFile.Save(fullSprivPath.c_str());

            spirvBinaryFiles.push_back(spirvPath);
        }

        return spirvBinaryFiles;
    }
}
