#include "target.h"
#include "utility.h"

#include "DirectXTex/DirectXTex.h"


using namespace skincut;
using namespace skincut::math;



Target::Target(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, 
	UINT width, UINT height, DXGI_FORMAT format, bool typeless) : 
	mDevice(device), mContext(context)
{
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = typeless ? DirectX::MakeTypeless(format) : format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	HREXCEPT(mDevice->CreateTexture2D(&desc, nullptr, mTexture.GetAddressOf()));

	D3D11_RENDER_TARGET_VIEW_DESC rtvdesc;
	rtvdesc.Format = format;
	rtvdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvdesc.Texture2D.MipSlice = 0;
	HREXCEPT(mDevice->CreateRenderTargetView(mTexture.Get(), 
		&rtvdesc, mRenderTarget.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvdesc;
	srvdesc.Format = format;
	srvdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvdesc.Texture2D.MostDetailedMip = 0;
	srvdesc.Texture2D.MipLevels = 1;
	HREXCEPT(mDevice->CreateShaderResourceView(mTexture.Get(), 
		&srvdesc, mShaderResource.GetAddressOf()));

	// viewport
	ZeroMemory(&mViewport, sizeof(D3D11_VIEWPORT));
	mViewport.TopLeftX = 0;
	mViewport.TopLeftY = 0;
	mViewport.Width = (FLOAT)width;
	mViewport.Height = (FLOAT)height;
	mViewport.MinDepth = 0.F;
	mViewport.MaxDepth = 1.F;

	// Create default blend state
	ZeroMemory(&mBlendDesc, sizeof(D3D11_BLEND_DESC));
	mBlendDesc.AlphaToCoverageEnable = FALSE;
	mBlendDesc.IndependentBlendEnable = FALSE;
	mBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	mBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	mBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	mBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	mBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	mBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	mBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	mBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	HREXCEPT(mDevice->CreateBlendState(&mBlendDesc, mBlendState.GetAddressOf()));

	mBlendFactor = Color(1,1,1,1);
	mSampleMask = 0xffffffff;


	// Clear buffer for first use
	mContext->ClearRenderTargetView(mRenderTarget.Get(), DirectX::Colors::Transparent);
}


Target::Target(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, 
	UINT width, UINT height, DXGI_FORMAT format, ComPtr<ID3D11Texture2D>& basetex) : 
	mDevice(device), mContext(context)
{
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	HREXCEPT(mDevice->CreateTexture2D(&desc, nullptr, &mTexture));

	if (basetex) context->CopyResource(mTexture.Get(), basetex.Get());

	D3D11_RENDER_TARGET_VIEW_DESC rtvdesc;
	rtvdesc.Format = format;
	rtvdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvdesc.Texture2D.MipSlice = 0;
	HREXCEPT(mDevice->CreateRenderTargetView(mTexture.Get(), 
		&rtvdesc, mRenderTarget.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvdesc;
	srvdesc.Format = format;
	srvdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvdesc.Texture2D.MostDetailedMip = 0;
	srvdesc.Texture2D.MipLevels = 1;
	HREXCEPT(mDevice->CreateShaderResourceView(mTexture.Get(), 
		&srvdesc, mShaderResource.GetAddressOf()));

	ZeroMemory(&mViewport, sizeof(D3D11_VIEWPORT));
	mViewport.TopLeftX = 0;
	mViewport.TopLeftY = 0;
	mViewport.Width = (FLOAT)width;
	mViewport.Height = (FLOAT)height;
	mViewport.MinDepth = 0.F;
	mViewport.MaxDepth = 1.F;

	// Create default blend state
	ZeroMemory(&mBlendDesc, sizeof(D3D11_BLEND_DESC));
	mBlendDesc.AlphaToCoverageEnable = FALSE;
	mBlendDesc.IndependentBlendEnable = FALSE;
	mBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	mBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	mBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	mBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	mBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	mBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	mBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	mBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	HREXCEPT(mDevice->CreateBlendState(&mBlendDesc, mBlendState.GetAddressOf()));

	mBlendFactor = Color(1,1,1,1);
	mSampleMask = 0xffffffff;
}

Target::Target(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, 
	ComPtr<ID3D11Texture2D>& texture, DXGI_FORMAT format) : 
	mDevice(device), mContext(context), mTexture(texture)
{
	D3D11_TEXTURE2D_DESC texDesc;
	mTexture->GetDesc(&texDesc);

	// Increase the count as the reference is now shared between two RenderTargets.
	mTexture.Get()->AddRef();

	D3D11_RENDER_TARGET_VIEW_DESC rtvdesc;
	rtvdesc.Format = format;
	rtvdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvdesc.Texture2D.MipSlice = 0;
	HREXCEPT(mDevice->CreateRenderTargetView(mTexture.Get(), 
		&rtvdesc, mRenderTarget.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvdesc;
	srvdesc.Format = format;
	srvdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvdesc.Texture2D.MostDetailedMip = 0;
	srvdesc.Texture2D.MipLevels = 1;
	HREXCEPT(mDevice->CreateShaderResourceView(mTexture.Get(), 
		&srvdesc, mShaderResource.GetAddressOf()));

	ZeroMemory(&mViewport, sizeof(D3D11_VIEWPORT));
	mViewport.TopLeftX = 0;
	mViewport.TopLeftY = 0;
	mViewport.Width = (FLOAT)texDesc.Width;
	mViewport.Height = (FLOAT)texDesc.Height;
	mViewport.MinDepth = 0.F;
	mViewport.MaxDepth = 1.F;

	// Create default blend state
	ZeroMemory(&mBlendDesc, sizeof(D3D11_BLEND_DESC));
	mBlendDesc.AlphaToCoverageEnable = FALSE;
	mBlendDesc.IndependentBlendEnable = FALSE;
	mBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	mBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	mBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	mBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	mBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	mBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	mBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	mBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	HREXCEPT(mDevice->CreateBlendState(&mBlendDesc, mBlendState.GetAddressOf()));

	mBlendFactor = Color(1,1,1,1);
	mSampleMask = 0xffffffff;
}


void Target::Clear()
{
	mContext->ClearRenderTargetView(mRenderTarget.Get(), DirectX::Colors::Black);
}

void Target::Clear(const math::Color& color)
{
	mContext->ClearRenderTargetView(mRenderTarget.Get(), color);
}


void Target::SetViewport(float width, float height, float mindepth, float maxdepth)
{
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = (FLOAT)width;
	viewport.Height = (FLOAT)height;
	viewport.MinDepth = mindepth;
	viewport.MaxDepth = maxdepth;
}


void Target::SetBlendState(D3D11_RENDER_TARGET_BLEND_DESC rtbdesc, Color blend, UINT sampleMask)
{
	//mBlendState->Release();
	mBlendDesc.RenderTarget[0] = rtbdesc;
	HREXCEPT(mDevice->CreateBlendState(&mBlendDesc, 
		mBlendState.ReleaseAndGetAddressOf()));

	mBlendFactor = blend;
	mSampleMask = sampleMask;
}

