#pragma once

#include <cfloat>
#include <string>
#include <d3d11.h>
#include <wrl/client.h>


using Microsoft::WRL::ComPtr;



namespace skincut
{
	class Texture
	{
	public:
		ComPtr<ID3D11Texture2D> mTexture;
		ComPtr<ID3D11ShaderResourceView> mShaderResource;


	public:
		Texture(ComPtr<ID3D11Device>& device,
				UINT width, UINT height,
				DXGI_FORMAT format,
				D3D11_USAGE usage,
				UINT bindflags);

		Texture(ComPtr<ID3D11Device>& device,
				UINT width, UINT height,
				DXGI_FORMAT format,
				D3D11_USAGE usage,
				UINT bindflags,
				const D3D11_SUBRESOURCE_DATA* data);

		Texture(ComPtr<ID3D11Device>& device,
				std::string path,
				D3D11_USAGE usage,
				UINT bindflags,
				UINT cpuflags,
				UINT miscflags,
				bool forceRGB);

		Texture(ComPtr<ID3D11Device>& device,
				D3D11_TEXTURE2D_DESC desc);

		Texture(ComPtr<ID3D11Device>& device,
				D3D11_TEXTURE2D_DESC desc,
				const D3D11_SUBRESOURCE_DATA* data);
	};
}

