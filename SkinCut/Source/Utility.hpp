#pragma once

#include <string>
#include <vector>
#include <exception>

#include <wrl/client.h>

#include <d3d11.h>
#include <DirectXColors.h>


using Microsoft::WRL::ComPtr;



namespace SkinCut
{
	// typedefs
	typedef DirectX::XMFLOAT2 float2;
	typedef DirectX::XMFLOAT3 float3;
	typedef DirectX::XMFLOAT4 float4;
	typedef DirectX::XMFLOAT2A float2a;
	typedef DirectX::XMFLOAT3A float3a;
	typedef DirectX::XMFLOAT4A float4a;
	typedef DirectX::XMFLOAT3X3 float3x3;
	typedef DirectX::XMFLOAT4X3 float4x3;
	typedef DirectX::XMFLOAT4X4 float4x4;
	typedef DirectX::XMFLOAT4X3A float4x3a;
	typedef DirectX::XMFLOAT4X4A float4x4a;

	typedef ID3D11DeviceContext ID3D11Context;


	// macros
#ifndef HR
#ifdef _DEBUG
#define HR(x) \
{ \
	HRESULT hr = x; \
	if (FAILED(hr)) \
	{ \
		LPWSTR output; \
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER, \
		              nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &output, 0, nullptr); \
		MessageBox(nullptr, output, L"Error", MB_OK); \
	} \
}
#else
#define HR(x) x;
#endif
#endif


#ifndef HREXCEPT
#ifdef _DEBUG
#define HREXCEPT(x) \
{ \
	HRESULT hr = x; \
	if (FAILED(hr)) \
	{ \
		LPWSTR output; \
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER, \
                      nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &output, 0, nullptr); \
		MessageBox(nullptr, output, L"Error", MB_OK); \
		throw std::exception(); \
	} \
}
#else
#define HREXCEPT(x) \
{ \
	HRESULT hr = x; \
	if (FAILED(hr)) \
	{ \
		throw std::exception(); \
	} \
}
#endif
#endif


#ifndef HRRETURN
#ifdef _DEBUG
#define HRRETURN(x) \
{ \
	HRESULT hr = x; \
	if (FAILED(hr)) \
	{ \
		LPWSTR output; \
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER, \
                      nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &output, 0, nullptr); \
		MessageBox(nullptr, output, L"Error", MB_OK); \
		return hr; \
	} \
}
#else
#define HRRETURN(x) \
{ \
	HRESULT hr = x; \
	if (FAILED(hr)) \
	{ \
		return hr; \
	} \
}
#endif
#endif



	enum DXGI_FORMAT_GROUP
	{
		DXGI_FORMAT_GROUP_UNKNOWN,
		DXGI_FORMAT_GROUP_RGBA32,
		DXGI_FORMAT_GROUP_RGBA16,
		DXGI_FORMAT_GROUP_RGBA8,
		DXGI_FORMAT_GROUP_RGB32,
		DXGI_FORMAT_GROUP_RG32,
		DXGI_FORMAT_GROUP_RG16,
		DXGI_FORMAT_GROUP_RG8,
		DXGI_FORMAT_GROUP_R32,
		DXGI_FORMAT_GROUP_R16,
		DXGI_FORMAT_GROUP_R8,
		DXGI_FORMAT_GROUP_R24G8
	};
	


	namespace Utility
	{
		// Random number generation
		float Random(float min = 0.0f, float max = 1.0f);
		std::vector<float> Random(unsigned int n, float min = 0.0f, float max = 1.0f);

		// Matrix operations
		DirectX::XMFLOAT4X4A MatrixMultiply(DirectX::XMFLOAT4X4 m0, DirectX::XMFLOAT4X4 m1);
		DirectX::XMFLOAT4X4A MatrixMultiply(DirectX::XMFLOAT4X4 m0, DirectX::XMFLOAT4X4 m1, DirectX::XMFLOAT4X4 m2);
		DirectX::XMFLOAT4X4A MatrixInverse(DirectX::XMFLOAT4X4 m);

		// Messages
		void ConsoleMessage(std::string msg = "");
		void ConsoleMessageW(std::wstring msg = L"");
		void DialogMessage(std::string msg = "");
		void DialogMessageW(std::wstring msg = L"");
		int ErrorMessage(std::exception& e);

		// String handling
		bool CompareString(std::string str0, std::string str1);
		bool CompareString(std::wstring str0, std::wstring str1);

		std::string wstr2str(const std::wstring& wstr);
		std::wstring str2wstr(const std::string& str);


		// Texture resources
		void GetTextureDim(ComPtr<ID3D11Resource>& resource, uint32_t& width, uint32_t& height);
		bool ValidCopy(ComPtr<ID3D11Texture2D>& src, ComPtr<ID3D11Texture2D>& dst);

		ComPtr<ID3D11Resource>  GetResource (ComPtr<ID3D11ShaderResourceView>& srv);
		ComPtr<ID3D11Texture1D> GetTexture1D(ComPtr<ID3D11ShaderResourceView>& srv);
		ComPtr<ID3D11Texture2D> GetTexture2D(ComPtr<ID3D11ShaderResourceView>& srv);
		ComPtr<ID3D11Texture3D> GetTexture3D(ComPtr<ID3D11ShaderResourceView>& srv);

		void GetTexture1D(ComPtr<ID3D11ShaderResourceView>& srv, 
						  ComPtr<ID3D11Texture1D>& tex, 
						  D3D11_TEXTURE1D_DESC& desc);
		void GetTexture2D(ComPtr<ID3D11ShaderResourceView>& srv, 
						  ComPtr<ID3D11Texture2D>& tex, 
						  D3D11_TEXTURE2D_DESC& desc);
		void GetTexture3D(ComPtr<ID3D11ShaderResourceView>& srv, 
						  ComPtr<ID3D11Texture3D>& tex,
						  D3D11_TEXTURE3D_DESC& desc);

		void SaveTexture(ComPtr<ID3D11Device>& device, 
						 ComPtr<ID3D11DeviceContext>& context, 
						 ComPtr<ID3D11Resource>& resource, 
						 LPCWSTR filename);
		void SaveTexture(ComPtr<ID3D11Device>& device, 
						 ComPtr<ID3D11DeviceContext>& context, 
						 ComPtr<ID3D11Texture2D>& texture, 
						 LPCWSTR filename);
		void SaveTexture(ComPtr<ID3D11Device>& device, 
						 ComPtr<ID3D11DeviceContext>& context, 
						 ComPtr<ID3D11ShaderResourceView>& srv, 
						 LPCWSTR filename);

		ComPtr<ID3D11ShaderResourceView> LoadTexture(ComPtr<ID3D11Device>& device, 
			const std::wstring& name, const bool srgb = true);
	}
}
