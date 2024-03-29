// Include Files
//==============

#include "../sContext.h"
#include "../ColorFormats.h"
#include "../Graphics.h"

#include "Includes.h"

#include <Engine/Asserts/Asserts.h>
#include <Engine/Logging/Logging.h>
#include <Engine/Platform/Platform.h>

#include <External/DirectXTex/Includes.h>

#include <sstream>

// Helper Function Declarations
//=============================

namespace
{
    eae6320::cResult CreateDevice( const unsigned int i_resolutionWidth, const unsigned int i_resolutionHeight );
    eae6320::cResult InitializeViews(const unsigned int i_resolutionWidth, const unsigned int i_resolutionHeight);
}

// Interface
//==========

// Initialization / Clean Up
//--------------------------

eae6320::cResult eae6320::Graphics::sContext::Initialize( const sInitializationParameters& i_initializationParameters )
{
    auto result = Results::success;

    windowBeingRenderedTo = i_initializationParameters.mainWindow;

    // Create an interface to a Direct3D device
    if ( !( (result = CreateDevice( i_initializationParameters.resolutionWidth, i_initializationParameters.resolutionHeight )) ) )
    {
        EAE6320_ASSERT( false );
        goto OnExit;
    }

    // Initialize the views
    if (!((result = InitializeViews(i_initializationParameters.resolutionWidth, i_initializationParameters.resolutionHeight))))
    {
        EAE6320_ASSERT(false);
        goto OnExit;
    }

OnExit:

    return result;
}

eae6320::cResult eae6320::Graphics::sContext::CleanUp()
{
    const auto result = Results::success;

    if (backBuffer)
    {
        backBuffer->Release();
        backBuffer = nullptr;
    }

    if (renderTargetView)
    {
        renderTargetView->Release();
        renderTargetView = nullptr;
    }
    if (depthStencilView)
    {
        depthStencilView->Release();
        depthStencilView = nullptr;
    }
    if ( direct3DImmediateContext )
    {
        direct3DImmediateContext->Release();
        direct3DImmediateContext = nullptr;
    }
    if ( direct3DDevice )
    {
        direct3DDevice->Release();
        direct3DDevice = nullptr;
    }
    if ( swapChain )
    {
        swapChain->Release();
        swapChain = nullptr;
    }

    windowBeingRenderedTo = nullptr;

    return result;
}

// Render
//-------

void eae6320::Graphics::sContext::ClearImageBuffer(const ColorFormats::sColor i_color) const
{
    EAE6320_ASSERT(direct3DImmediateContext);
    EAE6320_ASSERT(renderTargetView);
    const float color[4] = { i_color.R(), i_color.G(), i_color.B(), i_color.A() };
    direct3DImmediateContext->ClearRenderTargetView(renderTargetView, color);
}

void eae6320::Graphics::sContext::ClearDepthBuffer(const float i_depth) const
{
    EAE6320_ASSERT(depthStencilView);
    constexpr uint8_t stencilValue = 0; // Arbitrary if stencil isn't used
    direct3DImmediateContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, i_depth, stencilValue);
}

void eae6320::Graphics::sContext::BufferSwap() const
{
    EAE6320_ASSERT(swapChain);
    constexpr unsigned int swapImmediately = 0;
    constexpr unsigned int presentNextFrame = 0;
    const auto result = swapChain->Present(swapImmediately, presentNextFrame);
    EAE6320_ASSERT(SUCCEEDED(result));
}

// User actions
//-------------
void eae6320::Graphics::sContext::GetRawImageFromBackBuffer(Platform::sDataFromFile& o_rawImageData) const
{
    DirectX::ScratchImage image = {};

    // Captures a Direct3D render target and returns an image
    {
        const auto d3DResult = DirectX::CaptureTexture(g_context.direct3DDevice, g_context.direct3DImmediateContext, backBuffer, image);
        if (FAILED(d3DResult))
        {
            EAE6320_ASSERTF(false, "Couldn't copy the back buffer to cpu side memory (HRESULT %#010x)", d3DResult);
            eae6320::Logging::OutputError("DirectXTex failed to copy the back buffer to cpu side memory (HRESULT %#010x)", d3DResult);
            return;
        }
    }

    EAE6320_ASSERTF(image.GetImageCount() == 1, "There shouldn't be more than one image (mip) in back buffer since MSAA is disabled");
    constexpr unsigned int imageIndex = 0;
    const DirectX::Image& backBufferImage = image.GetImages()[imageIndex];
    EAE6320_ASSERTF(o_rawImageData.size == backBufferImage.slicePitch, "The slicePitch is the whole texture for a 2D image buffer");
    memcpy_s(o_rawImageData.data, o_rawImageData.size, backBufferImage.pixels, backBufferImage.slicePitch);
}

