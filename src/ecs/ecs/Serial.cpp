#include "Serial.h"

namespace ecs
{
    std::vector<size_t> split(const std::string& s, const std::string& delimiter)
    {
        size_t pos_start = 0;
        size_t pos_end;
        const size_t delim_len = delimiter.length();
        std::vector<size_t> res {};

        while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
        {
            std::string token = s.substr(pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            res.push_back(std::hash<std::string> {}(token));
        }

        res.push_back(std::hash<std::string> {}(s.substr(pos_start)));
        return res;
    }

    // tag at end of each component. used to jump to when the number of fields in a component has changed since their data was saved.
    char MAX_TAG = 0xFF;

    void Serial::Save(const char* filePath, EntityManager& e)
    {
        serial::SerialManager m;
        m.InitWrite();

        e.RefreshComponents();

        m.writer.WriteString(ComponentTypeNames);

        const EntityID entityCount = e.entityLocations.size();
        m.writer.Write<EntityID>(entityCount);

        for (EntityID entity = 0; entity < entityCount; ++entity)
        {
            const auto& [archetype, index] = e.entityLocations[entity];

            ArchetypeData& data = e.archetypeData[archetype];
            signature entityComponents = e.archetypeSignatures[archetype];

            uint componentCount = 0;
            for (uint j = 0; j < NumTypes; ++j)
                if (entityComponents[j])
                    componentCount++;

            m.writer.Write<uint>(componentCount);

            for (TypeID type = 0; type < NumTypes; ++type)
            {
                if (!entityComponents[type])
                    continue;

                m.writer.Write<TypeID>(type);
                const size_t offset = m.writer.Reserve<serial::uint_s>();

                ComponentInfo.Serialize(m, data.GetElem(index, type), type);
                m.writer.Write<char>(MAX_TAG);

                m.writer.WriteOver<serial::uint_s>(m.writer.GetCount() - offset - sizeof(serial::uint_s), offset);
            }
        }

        m.writer.SaveToFile(filePath);
        m.Destroy();
    }

    void Serial::Load(const char* filePath, EntityManager& e)
    {
        serial::SerialManager m;
        m.InitRead();
        m.reader.LoadFromFile(filePath);

        e.Destroy();
        e = {};

        const std::string targetTypeNames = m.reader.ReadString();

        const auto targetSplit = split(targetTypeNames, ", ");
        const auto sourceSplit = split(ComponentTypeNames, ", ");

        const uint targetNumTypes = targetSplit.size();
        assert(sourceSplit.size() == NumTypes);

        std::vector remap(targetNumTypes, NumTypes);
        for (size_t i = 0; i < targetSplit.size(); ++i)
            for (size_t j = 0; j < NumTypes; ++j)
                if (targetSplit[i] == sourceSplit[j])
                {
                    remap[i] = j;
                    break;
                }

        std::vector<std::unique_ptr<std::byte[]>> components;
        components.reserve(NumTypes);
        for (size_t i = 0; i < NumTypes; ++i)
            components.emplace_back(std::make_unique<std::byte[]>(ComponentInfo.GetTypeInfo(i).size));

        const EntityID entityCount = m.reader.Read<EntityID>();
        for (EntityID entity = 0; entity < entityCount; ++entity)
        {
            Entity i = e.NewEntity();
            assert(i == entity);

            for (uint componentCount = m.reader.Read<uint>(); componentCount > 0; --componentCount)
            {
                const TypeID retype = m.reader.Read<TypeID>();
                const size_t size = m.reader.Read<serial::uint_s>();
                const TypeID type = remap[retype];
                if (type >= NumTypes)
                {
                    m.reader.Skip(size);
                    continue;
                }

                std::byte* data = components[type].get();
                ComponentInfo.CopyDefault(data, type);
                ComponentInfo.Serialize(m, data, type);
                m.SkipToTag(MAX_TAG);

                e.AddComponent(entity, data, type);
            }
        }
        e.RefreshComponents();

        m.Destroy();
    }
}
