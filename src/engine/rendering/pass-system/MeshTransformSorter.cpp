#include "MeshTransformSorter.h"

#include <algorithm>

namespace rendering
{
    void MeshTransformSorter::Refresh()
    {
        if (addQueue.empty() && removeQueue.empty())
            return;

        std::sort(addQueue.begin(), addQueue.end());
        std::sort(removeQueue.begin(), removeQueue.end());

        if (!removeQueue.empty())
        {
            const auto newEnd = std::set_difference(sorted.begin(), sorted.end(), removeQueue.begin(), removeQueue.end(), sorted.begin());
            sorted.erase(newEnd, sorted.end());
        }

        if (!addQueue.empty())
        {
            sorted.insert(sorted.end(), addQueue.begin(), addQueue.end());
            std::inplace_merge(sorted.begin(), sorted.end() - addQueue.size(), sorted.end());
        }

        sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());

        addQueue.clear();
        removeQueue.clear();
    }
}
