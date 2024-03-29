/*
    The graphics "context" holds platform-specific graphics state

    Only a single instance of a context can exist,
    and it is globally accessible.
*/

#ifndef EAE6320_GRAPHICS_SCONTEXT_H
#define EAE6320_GRAPHICS_SCONTEXT_H

// Include Files
//==============

#include <Engine/Results/Results.h>
#include "ColorFormats.h"

#if defined( EAE6320_PLATFORM_WINDOWS )
#include <Engine/Windows/Includes.h>

#if defined( EAE6320_PLATFORM_GL )
#include "OpenGL/Includes.h"
#endif
#endif

// Forward Declarations
//=====================

#if defined( EAE6320_PLATFORM_D3D )
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11Texture2D;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
#endif

namespace eae6320
{
    namespace Graphics
    {
        struct sInitializationParameters;
        namespace ColorFormats
        {
            struct sColor;
        }
    }
}

namespace eae6320
{
    namespace Platform
    {
        struct sDataFromFile;
    }
}

// Class Declaration
//==================

namespace eae6320
{
    namespace Graphics
    {
        struct sContext
        {
            // Data
            //=====

#if defined( EAE6320_PLATFORM_WINDOWS )
            HWND windowBeingRenderedTo = nullptr;
#endif

#if defined( EAE6320_PLATFORM_D3D )
            // The device is an interface representing a graphics device (i.e. a graphics card)
            ID3D11Device* direct3DDevice = nullptr;
            // A device's immediate context can only be used by the main/render thread
            // (it is not thread safe)
            ID3D11DeviceContext* direct3DImmediateContext = nullptr;
            // A swap chain is like an array (a "chain") of textures
            // that are rendered to in sequence,
            // with a single one being currently displayed
            IDXGISwapChain* swapChain = nullptr;
            // The back buffer which is implicily created when the swap chain is created
            ID3D11Texture2D* backBuffer = nullptr;
            // In Direct3D "views" are objects that allow a texture to be used a particular way:
            // A render target view allows a texture to have color rendered to it
            ID3D11RenderTargetView* renderTargetView = nullptr;
            // A depth/stencil view allows a texture to have depth rendered to it
            ID3D11DepthStencilView* depthStencilView = nullptr;
#elif defined( EAE6320_PLATFORM_GL )
            HDC deviceContext = nullptr;
            HGLRC openGlRenderingContext = nullptr;
            uint16_t resolutionWidth = 0;
            uint16_t resolutionHeight = 0;
#endif

            // Interface
            //==========

            // Access
            //-------

            static sContext g_context;

            // Initialization / Clean Up
            //--------------------------

            cResult Initialize(const sInitializationParameters& i_initializationParameters);
            cResult CleanUp();

            ~sContext();

            // Render
            //-------

            void ClearImageBuffer(const ColorFormats::sColor i_color) const;
            void ClearDepthBuffer(const float i_depth) const;
            void BufferSwap() const;

            // User actions
            //-------------
            void GetRawImageFromBackBuffer(Platform::sDataFromFile& o_rawImageData) const;

            // Implementation
            //===============

        private:

            sContext() = default;
        };
    }
}

#endif    // EAE6320_GRAPHICS_SCONTEXT_H
