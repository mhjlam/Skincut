#include "Dashboard.hpp"

#include <sstream>

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <wrl/client.h>

#include <d3d11.h>
#include <d3dcompiler.h>

#include "ImGui/imgui.h"

#include "Light.hpp"
#include "Utility.hpp"
#include "Structures.hpp"
#include "Mathematics.hpp"


using Microsoft::WRL::ComPtr;
using namespace SkinCut;



namespace SkinCut {
	extern Configuration gConfig;
}

const int Dashboard::cVertexBufferSize = 30000;

ComPtr<ID3D11Device> Dashboard::mDevice;
ComPtr<ID3D11DeviceContext> Dashboard::mContext;

ComPtr<ID3DBlob> Dashboard::mVertexShaderBlob;
ComPtr<ID3DBlob> Dashboard::mPixelShaderBlob;
ComPtr<ID3D11Buffer> Dashboard::mVertexBuffer;
ComPtr<ID3D11Buffer> Dashboard::mVertexConstantBuffer;
ComPtr<ID3D11InputLayout> Dashboard::mInputLayout;
ComPtr<ID3D11VertexShader> Dashboard::mVertexShader;
ComPtr<ID3D11PixelShader> Dashboard::mPixelShader;
ComPtr<ID3D11SamplerState> Dashboard::mFontSampler;
ComPtr<ID3D11BlendState> Dashboard::mBlendState;
ComPtr<ID3D11ShaderResourceView> Dashboard::mFontTextureView;



