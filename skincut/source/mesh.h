#pragma once

#include <map>
#include <array>
#include <memory>
#include <string>
#include <vector>
#include <d3d11.h>
#include <sstream>
#include <wrl/client.h>
#include <unordered_set>
#include <unordered_map>

#include "hash.h"
#include "structures.h"
#include "mathematics.h"



namespace skincut
{
	typedef std::list<Link> LinkList;
	typedef std::map<Link, std::vector<Face*>> LinkFaceMap;
	typedef std::unordered_set<Face*, FaceHash, FaceHash> FaceSet;


	class Mesh
	{
	public: // constants
		static const float cMaxEdgeLength;
		static const float cInfluenceRadius;


	public:
		// mesh geometry / attributes
		std::vector<uint32_t> mIndexes; // numfaces * 3
		std::vector<Vertex> mVertexes;
		std::unordered_map<Vertex, uint32_t, VertexHash, VertexHash> mVertexTable;

		// mesh topology
		std::vector<Node*> mNodeArray; // continuous memory allows iteration to be fast
		std::vector<Edge*> mEdgeArray;
		std::vector<Face*> mFaceArray;

		std::unordered_set<Node*, NodeHash, NodeHash> mNodeTable; // buckets allows individual lookups to be fast
		std::unordered_set<Edge*, EdgeHash, EdgeHash> mEdgeTable;
		std::unordered_set<Face*, FaceHash, FaceHash> mFaceTable;


	public:
		Mesh(const std::wstring& meshname);
		~Mesh();

		void ParseMesh(const std::wstring& meshname, bool computeNormals = false);
		void LoadMesh(const std::wstring& filename);
		void SaveMesh(const std::wstring& filename);

		void RebuildIndexes(); // extract triangle list


	public: // mesh manipulation
		bool RayIntersection(math::Ray& ray); // any ray-face intersection
		bool RayIntersection(math::Ray& ray, Intersection& intersection); // closest ray-face intersection

		void Subdivide(Face*& face, SPLITMODE splitmode, math::Vector3& point); // subdivide face

		void FormCutline(Intersection& i0, Intersection& i1, std::list<Link>& cutline, math::Quadrilateral& cutquad);
		void FuseCutline(std::list<Link>& cutline, std::vector<Edge*>& cutedges);
		void OpenCutline(std::vector<Edge*>& edges, math::Quadrilateral& cutquad, bool gutter = true);

		void Neighbors(Face*& f, std::array<Face*, 3>& nbs);
		void Neighbors(Face*& f, std::array<std::pair<Face*, Edge*>, 3>& nbs);
		void ChainFaces(std::list<Link>& cutline, std::map<Link, std::vector<Face*>>& CF, float r);
		void ChainFaces(std::list<Link>& cutline, std::map<Link, std::vector<Face*>>& CF0, std::map<Link, std::vector<Face*>>& CF1, float r0, float r1);



	private: // geometry
		void Split2(Face*& f, Edge*& e0, math::Vector3& p = math::Vector3(), Edge** ec = nullptr);
		void Split3(Face*& f, math::Vector3& p = math::Vector3(), Edge** ec0 = nullptr, Edge** ec1 = nullptr, Edge** ec2 = nullptr);
		void Split4(Face*& f);
		void Split6(Face*& f);


	private: // topology
		void GenerateTopology(); // generate topology

		uint32_t MakeVertex(math::Vector3& p, math::Vector2& x, math::Vector3& n, math::Vector4& t, math::Vector3& b);
		uint32_t MakeVertex(Vertex& v0, Vertex& v1);
		uint32_t MakeVertex(Vertex& v0, Vertex& v1, math::Vector3 p);
		uint32_t MakeVertex(Vertex& v0, Vertex& v1, Vertex& v2);
		uint32_t MakeVertex(Vertex& v0, Vertex& v1, Vertex& v2, math::Vector3 p);

		Node* MakeNode(math::Vector3& p);
		Node* MakeNode(Node*& n0, Node*& n1);
		Node* MakeNode(Node*& n0, Node*& n1, Node*& n2);

		Edge* MakeEdge(Node*& n0, Node*& n1);
		Edge* MakeEdge(Node*& n0, Node*& n1, uint32_t i0, uint32_t i1);

		Face* MakeFace(Node*& n0, Node*& n1, Node*& n2, uint32_t i0, uint32_t i1, uint32_t i2);

		void RegisterEdge(Edge*& e, Face*& f0);
		void RegisterEdge(Edge*& e, Face*& f0, Face*& f1);
		void RegisterFace(Face*& f, Edge*& e0, Edge*& e1, Edge*& e2);

		void UpdateEdge(Edge*& e, Face*& f, Face*& fn);

		uint32_t CopyVertex(Vertex& v);
		Node* CopyNode(Node*& n);
		Edge* CopyEdge(Edge*& e);
		Face* CopyFace(Face*& f);

		void KillNode(Node*& n, bool del = false);
		void KillEdge(Edge*& e, bool del = false);
		void KillFace(Face*& f, bool del = false);
	};

}

