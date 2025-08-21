# feavr-engine
a game engine written in c++ using the Vulkan API.
includes my own entity component system (ECS) designed for ease of use and saving / loading at runtime.
the game engine is built on top of this ECS - each scene/level is its own ECS, allowing them to be efficiently subsituted / combined at runtime for easy game state manipulation. 
this also applies to the rendering pipeline - the ECS makes defining and applying materials as a sequence of existing passes efficient and easy
the game engine also includes a scalable asset system for loading and saving data such as shaders, entity component systems, scenes, textures. 
assets files are automatically tracked using events to detect {modification, moving, creation, deletion} and only then is their cached asset file (the data loaded at runtime) regenerated.
the asset system uses spirv-reflect to automate copying the binding information about structures used in shaders into c++ structs.

i am making this game engine for my own use first and foremost. i've become a bit tired of relying on existing frameworks (unity, unreal) which either require servere workarounds or limitations when implementing rendering pipelines or using command buffers.

the name feavr comes from its two aspirational use cases - finite element analysis, and virtual reality - coming soon i hope...
