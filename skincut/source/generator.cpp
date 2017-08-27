#include "generator.h"

#include <d3dcompiler.h>
#include <wincodec.h>
#include "DirectXTex/DirectXTex.h"
#include "DirectXTK/DDSTextureLoader.h"

#include "model.h"
#include "camera.h"
#include "shader.h"
#include "target.h"
#include "utility.h"
#include "structures.h"
#include "mathematics.h"
#include "vertexbuffer.h"




using Microsoft::WRL::ComPtr;
using namespace skincut;
using namespace skincut::math;



namespace skincut
{
	extern Configuration gConfig;
}


Generator::Generator(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context) : mDevice(device), mContext(context)
{
	auto shaderpath = [&](std::wstring name)
	{
		std::wstring resdir(gConfig.ResourcePath.begin(), gConfig.ResourcePath.end());
		return resdir + L"shaders/" + name;
	};

	mShaderStretch = std::make_shared<Shader>(mDevice, mContext, shaderpath(L"stretch.vs.cso"), shaderpath(L"stretch.ps.cso"));
	mShaderWoundPatch = std::make_shared<Shader>(mDevice, mContext, shaderpath(L"pass.vs.cso"), shaderpath(L"patch.ps.cso"));
}


// std::shared_ptr<Target> Generator::GenerateBeckmann(std::wstring outname)
// {
// 	D3D11_TEXTURE2D_DESC texDesc;
// 	ZeroMemory(&texDesc, sizeof(texDesc));
// 	texDesc.ArraySize           = 1;
// 	texDesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
// 	texDesc.CPUAccessFlags      = 0;
// 	texDesc.Format              = DXGI_FORMAT_R32_FLOAT;
// 	texDesc.Height              = 512;
// 	texDesc.Width               = 512;
// 	texDesc.MipLevels           = 1;
// 	texDesc.MiscFlags           = 0;
// 	texDesc.SampleDesc.Count    = 1;
// 	texDesc.SampleDesc.Quality  = 0;
// 	texDesc.Usage               = D3D11_USAGE_IMMUTABLE;
// 
// 	float* data = new float[512 * 512];
// 
// 	for (uint32_t x = 0; x < 512; ++x)
// 	{
// 		for (uint32_t y = 0; y < 512; ++y)
// 		{
// 			float ndoth = float(x) / float(512);
// 			float m = float(y) / float(512);
// 
// 			// Convert the 0.0..1.0 ranges to be -1.0..+1.0
// 			//ndoth = 2.0f * ndoth - 1.0f;
// 
// 			// Evaluate a Beckmann distribution for this element of the look-up table:
// 			float r_sq = m * m;
// 			float r_a = 1.0f / (4.0f * r_sq * pow(ndoth, 4));
// 			float r_b = ndoth * ndoth - 1.0f;
// 			float r_c = r_sq * ndoth * ndoth;
// 
// 			data[x + y * 512] = r_a * expf(r_b / r_c);
// 
// 			//float ph = 1.0f / (m*m * pow(ndoth, 4));
// 			//float ta = std::tanf(std::acosf(ndoth));
// 
// 			//data[x + y * 512] = ph * expf(-((ta*ta)/(m*m)));
// 		}
// 	}
// 
// 	D3D11_SUBRESOURCE_DATA initialData;
// 	initialData.pSysMem = (void*)data;
// 	initialData.SysMemPitch = sizeof(float) * 512;
// 	initialData.SysMemSlicePitch = 0;
// 	delete data;
// 
// 	ComPtr<ID3D11Texture2D> texture = nullptr;
// 	HREXCEPT(mDevice->CreateTexture2D(&texDesc, &initialData, &texture));
// 
// 
// 	if (!outname.empty())
// 	{
// 		std::wstring ddsname = outname + L".dds";
// 		std::wstring pngname = outname + L".png";
// 
// 		DirectX::ScratchImage image;
// 		HREXCEPT(DirectX::CaptureTexture(mDevice.Get(), mContext.Get(), texture.Get(), image));
// 		HREXCEPT(DirectX::SaveToDDSFile(*image.GetImage(0, 0, 0), 
// 			DirectX::DDS_FLAGS_NONE, ddsname.c_str()));
// 		HREXCEPT(DirectX::SaveToWICFile(*image.GetImage(0, 0, 0), 
// 			DirectX::WIC_FLAGS_NONE, GUID_ContainerFormatPng, pngname.c_str(), nullptr));
// 	}
// }


