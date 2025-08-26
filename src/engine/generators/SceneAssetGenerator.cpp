#include "SceneAssetGenerator.h"

#include <iostream>

#include "rendering/RenderingEngine.h"
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>

#include "assets-system/AssetManager.h"
#include "components/Tags.h"
#include "components/Transform.h"
#include "widgets/EngineWidget.h"
#include "ecs/EngineView.h"
#include "fmt/ranges.h"
#include "glm/gtx/matrix_decompose.hpp"
#include "rendering/Core.h"
#include "rendering/pass-system/Mesh.h"

void load_primitive_indices(const fastgltf::Asset& gltf, const fastgltf::Primitive& primitive, serial::array<uint32_t>& indices, const size_t vertexOffset)
{
    if (!primitive.indicesAccessor.has_value())
        return;

    const fastgltf::Accessor& indexAccessor = gltf.accessors[primitive.indicesAccessor.value()];
    indices.Reserve(indices.size() + indexAccessor.count);

    for (size_t i = 0; i < indexAccessor.count; ++i)
    {
        const uint32_t index = fastgltf::getAccessorElement<uint32_t>(gltf, indexAccessor, i);
        indices.push_back(index + static_cast<uint32_t>(vertexOffset));
    }
}

void load_primitive_vertices(const fastgltf::Asset& gltf, const fastgltf::Primitive& primitive, serial::array<rendering::Vertex>& vertices)
{
    const auto positionIt = primitive.findAttribute("POSITION");
    if (positionIt == primitive.attributes.end()) return;

    const fastgltf::Accessor& posAccessor = gltf.accessors[positionIt->second];
    const size_t vertexOffset = vertices.size();
    vertices.Resize(vertices.size() + posAccessor.count);

    // Load positions
    for (size_t i = 0; i < posAccessor.count; ++i)
    {
        const glm::vec3 position = fastgltf::getAccessorElement<glm::vec3>(gltf, posAccessor, i);

        rendering::Vertex& vertex = vertices.data()[vertexOffset + i];
        vertex.position = position;
        vertex.normal = { 0.0f, 0.0f, 1.0f };
        vertex.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        vertex.uv_x = 0.0f;
        vertex.uv_y = 0.0f;
    }

    // Load normals if present
    const auto normalIt = primitive.findAttribute("NORMAL");
    if (normalIt != primitive.attributes.end())
    {
        const fastgltf::Accessor& normalAccessor = gltf.accessors[normalIt->second];
        for (size_t i = 0; i < normalAccessor.count; ++i)
        {
            vertices.data()[vertexOffset + i].normal = fastgltf::getAccessorElement<glm::vec3>(gltf, normalAccessor, i);
        }
    }

    // Load UVs if present
    const auto uvIt = primitive.findAttribute("TEXCOORD_0");
    if (uvIt != primitive.attributes.end())
    {
        const fastgltf::Accessor& uvAccessor = gltf.accessors[uvIt->second];
        for (size_t i = 0; i < uvAccessor.count; ++i)
        {
            const glm::vec2 uv = fastgltf::getAccessorElement<glm::vec2>(gltf, uvAccessor, i);
            vertices.data()[vertexOffset + i].uv_x = uv.x;
            vertices.data()[vertexOffset + i].uv_y = uv.y;
        }
    }

    // Load colors if present
    const auto colorIt = primitive.findAttribute("COLOR_0");
    if (colorIt != primitive.attributes.end())
    {
        const fastgltf::Accessor& colorAccessor = gltf.accessors[colorIt->second];
        for (size_t i = 0; i < colorAccessor.count; ++i)
        {
            vertices.data()[vertexOffset + i].color = fastgltf::getAccessorElement<glm::vec4>(gltf, colorAccessor, i);
        }
    }
}

