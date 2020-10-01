#pragma once
#include <vector>
#include <DirectXMath.h>

class MeshRenderer;
class Camera;

class RenderManager
{
public:
	RenderManager() {}
	~RenderManager();

	void Flush();

	void Draw(const DirectX::XMMATRIX & projectionMatrix, const DirectX::XMMATRIX & viewMatrix);

	void AddFoliageObject(MeshRenderer* mr);
	void AddOpaqueObject(MeshRenderer* mr);
		
	void AddOpaqueInstantiatedObject(MeshRenderer* mr);
	void ClearInstanciatedOpaqueObjects();

	void DrawFoliage(const DirectX::XMMATRIX & projectionMatrix, const DirectX::XMMATRIX & viewMatrix);
	void DrawOpaque(const DirectX::XMMATRIX & projectionMatrix, const DirectX::XMMATRIX & viewMatrix);

	void SetGameCamera(Camera* cam) { m_pGameCamera = cam; }

	void RemoveOpaqueInstance(MeshRenderer* mr);

private:

	Camera* m_pGameCamera = nullptr;

	std::vector<MeshRenderer*> m_opaqueObjects;
	std::vector<MeshRenderer*> m_opaqueInstanciatedObjects;
	std::vector<MeshRenderer*> m_foliageObjects;
	//Entity* m_sky;
};