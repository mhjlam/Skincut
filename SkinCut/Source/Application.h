#pragma once

#include <array>
#include <memory>
#include <vector>
#include <d3d11.h>
#include <windows.h>
#include <wrl/client.h>
#include <unordered_map>
#include "DirectXTK/SpriteFont.h"
#include "DirectXTK/SpriteBatch.h"

#include "structures.h"


using DirectX::SpriteFont;
using DirectX::SpriteBatch;
using Microsoft::WRL::ComPtr;


namespace skincut
{
	class Light;
	class Model;
	class Decal;
	class Camera;
	class Renderer;
	class Generator;
	class Dashboard;
	class FrameBuffer;
	class Target;


	class Application
	{
	protected:
		HWND								mHwnd;

		ComPtr<ID3D11Device>				mDevice;
		ComPtr<IDXGISwapChain>				mSwapChain;
		ComPtr<ID3D11DeviceContext>			mContext;

		std::unique_ptr<SpriteFont>			mSpriteFont;
		std::unique_ptr<SpriteBatch>		mSpriteBatch;

		std::unique_ptr<Camera>				mCamera;
		std::vector<std::shared_ptr<Light>> mLights;
		std::vector<std::shared_ptr<Model>> mModels;

		std::unique_ptr<Renderer>			mRenderer;
		std::unique_ptr<Dashboard>			mDashboard;
		std::unique_ptr<Generator>			mGenerator;

		std::unique_ptr<Intersection>		mPointA;
		std::unique_ptr<Intersection>		mPointB;


	public:
		Application();
		virtual ~Application();

		virtual bool Initialize(HWND hWnd, const std::string& respath);
		virtual bool Update();
		virtual bool Render();
		virtual bool Reload();
		virtual LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


	protected: // subroutines
		bool LoadScene();
		bool LoadConfig();
		bool SetupRenderer();
		bool SetupDashboard();

		void Pick();
		void CreateCut(Intersection& ia, Intersection& ib);

		void Split();
		void DrawDecal();

		Intersection FindIntersection(math::Vector2 cursor, math::Vector2 resolution, math::Vector2 window, math::Matrix proj, math::Matrix view);

		void CreateWound(std::list<Link>& cutline, std::shared_ptr<Model>& model, std::shared_ptr<Target>& patch);
		void PaintWound(std::list<Link>& cutline, std::shared_ptr<Model>& model, std::shared_ptr<Target>& patch);


		void PerformanceTest();
	};
}

