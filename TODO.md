## Compilation
- [ ] See if debug options are correctly applied, at least on linux

## Optimizations
- [x] CPU-side back face culling
- [x] Cull faces that are not touching air
- [ ] Single triangle per quad
- [ ] LOD system
- [ ] Multithreading
- [ ] column wise RLE
- [ ] color pallete per chunk instead of textures?
- [ ] reduce vertex/quad to 1 int -> only doable if I implement the thing below
- [ ] gldraw multi indirect, and remove chunkID from each vertex
- [ ] greedy meshing
- [ ] improve greedy meshing to do everything in one pass
- [ ] frustum culling
- [ ] occlusion culling
- [ ] IBO and glMultiDrawElementsIndirect
- [x] glMultiDrawArraysIndirect
- [ ] optimize VBOs and indirect buffer to only allocate data at the start
- [ ] indirect being 1D array, see buildInfo
- [ ] voxels should at least be in a bitmap, store materials somewhere else
- [ ] test not using static for all buffers

## Fix
- [ ] Breaking voxels when world size is not 16x16x16 or 32x32x32
