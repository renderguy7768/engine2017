// Include Files
//==============

#include "cExampleGame.h"

#include <Engine/Asserts/Asserts.h>
#include <Engine/Camera/Camera.h>
#include <Engine/Camera/cFirstPersonCamera.h>
#include <Engine/Camera/cbCamera.h>
#include <Engine/Gameobject/cGameobject2D.h>
#include <Engine/Gameobject/cGameobject3D.h>
#include <Engine/Graphics/Graphics.h>
#include <Engine/Logging/Logging.h>
#include <Engine/Platform/Platform.h>
#include <Engine/UserInput/UserInput.h>

#include <sstream>
#include <vector>

namespace
{
    std::vector<eae6320::Gameobject::cGameobject2D*> s_2D_GameObject;
    std::vector<eae6320::Gameobject::cGameobject3D*> s_3D_GameObject;
    auto s_isPaused = false;
    auto currentElapsedTime = 0.0f;
    size_t s_2D_GameObject_Size = 0;
    size_t s_3D_GameObject_Size = 0;
    std::string s_executableDirectory = "";
    auto s_wasExecutableDirectorySearched = false;
    auto s_takeNextFrameScreenShot = false;
}

// Inherited Implementation
//=========================

// Configuration
//--------------

const char* eae6320::cExampleGame::GetExecutableDirectory() const
{
    if (!s_wasExecutableDirectorySearched)
    {
        s_wasExecutableDirectorySearched = true;
        std::string errorMessage;
        if (!Platform::GetCurrentWorkingDirectory(s_executableDirectory, &errorMessage))
        {
            eae6320::Logging::OutputError(errorMessage.c_str());
        }
    }
    return s_executableDirectory.c_str();
}

// Run
//----

void eae6320::cExampleGame::UpdateBasedOnInput()
{
    // Is the user pressing the ESC key?
    if (UserInput::IsKeyPressed(UserInput::KeyCodes::ESCAPE))
    {
        // Exit the application
        const auto result = Exit(EXIT_SUCCESS);
        EAE6320_ASSERT(result);
    }

    // Is the user pressing the P key?
    if (UserInput::IsKeyPressedOnce(UserInput::KeyCodes::P))
    {
        s_isPaused = !s_isPaused;
        s_isPaused ? SetSimulationRate(0.0f) : SetSimulationRate(1.0f);
    }

    // Is the user pressing the L key?
    if (UserInput::IsKeyPressedOnce(UserInput::KeyCodes::L))
    {
        s_takeNextFrameScreenShot = true;
    }
}

void eae6320::cExampleGame::UpdateBasedOnTime(const float i_elapsedSecondCount_sinceLastUpdate)
{
    currentElapsedTime += i_elapsedSecondCount_sinceLastUpdate;
    if (currentElapsedTime > 1.0f)
    {
        {
            auto const& gameObject2D = s_2D_GameObject[0];
            gameObject2D->m_useAlternateTexture = !gameObject2D->m_useAlternateTexture;
        }
        {
            auto const& gameObject2D = s_2D_GameObject[1];
            gameObject2D->m_useAlternateTexture = !gameObject2D->m_useAlternateTexture;
        }
        currentElapsedTime = 0.0f;
    }
}

void eae6320::cExampleGame::UpdateSimulationBasedOnInput()
{
    // Change current camera
    {
        Camera::ChangeCurrentCamera();
    }

    //Update current camera
    {
        Camera::GetCurrentCamera()->UpdateOrientation();
        Camera::GetCurrentCamera()->UpdatePosition();
    }

    // Update 3D Gameobjects
    {
        for (size_t i = 0; i < s_3D_GameObject_Size; i++)
        {
            s_3D_GameObject[i]->UpdateBasedOnSimulationInput();
        }
    }
}
void eae6320::cExampleGame::UpdateSimulationBasedOnTime(const float i_elapsedSecondCount_sinceLastUpdate)
{

    //Update current camera
    {
        Camera::GetCurrentCamera()->UpdateOrientation(i_elapsedSecondCount_sinceLastUpdate);
        Camera::GetCurrentCamera()->UpdatePosition(i_elapsedSecondCount_sinceLastUpdate);
    }

    // Update 3D Gameobjects
    {
        for (size_t i = 0; i < s_3D_GameObject_Size; i++)
        {
            s_3D_GameObject[i]->UpdateBasedOnSimulationTime(i_elapsedSecondCount_sinceLastUpdate);
        }
    }
}

