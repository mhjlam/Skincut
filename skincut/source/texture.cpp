#include "texture.h"
#include "utility.h"

#include "DirectXTK/DDSTextureLoader.h"


using namespace skincut;



Texture::Texture(ComPtr<ID3D11Device>& device, UINT width, UINT height, 
	DXGI_FORMAT format, D3D11_USAGE usage, UINT bindflags)
{
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = usage;
	desc.BindFlags = bindflags;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	device->CreateTexture2D(&desc, nullptr, mTexture.GetAddressOf());
}

Texture::Texture(ComPtr<ID3D11Device>& device, UINT width, UINT height, 
	DXGI_FORMAT format, D3D11_USAGE usage, UINT bindflags, const D3D11_SUBRESOURCE_DATA* data)
{
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = usage;
	desc.BindFlags = bindflags;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	device->CreateTexture2D(&desc, data, mTexture.GetAddressOf());
}

Texture::Texture(ComPtr<ID3D11Device>& device, std::string path, 
	D3D11_USAGE usage, UINT bindflags, UINT cpuflags, UINT miscflags, bool forceRGB)
{
	std::wstring wpath(path.begin(), path.end());

	ComPtr<ID3D11Resource> resource;
	DirectX::CreateDDSTextureFromFileEx(device.Get(), wpath.c_str(), 0, 
		usage, bindflags, 0, 0, forceRGB, resource.GetAddressOf(), mShaderResource.GetAddressOf());
	mShaderResource->GetResource(reinterpret_cast<ID3D11Resource**>(mTexture.GetAddressOf()));
}

Texture::Texture(ComPtr<ID3D11Device>& device, D3D11_TEXTURE2D_DESC desc)
{
	device->CreateTexture2D(&desc, nullptr, mTexture.GetAddressOf());
}

Texture::Texture(ComPtr<ID3D11Device>& device, 
	D3D11_TEXTURE2D_DESC desc, const D3D11_SUBRESOURCE_DATA* data)
{
	device->CreateTexture2D(&desc, data, mTexture.GetAddressOf());
}