// Helper Function Definitions
//============================

namespace
{
    eae6320::cResult CreateDevice( const unsigned int i_resolutionWidth, const unsigned int i_resolutionHeight )
    {
        auto& g_context = eae6320::Graphics::sContext::g_context;

        IDXGIAdapter* const useDefaultAdapter = nullptr;
        constexpr D3D_DRIVER_TYPE useHardwareRendering = D3D_DRIVER_TYPE_HARDWARE;
        constexpr HMODULE dontUseSoftwareRendering = nullptr;
        constexpr unsigned int flags = D3D11_CREATE_DEVICE_SINGLETHREADED
#ifdef EAE6320_GRAPHICS_ISDEVICEDEBUGINFOENABLED
            | D3D11_CREATE_DEVICE_DEBUG
#endif
            ;
        constexpr D3D_FEATURE_LEVEL* const useDefaultFeatureLevels = nullptr;
        constexpr unsigned int requestedFeatureLevelCount = 0;
        constexpr unsigned int sdkVersion = D3D11_SDK_VERSION;
        DXGI_SWAP_CHAIN_DESC swapChainDescription{};
        {
            {
                DXGI_MODE_DESC& bufferDescription = swapChainDescription.BufferDesc;

                bufferDescription.Width = i_resolutionWidth;
                bufferDescription.Height = i_resolutionHeight;
                {
                    DXGI_RATIONAL& refreshRate = bufferDescription.RefreshRate;

                    refreshRate.Numerator = 0;    // Refresh as fast as possible
                    refreshRate.Denominator = 1;
                }
                bufferDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                bufferDescription.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
                bufferDescription.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
            }
            {
                DXGI_SAMPLE_DESC& multiSamplingDescription = swapChainDescription.SampleDesc;

                multiSamplingDescription.Count = 1;
                multiSamplingDescription.Quality = 0;    // Anti-aliasing is disabled
            }
            swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDescription.BufferCount = 1;
            swapChainDescription.OutputWindow = g_context.windowBeingRenderedTo;
            swapChainDescription.Windowed = TRUE;
            swapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            swapChainDescription.Flags = 0;
        }
        D3D_FEATURE_LEVEL highestSupportedFeatureLevel;
        const auto d3DResult = D3D11CreateDeviceAndSwapChain( useDefaultAdapter, useHardwareRendering, dontUseSoftwareRendering,
            flags, useDefaultFeatureLevels, requestedFeatureLevelCount, sdkVersion, &swapChainDescription,
            &g_context.swapChain, &g_context.direct3DDevice, &highestSupportedFeatureLevel, &g_context.direct3DImmediateContext );
        if ( SUCCEEDED( d3DResult ) )
        {
            return eae6320::Results::success;
        }
        else
        {
            EAE6320_ASSERT( false );
            eae6320::Logging::OutputError( "Direct3D failed to create a Direct3D11 device with HRESULT %#010x", d3DResult );
            return eae6320::Results::Failure;
        }
    }

