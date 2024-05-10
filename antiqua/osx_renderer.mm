//
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
static id<MTLBuffer> renderGroupBuffer;
static u32 renderGroupBufferCurrentSize;

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

    // NOTE(dima): creating GPU buffer for render group
    {
        renderGroupBuffer = [metalDevice newBufferWithLength:KB(256)
                                    options:MTLResourceStorageModeShared];
        ASSERT(renderGroupBuffer != 0);
    }
}

MONExternC RENDER_ON_GPU(renderOnGpu)
{
    @autoreleasepool
    {
        /* TODO(dima): warnings to address:
           - do not have a standalone clear entry
           - do not store depth / color texture after the last render entry */

        renderGroupBufferCurrentSize = 0;

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

                    u8 *bufferContents = (u8 *)[renderGroupBuffer contents];
                    r32 vertices[2 * 3 * 2];
                    memcpy(vertices, (void*)&Entry->start, sizeof(Entry->start));
                    memcpy(vertices + 3, (void*)&Entry->color, sizeof(Entry->color));
                    memcpy(vertices + 6, (void*)&Entry->end, sizeof(Entry->end));
                    memcpy(vertices + 9, (void*)&Entry->color, sizeof(Entry->color));

                    u8 *renderGroupBufferDestStart = ((u8 *)[renderGroupBuffer contents])
                                                     + renderGroupBufferCurrentSize;
                    u8 sizeOfData = sizeof(r32) * ARRAY_COUNT(vertices); 
                    memcpy(renderGroupBufferDestStart,
                           vertices,
                           sizeOfData);
                    renderGroupBufferCurrentSize += sizeOfData;
                    u32 renderGroupBufferOffset = renderGroupBufferDestStart
                                               - ((u8 *)[renderGroupBuffer contents]);

                    id<MTLRenderCommandEncoder> renderCommandEnc =
                        [commandBuffer renderCommandEncoderWithDescriptor:renderPassDesc];
                    [renderCommandEnc setRenderPipelineState:renderPipelineStates[0]];
                    [renderCommandEnc setDepthStencilState:depthStencilState];
                    [renderCommandEnc setVertexBuffer: renderGroupBuffer 
                                      offset: renderGroupBufferOffset 
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

                    u8 *renderGroupBufferDestStart = ((u8 *)[renderGroupBuffer contents]) + renderGroupBufferCurrentSize;
                    memcpy(renderGroupBufferDestStart,
                           Entry->data,
                           Entry->size);
                    renderGroupBufferCurrentSize += Entry->size;
                    u32 renderGroupBufferOffset = renderGroupBufferDestStart
                                               - ((u8 *)[renderGroupBuffer contents]);

                    id<MTLRenderCommandEncoder> renderCommandEnc =
                        [commandBuffer renderCommandEncoderWithDescriptor:renderPassDesc];
                    [renderCommandEnc setRenderPipelineState:renderPipelineStates[0]];
                    [renderCommandEnc setDepthStencilState:depthStencilState];
                    [renderCommandEnc setVertexBuffer: renderGroupBuffer 
                                      offset: renderGroupBufferOffset
                                      attributeStride: 0
                                      atIndex: 5];
                    [renderCommandEnc setVertexBytes: &renderGroup->uniforms
                                      length: sizeof(renderGroup->uniforms)
                                      attributeStride: 0
                                      atIndex: 7];
                    [renderCommandEnc drawPrimitives:MTLPrimitiveTypeTriangle
                                      vertexStart: 0
                                      vertexCount: 36];
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
