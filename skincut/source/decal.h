#pragma once

#include <memory>
#include <dxgi.h>
#include <d3d11.h>
#include <wrl/client.h>

#include "mathematics.h"


using Microsoft::WRL::ComPtr;



namespace skincut
{

	class Texture;


	class Decal
	{
	public:
		unsigned int				mIndexCount;
		unsigned int				mVertexCount;

		// index buffer
		ComPtr<ID3D11Buffer>		mIndexBuffer;
		unsigned int				mIndexBufferSize;
		unsigned int				mIndexBufferOffset;
		DXGI_FORMAT					mIndexBufferFormat;

		// vertex buffer
		ComPtr<ID3D11Buffer>		mVertexBuffer;
		unsigned int				mVertexBufferSize;
		unsigned int				mVertexBufferStrides;
		unsigned int				mVertexBufferOffset;

		// misc
		math::Vector3				mNormal;
		math::Matrix				mWorldMatrix;
		std::shared_ptr<Texture>	mTexture;


	public:
		Decal(ComPtr<ID3D11Device>& device, std::shared_ptr<Texture>& texture, math::Matrix world = math::Matrix(), math::Vector3 normal = math::Vector3());
	};
}
