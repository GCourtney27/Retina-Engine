#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include "Color.h"
#include <assimp/material.h>

enum class TextureStorageType
{
	Invalid,
	None,
	EmbeddedIndexCompressed,
	EmbeddedIndexNonCompressed,
	EmbeddedCompressed,
	EmbeddedNonCompressed,
	Disk
};

class Texture
{
public:
	Texture() {}
	Texture(ID3D11Device * device, const Color & color, aiTextureType type);
	Texture(ID3D11Device * device, const Color * colorData, UINT width, UINT height, aiTextureType type);
	Texture(ID3D11Device * device, const std::string & filePath, aiTextureType type);
	Texture(ID3D11Device * device, const std::string & filePath);
	Texture(ID3D11Device * device, const uint8_t* pData, size_t size, aiTextureType type);

	void Initialize(ID3D11Device * device, const std::string & filePath);

	aiTextureType GetType();
	ID3D11ShaderResourceView * GetTextureResourceView();
	ID3D11ShaderResourceView ** GetTextureResourceViewAddress();

	ID3D11Resource * GetTexture();

private:
	void Initialize1x1ColorTexture(ID3D11Device * device, const Color & colorData, aiTextureType type);
	void InitializeColorTexture(ID3D11Device * device, const Color * colorData, UINT width, UINT height, aiTextureType type);
	Microsoft::WRL::ComPtr<ID3D11Resource>texture = nullptr;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureView = nullptr;
	aiTextureType type = aiTextureType::aiTextureType_UNKNOWN;
};