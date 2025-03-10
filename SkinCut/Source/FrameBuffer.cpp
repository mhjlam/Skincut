#include "FrameBuffer.hpp"
#include "Utility.hpp"


using namespace SkinCut;



FrameBuffer::FrameBuffer(ComPtr<ID3D11Device>& device, ComPtr<ID3D11Context>& context, ComPtr<IDXGISwapChain>& swapChain, 
						 DXGI_FORMAT depthFormatTex, DXGI_FORMAT depthFormatDsv, DXGI_FORMAT depthFormatSrv) 
	: mDevice(device), mContext(context)
{
	// Color buffer
	swapChain->GetBuffer(0, __uuidof(mColorTexture), reinterpret_cast<void**>(mColorTexture.GetAddressOf()));

	D3D11_TEXTURE2D_DESC desc;
	mColorTexture->GetDesc(&desc);
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	D3D11_RENDER_TARGET_VIEW_DESC colorBufferRtvDesc{};
	colorBufferRtvDesc.Format = desc.Format;
	colorBufferRtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	colorBufferRtvDesc.Texture2D.MipSlice = 0;
	device->CreateRenderTargetView(mColorTexture.Get(), &colorBufferRtvDesc, mColorBuffer.GetAddressOf());
	
	D3D11_SHADER_RESOURCE_VIEW_DESC colorBufferSrvDesc{};
	colorBufferSrvDesc.Format = desc.Format;
	colorBufferSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	colorBufferSrvDesc.TextureCube.MostDetailedMip = 0;
	colorBufferSrvDesc.TextureCube.MipLevels = 1;
	HREXCEPT(device->CreateShaderResourceView(mColorTexture.Get(), &colorBufferSrvDesc, mColorResource.GetAddressOf()));


	// Depth-stencil buffer
	D3D11_TEXTURE2D_DESC depthStencilTexDesc;
	ZeroMemory(&depthStencilTexDesc, sizeof(D3D11_TEXTURE2D_DESC));
	depthStencilTexDesc.Width = desc.Width;
	depthStencilTexDesc.Height = desc.Height;
	depthStencilTexDesc.MipLevels = 1;
	depthStencilTexDesc.ArraySize = 1;
	depthStencilTexDesc.Format = depthFormatTex;
	depthStencilTexDesc.SampleDesc.Count = 1;
	depthStencilTexDesc.SampleDesc.Quality = 0;
	depthStencilTexDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	device->CreateTexture2D(&depthStencilTexDesc, nullptr, &mDepthTexture);

	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
	depthStencilViewDesc.Format = depthFormatDsv;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(mDepthTexture.Get(), &depthStencilViewDesc, mDepthBuffer.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	ZeroMemory(&shaderResourceViewDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	shaderResourceViewDesc.Format = depthFormatSrv;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(mDepthTexture.Get(), &shaderResourceViewDesc, mDepthResource.GetAddressOf());


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
	mContext->ClearDepthStencilView(mDepthBuffer.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}


FrameBuffer::FrameBuffer(ComPtr<ID3D11Device>& device, ComPtr<ID3D11Context>& context, uint32_t width, uint32_t height, 
						 DXGI_FORMAT colorFormat, DXGI_FORMAT depthFormatTex, DXGI_FORMAT depthFormatDsv, DXGI_FORMAT depthFormatSrv) 
	: mDevice(device), mContext(context)
{
	// Color buffer
	D3D11_TEXTURE2D_DESC texDesc{};
	ZeroMemory(&texDesc, sizeof(D3D11_TEXTURE2D_DESC));
	texDesc.Format = colorFormat;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	HREXCEPT(mDevice->CreateTexture2D(&texDesc, nullptr, &mColorTexture));

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = colorFormat;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	HREXCEPT(mDevice->CreateRenderTargetView(mColorTexture.Get(), &rtvDesc, mColorBuffer.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = colorFormat;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	HREXCEPT(mDevice->CreateShaderResourceView(mColorTexture.Get(), &srvDesc, mColorResource.GetAddressOf()));


	// Depth-stencil buffer
	ZeroMemory(&texDesc, sizeof(D3D11_TEXTURE2D_DESC));
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = depthFormatTex;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	device->CreateTexture2D(&texDesc, nullptr, &mDepthTexture);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ZeroMemory(&dsvDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
	dsvDesc.Format = depthFormatDsv;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(mDepthTexture.Get(), &dsvDesc, mDepthBuffer.GetAddressOf());

	ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	srvDesc.Format = depthFormatSrv;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(mDepthTexture.Get(), &srvDesc, mDepthResource.GetAddressOf());


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
	mContext->ClearDepthStencilView(mDepthBuffer.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}


FrameBuffer::FrameBuffer(ComPtr<ID3D11Device>& device, ComPtr<ID3D11Context>& context, ComPtr<ID3D11Texture2D>& colortex, 
						 DXGI_FORMAT colorFormat, DXGI_FORMAT depthFormatTex, DXGI_FORMAT depthFormatDsv, DXGI_FORMAT depthFormatSrv)
	: mDevice(device), mContext(context)
{
	if (!colortex) throw;

	// Color buffer
	D3D11_TEXTURE2D_DESC texDesc{};
	colortex->GetDesc(&texDesc);
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = colorFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	HREXCEPT(mDevice->CreateTexture2D(&texDesc, nullptr, &mColorTexture));

	auto width = (FLOAT)texDesc.Width;
	auto height = (FLOAT)texDesc.Height;

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = colorFormat;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	HREXCEPT(mDevice->CreateRenderTargetView(mColorTexture.Get(), &rtvDesc, mColorBuffer.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = colorFormat;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	HREXCEPT(mDevice->CreateShaderResourceView(mColorTexture.Get(), &srvDesc, mColorResource.GetAddressOf()));

	// copy texture contents to new texture
	mContext->CopyResource(mColorTexture.Get(), colortex.Get());


	// Depth-stencil buffer
	ZeroMemory(&texDesc, sizeof(D3D11_TEXTURE2D_DESC));
	texDesc.Width = texDesc.Width;
	texDesc.Height = texDesc.Height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = depthFormatTex;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	device->CreateTexture2D(&texDesc, nullptr, &mDepthTexture);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	ZeroMemory(&dsvDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
	dsvDesc.Format = depthFormatDsv;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(mDepthTexture.Get(), &dsvDesc, mDepthBuffer.GetAddressOf());

	ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	srvDesc.Format = depthFormatSrv;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(mDepthTexture.Get(), &srvDesc, mDepthResource.GetAddressOf());


	// Viewport
	ZeroMemory(&mViewport, sizeof(D3D11_VIEWPORT));
	mViewport.TopLeftX = 0.0F;
	mViewport.TopLeftY = 0.0F;
	mViewport.Width = width;
	mViewport.Height = height;
	mViewport.MinDepth = 0.0F;
	mViewport.MaxDepth = 1.0F;


	// Initial buffer clear
	mContext->ClearRenderTargetView(mColorBuffer.Get(), DirectX::Colors::Black);
	mContext->ClearDepthStencilView(mDepthBuffer.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}


void FrameBuffer::Clear()
{
	mContext->ClearRenderTargetView(mColorBuffer.Get(), DirectX::Colors::Black);
	mContext->ClearDepthStencilView(mDepthBuffer.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void FrameBuffer::Clear(const Math::Color& color)
{
	mContext->ClearRenderTargetView(mColorBuffer.Get(), color);
	mContext->ClearDepthStencilView(mDepthBuffer.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

