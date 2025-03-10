#include "Mesh.hpp"


#include <map>
#include <fstream>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <functional>
#include <unordered_map>

#include <io.h>
#include <float.h> // _finite


#pragma warning(disable: 4996) // unsafe stdio functions

#ifdef _DEBUG
#ifndef DBG_NEW
#define DBG_NEW new (_NORMAL_BLOCK , __FILE__ , __LINE__)
#define new DBG_NEW
#endif
#endif


using namespace SkinCut;
using namespace SkinCut::Math;



const float Mesh::cMaxEdgeLength = 0.5f;
const float Mesh::cInfluenceRadius = 0.5f;


Mesh::Mesh(const std::wstring& name)
{
	mVertexes = std::vector<Vertex>();
	mVertexTable = std::unordered_map<Vertex, uint32_t, VertexHash, VertexHash>();

	mNodeArray = std::vector<Node*>();
	mEdgeArray = std::vector<Edge*>();
	mFaceArray = std::vector<Face*>();

	mNodeTable = std::unordered_set<Node*, NodeHash, NodeHash>();
	mEdgeTable = std::unordered_set<Edge*, EdgeHash, EdgeHash>();
	mFaceTable = std::unordered_set<Face*, FaceHash, FaceHash>();

	// Binary file name
	std::wstring binname = name + std::wstring(L".bin");

	// See if binary exists
	if (GetFileAttributes(binname.c_str()) != INVALID_FILE_ATTRIBUTES) {
		LoadMesh(binname);
		GenerateTopology();
	}
	else { // otherwise, load from .obj file
		ParseMesh(name, true);
		GenerateTopology();
		SaveMesh(binname); // write binary file
	}
}


Mesh::~Mesh() // need explicit destructor to clean up std::unique_ptr<Topology>
{
	for (auto node : mNodeArray) {
		if (node) {
			delete node;
			node = nullptr;
		}
	}

	for (auto edge : mEdgeArray) {
		if (edge) {
			delete edge;
			edge = nullptr;
		}
	}

	for (auto face : mFaceArray) {
		if (face) {
			delete face;
			face = nullptr;
		}
	}

	mNodeArray.clear();
	mNodeTable.clear();

	mEdgeArray.clear();
	mEdgeTable.clear();

	mFaceArray.clear();
	mFaceTable.clear();
}


void Mesh::RebuildIndexes()
{
	uint32_t i = 0;
	mIndexes.clear();
	mIndexes.resize(mFaceTable.size() * 3); // three indexes per face

	for (Face*& face : mFaceArray) {
		mIndexes[i++] = face->v[0];
		mIndexes[i++] = face->v[1];
		mIndexes[i++] = face->v[2];
	}
}


/*******************************************************************************
Mesh loading
*******************************************************************************/

