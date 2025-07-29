#include "simple_json.h"

#include <vector>
#include "Stream.h"

namespace serial
{
    std::vector<std::string> split(std::string&& s)
    {
        size_t pos_start = 0;
        size_t pos_end;
        std::vector<std::string> res {};

        if (s.ends_with('\0'))
            s.erase(s.find_first_of('\0'));

        if (s.empty())
            return res;

        while ((pos_end = s.find(',', pos_start)) != std::string::npos)
        {
            std::string token = s.substr(pos_start, pos_end - pos_start);
            pos_start = pos_end + 1;
            res.push_back(token);
        }

        res.push_back(s.substr(pos_start));
        return res;
    }

    std::string join(std::vector<std::string>&& s)
    {
        std::string res {};
        for (std::string& str : s)
        {
            if (str.ends_with('\0'))
                str.erase(str.find_first_of('\0'));
            res += str + ',';
        }
        return res.size() > 0 ? res.substr(0, res.size() - 1) : std::string();
    }

    simple_json_value_t& simple_json_value_t::operator=(const std::string& value)
    {
        object->strings.insert_or_assign(key, value);
        return *this;
    }

    simple_json_value_t& simple_json_value_t::operator=(const uint64_t value)
    {
        object->ints.insert_or_assign(key, value);
        return *this;
    }

    simple_json_value_t& simple_json_value_t::operator=(const double value)
    {
        object->reals.insert_or_assign(key, value);
        return *this;
    }

    simple_json_value_t::operator std::string() const { return object->strings[key]; }
    simple_json_value_t::operator unsigned long int() const { return object->ints[key]; }
    simple_json_value_t::operator double() const { return object->reals[key]; }

    void simple_json::Serialize(Stream& m)
    {
        if (m.loading)
        {
            const std::vector<std::string> stringKeys = split(m.reader.ReadString());
            const std::vector<std::string> intKeys = split(m.reader.ReadString());
            const std::vector<std::string> realKeys = split(m.reader.ReadString());

            const std::vector<std::string> stringValues = split(m.reader.ReadString());
            std::vector<uint64_t> intValues(intKeys.size());
            std::vector<double> realValues(realKeys.size());

            m.reader.ReadArray<uint64_t>(intValues.data(), intValues.size());
            m.reader.ReadArray<double>(realValues.data(), realValues.size());

            strings.clear();
            ints.clear();
            reals.clear();

            for (size_t i = 0; i < stringKeys.size(); ++i)
                strings.insert_or_assign(stringKeys[i], stringValues[i]);

            for (size_t i = 0; i < intKeys.size(); ++i)
                ints.insert_or_assign(intKeys[i], intValues[i]);

            for (size_t i = 0; i < realKeys.size(); ++i)
                reals.insert_or_assign(realKeys[i], realValues[i]);

        } else
        {
            std::vector<std::string> stringKeys {};
            std::vector<std::string> intKeys {};
            std::vector<std::string> realKeys {};
            std::vector<std::string> stringValues {};
            std::vector<uint64_t> intValues {};
            std::vector<double> realValues {};

            stringKeys.reserve(strings.size());
            intKeys.reserve(ints.size());
            realKeys.reserve(reals.size());
            stringValues.reserve(strings.size());
            intValues.reserve(ints.size());
            realValues.reserve(reals.size());

            for (const auto& [key, value] : strings)
            {
                stringKeys.push_back(key);
                stringValues.push_back(value);
            }
            for (const auto& [key, value] : ints)
            {
                intKeys.push_back(key);
                intValues.push_back(value);
            }
            for (const auto& [key, value] : reals)
            {
                realKeys.push_back(key);
                realValues.push_back(value);
            }

            m.writer.WriteString(join(std::move(stringKeys)));
            m.writer.WriteString(join(std::move(intKeys)));
            m.writer.WriteString(join(std::move(realKeys)));

            m.writer.WriteString(join(std::move(stringValues)));
            m.writer.WriteArray<uint64_t>(intValues.data(), intValues.size());
            m.writer.WriteArray<double>(realValues.data(), realValues.size());
        }
    }
}
