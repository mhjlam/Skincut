#include "Utility.hpp"

#include <string>
#include <locale>
#include <random>
#include <codecvt>
#include <sstream>
#include <iostream>
#include <functional>

#include <wincodec.h>

#include "DirectXTex/DirectXTex.h"
#include "DirectXTK/Inc/DDSTextureLoader.h"


using namespace SkinCut;



float Utility::Random(float min, float max)
{
	std::random_device device;
	auto engine = std::default_random_engine(device());

	// return uniformly distributed float between min and max
	std::uniform_real_distribution<float> distr(min, max);
	return distr(engine);
}

std::vector<float> Utility::Random(unsigned int n, float min, float max)
{
	std::random_device device;
	auto engine = std::default_random_engine(device());

	std::uniform_real_distribution<float> distr(min, max);

	std::vector<float> output(n);
	for (auto& f : output) {
		f = distr(engine);
	}
	return output;
}



DirectX::XMFLOAT4X4A Utility::MatrixMultiply(DirectX::XMFLOAT4X4 m0, DirectX::XMFLOAT4X4 m1)
{
	DirectX::XMMATRIX mat0 = DirectX::XMLoadFloat4x4(&m0);
	DirectX::XMMATRIX mat1 = DirectX::XMLoadFloat4x4(&m1);
	DirectX::XMMATRIX res = DirectX::XMMatrixMultiply(mat0, mat1);

	DirectX::XMFLOAT4X4A result{};
	DirectX::XMStoreFloat4x4(&result, res);
	return result;
}


DirectX::XMFLOAT4X4A Utility::MatrixMultiply(DirectX::XMFLOAT4X4 m0, DirectX::XMFLOAT4X4 m1, DirectX::XMFLOAT4X4 m2)
{
	DirectX::XMMATRIX mat0 = DirectX::XMLoadFloat4x4(&m0);
	DirectX::XMMATRIX mat1 = DirectX::XMLoadFloat4x4(&m1);
	DirectX::XMMATRIX mat2 = DirectX::XMLoadFloat4x4(&m2);
	DirectX::XMMATRIX res = DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(mat0, mat1), mat2);

	DirectX::XMFLOAT4X4A result{};
	DirectX::XMStoreFloat4x4(&result, res);
	return result;
}

DirectX::XMFLOAT4X4A Utility::MatrixInverse(DirectX::XMFLOAT4X4 m)
{
	DirectX::XMMATRIX mat = DirectX::XMLoadFloat4x4(&m);
	DirectX::XMMATRIX res = DirectX::XMMatrixInverse(nullptr, mat);

	DirectX::XMFLOAT4X4A result{};
	DirectX::XMStoreFloat4x4(&result, res);
	return result;
}



void Utility::ConsoleMessage(std::string msg)
{
	std::stringstream ss;
	ss << msg << std::endl;

	std::cout << ss.str();
	OutputDebugStringA(ss.str().c_str());
}

void Utility::ConsoleMessageW(std::wstring msg)
{
	std::wstringstream ss;
	ss << msg << std::endl;

	std::wcout << ss.str();
	OutputDebugStringW(ss.str().c_str());
}

void Utility::DialogMessage(std::string msg)
{
	MessageBoxA(nullptr, msg.c_str(), "Error", MB_ICONERROR | MB_OK);
}

void Utility::DialogMessageW(std::wstring msg)
{
	MessageBoxW(nullptr, msg.c_str(), L"Error", MB_ICONERROR | MB_OK);
}

int Utility::ErrorMessage(std::exception& e)
{
	std::stringstream ss;
	ss << "Critical error: " << e.what() << "." << std::endl << "Reload model?";
	return MessageBoxA(nullptr, ss.str().c_str(), "Error", MB_ICONERROR | MB_YESNOCANCEL);
}



bool Utility::CompareString(std::string str0, std::string str1)
{
	return (_stricmp(str0.c_str(), str1.c_str()) == 0);
}

bool Utility::CompareString(std::wstring str0, std::wstring str1)
{
	return (_wcsicmp(str0.c_str(), str1.c_str()) == 0);
}


std::string Utility::wstr2str(const std::wstring& wstr)
{
	// Convert wide Unicode string to an UTF8 string
	if (wstr.empty()) { return std::string(); }

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.to_bytes(wstr);
}

std::wstring Utility::str2wstr(const std::string& str)
{
	if (str.empty()) { return std::wstring(); }

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(str);
}


void Utility::GetTextureDim(ComPtr<ID3D11Resource>& resource, uint32_t& width, uint32_t& height)
{
	D3D11_RESOURCE_DIMENSION dim;
	resource->GetType(&dim);

	switch (dim) {
		case D3D11_RESOURCE_DIMENSION_TEXTURE2D: {
			auto texture = reinterpret_cast<ID3D11Texture2D*>(resource.Get());
			D3D11_TEXTURE2D_DESC desc;
			texture->GetDesc(&desc);
			width = desc.Width;
			height = desc.Height;
			break;
		}

		default: {
			width = 0;
			height = 0;
			break;
		}
	}
}


