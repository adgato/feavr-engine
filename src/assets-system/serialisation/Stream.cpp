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
}
