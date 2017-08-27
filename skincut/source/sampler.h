#pragma once

#include <d3d11.h>
#include <wrl/client.h>


using Microsoft::WRL::ComPtr;



namespace skincut
{
	class Sampler
	{
	public:
		ComPtr<ID3D11SamplerState> mSamplerState;

	public:
		Sampler(ComPtr<ID3D11Device>& device, D3D11_SAMPLER_DESC samplerDesc);
		Sampler(ComPtr<ID3D11Device>& device, D3D11_FILTER filter, D3D11_COMPARISON_FUNC compfunc);
		Sampler(ComPtr<ID3D11Device>& device, D3D11_FILTER filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
											  D3D11_TEXTURE_ADDRESS_MODE address = D3D11_TEXTURE_ADDRESS_CLAMP,
											  D3D11_COMPARISON_FUNC compfunc = D3D11_COMPARISON_NEVER, UINT anisotropy = 1);

	public:
		static D3D11_SAMPLER_DESC Point();
		static D3D11_SAMPLER_DESC Linear();
		static D3D11_SAMPLER_DESC Anisotropic();
		static D3D11_SAMPLER_DESC Comparison();
	};
}

