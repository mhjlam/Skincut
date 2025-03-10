#pragma once

#include <vector>
#include <d3d11.h>
#include <wrl/client.h>

#include "structures.h"
#include "mathematics.h"


using Microsoft::WRL::ComPtr;



namespace skincut
{
	class VertexBuffer
	{
	private:
		ComPtr<ID3D11Device> mDevice;

	public:
		UINT mVertexCount;
		UINT mOffsets;
		UINT mStrides;
		D3D11_PRIMITIVE_TOPOLOGY mTopology;
		ComPtr<ID3D11Buffer> mBuffer;

		VertexBuffer(ComPtr<ID3D11Device>& device);
		VertexBuffer(ComPtr<ID3D11Device>& device, std::vector<VertexPositionTexture>& vertices, D3D11_PRIMITIVE_TOPOLOGY topo);
		VertexBuffer(ComPtr<ID3D11Device>& device, math::Vector2 position, math::Vector2 scale);
		VertexBuffer(ComPtr<ID3D11Device>& device, math::Vector2 position, math::Vector2 scale, std::vector<VertexPositionTexture>& vertices, D3D11_PRIMITIVE_TOPOLOGY topo);

		void SetVertices(std::vector<VertexPositionTexture>& vertices);
	};
}

