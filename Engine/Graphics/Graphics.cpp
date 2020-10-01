#include "Graphics.h"
#include "..\Systems\FileSystem.h"
#include "..\Input\InputManager.h"
#include "..\Systems\BenchmarkingTimer.h"

#include "MaterialTexturedFoliage.h"
#include "MaterialTextured.h"
#include "..\Components\RigidBodyComponent.h"

#include <cstdlib>
#include <math.h>

#define PI 3.14159265

bool Graphics::Initialize(HWND hwnd, int width, int height, Engine* engine)
{
	windowWidth = width;
	windowHeight = height;
	m_pEngine = engine;

	if (!InitializeDirectX(hwnd))
		return false;


	if (!InitializeShaders())
		return false;

	pointLight = new PointLight(&(m_pEngine->GetScene()), *(new ID("Point Light")));
	pointLight->GetTransform().SetPosition(DirectX::XMFLOAT3(0.0f, 32.0f, -50.0f));
	pointLight->GetTransform().SetRotation(0.0f, 0.0f, 0.0f);
	pointLight->GetTransform().SetScale(1.0f, 1.0f, 1.0f);

	MeshRenderer* mr = pointLight->AddComponent<MeshRenderer>();
	mr->Initialize(pointLight, "..\\Assets\\Objects\\Primatives\\Sphere.fbx", Graphics::Instance()->GetDevice(), Graphics::Instance()->GetDeviceContext(), Graphics::Instance()->GetDefaultVertexShader(), m_pMaterial);
	EditorSelection* es = pointLight->AddComponent<EditorSelection>();
	es->Initialize(pointLight, 1.0f, pointLight->GetTransform().GetPosition());

	directionalLight = new DirectionalLight(&(m_pEngine->GetScene()), *(new ID("Directional Light")));
	directionalLight->GetTransform().SetPosition(DirectX::XMFLOAT3(0.0f, 1000.0f, -100.0f));
	directionalLight->GetTransform().SetRotation(0.0f, 0.0f, 0.0f);
	directionalLight->GetTransform().SetScale(1.0f, 1.0f, 1.0f);

	MeshRenderer* mrd = directionalLight->AddComponent<MeshRenderer>();
	mrd->Initialize(directionalLight, "..\\Assets\\Objects\\Primatives\\Sphere.fbx", Graphics::Instance()->GetDevice(), Graphics::Instance()->GetDeviceContext(), Graphics::Instance()->GetDefaultVertexShader(), m_pMaterial);
	EditorSelection* esd = directionalLight->AddComponent<EditorSelection>();
	esd->Initialize(directionalLight, 1.0f, directionalLight->GetTransform().GetPosition());

	if (!InitializeScene())
		return false;
	
	InitSkybox();

	// Setup ImGui
	InitialzeImGui(hwnd);

	//backBufferTex.Initialize(pDevice.Get(), "..\\Assets\\Objects\\Norway\\Opaque\\Rock02\\Rock02_Albedo.jpg");
	HRESULT hr = DirectX::CreateWICTextureFromFile(pDevice.Get(), L"..\\Assets\\Objects\\Norway\\Opaque\\Rock02\\Rock02_Albedo.jpg", nullptr, &backBufferSRV);
	if (FAILED(hr))
		ErrorLogger::Log("Failed ot create SRV for back buffer.");

	return true;
}

void Graphics::InitialzeImGui(HWND hwnd)
{
	pImGuiIO = new ImGuiIO();
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	ImGui::StyleColorsDark();
	
	//ImGui::StyleColorsClassic();
	//ImGui::StyleColorsLight();
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(pDevice.Get(), pDeviceContext.Get());
	*pImGuiIO = io;
}

void Graphics::InitSkybox()
{
	/*m_pSkyMaterial = dynamic_cast<MaterialSky*>(skybox->GetComponent<MeshRenderer>()->GetModel()->GetMaterial());
	return;*/
	skybox = new Entity((&m_pEngine->GetScene()), *(new ID("Sky Box")));
	
	skybox->GetTransform().SetPosition(0.0f, 0.0f, 0.0f);
	skybox->GetTransform().SetScale(50000.0f, 50000.0f, 50000.0f);
	skybox->GetTransform().SetRotation(0.0f, 0.0f, 0.0f);
	MeshRenderer* me = skybox->AddComponent<MeshRenderer>();
	me->Initialize(skybox, "..\\Assets\\Objects\\Primatives\\Sphere.fbx", pDevice.Get(), pDeviceContext.Get(), cb_vs_vertexshader, nullptr);

	// MountainTop
	// NewportLoft
	// LowOrbit
	// skybox1
	// skybox2
	// skybox3
	std::wstring skyboxType = L"MountainTop";
	std::wstring diffuse = L"..\\Assets\\Textures\\Skyboxes\\" + skyboxType + L"_Diff.dds";
	std::wstring envMap = L"..\\Assets\\Textures\\Skyboxes\\" + skyboxType + L"_EnvMap.dds";
	std::wstring IR = L"..\\Assets\\Textures\\Skyboxes\\" + skyboxType + L"_IR.dds";

	HRESULT hr = DirectX::CreateDDSTextureFromFile(pDevice.Get(), diffuse.c_str(), nullptr, &skyboxTextureSRV);
	if(FAILED(hr))
		ErrorLogger::Log("Failed to load dds diffuse texture for skybox");

	hr = DirectX::CreateDDSTextureFromFile(pDevice.Get(), envMap.c_str(), nullptr, &environmentMapSRV);
	if (FAILED(hr))
		ErrorLogger::Log("Failed to load dds texture for environment map");

	hr = DirectX::CreateDDSTextureFromFile(pDevice.Get(), IR.c_str(), nullptr, &irradianceMapSRV);
	if (FAILED(hr))
		ErrorLogger::Log("Failed to load dds texture for irradiance map");

	hr = DirectX::CreateWICTextureFromFile(pDevice.Get(), L"..\\Assets\\Textures\\Skyboxes\\ibl_brdf_lut.png", nullptr, &brdfLUTSRV);
	if (FAILED(hr))
		ErrorLogger::Log("Failed to load dds texture for brdfLUT map");


}

