#include "shader.h"

#include <cstdint>
#include <d3dcompiler.h>

#include "sampler.h"
#include "texture.h"
#include "utility.h"


using namespace skincut;



Shader::Shader(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, 
	const std::wstring vsfile, const std::wstring psfile) : mDevice(device), mContext(context)
{
	mStencilRef = 0;

	ComPtr<ID3DBlob> vsblob;
	ComPtr<ID3DBlob> psblob;

	// vertex shader
	if (vsfile.empty())
	{
		std::string vsstr = 
			"float4 main(float4 pos : POSITION) : SV_POSITION\n"
			"{\n"
			"	return pos;\n"
			"}\n";

		HREXCEPT(D3DCompile(vsstr.c_str(), vsstr.size(), "VS", nullptr, 
			nullptr, "main", "vs_4_0", 0, 0, vsblob.GetAddressOf(), nullptr));
	}
	else
	{
		HREXCEPT(D3DReadFileToBlob(vsfile.c_str(), vsblob.GetAddressOf()));
		InitializeConstantBuffers(vsblob, mVertexBuffers);
	}

	InitializeInputLayout(vsblob);
	HREXCEPT(mDevice->CreateVertexShader(vsblob->GetBufferPointer(), 
		vsblob->GetBufferSize(), nullptr, mVertexShader.GetAddressOf()));


	// pixel shader
	mPixelShader = nullptr;

	if (!psfile.empty())
	{
		HREXCEPT(D3DReadFileToBlob(psfile.c_str(), psblob.GetAddressOf()));
		InitializeConstantBuffers(psblob, mPixelBuffers);
		HREXCEPT(mDevice->CreatePixelShader(psblob->GetBufferPointer(), 
			psblob->GetBufferSize(), nullptr, mPixelShader.GetAddressOf()));
	}

	InitializeBlendState();
	InitializeDepthState();
}


void Shader::InitializeInputLayout(ComPtr<ID3DBlob>& vsblob)
{
	// Reflect shader info
	ComPtr<ID3D11ShaderReflection> shaderReflection;
	HREXCEPT(D3DReflect(vsblob->GetBufferPointer(), vsblob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)shaderReflection.GetAddressOf()));

	// Get shader info
	D3D11_SHADER_DESC shaderDesc;
	shaderReflection->GetDesc(&shaderDesc);

	// Read input layout description from shader info
	std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDesc;

	for (uint32_t i = 0; i < shaderDesc.InputParameters; ++i)
	{
		D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
		shaderReflection->GetInputParameterDesc(i, &paramDesc);

		// fill out input element desc
		D3D11_INPUT_ELEMENT_DESC elementDesc;
		elementDesc.SemanticName = paramDesc.SemanticName;
		elementDesc.SemanticIndex = paramDesc.SemanticIndex;
		elementDesc.InputSlot = 0;
		elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate = 0;

		// determine DXGI format
		if (paramDesc.Mask == 1)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) 
				elementDesc.Format = DXGI_FORMAT_R32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				elementDesc.Format = DXGI_FORMAT_R32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
		}
		else if (paramDesc.Mask <= 3)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
		}
		else if (paramDesc.Mask <= 7)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		}
		else if (paramDesc.Mask <= 15)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
			{
				elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

				if (strcmp(elementDesc.SemanticName, "POSITION") == 0) // hack for float3 position definitions
					elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
			}
		}

		// save element desc
		inputLayoutDesc.push_back(elementDesc);
	}       

	// try to create input layout
	HREXCEPT(mDevice->CreateInputLayout(&inputLayoutDesc[0], static_cast<UINT>(inputLayoutDesc.size()), 
		vsblob->GetBufferPointer(), vsblob->GetBufferSize(), mInputLayout.GetAddressOf()));
}


