#pragma once
#include "Pass.h"

namespace rendering::passes
{
    class PassInstance
    {
    public:
        std::shared_ptr<Pass> pass;
        std::vector<uint32_t> meshIdxs;
        
        PassInstance() = default;
        virtual ~PassInstance() = default;
        PassInstance(const PassInstance& other) = default;
        PassInstance& operator =(const PassInstance& other) = default;
        PassInstance(PassInstance&& other) = default;
        PassInstance& operator =(PassInstance&& other) = default;

        void Init(const std::shared_ptr<Pass>& instanceOf)
        {
            pass = instanceOf;
        }
        virtual void ConfigureInstance(VkCommandBuffer cmd) = 0;
        virtual void Destroy() {}
    };

    template <typename T> requires std::derived_from<T, Pass>
    class PassInst : public PassInstance
    {
    public:
        PassInst() = delete;
    };
}