void eae6320::cExampleGame::SubmitDataToBeRendered(const float i_elapsedSecondCount_systemTime, const float i_elapsedSecondCount_sinceLastSimulationUpdate)
{
    // Predict Camera
    {
        Camera::GetCurrentCamera()->PredictOrientation(i_elapsedSecondCount_sinceLastSimulationUpdate);
        Camera::GetCurrentCamera()->PredictPosition(i_elapsedSecondCount_sinceLastSimulationUpdate);
    }

    // Predict 3D Gameobjects
    {
        for (size_t i = 0; i < s_3D_GameObject_Size; i++)
        {
            s_3D_GameObject[i]->PredictSimulationBasedOnElapsedTime(i_elapsedSecondCount_sinceLastSimulationUpdate);
        }
    }

    // Submit Clear Color
    {
        /*Graphics::ColorFormats::sColor clearColor;
        if (!clearColor.SetColor(0.0f, 0.0f, 0.0f))
        {
            EAE6320_ASSERT(false);
            Logging::OutputError("All the SetColor parameters must be [0.0, 1.0]");
        }*/
        Graphics::SubmitClearColor();
    }

    // Submit Clear Depth
    {
        //constexpr auto depth = 1.0f;
        Graphics::SubmitClearDepth();
    }

    // Submit Current Camera
    {
        Graphics::SubmitCamera(Camera::GetCurrentCamera());
    }

    // Submit 3D Gameobjects
    {
        for (size_t i = 0; i < s_3D_GameObject_Size; i++)
        {
            Graphics::SubmitGameobject3D(s_3D_GameObject[i]);
        }
    }

    // Submit 2D Gameobjects
    {
        for (size_t i = 0; i < s_2D_GameObject_Size; i++)
        {
            Graphics::SubmitGameobject2D(s_2D_GameObject[i]);
        }
    }

    // Generate and submit screenshot name if requested
    {
        if (s_takeNextFrameScreenShot)
        {
            s_takeNextFrameScreenShot = false;

            constexpr char* const screenShotPrefix = "Screenshot_";
            constexpr char* const screenShotExt = ".png";

            std::stringstream screenShotFileName;
            screenShotFileName << screenShotPrefix << GetCurrentSystemTime() << screenShotExt;

            const std::string exeDir = GetExecutableDirectory();

            if (!exeDir.empty())
            {
                std::stringstream screenShotPath;
                constexpr char* const screenShotDirectory = "\\Screenshots\\";
                screenShotPath << exeDir << screenShotDirectory;
                std::string errorMessage;
                if (Platform::CreateDirectoryIfItDoesntExist(screenShotPath.str(), &errorMessage))
                {
                    screenShotPath << screenShotFileName.str();
                    Graphics::SubmitScreenShotName(screenShotPath.str());
                }
                else
                {
                    eae6320::Logging::OutputError("Unable to create screenshot directory: \"%s\", screenshots will be saved in the root directory of the executable", errorMessage.c_str());
                    Graphics::SubmitScreenShotName(screenShotFileName.str());
                }
            }
            else
            {
                Graphics::SubmitScreenShotName(screenShotFileName.str());
            }
        }
    }
}

// Initialization / Clean Up
//--------------------------

