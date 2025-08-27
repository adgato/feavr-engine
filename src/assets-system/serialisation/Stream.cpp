#include "Stream.h"

namespace serial
{
    bool Stream::MatchNextTag(const TagID tag)
    {
        if (!reading)
        {
            writer.Write<TagID>(tag);
            return true;
        }

        TagID readTag = reader.Read<TagID>();
        while (readTag < tag)
        {
            reader.Jump(reader.Read<uint>());
            readTag = reader.Read<TagID>();
        }
        if (readTag > tag)
            reader.Jump(-sizeof(TagID));

        return readTag == tag;
    }

    void Stream::Barrier(const TagID tag, size_t& size, size_t& offset)
    {
        if (offset > 0)
        {
            if (reading)
            {
                if (reader.GetCount() - offset != size)
                {
                    fmt::println("Warning: unexpected size serialized.");
                    reader.Jump(offset - reader.GetCount() + size);
                }
            } else
                writer.WriteOver<fsize>(writer.GetCount() - offset - sizeof(fsize), offset);
        }

        MatchNextTag(tag);

        if (reading)
        {
            size = reader.Read<fsize>();
            offset = reader.GetCount();
        } else
        {
            size = 0;
            offset = writer.Reserve<fsize>();
        }
    }
}