bool Utility::ValidCopy(ComPtr<ID3D11Texture2D>& src, ComPtr<ID3D11Texture2D>& dst)
{
	std::function<DXGI_FORMAT_GROUP(DXGI_FORMAT)> FormatGroup = [](DXGI_FORMAT format) {
		switch (format) {
			case DXGI_FORMAT_R32G32B32A32_TYPELESS:
			case DXGI_FORMAT_R32G32B32A32_FLOAT:
			case DXGI_FORMAT_R32G32B32A32_UINT:
			case DXGI_FORMAT_R32G32B32A32_SINT:
			case DXGI_FORMAT_BC2_UNORM:
			case DXGI_FORMAT_BC2_UNORM_SRGB:
			case DXGI_FORMAT_BC3_UNORM:
			case DXGI_FORMAT_BC3_UNORM_SRGB:
			case DXGI_FORMAT_BC5_UNORM:
			case DXGI_FORMAT_BC5_SNORM: {
				return DXGI_FORMAT_GROUP_RGBA32;
			}

			case DXGI_FORMAT_R16G16B16A16_TYPELESS:
			case DXGI_FORMAT_R16G16B16A16_FLOAT:
			case DXGI_FORMAT_R16G16B16A16_UNORM:
			case DXGI_FORMAT_R16G16B16A16_UINT:
			case DXGI_FORMAT_R16G16B16A16_SNORM:
			case DXGI_FORMAT_R16G16B16A16_SINT:
			case DXGI_FORMAT_BC1_UNORM:
			case DXGI_FORMAT_BC1_UNORM_SRGB:
			case DXGI_FORMAT_BC4_UNORM:
			case DXGI_FORMAT_BC4_SNORM: {
				return DXGI_FORMAT_GROUP_RGBA16;
			}

			case DXGI_FORMAT_R8G8B8A8_TYPELESS:
			case DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			case DXGI_FORMAT_R8G8B8A8_UINT:
			case DXGI_FORMAT_R8G8B8A8_SNORM:
			case DXGI_FORMAT_R8G8B8A8_SINT: {
				return DXGI_FORMAT_GROUP_RGBA8;
			}

			case DXGI_FORMAT_R32G32B32_TYPELESS:
			case DXGI_FORMAT_R32G32B32_FLOAT:
			case DXGI_FORMAT_R32G32B32_UINT:
			case DXGI_FORMAT_R32G32B32_SINT: {
				return DXGI_FORMAT_GROUP_RGB32;
			}

			case DXGI_FORMAT_R32G32_TYPELESS:
			case DXGI_FORMAT_R32G32_FLOAT:
			case DXGI_FORMAT_R32G32_UINT:
			case DXGI_FORMAT_R32G32_SINT: {
				return DXGI_FORMAT_GROUP_RG32;
			}

			case DXGI_FORMAT_R16G16_TYPELESS:
			case DXGI_FORMAT_R16G16_FLOAT:
			case DXGI_FORMAT_R16G16_UNORM:
			case DXGI_FORMAT_R16G16_UINT:
			case DXGI_FORMAT_R16G16_SNORM:
			case DXGI_FORMAT_R16G16_SINT: {
				return DXGI_FORMAT_GROUP_RG16;
			}

			case DXGI_FORMAT_R8G8_TYPELESS:
			case DXGI_FORMAT_R8G8_UNORM:
			case DXGI_FORMAT_R8G8_UINT:
			case DXGI_FORMAT_R8G8_SNORM:
			case DXGI_FORMAT_R8G8_SINT: {
				return DXGI_FORMAT_GROUP_RG8;
			}

			case DXGI_FORMAT_R32_TYPELESS:
			case DXGI_FORMAT_D32_FLOAT:
			case DXGI_FORMAT_R32_FLOAT:
			case DXGI_FORMAT_R32_UINT:
			case DXGI_FORMAT_R32_SINT:
			case DXGI_FORMAT_R9G9B9E5_SHAREDEXP: {
				return DXGI_FORMAT_GROUP_R32;
			}

			case DXGI_FORMAT_R16_TYPELESS:
			case DXGI_FORMAT_R16_FLOAT:
			case DXGI_FORMAT_D16_UNORM:
			case DXGI_FORMAT_R16_UNORM:
			case DXGI_FORMAT_R16_UINT:
			case DXGI_FORMAT_R16_SNORM:
			case DXGI_FORMAT_R16_SINT: {
				return DXGI_FORMAT_GROUP_R16;
			}

			case DXGI_FORMAT_R8_TYPELESS:
			case DXGI_FORMAT_R8_UNORM:
			case DXGI_FORMAT_R8_UINT:
			case DXGI_FORMAT_R8_SNORM:
			case DXGI_FORMAT_R8_SINT:
			case DXGI_FORMAT_A8_UNORM: {
				return DXGI_FORMAT_GROUP_R8;
			}

			case DXGI_FORMAT_R24G8_TYPELESS:
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
			case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
			case DXGI_FORMAT_X24_TYPELESS_G8_UINT: {
				return DXGI_FORMAT_GROUP_R24G8;
			}

			default: {
				return DXGI_FORMAT_GROUP_UNKNOWN;
			}
		}
	};


	// must be different resources
	if (src == dst) { return false; }

	D3D11_TEXTURE2D_DESC srcDesc;
	D3D11_TEXTURE2D_DESC dstDesc;
	src->GetDesc(&srcDesc);
	dst->GetDesc(&dstDesc);

	// copy resource has numerous restrictions:
	// https://msdn.microsoft.com/en-us/library/windows/desktop/ff476392(v=vs.85).aspx

	// must have identical dimensions (width, height, depth, size)
	if (srcDesc.Width != dstDesc.Width) { return false; }
	if (srcDesc.Height != dstDesc.Height) { return false; }
	if (srcDesc.ArraySize != dstDesc.ArraySize) { return false; }

	// must have compatible formats
	DXGI_FORMAT_GROUP srcFormatGroup = FormatGroup(srcDesc.Format);
	if (srcFormatGroup == DXGI_FORMAT_GROUP_UNKNOWN) { return false; }

	DXGI_FORMAT_GROUP dstFormatGroup = FormatGroup(dstDesc.Format);
	if (dstFormatGroup == DXGI_FORMAT_GROUP_UNKNOWN) { return false; }
	if (srcFormatGroup != dstFormatGroup) { return false; }

	return true;
}


