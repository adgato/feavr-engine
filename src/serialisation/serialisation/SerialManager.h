#pragma once
#include <cassert>

#include "ReadByteStream.h"
#include "WriteByteStream.h"
#include "SerialConcepts.h"
#include "array.h"

namespace serial
{
    class SerialManager
    {
        bool loading = false;

    public:
        WriteByteStream writer;
        ReadByteStream reader;

        void InitRead()
        {
            loading = true;
            Destroy();
        }

        void InitWrite()
        {
            loading = false;
            Destroy();
            writer.Init();
        }

        void Destroy()
        {
            reader.Destroy();
            writer.Destroy();
        }

        bool SkipToTag(const char tag)
        {
            if (!loading)
                return true;

            char readTag = reader.Read<char>();
            while (readTag < tag)
            {
                reader.Skip(reader.Read<uint_s>());
                readTag = reader.Read<char>();
            }
            if (readTag > tag)
                reader.Backup(sizeof(char));

            return readTag == tag;
        }

        template <typename T> requires std::is_arithmetic_v<T>
        void SerializeVar(T* pValue, const char tag)
        {
            if (loading)
            {
                if (!SkipToTag(tag))
                    return;
                const size_t size = reader.Read<uint_s>();
                if (size == sizeof(T))
                    *pValue = reader.Read<T>();
                else
                    reader.Skip(size);
            } else
            {
                writer.Write<char>(tag);
                writer.Write<uint_s>(sizeof(T));
                writer.Write<T>(*pValue);
            }
        }

        template <IsSerialType T>
        void SerializeCmp(T* pValue, const char tag)
        {
            size_t offset = 0;
            if (loading)
            {
                if (!SkipToTag(tag))
                    return;
                reader.Skip(sizeof(uint_s));
            } else
            {
                writer.Write<char>(tag);
                offset = writer.Reserve<uint_s>();
            }
            pValue->Serialize();
            if (!loading)
                writer.WriteOver<uint_s>(writer.GetCount() - offset - sizeof(uint_s), offset);
        }

        template <Serializable T>
        void SerializeArr(array<T>* pValue, const char tag)
        {
            size_t offset = 0;
            size_t count = 0;
            if (loading)
            {
                if (!SkipToTag(tag))
                    return;
                const size_t size = reader.Read<uint_s>();
                count = reader.Read<uint_s>();
                if constexpr (std::is_arithmetic_v<T>)
                    if (size != count * sizeof(T) + sizeof(uint_s))
                    {
                        reader.Skip(size - sizeof(uint_s));
                        return;
                    }
                pValue->Init(count);
            } else
            {
                count = pValue->size();
                writer.Write<char>(tag);
                offset = writer.Reserve<uint_s>();
                writer.Write<uint_s>(count);
            }

            T* data = pValue->data();
            if constexpr (std::is_arithmetic_v<T>)
            {
                if (loading)
                    reader.ReadArray<T>(data, count);
                else
                    writer.WriteArray<T>(data, count);
            }
            else
            {
                for (size_t i = 0; i < count; ++i)
                    (data + i)->Serialize();
            }

            if (!loading)
                writer.WriteOver<uint_s>(writer.GetCount() - offset - sizeof(uint_s), offset);
        }
    };
}
