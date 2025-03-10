#include "Texture.hpp"

#include "DirectXTK/Inc/DDSTextureLoader.h"

#include "Utility.hpp"


using namespace SkinCut;



Texture::Texture(ComPtr<ID3D11Device>& device, uint32_t width, uint32_t height, DXGI_FORMAT format, D3D11_USAGE usage, uint32_t bindFlags)
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
	desc.BindFlags = bindFlags;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	device->CreateTexture2D(&desc, nullptr, mTexture.GetAddressOf());
}

Texture::Texture(ComPtr<ID3D11Device>& device, uint32_t width, uint32_t height, DXGI_FORMAT format, D3D11_USAGE usage, uint32_t bindFlags, const D3D11_SUBRESOURCE_DATA* data)
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
	desc.BindFlags = bindFlags;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	device->CreateTexture2D(&desc, data, mTexture.GetAddressOf());
}

Texture::Texture(ComPtr<ID3D11Device>& device, std::string path, D3D11_USAGE usage, uint32_t bindFlags, uint32_t cpuFlags, uint32_t miscFlags, bool forceRgb)
{
	std::wstring wpath(path.begin(), path.end());

	DirectX::DDS_LOADER_FLAGS ddsLoaderFlags = DirectX::DDS_LOADER_DEFAULT;
	if (forceRgb) {
		ddsLoaderFlags = DirectX::DDS_LOADER_FORCE_SRGB;
	}

	ComPtr<ID3D11Resource> resource;
	DirectX::CreateDDSTextureFromFileEx(device.Get(), wpath.c_str(), 0, 
		usage, bindFlags, 0, 0, ddsLoaderFlags, resource.GetAddressOf(), mShaderResource.GetAddressOf());
	mShaderResource->GetResource(reinterpret_cast<ID3D11Resource**>(mTexture.GetAddressOf()));
}

Texture::Texture(ComPtr<ID3D11Device>& device, D3D11_TEXTURE2D_DESC desc)
{
	device->CreateTexture2D(&desc, nullptr, mTexture.GetAddressOf());
}

Texture::Texture(ComPtr<ID3D11Device>& device, D3D11_TEXTURE2D_DESC desc, const D3D11_SUBRESOURCE_DATA* data)
{
	device->CreateTexture2D(&desc, data, mTexture.GetAddressOf());
}

