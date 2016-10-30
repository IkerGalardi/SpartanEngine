/*
Copyright(c) 2016 Panos Karabelas

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

//= INCLUDES ===============================
#include "Socket.h"
#include "../Pools/GameObjectPool.h"
#include "../Logging/Log.h"
#include "../Components/MeshRenderer.h"
#include "../Graphics/Renderer.h"
#include "../Core/Settings.h"
#include "../Signals/Signaling.h"
#include "../FileSystem/ModelImporter.h"
#include "../Pools/TexturePool.h"
#include "../Core/Engine.h"
//==========================================

//= NAMESPACES =====
using namespace std;
//==================

Socket::Socket(Context* context) : Object(context)
{

}

Socket::~Socket()
{
}

//= STATE CONTROL ==============================================================
void Socket::StartEngine() const
{
	SET_ENGINE_MODE(Editor_Playing);
	EMIT_SIGNAL(SIGNAL_ENGINE_START);
}

void Socket::StopEngine()
{
	SET_ENGINE_MODE(Editor_Idle);
	EMIT_SIGNAL(SIGNAL_ENGINE_STOP);
}

void Socket::Update() const
{
	g_context->GetSubsystem<Engine>()->Update();
}

//=============================================================================

//= IO ========================================================================
void Socket::LoadModel(const string& filePath)
{
	g_context->GetSubsystem<ModelImporter>()->Load(new GameObject(), filePath);
}

void Socket::LoadModelAsync(const string& filePath)
{
	g_context->GetSubsystem<ModelImporter>()->LoadAsync(new GameObject(), filePath);
}

void Socket::SaveSceneToFileAsync(const string& filePath)
{
	return g_context->GetSubsystem<Scene>()->SaveToFileAsync(filePath);
}

void Socket::LoadSceneFromFileAsync(const string& filePath)
{
	return g_context->GetSubsystem<Scene>()->LoadFromFileAsync(filePath);
}

bool Socket::SaveSceneToFile(const string& filePath)
{
	return g_context->GetSubsystem<Scene>()->SaveToFile(filePath);
}

bool Socket::LoadSceneFromFile(const string& filePath)
{
	return g_context->GetSubsystem<Scene>()->LoadFromFile(filePath);
}
//==============================================================================

//= GRAPHICS ===================================================================
void Socket::SetViewport(int width, int height) const
{
	g_context->GetSubsystem<Renderer>()->SetResolution(width, height);
}
//==============================================================================

//= MISC =======================================================================
void Socket::SetPhysicsDebugDraw(bool enable)
{
	//m_renderer->SetPhysicsDebugDraw(enable);
}

PhysicsDebugDraw* Socket::GetPhysicsDebugDraw()
{
	return g_context->GetSubsystem<PhysicsWorld>()->GetPhysicsDebugDraw();
}

void Socket::ClearScene()
{
	g_context->GetSubsystem<Scene>()->Clear();
}

ImageImporter* Socket::GetImageLoader()
{
	return &ImageImporter::GetInstance();
}

void Socket::SetLogger(weak_ptr<ILogger> logger)
{
	Log::SetLogger(logger);
}

//==============================================================================

//= GAMEOBJECTS ================================================================
vector<GameObject*> Socket::GetAllGameObjects()
{
	return GameObjectPool::GetInstance().GetAllGameObjects();
}

vector<GameObject*> Socket::GetRootGameObjects()
{
	return GameObjectPool::GetInstance().GetRootGameObjects();
}

GameObject* Socket::GetGameObjectByID(string gameObjectID)
{
	return GameObjectPool::GetInstance().GetGameObjectByID(gameObjectID);
}

int Socket::GetGameObjectCount()
{
	return GameObjectPool::GetInstance().GetGameObjectCount();
}

void Socket::DestroyGameObject(GameObject* gameObject)
{
	if (!gameObject)
		return;

	GameObjectPool::GetInstance().RemoveGameObject(gameObject);
}

bool Socket::GameObjectExists(GameObject* gameObject)
{
	if (!gameObject) 
		return false;

	bool exists = GameObjectPool::GetInstance().GameObjectExists(gameObject);

	return exists;
}
//==============================================================================

//= STATS ======================================================================
float Socket::GetFPS() const
{
	return g_context->GetSubsystem<Timer>()->GetFPS();
}

int Socket::GetRenderedMeshesCount() const
{
	return g_context->GetSubsystem<Renderer>()->GetRenderedMeshesCount();
}

float Socket::GetDeltaTime() const
{
	return g_context->GetSubsystem<Timer>()->GetDeltaTimeMs();
}

float Socket::GetRenderTime() const
{
	return g_context->GetSubsystem<Timer>()->GetRenderTimeMs();
}
//==============================================================================

void Socket::SetMaterialTexture(GameObject* gameObject, TextureType type, string texturePath)
{
	LOG_INFO("1");

	if (!gameObject)
		return;

	LOG_INFO("2");

	auto meshRenderer = gameObject->GetComponent<MeshRenderer>();
	if (!meshRenderer)
		return;

	shared_ptr<Material> material = meshRenderer->GetMaterial();
	if (material)
	{
		LOG_INFO("3");
		// Load the texture, if the texture has already been loaded, the pool will return the existing one
		weak_ptr<Texture> texture = g_context->GetSubsystem<TexturePool>()->Add(texturePath);
		texture.lock()->SetType(type);

		// Set it to the material
		material->SetTextureByID(texture.lock()->GetID());
	}
	LOG_INFO("4");
}
