#pragma once

#include <dxgi.h>
#include <d3d11.h>
#include <wrl/client.h>

#include "mathematics.h"


using Microsoft::WRL::ComPtr;



namespace skincut
{
	// Render target
	class Target
	{
	private:
		ComPtr<ID3D11Device> mDevice;
		ComPtr<ID3D11DeviceContext> mContext;

	public:
		ComPtr<ID3D11Texture2D> mTexture;
		ComPtr<ID3D11BlendState> mBlendState;
		ComPtr<ID3D11RenderTargetView> mRenderTarget;
		ComPtr<ID3D11ShaderResourceView> mShaderResource;

		UINT mSampleMask; // sampling method (0xffffffff: point sampling)
		math::Color mBlendFactor;
		D3D11_VIEWPORT mViewport;
		D3D11_BLEND_DESC mBlendDesc;


	public:
		Target(ComPtr<ID3D11Device>& device, 
			   ComPtr<ID3D11DeviceContext>& context, 
			   UINT width, UINT height, 
			   DXGI_FORMAT format, bool typeless = true);
		Target(ComPtr<ID3D11Device>& device, 
			   ComPtr<ID3D11DeviceContext>& context, 
			   UINT width, UINT height, 
			   DXGI_FORMAT format, ComPtr<ID3D11Texture2D>& basetex);
		Target(ComPtr<ID3D11Device>& device, 
			   ComPtr<ID3D11DeviceContext>& context, 
			   ComPtr<ID3D11Texture2D>& texture, DXGI_FORMAT format);

		void Clear();
		void Clear(const math::Color& color);

		void SetViewport(float width, float height, float mindepth = 0, float maxdepth = 1);
		void SetBlendState(D3D11_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc, math::Color blendColor, UINT sampleMask);
	};
}

