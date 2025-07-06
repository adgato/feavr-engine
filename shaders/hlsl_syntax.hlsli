typedef unsigned long uint64_t;

#define SV_POSITION SV_POSITION
#define NORMAL NORMAL
#define COLOR COLOR
#define TEXCOORD0 TEXCOORD0

#define SV_VertexID SV_VertexID

#define SV_TARGET SV_TARGET

namespace vk
{
    template <typename T>
    T RawBufferLoad(in uint64_t deviceAddress, in uint alignment = 4);
    template <typename T>
    void RawBufferStore(in uint64_t deviceAddress, in T value, in uint alignment = 4);
}