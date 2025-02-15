/*
 *  Copyright 2019-2022 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

#include "WinHPreface.h"
#include <Windows.h>
#include "WinHPostface.h"

#include "UnityGraphicsEmulator.h"
#include "UnityAppBase.h"
#include "IUnityInterface.h"
#include "Errors.hpp"

HMODULE g_DLLHandle;

class UnityAppWin32 : public UnityAppBase
{
public:
    virtual bool OnWindowCreated(HWND hWnd, LONG WindowWidth, LONG WindowHeight) override final
    {
        try
        {
            InitGraphics(hWnd, WindowWidth, WindowHeight);
            InitScene();
        }
        catch (...)
        {
            LOG_ERROR_MESSAGE("Failed to initialize Unity scene");
            return false;
        }
        return true;
    }
};

namespace Diligent
{

NativeAppBase* CreateApplication()
{
    return new UnityAppWin32();
}

} // namespace Diligent

void* UnityAppBase::LoadPluginFunction(const char* FunctionName)
{
    auto Func = GetProcAddress(g_DLLHandle, FunctionName);
    VERIFY(Func != nullptr, "Failed to import plugin function \"", FunctionName, "\".");
    return reinterpret_cast<void*>(Func);
}

bool UnityAppBase::LoadPlugin()
{
    std::string LibName = m_Scene->GetPluginName();
#if _WIN64
    LibName += "_64";
#else
    LibName += "_32";
#endif

#ifdef DILIGENT_DEBUG
    LibName += "d";
#else
    LibName += "r";
#endif

    LibName += ".dll";
    g_DLLHandle = LoadLibraryA(LibName.c_str());
    if (g_DLLHandle == NULL)
    {
        LOG_ERROR_MESSAGE("Failed to load ", LibName, " library.");
        return false;
    }

    UnityPluginLoad    = reinterpret_cast<TUnityPluginLoad>(GetProcAddress(g_DLLHandle, "UnityPluginLoad"));
    UnityPluginUnload  = reinterpret_cast<TUnityPluginUnload>(GetProcAddress(g_DLLHandle, "UnityPluginUnload"));
    GetRenderEventFunc = reinterpret_cast<TGetRenderEventFunc>(GetProcAddress(g_DLLHandle, "GetRenderEventFunc"));
    if (UnityPluginLoad == nullptr || UnityPluginUnload == nullptr || GetRenderEventFunc == nullptr)
    {
        LOG_ERROR_MESSAGE("Failed to import plugin functions from ", LibName, " library.");
        FreeLibrary(g_DLLHandle);
        return false;
    }

    return true;
}

void UnityAppBase::UnloadPlugin()
{
    m_GraphicsEmulator->InvokeDeviceEventCallback(kUnityGfxDeviceEventShutdown);
    UnityPluginUnload();
    FreeLibrary(g_DLLHandle);
    g_DLLHandle = NULL;
}
