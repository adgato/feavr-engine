#include <iostream>
#include "OLD_loader.h"

#include "rendering/VulkanEngine.h"
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>

void loadPrimitiveIndices(const fastgltf::Asset& gltf, const fastgltf::Primitive& primitive, std::vector<uint32_t>& indices, const size_t vertexOffset)
{
    if (!primitive.indicesAccessor.has_value())
        return;

    const fastgltf::Accessor& indexAccessor = gltf.accessors[primitive.indicesAccessor.value()];
    indices.reserve(indices.size() + indexAccessor.count);

    for (size_t i = 0; i < indexAccessor.count; ++i)
    {
        const uint32_t index = fastgltf::getAccessorElement<uint32_t>(gltf, indexAccessor, i);
        indices.push_back(index + static_cast<uint32_t>(vertexOffset));
    }
}

void loadPrimitiveVertices(const fastgltf::Asset& gltf, const fastgltf::Primitive& primitive, std::vector<rendering::Vertex>& vertices)
{
    const auto positionIt = primitive.findAttribute("POSITION");
    if (positionIt == primitive.attributes.end()) return;

    const fastgltf::Accessor& posAccessor = gltf.accessors[positionIt->second];
    const size_t vertexOffset = vertices.size();
    vertices.resize(vertices.size() + posAccessor.count);

    // Load positions
    for (size_t i = 0; i < posAccessor.count; ++i)
    {
        const glm::vec3 position = fastgltf::getAccessorElement<glm::vec3>(gltf, posAccessor, i);

        rendering::Vertex& vertex = vertices[vertexOffset + i];
        vertex.position = position;
        vertex.normal = {0.0f, 0.0f, 1.0f};
        vertex.color = {1.0f, 1.0f, 1.0f, 1.0f};
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
            vertices[vertexOffset + i].normal = fastgltf::getAccessorElement<glm::vec3>(gltf, normalAccessor, i);
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
            vertices[vertexOffset + i].uv_x = uv.x;
            vertices[vertexOffset + i].uv_y = uv.y;
        }
    }

    // Load colors if present
    const auto colorIt = primitive.findAttribute("COLOR_0");
    if (colorIt != primitive.attributes.end())
    {
        const fastgltf::Accessor& colorAccessor = gltf.accessors[colorIt->second];
        for (size_t i = 0; i < colorAccessor.count; ++i)
        {
            vertices[vertexOffset + i].color = fastgltf::getAccessorElement<glm::vec4>(gltf, colorAccessor, i);
        }
    }
}

void loadGltf(Core* core, std::string_view filePath)
{
    fmt::print("Loading GLTF: {}", filePath);

    // rendering::PassManager& passManager = core->engine.passManager;
    rendering::PassMeshManager& passMeshManager = core->engine.sys.Get<rendering::PassMeshManager>();
    rendering::Material<default_pass::Pass>& defaultMaterial = core->engine.defaultMaterial;

    fastgltf::Parser parser{};

    constexpr auto gltfOptions =
        fastgltf::Options::DontRequireValidAssetMember |
        fastgltf::Options::AllowDouble |
        fastgltf::Options::LoadGLBBuffers |
        fastgltf::Options::LoadExternalBuffers;

    // fastgltf::Options::LoadExternalImages;

    fastgltf::GltfDataBuffer data;
    data.loadFromFile(filePath);

    fastgltf::Asset gltf;

    std::filesystem::path path = filePath;

    auto type = determineGltfFileType(&data);

    if (type == fastgltf::GltfType::glTF)
    {
        auto load = parser.loadGLTF(&data, path.parent_path(), gltfOptions);
        if (load)
            gltf = std::move(load.get());
        else
        {
            std::cerr << "Failed to load glTF: " << to_underlying(load.error()) << '\n';
            return;
        }
    }
    else if (type == fastgltf::GltfType::GLB)
    {
        auto load = parser.loadBinaryGLTF(&data, path.parent_path(), gltfOptions);
        if (load)
            gltf = std::move(load.get());
        else
        {
            std::cerr << "Failed to load glTF: " << to_underlying(load.error()) << '\n';
            return;
        }
    }
    else
    {
        std::cerr << "Failed to determine glTF container" << '\n';
        return;
    }

    // save reallocation
    std::vector<uint32_t> indices;
    std::vector<rendering::Vertex> vertices;

    for (fastgltf::Mesh& mesh : gltf.meshes)
    {
        indices.clear();
        vertices.clear();
        for (const auto& primitive : mesh.primitives)
        {
            loadPrimitiveIndices(gltf, primitive, indices, vertices.size());
            loadPrimitiveVertices(gltf, primitive, vertices);
        }

        const uint32_t meshIndex = passMeshManager.AddMesh(core->swapchain.UploadMesh(indices, vertices), rendering::MeshAssetSource{});

        uint32_t cumCount = 0;
        for (auto&& p : mesh.primitives)
        {
            uint32_t count = static_cast<uint32_t>(gltf.accessors[p.indicesAccessor.value()].count);

            defaultMaterial.AddSubMesh(meshIndex, cumCount, count);

            cumCount += count;
        }
    }
    core->engine.sys.Get<ecs::PassEntityManager>().RefreshComponents();

    // load all nodes and their meshes
    for (fastgltf::Node& node : gltf.nodes)
    {
        if (!node.meshIndex.has_value())
            continue;

        rendering::Transform newNode;
        std::visit(fastgltf::visitor
                   {
                       [&](const fastgltf::Node::TransformMatrix& matrix) { std::memcpy(&newNode.transform, matrix.data(), sizeof(matrix)); },
                       [&](const fastgltf::Node::TRS& transform)
                       {
                           const glm::vec3 tl(transform.translation[0], transform.translation[1], transform.translation[2]);
                           const glm::quat rot(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]);
                           const glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);
                           newNode.transform = translate(glm::mat4(1.f), tl) * toMat4(rot) * scale(glm::mat4(1.f), sc);
                       }
                   }, node.transform
        );

        ecs::Entity e = core->engine.sys.Get<ecs::MainEntityManager>().NewEntity<rendering::Transform>(newNode);

        uint32_t meshIdx = static_cast<uint32_t>(*node.meshIndex);

        for (const auto& [submeshId, passMesh, data] : core->engine.sys.Get<ecs::PassEntityManager>().View<rendering::SubMesh, default_pass::Component>())
        {
            if (passMesh.meshIndex == meshIdx)
                data.entities->push_back(e);
        }
    }
    core->engine.sys.Get<ecs::MainEntityManager>().RefreshComponents();
}
