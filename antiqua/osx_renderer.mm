#include "types.h"
#include "osx_renderer.h"
#include "antiqua_render_group.h"

#import <Metal/MTLDevice.h>
#import <Metal/MTLCommandQueue.h>
#import <Metal/MTLCommandBuffer.h>
#import <QuartzCore/CAMetalLayer.h>

static id<MTLDevice> metalDevice;
static id<MTLCommandQueue> commandQueue;
static CAMetalLayer *layer;
static id<MTLTexture> depthTex = 0;
static id<MTLRenderPipelineState> renderPipelineStates[5];
static id<MTLDepthStencilState> depthStencilState;
static id<MTLBuffer> renderGroupVertexBuffer;
static u32 renderGroupVertexBufferCurrentSize;
static id<MTLBuffer> renderGroupIndexBuffer;
static u32 renderGroupIndexBufferCurrentSize;

MONExternC INIT_RENDERER(initRenderer)
{
    metalDevice = ((CAMetalLayer *)metalLayer).device;
    commandQueue = [metalDevice newCommandQueue];
    layer = (CAMetalLayer *)metalLayer;

    // NOTE(dima): creating render pipeline states
    NSURL *libraryURL = [[NSBundle mainBundle] URLForResource:@"shaders"
                                               withExtension:@"metallib"];
    ASSERT(libraryURL != 0);

    id<MTLLibrary> lib = [metalDevice newLibraryWithURL:libraryURL
                          error:0];
    ASSERT(lib != 0);

    {
        id<MTLFunction> vertexFn = [lib newFunctionWithName:@"vertexShader"];
        id<MTLFunction> fragmentFn = [lib newFunctionWithName:@"fragmentShader"];

        MTLRenderPipelineDescriptor *renderPipelineDesc =
            [[MTLRenderPipelineDescriptor alloc] init];
        renderPipelineDesc.vertexFunction = vertexFn;
        renderPipelineDesc.fragmentFunction = fragmentFn;
        renderPipelineDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        renderPipelineDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

        id<MTLRenderPipelineState> renderPipelineState =
            [metalDevice newRenderPipelineStateWithDescriptor:renderPipelineDesc
                         error:0];
        ASSERT(renderPipelineState != 0);

        [lib release];
        [vertexFn release];
        [fragmentFn release];
        [renderPipelineDesc release];

        renderPipelineStates[0] = renderPipelineState;
    }

    {
        id<MTLFunction> vertexFn = [lib newFunctionWithName:@"vertexShaderTile"];
        id<MTLFunction> fragmentFn = [lib newFunctionWithName:@"fragmentShaderTile"];

        MTLRenderPipelineDescriptor *renderPipelineDesc =
            [[MTLRenderPipelineDescriptor alloc] init];
        renderPipelineDesc.vertexFunction = vertexFn;
        renderPipelineDesc.fragmentFunction = fragmentFn;
        renderPipelineDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        renderPipelineDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

        id<MTLRenderPipelineState> renderPipelineState =
            [metalDevice newRenderPipelineStateWithDescriptor:renderPipelineDesc
                         error:0];
        ASSERT(renderPipelineState != 0);

        [lib release];
        [vertexFn release];
        [fragmentFn release];
        [renderPipelineDesc release];

        renderPipelineStates[1] = renderPipelineState;
    }

    {
        id<MTLFunction> vertexFn = [lib newFunctionWithName:@"vertexShaderMesh"];
        id<MTLFunction> fragmentFn = [lib newFunctionWithName:@"fragmentShaderMesh"];

        MTLRenderPipelineDescriptor *renderPipelineDesc =
            [[MTLRenderPipelineDescriptor alloc] init];
        renderPipelineDesc.vertexFunction = vertexFn;
        renderPipelineDesc.fragmentFunction = fragmentFn;
        renderPipelineDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        renderPipelineDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

        id<MTLRenderPipelineState> renderPipelineState =
            [metalDevice newRenderPipelineStateWithDescriptor:renderPipelineDesc
                         error:0];
        ASSERT(renderPipelineState != 0);

        [lib release];
        [vertexFn release];
        [fragmentFn release];
        [renderPipelineDesc release];

        renderPipelineStates[2] = renderPipelineState;
    }

    // NOTE(dima): creating depth stencil state
    {
        MTLDepthStencilDescriptor *depthStencilDesc = [[MTLDepthStencilDescriptor alloc] init];
        depthStencilDesc.depthWriteEnabled = true;
        depthStencilDesc.depthCompareFunction = MTLCompareFunctionLess;
        depthStencilState =
            [metalDevice newDepthStencilStateWithDescriptor:depthStencilDesc];
        ASSERT(depthStencilState != 0);

        [depthStencilDesc release];
    }

    // NOTE(dima): creating GPU buffers for render group
    {
        renderGroupVertexBuffer = [metalDevice newBufferWithLength:KB(256)
                                    options:MTLResourceStorageModeShared];
        ASSERT(renderGroupVertexBuffer != 0);

        renderGroupIndexBuffer = [metalDevice newBufferWithLength:KB(128)
                                    options:MTLResourceStorageModeShared];
        ASSERT(renderGroupIndexBuffer != 0);
    }
}

