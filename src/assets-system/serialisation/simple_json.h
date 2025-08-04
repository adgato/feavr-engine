#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>


namespace serial
{
    class simple_json;
    class Stream;

    class simple_json_value_t
    {
        simple_json* object;
        std::string key;

    public:
        simple_json_value_t() = delete;
        simple_json_value_t(simple_json* object, std::string_view&& key) : object(object)
        {
            this->key = std::move(key);
        }

        simple_json_value_t& operator=(const std::string& value);
        simple_json_value_t& operator=(uint64_t value);
        simple_json_value_t& operator=(double value);

        explicit operator std::string() const;
        explicit operator uint64_t() const;
        explicit operator double() const;
    };

    class simple_json
    {
        friend simple_json_value_t;
        std::unordered_map<std::string, std::string> strings {};
        std::unordered_map<std::string, uint64_t> ints {};
        std::unordered_map<std::string, double> reals {};

    public:
        simple_json_value_t operator[](std::string_view&& key)
        {
            return simple_json_value_t(this, std::move(key));
        }

        const std::string* GetIfString(const std::string& key) const
        {
            const auto it = strings.find(key);
            return it == strings.end() ? nullptr : &it->second;
        }

        const uint64_t* GetIfInt(const std::string& key) const
        {
            const auto it = ints.find(key);
            return it == ints.end() ? nullptr : &it->second;
        }

        const double* GetIfReal(const std::string& key) const
        {
            const auto it = reals.find(key);
            return it == reals.end() ? nullptr : &it->second;
        }

        void Serialize(Stream& m);
    };
}
