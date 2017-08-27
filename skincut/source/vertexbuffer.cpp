#include "vertexbuffer.h"

#include "utility.h"
#include "structures.h"


using namespace skincut;
using namespace skincut::math;



VertexBuffer::VertexBuffer(ComPtr<ID3D11Device>& device) 
	: mDevice(device)
{
	VertexPositionTexture vbdata[] =
	{
		{ math::Vector3(-1, -1, 0), math::Vector2(0, 1) },
		{ math::Vector3(-1,  1, 0), math::Vector2(0, 0) },
		{ math::Vector3( 1, -1, 0), math::Vector2(1, 1) },
		{ math::Vector3( 1,  1, 0), math::Vector2(1, 0) }
	};

	mVertexCount = _countof(vbdata);
	mOffsets = 0;
	mStrides = sizeof(VertexPositionTexture);
	
	D3D11_BUFFER_DESC bdesc;
	bdesc.ByteWidth =  _countof(vbdata) * sizeof(VertexPositionTexture);
	bdesc.Usage = D3D11_USAGE_DEFAULT;
	bdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bdesc.CPUAccessFlags = 0;
	bdesc.MiscFlags = 0;
	bdesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA srd;
	srd.pSysMem = vbdata;
	srd.SysMemPitch = 0;
	srd.SysMemSlicePitch = 0;

	mTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;

	HREXCEPT(mDevice->CreateBuffer(&bdesc, &srd, mBuffer.GetAddressOf()));
}


VertexBuffer::VertexBuffer(ComPtr<ID3D11Device>& device, std::vector<VertexPositionTexture>& vertices, D3D11_PRIMITIVE_TOPOLOGY topo) 
	: mDevice(device), mTopology(topo)
{
	mVertexCount = static_cast<UINT>(vertices.size());
	mOffsets = 0;
	mStrides = sizeof(VertexPositionTexture);

	D3D11_BUFFER_DESC bdesc;
	bdesc.ByteWidth = mVertexCount * sizeof(VertexPositionTexture);
	bdesc.Usage = D3D11_USAGE_DEFAULT;
	bdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bdesc.CPUAccessFlags = 0;
	bdesc.MiscFlags = 0;
	bdesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA srd;
	srd.pSysMem = &vertices[0];
	srd.SysMemPitch = 0;
	srd.SysMemSlicePitch = 0;

	HREXCEPT(mDevice->CreateBuffer(&bdesc, &srd, mBuffer.GetAddressOf()));
}


VertexBuffer::VertexBuffer(ComPtr<ID3D11Device>& device, Vector2 position, Vector2 scale) 
	: mDevice(device)
{
	Matrix translation = Matrix::CreateTranslation(Vector3(position.x, position.y, 0.0f));
	Matrix scaling = Matrix::CreateScale(Vector3(scale.x, scale.y, 0.0f));
	Matrix transform = scaling * translation;

	VertexPositionTexture vbdata[] =
	{
		{ math::Vector3(-1, -1, 0), math::Vector2(0, 1) },
		{ math::Vector3(-1,  1, 0), math::Vector2(0, 0) },
		{ math::Vector3( 1, -1, 0), math::Vector2(1, 1) },
		{ math::Vector3( 1,  1, 0), math::Vector2(1, 0) }
	};

	for (auto v : vbdata)
	{
		Vector3::Transform(v.position, transform);
		Vector2::Transform(v.texcoord, transform);
	}

	mVertexCount = _countof(vbdata);
	mOffsets = 0;
	mStrides = sizeof(VertexPositionTexture);

	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.ByteWidth =  _countof(vbdata) * sizeof(VertexPositionTexture);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA subresourceData;
	subresourceData.pSysMem = vbdata;
	subresourceData.SysMemPitch = 0;
	subresourceData.SysMemSlicePitch = 0;

	mTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;

	HREXCEPT(mDevice->CreateBuffer(&bufferDesc, &subresourceData, mBuffer.GetAddressOf()));
}


VertexBuffer::VertexBuffer(ComPtr<ID3D11Device>& device, math::Vector2 position, math::Vector2 scale, std::vector<VertexPositionTexture>& vertices, D3D11_PRIMITIVE_TOPOLOGY topo)
	: mDevice(device), mTopology(topo)
{
	Matrix translation = Matrix::CreateTranslation(Vector3(position.x, position.y, 0.0f));
	Matrix scaling = Matrix::CreateScale(Vector3(scale.x, scale.y, 0.0f));
	Matrix transform = scaling * translation;

	for (auto v : vertices)
	{
		Vector3::Transform(v.position, transform);
		Vector2::Transform(v.texcoord, transform);
	}

	mVertexCount = static_cast<UINT>(vertices.size());
	mOffsets = 0;
	mStrides = sizeof(VertexPositionTexture);

	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.ByteWidth = mVertexCount * sizeof(VertexPositionTexture);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA subresourceData;
	subresourceData.pSysMem = &vertices[0];
	subresourceData.SysMemPitch = 0;
	subresourceData.SysMemSlicePitch = 0;

	HREXCEPT(mDevice->CreateBuffer(&bufferDesc, &subresourceData, mBuffer.GetAddressOf()));
}


void VertexBuffer::SetVertices(std::vector<VertexPositionTexture>& vertices)
{
	mVertexCount = static_cast<UINT>(vertices.size());

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.ByteWidth = mVertexCount * sizeof(VertexPositionTexture);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA subresourceData;
	ZeroMemory(&subresourceData, sizeof(D3D11_SUBRESOURCE_DATA));
	subresourceData.pSysMem = &vertices[0];

	HREXCEPT(mDevice->CreateBuffer(&bufferDesc, &subresourceData, mBuffer.ReleaseAndGetAddressOf()));
}

