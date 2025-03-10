#pragma once

#include <vector>

#include <d3d11.h>
#include <wrl/client.h>


using Microsoft::WRL::ComPtr;



struct ImDrawList;


namespace SkinCut
{
	class Light;
	struct Configuration;
	extern Configuration gConfig;


	class Dashboard
	{
	private:
		struct CUSTOMVERTEX
		{
			float pos[2];
			float tex[2];
			unsigned int col;
		};

		struct VERTEX_CONSTANT_BUFFER
		{
			float mvp[4][4];
		};


		HWND  mHwnd;
		INT64 mTime;
		INT64 mTicksPerSecond;
		static const int cVertexBufferSize;

		static ComPtr<ID3D11Device> mDevice;
		static ComPtr<ID3D11DeviceContext> mContext;

		static ComPtr<ID3DBlob> mVertexShaderBlob;
		static ComPtr<ID3DBlob> mPixelShaderBlob;
		static ComPtr<ID3D11Buffer> mVertexBuffer;
		static ComPtr<ID3D11Buffer> mVertexConstantBuffer;
		static ComPtr<ID3D11InputLayout> mInputLayout;
		static ComPtr<ID3D11VertexShader> mVertexShader;
		static ComPtr<ID3D11PixelShader> mPixelShader;
		static ComPtr<ID3D11SamplerState> mFontSampler;
		static ComPtr<ID3D11BlendState> mBlendState;
		static ComPtr<ID3D11ShaderResourceView> mFontTextureView;


	public:
		Dashboard(HWND hwnd, ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context);
		~Dashboard();

		void Update();
		void Render(std::vector<Light*>& lights);

		static void RenderDrawLists(ImDrawList** const cmdList, int numCmdList);
	};

}