MONExternC RENDER_ON_GPU(renderOnGpu)
{
    @autoreleasepool
    {
        /* TODO(dima): warnings to address:
           - do not have a standalone clear entry
           - do not store depth / color texture after the last render entry */

        renderGroupVertexBufferCurrentSize = 0;
        renderGroupIndexBufferCurrentSize = 0;

        id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
        id<CAMetalDrawable> drawable = [layer nextDrawable];

        RenderGroupEntryHeader *EntryHeader = (RenderGroupEntryHeader *)renderGroup->pushBufferBase;
        while (EntryHeader)
        {
            switch (EntryHeader->type)
            {
                case RenderGroupEntryType_RenderEntryClear:
                {
                    RenderEntryClear *Entry = (RenderEntryClear *)(EntryHeader + 1);
                    MTLRenderPassDescriptor *renderPassDesc =
                        [MTLRenderPassDescriptor renderPassDescriptor];
                    renderPassDesc.colorAttachments[0].texture = drawable.texture;
                    renderPassDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
                    renderPassDesc.colorAttachments[0].clearColor =
                        MTLClearColorMake(Entry->color.x,
                                          Entry->color.y,
                                          Entry->color.z,
                                          Entry->color.w);

                    id<MTLRenderCommandEncoder> renderCommandEnc =
                        [commandBuffer renderCommandEncoderWithDescriptor:renderPassDesc];

                    [renderCommandEnc endEncoding];

                    break;
                }
                case RenderGroupEntryType_RenderEntryPoint:
                {
                    RenderEntryPoint *Entry = (RenderEntryPoint *)(EntryHeader + 1);
                    MTLRenderPassDescriptor *renderPassDesc =
                        [MTLRenderPassDescriptor renderPassDescriptor];
                    renderPassDesc.colorAttachments[0].texture = drawable.texture;
                    renderPassDesc.colorAttachments[0].loadAction = MTLLoadActionLoad;

                    if (!depthTex ||
                        layer.drawableSize.width != depthTex.width ||
                        layer.drawableSize.height != depthTex.height)
                    {
                        if (depthTex)
                        {
                            [depthTex release];
                            renderPassDesc.depthAttachment.loadAction = MTLLoadActionLoad;
                        }
                        else
                        {
                            renderPassDesc.depthAttachment.loadAction = MTLLoadActionClear;
                        }
                        MTLTextureDescriptor *depthTexDesc =
                            [MTLTextureDescriptor
                             texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                             width:layer.drawableSize.width
                             height:layer.drawableSize.height
                             mipmapped:NO];
                        depthTexDesc.usage = MTLTextureUsageRenderTarget
                                             | MTLTextureUsageShaderRead;
                        depthTex = [metalDevice newTextureWithDescriptor:depthTexDesc];
                    }
                    renderPassDesc.depthAttachment.texture = depthTex;
                    renderPassDesc.depthAttachment.storeAction = MTLStoreActionStore;
                    renderPassDesc.depthAttachment.clearDepth = 1.0f;

                    r32 vertices[3 * 2];
                    V4 pointPosCamera = v4(Entry->position.x,
                                           Entry->position.y,
                                           Entry->position.z,
                                           1.0f);
                    memcpy(vertices, (void*)&Entry->position, sizeof(V3));
                    memcpy(vertices + 3, (void*)&Entry->color, sizeof(Entry->color));

                    u8 *renderGroupVertexBufferDestStart = ((u8 *)[renderGroupVertexBuffer contents])
                                                     + renderGroupVertexBufferCurrentSize;
                    u8 sizeOfData = sizeof(r32) * ARRAY_COUNT(vertices); 
                    memcpy(renderGroupVertexBufferDestStart,
                           vertices,
                           sizeOfData);
                    renderGroupVertexBufferCurrentSize += sizeOfData;
                    u32 renderGroupVertexBufferOffset = renderGroupVertexBufferDestStart
                                               - ((u8 *)[renderGroupVertexBuffer contents]);

                    id<MTLRenderCommandEncoder> renderCommandEnc =
                        [commandBuffer renderCommandEncoderWithDescriptor:renderPassDesc];
                    [renderCommandEnc setRenderPipelineState:renderPipelineStates[0]];
                    [renderCommandEnc setDepthStencilState:depthStencilState];
                    [renderCommandEnc setVertexBuffer: renderGroupVertexBuffer 
                                      offset: renderGroupVertexBufferOffset 
                                      attributeStride: 0
                                      atIndex: 5];
                    [renderCommandEnc setVertexBytes: &renderGroup->uniforms
                                      length: sizeof(renderGroup->uniforms)
                                      attributeStride: 0
                                      atIndex: 7];
                    [renderCommandEnc drawPrimitives:MTLPrimitiveTypePoint
                                      vertexStart: 0
                                      vertexCount: 1];
                    [renderCommandEnc endEncoding];

                    break;
                }
                case RenderGroupEntryType_RenderEntryLine:
                {
                    RenderEntryLine *Entry = (RenderEntryLine *)(EntryHeader + 1);

                    MTLRenderPassDescriptor *renderPassDesc =
                        [MTLRenderPassDescriptor renderPassDescriptor];
                    renderPassDesc.colorAttachments[0].texture = drawable.texture;
                    renderPassDesc.colorAttachments[0].loadAction = MTLLoadActionLoad;

                    if (!depthTex ||
                        layer.drawableSize.width != depthTex.width ||
                        layer.drawableSize.height != depthTex.height)
                    {
                        if (depthTex)
                        {
                            [depthTex release];
                            renderPassDesc.depthAttachment.loadAction = MTLLoadActionLoad;
                        }
                        else
                        {
                            renderPassDesc.depthAttachment.loadAction = MTLLoadActionClear;
                        }
                        MTLTextureDescriptor *depthTexDesc =
                            [MTLTextureDescriptor
                             texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                             width:layer.drawableSize.width
                             height:layer.drawableSize.height
                             mipmapped:NO];
                        depthTexDesc.usage = MTLTextureUsageRenderTarget
                                             | MTLTextureUsageShaderRead;
                        depthTex = [metalDevice newTextureWithDescriptor:depthTexDesc];
                    }
                    renderPassDesc.depthAttachment.texture = depthTex;
                    renderPassDesc.depthAttachment.storeAction = MTLStoreActionStore;
                    renderPassDesc.depthAttachment.clearDepth = 1.0f;

                    r32 vertices[2 * 3 * 2];
                    memcpy(vertices, (void*)&Entry->start, sizeof(Entry->start));
                    memcpy(vertices + 3, (void*)&Entry->color, sizeof(Entry->color));
                    memcpy(vertices + 6, (void*)&Entry->end, sizeof(Entry->end));
                    memcpy(vertices + 9, (void*)&Entry->color, sizeof(Entry->color));

                    u8 *renderGroupVertexBufferDestStart = ((u8 *)[renderGroupVertexBuffer contents])
                                                     + renderGroupVertexBufferCurrentSize;
                    u8 sizeOfData = sizeof(r32) * ARRAY_COUNT(vertices); 
                    memcpy(renderGroupVertexBufferDestStart,
                           vertices,
                           sizeOfData);
                    renderGroupVertexBufferCurrentSize += sizeOfData;
                    u32 renderGroupVertexBufferOffset = renderGroupVertexBufferDestStart
                                               - ((u8 *)[renderGroupVertexBuffer contents]);

                    id<MTLRenderCommandEncoder> renderCommandEnc =
                        [commandBuffer renderCommandEncoderWithDescriptor:renderPassDesc];
                    [renderCommandEnc setRenderPipelineState:renderPipelineStates[0]];
                    [renderCommandEnc setDepthStencilState:depthStencilState];
                    [renderCommandEnc setVertexBuffer: renderGroupVertexBuffer 
                                      offset: renderGroupVertexBufferOffset 
                                      attributeStride: 0
                                      atIndex: 5];
                    [renderCommandEnc setVertexBytes: &renderGroup->uniforms
                                      length: sizeof(renderGroup->uniforms)
                                      attributeStride: 0
                                      atIndex: 7];
                    [renderCommandEnc drawPrimitives:MTLPrimitiveTypeLine
                                      vertexStart: 0
                                      vertexCount: 2];
                    [renderCommandEnc endEncoding];
                    break;
                }
                case RenderGroupEntryType_RenderEntryMesh:
                {
                    RenderEntryMesh *Entry = (RenderEntryMesh *)(EntryHeader + 1);

                    MTLRenderPassDescriptor *renderPassDesc =
                        [MTLRenderPassDescriptor renderPassDescriptor];
                    renderPassDesc.colorAttachments[0].texture = drawable.texture;
                    renderPassDesc.colorAttachments[0].loadAction = MTLLoadActionLoad;

                    if (!depthTex ||
                        layer.drawableSize.width != depthTex.width ||
                        layer.drawableSize.height != depthTex.height)
                    {
                        if (depthTex)
                        {
                            [depthTex release];
                            renderPassDesc.depthAttachment.loadAction = MTLLoadActionLoad;
                        }
                        else
                        {
                            renderPassDesc.depthAttachment.loadAction = MTLLoadActionClear;
                        }
                        MTLTextureDescriptor *depthTexDesc =
                            [MTLTextureDescriptor
                             texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                             width:layer.drawableSize.width
                             height:layer.drawableSize.height
                             mipmapped:NO];
                        depthTexDesc.usage = MTLTextureUsageRenderTarget
                                             | MTLTextureUsageShaderRead;
                        depthTex = [metalDevice newTextureWithDescriptor:depthTexDesc];
                    }
                    renderPassDesc.depthAttachment.texture = depthTex;
                    renderPassDesc.depthAttachment.storeAction = MTLStoreActionStore;
                    renderPassDesc.depthAttachment.clearDepth = 1.0f;

                    u8 *renderGroupVertexBufferDestStart = ((u8 *)[renderGroupVertexBuffer contents]) + renderGroupVertexBufferCurrentSize;
                    memcpy(renderGroupVertexBufferDestStart,
                           Entry->vertices,
                           Entry->sizeOfVertices);
                    renderGroupVertexBufferCurrentSize += Entry->sizeOfVertices;
                    u32 renderGroupVertexBufferOffset = renderGroupVertexBufferDestStart
                                               - ((u8 *)[renderGroupVertexBuffer contents]);

                    u8 *renderGroupIndexBufferDestStart = ((u8 *)[renderGroupIndexBuffer contents]) + renderGroupIndexBufferCurrentSize;
                    memcpy(renderGroupIndexBufferDestStart,
                           Entry->indices,
                           Entry->sizeOfIndices);
                    renderGroupIndexBufferCurrentSize += Entry->sizeOfIndices;
                    u32 renderGroupIndexBufferOffset = renderGroupIndexBufferDestStart
                                               - ((u8 *)[renderGroupIndexBuffer contents]);

                    id<MTLRenderCommandEncoder> renderCommandEnc =
                        [commandBuffer renderCommandEncoderWithDescriptor:renderPassDesc];
                    [renderCommandEnc setRenderPipelineState:renderPipelineStates[2]];
                    [renderCommandEnc setDepthStencilState:depthStencilState];
                    [renderCommandEnc setVertexBuffer: renderGroupVertexBuffer 
                                      offset: renderGroupVertexBufferOffset
                                      attributeStride: 0
                                      atIndex: 5];
                    [renderCommandEnc setVertexBytes: &renderGroup->uniforms
                                      length: sizeof(renderGroup->uniforms)
                                      attributeStride: 0
                                      atIndex: 7];
                    [renderCommandEnc setVertexBytes: &Entry->modelMatrix
                                      length: sizeof(Entry->modelMatrix)
                                      attributeStride: 0
                                      atIndex: 8];
                    [renderCommandEnc drawIndexedPrimitives: MTLPrimitiveTypeTriangle
                                      indexCount: Entry->sizeOfIndices / sizeof(s16)
                                      indexType: MTLIndexTypeUInt16
                                      indexBuffer: renderGroupIndexBuffer
                                      indexBufferOffset: renderGroupIndexBufferOffset];
                    [renderCommandEnc endEncoding];
                    break;
                }
                case RenderGroupEntryType_RenderEntryTile:
                {
                    RenderEntryTile *Entry = (RenderEntryTile *)(EntryHeader + 1);

                    MTLRenderPassDescriptor *renderPassDesc =
                        [MTLRenderPassDescriptor renderPassDescriptor];
                    renderPassDesc.colorAttachments[0].texture = drawable.texture;
                    renderPassDesc.colorAttachments[0].loadAction = MTLLoadActionLoad;

                    if (!depthTex ||
                        layer.drawableSize.width != depthTex.width ||
                        layer.drawableSize.height != depthTex.height)
                    {
                        if (depthTex)
                        {
                            [depthTex release];
                            renderPassDesc.depthAttachment.loadAction = MTLLoadActionLoad;
                        }
                        else
                        {
                            renderPassDesc.depthAttachment.loadAction = MTLLoadActionClear;
                        }
                        MTLTextureDescriptor *depthTexDesc =
                            [MTLTextureDescriptor
                             texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                             width:layer.drawableSize.width
                             height:layer.drawableSize.height
                             mipmapped:NO];
                        depthTexDesc.usage = MTLTextureUsageRenderTarget
                                             | MTLTextureUsageShaderRead;
                        depthTex = [metalDevice newTextureWithDescriptor:depthTexDesc];
                    }
                    renderPassDesc.depthAttachment.texture = depthTex;
                    renderPassDesc.depthAttachment.storeAction = MTLStoreActionStore;
                    renderPassDesc.depthAttachment.clearDepth = 1.0f;

                    u8 *renderGroupVertexBufferDestStart = ((u8 *)[renderGroupVertexBuffer contents]) + renderGroupVertexBufferCurrentSize;
                    u32 currSize = 0;
                    memcpy(renderGroupVertexBufferDestStart,
                           (void *)&Entry->tileCountPerSide,
                           sizeof(Entry->tileCountPerSide));
                    currSize += sizeof(Entry->tileCountPerSide);
                    memcpy(renderGroupVertexBufferDestStart + currSize,
                           (void *)&Entry->tileSideLength,
                           sizeof(Entry->tileSideLength));
                    currSize += sizeof(Entry->tileSideLength);
                    memcpy(renderGroupVertexBufferDestStart + currSize,
                           (void *)&Entry->color,
                           sizeof(Entry->color));
                    currSize += sizeof(Entry->color);
                    memcpy(renderGroupVertexBufferDestStart + currSize,
                           (void *)&Entry->originTileCenterPositionWorld,
                           sizeof(Entry->originTileCenterPositionWorld));
                    currSize += sizeof(Entry->originTileCenterPositionWorld);

                    renderGroupVertexBufferCurrentSize += currSize;
                    u32 renderGroupVertexBufferOffset = renderGroupVertexBufferDestStart
                                               - ((u8 *)[renderGroupVertexBuffer contents]);

                    id<MTLRenderCommandEncoder> renderCommandEnc =
                        [commandBuffer renderCommandEncoderWithDescriptor:renderPassDesc];
                    [renderCommandEnc setTriangleFillMode:MTLTriangleFillModeLines];
                    [renderCommandEnc setRenderPipelineState:renderPipelineStates[1]];
                    [renderCommandEnc setDepthStencilState:depthStencilState];
                    [renderCommandEnc setVertexBuffer: renderGroupVertexBuffer 
                                      offset: renderGroupVertexBufferOffset
                                      attributeStride: 0
                                      atIndex: 5];
                    [renderCommandEnc setVertexBytes: &renderGroup->uniforms
                                      length: sizeof(renderGroup->uniforms)
                                      attributeStride: 0
                                      atIndex: 7];
                    [renderCommandEnc drawPrimitives:MTLPrimitiveTypeTriangleStrip
                                      vertexStart: 0
                                      vertexCount: 4
                                      instanceCount: Entry->tileCountPerSide * Entry->tileCountPerSide
                                      baseInstance: 0];
                    [renderCommandEnc endEncoding];
                    break;
                }
                default:
                    break;
            }

            EntryHeader = EntryHeader->next;
        }

        [commandBuffer presentDrawable:drawable];
        [commandBuffer commit];
    }
}
