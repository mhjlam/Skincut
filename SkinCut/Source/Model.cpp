#include "model.h"

#include <map>
#include <list>
#include <unordered_map>
#include "SimpleJSON/JSON.h"
#include "DirectXTK/DDSTextureLoader.h"

#include "mesh.h"
#include "utility.h"


using Microsoft::WRL::ComPtr;
using namespace skincut;
using namespace skincut::math;



namespace skincut
{
	extern Configuration gConfig;
}


Model::Model(ComPtr<ID3D11Device>& device, 
			 Vector3 position, 
			 Vector2 rotation, 
			 std::wstring meshpath, 
			 std::wstring colorpath, 
			 std::wstring normalpath, 
			 std::wstring specularpath, 
			 std::wstring discolorpath,
	         std::wstring occlusionpath) : mDevice(device)
{
	mTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	mPosition = position;
	mRotation = rotation;

	mMatrixWVP = Matrix::Identity();
	mMatrixWorld = Matrix::Identity();

	mVertexBufferSize = 0;
	mVertexBufferOffset = 0;
	mVertexBufferStrides = sizeof(Vertex);

	mIndexBufferSize = 0;
	mIndexBufferOffset = 0;
	mIndexBufferFormat = DXGI_FORMAT_R32_UINT;

	mColorWire = Color(0, 0, 0, 1);
	mColorSolid = Color(0, 0, 0, 1);

	mLoadInfo.position = position;
	mLoadInfo.rotation = rotation;
	mLoadInfo.meshpath = meshpath;
	mLoadInfo.colorpath = colorpath;
	mLoadInfo.normalpath = normalpath;
	mLoadInfo.specularpath = specularpath;
	mLoadInfo.discolorpath = discolorpath;
	mLoadInfo.occlusionpath = occlusionpath;

	LoadResources(mLoadInfo);
}


Model::~Model()
{

}


void Model::Update(const Matrix view, const Matrix projection)
{
	mMatrixWVP = mMatrixWorld * view * projection;
}


void Model::Reload()
{
	mMesh.reset();
	mColorMap.Reset();
	mNormalMap.Reset();
	mSpecularMap.Reset();
	mDiscolorMap.Reset();
	mOcclusionMap.Reset();
	LoadResources(mLoadInfo);
}



void Model::LoadResources(ModelLoadInfo li)
{
	mMesh = std::unique_ptr<Mesh>(new Mesh(li.meshpath));
	if (!mMesh) throw std::exception("Could not create mesh!");

	// load texture maps
	mColorMap = Utility::LoadTexture(mDevice, li.colorpath);
	mNormalMap = Utility::LoadTexture(mDevice, li.normalpath);
	mSpecularMap = Utility::LoadTexture(mDevice, li.specularpath);
	mDiscolorMap = Utility::LoadTexture(mDevice, li.discolorpath);
	mOcclusionMap = Utility::LoadTexture(mDevice, li.occlusionpath);

	RebuildBuffers(mMesh->mVertexes, mMesh->mIndexes);
}


void Model::RebuildBuffers(std::vector<Vertex>& vertexes, std::vector<uint32_t>& indexes)
{
	mMesh->RebuildIndexes();

	mVertexBuffer.Reset();
	RebuildVertexBuffer(vertexes);

	mIndexBuffer.Reset();
	RebuildIndexBuffer(indexes);
}


void Model::RebuildVertexBuffer(std::vector<Vertex>& vertexes)
{
	// Set vertex buffer properties.
	mVertexBufferSize = sizeof(Vertex) * static_cast<uint32_t>(vertexes.size());
	mVertexBufferStrides = sizeof(Vertex);
	mVertexBufferOffset = 0;

	// Create dynamic vertex buffer description.
	D3D11_BUFFER_DESC vbdesc;
	vbdesc.Usage = D3D11_USAGE_DYNAMIC;
	vbdesc.ByteWidth = mVertexBufferSize;
	vbdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vbdesc.MiscFlags = 0;
	vbdesc.StructureByteStride = 0;

	// Create subresource data structure for vertex buffer.
	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = &vertexes[0];
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Create vertex buffer.
	HREXCEPT(mDevice->CreateBuffer(&vbdesc, &vertexData, mVertexBuffer.GetAddressOf()));
}


void Model::RebuildIndexBuffer(std::vector<uint32_t>& indexes)
{
	// Set index buffer properties.
	mIndexBufferSize = sizeof(uint32_t) * static_cast<uint32_t>(indexes.size());
	mIndexBufferOffset = 0;

	// Create static index buffer description.
	D3D11_BUFFER_DESC ibdesc;
	ibdesc.Usage = D3D11_USAGE_DEFAULT;
	ibdesc.ByteWidth = mIndexBufferSize;
	ibdesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibdesc.CPUAccessFlags = 0;
	ibdesc.MiscFlags = 0;
	ibdesc.StructureByteStride = 0;

	// Create subresource data structure for index buffer.
	D3D11_SUBRESOURCE_DATA indexData;
	indexData.pSysMem = &indexes[0];
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create index buffer.
	HREXCEPT(mDevice->CreateBuffer(&ibdesc, &indexData, mIndexBuffer.GetAddressOf()));
}



/*******************************************************************************
Mesh operations
*******************************************************************************/

bool Model::RayIntersection(Ray& ray)
{
	return mMesh->RayIntersection(ray);
}

bool Model::RayIntersection(Ray& ray, Intersection& intersection)
{
	return mMesh->RayIntersection(ray, intersection);
}


void Model::Subdivide(Face*& face, SPLITMODE splitmode, Vector3& point)
{
	mMesh->Subdivide(face, splitmode, point); // split a particular face
	RebuildBuffers(mMesh->mVertexes, mMesh->mIndexes);
}


void Model::FormCutline(Intersection& i0, Intersection& i1, std::list<Link>& cutline, Quadrilateral& cutquad)
{
	mMesh->FormCutline(i0, i1, cutline, cutquad);
	if (cutline.empty()) throw std::exception("Unable to form cutting line.");
}

void Model::FuseCutline(std::list<Link>& cutline, std::vector<Edge*>& cutedges)
{
	mMesh->FuseCutline(cutline, cutedges);
	RebuildBuffers(mMesh->mVertexes, mMesh->mIndexes);
}

void Model::OpenCutline(std::vector<Edge*>& edges, Quadrilateral& cutquad, bool gutter)
{
	mMesh->OpenCutline(edges, cutquad, gutter);
	RebuildBuffers(mMesh->mVertexes, mMesh->mIndexes);
}


void Model::ChainFaces(LinkList& chain, LinkFaceMap& chainfaces, float radius)
{
	mMesh->ChainFaces(chain, chainfaces, radius);
}

void Model::ChainFaces(LinkList& chain, LinkFaceMap& cfouter, LinkFaceMap& cfinner, float router, float rinner)
{
	mMesh->ChainFaces(chain, cfouter, cfinner, router, rinner);
}



UINT Model::IndexCount()
{
	return static_cast<UINT>(mMesh->mIndexes.size());
}