eae6320::cResult eae6320::cExampleGame::Initialize()
{
    cResult result;
    // Create a camera
    {
        AddNewCamera(reinterpret_cast<Camera::cbCamera*>(Camera::cFirstPersonCamera::Initialize(Math::sVector(0.0f, 0.0f, 10.0f))));
    }
    // Creating all 3D gameobjects
    {
        {
            Gameobject::cGameobject3D* gameobject3D;
            if (!((result = Gameobject::cGameobject3D::Load("fake_go3d1_path", gameobject3D, Math::sVector(0.0f, 0.0f, 0.0f), "data/Meshes/ball.bmf", "data/Materials/ball.bmaf", Gameplay::DEFAULT_GAMEOBJECT_CONTROLLER))))
            {
                EAE6320_ASSERT(false);
                goto OnExit;
            }
            s_3D_GameObject.push_back(gameobject3D);
        }
        {
            Gameobject::cGameobject3D* gameobject3D;
            if (!((result = Gameobject::cGameobject3D::Load("fake_go3d2_path", gameobject3D, Math::sVector(0.0f, -2.0f, 0.0f), "data/Meshes/floor.bmf", "data/Materials/floor.bmaf", Gameplay::NO_CONTROLLER))))
            {
                EAE6320_ASSERT(false);
                goto OnExit;
            }
            s_3D_GameObject.push_back(gameobject3D);
        }
        {
            Gameobject::cGameobject3D* gameobject3D;
            if (!((result = Gameobject::cGameobject3D::Load("fake_go3d4_path", gameobject3D, Math::sVector(0.0f, -2.0f, 4.0f), "data/Meshes/capsule.bmf", "data/Materials/capsule2.bmaf", Gameplay::NO_CONTROLLER))))
            {
                EAE6320_ASSERT(false);
                goto OnExit;
            }
            s_3D_GameObject.push_back(gameobject3D);
        }
        {
            Gameobject::cGameobject3D* gameobject3D;
            if (!((result = Gameobject::cGameobject3D::Load("fake_go3d3_path", gameobject3D, Math::sVector(2.0f, -2.0f, 0.0f), "data/Meshes/capsule.bmf", "data/Materials/capsule.bmaf", Gameplay::NO_CONTROLLER))))
            {
                EAE6320_ASSERT(false);
                goto OnExit;
            }
            s_3D_GameObject.push_back(gameobject3D);
        }

        /*{
            Gameobject::cGameobject3D* gameobject3D;
            if (!((result = Gameobject::cGameobject3D::Load("fake_go3d3_path", gameobject3D, Math::sVector(0.0f, 0.0f, -10.0f), "data/Meshes/dino.bmf", "data/Materials/dino.bmaf", Gameplay::DEFAULT_GAMEOBJECT_CONTROLLER))))
            {
                EAE6320_ASSERT(false);
                goto OnExit;
            }
            s_3D_GameObject.push_back(gameobject3D);
        }*/
    }
    // Creating all 2D gameobjects
    {
        {
            Gameobject::cGameobject2D* gameobject2D;
            if (!((result = Gameobject::cGameobject2D::Load("fake_go2d1_path", gameobject2D, 0, 0, 128, 128, Transform::TOP_LEFT, "data/Effects/sprite.bef", "data/Textures/Sprites/happy.btf", "data/Textures/Sprites/smiling.btf"))))
            {
                EAE6320_ASSERT(false);
                goto OnExit;
            }
            s_2D_GameObject.push_back(gameobject2D);
        }
        {
            Gameobject::cGameobject2D* gameobject2D;
            if (!((result = Gameobject::cGameobject2D::Load("fake_go2d2_path", gameobject2D, 0, 0, 128, 128, Transform::BOTTOM_RIGHT, "data/Effects/sprite.bef", "data/Textures/Sprites/sad.btf", "data/Textures/Sprites/happy.btf"))))
            {
                EAE6320_ASSERT(false);
                goto OnExit;
            }
            s_2D_GameObject.push_back(gameobject2D);
        }
    }

OnExit:
    s_2D_GameObject_Size = s_2D_GameObject.size();
    s_3D_GameObject_Size = s_3D_GameObject.size();
    return result;
}

eae6320::cResult eae6320::cExampleGame::CleanUp()
{
    // Clean up cameras
    {
        Camera::CleanUp();
    }

    // Clean up 2d gameobject
    {
        for (size_t i = 0; i < s_2D_GameObject_Size; i++)
        {
            s_2D_GameObject[i]->DecrementReferenceCount();
        }
        s_2D_GameObject.clear();
    }

    // Clean up 3d gameobject
    {
        for (size_t i = 0; i < s_3D_GameObject_Size; i++)
        {
            s_3D_GameObject[i]->DecrementReferenceCount();
        }
        s_3D_GameObject.clear();
    }

    // Reset all globals
    {
        s_isPaused = false;
        currentElapsedTime = 0.0f;
        s_2D_GameObject_Size = 0;
        s_3D_GameObject_Size = 0;
        s_executableDirectory = "";
        s_wasExecutableDirectorySearched = false;
        s_takeNextFrameScreenShot = false;
    }

    return Results::success;
}
