#include "framebuffer.h"
#include "utility.h"


using namespace skincut;



FrameBuffer::FrameBuffer(ComPtr<ID3D11Device>& device, 
						 ComPtr<ID3D11Context>& context, 
						 ComPtr<IDXGISwapChain>& swapchain, 
						 DXGI_FORMAT depthfmttex, 
						 DXGI_FORMAT depthfmtdsv, 
						 DXGI_FORMAT depthfmtsrv) : mDevice(device), mContext(context)
{
	// Color buffer
	swapchain->GetBuffer(0, __uuidof(mColorTexture), 
		reinterpret_cast<void**>(mColorTexture.GetAddressOf()));

	D3D11_TEXTURE2D_DESC desc;
	mColorTexture->GetDesc(&desc);
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	D3D11_RENDER_TARGET_VIEW_DESC rtdesc;
	rtdesc.Format = desc.Format;
	rtdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtdesc.Texture2D.MipSlice = 0;
	device->CreateRenderTargetView(mColorTexture.Get(), &rtdesc, 
		mColorBuffer.GetAddressOf());
	
	D3D11_SHADER_RESOURCE_VIEW_DESC srdesc;
	srdesc.Format = desc.Format;
	srdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srdesc.TextureCube.MostDetailedMip = 0;
	srdesc.TextureCube.MipLevels = 1;
	HREXCEPT(device->CreateShaderResourceView(mColorTexture.Get(), &srdesc, 
		mColorResource.GetAddressOf()));


	// Depth-stencil buffer
	D3D11_TEXTURE2D_DESC texdesc;
	ZeroMemory(&texdesc, sizeof(D3D11_TEXTURE2D_DESC));
	texdesc.Width = desc.Width;
	texdesc.Height = desc.Height;
	texdesc.MipLevels = 1;
	texdesc.ArraySize = 1;
	texdesc.Format = depthfmttex;
	texdesc.SampleDesc.Count = 1;
	texdesc.SampleDesc.Quality = 0;
	texdesc.Usage = D3D11_USAGE_DEFAULT;
	texdesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	device->CreateTexture2D(&texdesc, nullptr, &mDepthTexture);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvdesc;
	ZeroMemory(&dsvdesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
	dsvdesc.Format = depthfmtdsv;
	dsvdesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvdesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(mDepthTexture.Get(), &dsvdesc, 
		mDepthBuffer.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC srvdesc;
	ZeroMemory(&srvdesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	srvdesc.Format = depthfmtsrv;
	srvdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvdesc.Texture2D.MostDetailedMip = 0;
	srvdesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(mDepthTexture.Get(), &srvdesc, 
		mDepthResource.GetAddressOf());


	// Viewport
	ZeroMemory(&mViewport, sizeof(D3D11_VIEWPORT));
	mViewport.TopLeftX = 0.0F;
	mViewport.TopLeftY = 0.0F;
	mViewport.Width = (FLOAT)desc.Width;
	mViewport.Height = (FLOAT)desc.Height;
	mViewport.MinDepth = 0.0F;
	mViewport.MaxDepth = 1.0F;


	// Initial buffer clear
	mContext->ClearRenderTargetView(mColorBuffer.Get(), DirectX::Colors::Black);
	mContext->ClearDepthStencilView(mDepthBuffer.Get(), 
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}


FrameBuffer::FrameBuffer(ComPtr<ID3D11Device>& device, 
						 ComPtr<ID3D11Context>& context, 
						 UINT width, UINT height, 
						 DXGI_FORMAT colorfmt, 
						 DXGI_FORMAT depthfmttex,
						 DXGI_FORMAT depthfmtdsv,
						 DXGI_FORMAT depthfmtsrv) : mDevice(device), mContext(context)
{
	// Color buffer
	D3D11_TEXTURE2D_DESC texdesc;
	ZeroMemory(&texdesc, sizeof(D3D11_TEXTURE2D_DESC));
	texdesc.Format = colorfmt;
	texdesc.Width = width;
	texdesc.Height = height;
	texdesc.MipLevels = 1;
	texdesc.ArraySize = 1;
	texdesc.SampleDesc.Count = 1;
	texdesc.SampleDesc.Quality = 0;
	texdesc.Usage = D3D11_USAGE_DEFAULT;
	texdesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	HREXCEPT(mDevice->CreateTexture2D(&texdesc, nullptr, &mColorTexture));

	D3D11_RENDER_TARGET_VIEW_DESC rtvdesc;
	rtvdesc.Format = colorfmt;
	rtvdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvdesc.Texture2D.MipSlice = 0;
	HREXCEPT(mDevice->CreateRenderTargetView(mColorTexture.Get(), &rtvdesc, 
		mColorBuffer.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvdesc;
	srvdesc.Format = colorfmt;
	srvdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvdesc.Texture2D.MostDetailedMip = 0;
	srvdesc.Texture2D.MipLevels = 1;
	HREXCEPT(mDevice->CreateShaderResourceView(mColorTexture.Get(), &srvdesc, 
		mColorResource.GetAddressOf()));


	// Depth-stencil buffer
	ZeroMemory(&texdesc, sizeof(D3D11_TEXTURE2D_DESC));
	texdesc.Width = width;
	texdesc.Height = height;
	texdesc.MipLevels = 1;
	texdesc.ArraySize = 1;
	texdesc.Format = depthfmttex;
	texdesc.SampleDesc.Count = 1;
	texdesc.SampleDesc.Quality = 0;
	texdesc.Usage = D3D11_USAGE_DEFAULT;
	texdesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	device->CreateTexture2D(&texdesc, nullptr, &mDepthTexture);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvdesc;
	ZeroMemory(&dsvdesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
	dsvdesc.Format = depthfmtdsv;
	dsvdesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvdesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(mDepthTexture.Get(), &dsvdesc, 
		mDepthBuffer.GetAddressOf());

	ZeroMemory(&srvdesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	srvdesc.Format = depthfmtsrv;
	srvdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvdesc.Texture2D.MostDetailedMip = 0;
	srvdesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(mDepthTexture.Get(), &srvdesc, 
		mDepthResource.GetAddressOf());


	// Viewport
	ZeroMemory(&mViewport, sizeof(D3D11_VIEWPORT));
	mViewport.TopLeftX = 0.0F;
	mViewport.TopLeftY = 0.0F;
	mViewport.Width = (FLOAT)width;
	mViewport.Height = (FLOAT)height;
	mViewport.MinDepth = 0.0F;
	mViewport.MaxDepth = 1.0F;


	// Initial buffer clear
	mContext->ClearRenderTargetView(mColorBuffer.Get(), DirectX::Colors::Black);
	mContext->ClearDepthStencilView(mDepthBuffer.Get(), 
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}


FrameBuffer::FrameBuffer(ComPtr<ID3D11Device>& device, 
						 ComPtr<ID3D11Context>& context, 
						 ComPtr<ID3D11Texture2D>& colortex, 
						 DXGI_FORMAT colorfmt, 
						 DXGI_FORMAT depthfmttex,
						 DXGI_FORMAT depthfmtdsv,
						 DXGI_FORMAT depthfmtsrv) : mDevice(device), mContext(context)
{
	if (!colortex) throw;

	// Color buffer
	D3D11_TEXTURE2D_DESC texdesc;
	colortex->GetDesc(&texdesc);
	texdesc.MipLevels = 1;
	texdesc.ArraySize = 1;
	texdesc.Format = colorfmt;
	texdesc.SampleDesc.Count = 1;
	texdesc.SampleDesc.Quality = 0;
	texdesc.Usage = D3D11_USAGE_DEFAULT;
	texdesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	HREXCEPT(mDevice->CreateTexture2D(&texdesc, nullptr, &mColorTexture));

	D3D11_RENDER_TARGET_VIEW_DESC rtvdesc;
	rtvdesc.Format = colorfmt;
	rtvdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvdesc.Texture2D.MipSlice = 0;
	HREXCEPT(mDevice->CreateRenderTargetView(mColorTexture.Get(), &rtvdesc, 
		mColorBuffer.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvdesc;
	srvdesc.Format = colorfmt;
	srvdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvdesc.Texture2D.MostDetailedMip = 0;
	srvdesc.Texture2D.MipLevels = 1;
	HREXCEPT(mDevice->CreateShaderResourceView(mColorTexture.Get(), &srvdesc, 
		mColorResource.GetAddressOf()));

	// copy texture contents to new texture
	mContext->CopyResource(mColorTexture.Get(), colortex.Get());


	// Depth-stencil buffer
	D3D11_TEXTURE2D_DESC texdescDepth;
	ZeroMemory(&texdescDepth, sizeof(D3D11_TEXTURE2D_DESC));
	texdescDepth.Width = texdesc.Width;
	texdescDepth.Height = texdesc.Height;
	texdescDepth.MipLevels = 1;
	texdescDepth.ArraySize = 1;
	texdescDepth.Format = depthfmttex;
	texdescDepth.SampleDesc.Count = 1;
	texdescDepth.SampleDesc.Quality = 0;
	texdescDepth.Usage = D3D11_USAGE_DEFAULT;
	texdescDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	device->CreateTexture2D(&texdescDepth, nullptr, &mDepthTexture);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvdesc;
	ZeroMemory(&dsvdesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
	dsvdesc.Format = depthfmtdsv;
	dsvdesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvdesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(mDepthTexture.Get(), &dsvdesc, 
		mDepthBuffer.GetAddressOf());

	ZeroMemory(&srvdesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	srvdesc.Format = depthfmtsrv;
	srvdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvdesc.Texture2D.MostDetailedMip = 0;
	srvdesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(mDepthTexture.Get(), &srvdesc, 
		mDepthResource.GetAddressOf());


	// Viewport
	ZeroMemory(&mViewport, sizeof(D3D11_VIEWPORT));
	mViewport.TopLeftX = 0.0F;
	mViewport.TopLeftY = 0.0F;
	mViewport.Width = (FLOAT)texdesc.Width;
	mViewport.Height = (FLOAT)texdesc.Height;
	mViewport.MinDepth = 0.0F;
	mViewport.MaxDepth = 1.0F;


	// Initial buffer clear
	mContext->ClearRenderTargetView(mColorBuffer.Get(), DirectX::Colors::Black);
	mContext->ClearDepthStencilView(mDepthBuffer.Get(), 
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}


void FrameBuffer::Clear()
{
	mContext->ClearRenderTargetView(mColorBuffer.Get(), DirectX::Colors::Black);
	mContext->ClearDepthStencilView(mDepthBuffer.Get(), 
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void FrameBuffer::Clear(const math::Color& color)
{
	mContext->ClearRenderTargetView(mColorBuffer.Get(), color);
	mContext->ClearDepthStencilView(mDepthBuffer.Get(), 
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

