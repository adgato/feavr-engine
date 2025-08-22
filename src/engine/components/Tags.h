#pragma once
#include "serialisation/array.h"

struct Tags
{
    using String = serial::array<char>;
    SERIALIZABLE(0, serial::array<String>) tags;

    void Serialize(serial::Stream& m);

    void AddTag(const char* text);

    bool ContainsTag(const String& tag);

    void RemoveTag(size_t index);

    void Widget();

    void Destroy();
};
