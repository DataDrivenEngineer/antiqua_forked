#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include "types.h"
#include "osx_renderer.h"
#include "antiqua_render_group.h"
#include "antiqua_platform.h"

#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalLayer.hpp>

static MTL::Device *device;
static MTL::CommandQueue *commandQueue;
static CA::MetalLayer *layer;
static MTL::Texture *depthTex = 0;

MONExternC void initRenderer(void *metalLayer)
{
    device = ((CA::MetalLayer *)metalLayer)->device();
    commandQueue = device->newCommandQueue();
    layer = (CA::MetalLayer *)metalLayer;
}

MONExternC RENDER_ON_GPU(renderOnGpu)
{
    NS::AutoreleasePool * autoreleasePool = NS::AutoreleasePool::alloc()->init();
    {
        MTL::Library *lib = device->newDefaultLibrary();
        ASSERT(lib != 0);

        MTL::Function *vertexFn = lib->newFunction(NS::String::string("vertexShader",
                                                                      NS::UTF8StringEncoding));
        MTL::Function *fragmentFn = lib->newFunction(NS::String::string("fragmentShader",
                                                                        NS::UTF8StringEncoding));
        MTL::RenderPipelineDescriptor *renderPipelineDesc = MTL::RenderPipelineDescriptor::alloc()->init();
        renderPipelineDesc->setVertexFunction(vertexFn);
        renderPipelineDesc->setFragmentFunction(fragmentFn);
        renderPipelineDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
        renderPipelineDesc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

        NS::Error *err = 0;
        MTL::RenderPipelineState *renderPipelineState =
            device->newRenderPipelineState(renderPipelineDesc,
                                           &err);
        ASSERT(renderPipelineState != 0);

        lib->release();
        vertexFn->release();
        fragmentFn->release();
        renderPipelineDesc->release();

        MTL::CommandBuffer *commandBuffer = commandQueue->commandBuffer();

        CA::MetalDrawable *drawable = layer->nextDrawable();

        MTL::RenderPassDescriptor *renderPassDesc = MTL::RenderPassDescriptor::renderPassDescriptor();
        renderPassDesc->colorAttachments()->object(0)->setTexture(drawable->texture());
        renderPassDesc->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
        renderPassDesc->colorAttachments()->object(0)->setClearColor(MTL::ClearColor::Make(0.3f, 0.3f, 0.3f, 1.0f));

        if (!depthTex ||
            layer->drawableSize().width != depthTex->width() ||
            layer->drawableSize().height != depthTex->height())
        {
            if (depthTex)
            {
                depthTex->release();
            }
            MTL::TextureDescriptor *depthTexDesc =
                MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatDepth32Float,
                                                            layer->drawableSize().width,
                                                            layer->drawableSize().height,
                                                            false);
            depthTexDesc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
            depthTex = device->newTexture(depthTexDesc);
        }
        MTL::RenderPassDepthAttachmentDescriptor *renderPassDepthAttachmentDesc =
            MTL::RenderPassDepthAttachmentDescriptor::alloc()->init();
        renderPassDepthAttachmentDesc->setTexture(depthTex);
//        renderPassDesc->depthAttachment().loadAction = MTLLoadActionClear;
//        renderPassDesc->depthAttachment().storeAction = MTLStoreActionStore;
        renderPassDepthAttachmentDesc->setClearDepth(1.0f);
        renderPassDesc->setDepthAttachment(renderPassDepthAttachmentDesc);

        MTL::DepthStencilDescriptor *depthStencilDesc = MTL::DepthStencilDescriptor::alloc()->init();
        depthStencilDesc->setDepthWriteEnabled(true);
        depthStencilDesc->setDepthCompareFunction(MTL::CompareFunctionLess);
        MTL::DepthStencilState *depthStencilState = device->newDepthStencilState(depthStencilDesc);
        depthStencilDesc->release();

        r32 vertices[] =
        {
            // front face
            -0.5f, 0.5f, 0.0f,  // vertex #0 - coordinates
            1.0f, 0.0f, 0.0f, // color = red
            -0.5f, -0.5f, 0.0f,  // vertex #1 - coordinates
            1.0f, 0.0f, 0.0f, // color = red
            0.5f, -0.5f, 0.0f,   // vertex #2 - coordinates
            1.0f, 0.0f, 0.0f, // color = red
            0.5f, -0.5f, 0.0f,   // vertex #2 - coordinates
            1.0f, 0.0f, 0.0f, // color = red
            0.5f, 0.5f, 0.0f,   // vertex #3 - coordinates
            1.0f, 0.0f, 0.0f, // color = red
            -0.5f, 0.5f, 0.0f, // vertex #0 - coordinates
            1.0f, 0.0f, 0.0f, // color = red
            // back face
            -0.5f, 0.5f, 1.0f,  // vertex #4 - coordinates
            0.0f, 1.0f, 0.0f, // color = green
            -0.5f, -0.5f, 1.0f,  // vertex #5 - coordinates
            0.0f, 1.0f, 0.0f, // color = green
            0.5f, -0.5f, 1.0f,   // vertex #6 - coordinates
            0.0f, 1.0f, 0.0f, // color = green
            0.5f, -0.5f, 1.0f,   // vertex #6 - coordinates
            0.0f, 1.0f, 0.0f, // color = green
            0.5f, 0.5f, 1.0f,    // vertex #7 - coordinates
            0.0f, 1.0f, 0.0f, // color = green
            -0.5f, 0.5f, 1.0f,  // vertex #4 - coordinates
            0.0f, 1.0f, 0.0f, // color = green
            // left face
            -0.5f, 0.5f, 1.0f,  // vertex #4 - coordinates
            0.0f, 0.0f, 1.0f, // color = blue
            -0.5f, -0.5f, 1.0f,  // vertex #5 - coordinates
            0.0f, 0.0f, 1.0f, // color = blue
            -0.5f, -0.5f, 0.0f,  // vertex #1 - coordinates
            0.0f, 0.0f, 1.0f, // color = blue
            -0.5f, -0.5f, 0.0f,  // vertex #1 - coordinates
            0.0f, 0.0f, 1.0f, // color = blue
            -0.5f, 0.5f, 0.0f,  // vertex #0 - coordinates
            0.0f, 0.0f, 1.0f, // color = blue
            -0.5f, 0.5f, 1.0f,  // vertex #4 - coordinates
            0.0f, 0.0f, 1.0f, // color = blue
            // right face
            0.5f, 0.5f, 0.0f,   // vertex #3 - coordinates
            1.0f, 0.4f, 0.0f, // color = orange
            0.5f, -0.5f, 0.0f,   // vertex #2 - coordinates
            1.0f, 0.4f, 0.0f, // color = orange
            0.5f, -0.5f, 1.0f,   // vertex #6 - coordinates
            1.0f, 0.4f, 0.0f, // color = orange
            0.5f, -0.5f, 1.0f,   // vertex #6 - coordinates
            1.0f, 0.4f, 0.0f, // color = orange
            0.5f, 0.5f, 1.0f,    // vertex #7 - coordinates
            1.0f, 0.4f, 0.0f, // color = orange
            0.5f, 0.5f, 0.0f,   // vertex #3 - coordinates
            1.0f, 0.4f, 0.0f, // color = orange
            // top face
            -0.5f, 0.5f, 1.0f,  // vertex #4 - coordinates
            1.0f, 0.0f, 1.0f, // color = purple
            -0.5f, 0.5f, 0.0f,  // vertex #0 - coordinates
            1.0f, 0.0f, 1.0f, // color = purple
            0.5f, 0.5f, 0.0f,   // vertex #3 - coordinates
            1.0f, 0.0f, 1.0f, // color = purple
            0.5f, 0.5f, 0.0f,   // vertex #3 - coordinates
            1.0f, 0.0f, 1.0f, // color = purple
            0.5f, 0.5f, 1.0f,    // vertex #7 - coordinates
            1.0f, 0.0f, 1.0f, // color = purple
            -0.5f, 0.5f, 1.0f,  // vertex #4 - coordinates
            1.0f, 0.0f, 1.0f, // color = purple
            // bottom face
            -0.5f, -0.5f, 1.0f,  // vertex #5 - coordinates
            0.0f, 1.0f, 1.0f, // color = cyan
            -0.5f, -0.5f, 0.0f,  // vertex #1 - coordinates
            0.0f, 1.0f, 1.0f, // color = cyan
            0.5f, -0.5f, 0.0f,   // vertex #2 - coordinates
            0.0f, 1.0f, 1.0f, // color = cyan
            0.5f, -0.5f, 0.0f,   // vertex #2 - coordinates
            0.0f, 1.0f, 1.0f, // color = cyan
            0.5f, -0.5f, 1.0f,   // vertex #6 - coordinates
            0.0f, 1.0f, 1.0f, // color = cyan
            -0.5f, -0.5f, 1.0f,  // vertex #5 - coordinates
            0.0f, 1.0f, 1.0f, // color = cyan
        };

        MTL::Buffer *vertexBuffer = device->newBuffer(vertices,
                                                      sizeof(r32) * ARRAY_COUNT(vertices),
                                                      MTL::ResourceStorageModeShared);

        MTL::RenderCommandEncoder *renderCommandEnc = commandBuffer->renderCommandEncoder(renderPassDesc);
        renderCommandEnc->setRenderPipelineState(renderPipelineState);
        renderCommandEnc->setDepthStencilState(depthStencilState);
        renderCommandEnc->setVertexBuffer(vertexBuffer, 0, 5);
        renderCommandEnc->setVertexBytes(&renderGroup->uniforms, sizeof(renderGroup->uniforms), 7);
//        [renderCommandEnc setVertexBytes: mousePos
//                          length: mousePosSizeInBytes
//                          attributeStride: 0
//                          atIndex: 8];
        renderCommandEnc->drawPrimitives(MTL::PrimitiveTypeTriangle,
                                         NS::UInteger(0),
                                         NS::UInteger(36));
        renderCommandEnc->endEncoding();
        commandBuffer->presentDrawable(drawable);
        commandBuffer->commit();

        renderPassDepthAttachmentDesc->release();
        renderPipelineState->release();
        depthStencilState->release();
        vertexBuffer->release();
    }

    autoreleasePool->release();
}
