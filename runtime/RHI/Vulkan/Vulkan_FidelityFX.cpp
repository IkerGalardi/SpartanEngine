/*
Copyright(c) 2016-2024 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//= INCLUDES ==================================
#include "pch.h"
#include "../RHI_FidelityFX.h"
#include "../RHI_Implementation.h"
#include "../RHI_CommandList.h"
#include "../RHI_Texture.h"
#include "../RHI_TextureCube.h"
#include "../RHI_StructuredBuffer.h"
#include "../Rendering/Renderer_Buffers.h"
#include "../../World/Components/Camera.h"
SP_WARNINGS_OFF
#include <FidelityFX/host/backends/vk/ffx_vk.h>
#include <FidelityFX/host/ffx_fsr3.h>
#include <FidelityFX/host/ffx_sssr.h>
#include <FidelityFX/host/ffx_brixelizer.h>
#include <FidelityFX/host/ffx_brixelizergi.h>
SP_WARNINGS_ON
//=============================================

//= NAMESPACES ===============
using namespace Spartan::Math;
using namespace std;
//============================

namespace Spartan
{
    namespace // holds ffx structures, adapter functions and resources like scratch buffers
    {
        // common
        FfxInterface ffx_interface = {};

        // fsr 3
        bool fsr3_context_created                                                 = false;
        FfxFsr3UpscalerContext fsr3_context                                       = {};
        FfxFsr3UpscalerContextDescription fsr3_description_context                = {};
        FfxFsr3UpscalerDispatchDescription fsr3_description_dispatch              = {};
        FfxFsr3UpscalerGenerateReactiveDescription fsr3_description_reactive_mask = {};
        uint32_t fsr3_jitter_index                                                = 0;

        // sssr
        bool sssr_context_created                            = false;
        FfxSssrContext sssr_context                          = {};
        FfxSssrContextDescription sssr_description_context   = {};
        FfxSssrDispatchDescription sssr_description_dispatch = {};
        shared_ptr<RHI_Texture> sssr_cubemap;

        // brixelizer gi
        bool brixelizer_gi_context_created                                      = false;
        FfxBrixelizerGIContext brixelizer_gi_context                            = {};
        FfxBrixelizerContext brixelizer_context                                 = {};
        FfxBrixelizerGIContextDescription brixelizer_gi_description_context     = {};
        FfxBrixelizerContextDescription brixelizer_description_context          = {};
        FfxBrixelizerGIDispatchDescription brixelizer_gi_description_dispatch   = {};
        FfxBrixelizerUpdateDescription brixelizer_description_update            = {};
        FfxBrixelizerBakedUpdateDescription brixelizer_description_update_baked = {};
        const float brixelizer_gi_mesh_unit_size                                = 0.2f;
        const float brixelizer_gi_cascade_size_ratio                            = 2.0f;
        const uint32_t brixelizer_gi_max_cascades                               = 24;
        const uint32_t brixelizer_gi_cascade_count                              = brixelizer_gi_max_cascades / 3;
        shared_ptr<RHI_StructuredBuffer> brixelizer_gi_buffer_scratch           = nullptr;

        void ffx_message_callback(FfxMsgType type, const wchar_t* message)
        {
            if (type == FFX_MESSAGE_TYPE_ERROR)
            {
                SP_LOG_ERROR("AMD FidelityFX: %ls", message);
            }
            else if (type == FFX_MESSAGE_TYPE_WARNING)
            {
                SP_LOG_WARNING("AMD FidelityFX: %ls", message);
            }
        }

        FfxSurfaceFormat to_ffx_surface_format(const RHI_Format format)
        {
            switch (format)
            {
            case RHI_Format::R32G32B32A32_Float:
                return FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT;
            case RHI_Format::R16G16B16A16_Float:
                return FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
            case RHI_Format::R32G32_Float:
                return FFX_SURFACE_FORMAT_R32G32_FLOAT;
            case RHI_Format::R8_Uint:
                return FFX_SURFACE_FORMAT_R8_UINT;
            case RHI_Format::R32_Uint:
                return FFX_SURFACE_FORMAT_R32_UINT;
            case RHI_Format::R8G8B8A8_Unorm:
                return FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
            case RHI_Format::R11G11B10_Float:
                return FFX_SURFACE_FORMAT_R11G11B10_FLOAT;
            case RHI_Format::R16G16_Float:
                return FFX_SURFACE_FORMAT_R16G16_FLOAT;
            case RHI_Format::R16_Uint:
                return FFX_SURFACE_FORMAT_R16_UINT;
            case RHI_Format::R16_Float:
                return FFX_SURFACE_FORMAT_R16_FLOAT;
            case RHI_Format::R16_Unorm:
                return FFX_SURFACE_FORMAT_R16_UNORM;
            case RHI_Format::R8_Unorm:
                return FFX_SURFACE_FORMAT_R8_UNORM;
            case RHI_Format::R8G8_Unorm:
                return FFX_SURFACE_FORMAT_R8G8_UNORM;
            case RHI_Format::R32_Float:
            case RHI_Format::D32_Float:
                return FFX_SURFACE_FORMAT_R32_FLOAT;
            case RHI_Format::Max:
                return FFX_SURFACE_FORMAT_UNKNOWN;
            default:
                SP_ASSERT_MSG(false, "Unsupported format");
                return FFX_SURFACE_FORMAT_UNKNOWN;
            }
        }

        FfxResourceStates to_ffx_resource_state(RHI_Image_Layout layout)
        {
            switch (layout)
            {
            case RHI_Image_Layout::General:
                return FFX_RESOURCE_STATE_COMMON;
            case RHI_Image_Layout::Attachment:
                return FFX_RESOURCE_STATE_RENDER_TARGET;
            case RHI_Image_Layout::Shader_Read:
                return FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ;
            case RHI_Image_Layout::Transfer_Source:
                return FFX_RESOURCE_STATE_COPY_SRC;
            case RHI_Image_Layout::Transfer_Destination:
                return FFX_RESOURCE_STATE_COPY_DEST;
            case RHI_Image_Layout::Present_Source:
                return FFX_RESOURCE_STATE_PRESENT;
            default:
                SP_ASSERT_MSG(false, "Unsupported layout");
                return FFX_RESOURCE_STATE_COMMON;
            }
        }

        FfxResource to_ffx_resource(RHI_Texture* texture, RHI_StructuredBuffer* buffer, const wchar_t* name)
        {
            void* resource                     = nullptr;
            FfxResourceDescription description = {};
            FfxResourceStates state            = FFX_RESOURCE_STATE_COMMON;

            if (texture)
            {
                resource = texture->GetRhiResource();
                state    = to_ffx_resource_state(texture->GetLayout(0));

                // usage
                bool is_cubemap = texture->GetResourceType() == ResourceType::TextureCube;
                uint32_t usage = FFX_RESOURCE_USAGE_READ_ONLY;
                if (texture->IsDepthFormat())
                    usage |= FFX_RESOURCE_USAGE_DEPTHTARGET;
                if (texture->IsUav())
                    usage |= FFX_RESOURCE_USAGE_UAV;
                if (texture->GetResourceType() == ResourceType::Texture2dArray || is_cubemap)
                    usage |= FFX_RESOURCE_USAGE_ARRAYVIEW; // can be used for both 2d arrays and cubemaps
                if (texture->IsRtv())
                    usage |= FFX_RESOURCE_USAGE_RENDERTARGET;

                // description
                description.type     = is_cubemap ? FFX_RESOURCE_TYPE_TEXTURE_CUBE : FFX_RESOURCE_TYPE_TEXTURE2D;
                description.width    = texture->GetWidth();
                description.height   = texture->GetHeight();
                description.mipCount = texture->GetMipCount();
                description.depth    = texture->GetArrayLength();
                description.format   = to_ffx_surface_format(texture->GetFormat());
                description.flags    = FfxResourceFlags::FFX_RESOURCE_FLAGS_NONE;
                description.usage    = static_cast<FfxResourceUsage>(usage);
            }
            else
            {
                resource         = buffer->GetRhiResource();
                description.type = FFX_RESOURCE_TYPE_BUFFER;
                state            = FFX_RESOURCE_STATE_COMMON;
            }

            return ffxGetResourceVK(
                resource,
                description,
                const_cast<wchar_t*>(name),
                state
            );
        }

        void set_ffx_float3(FfxFloat32x3& dest, const Vector3& source)
        {
            dest[0] = source.x;
            dest[1] = source.y;
            dest[2] = source.z;
        }

        void set_ffx_float16(float* ffx_matrix, const Matrix& matrix)
        {
            const float* data = matrix.Data();
            memcpy(ffx_matrix, data, sizeof(Matrix));
        }
    }

    void RHI_FidelityFX::Initialize()
    {
        // ffx interface
        {
            // all used contexts need to be accounted for here
            const size_t countext_count =
                FFX_FSR3_CONTEXT_COUNT       +
                FFX_SSSR_CONTEXT_COUNT       +
                FFX_BRIXELIZER_CONTEXT_COUNT +
                FFX_BRIXELIZER_GI_CONTEXT_COUNT;
            
            VkDeviceContext device_context  = {};
            device_context.vkDevice         = RHI_Context::device;
            device_context.vkPhysicalDevice = RHI_Context::device_physical;
            device_context.vkDeviceProcAddr = vkGetDeviceProcAddr;
            
            const size_t scratch_buffer_size = ffxGetScratchMemorySizeVK(RHI_Context::device_physical, countext_count);
            void* scratch_buffer             = calloc(1, scratch_buffer_size);
            
            FfxErrorCode error_code = ffxGetInterfaceVK(&ffx_interface, ffxGetDeviceVK(&device_context), scratch_buffer, scratch_buffer_size, countext_count);
            SP_ASSERT(error_code == FFX_OK);
        }

        // assets
        {
            sssr_cubemap = make_unique<RHI_TextureCube>(1, 1, RHI_Format::R16G16B16A16_Float, RHI_Texture_Srv, "ffx_environment");

            uint32_t size = 1 << 30;
            brixelizer_gi_buffer_scratch = make_shared<RHI_StructuredBuffer>(size, 1, "ffx_brixelizer_gi_scratch");
        }
    }

    void RHI_FidelityFX::DestroyContexts()
    {
        // brixelizer gi
        if (brixelizer_gi_context_created)
        {
            SP_ASSERT(ffxBrixelizerContextDestroy(&brixelizer_context) == FFX_OK);
            SP_ASSERT(ffxBrixelizerGIContextDestroy(&brixelizer_gi_context) == FFX_OK);
            brixelizer_gi_context_created = false;
        }

        // sssr
        if (sssr_context_created)
        {
            SP_ASSERT(ffxSssrContextDestroy(&sssr_context) == FFX_OK);
            sssr_context_created = false;
        }

        // fsr 3
        if (fsr3_context_created)
        {
            SP_ASSERT(ffxFsr3UpscalerContextDestroy(&fsr3_context) == FFX_OK);
            fsr3_context_created = false;
        }
    }

    void RHI_FidelityFX::Shutdown()
    {
        sssr_cubemap                 = nullptr;
        brixelizer_gi_buffer_scratch = nullptr;

        DestroyContexts();

        // ffx interface
        if (ffx_interface.scratchBuffer != nullptr)
        {
            free(ffx_interface.scratchBuffer);
        }
    }

    void RHI_FidelityFX::Resize(const Vector2& resolution_render, const Vector2& resolution_output)
    {
        // contexts are resolution dependent, so we destroy and (re)create them when resizing
        DestroyContexts();

        // fs3
        if (!fsr3_context_created)
        {
            // description
            fsr3_description_context.maxRenderSize.width    = static_cast<uint32_t>(resolution_render.x);
            fsr3_description_context.maxRenderSize.height   = static_cast<uint32_t>(resolution_render.y);
            fsr3_description_context.maxUpscaleSize.width   = static_cast<uint32_t>(resolution_output.x);
            fsr3_description_context.maxUpscaleSize.height  = static_cast<uint32_t>(resolution_output.y);
            fsr3_description_context.flags                  = FFX_FSR3_ENABLE_UPSCALING_ONLY | FFX_FSR3_ENABLE_DEPTH_INVERTED | FFX_FSR3_ENABLE_DYNAMIC_RESOLUTION;
            fsr3_description_context.flags                 |= FFX_FSR3_ENABLE_HIGH_DYNAMIC_RANGE; // hdr input
            #ifdef DEBUG
            fsr3_description_context.flags                 |= FFX_FSR3_ENABLE_DEBUG_CHECKING;
            fsr3_description_context.fpMessage              = &ffx_message_callback;
            #endif
            fsr3_description_context.backendInterface       = ffx_interface;

            // context
            SP_ASSERT(ffxFsr3UpscalerContextCreate(&fsr3_context, &fsr3_description_context) == FFX_OK);
            fsr3_context_created = true;

            // reset jitter index
            fsr3_jitter_index = 0;
        }

        // sssr
        if (!sssr_context_created)
        {
             sssr_description_context.renderSize.width           = static_cast<uint32_t>(resolution_render.x);
             sssr_description_context.renderSize.height          = static_cast<uint32_t>(resolution_render.y);
             sssr_description_context.normalsHistoryBufferFormat = to_ffx_surface_format(RHI_Format::R16G16B16A16_Float);
             sssr_description_context.flags                      = FFX_SSSR_ENABLE_DEPTH_INVERTED;
             sssr_description_context.backendInterface           = ffx_interface;

             SP_ASSERT(ffxSssrContextCreate(&sssr_context, &sssr_description_context) == FFX_OK);
             sssr_context_created = true;
        }

        // brixelizer gi
        if (!brixelizer_gi_context_created)
        {
            // brixelizer context
            {
                // sdf
                brixelizer_description_context.sdfCenter[0]     = 0.0f;
                brixelizer_description_context.sdfCenter[1]     = 0.0f;
                brixelizer_description_context.sdfCenter[2]     = 0.0f;

                // cascades
                brixelizer_description_context.numCascades = brixelizer_gi_cascade_count;
                float voxel_size = brixelizer_gi_mesh_unit_size;
                for (uint32_t i = 0; i < brixelizer_description_context.numCascades; ++i)
                {
                    FfxBrixelizerCascadeDescription* cascadeDesc  = &brixelizer_description_context.cascadeDescs[i];
                    cascadeDesc->flags                            = (FfxBrixelizerCascadeFlag)(FFX_BRIXELIZER_CASCADE_STATIC | FFX_BRIXELIZER_CASCADE_DYNAMIC);
                    cascadeDesc->voxelSize                        = brixelizer_gi_mesh_unit_size;
                    voxel_size                                   *= brixelizer_gi_cascade_size_ratio;
                }

                // misc
                #ifdef DEBUG
                brixelizer_description_context.flags = FFX_BRIXELIZER_CONTEXT_FLAG_ALL_DEBUG;
                #endif
                brixelizer_description_context.backendInterface = ffx_interface;

                SP_ASSERT(ffxBrixelizerContextCreate(&brixelizer_description_context, &brixelizer_context) == FFX_OK);
            }

            // brixelizer gi context (sits on top of the brixelizer context)
            {
                brixelizer_gi_description_context.internalResolution = FfxBrixelizerGIInternalResolution::FFX_BRIXELIZER_GI_INTERNAL_RESOLUTION_NATIVE;
                brixelizer_gi_description_context.displaySize.width  = static_cast<uint32_t>(resolution_render.x);
                brixelizer_gi_description_context.displaySize.height = static_cast<uint32_t>(resolution_render.y);
                brixelizer_gi_description_context.flags              = FfxBrixelizerGIFlags::FFX_BRIXELIZER_GI_FLAG_DEPTH_INVERTED;
                brixelizer_gi_description_context.backendInterface   = ffx_interface;
                
                SP_ASSERT(ffxBrixelizerGIContextCreate(&brixelizer_gi_context, &brixelizer_gi_description_context) == FFX_OK);
            }

            brixelizer_gi_context_created = true;
        }
    }

    void RHI_FidelityFX::FSR3_ResetHistory()
    {
        fsr3_description_dispatch.reset = true;
    }

    void RHI_FidelityFX::FSR3_GenerateJitterSample(float* x, float* y)
    {
        // get jitter phase count
        const uint32_t resolution_render_x = static_cast<uint32_t>(fsr3_description_context.maxRenderSize.width);
        const uint32_t resolution_render_y = static_cast<uint32_t>(fsr3_description_context.maxRenderSize.height);
        const int32_t jitter_phase_count   = ffxFsr3GetJitterPhaseCount(resolution_render_x, resolution_render_x);

        // ensure fsr3_jitter_index is properly wrapped around the jitter_phase_count
        fsr3_jitter_index = (fsr3_jitter_index + 1) % jitter_phase_count;

        // generate jitter sample
        FfxErrorCode result = ffxFsr3GetJitterOffset(&fsr3_description_dispatch.jitterOffset.x, &fsr3_description_dispatch.jitterOffset.y, fsr3_jitter_index, jitter_phase_count);
        SP_ASSERT(result == FFX_OK);

        // adjust the jitter offset for the projection matrix, based on the function comments
        *x =  2.0f * fsr3_description_dispatch.jitterOffset.x / resolution_render_x;
        *y = -2.0f * fsr3_description_dispatch.jitterOffset.y / resolution_render_y;
    }

    void RHI_FidelityFX::FSR3_Dispatch
    (
        RHI_CommandList* cmd_list,
        Camera* camera,
        const float delta_time_sec,
        const float sharpness,
        const float exposure,
        const float resolution_scale,
        RHI_Texture* tex_color,
        RHI_Texture* tex_depth,
        RHI_Texture* tex_velocity,
        RHI_Texture* tex_color_opaque,
        RHI_Texture* tex_reactive,
        RHI_Texture* tex_output
    )
    {
        // documentation: https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/main/docs/techniques/super-resolution-upscaler.md
        // requires:      VK_KHR_get_memory_requirements2

        // output is displayed in the viewport via imgui so we add
        // a barrier to ensure FSR has done whatever it needs to do
        cmd_list->InsertBarrierTextureReadWrite(tex_output);
        cmd_list->InsertPendingBarrierGroup();

        // generate reactive mask
        {
            // set resources
            fsr3_description_reactive_mask.commandList       = ffxGetCommandListVK(static_cast<VkCommandBuffer>(cmd_list->GetRhiResource()));
            fsr3_description_reactive_mask.colorOpaqueOnly   = to_ffx_resource(tex_color_opaque, nullptr, L"fsr3_color_opaque");
            fsr3_description_reactive_mask.colorPreUpscale   = to_ffx_resource(tex_color,        nullptr, L"fsr3_color");
            fsr3_description_reactive_mask.outReactive       = to_ffx_resource(tex_reactive,     nullptr, L"fsr3_reactive");

            // configure
            fsr3_description_reactive_mask.renderSize.width  = static_cast<uint32_t>(tex_velocity->GetWidth() * resolution_scale);
            fsr3_description_reactive_mask.renderSize.height = static_cast<uint32_t>(tex_velocity->GetHeight() * resolution_scale);
            fsr3_description_reactive_mask.scale             = 1.0f; // global multiplier for reactivity, higher values increase overall reactivity
            fsr3_description_reactive_mask.cutoffThreshold   = 0.8f; // difference threshold, lower values make more pixels reactive
            fsr3_description_reactive_mask.binaryValue       = 1.0f; // value assigned to reactive pixels in the mask (typically 1.0)
            fsr3_description_reactive_mask.flags             = 0;

            // dispatch
            SP_ASSERT(ffxFsr3UpscalerContextGenerateReactiveMask(&fsr3_context, &fsr3_description_reactive_mask) == FFX_OK);
        }

        // upscale
        {
            // set resources
            fsr3_description_dispatch.commandList   = ffxGetCommandListVK(static_cast<VkCommandBuffer>(cmd_list->GetRhiResource()));
            fsr3_description_dispatch.color         = to_ffx_resource(tex_color,    nullptr, L"fsr3_color");
            fsr3_description_dispatch.depth         = to_ffx_resource(tex_depth,    nullptr, L"fsr3_depth");
            fsr3_description_dispatch.motionVectors = to_ffx_resource(tex_velocity, nullptr, L"fsr3_velocity");
            fsr3_description_dispatch.reactive      = to_ffx_resource(tex_reactive, nullptr, L"fsr3_reactive");
            fsr3_description_dispatch.output        = to_ffx_resource(tex_output,   nullptr, L"fsr3_output");

            // configure
            fsr3_description_dispatch.motionVectorScale.x    = -static_cast<float>(tex_velocity->GetWidth());
            fsr3_description_dispatch.motionVectorScale.y    = -static_cast<float>(tex_velocity->GetHeight());
            fsr3_description_dispatch.enableSharpening       = sharpness != 0.0f;
            fsr3_description_dispatch.sharpness              = sharpness;
            fsr3_description_dispatch.frameTimeDelta         = delta_time_sec * 1000.0f;                         // seconds to milliseconds
            fsr3_description_dispatch.preExposure            = exposure;                                         // the exposure value if not using FFX_FSR3_ENABLE_AUTO_EXPOSURE
            fsr3_description_dispatch.renderSize.width       = fsr3_description_reactive_mask.renderSize.width;
            fsr3_description_dispatch.renderSize.height      = fsr3_description_reactive_mask.renderSize.height;
            fsr3_description_dispatch.cameraNear             = camera->GetFarPlane();                            // far as near because we are using reverse-z
            fsr3_description_dispatch.cameraFar              = camera->GetNearPlane();                           // near as far because we are using reverse-z
            fsr3_description_dispatch.cameraFovAngleVertical = camera->GetFovVerticalRad();

            // dispatch
            SP_ASSERT(ffxFsr3UpscalerContextDispatch(&fsr3_context, &fsr3_description_dispatch) == FFX_OK);
            fsr3_description_dispatch.reset = false;
        }
    }

    void RHI_FidelityFX::SSSR_Dispatch(
        RHI_CommandList* cmd_list,
        Cb_Frame* cb_frame,
        const float resolution_scale,
        RHI_Texture* tex_color,
        RHI_Texture* tex_depth,
        RHI_Texture* tex_velocity,
        RHI_Texture* tex_normal,
        RHI_Texture* tex_material,
        RHI_Texture* tex_brdf,
        RHI_Texture* tex_output
    )
    {
        // documentation: https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/main/docs/techniques/stochastic-screen-space-reflections.md

        // transition the depth to shader read, to avoid validation errors caused by ffx
        // when trying to create a depth view that is incompatible with the resource properties
        tex_depth->SetLayout(RHI_Image_Layout::Shader_Read, cmd_list);
        cmd_list->InsertPendingBarrierGroup();

        // set resources
        sssr_description_dispatch.commandList        = ffxGetCommandListVK(static_cast<VkCommandBuffer>(cmd_list->GetRhiResource()));
        sssr_description_dispatch.color              = to_ffx_resource(tex_color,          nullptr, L"sssr_color");
        sssr_description_dispatch.depth              = to_ffx_resource(tex_depth,          nullptr, L"sssr_depth");
        sssr_description_dispatch.motionVectors      = to_ffx_resource(tex_velocity,       nullptr, L"sssr_velocity");
        sssr_description_dispatch.normal             = to_ffx_resource(tex_normal,         nullptr, L"sssr_normal");
        sssr_description_dispatch.materialParameters = to_ffx_resource(tex_material,       nullptr, L"sssr_roughness"); // FfxSssrDispatchDescription specifies the channel
        sssr_description_dispatch.environmentMap     = to_ffx_resource(sssr_cubemap.get(), nullptr, L"sssr_environment");
        sssr_description_dispatch.brdfTexture        = to_ffx_resource(tex_brdf,           nullptr, L"sssr_brdf");
        sssr_description_dispatch.output             = to_ffx_resource(tex_output,         nullptr, L"sssr_output");
 
        // set render size
        sssr_description_dispatch.renderSize.width  = static_cast<uint32_t>(tex_color->GetWidth()  * resolution_scale);
        sssr_description_dispatch.renderSize.height = static_cast<uint32_t>(tex_color->GetHeight() * resolution_scale);

        // set camera matrices
        {
            auto adjust_matrix_view = [](const Matrix& matrix)
            {
                Matrix adjusted = matrix.Transposed();

                // negate the third row to switch handedness
                adjusted.m20 = -adjusted.m20;
                adjusted.m21 = -adjusted.m21;
                adjusted.m22 = -adjusted.m22;
                adjusted.m23 = -adjusted.m23;

                return adjusted;
            };

            auto adjust_matrix_projection = [](const Matrix& matrix)
            {
                Matrix adjusted = matrix.Transposed();

                // adjust for reverse-z
                adjusted.m22 = 0.0f;
                adjusted.m23 = matrix.m32; // near plane value
                adjusted.m32 = -1.0f;
                adjusted.m33 = 0.0f;

                return adjusted;
            };

            Matrix view                     = adjust_matrix_view(cb_frame->view);
            Matrix projection               = adjust_matrix_projection(cb_frame->projection);
            static Matrix view_projection   = Matrix::Identity;
            Matrix view_inv                 = Matrix::Invert(view);
            Matrix projection_inv           = Matrix::Invert(projection);
            Matrix view_projection_previous = view_projection;
            view_projection                 = projection * view;
            Matrix view_projection_inv      = Matrix::Invert(view_projection);

            // ffx expects column major layout
            set_ffx_float16(sssr_description_dispatch.view,               view);
            set_ffx_float16(sssr_description_dispatch.invView,            view_inv);
            set_ffx_float16(sssr_description_dispatch.projection,         projection);
            set_ffx_float16(sssr_description_dispatch.invProjection,      projection_inv);
            set_ffx_float16(sssr_description_dispatch.invViewProjection,  view_projection_inv);
            set_ffx_float16(sssr_description_dispatch.prevViewProjection, view_projection_previous);
        }

        // set sssr specific parameters
        sssr_description_dispatch.motionVectorScale.x                   = -0.5f; // expects [-0.5, 0.5] range
        sssr_description_dispatch.motionVectorScale.y                   = -0.5f; // expects [-0.5, 0.5] range, +Y as top-down
        sssr_description_dispatch.normalUnPackMul                       = 1.0f;
        sssr_description_dispatch.normalUnPackAdd                       = 0.0f;
        sssr_description_dispatch.depthBufferThickness                  = 0.08f; // hit acceptance bias, larger values can cause streaks, lower values can cause holes
        sssr_description_dispatch.varianceThreshold                     = 0.0f;  // luminance differences between history results will trigger an additional ray if they are greater than this threshold value
        sssr_description_dispatch.maxTraversalIntersections             = 32;    // caps the maximum number of lookups that are performed from the depth buffer hierarchy, most rays should end after about 20 lookups
        sssr_description_dispatch.minTraversalOccupancy                 = 4;     // exit the core loop early if less than this number of threads are running
        sssr_description_dispatch.mostDetailedMip                       = 0;
        sssr_description_dispatch.temporalStabilityFactor               = 0.5f;  // the accumulation of history values, Higher values reduce noise, but are more likely to exhibit ghosting artifacts
        sssr_description_dispatch.temporalVarianceGuidedTracingEnabled  = true;  // whether a ray should be spawned on pixels where a temporal variance is detected or not
        sssr_description_dispatch.samplesPerQuad                        = 1;     // the minimum number of rays per quad, variance guided tracing can increase this up to a maximum of 4
        sssr_description_dispatch.iblFactor                             = 0.0f;
        sssr_description_dispatch.roughnessChannel                      = 0;
        sssr_description_dispatch.isRoughnessPerceptual                 = true;
        sssr_description_dispatch.roughnessThreshold                    = 1.0f;  // regions with a roughness value greater than this threshold won't spawn rays

        // dispatch
        FfxErrorCode error_code = ffxSssrContextDispatch(&sssr_context, &sssr_description_dispatch);
        SP_ASSERT(error_code == FFX_OK);
    }

    void RHI_FidelityFX::BrixelizerGI_Update(
        RHI_CommandList* cmd_list,
        Cb_Frame* cb_frame
    )
    {
        FfxBrixelizerStats stats = {};
        size_t scratchBufferSize = 0;
        FfxBrixelizerDebugVisualizationDescription debug_description = {};

        //brixelizer_description_update.resources;                                                           // structure containing all resources to be used by the Brixelizer context
        brixelizer_description_update.frameIndex                = cb_frame->frame;                           // index of the current frame
        //brixelizer_description_update.sdfCenter[3];                                                        // center of the cascades
        #ifdef DEBUG
        brixelizer_description_update.populateDebugAABBsFlags = FFX_BRIXELIZER_POPULATE_AABBS_CASCADE_AABBS; // Flags determining which AABBs to draw in a debug visualization
        brixelizer_description_update.debugVisualizationDesc  = &debug_description;                          // optional debug visualization description. If this parameter is set to <c><i>NULL</i></c> no debug visualization is drawn
        #endif
        brixelizer_description_update.maxReferences           = 32 * (1 << 20);                              // maximum number of triangle voxel references to be stored in the update
        brixelizer_description_update.triangleSwapSize        = 300 * (1 << 20);                             // size of the swap space available to be used for storing triangles in the update
        brixelizer_description_update.maxBricksPerBake        = 1 << 14;                                     // maximum number of bricks to be updated
        brixelizer_description_update.outScratchBufferSize    = &scratchBufferSize;                          // optional pointer to a <c><i>size_t</i></c> to receive the size of the GPU scratch buffer needed to process the update
        brixelizer_description_update.outStats                = &stats;                                      // optional pointer to an <c><i>FfxBrixelizerStats</i></c> struct to receive statistics for the update. Note, stats read back after a call to update do

        // bake update
        FfxCommandList ffx_command_list = ffxGetCommandListVK(static_cast<VkCommandBuffer>(cmd_list->GetRhiResource()));
        FfxErrorCode error_code         = ffxBrixelizerBakeUpdate(&brixelizer_context, &brixelizer_description_update, &brixelizer_description_update_baked);
        SP_ASSERT(error_code == FFX_OK);

        SP_ASSERT_MSG(scratchBufferSize < brixelizer_gi_buffer_scratch->GetObjectSize(), "Required Brixelizer scratch memory size larger than available GPU buffer");

        // update
        FfxResource scratch_buffer = to_ffx_resource(nullptr, brixelizer_gi_buffer_scratch.get(), L"ffxBrixelizerUpdate_scratch_buffer");
        error_code                 = ffxBrixelizerUpdate(&brixelizer_context, &brixelizer_description_update_baked, scratch_buffer, ffx_command_list);
        SP_ASSERT(error_code == FFX_OK);
    }

    void RHI_FidelityFX::BrixelizerGI_Dispatch(
        RHI_CommandList* cmd_list,
        Cb_Frame* cb_frame,
        RHI_Texture* tex_depth,
        RHI_Texture* tex_velocity,
        RHI_Texture* tex_normal,
        RHI_Texture* tex_material,
        RHI_Texture* tex_diffuse_gi,
        RHI_Texture* tex_specular_gi
    )
    {
        // update bricks
        BrixelizerGI_Update(cmd_list, cb_frame);

        // set camera matrices
        {
            // ffx expects row major order
            set_ffx_float16(brixelizer_gi_description_dispatch.view,           cb_frame->view);
            set_ffx_float16(brixelizer_gi_description_dispatch.prevView,       cb_frame->view_previous);
            set_ffx_float16(brixelizer_gi_description_dispatch.projection,     cb_frame->projection);
            set_ffx_float16(brixelizer_gi_description_dispatch.prevProjection, cb_frame->projection_previous);
        }

        // set textures
        brixelizer_gi_description_dispatch.environmentMap   = to_ffx_resource(sssr_cubemap.get(), nullptr, L"brixelizer_environment");
        //brixelizer_gi_description_dispatch.prevLitOutput  =
        brixelizer_gi_description_dispatch.depth            = to_ffx_resource(tex_depth, nullptr, L"brixelizer_gi_depth");
        //brixelizer_gi_description_dispatch.historyDepth   =
        brixelizer_gi_description_dispatch.normal           = to_ffx_resource(tex_normal, nullptr, L"brixelizer_gi_normal");
        //brixelizer_gi_description_dispatch.historyNormal  =
        brixelizer_gi_description_dispatch.roughness        = to_ffx_resource(tex_material, nullptr, L"brixelizer_gi_roughness");
        brixelizer_gi_description_dispatch.motionVectors    = to_ffx_resource(tex_velocity, nullptr, L"brixelizer_gi_velocity");
        //brixelizer_gi_description_dispatch.noiseTexture   =
        brixelizer_gi_description_dispatch.outputDiffuseGI  = to_ffx_resource(tex_diffuse_gi, nullptr, L"brixelizer_gi_diffuse_gi");
        brixelizer_gi_description_dispatch.outputSpecularGI = to_ffx_resource(tex_specular_gi, nullptr, L"brixelizer_gi_specular_gi");

         // set sdf/spatial parameters
        set_ffx_float3(brixelizer_gi_description_dispatch.cameraPosition, cb_frame->camera_position); // camera position
        //brixelizer_gi_description_dispatch.startCascade        = // index of the start cascade for use with ray marching with Brixelizer
        //brixelizer_gi_description_dispatch.endCascade          = // index of the end cascade for use with ray marching with Brixelizer
        //brixelizer_gi_description_dispatch.rayPushoff          = // distance from a surface along the normal vector to offset the diffuse ray origin
        //brixelizer_gi_description_dispatch.sdfSolveEps         = // epsilon value for ray marching to be used with Brixelizer for diffuse rays
        //brixelizer_gi_description_dispatch.specularRayPushoff  = // distance from a surface along the normal vector to offset the specular ray origin
        //brixelizer_gi_description_dispatch.specularSDFSolveEps = // epsilon value for ray marching to be used with Brixelizer for specular rays
        //brixelizer_gi_description_dispatch.tMin                = // TMin value for use with Brixelizer
        //brixelizer_gi_description_dispatch.tMax                = // TMax value for use with Brixelizer
        //brixelizer_gi_description_dispatch.sdfAtlas            = // SDF Atlas resource used by Brixelizer
        //brixelizer_gi_description_dispatch.bricksAABBs         = // brick AABBs resource used by Brixelizer
        for (uint32_t i = 0; i < 24; ++i)
        {
            //brixelizer_gi_description_dispatch.cascadeAABBTrees[i] = // cascade AABB tree resources used by Brixelizer
            //brixelizer_gi_description_dispatch.cascadeBrickMaps[i] = // cascade brick map resources used by Brixelizer
        }

        // set engine specific parameters
        brixelizer_gi_description_dispatch.normalsUnpackMul        = 1.0f; // a multiply factor to transform the normal to the space expected by Brixelizer GI
        brixelizer_gi_description_dispatch.normalsUnpackAdd        = 0.0f; // an offset to transform the normal to the space expected by Brixelizer GI
        brixelizer_gi_description_dispatch.isRoughnessPerceptual   = true; // if false, we assume roughness squared was stored in the Gbuffer
        brixelizer_gi_description_dispatch.roughnessChannel        = 0;    // the channel to read the roughness from the roughness texture
        brixelizer_gi_description_dispatch.roughnessThreshold      = 1.0f; // regions with a roughness value greater than this threshold won't spawn specular rays
        brixelizer_gi_description_dispatch.environmentMapIntensity = 0.0f; // value to scale the contribution from the environment map
        brixelizer_gi_description_dispatch.motionVectorScale.x     = 1.0f; // scale factor to apply to motion vectors
        brixelizer_gi_description_dispatch.motionVectorScale.y     = 1.0f; // scale factor to apply to motion vectors

        // get the underlying brixelizer context (not the GI one)
        FfxErrorCode error_code = ffxBrixelizerGetRawContext(&brixelizer_context, &brixelizer_gi_description_dispatch.brixelizerContext);
        SP_ASSERT(error_code == FFX_OK);

        // dispatch
        FfxCommandList ffx_command_list = ffxGetCommandListVK(static_cast<VkCommandBuffer>(cmd_list->GetRhiResource()));
        error_code                      = ffxBrixelizerGIContextDispatch(&brixelizer_gi_context, &brixelizer_gi_description_dispatch, ffx_command_list);
        SP_ASSERT(error_code == FFX_OK);
    }
}
