#pragma once

#include <map>
#include <list>
#include <memory>
#include <string>
#include <vector>
#include <d3d11.h>
#include <wrl/client.h>

#include "structures.h"
#include "mathematics.h"


using Microsoft::WRL::ComPtr;



namespace skincut
{
	class Mesh;

	typedef std::list<Link> LinkList;
	typedef std::map<Link, std::vector<Face*>> LinkFaceMap;


	struct ModelLoadInfo
	{
		math::Vector3 position;
		math::Vector2 rotation;
		std::wstring meshpath;
		std::wstring colorpath;
		std::wstring normalpath;
		std::wstring specularpath;
		std::wstring discolorpath;
		std::wstring occlusionpath;
	};

	
	class Model
	{
	private:
		ModelLoadInfo mLoadInfo;
		ComPtr<ID3D11Device> mDevice;

	public:
		std::unique_ptr<Mesh> mMesh;

		// general data
		math::Vector3 mPosition;
		math::Vector2 mRotation; // spherical coordinates (polar angle, azimuth angle)

		// matrices
		math::Matrix mMatrixWorld;
		math::Matrix mMatrixWVP;

		// vertex buffer
		UINT mVertexBufferSize;
		UINT mVertexBufferStrides;
		UINT mVertexBufferOffset;
		D3D11_PRIMITIVE_TOPOLOGY mTopology;
		ComPtr<ID3D11Buffer> mVertexBuffer;

		// index buffer
		UINT mIndexBufferSize;
		UINT mIndexBufferOffset;
		DXGI_FORMAT mIndexBufferFormat;
		ComPtr<ID3D11Buffer> mIndexBuffer;

		// material data
		math::Color mColorWire;
		math::Color mColorSolid;

		ComPtr<ID3D11ShaderResourceView> mColorMap;
		ComPtr<ID3D11ShaderResourceView> mNormalMap;
		ComPtr<ID3D11ShaderResourceView> mSpecularMap;
		ComPtr<ID3D11ShaderResourceView> mDiscolorMap;
		ComPtr<ID3D11ShaderResourceView> mOcclusionMap;


	public: // constructor
		Model(ComPtr<ID3D11Device>& device,
			  math::Vector3 position,
			  math::Vector2 rotation,
			  std::wstring meshpath,
			  std::wstring colorpath,
			  std::wstring normalpath,
			  std::wstring specularpath,
			  std::wstring discolorpath,
			  std::wstring occlusionpath);
		~Model();


	private:
		void LoadResources(ModelLoadInfo li);

		void RebuildBuffers(std::vector<Vertex>& vertexes, std::vector<uint32_t>& indexes);
		void RebuildVertexBuffer(std::vector<Vertex>& vertexes);
		void RebuildIndexBuffer(std::vector<uint32_t>& indexes);


	public:
		void Update(const math::Matrix view, const math::Matrix projection);
		void Reload();

		bool RayIntersection(math::Ray& ray);
		bool RayIntersection(math::Ray& ray, Intersection& intersection);

		void Subdivide(Face*& face, SPLITMODE splitmode, math::Vector3& point);

		void FormCutline(Intersection& i0, Intersection& i1, std::list<Link>& cutline, math::Quadrilateral& cutquad);
		void FuseCutline(std::list<Link>& cutline, std::vector<Edge*>& edges);
		void OpenCutline(std::vector<Edge*>& edges, math::Quadrilateral& cutquad, bool gutter = true);

		void ChainFaces(LinkList& chain, LinkFaceMap& cf, float r);
		void ChainFaces(LinkList& chain, LinkFaceMap& cfo, LinkFaceMap& cfi, float ro, float ri);

		UINT IndexCount();
	};

}

