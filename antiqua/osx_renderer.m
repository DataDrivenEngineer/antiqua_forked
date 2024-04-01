//
#include "types.h"
#include "osx_renderer.h"

#import <Metal/MTLDevice.h>
#import <Metal/MTLCommandQueue.h>
#import <Metal/MTLCommandBuffer.h>

static id<MTLDevice> metalDevice;
static id<MTLCommandQueue> commandQueue;
static CAMetalLayer *layer;
static id<MTLTexture> depthTex = 0;

void InitRenderer(CAMetalLayer *_layer)
{
    metalDevice = _layer.device;
    commandQueue = [metalDevice newCommandQueue];
    layer = _layer;
}

MONExternC RENDER_ON_GPU(renderOnGpu)
{
    @autoreleasepool
    {
        NSURL *libraryURL = [[NSBundle mainBundle] URLForResource:@"shaders"
                                                   withExtension:@"metallib"];
        ASSERT(libraryURL != 0);

        id<MTLLibrary> lib = [metalDevice newLibraryWithURL:libraryURL
                              error:0];
        ASSERT(lib != 0);

        id<MTLFunction> vertexFn = [lib newFunctionWithName:@"vertexShader"];
        id<MTLFunction> fragmentFn = [lib newFunctionWithName:@"fragmentShader"];

        MTLRenderPipelineDescriptor *renderPipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
        renderPipelineDesc.vertexFunction = vertexFn;
        renderPipelineDesc.fragmentFunction = fragmentFn;
        renderPipelineDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        renderPipelineDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

        id<MTLRenderPipelineState> renderPipelineState = [metalDevice newRenderPipelineStateWithDescriptor:renderPipelineDesc
                                                          error:0];
        ASSERT(renderPipelineState != 0);

        [lib release];
        [vertexFn release];
        [fragmentFn release];
        [renderPipelineDesc release];

        id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];

        id<CAMetalDrawable> drawable = [layer nextDrawable];

        MTLRenderPassDescriptor *renderPassDesc = [MTLRenderPassDescriptor renderPassDescriptor];
        renderPassDesc.colorAttachments[0].texture = drawable.texture;
        renderPassDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
        renderPassDesc.colorAttachments[0].clearColor = MTLClearColorMake(0.3f, 0.3f, 0.3f, 1.0f);

        if (!depthTex ||
            layer.drawableSize.width != depthTex.width ||
            layer.drawableSize.height != depthTex.height)
        {
            if (depthTex)
            {
                [depthTex release];
            }
            MTLTextureDescriptor *depthTexDesc = [MTLTextureDescriptor
                                                  texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                                  width:layer.drawableSize.width
                                                  height:layer.drawableSize.height
                                                  mipmapped:NO];
            depthTexDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
            depthTex = [metalDevice newTextureWithDescriptor:depthTexDesc];
        }
        renderPassDesc.depthAttachment.texture = depthTex;
        renderPassDesc.depthAttachment.loadAction = MTLLoadActionClear;
        renderPassDesc.depthAttachment.storeAction = MTLStoreActionStore;
        renderPassDesc.depthAttachment.clearDepth = 1.0f;

        MTLDepthStencilDescriptor *depthStencilDesc = [[MTLDepthStencilDescriptor alloc] init];
        depthStencilDesc.depthWriteEnabled = true;
        depthStencilDesc.depthCompareFunction = MTLCompareFunctionLess;
        id<MTLDepthStencilState> depthStencilState = [metalDevice newDepthStencilStateWithDescriptor:depthStencilDesc];
        [depthStencilDesc release];

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

        id<MTLBuffer> vertexBuffer = [metalDevice newBufferWithBytes:vertices 
                                     length:sizeof(r32) * ARRAY_COUNT(vertices)
                                     options:MTLResourceStorageModeShared];

        static r32 angle = 0.0f;
        angle += 0.01f;

        id<MTLRenderCommandEncoder> renderCommandEnc = [commandBuffer
                                                        renderCommandEncoderWithDescriptor:renderPassDesc];
        [renderCommandEnc setRenderPipelineState:renderPipelineState];
        [renderCommandEnc setDepthStencilState:depthStencilState];
        [renderCommandEnc setVertexBuffer: vertexBuffer 
                          offset: 0
                          attributeStride: 0
                          atIndex: 5];
        [renderCommandEnc setVertexBytes: &angle
                          length: sizeof(r32)
                          attributeStride: 0
                          atIndex: 7];
        [renderCommandEnc drawPrimitives:MTLPrimitiveTypeTriangle
                          vertexStart: 0
                          vertexCount: 36];
        [renderCommandEnc endEncoding];
        [commandBuffer presentDrawable:drawable];
        [commandBuffer commit];

        [renderPipelineState release];
        [depthStencilState release];
        [vertexBuffer release];
    }
}