ComPtr<ID3D11Resource> 
Utility::GetResource(ComPtr<ID3D11ShaderResourceView>& srv)
{
	ComPtr<ID3D11Resource> resource;
	srv->GetResource(resource.GetAddressOf());
	return resource;
}


ComPtr<ID3D11Texture1D>
Utility::GetTexture1D(ComPtr<ID3D11ShaderResourceView>& srv)
{
	ComPtr<ID3D11Resource> resource;
	ComPtr<ID3D11Texture1D> texture;
	srv->GetResource(resource.GetAddressOf());
	HREXCEPT(resource.Get()->QueryInterface(__uuidof(ID3D11Texture1D), 
		reinterpret_cast<void**>(texture.GetAddressOf())));
	return texture;
}

ComPtr<ID3D11Texture2D>
Utility::GetTexture2D(ComPtr<ID3D11ShaderResourceView>& srv)
{
	ComPtr<ID3D11Resource> resource;
	ComPtr<ID3D11Texture2D> texture;
	srv->GetResource(resource.GetAddressOf());
	HREXCEPT(resource.Get()->QueryInterface(__uuidof(ID3D11Texture2D), 
		reinterpret_cast<void**>(texture.GetAddressOf())));
	return texture;
}

ComPtr<ID3D11Texture3D>
Utility::GetTexture3D(ComPtr<ID3D11ShaderResourceView>& srv)
{
	ComPtr<ID3D11Resource> resource;
	ComPtr<ID3D11Texture3D> texture;
	srv->GetResource(resource.GetAddressOf());
	HREXCEPT(resource.Get()->QueryInterface(__uuidof(ID3D11Texture3D), 
		reinterpret_cast<void**>(texture.GetAddressOf())));
	return texture;
}


void Utility::GetTexture1D(ComPtr<ID3D11ShaderResourceView>& srv, ComPtr<ID3D11Texture1D>& tex, D3D11_TEXTURE1D_DESC& desc)
{
	ComPtr<ID3D11Resource> resource;
	srv->GetResource(resource.GetAddressOf());
	HREXCEPT(resource.Get()->QueryInterface(__uuidof(ID3D11Texture1D), 
		reinterpret_cast<void**>(tex.GetAddressOf())));
	tex->GetDesc(&desc);
}

void Utility::GetTexture2D(ComPtr<ID3D11ShaderResourceView>& srv, ComPtr<ID3D11Texture2D>& tex, D3D11_TEXTURE2D_DESC& desc)
{
	ComPtr<ID3D11Resource> resource;
	srv->GetResource(resource.GetAddressOf());
	HREXCEPT(resource.Get()->QueryInterface(__uuidof(ID3D11Texture2D), 
		reinterpret_cast<void**>(tex.GetAddressOf())));
	tex->GetDesc(&desc);
}

