#include "Engine.h"
#include <thread>

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,	_In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{

	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to call CoInitialize.");
		return -1;
	}


	Engine engine;

	if (!engine.Initialize(hInstance, "Insight Engine (Beta Build - WIP)", "EngineWindowClass", 1600, 900))
		return -1;

	while (engine.ProccessMessages() == true)
	{
		engine.Update();
		engine.RenderFrame();
	}

	engine.Shutdown();

	/*std::thread logic(&Engine::Update, Engine());
	std::thread renderer(&Engine::RenderFrame, Engine());
	logic.join();
	renderer.join();*/

	return 0;
}