std::vector<std::string> SceneAssetGenerator::GenerateAssets(const std::string& assetPath, std::vector<std::byte>&& contents)
{
    Core core {};
    ecs::Engine& engine = core.engine;
    rendering::PassSystem& passManager = core.renderer.passManager;

    fastgltf::Parser parser {};

    constexpr auto gltfOptions =
            fastgltf::Options::DontRequireValidAssetMember |
            fastgltf::Options::AllowDouble |
            fastgltf::Options::LoadGLBBuffers |
            fastgltf::Options::LoadExternalBuffers;

    // fastgltf::Options::LoadExternalImages;

    size_t byteCount = contents.size();
    contents.resize(byteCount + fastgltf::getGltfBufferPadding());

    fastgltf::GltfDataBuffer data;
    data.fromByteView(reinterpret_cast<std::uint8_t*>(contents.data()), byteCount, contents.size());

    fastgltf::Asset gltf;

    std::filesystem::path path = assets_system::ASSET_DIR + assetPath;

    auto type = determineGltfFileType(&data);

    if (type == fastgltf::GltfType::glTF)
    {
        auto load = parser.loadGLTF(&data, path.parent_path(), gltfOptions);
        if (load)
            gltf = std::move(load.get());
        else
        {
            fmt::println(stderr, "Failed to load glTF: {}", to_underlying(load.error()));
            return {};
        }
    } else if (type == fastgltf::GltfType::GLB)
    {
        auto load = parser.loadBinaryGLTF(&data, path.parent_path(), gltfOptions);
        if (load)
            gltf = std::move(load.get());
        else
        {
            fmt::println(stderr, "Failed to load glTF: {}", to_underlying(load.error()));
            return {};
        }
    } else
    {
        fmt::println(stderr, "Failed to determine glTF container");
        return {};
    }

    // save reallocation
    serial::array<uint32_t> indices;
    serial::array<rendering::Vertex> vertices;


    std::vector<std::vector<ecs::EntityID>> meshTransforms;
    meshTransforms.resize(gltf.meshes.size());

    // load all nodes and their meshes
    for (fastgltf::Node& node : gltf.nodes)
    {
        if (!node.meshIndex.has_value())
            continue;

        Transform newNode;
        std::visit(fastgltf::visitor
                   {
                       [&](const fastgltf::Node::TransformMatrix& matrix)
                       {
                           glm::vec3 skew;
                           glm::vec4 perspective;
                           glm::mat4 transform;

                           std::memcpy(&transform, matrix.data(), sizeof(matrix));
                           glm::decompose(transform, newNode.SetScale(), newNode.SetRotation(), newNode.SetPosition(), skew, perspective);
                       },
                       [&](const fastgltf::Node::TRS& transform)
                       {
                           newNode.SetPosition() = glm::vec3(transform.translation[0], transform.translation[1], transform.translation[2]);
                           newNode.SetRotation() = glm::quat(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]);
                           newNode.SetScale() = glm::vec3(transform.scale[0], transform.scale[1], transform.scale[2]);
                       }
                   }, node.transform
        );

        ecs::Entity e = engine.New();
        engine.Add<Transform>(e, newNode);
        engine.Add<Model>(e, {});

        Tags tags {};
        tags.AddTag(assetPath.c_str());
        tags.AddTag(node.name.c_str());
        engine.Add<Tags>(e, tags);

        uint32_t meshIdx = static_cast<uint32_t>(*node.meshIndex);

        meshTransforms[meshIdx].emplace_back(e);
    }

    engine.Refresh();

    rendering::Material<DefaultPass, IdentifyPass> material { engine, passManager };

    for (size_t i = 0; i < gltf.meshes.size(); ++i)
    {
        indices.Resize(0);
        vertices.Resize(0);
        for (const auto& primitive : gltf.meshes[i].primitives)
        {
            load_primitive_indices(gltf, primitive, indices, vertices.size());
            load_primitive_vertices(gltf, primitive, vertices);
        }

        ecs::Entity meshIndex = engine.New();
        Mesh renderMesh {};
        renderMesh.SetMeshData(indices, vertices);
        engine.Add<Mesh>(meshIndex, renderMesh);
        for (ecs::Entity e : meshTransforms[i])
        {
            *engine.Get<Model>(e).meshRef = meshIndex;
            material.Apply(e, false);
        }
    }

    engine.Refresh();

    for (size_t i = 0; i < gltf.meshes.size(); ++i)
    {
        uint32_t cumCount = 0;
        for (const auto& primitive : gltf.meshes[i].primitives)
        {
            uint32_t count = static_cast<uint32_t>(gltf.accessors[primitive.indicesAccessor.value()].count);

            for (ecs::Entity e : meshTransforms[i])
                material.AddSubmesh(e, { cumCount, count });
            cumCount += count;
        }
    }

    const std::string sceneFileName = assetPath + ".asset";
    const std::string fullPath = assets_system::GEN_ASSET_DIR + sceneFileName;

    serial::Stream m;
    m.InitWrite();

    passManager.Serialize(m);
    engine.Serialize(m);

    assets_system::AssetFile sceneAsset("SCNE", 0);

    sceneAsset.WriteToBlob(m, true);

    sceneAsset.Save(fullPath.c_str());

    engine.Destroy();

    return { sceneFileName };
}