bool Graphics::InitializeDirectX(HWND hwnd)
{
	try
	{
		std::vector<AdapterData> adapters = AdapterReader::GetAdapters();
		
		if (adapters.size() < 1)
		{
			ErrorLogger::Log("No DirectX compatable adapters where found when initializing Direct3D 11.");
			return false;
		}

		// -- Initialize Swap Chain -- //
		DXGI_SWAP_CHAIN_DESC scd = { 0 };
		scd.BufferDesc.Width = windowWidth;
		scd.BufferDesc.Height = windowHeight;
		scd.BufferDesc.RefreshRate.Numerator = 60;
		scd.BufferDesc.RefreshRate.Denominator = 1;
		scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		//scd.SampleDesc.Count = 1;
		scd.SampleDesc.Count = 4;// New
		scd.SampleDesc.Quality = 0;
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scd.BufferCount = 1;
		scd.OutputWindow = hwnd;
		scd.Windowed = TRUE;
		scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		
		HRESULT hr;
		hr = D3D11CreateDeviceAndSwapChain(adapters[1].pAdapter, //IDXGI Adapter: 0 for Intel gpu; 1 for NVIDIA
			D3D_DRIVER_TYPE_UNKNOWN,
			NULL, // NULL for software driver type
			NULL, // Flags for runtime layers
			NULL, // Feature levels array
			0, //Number of Feature levels in array
			D3D11_SDK_VERSION,
			&scd, // Swapchain desciption
			pSwapchain.GetAddressOf(), // Swapchain address
			pDevice.GetAddressOf(), // Device address
			NULL, //Supported feature level
			pDeviceContext.GetAddressOf()); // Device context address
		COM_ERROR_IF_FAILED(hr, "Failed to create device swap chain.");
		
		hr = pSwapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(pBackBuffer.GetAddressOf()));
		COM_ERROR_IF_FAILED(hr, "Failed to set buffer for swap chain.");

		hr = pDevice->CreateRenderTargetView(pBackBuffer.Get(), NULL, pRenderTargetView.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create render target view for back buffer.");


		// -- Create Depth/Stencil Buffer -- //
		CD3D11_TEXTURE2D_DESC depthStencilDesc(DXGI_FORMAT_D24_UNORM_S8_UINT, windowWidth, windowHeight);
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.SampleDesc.Count = 4;// New
		depthStencilDesc.SampleDesc.Quality = 0;// New
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		hr = pDevice->CreateTexture2D(&depthStencilDesc, NULL, pDepthStencilBuffer.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create depth stencil buffer Texture2D.");

		hr = pDevice->CreateDepthStencilView(pDepthStencilBuffer.Get(), NULL, pDepthStencilView.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create depth stencil view.");

		pDeviceContext->OMSetRenderTargets(1, pRenderTargetView.GetAddressOf(), pDepthStencilView.Get());
		

		// Create depth stencil state
		CD3D11_DEPTH_STENCIL_DESC depthstancildesc(D3D11_DEFAULT); // Z Buffer
		depthstancildesc.DepthFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL;
		hr = pDevice->CreateDepthStencilState(&depthstancildesc, pDepthStencilState.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Filaed to create depth stencil state.");


		// Create and set the ViewPort
		CD3D11_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight));
		pDeviceContext->RSSetViewports(1, &viewport);


		// Create Rasterizer State/s
		// Default
		CD3D11_RASTERIZER_DESC rasterizerDesc(D3D11_DEFAULT);
		rasterizerDesc.AntialiasedLineEnable = true;// New
		rasterizerDesc.MultisampleEnable = true;
		hr = pDevice->CreateRasterizerState(&rasterizerDesc, pRasterizerState.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create rasterizer state.");

		// Skybox and foliage
		CD3D11_RASTERIZER_DESC rasterizerDescCULLNONE(D3D11_DEFAULT);
		rasterizerDesc.AntialiasedLineEnable = true;// New
		rasterizerDesc.MultisampleEnable = true;
		rasterizerDescCULLNONE.CullMode = D3D11_CULL_MODE::D3D11_CULL_NONE; // Uncomment to draw both sides of the mesh
		hr = pDevice->CreateRasterizerState(&rasterizerDescCULLNONE, pRasterizerStateCULLNONE.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create rasterizer state.");

		D3D11_DEPTH_STENCIL_DESC dssDesc;
		ZeroMemory(&dssDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
		dssDesc.DepthEnable = true;
		dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dssDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		

		// Create Blend State
		D3D11_BLEND_DESC blendDesc = { 0 };
		D3D11_RENDER_TARGET_BLEND_DESC rtbd = { 0 };
		rtbd.BlendEnable = true;
		rtbd.SrcBlend = D3D11_BLEND::D3D11_BLEND_SRC_ALPHA;
		rtbd.DestBlend = D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA;
		rtbd.BlendOp = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
		rtbd.SrcBlendAlpha = D3D11_BLEND::D3D11_BLEND_ONE;
		rtbd.DestBlendAlpha = D3D11_BLEND::D3D11_BLEND_ZERO;
		rtbd.BlendOpAlpha = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
		rtbd.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALL;
		blendDesc.RenderTarget[0] = rtbd;
		hr = pDevice->CreateBlendState(&blendDesc, pBlendState.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create blend state.");

		pSpriteBatch = std::make_unique<DirectX::SpriteBatch>(pDeviceContext.Get());
		pSpriteFont = std::make_unique<DirectX::SpriteFont>(pDevice.Get(), L"..\\Assets\\Fonts\\calibri.spritefont");

		// Create sampler description for sampler state
		CD3D11_SAMPLER_DESC samplerDesc(D3D11_DEFAULT);
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		samplerDesc.BorderColor[0] = 0;
		samplerDesc.BorderColor[1] = 0;
		samplerDesc.BorderColor[2] = 0;
		samplerDesc.BorderColor[3] = 0;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
		hr = pDevice->CreateSamplerState(&samplerDesc, samplerState.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create sampler state.");

	}
	catch (COMException & exception)
	{
		ErrorLogger::Log(exception);
		return false;
	}
	return true;
}

void Graphics::RenderFrame()
{
	Debug::ScopedTimer timer;
	m_frameTimer.tick();

	if (Debug::Editor::Instance()->PlayingGame())
		m_pSelectedCamera = m_pEngine->GetPlayer()->GetPlayerCamera();
	else
		m_pSelectedCamera = &editorCamera;


	const DirectX::XMMATRIX & ProjMat = m_pSelectedCamera->GetProjectionMatrix();
	const DirectX::XMMATRIX & ViewMat = m_pSelectedCamera->GetViewMatrix();

	// -- These Constant Buffers dont get included in materials becasue they change on a per-scene basis -- //
#pragma region
	// -- Update Light Shader Information -- //
	cb_ps_light.data.dynamicLightColor = pointLight->lightColor;
	cb_ps_light.data.dynamicLightStrength = pointLight->lightStrength;
	cb_ps_light.data.dynamicLightPosition = pointLight->GetTransform().GetPosition();
	cb_ps_light.data.dynamicLightAttenuation_a = pointLight->attenuation_a;
	cb_ps_light.data.dynamicLightAttenuation_b = pointLight->attenuation_b;
	cb_ps_light.data.dynamicLightAttenuation_c = pointLight->attenuation_c;
	cb_ps_light.ApplyChanges();

	cb_ps_directionalLight.data.Color = directionalLight->lightColor;
	cb_ps_directionalLight.data.Strength = directionalLight->lightStrength;
	cb_ps_directionalLight.data.Direction = directionalLight->GetTransform().GetPosition();
	cb_ps_directionalLight.ApplyChanges();

	// -- Update Pixel Shader Per Frame Informaiton -- //
	cb_ps_PerFrame.data.deltaTime = m_deltaTime;
	if (Debug::Editor::Instance()->PlayingGame())
		cb_ps_PerFrame.data.camPosition = m_pEngine->GetPlayer()->GetPlayerCamera()->GetTransform().GetPosition();
	else
		cb_ps_PerFrame.data.camPosition = editorCamera.GetTransform().GetPosition();
	cb_ps_PerFrame.ApplyChanges();

	// -- Update Vertex Shader Per Frame Informaiton -- //
	cb_vs_PerFrame.data.deltaTime = m_deltaTime;
	time = (float)m_pEngine->GetFrameTimer().seconds();
	cb_vs_PerFrame.data.time = time;
	cb_vs_PerFrame.ApplyChanges();

	// -- Set Pixel Shader Constant Buffers -- //
	pDeviceContext->PSSetConstantBuffers(0, 1, cb_ps_light.GetAddressOf());
	pDeviceContext->PSSetConstantBuffers(1, 1, cb_ps_PerFrame.GetAddressOf());
	pDeviceContext->PSSetConstantBuffers(3, 1, cb_ps_directionalLight.GetAddressOf());

	// -- Set Vertex Shader Constant Buffers -- //
	pDeviceContext->VSSetConstantBuffers(1, 1, cb_vs_PerFrame.GetAddressOf());
#pragma endregion Per Scene

	// -- Start ImGui frame -- //
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
	ImGui::DockSpaceOverViewport(0, ImGuiDockNodeFlags_PassthruCentralNode);
	
	// -- Clear Background Color for Scene -- //
	float bgcolor[] = { 0.01f, 0.01f, 0.01f, 1.0f };
	pDeviceContext->ClearRenderTargetView(pRenderTargetView.Get(), bgcolor);
	pDeviceContext->ClearDepthStencilView(pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	if(m_drawWireframe)
		pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_LINELIST);
	else
		pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	

	// Set Rasterizer state to CULLNONE to draw skybox
	pDeviceContext->RSSetState(pRasterizerStateCULLNONE.Get());

	// -- Set Shader Samplers -- //
	pDeviceContext->PSSetSamplers(0, 1, samplerState.GetAddressOf());

	// -- Draw Skybox -- //
	pDeviceContext->IASetInputLayout(skyVertexShader.GetInputLayout());
	pDeviceContext->PSSetShaderResources(0, 1, &skyboxTextureSRV);
	pDeviceContext->PSSetShader(skyPixelShader.GetShader(), NULL, 0);
	pDeviceContext->VSSetShader(skyVertexShader.GetShader(), NULL, 0);
	skybox->Draw(ProjMat, ViewMat);
	
	// Reset Rasterizer state for rest of geometry
	pDeviceContext->RSSetState(pRasterizerState.Get());
	pDeviceContext->OMSetDepthStencilState(pDepthStencilState.Get(), 0);
	pDeviceContext->OMSetBlendState(pBlendState.Get(), NULL, 0xFFFFFFFF);

	// -- Set IBL resources for shader slots -- //
	pDeviceContext->PSSetShaderResources(5, 1, &irradianceMapSRV);
	pDeviceContext->PSSetShaderResources(6, 1, &environmentMapSRV);
	pDeviceContext->PSSetShaderResources(7, 1, &brdfLUTSRV);

	// -- Draw Scene Objects -- //
	m_pEngine->GetScene().GetRenderManager().DrawOpaque(ProjMat, ViewMat);
		pDeviceContext->RSSetState(pRasterizerStateCULLNONE.Get());
	m_pEngine->GetScene().GetRenderManager().DrawFoliage(ProjMat, ViewMat);
		pDeviceContext->RSSetState(pRasterizerState.Get());

	// -- Update 2D shaders -- //
	pDeviceContext->IASetInputLayout(vertexshader_2d.GetInputLayout());
	pDeviceContext->PSSetShader(pixelshader_2d.GetShader(), NULL, 0);
	pDeviceContext->VSSetShader(vertexshader_2d.GetShader(), NULL, 0);
	//sprite.Draw(camera2D.GetWorldmatrix() * camera2D.GetOrthoMatrix()); // Draws hello world sprite image

	// -- Draw Text -- //
	static int fpsCounter = 0;
	static std::string fpsString = "FPS: 0";
	
	if (m_drawfpsCPU)
	{
		fpsString += "CPU " + std::to_string(static_cast<int>(m_pEngine->GetFrameTimer().fps()));
		if (m_drawFrameTimeCPU)
			fpsString += " / " + std::to_string(static_cast<int>(m_pEngine->GetFrameTimer().milliseconds())) + " ms\n";
		else
			fpsString += " fps\n";
	}

	if (m_drawfpsGPU)
	{
		fpsString += "GPU " + std::to_string(static_cast<int>(m_frameTimer.fps()));
		if (m_drawFrameTimeGPU)
			fpsString += " / " + std::to_string(static_cast<int>(m_frameTimer.milliseconds())) + " ms \n";
		else
			fpsString += " fps\n";
	}

	if (m_logDrawCalls)
	{
		fpsString += "Draw Calls: " + std::to_string(m_drawCalls) + "\n";
		m_drawCalls = 0;
	}
	pSpriteBatch->Begin();
	pSpriteFont->DrawString(pSpriteBatch.get(), StringHelper::StringToWide(fpsString).c_str(), DirectX::XMFLOAT2(0, 50), DirectX::Colors::White, 0.0f, DirectX::XMFLOAT2(0.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 1.0f));
	pSpriteBatch->End();
	fpsString = "";


	// -- Update ImGui -- //
	static bool showEditor = true;
	if (InputManager::Instance()->keyboard.KeyIsPressed('Y'))
		showEditor = false;

	if (InputManager::Instance()->keyboard.KeyIsPressed('U'))
		showEditor = true;
	if(showEditor)
		UpdateImGuiWidgets();
	else
		ImGui::Render();

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	if (pImGuiIO->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	// -- Flip Buffer and Present-- //
	pSwapchain->Present(0, NULL); // Enable Vertical sync with 1 or 0
}

void Graphics::Update(const float& deltaTime)
{
	m_deltaTime = deltaTime;
	editorCamera.Update(deltaTime);
}

void Graphics::Shutdown()
{
	// Raw pointers
	delete pointLight;
	delete directionalLight;
	delete m_pMaterial;
	delete skybox;
	delete m_pSkyMaterial;
	delete pImGuiIO;
	delete m_pEngine;
	delete skyTexture;
	delete skyboxTextureSRV;
	delete irradianceMap;
	delete irradianceMapSRV;
	delete environmentMap;
	delete environmentMapSRV;
	delete brdfLUTtex;
	delete brdfLUTSRV;

	// WRL pointers
	pDevice->Release();
	pDeviceContext->Release();
	pSwapchain->Release();
	pRenderTargetView->Release();
	pDepthStencilView->Release();
	pDepthStencilBuffer->Release();
	pDepthStencilState->Release();
	pRasterizerState->Release();
	pRasterizerState->Release();
	pRasterizerState->Release();
	samplerState->Release();

	// Smart pointers
	pSpriteBatch.release();
	pSpriteFont.release();

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

bool Graphics::InitializeShaders()
{
#pragma region DetermineShaderPath
	{
#ifdef _DEBUG // Debug Mode
#ifdef _WIN64 //x64
		m_shaderFolder = L"..\\bin\\x64\\Debug\\";
#else // x86 (Win32)
		m_shaderFolder = L"..\\bin\\Win32\\Debug\\";
#endif
#else // Release Mode
#ifdef _WIN64 //x64
		m_shaderFolder = L"..\\bin\\x64\\Release\\";
#else //x86 (Win32)
		m_shaderFolder = L"..\\bin\\Win32\\Release\\";
#endif
#endif
	}
#pragma endregion

	// 2D shaders
	D3D11_INPUT_ELEMENT_DESC layout2D[] =
	{
		{"POSITION", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0  }
	};
	UINT numElements2D = ARRAYSIZE(layout2D);

	if (!vertexshader_2d.Initialize(pDevice, m_shaderFolder + L"vertexshader_2d.cso", layout2D, numElements2D))
		return false;

	if (!pixelshader_2d.Initialize(pDevice, m_shaderFolder + L"pixelshader_2d.cso"))
		return false;

	// 3D shaders
	// -- Initialize PBR Shaders -- //
	D3D11_INPUT_ELEMENT_DESC defaultLayout3D[] =
	{
		{"POSITION", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0  },
		{"NORMAL", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0  },
		{"TANGENT", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0  },
		{"BITANGENT", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0  }
	};
	UINT defaultNumElements3D = ARRAYSIZE(defaultLayout3D);

	// Sky
	if (!skyVertexShader.Initialize(pDevice, m_shaderFolder + L"Sky_vs.cso", defaultLayout3D, defaultNumElements3D))
	{
		ErrorLogger::Log("Failed to initialize Sky vertex shader");
		return false;
	}
	if (!skyPixelShader.Initialize(pDevice, m_shaderFolder + L"Sky_ps.cso"))
	{
		ErrorLogger::Log("Failed to initialize Sky pixel shader");
		return false;
	}

	return true;
}

bool Graphics::InitializeScene()
{
	try
	{
		// Initialize Constant Buffer(s)
		HRESULT hr = cb_vs_vertexshader_2d.Initialize(pDevice.Get(), pDeviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize constant buffer 2d for vertex shader.");

		hr = cb_vs_vertexshader.Initialize(pDevice.Get(), pDeviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize constant buffer for vertex shader.");
		
		hr = cb_ps_light.Initialize(pDevice.Get(), pDeviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize constant buffer for pixel shader.");

		hr = cb_ps_PerFrame.Initialize(pDevice.Get(), pDeviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize constant buffer for pixel shader utilites.");

		hr = cb_vs_PerFrame.Initialize(pDevice.Get(), pDeviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize constant buffer for vertex shader utilites.");

		hr = cb_ps_directionalLight.Initialize(pDevice.Get(), pDeviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize constant buffer for directional light for use in pixel shader.");

		//editorCamera.Initialize(&m_pEngine->GetScene(), (*new ID("EditorCamera")));

		// Initialize light shader values
		cb_ps_light.data.ambientLightColor = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
		//cb_ps_light.data.ambientLightStrength = 2.498f;
		//cb_ps_light.data.ambientLightStrength = 4.4f;
		cb_ps_light.data.ambientLightStrength = 1.0f;

		cb_ps_directionalLight.data.Color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
		cb_ps_directionalLight.data.Strength = 20.0f;
		cb_ps_directionalLight.data.Direction = XMFLOAT3(0.25f, 0.5f, -1.0f);
		cb_ps_directionalLight.ApplyChanges();
		cb_ps_PerFrame.data.camPosition = editorCamera.GetTransform().GetPosition();
		cb_ps_PerFrame.data.deltaTime = 0.5f;

		cb_vs_PerFrame.data.deltaTime = 0.5f;
		cb_vs_PerFrame.data.time = 0.5f;

		pointLight->lightStrength = 1.0f;
		pointLight->attenuation_a = 0.5f;
		pointLight->attenuation_b = 0.0f;
		pointLight->attenuation_c = 0.0f;


		// Hello World sprite
		if (!sprite.Initialize(pDevice.Get(), pDeviceContext.Get(), 256, 256, "..\\Assets\\Textures\\cat.jpg", cb_vs_vertexshader_2d))
		{
			ErrorLogger::Log("Failed to initilize sprite");
			return false;
		}
		camera2D.SetProjectionValues((float)windowWidth, (float)windowHeight, 0.0f, 1.0f);

		editorCamera.GetTransform().SetPosition(DirectX::XMFLOAT3(0.0f, 5.0f, -40.0f));
		editorCamera.SetProjectionValues(75.0f, static_cast<float>(windowWidth) / static_cast<float>(windowHeight), 0.1f, 4000.0f);

	}
	catch (COMException & exception)
	{
		ErrorLogger::Log(exception);
		return false;
	}
	return true;
}

void Graphics::UpdateImGuiWidgets()
{
	using namespace Debug;
	Entity* pSelectedEntity = Editor::Instance()->GetSelectedEntity();
	std::list<Entity*>* entities = m_pEngine->GetScene().GetAllEntities();

	// ImGuizmo Experimental tool
	//ImGuizmo::BeginFrame();
	//ImGuizmo::Enable(true);

	////ImGuiIO& io = ImGui::GetIO();
	//XMFLOAT4X4 camViewTemp;
	//XMStoreFloat4x4(&camViewTemp, editorCamera.GetViewMatrix());
	//float camView[16] =
	//{
	//	camViewTemp._11,camViewTemp._21, camViewTemp._31, camViewTemp._41,
	//	camViewTemp._12,camViewTemp._22, camViewTemp._32, camViewTemp._42,
	//	camViewTemp._13,camViewTemp._23, camViewTemp._33, camViewTemp._43,
	//	camViewTemp._14,camViewTemp._24, camViewTemp._34, camViewTemp._44
	//};
	//XMFLOAT4X4 camPojTemp;
	//XMStoreFloat4x4(&camPojTemp, editorCamera.GetProjectionMatrix());
	//float camProj[16] =
	//{
	//	camPojTemp._11, camPojTemp._21, camPojTemp._31, camPojTemp._41,
	//	camPojTemp._12, camPojTemp._22, camPojTemp._32, camPojTemp._42,
	//	camPojTemp._13, camPojTemp._23, camPojTemp._33, camPojTemp._43,
	//	camPojTemp._14, camPojTemp._24, camPojTemp._34, camPojTemp._44
	//};
	//XMFLOAT4X4 objMatTemp;
	//XMStoreFloat4x4(&objMatTemp, pSelectedEntity->GetTransform().GetWorldMatrix());
	//float objMat[16] =
	//{
	//	objMatTemp._11, objMatTemp._21, objMatTemp._31, objMatTemp._41,
	//	objMatTemp._12, objMatTemp._22, objMatTemp._32, objMatTemp._42,
	//	objMatTemp._13, objMatTemp._23, objMatTemp._33, objMatTemp._43,
	//	objMatTemp._14, objMatTemp._24, objMatTemp._34, objMatTemp._44
	//};
	//static float identityMatrix[16] =
	//{ 
	//	1.f, 0.f, 0.f, 0.f,
	//	0.f, 1.f, 0.f, 0.f,
	//	0.f, 0.f, 1.f, 0.f,
	//	0.f, 0.f, 0.f, 1.f 
	//};
	//static float defaultMatrix[16] =
	//{
	//	1.f, 0.f, 0.f, 0.f,
	//	0.f, 1.f, 0.f, 0.f,
	//	0.f, 0.f, 1.f, 0.f,
	//	0.f, 0.f, 0.f, 1.f 
	//};
	//ImGuizmo::DrawCube(camView, camProj, defaultMatrix);

	//ImGuizmo::SetDrawlist();

	//ImGuizmo::SetRect(0, 0, pImGuiIO->DisplaySize.x, pImGuiIO->DisplaySize.y);
	//ImGuizmo::Manipulate(camView, camProj, ImGuizmo::TRANSLATE, ImGuizmo::LOCAL, objMat);

	//if (ImGuizmo::IsOver())
	//	Debug::Editor::Instance()->DebugLog("Mouse is over");

	//ImGui::Begin("Game");
	//{
	//	/*ID3D11ShaderResourceView* my_texture_view;
	//	D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
	//	pDevice->CreateShaderResourceView(backBufferTex.GetTexture(), 0, backBufferTex.GetTextureResourceViewAddress());
	//	ImGui::Image((void*)backBufferTex.GetTextureResourceViewAddress(), ImVec2(1024, 1024));*/

	//	/*ID3D11ShaderResourceView* my_texture_view;
	//	D3D11_SHADER_RESOURCE_VIEW_DESC my_shader_resource_view_desc = {};
	//	DirectX::CreateWICTextureFromFile(pDevice.Get(), L"..\\Assets\\Textures\\Skyboxes\\ibl_brdf_lut.png", nullptr, &brdfLUTSRV);

	//	pDevice->CreateShaderResourceView(pBackBuffer.Get(), &my_shader_resource_view_desc, &my_texture_view);*/
	//	//HRESULT hr = DirectX::CreateWICTextureFromFile(pDevice.Get(), L"..\\Assets\\Textures\\Skyboxes\\ibl_brdf_lut.png", nullptr, &backBufferSRV);
	//	
	//	D3D11_TEXTURE2D_DESC textureDesc;
	//	ZeroMemory(&textureDesc, sizeof(textureDesc));
	//	// Setup the render target texture description.
	//	textureDesc.Width = 1920;
	//	textureDesc.Height = 1080;
	//	textureDesc.MipLevels = 1;
	//	textureDesc.ArraySize = 1;
	//	textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	//	textureDesc.SampleDesc.Count = 1;
	//	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	//	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	//	textureDesc.CPUAccessFlags = 0;
	//	textureDesc.MiscFlags = 0;

	//	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
	//	shaderResourceViewDesc.Format = textureDesc.Format;
	//	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	//	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	//	shaderResourceViewDesc.Texture2D.MipLevels = 1;
	//	pDevice->CreateShaderResourceView(pBackBuffer.Get(), &shaderResourceViewDesc, &backBufferSRV);
	//	ImGui::Image((void*)backBufferSRV, ImVec2(512, 512));
	//	DirectX::CreateWicTextureFromMemory()

	//}
	//ImGui::End();
	// Menu Bar
	if (ImGui::Begin("Menu Bar", NULL, ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Save"))
					FileSystem::Instance()->WriteSceneToJSON(&m_pEngine->GetScene());
				ImGui::MenuItem("Open");
				ImGui::MenuItem("New Scene");
				
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Engine DEBUG"))
			{
				if (ImGui::MenuItem("Draw CPU fps"))
				{
					m_drawfpsCPU = !m_drawfpsCPU;
				}
				if (ImGui::MenuItem("Draw CPU Frame Time"))
				{
					m_drawFrameTimeCPU = !m_drawFrameTimeCPU;
				}
				if (ImGui::MenuItem("Draw GPU fps"))
				{
					m_drawfpsGPU = !m_drawfpsGPU;
				}
				if (ImGui::MenuItem("Draw GPU Frame Time"))
				{
					m_drawFrameTimeGPU = !m_drawFrameTimeGPU;
				}
				if (ImGui::MenuItem("Enable Wireframe"))
					m_drawWireframe = !m_drawWireframe;

				if (ImGui::MenuItem("Log Draw Calls"))
					m_logDrawCalls = !m_logDrawCalls;

				ImGui::EndMenu();
			}
			
			ImGui::EndMenuBar();
		}
	}
	ImGui::End();

	ImGui::Begin("World Outliner");
	{
		std::list<Entity*>::iterator iter;
		for (iter = entities->begin(); iter != entities->end(); iter++)
		{
			if (ImGui::Button((*iter)->GetID().GetName().c_str(), { 150.0f, 20.0f }))
			{
				Editor::Instance()->SetSelectedEntity((*iter));
			}
		}
	}
	ImGui::End();

	ImGui::Begin("Console");
	{
		if (ImGui::Button("Clear", { 100, 20 }))
		{
			Editor::Instance()->ClearConsole();
		}

		ImGui::SameLine();
		static bool clearOnPlay = Editor::Instance()->GetClearConsoleOnPlay();
		if (ImGui::Checkbox("Clear on play", &clearOnPlay))
		{
			Editor::Instance()->SetCLearConsoleOnPlay(clearOnPlay);
		}

		ImGui::Text(Editor::Instance()->GetLogStatement().c_str());
	}
	ImGui::End();

	ImGui::Begin("Editor");
	{
		ImGui::Text("Status: ");
		ImGui::SameLine();
		std::string playStatus = "";
		if (Editor::Instance()->PlayingGame())
			playStatus += "Playing";
		else
			playStatus += "Not Playing";
		ImGui::Text(playStatus.c_str());

		if (ImGui::Button("Play", { 50.0f, 20.0f }))
		{
			if(!Editor::Instance()->PlayingGame())
				Editor::Instance()->PlayGame();// Editor calles scene OnStart
		}
		if (ImGui::Button("Stop", { 50.0f, 20.0f }) || InputManager::Instance()->keyboard.KeyIsPressed(27))
		{
			Editor::Instance()->StopGame();
		}
		//ImGui::Checkbox("Editor Camera Enabled", &m_editorCamEnabled);
		
	}
	ImGui::End();
	
	ImGui::Begin("Lighting");
	{
		ImGui::Text("Ambient/IBL Light");
		ImGui::DragFloat3("Color Override", &cb_ps_light.data.ambientLightColor.x, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Strength Override", &cb_ps_light.data.ambientLightStrength, 0.01f, 0.0f, 10.0f);

		ImGui::Text("Directional Light");
		ImGui::DragFloat3("Directional Color", &directionalLight->lightColor.x, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Directional Strength", &directionalLight->lightStrength, 0.01f, 0.0f, 50.0f);

		ImGui::Text("Point Light");
		ImGui::DragFloat3("Point Color", &pointLight->lightColor.x, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Point Strength", &pointLight->lightStrength, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Attenuation A", &pointLight->attenuation_a, 0.01f, 0.1f, 10.0f);
		ImGui::DragFloat("Attenuation B", &pointLight->attenuation_b, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Attenuation C", &pointLight->attenuation_c, 0.01f, 0.0f, 10.0f);
	}
	ImGui::End();

	std::string entityName = pSelectedEntity->GetID().GetName();
	ImGui::Begin("Inspector");
	{
		ImGui::Text(entityName.c_str());
		ImGui::TextColored({100, 100, 100, 100}, "Transform");
		ImGui::DragFloat3("Position", &pSelectedEntity->GetTransform().GetPosition().x, 0.1f, -2000.0f, 2000.0f);
		ImGui::DragFloat3("Rotation", &pSelectedEntity->GetTransform().GetRotation().x, 0.1f, -100.0f, 100.0f);
		ImGui::DragFloat3("Scale", &pSelectedEntity->GetTransform().GetScale().x, 0.1f, -500.0f, 500.0f);

		ImGui::NewLine();
		std::vector<Component*> objectComponents = pSelectedEntity->GetAllComponents();
		std::vector<Component*>::iterator iter;
		for (iter = objectComponents.begin(); iter != objectComponents.end(); iter++)
		{
			ImGui::NewLine();

			(*iter)->OnImGuiRender();
		}

	}
	ImGui::End();

	//static int creationCounter = 33;
	static int creationCounter = 0;

	ImGui::Begin("Entity Creator");
	{
		if (ImGui::Button("Create Rock Asset"))
		{
			creationCounter++;
			std::string creationCount = "Rock-" + std::to_string(creationCounter);
			Entity* entity = new Entity(&m_pEngine->GetScene(), (*new ID()));
			entity->GetID().SetName(creationCount);
			entity->GetID().SetTag("Untagged");
			entity->GetID().SetType("Entity");
			entity->GetTransform().SetPosition(editorCamera.GetTransform().GetPosition());
			entity->GetTransform().SetRotation(0.0f, 0.0f, 0.0f);
			entity->GetTransform().SetScale(1.0f, 1.0f, 1.0f);
			// mr
			//Material* mat = new MaterialTextured(Material::eMaterialType::PBR_DEFAULT);
			Material* mat = nullptr;
			mat = mat->SetMaterialByType(Material::eMaterialType::PBR_DEFAULT, Material::eFlags::NOFLAGS);
			//mat = mat->SetMaterialByType(Material::eMaterialType::PBR_DEFAULT, Material::eFlags::FOLIAGE);

			//mat->Initiailze(pDevice.Get(), pDeviceContext.Get(), Material::eFlags::FOLIAGE);
			mat->Initiailze(pDevice.Get(), pDeviceContext.Get(), Material::eFlags::NOFLAGS);
			std::string file = "..\\Assets\\Objects\\\MossyRock\\\MossyRock_LOD2.fbx";
			MeshRenderer* mr = entity->AddComponent<MeshRenderer>();
			mr->Initialize(entity, file, pDevice.Get(), pDeviceContext.Get(), this->GetDefaultVertexShader(), mat);
			entity->SetHasMeshRenderer(true);
			//LuaScript
			entity->AddComponent<LuaScript>()->Initialize(entity, "NONE");
			//Es
			entity->AddComponent<EditorSelection>()->Initialize(entity, 10.0f, entity->GetTransform().GetPosition());

			//m_pEngine->GetScene().GetRenderManager().AddFoliageObject(mr);
			m_pEngine->GetScene().GetRenderManager().AddOpaqueObject(mr);
			m_pEngine->GetScene().AddEntity(entity);
		}
	}
	ImGui::End();
	if (ImGui::IsAnyItemHovered())
		Editor::Instance()->rayCastEnabled = false;

	// Assemble Draw Data
	ImGui::Render();
}
