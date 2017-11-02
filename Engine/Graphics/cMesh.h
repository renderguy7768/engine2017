/*
A mesh is a 3d representation of an entity on screen
*/

#ifndef EAE6320_GRAPHICS_CMESH_H
#define EAE6320_GRAPHICS_CMESH_H

// Include Files
//==============

#include <Engine/Assets/ReferenceCountedAssets.h>
#include <Engine/Results/Results.h>
#include <Engine/Graphics/Configuration.h>

#ifdef EAE6320_PLATFORM_GL
#include "OpenGL/Includes.h"
#endif

// Forward Declarations
//=====================

#if defined( EAE6320_PLATFORM_D3D )
struct ID3D11Buffer;
struct ID3D11InputLayout;
#endif

namespace eae6320
{
	namespace Graphics
	{
		namespace HelperStructs 
		{
			struct sMeshData;
		}
	}
}

// Class Declaration
//==================

namespace eae6320
{
	namespace Graphics
	{
		class cMesh
		{
			// Interface
			//==========

		public:

			// Initialization / Clean Up
			//--------------------------

			static cResult Load(cMesh*& o_mesh, const HelperStructs::sMeshData& i_meshData);

			EAE6320_ASSETS_DECLAREDELETEDREFERENCECOUNTEDFUNCTIONS(cMesh);

			// Reference Counting
			//-------------------

			EAE6320_ASSETS_DECLAREREFERENCECOUNTINGFUNCTIONS();

			// Render
			//-------

			void Draw() const;

			// Data
			//=====

		private:
			uint32_t m_numberOfIndices;
#if defined( EAE6320_PLATFORM_D3D )
			// A vertex buffer holds the data for each vertex
			ID3D11Buffer* m_vertexBuffer = nullptr;
			// An index buffer holds the indices for vertex data in correct order
			ID3D11Buffer* m_indexBuffer = nullptr;
			// D3D has an "input layout" object that associates the layout of the vertex format struct
			// with the input from a vertex shader
			ID3D11InputLayout* m_vertexInputLayout = nullptr;
#elif defined( EAE6320_PLATFORM_GL )
			// A vertex array encapsulates the vertex data, index data and the vertex input layout
			GLuint m_vertexArrayId;
#ifdef EAE6320_GRAPHICS_ISDEVICEDEBUGINFOENABLED
			// OpenGL debuggers don't seem to support freeing the vertex buffer
			// and letting the vertex array object hold a reference to it,
			// and so if debug info is enabled an explicit reference is held

			// A vertex buffer holds the data for each vertex
			GLuint m_vertexBufferId;
			// An index buffer holds the indices for vertex data in correct order
			GLuint m_indexBufferId;
#endif
#endif
			EAE6320_ASSETS_DECLAREREFERENCECOUNT();
			bool m_isIndexing16Bit;

			// Implementation
			//===============

			// Initialization / Clean Up
			//--------------------------

			cMesh() = default;
			~cMesh();

			cResult Initialize(const HelperStructs::sMeshData& i_meshData);
			cResult CleanUp();
		};
	}
}

#endif	// EAE6320_GRAPHICS_CMESH_H