    eae6320::cResult InitializeViews(const unsigned int i_resolutionWidth, const unsigned int i_resolutionHeight)
    {
        auto result = eae6320::Results::success;

        ID3D11Texture2D* depthBuffer = nullptr;

        auto& g_context = eae6320::Graphics::sContext::g_context;
        auto* const direct3DDevice = g_context.direct3DDevice;
        EAE6320_ASSERT(direct3DDevice);
        auto* const direct3DImmediateContext = g_context.direct3DImmediateContext;
        EAE6320_ASSERT(direct3DImmediateContext);

        // Create a "render target view" of the back buffer
        // (the back buffer was already created by the call to D3D11CreateDeviceAndSwapChain(),
        // but a "view" of it is required to use as a "render target",
        // meaning a texture that the GPU can render to)
        {
            // Get the back buffer from the swap chain
            {
                constexpr unsigned int bufferIndex = 0;    // This must be 0 since the swap chain is discarded
                const auto d3DResult = g_context.swapChain->GetBuffer(bufferIndex, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&g_context.backBuffer));
                if (FAILED(d3DResult))
                {
                    result = eae6320::Results::Failure;
                    EAE6320_ASSERTF(false, "Couldn't get the back buffer from the swap chain (HRESULT %#010x)", d3DResult);
                    eae6320::Logging::OutputError("Direct3D failed to get the back buffer from the swap chain (HRESULT %#010x)", d3DResult);
                    goto OnExit;
                }
            }
            // Create the view
            {
                constexpr D3D11_RENDER_TARGET_VIEW_DESC* const accessAllSubResources = nullptr;
                const auto d3DResult = direct3DDevice->CreateRenderTargetView(g_context.backBuffer, accessAllSubResources, &g_context.renderTargetView);
                if (FAILED(d3DResult))
                {
                    result = eae6320::Results::Failure;
                    EAE6320_ASSERTF(false, "Couldn't create render target view (HRESULT %#010x)", d3DResult);
                    eae6320::Logging::OutputError("Direct3D failed to create the render target view (HRESULT %#010x)", d3DResult);
                    goto OnExit;
                }
            }
        }
        // Create a depth/stencil buffer and a view of it
        {
            // Unlike the back buffer no depth/stencil buffer exists until and unless it is explicitly created
            {
                D3D11_TEXTURE2D_DESC textureDescription{};
                {
                    textureDescription.Width = i_resolutionWidth;
                    textureDescription.Height = i_resolutionHeight;
                    textureDescription.MipLevels = 1;    // A depth buffer has no MIP maps
                    textureDescription.ArraySize = 1;
                    textureDescription.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;    // 24 bits for depth and 8 bits for stencil
                    {
                        DXGI_SAMPLE_DESC& sampleDescription = textureDescription.SampleDesc;

                        sampleDescription.Count = 1;    // No multisampling
                        sampleDescription.Quality = 0;    // Doesn't matter when Count is 1
                    }
                    textureDescription.Usage = D3D11_USAGE_DEFAULT;    // Allows the GPU to write to it
                    textureDescription.BindFlags = D3D11_BIND_DEPTH_STENCIL;
                    textureDescription.CPUAccessFlags = 0;    // CPU doesn't need access
                    textureDescription.MiscFlags = 0;
                }
                // The GPU renders to the depth/stencil buffer and so there is no initial data
                // (like there would be with a traditional texture loaded from disk)
                constexpr D3D11_SUBRESOURCE_DATA* const noInitialData = nullptr;
                const auto d3DResult = direct3DDevice->CreateTexture2D(&textureDescription, noInitialData, &depthBuffer);
                if (FAILED(d3DResult))
                {
                    result = eae6320::Results::Failure;
                    EAE6320_ASSERTF(false, "Couldn't create depth buffer (HRESULT %#010x)", d3DResult);
                    eae6320::Logging::OutputError("Direct3D failed to create the depth buffer resource (HRESULT %#010x)", d3DResult);
                    goto OnExit;
                }
            }
            // Create the view
            {
                constexpr D3D11_DEPTH_STENCIL_VIEW_DESC* const noSubResources = nullptr;
                const auto d3DResult = direct3DDevice->CreateDepthStencilView(depthBuffer, noSubResources, &g_context.depthStencilView);
                if (FAILED(d3DResult))
                {
                    result = eae6320::Results::Failure;
                    EAE6320_ASSERTF(false, "Couldn't create depth stencil view (HRESULT %#010x)", d3DResult);
                    eae6320::Logging::OutputError("Direct3D failed to create the depth stencil view (HRESULT %#010x)", d3DResult);
                    goto OnExit;
                }
            }
        }

        // Bind the views
        {
            constexpr unsigned int renderTargetCount = 1;
            direct3DImmediateContext->OMSetRenderTargets(renderTargetCount, &g_context.renderTargetView, g_context.depthStencilView);
        }
        // Specify that the entire render target should be visible
        {
            D3D11_VIEWPORT viewPort{};
            {
                viewPort.TopLeftX = viewPort.TopLeftY = 0.0f;
                viewPort.Width = static_cast<float>(i_resolutionWidth);
                viewPort.Height = static_cast<float>(i_resolutionHeight);
                viewPort.MinDepth = 0.0f;
                viewPort.MaxDepth = 1.0f;
            }
            constexpr unsigned int viewPortCount = 1;
            direct3DImmediateContext->RSSetViewports(viewPortCount, &viewPort);
        }

    OnExit:

        // Regardless of success or failure we release the depth buffer, the back buffer is used for screenshot ability later if requested
        // (if successful the views will hold internal references to the resources)
        if (depthBuffer)
        {
            depthBuffer->Release();
            depthBuffer = nullptr;
        }

        return result;
    }
}
