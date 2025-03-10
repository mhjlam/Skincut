#pragma once

#include <map>
#include <list>
#include <dxgi.h>
#include <memory>
#include <vector>
#include <d3d11.h>
#include <wchar.h>
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <unordered_map>
#include <DirectXColors.h>

#include "structures.h"
#include "mathematics.h"


using Microsoft::WRL::ComPtr;



namespace skincut
{
	class Decal;
	class Light;
	class Model;
	class Camera;
	class Shader;
	class Sampler;
	class Texture;
	class FrameBuffer;
	class Target;
	class VertexBuffer;


	class Renderer
	{
	public:
		unsigned int					mWidth;
		unsigned int					mHeight;
		D3D_DRIVER_TYPE					mDriverType;
		D3D_FEATURE_LEVEL				mFeatureLevel;

		ComPtr<ID3D11Device>			mDevice;
		ComPtr<IDXGISwapChain>			mSwapChain;
		ComPtr<ID3D11DeviceContext>		mContext;
		ComPtr<ID3D11RasterizerState>	mRasterizer;
		ComPtr<ID3D11DepthStencilState>	mDepthStencil;

		std::shared_ptr<FrameBuffer>	mBackBuffer;
		std::shared_ptr<VertexBuffer>	mScreenBuffer;


	private:
		std::vector<math::Color> mKernel;
		std::vector<std::shared_ptr<Decal>> mDecals;
		std::unordered_map<std::string, std::shared_ptr<Shader>> mShaders;
		std::unordered_map<std::string, std::shared_ptr<Sampler>> mSamplers;
		std::unordered_map<std::string, std::shared_ptr<Texture>> mResources;
		std::unordered_map<std::string, std::shared_ptr<Target>> mTargets;


	public:
		Renderer(HWND hwnd, UINT width, UINT height);
		~Renderer();

		void Resize(UINT width, UINT height);
		void Render(std::vector<std::shared_ptr<Model>>& models, 
					std::vector<std::shared_ptr<Light>>& lights, 
					std::unique_ptr<Camera>& camera);

		void CreateWoundDecal(Intersection& intersection);
		void CreateWoundDecal(Intersection& i0, Intersection& i1);
		void PaintWoundPatch(std::shared_ptr<Model>& model, std::shared_ptr<Target>& patch, 
			std::map<Link, std::vector<Face*>>& innerfaces, float cutwidth, float cutheight);
		void PaintDiscoloration(std::shared_ptr<Model>& model, std::map<Link, std::vector<Face*>>& outerfaces, float cutheight);


	private:
		void InitializeDevice(HWND hwnd);
		void InitializeShaders();
		void InitializeSamplers();
		void InitializeResources();
		void InitializeRasterizer();
		void InitializeTargets();
		void InitializeKernel();

		void Draw(std::shared_ptr<VertexBuffer>& vertexbuffer,
			      std::shared_ptr<Shader>& shader,
			      D3D11_VIEWPORT viewport,
			      ID3D11DepthStencilView* depthbuffer,
			      std::vector<ID3D11RenderTargetView*>& targets,
			      std::vector<ID3D11ShaderResourceView*>& resources,
			      std::vector<ID3D11SamplerState*>& samplers,
			      D3D11_FILL_MODE fillmode = D3D11_FILL_SOLID);

		void Draw(std::shared_ptr<Model>& model,
			      std::shared_ptr<Shader>& shader,
			      std::shared_ptr<FrameBuffer>& buffer,
			      std::vector<ID3D11RenderTargetView*>& targets,
			      std::vector<ID3D11ShaderResourceView*>& resources,
			      std::vector<ID3D11SamplerState*>& samplers,
			      D3D11_FILL_MODE fillmode = D3D11_FILL_SOLID);

		void RenderDepth(std::shared_ptr<Model>& model, 
						 std::vector<std::shared_ptr<Light>>& lights, 
						 std::unique_ptr<Camera>& camera);
		void RenderLighting(std::shared_ptr<Model>& model, 
							std::vector<std::shared_ptr<Light>>& lights, 
							std::unique_ptr<Camera>& camera);
		void RenderScattering();
		void RenderSpeculars();
		void RenderDecals(std::unique_ptr<Camera>& camera);

		void RenderBlinnPhong(std::shared_ptr<Model>& model, 
							  std::vector<std::shared_ptr<Light>>& lights, 
							  std::unique_ptr<Camera>& camera);
		void RenderLambertian(std::shared_ptr<Model>& model, 
							  std::vector<std::shared_ptr<Light>>& lights, 
							  std::unique_ptr<Camera>& camera);


		void SetRasterizerState(D3D11_FILL_MODE fillmode = D3D11_FILL_SOLID, D3D11_CULL_MODE cullmode = D3D11_CULL_BACK);
		void SetRasterizerState(D3D11_RASTERIZER_DESC desc);

		void ClearBuffer(std::shared_ptr<FrameBuffer>& buffer, const math::Color& color = DirectX::Colors::Black);
		void ClearBuffer(std::shared_ptr<Target>& target, const math::Color& color = DirectX::Colors::Black);

		void CopyBuffer(std::shared_ptr<FrameBuffer>& src, std::shared_ptr<FrameBuffer>& dst);

		void UnbindResources(UINT numViews, UINT startSlot = 0);
		void UnbindRenderTargets(UINT numViews);
	};
}

