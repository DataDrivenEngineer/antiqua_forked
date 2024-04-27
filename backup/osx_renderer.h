//
#ifndef OSX_RENDERER_H
#define OSX_RENDERER_H

struct RenderGroup;
#define RENDER_ON_GPU(name) void name(void *thread, struct RenderGroup *renderGroup)
typedef RENDER_ON_GPU(RenderOnGpu);

MONExternC void initRenderer(void *metalLayer);

//MONExternC RENDER_ON_GPU(renderOnGpu);

#endif