void Utility::GetTexture3D(ComPtr<ID3D11ShaderResourceView>& srv, ComPtr<ID3D11Texture3D>& texture, D3D11_TEXTURE3D_DESC& desc)
{
	ComPtr<ID3D11Resource> resource;
	srv->GetResource(resource.GetAddressOf());
	HREXCEPT(resource.Get()->QueryInterface(__uuidof(ID3D11Texture3D), 
		reinterpret_cast<void**>(texture.GetAddressOf())));
	texture->GetDesc(&desc);
}



void Utility::SaveTexture(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, 
	ComPtr<ID3D11Resource>& resource, LPCWSTR filename)
{
	DirectX::ScratchImage image;
	ComPtr<ID3D11Texture2D> texture;
	HREXCEPT(resource.Get()->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(texture.GetAddressOf())));

	if (SUCCEEDED(CaptureTexture(device.Get(), context.Get(), texture.Get(), image))) {
		SaveToWICFile(*image.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE, GUID_ContainerFormatPng, filename, nullptr);
	}
}

void Utility::SaveTexture(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, ComPtr<ID3D11Texture2D>& texture, LPCWSTR fileName)
{
	DirectX::ScratchImage image;
	if (FAILED(CaptureTexture(device.Get(), context.Get(), texture.Get(), image))) { return; }
	if (FAILED(SaveToWICFile(*image.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE, GUID_ContainerFormatPng, fileName, nullptr))) { return; }
}

void Utility::SaveTexture(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, ComPtr<ID3D11ShaderResourceView>& srv, LPCWSTR fileName)
{
	DirectX::ScratchImage image;
	ComPtr<ID3D11Resource> resource;
	ComPtr<ID3D11Texture2D> texture;
	srv->GetResource(&resource);

	HREXCEPT(resource.Get()->QueryInterface(__uuidof(ID3D11Texture2D), 
		reinterpret_cast<void**>(texture.GetAddressOf())));

	if (SUCCEEDED(CaptureTexture(device.Get(), context.Get(), texture.Get(), image))) {
		SaveToWICFile(*image.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE, GUID_ContainerFormatPng, fileName, nullptr);
	}
}


ComPtr<ID3D11ShaderResourceView> Utility::LoadTexture(ComPtr<ID3D11Device>& device, const std::wstring& name, const bool srgb)
{
	ComPtr<ID3D11ShaderResourceView> srv;
	HMODULE mod = GetModuleHandle(nullptr);
	HRSRC hrsrc = FindResource(mod, name.c_str(), RT_RCDATA);

	if (hrsrc) { // resource exists
		HGLOBAL res = LoadResource(mod, hrsrc);
		size_t size = SizeofResource(mod, hrsrc);
		LPCVOID src = LockResource(res);

		DirectX::DDS_LOADER_FLAGS ddsLoaderFlags = DirectX::DDS_LOADER_DEFAULT;
		if (srgb) {
			ddsLoaderFlags = DirectX::DDS_LOADER_FORCE_SRGB;
		}

		HREXCEPT(DirectX::CreateDDSTextureFromMemoryEx(device.Get(), (uint8_t*)src, size, 0,
			D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, ddsLoaderFlags, nullptr, srv.GetAddressOf()));
	}
	else { // try to load from file
		WIN32_FIND_DATA finddata;
		HANDLE handle = FindFirstFile(name.c_str(), &finddata);

		if (handle != INVALID_HANDLE_VALUE) // found
		{
			FindClose(handle);

			DirectX::DDS_LOADER_FLAGS ddsLoaderFlags = DirectX::DDS_LOADER_DEFAULT;
			if (srgb) {
				ddsLoaderFlags = DirectX::DDS_LOADER_FORCE_SRGB;
			}

			if (FAILED(DirectX::CreateDDSTextureFromFileEx(device.Get(), name.c_str(), 0, 
				D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, ddsLoaderFlags, nullptr, srv.GetAddressOf()))) {
					throw std::exception("Texture load error");
			}
		}
		else
		{
			const std::wstring name2 = std::wstring(L"Resources\\") + name;
			HANDLE handle = FindFirstFile(name2.c_str(), &finddata);

			if (handle != INVALID_HANDLE_VALUE) {
				FindClose(handle);

				DirectX::DDS_LOADER_FLAGS ddsLoaderFlags = DirectX::DDS_LOADER_DEFAULT;
				if (srgb) {
					ddsLoaderFlags = DirectX::DDS_LOADER_FORCE_SRGB;
				}

				if (FAILED(DirectX::CreateDDSTextureFromFileEx(device.Get(), name2.c_str(), 0, 
					D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, ddsLoaderFlags, nullptr, srv.GetAddressOf()))) {
					throw std::exception("Texture load error");
				}
			}
			else {
				FindClose(handle);
				throw std::exception("Texture load error");
			}
		}
	}

	return srv;
}

