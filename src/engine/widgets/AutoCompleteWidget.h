#pragma once
#include <span>
#include <string>
#include <vector>
#include <memory>

namespace widgets
{
    class AutoCompleteWidget
    {
        std::vector<int> filteredIndices {};
        std::unique_ptr<char[]> buffer = std::make_unique<char[]>(256);
        int selectedIndex = 0;
        bool alreadyOpen = false;

        void ClearBuffer();

    public:
        AutoCompleteWidget() { ClearBuffer(); }
        bool AutoComplete(const char* label, int* index, const std::span<std::string>& items);
    };
}
