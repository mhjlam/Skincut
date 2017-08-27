#include "decal.h"

#include "texture.h"
#include "utility.h"
#include "structures.h"


using namespace skincut;
using namespace skincut::math;



Decal::Decal(ComPtr<ID3D11Device>& device, 
			 std::shared_ptr<Texture>& texture, 
			 Matrix world, Vector3 normal) : 
	mTexture(texture), mWorldMatrix(world), mNormal(normal)
{
	// vertex buffer
	VertexPosition vertexes[] = 
	{
		{ Vector3(-0.5f,  0.5f, -0.5f) },
		{ Vector3( 0.5f,  0.5f, -0.5f) },
		{ Vector3( 0.5f,  0.5f,  0.5f) },
		{ Vector3(-0.5f,  0.5f,  0.5f) },
		{ Vector3(-0.5f, -0.5f, -0.5f) },
		{ Vector3( 0.5f, -0.5f, -0.5f) },
		{ Vector3( 0.5f, -0.5f,  0.5f) },
		{ Vector3(-0.5f, -0.5f,  0.5f) }
	};

	mVertexCount = ARRAYSIZE(vertexes);
	mVertexBufferSize = mVertexCount * sizeof(VertexPosition);
	mVertexBufferStrides = sizeof(VertexPosition);
	mVertexBufferOffset = 0;

	D3D11_BUFFER_DESC vbdesc;
	ZeroMemory(&vbdesc, sizeof(D3D11_BUFFER_DESC));
	vbdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbdesc.ByteWidth = mVertexBufferSize;
	vbdesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA vbdata;
	ZeroMemory(&vbdata, sizeof(D3D11_SUBRESOURCE_DATA));
	vbdata.pSysMem = &vertexes[0];

	HREXCEPT(device->CreateBuffer(&vbdesc, &vbdata, &mVertexBuffer));


	// index buffer
	UINT indexes[] =
	{
		3, 1, 0,   2, 1, 3,
		0, 5, 4,   1, 5, 0,
		3, 4, 7,   0, 4, 3,
		1, 6, 5,   2, 6, 1,
		2, 7, 6,   3, 7, 2,
		6, 4, 5,   7, 4, 6
	};

	mIndexCount = ARRAYSIZE(indexes);
	mIndexBufferSize = mIndexCount * sizeof(UINT);
	mIndexBufferOffset = 0;
	mIndexBufferFormat = DXGI_FORMAT_R32_UINT;

	D3D11_BUFFER_DESC ibdesc;
	ZeroMemory(&ibdesc, sizeof(D3D11_BUFFER_DESC));
	ibdesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibdesc.ByteWidth = mIndexBufferSize;
	ibdesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA ibdata;
	ZeroMemory(&ibdata, sizeof(D3D11_SUBRESOURCE_DATA));
	ibdata.pSysMem = indexes;


	HREXCEPT(device->CreateBuffer(&ibdesc, &ibdata, &mIndexBuffer));
}

