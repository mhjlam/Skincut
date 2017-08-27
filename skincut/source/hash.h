#pragma once

#include <cstdint>


namespace skincut
{
	struct Indexer;
	struct Vertex;
	struct Node;
	struct Edge;
	struct Face;

	namespace math
	{
		struct Vector2;
		struct Vector3;
		struct Vector4;
	}

	std::uint32_t hash_vector2(math::Vector2 const& v);
	std::uint32_t hash_vector3(math::Vector3 const& v);
	std::uint32_t hash_vector4(math::Vector4 const& v);


	struct IndexerHash
	{
		std::uint32_t operator()(const Indexer& i) const;
		bool operator()(const Indexer& i0, const Indexer& i1) const;
	};

	struct VertexHash
	{
		std::uint32_t operator()(const Vertex& v) const;
		bool operator()(const Vertex& v0, const Vertex& v1) const;
	};

	struct NodeHash
	{
		std::uint32_t operator()(const Node& n) const;
		std::uint32_t operator()(const Node* n) const;
		bool operator()(const Node& n0, const Node& n1) const;
		bool operator()(const Node* n0, const Node* n1) const;
	};

	struct EdgeHash
	{
		std::uint32_t operator()(const Edge& e) const;
		std::uint32_t operator()(const Edge* e) const;
		bool operator()(const Edge& e0, const Edge& e1) const;
		bool operator()(const Edge* e0, const Edge* e1) const;
	};

	struct FaceHash
	{
		std::uint32_t operator()(const Face& f) const;
		std::uint32_t operator()(const Face* f) const;
		bool operator()(const Face& f0, const Face& f1) const;
		bool operator()(const Face* f0, const Face* f1) const;
	};

}