void Shader::InitializeConstantBuffers(ComPtr<ID3DBlob>& blob, std::vector<ComPtr<ID3D11Buffer>>& buffers)
{
	// Reflect shader info
	ComPtr<ID3D11ShaderReflection> shaderReflection;
	HREXCEPT(D3DReflect(blob->GetBufferPointer(), blob->GetBufferSize(), 
		IID_ID3D11ShaderReflection, (void**)shaderReflection.GetAddressOf()));

	// Get shader info
	D3D11_SHADER_DESC shaderDesc;
	shaderReflection->GetDesc(&shaderDesc);

	for (uint8_t i = 0; i < shaderDesc.ConstantBuffers; ++i)
	{
		auto srcb = shaderReflection->GetConstantBufferByIndex(i);
		D3D11_SHADER_BUFFER_DESC shaderBufferDesc;
		srcb->GetDesc(&shaderBufferDesc);

		// vertex shader constant buffer
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.ByteWidth = shaderBufferDesc.Size;
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		ComPtr<ID3D11Buffer> buffer;
		HREXCEPT(mDevice->CreateBuffer(&bufferDesc, nullptr, buffer.GetAddressOf()));
		buffers.push_back(buffer);
	}
}


D3D11_BLEND_DESC Shader::DefaultBlendDesc()
{
	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(blendDesc));
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	for (uint8_t i = 0; i < 8; ++i)
	{
		blendDesc.RenderTarget[i].BlendEnable = FALSE;
		blendDesc.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[i].DestBlend = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	}
	return blendDesc;
}

D3D11_DEPTH_STENCIL_DESC Shader::DefaultDepthDesc()
{
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilDesc.StencilEnable = FALSE;
	depthStencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	depthStencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	return depthStencilDesc;
}



void Shader::InitializeBlendState()
{
	mBlendMask = 0xFFFFFFFF;
	mBlendFactor = DirectX::Colors::Black;

	D3D11_BLEND_DESC blendDesc = DefaultBlendDesc();
	HREXCEPT(mDevice->CreateBlendState(&blendDesc, 
		mBlendState.GetAddressOf()));
}

void Shader::InitializeDepthState()
{
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = DefaultDepthDesc();
	HREXCEPT(mDevice->CreateDepthStencilState(&depthStencilDesc, 
		mDepthState.GetAddressOf()));
}
 

void Shader::SetBlendState(D3D11_BLEND srcBlend, D3D11_BLEND destBlend, 
	D3D11_BLEND_OP blendOp, const float* factor, UINT mask)
{
	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(blendDesc));
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	for (uint8_t i = 0; i < 8; ++i)
	{
		blendDesc.RenderTarget[i].BlendEnable = 
			(srcBlend != D3D11_BLEND_ONE) || (destBlend != D3D11_BLEND_ZERO);
		blendDesc.RenderTarget[i].SrcBlend = srcBlend;
		blendDesc.RenderTarget[i].SrcBlendAlpha = srcBlend;
		blendDesc.RenderTarget[i].DestBlend = destBlend;
		blendDesc.RenderTarget[i].DestBlendAlpha = destBlend;
		blendDesc.RenderTarget[i].BlendOp = blendOp;
		blendDesc.RenderTarget[i].BlendOpAlpha = blendOp;
		blendDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	}

	HREXCEPT(mDevice->CreateBlendState(&blendDesc, 
		mBlendState.ReleaseAndGetAddressOf()));
	mBlendFactor = factor;
	mBlendMask = mask;
}

void Shader::SetBlendState(D3D11_BLEND_DESC desc, const float* factor, UINT mask)
{
	mDevice->CreateBlendState(&desc, 
		mBlendState.ReleaseAndGetAddressOf());
	mBlendFactor = factor;
	mBlendMask = mask;
}


void Shader::SetDepthState(bool enableDepth, bool writeDepth, bool enableStencil, unsigned int ref)
{
	D3D11_DEPTH_STENCIL_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.DepthEnable = enableDepth;
	desc.DepthWriteMask = writeDepth ? 
		D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	desc.StencilEnable = enableStencil;
	desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	desc.BackFace = desc.FrontFace;
	mDevice->CreateDepthStencilState(&desc, 
		mDepthState.ReleaseAndGetAddressOf());

	mStencilRef = ref;
}

void Shader::SetDepthState(D3D11_DEPTH_STENCIL_DESC desc, unsigned int ref)
{
	mDevice->CreateDepthStencilState(&desc, 
		mDepthState.ReleaseAndGetAddressOf());
	mStencilRef = ref;
}