void Mesh::ParseMesh(const std::wstring& name, bool computeNormals)
{
	std::stringstream contents;

	HMODULE mod = GetModuleHandle(nullptr);
	HRSRC hrsrc = FindResource(mod, name.c_str(), RT_RCDATA);

	if (hrsrc) { // load from resource
		HGLOBAL res = LoadResource(GetModuleHandle(nullptr), hrsrc);
		char* data = (char*)LockResource(res); // does not actually lock memory
		if (!data) throw std::exception("Mesh loading error: Unable to find mesh file.");
		contents = std::stringstream(data);
	}
	else { // try to load from file
		std::ifstream file(name);

		if (file) { // found
			std::copy(std::istreambuf_iterator<char>(file), 
				std::istreambuf_iterator<char>(), 
				std::ostreambuf_iterator<char>(contents)); // copy contents to stringstream
		}
		else { // not found
			// try to load from resource folder
			std::wstring meshname2 = std::wstring(L"Resources\\") + name;
			file = std::ifstream(meshname2);

			if (file) {
				// copy contents to stringstream
				std::copy(std::istreambuf_iterator<char>(file), 
					std::istreambuf_iterator<char>(), 
					std::ostreambuf_iterator<char>(contents));
			}
			else {
				throw std::exception("Mesh loading error: Unable to find mesh file");
			}
		}
	}

	if (!contents) {
		throw std::exception("Mesh loading error: Unable to read mesh file.");
	}

	
	// Parse file contents (and obtain arrays of vertexes and indexes)
	char space;
	uint32_t index = 0;
	std::string word;

	std::vector<Vector3> positions;
	std::vector<Vector2> texCoords;
	std::vector<Vector3> faceNormals;

	std::vector<Indexer> indexers;	// obj vertex definition (indexes to position/texcoord/normal) [with duplicates]
	std::unordered_map<Indexer, uint32_t, IndexerHash, IndexerHash> indexerMap; // unique indexers

	// Direct3D uses a left-handed coordinate system. If you are porting an application that is based on 
	// a right-handed coordinate system, you must make two changes to the data passed to Direct3D:
	// 1. Flip the order of triangle vertices so that the system traverses them clockwise from the front. 
	//    In other words, if the vertices are v0, v1, v2, pass them to Direct3D as v0, v2, v1.
	// 2. Use the view matrix to scale world space by -1 in the z-direction. 
	//    To do this, flip the sign of the _31, _32, _33, and _34 member of the view matrix structure.

	auto decr = [&](Indexer& indexer) -> void { // convert indices to zero-based format
		indexer.pi--;
		if (indexer.xi > 0) indexer.xi--;
		if (indexer.ni > 0) indexer.ni--;
	};

	while (!contents.eof()) {
		contents >> word;
		if (!contents) break;

		if (word.compare("#") == 0) {
			contents.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			continue;
		}
		else if (word.compare("v") == 0) {
			float x, y, z;
			contents >> x >> y >> z;
			positions.push_back(Vector3(x, z, y)); // convert to left-handed
		}
		else if (word.compare("vn") == 0) {
			float x, y, z;
			contents >> x >> y >> z;
			faceNormals.push_back(Vector3(x, z, y)); // convert to left-handed
		}
		else if (word.compare("vt") == 0) {
			float u, v;
			contents >> u >> v;
			texCoords.push_back(Vector2(u, 1.f-v)); // convert to top-left
		}
		else if (word.compare("g") == 0) {
			// group (mesh part)
			contents.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			continue;
		}
		else if (word.compare("s") == 0) {
			// smoothing group
			contents.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			continue;
		}

		// faces must always be defined last, or the logic used here breaks down
		if (word.compare("f") == 0) {
			Indexer faceDef[3]{};
			ZeroMemory(&faceDef[0], sizeof(Indexer)*3);

			for (auto& indexer : faceDef) {
				// Supported formats:
				// [v|vt|vn]	(vertex position | texcoord | normal)
				// [v|vt]		(vertex position | texcoord)
				// [v]			(vertex position)

				if (!texCoords.empty() && !faceNormals.empty()) { // case 1
					contents >> indexer.pi >> space >> indexer.xi >> space >> indexer.ni;
					decr(indexer); // decrement indices to make them zero-based
				}
				else if (!texCoords.empty() && faceNormals.empty()) { // case 2
					contents >> indexer.pi >> space >> indexer.xi;
					decr(indexer);
				}
				else if (texCoords.empty() && faceNormals.empty()) { // case 3
					contents >> indexer.pi;
					decr(indexer);
				}
				else {
					throw std::exception("Mesh loading error: Unsupported face definition format.");
				}
			}

			// convert from right-handed to left-handed system
			std::swap(faceDef[1], faceDef[2]);

			for (auto& indexer : faceDef) {
				indexers.push_back(indexer);

				// insert indexer and increment index
				auto entry = indexerMap.emplace(indexer, index);

				if (entry.second) { // new indexer
					mIndexes.push_back(index++);

					if (index == (uint32_t)-1) {
						throw std::exception("Mesh loading error: Too many indexes (mesh is too large).");
					}
				}
				else { // indexer already exists
					mIndexes.push_back(entry.first->second);
				}
			}
		}
	}

	// Analyze the data that was read.
	if (positions.empty()) { // Throw error if no positions are recorded.
		throw std::exception("Mesh loading error: No vertices found.");
	}

	// Compute normals, tangents and bitangents
	std::vector<Vector3> normals(positions.size());
	std::vector<Vector4> tangents(positions.size());
	std::vector<Vector3> bitangents(positions.size());

	for (uint32_t i = 0; i < indexers.size(); i+=3) {
		const Vector3 p1 = positions[indexers[i+0].pi];
		const Vector3 p2 = positions[indexers[i+1].pi];
		const Vector3 p3 = positions[indexers[i+2].pi];

		const Vector2 uv1 = texCoords[indexers[i+0].xi];
		const Vector2 uv2 = texCoords[indexers[i+1].xi];
		const Vector2 uv3 = texCoords[indexers[i+2].xi];

		if (faceNormals.empty() || computeNormals) {
			// explicitly compute face normal
			Vector3 facenormal = Vector3::Cross(p2-p1, p3-p1);
			
			normals[indexers[i+0].pi] += facenormal;
			normals[indexers[i+1].pi] += facenormal;
			normals[indexers[i+2].pi] += facenormal;
		}
		else {
			// use face normals from obj file
			normals[indexers[i+0].pi] += faceNormals[indexers[i+0].ni];
			normals[indexers[i+1].pi] += faceNormals[indexers[i+1].ni];
			normals[indexers[i+2].pi] += faceNormals[indexers[i+2].ni];
		}
		
		// Compute tangent (http://www.terathon.com/code/tangent.html)
		float x1 = (p2.x - p1.x), y1 = (p2.y - p1.y), z1 = (p2.z - p1.z);
		float x2 = (p3.x - p1.x), y2 = (p3.y - p1.y), z2 = (p3.z - p1.z);
		float s1 = (uv2.x - uv1.x), t1 = (uv2.y - uv1.y);
		float s2 = (uv3.x - uv1.x), t2 = (uv3.y - uv1.y);

		// inverse of the (s,t) matrix
		float r = 1.0f / (s1 * t2 - s2 * t1);
		if (!_finite(r)) {
			r = 0.0f;
		}

		// tangent points in the u texture direction
		Vector4 tangent   = Vector4((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r, 0);

		// bitangent points in the v texture direction
		Vector3 bitangent = Vector3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

		tangents[indexers[i+0].pi] += tangent;
		tangents[indexers[i+1].pi] += tangent;
		tangents[indexers[i+2].pi] += tangent;

		bitangents[indexers[i+0].pi] += bitangent;
		bitangents[indexers[i+1].pi] += bitangent;
		bitangents[indexers[i+2].pi] += bitangent;
	}

	// Create vertexes from vertex definitions.
	mVertexes.resize(indexerMap.size());
	for (auto& indexer : indexerMap) {
		Vertex vertex;
		vertex.position = positions[indexer.first.pi];
		vertex.texcoord = texCoords[indexer.first.xi];

		// position index can be used because index order is equivalent
		vertex.normal = Vector3::Normalize(normals[indexer.first.pi]); 
		vertex.tangent = tangents[indexer.first.pi];
		vertex.bitangent = bitangents[indexer.first.pi];
		
		Vector3 vertexTangent = Vector3(vertex.tangent.x, vertex.tangent.y, vertex.tangent.z);

		// normalize and orthogonalize tangent with Gram-Schmidt
		Vector3 tangent = Vector3::Normalize(vertexTangent - vertex.normal * Vector3::Dot(vertex.normal, vertexTangent));

		// handedness of bitangent = sign of determinant of TBN matrix
		float handedness = Math::Sign(Vector3::Dot(Vector3::Cross(vertex.normal, vertexTangent), vertex.bitangent));

		vertex.tangent = Vector4(tangent.x, tangent.y, tangent.z, handedness); // store bitangent handedness in alpha channel

		mVertexes[indexer.second] = vertex;
	}
}


void Mesh::LoadMesh(const std::wstring& filename)
{
	std::ifstream in(filename, std::ios::in | std::ios::binary);
	if (!in) {
		throw std::exception("Mesh loading error: Unable to find mesh file");
	}
	
	uint32_t indexCount = 0;
	uint32_t vertexCount = 0;

	mIndexes.clear();
	in.read((char*)&indexCount, sizeof(uint32_t));
	mIndexes.resize(indexCount);
	for (uint32_t i = 0; i < indexCount; ++i) {
		in.read((char*)&mIndexes[i], sizeof(uint32_t));
	}

	mVertexes.clear();
	in.read((char*)&vertexCount, sizeof(uint32_t));
	mVertexes.resize(vertexCount);
	for (uint32_t i = 0; i < vertexCount; ++i) {
		in.read((char*)&mVertexes[i], sizeof(Vertex));
	}

	in.close();
}


void Mesh::SaveMesh(const std::wstring& filename)
{
	std::ofstream out(filename, std::ios::out | std::ios::binary);

	uint32_t indexCount = static_cast<uint32_t>(mIndexes.size());
	uint32_t vertexCount = static_cast<uint32_t>(mVertexes.size());

	out.write((char*)&indexCount, sizeof(uint32_t));
	for (uint32_t i = 0; i < indexCount; ++i) {
		out.write((char*)&mIndexes[i], sizeof(uint32_t));
	}

	out.write((char*)&vertexCount, sizeof(uint32_t));
	for (uint32_t i = 0; i < vertexCount; ++i) {
		out.write((char*)&mVertexes[i], sizeof(Vertex));
	}

	out.close();
}



/*******************************************************************************
Mesh manipulation
*******************************************************************************/

void Mesh::Subdivide(Face*& f, SplitType splitMode, Vector3& p)
{
	switch (splitMode) {
		case SplitType::SPLIT3: {
			Split3(f, p);
			break;
		}

		case SplitType::SPLIT4: {
			// 4-split target face and 2-split its neighbors
			std::array<std::pair<Face*,Edge*>,3> neighbors;
			Neighbors(f, neighbors);

			Split4(f);
			for (auto& nb : neighbors) {
				Split2(nb.first, nb.second);
				delete nb.second;
			}
			break;
		}

		case SplitType::SPLIT6: {
			// 6-split target face and 2-split its neighbors
			std::array<std::pair<Face*,Edge*>,3> neighbors;
			Neighbors(f, neighbors);

			Split6(f);
			for (auto& nb : neighbors) {
				Split2(nb.first, nb.second);
				delete nb.second;
			}
			break;
		}
	}
}


bool Mesh::RayIntersection(Ray& ray)
{
	// Iterate over the faces of the mesh.
	for (auto face : mFaceArray) {
		float t, u, v;

		Vector3 v0 = face->n[0]->p;
		Vector3 v1 = face->n[1]->p;
		Vector3 v2 = face->n[2]->p;

		// Test whether the ray passes through the triangle face.
		if (Math::RayTriangleIntersection(ray, Triangle(v0, v1, v2), t, u, v)) {
			return true;
		}
	}

	return false;
}

bool Mesh::RayIntersection(Ray& ray, Intersection& ix)
{
	// Find the closest intersection of the ray with the model.
	bool hit = false;
	float tmin = std::numeric_limits<float>::max();

	for (auto face : mFaceArray) {
		Vertex v0 = mVertexes[face->v[0]];
		Vertex v1 = mVertexes[face->v[1]];
		Vertex v2 = mVertexes[face->v[2]];

		// Test whether the ray passes through the triangle face.
		float t, u, v;
		if (Math::RayTriangleIntersection(ray, Triangle(v0.position, v1.position, v2.position), t, u, v)) {
			hit = true;

			// Replace if this intersection occurs earlier along the ray.
			if (t < tmin) {
				ix.dist = t;

				// (x,y,z) = origin + (distance * direction)
				ix.pos_os = ray.origin + (t * ray.direction);
				ix.pos_ts = Vector2::Barycentric(v0.texcoord, v1.texcoord, v2.texcoord, u, v);
				ix.face = face;

				tmin = t;
			}
		}
	}

	return hit;
}


void Mesh::FormCutline(Intersection& i0, Intersection& i1, std::list<Link>& cutLine, Quadrilateral& cutQuad)
{
	bool loop = true;
	Face* f = i0.face; // start at first intersected face
	Vector3 p0 = i0.pos_os;
	Vector3 p1 = Vector3();
	Vector2 x0 = i0.pos_ts;
	Vector2 x1 = Vector2();
	std::unordered_set<Edge*, EdgeHash, EdgeHash> table;

	// form cutting quadrilateral with intersection data
	Vector3 q0 = i0.ray.origin + (i0.ray.direction * i0.nearz);
	Vector3 q1 = i0.ray.origin + (i0.ray.direction * i0.farz);
	Vector3 q2 = i1.ray.origin + (i1.ray.direction * i1.farz);
	Vector3 q3 = i1.ray.origin + (i1.ray.direction * i1.nearz);
	cutQuad = Quadrilateral(q0, q1, q2, q3);

	while (loop) { // iterate over faces that lie on the cutting line
		loop = false;

		for (uint8_t i = 0; i < 3; ++i) { // iterate over the three edges of a face
			Edge* edge = f->e[i];

			// mark edges that have been visited already
			if (table.insert(edge).second) { // continue if edge not visited
				// edge endpoints
				Vertex ep0 = mVertexes[f->v[i]];
				Vertex ep1 = mVertexes[f->v[(i+1) % 3]];

				float t;
				Ray ray(ep0.position, ep1.position - ep0.position);

				// test intersection between edge and cutting quad
				if (RayQuadIntersection(ray, cutQuad, t) && t <= 1) {
					// compute intersection point
					p1 = Vector3::Lerp(ep0.position, ep1.position, t); // r.o + t*r.d;

					// compute second texture coordinate
					x1 = Vector2::Lerp(ep0.texcoord, ep1.texcoord, t);

					// add segment to cutline chain
					cutLine.push_back(Link(f, p0, p1, x0, x1));

					// prepare first endpoint of next segment
					p0 = p1; // (this cannot be done for texcoords due to seams)

					// continue with neighboring face of edge that tested positively
					f = (edge->f[1] == f) ? edge->f[0] : edge->f[1];

					// compute texture coordinate for next segment
					for (uint32_t i : f->v) {
						if (mVertexes[i].position == ep0.position) {
							ep0 = mVertexes[i];
						}
						else if (mVertexes[i].position == ep1.position) {
							ep1 = mVertexes[i];
						}
					}
					x0 = Vector2::Lerp(ep0.texcoord, ep1.texcoord, t);

					loop = true;

					// skip testing other edges
					break;
				}
			}
		}
	}

	// add final segment
	cutLine.push_back(Link(f, p1, i1.pos_os, x0, i1.pos_ts));
}


void Mesh::FuseCutline(std::list<Link>& cutLine, std::vector<Edge*>& cutEdges)
{
	// N(p0) & N(p1) => p0=p1 or p0->p1 is f->e[0,1,2]
	// N(p0) & E(p1) => split2(p1)
	// N(p0) & F(p1) => split3(p1)

	// E(p0) & N(p1) => split2(p0)
	// E(p0) & E(p1) => split2(p1), split2(p0)
	// E(p0) & F(p1) => split3(p1), split2(p0)

	// F(p0) & N(p1) => split3(p0)
	// F(p0) & E(p1) => split3(p0), split2(p1)
	// F(p0) & F(p1) => split3(p0), split3(p1)

	std::function<Node*(Vector3& p, Face*& f)> N = [&](Vector3& p, Face*& f) -> Node* {
		if (Equal(p, f->n[0]->p)) return f->n[0];
		if (Equal(p, f->n[1]->p)) return f->n[1];
		if (Equal(p, f->n[2]->p)) return f->n[2];
		return nullptr;
	};

	std::function<Edge*(Vector3& p, Face*& f)> E = [&](Vector3& p, Face*& f) -> Edge* {
		if (SegmentPointIntersection(f->n[0]->p, f->n[1]->p, p)) return f->e[0];
		if (SegmentPointIntersection(f->n[1]->p, f->n[2]->p, p)) return f->e[1];
		if (SegmentPointIntersection(f->n[0]->p, f->n[2]->p, p)) return f->e[2];
		return nullptr;
	};


	std::unordered_set<Edge*, EdgeHash, EdgeHash> sides;

	for (auto l = cutLine.begin(); l != cutLine.end(); ++l) {
		Face*& f = l->f;
		Vector3& p0 = l->p0;
		Vector3& p1 = l->p1;

		Node* n0 = nullptr;
		Node* n1 = nullptr;
		Edge* e0 = nullptr;
		Edge* e1 = nullptr;

		// N(p0)
		if (n0 = N(p0, f)) {
			// 1. N(p0) & N(p1) => p0=p1 or p0->p1 is f->e[0,1,2]
			if (n1 = N(p1, f)) {
				// p0=p1; no edge to add
				if (n0 == n1) continue;

				// p0->p1 is f->e[0,1,2]
				Edge* ec = nullptr;
				if (n0 == f->n[0]) {
					ec = (n1 == f->n[1]) ? f->e[0] : f->e[2];
				}
				else if (n0 == f->n[1]) {
					ec = (n1 == f->n[0]) ? f->e[0] : f->e[1];
				}
				else if (n0 == f->n[2]) {
					ec = (n1 == f->n[0]) ? f->e[2] : f->e[1];
				}
				else {
					throw std::exception("Mesh degeneracy detected!");
				}

				// add splitting edge to collection
				cutEdges.push_back(ec);
			}

			// 2. N(p0) & E(p1) => split2(p1)
			else if (e1 = E(p1, f)) {
				// mark edge for removal
				sides.insert(e1);

				// 2-split at p1
				Edge* ec = nullptr;
				Split2(f, e1, p1, &ec);

				// add splitting edge to collection
				std::swap(ec->p[0], ec->p[1]); // reverse direction
				std::swap(ec->f[0], ec->f[1]); // flip face references
				cutEdges.push_back(ec);
			}

			// 3. N(p0) & F(p1) => split3(p1)
			else {
				// 3-split at p1
				Edge *ec0, *ec1, *ec2;
				Split3(f, p1, &ec0, &ec1, &ec2);

				// one of the splitters lies on the cutting line
				Edge* ec = nullptr;
				if (n0 == ec0->p[1].first) {
					ec = ec0;
				}
				else if (n0 == ec1->p[1].first) {
					ec = ec1;
				}
				else if (n0 == ec2->p[1].first) {
					ec = ec2;
				}
				else {
					throw std::exception("Mesh degeneracy detected!");
				}

				// add splitting edge to collection
				std::swap(ec->p[0], ec->p[1]); // reverse direction
				std::swap(ec->f[0], ec->f[1]); // flip face references
				cutEdges.push_back(ec);
			}
		}

		// E(p0)
		else if (e0 = E(p0, f)) {
			// 4. E(p0) & N(p1) => split2(p0)
			if (n1 = N(p1, f)) {
				// mark edge for removal
				sides.insert(e0);

				// 2-split at p0
				Edge* ec = nullptr;
				Split2(f, e0, p0, &ec);

				// add splitting edge to collection
				cutEdges.push_back(ec);
			}

			// 5. E(p0) & E(p1) => split2(p1), split2(p0)
			else if (e1 = E(p1, f)) {
				// mark edges for removal
				sides.insert(e0);
				sides.insert(e1);

				// 2-split at p1
				Edge* ec = nullptr;
				Split2(f, e1, p1, &ec);
				
				// find out which face p0 is in
				f = (e0 == ec->f[0]->e[1]) ? ec->f[0] : ec->f[1];

				// 2-split at p0
				Split2(f, e0, p0, &ec);

				// add splitting edge to collection
				cutEdges.push_back(ec);
			}

			// 6. E(p0) & F(p1) => split3(p1), split2(p0)
			else {
				// mark edge for removal
				sides.insert(e0);

				// 3-split at p1
				Edge *ec0, *ec1, *ec2;
				Split3(f, p1, &ec0, &ec1, &ec2);

				// find out which child face p0 is in
				Face* fc0 = ec0->f[0];
				Face* fc1 = ec1->f[0];
				Face* fc2 = ec2->f[0];
				f = (e0 == fc0->e[1]) ? fc0 : (e0 == fc1->e[1]) ? fc1 : fc2;

				// 2-split at p0
				Edge* ec = nullptr;
				Split2(f, e0, p0, &ec);

				// add splitting edge to collection
				cutEdges.push_back(ec);
			}
		}

		// F(p0)
		else {
			// 7. F(p0) & N(p1) => split3(p0)
			if (n1 = N(p1, f)) {
				// 3-split at p0
				Edge *ec0, *ec1, *ec2;
				Split3(f, p0, &ec0, &ec1, &ec2);

				// one of the splitters lies on the cutting line
				Edge* ec = nullptr;
				if (n1 == ec0->p[1].first) {
					ec = ec0;
				}
				else if (n1 == ec1->p[1].first) {
					ec = ec1;
				}
				else if (n1 == ec2->p[1].first) {
					ec = ec2;
				}
				else {
					throw std::exception("Mesh degeneracy detected!");
				}

				// add splitting edge to collection
				cutEdges.push_back(ec);
			}

			// 8. F(p0) & E(p1) => split3(p0), split2(p1)
			else if (e1 = E(p1, f)) {
				// mark edge for removal
				sides.insert(e1);

				// 3-split at p0
				Edge *ec0, *ec1, *ec2;
				Split3(f, p0, &ec0, &ec1, &ec2);

				// find out which child face p1 is in
				Face* fc0 = ec0->f[0];
				Face* fc1 = ec1->f[0];
				Face* fc2 = ec2->f[0];
				f = (e1 == fc0->e[1]) ? fc0 : (e1 == fc1->e[1]) ? fc1 : fc2;

				// 2-split at p1
				Edge* ec = nullptr;
				Split2(f, e1, p1, &ec);

				// add splitting edge to collection
				std::swap(ec->p[0], ec->p[1]); // reverse direction
				std::swap(ec->f[0], ec->f[1]); // flip face references
				cutEdges.push_back(ec);
			}

			// 9. F(p0) & F(p1) => split3(p0), split3(p1)
			else {
				// both points in same face; not supported
				throw std::exception("Cut chain must have at least two links");
			}
		}
	}

	// delete edges that have been split
	for (auto edge : sides) {
		delete edge;
	}
}


void Mesh::OpenCutLine(std::vector<Edge*>& EC, Math::Quadrilateral& cutQuad, bool gutter)
{
	uint32_t nEC = static_cast<uint32_t>(EC.size());
	if (EC.size() < 2) { return; }		// must have at least two segments

	std::vector<Face*> FU, FL;			// upper/lower faces
	std::vector<Edge*> EU, EL;			// upper/lower edges
	std::vector<Node*> NU, NL, NI;		// upper/lower/inner nodes

	std::vector<uint32_t> VU, VL;		// upper/lower border vertexes
	std::vector<uint32_t> WU, WL, WI;	// upper/lower/inner gutter vertexes
	
	// Compute length, depth, and cut opening displacement
	float cutLength = 0.0f;
	for (auto e : EC) {
		cutLength += Vector3::Distance(e->n[0]->p, e->n[1]->p); cutLength *= 20; // convert to cm
	}
	float cutDepth = std::max(0.1f, std::min(1.0f, 0.2f*cutLength)); // min: 0.1cm (0.005 units), max: 1cm (0.05 units)

	float depthSteps = (cutDepth-0.1f)/0.02f;
	float cutWidth = (0.0111f + (0.0002f * depthSteps)) * std::logf(cutLength) + (0.0415f + (0.0015f * depthSteps));
	//float cutWidth = (0.01f*std::logf(L)+0.04f)/20.f; //float cod = 0.005f * std::sqrtf(L);

	// convert to object-space units (1cm = 0.05 units)
	cutDepth /= 20.0f;
	cutWidth /= 20.0f;
	float halfCutWidth = cutWidth*0.5f;

	// Compute direction vectors
	Vector3 inward = Vector3::Normalize(cutQuad.v1 - cutQuad.v0);
	Vector3 upward = Vector3::Normalize(Vector3::Cross(inward, cutQuad.v3 - cutQuad.v0));
	
	// Compute texture coordinate coefficients
	float uMin = 0.00000f, vmin = 0.00000f; // based on size of
	float uMax = 0.06250f, vmax = 0.03125f; // gutter patch in color map
	float uStep = (uMax - uMin) / (float)nEC;
	
	std::function<float(float x)> CutOpeningDisplacement = [=](float x) -> float {
		return -std::powf(2.0f * x - 1.0f, 2.0f) + 1.0f;
		//return (0.5f * std::sinf((float)Math::TwoPi * (x - 0.25f)) + 0.5f);
	};


	//////////////////////////////
	// 1. CREATE TOPOLOGY/GEOMETRY

	for (uint32_t i = 0; i < nEC; ++i) {
		// acquire information
		Edge* ce = EC[i];
		Node* n0 = ce->p[0].first;
		Node* n1 = ce->p[1].first;
		Vertex v0 = mVertexes[ce->p[0].second];
		Vertex v1 = mVertexes[ce->p[1].second];
		Vector3 p0 = v0.position;
		Vector3 p1 = v1.position;
		
		// cut opening displacements at p0 and p1
		float cod0 = halfCutWidth * CutOpeningDisplacement(float(i) / (float)nEC);
		float cod1 = halfCutWidth * CutOpeningDisplacement(float(i+1) / (float)nEC);

		// new positions
		Vector3 p0u = p0 + upward * cod0;
		Vector3 p1u = p1 + upward * cod1;
		Vector3 p0l = p0 - upward * cod0;
		Vector3 p1l = p1 - upward * cod1;

		// new nodes for border
		Node* n0u = n0;
		Node* n0l = n0;
		Node* n1u = n1;
		Node* n1l = n1;

		// new vertexes for border
		uint32_t v0u = ce->p[0].second;
		uint32_t v0l = ce->p[0].second;
		uint32_t v1u = ce->p[1].second;
		uint32_t v1l = ce->p[1].second;
		
		// create new topology/geometry for border
		if (ce == EC.front()) { // first edge
			n0u = MakeNode(p0);
			n0l = MakeNode(p0);
			n1u = MakeNode(p1u);
			n1l = MakeNode(p1l);

			v0u = MakeVertex(p0,  v0.texcoord, v0.normal, v0.tangent, v0.bitangent);
			v0l = MakeVertex(p0,  v0.texcoord, v0.normal, v0.tangent, v0.bitangent);
			v1u = MakeVertex(p1u, v1.texcoord, v1.normal, v1.tangent, v1.bitangent);
			v1l = MakeVertex(p1l, v1.texcoord, v1.normal, v1.tangent, v1.bitangent);
		}
		else if (ce == EC.back()) { // last edge
			n0u = MakeNode(p0u);
			n0l = MakeNode(p0l);
			n1u = MakeNode(p1);
			n1l = MakeNode(p1);

			v0u = MakeVertex(p0u, v0.texcoord, v0.normal, v0.tangent, v0.bitangent);
			v0l = MakeVertex(p0l, v0.texcoord, v0.normal, v0.tangent, v0.bitangent);
			v1u = MakeVertex(p1,  v1.texcoord, v1.normal, v1.tangent, v1.bitangent);
			v1l = MakeVertex(p1,  v1.texcoord, v1.normal, v1.tangent, v1.bitangent);
		}
		else { // intermediate edges
			n0u = MakeNode(p0u);
			n0l = MakeNode(p0l);
			n1u = MakeNode(p1u);
			n1l = MakeNode(p1l);

			v0u = MakeVertex(p0u, v0.texcoord, v0.normal, v0.tangent, v0.bitangent);
			v0l = MakeVertex(p0l, v0.texcoord, v0.normal, v0.tangent, v0.bitangent);
			v1u = MakeVertex(p1u, v1.texcoord, v1.normal, v1.tangent, v1.bitangent);
			v1l = MakeVertex(p1l, v1.texcoord, v1.normal, v1.tangent, v1.bitangent);
		}

		NU.push_back(n0u);
		NU.push_back(n1u);
		NL.push_back(n0l);
		NL.push_back(n1l);

		VU.push_back(v0u);
		VU.push_back(v1u);
		VL.push_back(v0l);
		VL.push_back(v1l);

		EU.push_back(MakeEdge(n0u, n1u));
		EL.push_back(MakeEdge(n0l, n1l));

		FU.push_back(ce->f[0]);
		FL.push_back(ce->f[1]);


		// create new topology/geometry for gutter
		if (!gutter) { continue; }

		// new positions
		Vector3 p0i = p0 + inward * cutDepth;
		Vector3 p1i = p1 + inward * cutDepth;

		// new nodes for gutter
		Node* n0i = n0;
		Node* n1i = n1;

		// new vertexes for gutter
		uint32_t w0u = ce->p[0].second;
		uint32_t w0i = ce->p[0].second;
		uint32_t w0l = ce->p[0].second;

		uint32_t w1u = ce->p[1].second;
		uint32_t w1i = ce->p[1].second;
		uint32_t w1l = ce->p[1].second;

		// new texture coordinates
		float x0 = uMin + (float)i * uStep;
		float x1 = x0 + uStep;

		Vector2 x0b = Vector2(x0, vmin);
		Vector2 x1b = Vector2(x1, vmin);
		Vector2 x0i = Vector2(x0, vmax);
		Vector2 x1i = Vector2(x1, vmax);

		if (ce == EC.front()) { // first edge
			n0i = MakeNode(p0i);
			n1i = MakeNode(p1i);

			w0u = MakeVertex(p0,  x0b, v0.normal, v0.tangent, v0.bitangent);
			w0i = MakeVertex(p0,  x0i, v0.normal, v0.tangent, v0.bitangent);
			w0l = MakeVertex(p0,  x0b, v0.normal, v0.tangent, v0.bitangent);

			w1u = MakeVertex(p1u, x1b, v1.normal, v1.tangent, v1.bitangent);
			w1i = MakeVertex(p1i, x1i, v1.normal, v1.tangent, v1.bitangent);
			w1l = MakeVertex(p1l, x1b, v1.normal, v1.tangent, v1.bitangent);
		}
		else if (ce == EC.back()) { // last edge
			n0i = MakeNode(p0i);
			n1i = MakeNode(p1i);

			w0u = MakeVertex(p0u, x0b, v0.normal, v0.tangent, v0.bitangent);
			w0i = MakeVertex(p0i, x0i, v0.normal, v0.tangent, v0.bitangent);
			w0l = MakeVertex(p0l, x0b, v0.normal, v0.tangent, v0.bitangent);

			w1u = MakeVertex(p1,  x1b, v1.normal, v1.tangent, v1.bitangent);
			w1i = MakeVertex(p1,  x1i, v1.normal, v1.tangent, v1.bitangent);
			w1l = MakeVertex(p1,  x1b, v1.normal, v1.tangent, v1.bitangent);
		}
		else { // intermediate edges
			n0i = MakeNode(p0i);
			n1i = MakeNode(p1i);

			w0u = MakeVertex(p0u, x0b, v0.normal, v0.tangent, v0.bitangent);
			w0i = MakeVertex(p0i, x0i, v0.normal, v0.tangent, v0.bitangent);
			w0l = MakeVertex(p0l, x0b, v0.normal, v0.tangent, v0.bitangent);

			w1u = MakeVertex(p1u, x1b, v1.normal, v1.tangent, v1.bitangent);
			w1i = MakeVertex(p1i, x1i, v1.normal, v1.tangent, v1.bitangent);
			w1l = MakeVertex(p1l, x1b, v1.normal, v1.tangent, v1.bitangent);
		}

		NI.push_back(n0i);
		NI.push_back(n1i);

		WU.push_back(w0u);
		WU.push_back(w1u);
		WI.push_back(w0i);
		WI.push_back(w1i);
		WL.push_back(w0l);
		WL.push_back(w1l);
	}


	// Find all upper/lower faces
	FaceSet FUT = FaceSet(FU.begin(), FU.end());
	FaceSet FLT = FaceSet(FL.begin(), FL.end());
	
	std::function<void(Face*& f, std::unordered_set<Face*, FaceHash, FaceHash>& table)> AddFace = 
		[&](Face*& f, std::unordered_set<Face*, FaceHash, FaceHash>& table) -> void
	{
		if (FUT.find(f) != FUT.end()) { return; }
		if (FLT.find(f) != FLT.end()) { return; }

		for (auto e : EC) {
			for (uint8_t i = 0; i < 3; ++i) {
				// add face to table if it references either vertex (or node) of ce
				if (f->v[i] == e->p[0].second || f->v[i] == e->p[1].second) {
					table.insert(f);

					std::array<Face*,3> nbs;
					Neighbors(f, nbs);

					// continue with neighbors
					if (nbs[0]) { AddFace(nbs[0], table); }
					if (nbs[1]) { AddFace(nbs[1], table); }
					if (nbs[2]) { AddFace(nbs[2], table); }

					break;
				}
			}
		}
	};

	for (auto f : FU) { // find all upper faces
		std::array<Face*,3> nbs;
		Neighbors(f, nbs);

		AddFace(nbs[0], FUT);
		AddFace(nbs[1], FUT);
		AddFace(nbs[2], FUT);
	}

	for (auto f : FL) { // find all lower faces
		std::array<Face*,3> nbs;
		Neighbors(f, nbs);

		AddFace(nbs[0], FLT);
		AddFace(nbs[1], FLT);
		AddFace(nbs[2], FLT);
	}


	////////////////////////////////////
	// 2. CLEAVE CUT (UPDATE REFERENCES)

	for (uint32_t i = 0, j = 0; i < EC.size(); ++i, j+=2) {
		// center/upper/lower edges
		Edge* ec = EC[i];
		Edge* eu = EU[i];
		Edge* el = EL[i];

		// center/upper/lower nodes
		Node* n0 = ec->p[0].first;
		Node* n1 = ec->p[1].first;
		Node* n0u = NU[j];
		Node* n0l = NL[j];
		Node* n1u = NU[j+1];
		Node* n1l = NL[j+1];

		// center/upper/lower vertexes
		uint32_t v0 = ec->p[0].second;
		uint32_t v1 = ec->p[1].second;
		uint32_t v0u = VU[j];
		uint32_t v0l = VL[j];
		uint32_t v1u = VU[j+1];
		uint32_t v1l = VL[j+1];

		// update upper face references
		for (auto f : FUT) {
			for (uint8_t k = 0; k < 3; ++k) {
				// update face-node references
				if (f->n[k] == n0) { f->n[k] = n0u; }
				if (f->n[k] == n1) { f->n[k] = n1u; }

				// update face-vertex references
				if (f->v[k] == v0) { f->v[k] = v0u; }
				if (f->v[k] == v1) { f->v[k] = v1u; }

				// update face-edge reference
				if (f->e[k] == ec) {
					eu->f[0] = f;
					f->e[k] = eu;
				}
				
				// update edge-node references
				if (f->e[k]->n[0] == n0) { f->e[k]->n[0] = n0u; }
				if (f->e[k]->n[1] == n0) { f->e[k]->n[1] = n0u; }
				if (f->e[k]->n[0] == n1) { f->e[k]->n[0] = n1u; }
				if (f->e[k]->n[1] == n1) { f->e[k]->n[1] = n1u; }
			}
		}

		// update lower face references
		for (auto f : FLT) {
			for (uint8_t k = 0; k < 3; ++k) {
				// update face-node references
				if (f->n[k] == n0) { f->n[k] = n0l; }
				if (f->n[k] == n1) { f->n[k] = n1l; }

				// update face-vertex references
				if (f->v[k] == v0) { f->v[k] = v0l; }
				if (f->v[k] == v1) { f->v[k] = v1l; }

				// update face-edge reference
				if (f->e[k] == ec) {
					el->f[0] = f;
					f->e[k] = el;
				}

				// update edge-node references
				if (f->e[k]->n[0] == n0) { f->e[k]->n[0] = n0l; }
				if (f->e[k]->n[1] == n0) { f->e[k]->n[1] = n0l; }
				if (f->e[k]->n[0] == n1) { f->e[k]->n[0] = n1l; }
				if (f->e[k]->n[1] == n1) { f->e[k]->n[1] = n1l; }
			}
		}
	}

	
	///////////////////////////
	// 3. CREATE CUTTING GUTTER

	if (!gutter) { return; }
	
	for (uint32_t i = 0, j = 0; i < EC.size(); ++i, j+=2) {
		// upper/center/lower edges
		Edge* eu = EU[i];
		Edge* ec = EC[i];
		Edge* el = EL[i];

		// upper/inner/lower nodes
		Node* n0u = NU[j];
		Node* n0i = NI[j];
		Node* n0l = NL[j];
		Node* n1u = NU[j+1];
		Node* n1i = NI[j+1];
		Node* n1l = NL[j+1];

		// upper/inner/lower vertexes
		uint32_t w0u = WU[j];
		uint32_t w0i = WI[j];
		uint32_t w0l = WL[j];
		uint32_t w1u = WU[j+1];
		uint32_t w1i = WI[j+1];
		uint32_t w1l = WL[j+1];

		if (ec == EC.front()) { // first segment
			// inner edge
			Edge* ei = MakeEdge(n0i, n1i);

			// inner-to-upper/lower edges
			Edge* e1u = MakeEdge(n1u, n1i);
			Edge* e1l = MakeEdge(n1i, n1l);

			// inner upper/lower faces
			Face* fiu = MakeFace(n0u, n1u, n1i, w0u, w1u, w1i);
			Face* fil = MakeFace(n0l, n1i, n1l, w0l, w1i, w1l);

			// edge-face associations
			RegisterEdge(eu,  fiu);			// upper edge
			RegisterEdge(el,  fil);			// lower edge
			RegisterEdge(e1u, fiu);			// inner upper edge
			RegisterEdge(e1l, fil);			// inner lower edge
			RegisterEdge(ei,  fiu, fil);	// inner edge

			// face-edge associations
			RegisterFace(fiu, eu, e1u, ei);	// inner upper face
			RegisterFace(fil, ei, e1l, el);	// inner lower face
		}
		else if (ec == EC.back()) { // last segment
			// inner edge
			Edge* ei = MakeEdge(n0i, n1i);

			// inner-to-upper/lower edges
			Edge* e0u = MakeEdge(n0u, n0i);
			Edge* e0l = MakeEdge(n0i, n0l);

			// inner upper/lower faces
			Face* fiu = MakeFace(n0u, n1u, n0i, w0u, w1u, w0i);
			Face* fil = MakeFace(n0i, n1l, n0l, w0i, w1l, w0l);

			// edge-face associations
			RegisterEdge(eu,  fiu);			// upper edge
			RegisterEdge(el,  fil);			// lower edge
			RegisterEdge(e0u, fiu);			// inner upper edge
			RegisterEdge(e0l, fil);			// inner lower edge
			RegisterEdge(ei,  fiu, fil);	// inner edge

			// face-edge associations
			RegisterFace(fiu, eu, ei, e0u);	// inner upper face
			RegisterFace(fil, ei, el, e0l);	// inner lower face
		}
		else { // intermediate segments
			// inner edges
			Edge* eui = MakeEdge(n0i, n1u);
			Edge* eii = MakeEdge(n0i, n1i);
			Edge* eil = MakeEdge(n0l, n1i);

			// inner-to-upper/lower edges
			Edge* e0u = MakeEdge(n0i, n0u);
			Edge* e0l = MakeEdge(n0i, n0l);
			Edge* e1u = MakeEdge(n1i, n1u);
			Edge* e1l = MakeEdge(n1i, n1l);

			// inner upper/lower faces
			Face* fiu0 = MakeFace(n0u, n1u, n0i, w0u, w1u, w0i);
			Face* fiu1 = MakeFace(n0i, n1u, n1i, w0i, w1u, w1i);
			Face* fil0 = MakeFace(n0i, n1i, n0l, w0i, w1i, w0l);
			Face* fil1 = MakeFace(n0l, n1i, n1l, w0l, w1i, w1l);

			// edge-face associations
			RegisterEdge(eu,  fiu0);		// upper edge
			RegisterEdge(el,  fil1);		// lower edge
			RegisterEdge(e0u, fiu0);		// left inner upper edge
			RegisterEdge(e1u, fiu1);		// right inner upper edge
			RegisterEdge(e0l, fil0);		// left inner lower edge
			RegisterEdge(e1l, fil1);		// right inner lower edge
			RegisterEdge(eui, fiu0, fiu1);	// upper inner diagonal edge
			RegisterEdge(eii, fiu1, fil0);	// inner edge
			RegisterEdge(eil, fil0, fil1);	// lower inner diagonal edge

			// face-edge associations
			RegisterFace(fiu0, eu,  eui, e0u);
			RegisterFace(fiu1, eui, e1u, eii);
			RegisterFace(fil0, eii, eil, e0l);
			RegisterFace(fil1, eil, e1l, el );
		}

		// Remove center edge
		KillEdge(ec, true);
	}
}


void Mesh::ChainFaces(std::list<Link>& chain, std::map<Link, std::vector<Face*>>& cf, float r)
{
	// faces located within given radius from cutline
	FaceSet faces;

	// for each link: add incident face to collection associated with link
	for (auto& link : chain) {
		link.rank = static_cast<uint32_t>(cf.size()); // add comparison quantity to order links
		std::vector<Face*> Fl;
		Fl.push_back(link.f);
		cf.emplace(link, Fl);
	}

	std::function<void(Link&, Face*&)> NeighborWalk = [&](Link& link, Face*& face)
	{
		// Find neighboring faces
		std::array<Face*,3> neighbors;
		Neighbors(face, neighbors);

		// for each neighboring face
		for (Face*& neighbor : neighbors) {
			// skip face if invalid
			if (!neighbor) { continue; }

			// skip face if it lies on the cutline
			if (neighbor == cf.find(link)->second.at(0)) { continue; }

			// skip face if it was already added before
			if (faces.find(neighbor) != faces.end()) { continue; }

			// acquire texture-space vertices of face
			std::array<Vector2, 3> vertices = {
				mVertexes[neighbor->v[0]].texcoord,
				mVertexes[neighbor->v[1]].texcoord,
				mVertexes[neighbor->v[2]].texcoord
			};

			// iterate over vertices of face
			for (auto& vertex : vertices) {
				// compute distance from vertex to cutline segment
				//float dist = Math::PointLineDistance(vertex, link.x0, link.x1);
				Vector2 segcenter = Vector2::Lerp(link.x0, link.x1, 0.5f);
				float distance = Vector2::Distance(vertex, segcenter);

				// determine whether vertex lies inside radius
				if (distance <= r) {
					faces.insert(neighbor);
					NeighborWalk(link, neighbor); // continue recursion if face lies in radius
					break; // skip other vertices
				}
			}
		}
	};

	// find all faces nearest to each link
	for (auto& link : chain) {
		NeighborWalk(link, link.f);
	}

	// for each face surrounding the cut, determine which segment it is closest to
	for (auto face : faces) {
		Link minLink;
		float minDist = std::numeric_limits<float>::max();

		Vector2 t0 = mVertexes[face->v[0]].texcoord;
		Vector2 t1 = mVertexes[face->v[1]].texcoord;
		Vector2 t2 = mVertexes[face->v[2]].texcoord;

		Vector2 tricenter = Vector2::Barycentric(t0, t1, t2, 0.33f, 0.33f);

		for (auto& link : chain) {
			//float pointlineDistance = Math::PointLineDistance(tricenter, link.x0, link.x1);
			Vector2 segmentCenter = Vector2::Lerp(link.x0, link.x1, 0.5f);
			float distance = Vector2::Distance(tricenter, segmentCenter);

			if (distance < minDist) {
				minLink = link;
				minDist = distance;
			}
		}

		cf.at(minLink).push_back(face);
	}
}


void Mesh::ChainFaces(std::list<Link>& chain, std::map<Link, std::vector<Face*>>& cf_outer, std::map<Link, std::vector<Face*>>& cf_inner, float r_outer, float r_inner)
{
	// set of nearest faces associated with each link
	cf_outer.clear();
	cf_inner.clear();

	// faces located within given radius from cutline
	FaceSet f_outer;
	FaceSet f_inner;

	// for each link: add incident face to collection associated with link
	for (auto& l : chain) {
		l.rank = static_cast<uint32_t>(cf_outer.size()); // used as comparison factor to order links
		std::vector<Face*> Fl;
		Fl.push_back(l.f);

		cf_outer.emplace(l, Fl);
		cf_inner.emplace(l, Fl);
	}

	std::function<void(Link& link, Face*& face)> NeighborWalk = [&](Link& link, Face*& face)
	{
		// Find neighboring faces
		std::array<Face*,3> neighbors;
		Neighbors(face, neighbors);

		// for each neighboring face
		for (Face*& neighbor : neighbors) {
			// skip face if invalid
			if (!neighbor) { continue; }

			// skip face if it lies on the cutline
			if (neighbor == cf_outer.find(link)->second.at(0)) { continue; }

			// skip face if it was already added before
			if (f_outer.find(neighbor) != f_outer.end()) { continue; }

			// acquire texture-space vertices of face
			std::array<Vector2, 3> vertices = {
				mVertexes[neighbor->v[0]].texcoord,
				mVertexes[neighbor->v[1]].texcoord,
				mVertexes[neighbor->v[2]].texcoord
			};

			// iterate over vertices of face
			for (auto& vertex : vertices) {
				// compute distance from vertex to cutline segment
				//float dist = Math::PointLineDistance(vertex, link.x0, link.x1);
				Vector2 segcenter = Vector2::Lerp(link.x0, link.x1, 0.5f);
				float distance = Vector2::Distance(vertex, segcenter);

				// determine whether vertex lies inside radius
				if (distance <= r_outer)
				{
					f_outer.insert(neighbor);
					if (distance <= r_inner) { // add to inner faces if within that radius
						f_inner.insert(neighbor);
					}
					NeighborWalk(link, neighbor); // continue recursion if face lies in radius
					break; // skip other vertices
				}
			}
		}
	};

	std::function<void(Face*, LinkFaceMap&)> AssociateFaces = [&](Face* f, LinkFaceMap& cf)
	{
		Link l_min;
		float d_min = std::numeric_limits<float>::max();

		Vector2 t0 = mVertexes[f->v[0]].texcoord;
		Vector2 t1 = mVertexes[f->v[1]].texcoord;
		Vector2 t2 = mVertexes[f->v[2]].texcoord;

		Vector2 tricenter = Vector2::Barycentric(t0, t1, t2, 0.33f, 0.33f);

		for (auto& link : chain) {
			//float pointlineDistance = Math::PointLineDistance(tricenter, link.x0, link.x1);
			Vector2 segcenter = Vector2::Lerp(link.x0, link.x1, 0.5f);
			float dist = Vector2::Distance(tricenter, segcenter);

			if (dist < d_min) {
				l_min = link;
				d_min = dist;
			}
		}

		cf.at(l_min).push_back(f);
	};

	// find all faces nearest to each link
	for (auto& l : chain) {
		NeighborWalk(l, l.f);
	}
	
	// associate each face surrounding cutline with closest link
	for (auto f : f_outer) {
		AssociateFaces(f, cf_outer);
	}
	for (auto f : f_inner) {
		AssociateFaces(f, cf_inner);
	}
}


void Mesh::Neighbors(Face*& f, std::array<Face*,3>& neighbors)
{
	for (uint8_t i = 0; i < 3; ++i) {
		if (f->e[i]->f[0] == f) {
			neighbors[i] = f->e[i]->f[1];
		}
		else if (f->e[i]->f[1] == f) {
			neighbors[i] = f->e[i]->f[0];
		}
		else {
			throw std::exception("Degenerate mesh detected!");
		}
	}
}


void Mesh::Neighbors(Face*& f, std::array<std::pair<Face*,Edge*>,3>& neighbors)
{
	for (uint8_t i = 0; i < 3; ++i) {
		if (f->e[i]->f[0] == f) {
			neighbors[i] = std::pair<Face*, Edge*>(f->e[i]->f[1], f->e[i]);
		}
		else if (f->e[i]->f[1] == f) {
			neighbors[i] = std::pair<Face*, Edge*>(f->e[i]->f[0], f->e[i]);
		}
		else {
			throw std::exception("Degenerate mesh detected!");
		}
	}
}



/*******************************************************************************
Geometry
*******************************************************************************/

void Mesh::Split2(Face*& f, Edge*& es, Vector3 p, Edge** ec)
{
	//       n_1
	//       /|\ 
	//  e_1 / | \ e_2
	//     /  |  \ 
	//    /   |   \
	// n_0¯¯¯m_n¯¯¯n_2
	//       e_0

	// Get existing data
	std::array<Node*, 3> n;
	std::array<Edge*, 3> e;

	std::array<uint32_t, 3> i;
	std::array<Vertex, 3> v;

	// Acquire nodes, indexes, vertexes
	for (uint8_t k = 0; k < 3; ++k) {
		if (f->n[k] == es->n[0]) { // n0 (or n2)
			n[0] = f->n[k];
			i[0] = f->v[k];
			v[0] = mVertexes[i[0]];
		}
		else if (f->n[k] == es->n[1]) { // n2 (or n0)
			n[2] = f->n[k];
			i[2] = f->v[k];
			v[2] = mVertexes[i[2]];
		}
		else { // n1
			n[1] = f->n[k];
			i[1] = f->v[k];
			v[1] = mVertexes[i[1]];
		}
	}

	// Acquire edges
	for (uint8_t k = 0; k < 3; ++k) {
		if (f->e[k] == es) {
			e[2] = f->e[k];
			e[0] = f->e[(k+1) % 3];
			e[1] = f->e[(k+2) % 3];
			break;
		}
	}

	// Make sure n0 is to the left of the midpoint and n2 is to its right
	Vector3 N = Vector3::Normalize(mVertexes[f->v[0]].normal + mVertexes[f->v[1]].normal + mVertexes[f->v[2]].normal);
	Vector3 V = Vector3::Normalize(Vector3::Cross(n[1]->p - n[0]->p, n[2]->p - n[0]->p));

	if (Vector3::Dot(N, V) < 0) {
		std::swap(n[0], n[2]);
		std::swap(i[0], i[2]);
		std::swap(v[0], v[2]);
	}

	// Declare new data:
	Node* nm; // midpoint node
	uint32_t im; // midpoint index
	std::array<Edge*, 1> ei{}; // interior edge
	std::array<Edge*, 2> ex{}; // exterior edges
	std::array<Face*, 2> fc{}; // child faces

	// Create midpoint
	if (p == Vector3()) {
		nm = MakeNode(n[0], n[2]);
		im = MakeVertex(v[0], v[2]);
	}
	else { // specific point is given
		nm = MakeNode(p);
		im = MakeVertex(v[0], v[2], p);
	}

	// Create interior edge (splitting edge)
	ei[0] = MakeEdge(nm, n[1], im, i[1]);

	// Create exterior edges
	ex[0] = MakeEdge(n[0], nm);
	ex[1] = MakeEdge(nm, n[2]);

	// Create child faces
	fc[0] = MakeFace(nm, n[0], n[1], im, i[0], i[1]);
	fc[1] = MakeFace(nm, n[1], n[2], im, i[1], i[2]);

	// Register interior edge
	RegisterEdge(ei[0], fc[0], fc[1]);

	// Register exterior edges
	RegisterEdge(ex[0], fc[0]);
	RegisterEdge(ex[1], fc[1]);

	// Register faces
	RegisterFace(fc[0], ex[0], e[0], ei[0]);
	RegisterFace(fc[1], ei[0], e[1], ex[1]);

	// Update face adjacency of e0 and e1
	UpdateEdge(e[0], f, fc[0]);
	UpdateEdge(e[1], f, fc[1]);

	// Initialize output data
	if (ec) { *ec = ei[0]; }

	// Remove edge and face (but do not delete edge yet; may still be in use)
	KillEdge(es);
	KillFace(f, true);
}


void Mesh::Split3(Face*& f, Vector3 p, Edge** ec0, Edge** ec1, Edge** ec2)
{
	//       n_1
	//       / \ 
	//  e_0 /   \ e_1
	//     / n_m \ 
	//    /       \
	// n_0¯¯¯¯¯¯¯¯¯n_2
	//       e_2

	// Acquire edges, nodes, indexes, vertexes
	std::array<Edge*,    3> e = { f->e[0], f->e[1], f->e[2] };
	std::array<Node*,    3> n = { f->n[0], f->n[1], f->n[2] };
	std::array<uint32_t, 3> i = { f->v[0], f->v[1], f->v[2] };
	std::array<Vertex,   3> v = { mVertexes[i[0]], mVertexes[i[1]], mVertexes[i[2]] };

	// Declare new data
	Node*    nm; // midpoint node
	uint32_t im; // midpoint index
	std::array<Edge*, 3> ei{}; // interior edges
	std::array<Face*, 3> fc{}; // child faces

	// Create node
	if (p == Vector3()) {
		nm = MakeNode(n[0], n[1], n[2]);
		im = MakeVertex(v[0], v[1], v[2]);
	}
	else { // specific point is given
		nm = MakeNode(p);
		im = MakeVertex(v[0], v[1], v[2], p);
	}

	// Create interior edges
	ei[0] = MakeEdge(nm, n[0], im, i[0]);
	ei[1] = MakeEdge(nm, n[1], im, i[1]);
	ei[2] = MakeEdge(nm, n[2], im, i[2]);

	// Create child faces
	fc[0] = MakeFace(nm, n[0], n[1], im, i[0], i[1]);
	fc[1] = MakeFace(nm, n[1], n[2], im, i[1], i[2]);
	fc[2] = MakeFace(nm, n[2], n[0], im, i[2], i[0]);

	// Register interior edges
	RegisterEdge(ei[0], fc[2], fc[0]);
	RegisterEdge(ei[1], fc[0], fc[1]);
	RegisterEdge(ei[2], fc[1], fc[2]);

	// Register child faces
	RegisterFace(fc[0], ei[0], e[0], ei[1]);
	RegisterFace(fc[1], ei[1], e[1], ei[2]);
	RegisterFace(fc[2], ei[2], e[2], ei[0]);

	// Update face adjacency of e0, e1, e2
	UpdateEdge(e[0], f, fc[0]);
	UpdateEdge(e[1], f, fc[1]);
	UpdateEdge(e[2], f, fc[2]);

	// Initialize output data
	if (ec0) *ec0 = ei[0]; 
	if (ec1) *ec1 = ei[1];
	if (ec2) *ec2 = ei[2];

	// Remove face
	KillFace(f, true);
}


void Mesh::Split4(Face*& f)
{
	// Get existing data:
	std::array<Node*,    3> n = { f->n[0], f->n[1], f->n[2] };
	std::array<Edge*,    3> e = { f->e[0], f->e[1], f->e[2] };

	std::array<uint32_t, 3> i = { f->v[0], f->v[1], f->v[2] };
	std::array<Vertex,   3> v = { mVertexes[i[0]], mVertexes[i[1]], mVertexes[i[2]] };

	// Declare new data:
	std::array<uint32_t, 3> im{}; // midpoint indexes
	std::array<Node*,    3> nm{}; // midpoint nodes
	std::array<Edge*,    3> ei{}; // interior edges
	std::array<Edge*,    6> ex{}; // exterior edges
	std::array<Face*,    4> fc{}; // child faces

	// Create vertices
	im[0] = MakeVertex(v[0], v[1]);
	im[1] = MakeVertex(v[1], v[2]);
	im[2] = MakeVertex(v[2], v[0]);

	// Create nodes
	nm[0] = MakeNode(n[0], n[1]);
	nm[1] = MakeNode(n[1], n[2]);
	nm[2] = MakeNode(n[2], n[0]);

	// Create interior edges
	ei[0] = MakeEdge(nm[0], nm[1]);
	ei[1] = MakeEdge(nm[1], nm[2]);
	ei[2] = MakeEdge(nm[2], nm[0]);

	// Create exterior edges
	ex[0] = MakeEdge(n[0], nm[0]);
	ex[1] = MakeEdge(nm[0], n[1]);
	ex[2] = MakeEdge(n[1], nm[1]);
	ex[3] = MakeEdge(nm[1], n[2]);
	ex[4] = MakeEdge(n[2], nm[2]);
	ex[5] = MakeEdge(nm[2], n[0]);

	// Create child faces
	fc[0] = MakeFace( n[0], nm[0], nm[2],  i[0], im[0], im[2]);
	fc[1] = MakeFace(nm[0],  n[1], nm[1], im[0],  i[1], im[1]);
	fc[2] = MakeFace(nm[1],  n[2], nm[2], im[1],  i[2], im[2]);
	fc[3] = MakeFace(nm[0], nm[1], nm[2], im[0], im[1], im[2]);

	// Register interior edges
	RegisterEdge(ei[0], fc[1], fc[3]);
	RegisterEdge(ei[1], fc[2], fc[3]);
	RegisterEdge(ei[2], fc[0], fc[3]);

	// Register exterior edges
	RegisterEdge(ex[0], fc[0]);
	RegisterEdge(ex[1], fc[1]);
	RegisterEdge(ex[2], fc[1]);
	RegisterEdge(ex[3], fc[2]);
	RegisterEdge(ex[4], fc[2]);
	RegisterEdge(ex[5], fc[0]);

	// Register child faces
	RegisterFace(fc[0], ex[0], ei[2], ex[5]);
	RegisterFace(fc[1], ex[1], ex[2], ei[0]);
	RegisterFace(fc[2], ex[3], ex[4], ei[1]);
	RegisterFace(fc[3], ei[0], ei[1], ei[2]);

	// Remove edges and face
	KillEdge(e[0]);
	KillEdge(e[1]);
	KillEdge(e[2]);
	KillFace(f, true);
}


void Mesh::Split6(Face*& f)
{
	// Get existing data: 3 edges, 3 vertices
	std::array<Node*,    3> n = { f->n[0], f->n[1], f->n[2] };
	std::array<Edge*,    3> e = { f->e[0], f->e[1], f->e[2] };

	std::array<uint32_t, 3> i = { f->v[0], f->v[1], f->v[2] };
	std::array<Vertex,   3> v = { mVertexes[i[0]], mVertexes[i[1]], mVertexes[i[2]] };

	// Declare new data:
	std::array<uint32_t, 4> im{}; // midpoint verts; mv[0] is the center vert
	std::array<Node*,    4> nm{}; // midpoint nodes; mn[0] is the center node
	std::array<Edge*,    6> ei{}; // interior edges
	std::array<Edge*,    6> ex{}; // exterior edges
	std::array<Face*,    6> fc{}; // child faces

	// Create vertexes
	im[0] = MakeVertex(v[0], v[1], v[2]);
	im[1] = MakeVertex(v[0], v[1]);
	im[2] = MakeVertex(v[1], v[2]);
	im[3] = MakeVertex(v[2], v[0]);

	// Create nodes
	nm[0] = MakeNode(n[0], n[1], n[2]);
	nm[1] = MakeNode(n[0], n[1]);
	nm[2] = MakeNode(n[1], n[2]);
	nm[3] = MakeNode(n[2], n[0]);

	// Create interior edges
	ei[0] = MakeEdge(nm[0],  n[0]);
	ei[1] = MakeEdge(nm[0], nm[1]);
	ei[2] = MakeEdge(nm[0],  n[1]);
	ei[3] = MakeEdge(nm[0], nm[2]);
	ei[4] = MakeEdge(nm[0],  n[2]);
	ei[5] = MakeEdge(nm[0], nm[3]);

	// Create exterior edges
	ex[0] = MakeEdge( n[0], nm[1]);
	ex[1] = MakeEdge(nm[1],  n[1]);
	ex[2] = MakeEdge( n[1], nm[2]);
	ex[3] = MakeEdge(nm[2],  n[2]);
	ex[4] = MakeEdge( n[2], nm[3]);
	ex[5] = MakeEdge(nm[3],  n[0]);

	// Create child faces
	fc[0] = MakeFace(nm[0],  n[0], nm[1], im[0],  i[0], im[1]);
	fc[1] = MakeFace(nm[0], nm[1],  n[1], im[0], im[1],  i[1]);
	fc[2] = MakeFace(nm[0],  n[1], nm[2], im[0],  i[1], im[2]);
	fc[3] = MakeFace(nm[0], nm[2],  n[2], im[0], im[2],  i[2]);
	fc[4] = MakeFace(nm[0],  n[2], nm[3], im[0],  i[2], im[3]);
	fc[5] = MakeFace(nm[0], nm[3],  n[0], im[0], im[3],  i[0]);

	// Register interior edges
	RegisterEdge(ei[0], fc[0], fc[5]);
	RegisterEdge(ei[1], fc[1], fc[0]);
	RegisterEdge(ei[2], fc[2], fc[1]);
	RegisterEdge(ei[3], fc[3], fc[2]);
	RegisterEdge(ei[4], fc[4], fc[3]);
	RegisterEdge(ei[5], fc[5], fc[4]);

	// Register exterior edges (set faces)
	RegisterEdge(ex[0], fc[0]);
	RegisterEdge(ex[1], fc[1]);
	RegisterEdge(ex[2], fc[2]);
	RegisterEdge(ex[3], fc[3]);
	RegisterEdge(ex[4], fc[4]);
	RegisterEdge(ex[5], fc[5]);

	// Register child faces
	RegisterFace(fc[0], ei[0], ex[0], ei[1]);
	RegisterFace(fc[1], ei[1], ex[1], ei[2]);
	RegisterFace(fc[2], ei[2], ex[2], ei[3]);
	RegisterFace(fc[3], ei[3], ex[3], ei[4]);
	RegisterFace(fc[4], ei[4], ex[4], ei[5]);
	RegisterFace(fc[5], ei[5], ex[5], ei[0]);

	// Remove edges and face
	KillEdge(e[0]);
	KillEdge(e[1]);
	KillEdge(e[2]);
	KillFace(f, true);
}





/*******************************************************************************
Topology
*******************************************************************************/

void Mesh::GenerateTopology()
{
	// Use list of indices to set up faces.
	for (uint32_t i = 0; i < mIndexes.size(); i+=3) { // each triplet makes up a face
		// Acquire indexes
		uint32_t i0 = mIndexes[i+0];
		uint32_t i1 = mIndexes[i+1];
		uint32_t i2 = mIndexes[i+2];

		// Acquire vertexes
		Vertex& v0 = mVertexes[i0];
		Vertex& v1 = mVertexes[i1];
		Vertex& v2 = mVertexes[i2];

		// Create nodes
		Node* n0 = MakeNode(v0.position);
		Node* n1 = MakeNode(v1.position);
		Node* n2 = MakeNode(v2.position);

		// Create edges
		Edge* e0 = MakeEdge(n0, n1);
		Edge* e1 = MakeEdge(n1, n2);
		Edge* e2 = MakeEdge(n2, n0);

		// Create face
		Face* f = MakeFace(n0, n1, n2, i0, i1, i2);

		// Register edges
		RegisterEdge(e0, f);
		RegisterEdge(e1, f);
		RegisterEdge(e2, f);

		// Register face
		RegisterFace(f, e0, e1, e2);
	}
}


uint32_t Mesh::MakeVertex(Vector3& p, Vector2& x, Vector3& n, Vector4& t, Vector3& b)
{
	Vertex vertex;
	vertex.position  = p;
	vertex.texcoord  = x;
	vertex.normal    = n;
	vertex.tangent   = t;
	vertex.bitangent = b;

	uint32_t index = static_cast<uint32_t>(mVertexes.size());
	auto entry = mVertexTable.emplace(vertex, index);

	if (!entry.second) {
		index = entry.first->second; // already exists
	}
	else {
		mVertexes.push_back(vertex);
	}

	return index;
}

uint32_t Mesh::MakeVertex(Vertex& v0, Vertex& v1)
{
	Vertex vertex;
	vertex.position  = Vector3::Lerp(v0.position, v1.position, 0.5f);
	vertex.texcoord  = Vector2::Lerp(v0.texcoord, v1.texcoord, 0.5f);
	vertex.normal    = Vector3::Normalize(Vector3::Lerp(v0.normal, v1.normal, 0.5f));
	vertex.tangent   = Vector4::Normalize(Vector4::Lerp(v0.tangent, v1.tangent, 0.5f));
	vertex.bitangent = Vector3::Normalize(Vector3::Lerp(v0.bitangent, v1.bitangent, 0.5f));
	vertex.tangent.w = Math::Sign(Matrix(Vector3((const float*)vertex.tangent), 
		vertex.bitangent, vertex.normal).Determinant()); // bitangent handedness

	uint32_t index = static_cast<uint32_t>(mVertexes.size());
	auto entry = mVertexTable.emplace(vertex, index);

	if (!entry.second) {
		index = entry.first->second; // already exists
	}
	else {
		mVertexes.push_back(vertex);
	}

	return index;
}

uint32_t Mesh::MakeVertex(Vertex& v0, Vertex& v1, Vector3 p)
{
	// Parametric distance from v0 to p
	float t = Vector3::Distance(v0.position, p) / Vector3::Distance(v0.position, v1.position);

	Vertex vertex;
	vertex.position  = p;
	vertex.texcoord  = Vector2::Lerp(v0.texcoord, v1.texcoord, t);
	vertex.normal    = Vector3::Normalize(Vector3::Lerp(v0.normal, v1.normal, t));
	vertex.tangent   = Vector4::Normalize(Vector4::Lerp(v0.tangent, v1.tangent, t));
	vertex.bitangent = Vector3::Normalize(Vector3::Lerp(v0.bitangent, v1.bitangent, t));
	vertex.tangent.w = Math::Sign(Matrix(Vector3((const float*)vertex.tangent), 
		vertex.bitangent, vertex.normal).Determinant()); // bitangent handedness

	uint32_t index = static_cast<uint32_t>(mVertexes.size());
	auto entry = mVertexTable.emplace(vertex, index);

	if (!entry.second) {
		index = entry.first->second; // already exists
	}
	else {
		mVertexes.push_back(vertex);
	}

	return index;
}

uint32_t Mesh::MakeVertex(Vertex& v0, Vertex& v1, Vertex& v2)
{
	Vertex vertex;
	vertex.position  = Vector3::Barycentric(v0.position, v1.position, v2.position, (float)Math::cOneThird, (float)Math::cOneThird);
	vertex.texcoord  = Vector2::Barycentric(v0.texcoord, v1.texcoord, v2.texcoord, (float)Math::cOneThird, (float)Math::cOneThird);
	vertex.normal    = Vector3::Normalize(Vector3::Barycentric(v0.normal, v1.normal, v2.normal, (float)Math::cOneThird, (float)Math::cOneThird));
	vertex.tangent   = Vector4::Normalize(Vector4::Barycentric(v0.tangent, v1.tangent, v2.tangent, (float)Math::cOneThird, (float)Math::cOneThird));
	vertex.bitangent = Vector3::Normalize(Vector3::Barycentric(v0.bitangent, v1.bitangent, v2.bitangent, (float)Math::cOneThird, (float)Math::cOneThird));
	vertex.tangent.w = Math::Sign(Matrix(Vector3((const float*)vertex.tangent), vertex.bitangent, vertex.normal).Determinant());

	uint32_t index = static_cast<uint32_t>(mVertexes.size());
	auto entry = mVertexTable.emplace(vertex, index);

	if (!entry.second) {
		index = entry.first->second; // already exists
	}
	else {
		mVertexes.push_back(vertex);
	}

	return index;
}

uint32_t Mesh::MakeVertex(Vertex& v0, Vertex& v1, Vertex& v2, Vector3 p)
{
	// Compute barycentric coordinates of point inside face
	float u, v, w;
	Barycentric(p, v0.position, v1.position, v2.position, u, v, w);

	Vertex vertex;
	vertex.position  = p;
	vertex.texcoord  = Vector2::Barycentric(v0.texcoord, v1.texcoord, v2.texcoord, v, w);
	vertex.normal    = Vector3::Normalize(Vector3::Barycentric(v0.normal, v1.normal, v2.normal, v, w));
	vertex.tangent   = Vector4::Normalize(Vector4::Barycentric(v0.tangent, v1.tangent, v2.tangent, v, w));
	vertex.bitangent = Vector3::Normalize(Vector3::Barycentric(v0.bitangent, v1.bitangent, v2.bitangent, v, w));
	vertex.tangent.w = Math::Sign(Matrix(Vector3((const float*)vertex.tangent), vertex.bitangent, vertex.normal).Determinant());

	uint32_t index = static_cast<uint32_t>(mVertexes.size());
	auto entry = mVertexTable.emplace(vertex, index);

	if (!entry.second) {
		index = entry.first->second; // already exists
	}
	else {
		mVertexes.push_back(vertex);
	}

	return index;
}


Node* Mesh::MakeNode(Vector3& p)
{
	Node* node = new Node;
	node->p = p;

	auto entry = mNodeTable.insert(node);
	if (!entry.second) { // already exists
		delete node;
		node = (*entry.first);
	}
	else {
		mNodeArray.push_back(node);
	}

	return node;
}

Node* Mesh::MakeNode(Node*& n0, Node*& n1)
{
	Node* node = new Node;
	node->p = Vector3::Lerp(n0->p, n1->p, 0.5f);

	auto entry = mNodeTable.insert(node);
	if (!entry.second) { // already exists
		delete node;
		node = (*entry.first);
	}
	else {
		mNodeArray.push_back(node);
	}

	return node;
}

Node* Mesh::MakeNode(Node*& n0, Node*& n1, Node*& n2)
{
	Node* node = new Node;
	node->p = Vector3::Barycentric(n0->p, n1->p, n2->p, (float)Math::cOneThird, (float)Math::cOneThird);

	auto entry = mNodeTable.insert(node);
	if (!entry.second) { // already exists
		delete node;
		node = (*entry.first);
	}
	else {
		mNodeArray.push_back(node);
	}

	return node;
}


Edge* Mesh::MakeEdge(Node*& n0, Node*& n1)
{
	Edge* edge = new Edge;
	edge->n[0] = n0;
	edge->n[1] = n1;
	edge->f[0] = nullptr;
	edge->f[1] = nullptr;

	// Invariant for hashing: n0 should come geometrically before n1
	if (edge->n[1]->p < edge->n[0]->p) {
		std::swap(edge->n[0], edge->n[1]);
	}

	auto entry = mEdgeTable.insert(edge); // automatically discards duplicates
	if (!entry.second) { // already exists
		delete edge;
		edge = (*entry.first);
	}
	else {
		mEdgeArray.push_back(edge);
	}

	return edge;
}

Edge* Mesh::MakeEdge(Node*& n0, Node*& n1, uint32_t i0, uint32_t i1)
{
	Edge* edge = new Edge;
	edge->n[0] = n0;
	edge->n[1] = n1;
	edge->f[0] = nullptr;
	edge->f[1] = nullptr;
	edge->p[0] = std::make_pair(n0, i0);
	edge->p[1] = std::make_pair(n1, i1);

	// Invariant for hashing: n0 should come geometrically before n1
	if (edge->n[1]->p < edge->n[0]->p) {
		std::swap(edge->n[0], edge->n[1]);
	}

	auto entry = mEdgeTable.insert(edge); // automatically discards duplicates
	if (!entry.second) { // already exists
		delete edge;
		edge = (*entry.first);
	}
	else {
		mEdgeArray.push_back(edge);
	}

	return edge;
}


Face* Mesh::MakeFace(Node*& n0, Node*& n1, Node*& n2, uint32_t i0, uint32_t i1, uint32_t i2)
{
	Face* face = new Face;
	face->v[0] = i0;
	face->v[1] = i1;
	face->v[2] = i2;
	face->n[0] = n0;
	face->n[1] = n1;
	face->n[2] = n2;
	face->e[0] = nullptr;
	face->e[1] = nullptr;
	face->e[2] = nullptr;

	auto entry = mFaceTable.insert(face);
	if (!entry.second) { // already exists
		delete face;
		face = (*entry.first);
	}
	else {
		mFaceArray.push_back(face);
	}

	return face;
}



void Mesh::RegisterEdge(Edge*& e, Face*& f)
{
	if (e->f[0] == nullptr && e->f[1] == nullptr) {
		e->f[0] = f; // set first face
	}
	else if (e->f[0] != nullptr && e->f[1] == nullptr) {
		e->f[1] = f; // set second face
	}
	else if (e->f[0] == nullptr && e->f[1] != nullptr) {
		e->f[0] = f; // set first face and swap
		std::swap(e->f[0], e->f[1]);
	}
}

void Mesh::RegisterEdge(Edge*& e, Face*& f0, Face*& f1)
{
	e->f[0] = f0;
	e->f[1] = f1;
}

void Mesh::RegisterFace(Face*& f, Edge*& e0, Edge*& e1, Edge*& e2)
{
	f->e[0] = e0;
	f->e[1] = e1;
	f->e[2] = e2;
}


void Mesh::UpdateEdge(Edge*& e, Face*& f, Face*& fn)
{
	if (e->f[0] == f) {
		e->f[0] = fn;
	}
	else if (e->f[1] == f) {
		e->f[1] = fn;
	}
}



uint32_t Mesh::CopyVertex(Vertex& v)
{
	Vertex vertex = v;

	uint32_t index = static_cast<uint32_t>(mVertexes.size());
	auto entry = mVertexTable.emplace(vertex, index);

	if (!entry.second) {
		index = entry.first->second;
	}
	else {
		mIndexes.push_back(index);
		mVertexes.push_back(vertex);
	}

	return index;
}

Node* Mesh::CopyNode(Node*& n)
{
	Node* node = new Node;
	node->p = n->p;

	auto entry = mNodeTable.insert(node);
	if (!entry.second) {
		delete node;
		node = (*entry.first);
	}
	else {
		mNodeArray.push_back(node);
	}

	return node;
}

Edge* Mesh::CopyEdge(Edge*& e)
{
	Edge* edge = new Edge;
	edge->n[0] = CopyNode(e->n[0]);
	edge->n[1] = CopyNode(e->n[1]);
	edge->f[0] = nullptr;
	edge->f[1] = nullptr;

	if (edge->n[1]->p < edge->n[0]->p) {
		std::swap(edge->n[0], edge->n[1]);
	}

	auto entry = mEdgeTable.insert(edge);
	if (!entry.second) {
		delete edge;
		edge = (*entry.first);
	}
	else {
		mEdgeArray.push_back(edge);
	}

	return edge;
}

Face* Mesh::CopyFace(Face*& f)
{
	Face* face = new Face;
	face->v[0] = f->v[0];
	face->v[1] = f->v[1];
	face->v[2] = f->v[2];
	face->n[0] = MakeNode(f->n[0]->p);
	face->n[1] = MakeNode(f->n[1]->p);
	face->n[2] = MakeNode(f->n[2]->p);
	face->e[0] = MakeEdge(f->n[0], f->n[1]);
	face->e[1] = MakeEdge(f->n[1], f->n[2]);
	face->e[2] = MakeEdge(f->n[2], f->n[0]);

	mFaceTable.insert(face);
	mFaceArray.push_back(face);

	RegisterEdge(face->e[0], face);
	RegisterEdge(face->e[1], face);
	RegisterEdge(face->e[2], face);

	return face;
}


void Mesh::KillNode(Node*& n, bool del)
{
	mNodeTable.erase(n);
	mNodeArray.erase(std::remove(mNodeArray.begin(), mNodeArray.end(), n), mNodeArray.end());
	if (del && n) delete n;
}

void Mesh::KillEdge(Edge*& e, bool del)
{
	mEdgeTable.erase(e);
	mEdgeArray.erase(std::remove(mEdgeArray.begin(), mEdgeArray.end(), e), mEdgeArray.end());
	if (del && e) delete e;
}

void Mesh::KillFace(Face*& f, bool del)
{
	mFaceTable.erase(f);
	mFaceArray.erase(std::remove(mFaceArray.begin(), mFaceArray.end(), f), mFaceArray.end());
	if (del && f) delete f;
}