std::shared_ptr<Target> Generator::GenerateStretch(std::shared_ptr<Model>& model, std::wstring outname)
{
	/*
	STRETCH CORRECTION MAP
	After that a map is rendered that holds information about how much the UV layout is stretched, 
	compared to the real dimensions. This is done by unwrapping the mesh into texture space (see below) 
	and saving the the screen space derivatives of the world coordinates for U and V direction, 
	which gives a rough estimation of the stretching. 

	Rendering in texture space:
	To project the mesh into texture space, a vertex shader is used. The position of each vertex 
	is set to its UV coordinate and the world coordinates are stored in an extra texture coordinate. 
	This way the mesh is rendered in its UV layout and the fragment shader can still access the "real" 
	vertex position for lighting and things like that.
	*/

	struct VS_CBUFFER_DATA
	{
		math::Matrix World;
		math::Matrix WorldInverse;
		math::Matrix WorldViewProjection;
	};

	struct PS_CBUFFER_DATA
	{
		math::Color Color;
	};

	auto target = std::make_shared<Target>(mDevice, mContext, 512, 512, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);

	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0.0F;
	viewport.TopLeftY = 0.0F;
	viewport.Width = 512.0F;
	viewport.Height = 512.0F;
	viewport.MinDepth = 0.0F;
	viewport.MaxDepth = 1.0F;

	ComPtr<ID3D11Buffer> vscbuffer;
	VS_CBUFFER_DATA vscbdata = { model->mMatrixWorld, model->mMatrixWorld.Invert().Transpose(), model->mMatrixWVP };
	D3D11_BUFFER_DESC vscbdesc = { sizeof(vscbdata), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
	D3D11_SUBRESOURCE_DATA vscbres = { &vscbdata, 0, 0 };
	mDevice->CreateBuffer(&vscbdesc, &vscbres, &vscbuffer);

	ComPtr<ID3D11Buffer> pscbuffer;
	PS_CBUFFER_DATA pscbdata = { math::Color(1,0,1,1) };
	D3D11_BUFFER_DESC pscbdesc = { sizeof(pscbdata), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
	D3D11_SUBRESOURCE_DATA pscbres = { &pscbdata, 0, 0 };
	mDevice->CreateBuffer(&pscbdesc, &pscbres, &pscbuffer);
	
	mContext->IASetInputLayout(mShaderStretch->mInputLayout.Get());
	mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mContext->IASetIndexBuffer(model->mIndexBuffer.Get(), model->mIndexBufferFormat, model->mIndexBufferOffset);
	mContext->IASetVertexBuffers(0, 1, model->mVertexBuffer.GetAddressOf(), &model->mVertexBufferStrides, &model->mVertexBufferOffset);
	mContext->VSSetConstantBuffers(0, 1, vscbuffer.GetAddressOf());
	mContext->PSSetConstantBuffers(0, 1, pscbuffer.GetAddressOf());
	mContext->VSSetShader(mShaderStretch->mVertexShader.Get(), 0, 0);
	mContext->PSSetShader(mShaderStretch->mPixelShader.Get(), 0, 0);
	mContext->RSSetState(nullptr); // maybe need to change CullMode to NONE
	mContext->RSSetViewports(1, &viewport);
	mContext->OMSetRenderTargets(1, target->mRenderTarget.GetAddressOf(), nullptr);
	
	mContext->DrawIndexed(model->IndexCount(), 0, 0);

	
	if (!outname.empty())
	{
		std::wstring ddsname = outname + L".dds";
		std::wstring pngname = outname + L".png";

		ComPtr<ID3D11Resource> tempres;
		ComPtr<ID3D11Texture2D> temptex;
		target->mShaderResource.Get()->GetResource(tempres.GetAddressOf());
		tempres.Get()->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(temptex.GetAddressOf()));

		DirectX::ScratchImage image;
		CaptureTexture(mDevice.Get(), mContext.Get(), temptex.Get(), image);
		//SaveToDDSFile(*image.GetImage(0, 0, 0), DirectX::DDS_FLAGS_NONE, ddsname.c_str());
		SaveToWICFile(*image.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE, GUID_ContainerFormatPng, pngname.c_str(), nullptr);
	}

	return target;
}


std::shared_ptr<Target> Generator::GenerateWoundPatch(UINT width, UINT height, std::wstring outname)
{
	D3D11_MAPPED_SUBRESOURCE msr;
	HREXCEPT(mContext->Map(mShaderWoundPatch->mPixelBuffers[0].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
	CB_PATCH_PS* cbps = (CB_PATCH_PS*)msr.pData;
	cbps->Discolor = Color(0.58, 0.26, 0.29, 1.00); // float4(0.58, 0.27, 0.28, 1.00);
	cbps->LightColor = Color(0.89, 0.71, 0.65, 1.00); // float4(0.65, 0.36, 0.37, 1.00);
	cbps->InnerColor = Color(0.54, 0.00, 0.01, 1.00);
	cbps->OffsetX = Utility::Random(0.0f, 100.0f);
	cbps->OffsetY = Utility::Random(0.0f, 100.0f);
	mContext->Unmap(mShaderWoundPatch->mPixelBuffers[0].Get(), 0);

	auto buffer = std::make_unique<VertexBuffer>(mDevice);
	auto target = std::make_shared<Target>(mDevice, mContext, width, height, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, false);

	mContext->IASetInputLayout(mShaderWoundPatch->mInputLayout.Get());
	mContext->IASetPrimitiveTopology(buffer->mTopology);
	mContext->IASetVertexBuffers(0, 1, buffer->mBuffer.GetAddressOf(), &buffer->mStrides, &buffer->mOffsets);

	mContext->VSSetShader(mShaderWoundPatch->mVertexShader.Get(), nullptr, 0);

	mContext->PSSetConstantBuffers(0, static_cast<UINT>(mShaderWoundPatch->mPixelBuffers.size()), mShaderWoundPatch->mPixelBuffers[0].GetAddressOf());
	mContext->PSSetShader(mShaderWoundPatch->mPixelShader.Get(), nullptr, 0);

	mContext->RSSetState(nullptr); // default rasterizer
	mContext->RSSetViewports(1, &target->mViewport);

	mContext->OMSetRenderTargets(1, target->mRenderTarget.GetAddressOf(), nullptr);
	mContext->OMSetBlendState(target->mBlendState.Get(), target->mBlendFactor, target->mSampleMask);
	mContext->OMSetDepthStencilState(mShaderWoundPatch->mDepthState.Get(), 0);

	mContext->Draw(buffer->mVertexCount, 0);

	mContext->OMSetRenderTargets(0, nullptr, nullptr);

	
	if (!outname.empty())
	{
		std::wstring ddsname = outname + L".dds";
		std::wstring pngname = outname + L".png";

		ComPtr<ID3D11Resource> tempres;
		ComPtr<ID3D11Texture2D> temptex;
		target->mShaderResource.Get()->GetResource(tempres.GetAddressOf());
		tempres.Get()->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(temptex.GetAddressOf()));

		DirectX::ScratchImage image;
		HREXCEPT(DirectX::CaptureTexture(mDevice.Get(), mContext.Get(), temptex.Get(), image));
		//HREXCEPT(DirectX::SaveToDDSFile(*image.GetImage(0, 0, 0), DirectX::DDS_FLAGS_NONE, ddsname.c_str()));
		HREXCEPT(DirectX::SaveToWICFile(*image.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE, GUID_ContainerFormatPng, pngname.c_str(), nullptr));
	}

	return target;
}

