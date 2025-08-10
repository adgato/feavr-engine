#pragma once
#include <cassert>
#include <cstring>
#include <string>
#include <utility>

template <typename T>
static const char* RawTypeName()
{
#ifdef _MSC_VER
    return __FUNCSIG__;
#else
    return __PRETTY_FUNCTION__;
#endif
}

static std::pair<size_t, size_t> GetTypeNameBounds()
{
    const char* str = RawTypeName<int>();

    // Find "int" in the string
    for (size_t i = 0; str[i] != '\0'; i++)
        if (str[i] == 'i' && str[i + 1] == 'n' && str[i + 2] == 't')
        {
            size_t leading_junk = i;
            size_t trailing_junk = std::strlen(str) - i - 3;
            return { leading_junk, trailing_junk };
        }

    // Fallback - shouldn't happen with standard compilers
    assert(false && "Template typename location not found.");
    return { 0, 0 };
}

template <typename T>
const char* NameOf()
{
    static std::string name = []
    {
        const char* raw = RawTypeName<T>();
        auto [leading, trailing] = GetTypeNameBounds();
        return std::string(raw + leading, std::strlen(raw) - leading - trailing);
    }();
    return name.c_str();
}