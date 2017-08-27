#include "hash.h"

#include "structures.h"
#include "mathematics.h"


using namespace skincut;
using namespace skincut::math;



// Code from boost: boost/functional/hash.hpp
// Reciprocal of the golden ratio helps spread entropy and handles duplicates. 
// Magic value 0x9e3779b9: http://stackoverflow.com/questions/4948780

template <class T>
inline void hash_combine(std::uint32_t& seed, const T& v)
{
	seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}


std::uint32_t skincut::hash_vector2(math::Vector2 const& p)
{
	std::uint32_t seed = 0;
	hash_combine(seed, p.x);
	hash_combine(seed, p.y);
	return seed;
};

std::uint32_t skincut::hash_vector3(math::Vector3 const& p)
{
	std::uint32_t seed = 0;
	hash_combine(seed, p.x);
	hash_combine(seed, p.y);
	hash_combine(seed, p.z);
	return seed;
};

std::uint32_t skincut::hash_vector4(math::Vector4 const& p)
{
	std::uint32_t seed = 0;
	hash_combine(seed, p.x);
	hash_combine(seed, p.y);
	hash_combine(seed, p.z);
	hash_combine(seed, p.w);
	return seed;
};



std::uint32_t IndexerHash::operator()(const Indexer& it) const
{
	std::uint32_t seed = 0;
	hash_combine(seed, it.pi);
	hash_combine(seed, it.ni);
	hash_combine(seed, it.xi);
	return seed;
}

bool IndexerHash::operator()(const Indexer& i0, const Indexer& i1) const
{
	return std::tie(i0.pi, i0.ni, i0.xi) == 
		   std::tie(i1.pi, i1.ni, i1.xi);
}



std::uint32_t VertexHash::operator()(const Vertex& v) const
{
	std::uint32_t seed = 0;
	hash_combine(seed, hash_vector3(v.position));
	hash_combine(seed, hash_vector2(v.texcoord));
	hash_combine(seed, hash_vector3(v.normal));
	hash_combine(seed, hash_vector4(v.tangent));
	return seed;
}

bool VertexHash::operator()(const Vertex& v0, const Vertex& v1) const
{
	return std::tie(v0.position, v0.texcoord, v0.normal, v0.tangent) == 
		   std::tie(v1.position, v1.texcoord, v1.normal, v1.tangent);
}



std::uint32_t NodeHash::operator()(const Node& n) const
{
	std::uint32_t seed = 0;
	hash_combine(seed, hash_vector3(n.p));
	return seed;
}

std::uint32_t NodeHash::operator()(const Node* n) const
{
	std::uint32_t seed = 0;
	hash_combine(seed, hash_vector3(n->p));
	return seed;
}

bool NodeHash::operator()(const Node& n0, const Node& n1) const
{
	return n0.p == n1.p;
}

bool NodeHash::operator()(const Node* n0, const Node* n1) const
{
	return n0->p == n1->p;
}



std::uint32_t EdgeHash::operator()(const Edge& e) const
{
	std::uint32_t seed = 0;
	hash_combine(seed, e.n[0]);
	hash_combine(seed, e.n[1]);
	return seed;
}

std::uint32_t EdgeHash::operator()(const Edge* e) const
{
	std::uint32_t seed = 0;
	hash_combine(seed, e->n[0]);
	hash_combine(seed, e->n[1]);
	return seed;
}

bool EdgeHash::operator()(const Edge& e0, const Edge& e1) const
{
	return std::tie(e0.n[0], e0.n[1]) == 
		   std::tie(e1.n[0], e1.n[1]);
}

bool EdgeHash::operator()(const Edge* e0, const Edge* e1) const
{
	return std::tie(e0->n[0], e0->n[1]) == 
		   std::tie(e1->n[0], e1->n[1]);
}



std::uint32_t FaceHash::operator()(const Face& f) const
{
	std::uint32_t seed = 0;
	hash_combine(seed, f.n[0]);
	hash_combine(seed, f.n[1]);
	hash_combine(seed, f.n[2]);
	return seed;
}

std::uint32_t FaceHash::operator()(const Face* f) const
{
	std::uint32_t seed = 0;
	hash_combine(seed, f->n[0]);
	hash_combine(seed, f->n[1]);
	hash_combine(seed, f->n[2]);
	return seed;
}

bool FaceHash::operator()(const Face& f0, const Face& f1) const
{
	return std::tie(f0.n[0], f0.n[1], f0.n[2]) == 
		   std::tie(f1.n[0], f1.n[1], f1.n[2]);
}

bool FaceHash::operator()(const Face* f0, const Face* f1) const
{
	return std::tie(f0->n[0], f0->n[1], f0->n[2]) == 
		   std::tie(f1->n[0], f1->n[1], f1->n[2]);
}