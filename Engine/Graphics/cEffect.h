#ifndef EAE6320_GRAPHICS_CEFFECT_H
#define EAE6320_GRAPHICS_CEFFECT_H

// Include Files
//==============

#include "cShader.h"
#include "cRenderState.h"

// Class Declaration
//==================

namespace eae6320
{
	namespace Graphics
	{
		class cEffect
		{
			// Interface
			//==========

		public:

			// Initialization / Clean Up
			//--------------------------

			cResult Initialize();
			cResult CleanUp();

			// Render
			//-------

			void Bind() const;

			// Data
			//=====

		private:

			cShader::Handle m_vertexShader;
			cShader::Handle m_fragmentShader;
			cRenderState m_renderState;
#if defined( EAE6320_PLATFORM_GL )
			GLuint m_programId = 0;
#endif

			// Implementation
			//===============

		private:

			// Initialization / Clean Up
			//--------------------------

#if defined( EAE6320_PLATFORM_GL )
			cResult InitializeGL();
#endif

		};
	}
}

#endif	// EAE6320_GRAPHICS_CEFFECT_H