Dashboard::Dashboard(HWND hwnd, ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context)
{
	mHwnd = hwnd;
	mDevice = device;
	mContext = context;

	QueryPerformanceCounter((LARGE_INTEGER*)&mTime);
	QueryPerformanceFrequency((LARGE_INTEGER*)&mTicksPerSecond);

	// keyboard mapping
	ImGuiIO& io = ImGui::GetIO();
	io.KeyMap[ImGuiKey_Tab] = VK_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
	io.KeyMap[ImGuiKey_DownArrow] = VK_UP;
	io.KeyMap[ImGuiKey_Home] = VK_HOME;
	io.KeyMap[ImGuiKey_End] = VK_END;
	io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
	io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
	io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
	io.KeyMap[ImGuiKey_A] = 'A';
	io.KeyMap[ImGuiKey_C] = 'C';
	io.KeyMap[ImGuiKey_V] = 'V';
	io.KeyMap[ImGuiKey_X] = 'X';
	io.KeyMap[ImGuiKey_Y] = 'Y';
	io.KeyMap[ImGuiKey_Z] = 'Z';

	io.RenderDrawListsFn = RenderDrawLists;
	io.ImeWindowHandle = mHwnd;
	

	auto shaderpath = [&](std::wstring name)
	{
		std::wstring resourcePath = Utility::str2wstr(gConfig.ResourcePath);
		return resourcePath + L"Shaders\\" + name;
	};

	HREXCEPT(D3DReadFileToBlob(shaderpath(L"imguivs.cso").c_str(), mVertexShaderBlob.GetAddressOf()));
	HREXCEPT(mDevice->CreateVertexShader(mVertexShaderBlob->GetBufferPointer(), 
		mVertexShaderBlob->GetBufferSize(), nullptr, mVertexShader.GetAddressOf()));

	HREXCEPT(D3DReadFileToBlob(shaderpath(L"imguips.cso").c_str(), mPixelShaderBlob.GetAddressOf()));
	HREXCEPT(mDevice->CreatePixelShader(mPixelShaderBlob->GetBufferPointer(), 
		mPixelShaderBlob->GetBufferSize(), nullptr, mPixelShader.GetAddressOf()));


	// Create the input layout
	D3D11_INPUT_ELEMENT_DESC localLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, (size_t)(&((CUSTOMVERTEX*)0)->pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,	 0, (size_t)(&((CUSTOMVERTEX*)0)->col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,		 0, (size_t)(&((CUSTOMVERTEX*)0)->tex), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	HREXCEPT(mDevice->CreateInputLayout(localLayout, 3, mVertexShaderBlob->GetBufferPointer(), 
		mVertexShaderBlob->GetBufferSize(), mInputLayout.GetAddressOf()));

	// Create the constant buffer
	D3D11_BUFFER_DESC cbDesc{};
	cbDesc.ByteWidth = sizeof(VERTEX_CONSTANT_BUFFER);
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	mDevice->CreateBuffer(&cbDesc, nullptr, mVertexConstantBuffer.GetAddressOf());

	// Create the blending setup
	D3D11_BLEND_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.AlphaToCoverageEnable = false;
	desc.RenderTarget[0].BlendEnable = true;
	desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	mDevice->CreateBlendState(&desc, mBlendState.GetAddressOf());

	// Create the vertex buffer
	D3D11_BUFFER_DESC bufferDesc;
	memset(&bufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = cVertexBufferSize * sizeof(CUSTOMVERTEX);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	HREXCEPT(mDevice->CreateBuffer(&bufferDesc, nullptr, mVertexBuffer.GetAddressOf()));


	// Font texture
	{
		ImGuiIO& io = ImGui::GetIO();

		// Build
		unsigned char* pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

		// Create DX11 texture
		D3D11_TEXTURE2D_DESC texDesc;
		ZeroMemory(&texDesc, sizeof(texDesc));
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.SampleDesc.Count = 1;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;

		ComPtr<ID3D11Texture2D> pTexture;
		D3D11_SUBRESOURCE_DATA data{};
		data.pSysMem = pixels;
		data.SysMemPitch = texDesc.Width * 4;
		data.SysMemSlicePitch = 0;
		mDevice->CreateTexture2D(&texDesc, &data, pTexture.GetAddressOf());

		// Create texture view
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		mDevice->CreateShaderResourceView(pTexture.Get(), &srvDesc, mFontTextureView.GetAddressOf());

		// Store our identifier
		io.Fonts->TexID = (void*)mFontTextureView.Get();

		// Create texture sampler
		D3D11_SAMPLER_DESC samplerDesc;
		ZeroMemory(&samplerDesc, sizeof(samplerDesc));
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.MipLODBias = 0.f;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		samplerDesc.MinLOD = 0.f;
		samplerDesc.MaxLOD = 0.f;
		mDevice->CreateSamplerState(&samplerDesc, mFontSampler.GetAddressOf());
	}
}

Dashboard::~Dashboard()
{
	mFontSampler = nullptr;
	mFontTextureView = nullptr;
	mVertexBuffer = nullptr;
	mBlendState = nullptr;
	mPixelShader = nullptr;
	mPixelShaderBlob = nullptr;
	mVertexConstantBuffer = nullptr;
	mInputLayout = nullptr;
	mVertexShader = nullptr;
	mVertexShaderBlob = nullptr;

	ImGui::Shutdown();
}


void Dashboard::Update()
{
	ImGuiIO& io = ImGui::GetIO();

	// Setup display size (every frame to accommodate for window resizing)
	RECT rect;
	GetClientRect(mHwnd, &rect);
	io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

	// Setup time step
	INT64 current_time{};
	QueryPerformanceCounter((LARGE_INTEGER*)&current_time); 
	io.DeltaTime = (float)(current_time - mTime) / mTicksPerSecond;
	mTime = current_time;

	// Read keyboard modifiers inputs
	io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
	io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
	io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
	// io.KeysDown : filled by WM_KEYDOWN/WM_KEYUP events
	// io.MousePos : filled by WM_MOUSEMOVE events
	// io.MouseDown : filled by WM_*BUTTON* events
	// io.MouseWheel : filled by WM_MOUSEWHEEL events

	// Start the frame
	ImGui::NewFrame();
}


void Dashboard::Render(std::vector<Light*>& lights)
{
	if (!mVertexBuffer) {
		return;
	}

	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiSetCond_FirstUseEver);

	ImGui::Begin("Settings");
	{
		if (ImGui::CollapsingHeader("Renderer", "idRenderer", true, true)) {
			int renderMode = ToInt(gConfig.RenderMode);
			ImGui::Combo("", &renderMode, "Kelemen/Szirmay-Kalos\0Blinn-Phong\0Lambertian reflectance");
		}

		if (ImGui::CollapsingHeader("Features", "idFeatures", true, true)) {
			ImGui::Checkbox("Wireframe", &gConfig.EnableWireframe);
			ImGui::Checkbox("Color mapping", &gConfig.EnableColor);
			ImGui::Checkbox("Normal mapping", &gConfig.EnableBumps);
			ImGui::Checkbox("Shadow mapping", &gConfig.EnableShadows);
			ImGui::Checkbox("Specular mapping", &gConfig.EnableSpeculars);
			ImGui::Checkbox("Occlusion mapping", &gConfig.EnableOcclusion);
			ImGui::Checkbox("Irradiance mapping", &gConfig.EnableIrradiance);
			ImGui::Checkbox("Subsurface scattering", &gConfig.EnableScattering);
		}

		if (ImGui::CollapsingHeader("Shading", "idShading", true, true)) {
			ImGui::SliderFloat("Ambient", &gConfig.Ambient, 0.0f, 1.0f);
			ImGui::SliderFloat("Fresnel", &gConfig.Fresnel, 0.0f, 1.0f);
			ImGui::SliderFloat("Bumpiness", &gConfig.Bumpiness, 0.0f, 1.0f);
			ImGui::SliderFloat("Roughness", &gConfig.Roughness, 0.0f, 1.0f);
			ImGui::SliderFloat("Specularity", &gConfig.Specularity, 0.0f, 2.0f);
			ImGui::SliderFloat("Scattering", &gConfig.Scattering, 0.0f, 0.1f);
			ImGui::SliderFloat("Translucency", &gConfig.Translucency, 0.0f, 1.0f);
		}

		if (ImGui::CollapsingHeader("Lights", "idLights", true, true)) {
			for (auto light : lights) {
				ImGui::SliderFloat(light->mName.c_str(), &light->mBrightness, 0.0f, 1.0f);
			}
		}

		ImGui::Separator();
		ImGui::Text("FPS: %.1f (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
	}
	ImGui::End();

	ImGui::Render();
}


void Dashboard::RenderDrawLists(ImDrawList** const cmdLists, int numCmdLists)
{
	// Copy and convert all vertices into a single contiguous buffer
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	if (FAILED(mContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) return;

	CUSTOMVERTEX* vertexDst = (CUSTOMVERTEX*)mappedResource.pData;
	for (int n = 0; n < numCmdLists; n++)
	{
		const ImDrawList* cmdList = cmdLists[n];
		const ImDrawVert* vertexSrc = &cmdList->vtx_buffer[0];
		for (size_t i = 0; i < cmdList->vtx_buffer.size(); i++)
		{
			vertexDst->pos[0] = vertexSrc->pos.x;
			vertexDst->pos[1] = vertexSrc->pos.y;
			vertexDst->tex[0] = vertexSrc->uv.x;
			vertexDst->tex[1] = vertexSrc->uv.y;
			vertexDst->col = vertexSrc->col;
			vertexDst++;
			vertexSrc++;
		}
	}
	mContext->Unmap(mVertexBuffer.Get(), 0);

	// Setup orthographic projection matrix into our constant buffer
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		if (FAILED(mContext->Map(mVertexConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
			return;
		}

		VERTEX_CONSTANT_BUFFER* pConstantBuffer = (VERTEX_CONSTANT_BUFFER*)mappedResource.pData;
		const float L = 0.0f;
		const float R = ImGui::GetIO().DisplaySize.x;
		const float B = ImGui::GetIO().DisplaySize.y;
		const float T = 0.0f;
		const float MVP[4][4] = {
			{ 2.0f/(R-L),   0.0f,           0.0f,       0.0f },
			{ 0.0f,         2.0f/(T-B),     0.0f,       0.0f },
			{ 0.0f,         0.0f,           0.5f,       0.0f },
			{ (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
		};
		memcpy(&pConstantBuffer->mvp, MVP, sizeof(MVP));
		mContext->Unmap(mVertexConstantBuffer.Get(), 0);
	}

	// Setup viewport
	D3D11_VIEWPORT viewPort;
	memset(&viewPort, 0, sizeof(D3D11_VIEWPORT));
	viewPort.Width = ImGui::GetIO().DisplaySize.x;
	viewPort.Height = ImGui::GetIO().DisplaySize.y;
	viewPort.MinDepth = 0.0f;
	viewPort.MaxDepth = 1.0f;
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	mContext->RSSetViewports(1, &viewPort);

	// Bind shader and vertex buffers
	unsigned int stride = sizeof(CUSTOMVERTEX);
	unsigned int offset = 0;
	mContext->IASetInputLayout(mInputLayout.Get());
	mContext->IASetVertexBuffers(0, 1, mVertexBuffer.GetAddressOf(), &stride, &offset);
	mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mContext->VSSetShader(mVertexShader.Get(), nullptr, 0);
	mContext->VSSetConstantBuffers(0, 1, mVertexConstantBuffer.GetAddressOf());
	mContext->PSSetShader(mPixelShader.Get(), nullptr, 0);
	mContext->PSSetSamplers(0, 1, mFontSampler.GetAddressOf());

	// Setup render state
	const float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
	mContext->OMSetBlendState(mBlendState.Get(), blendFactor, 0xffffffff);

	// Render command lists
	int vertexOffset = 0;
	for (int n = 0; n < numCmdLists; n++) 	{
		const ImDrawList* cmdList = cmdLists[n];
		for (size_t cmd_i = 0; cmd_i < cmdList->commands.size(); cmd_i++) {
			const ImDrawCmd* pcmd = &cmdList->commands[cmd_i];
			if (pcmd->user_callback) {
				pcmd->user_callback(cmdList, pcmd);
			}
			else {
				const D3D11_RECT rect = {
					(long)pcmd->clip_rect.x,
					(long)pcmd->clip_rect.y, 
					(long)pcmd->clip_rect.z, 
					(long)pcmd->clip_rect.w
				};

				mContext->PSSetShaderResources(0, 1, (ID3D11ShaderResourceView**)&pcmd->texture_id);
				mContext->RSSetScissorRects(1, &rect); 
				mContext->Draw(pcmd->vtx_count, vertexOffset);
			}
			vertexOffset += pcmd->vtx_count;
		}
	}

	// Restore modified state
	mContext->IASetInputLayout(nullptr);
	mContext->PSSetShader(nullptr, nullptr, 0);
	mContext->VSSetShader(nullptr, nullptr, 0);
}

