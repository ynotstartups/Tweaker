#include <iostream>
#include <array>
#include <unordered_map>

#include <vcg/complex/complex.h>
#include <wrap/io_trimesh/import_stl.h>
#include <vcg/complex/algorithms/clean.h>

class MyVertex; class MyFace;
struct MyUsedTypes : public vcg::UsedTypes<vcg::Use<MyVertex>   ::AsVertexType,
                            vcg::Use<MyFace>     ::AsFaceType>{};
class MyVertex  : public vcg::Vertex< MyUsedTypes,
    vcg::vertex::Coord3f,
    vcg::vertex::Normal3f,
    vcg::vertex::Mark,
    vcg::vertex::BitFlags  >{};

class MyFace    : public vcg::Face< MyUsedTypes,
    vcg::face::FFAdj,
    vcg::face::Normal3f,
    vcg::face::VertexRef,
    vcg::face::Mark,
    vcg::face::BitFlags > {};
class MyMesh    : public vcg::tri::TriMesh< std::vector<MyVertex>, std::vector<MyFace> > {};

bool loadMesh(MyMesh & mesh, const std::string filepath) {

    int a = 2; // TODO: understand what this is
    if(vcg::tri::io::ImporterSTL<MyMesh>::Open(mesh, filepath.c_str(),  a))
    {
        printf("Error reading file  %s\n", filepath.c_str());
        return false;
    }

    bool RemoveDegenerateFlag=false;
    vcg::tri::Clean<MyMesh>::RemoveDuplicateVertex(mesh, RemoveDegenerateFlag);

    return true;
}

struct ArrayHasher {
    std::size_t operator() (const std::array<float, 3> & a) const {
        std::size_t h = 0;
        for (auto e : a) {
            h ^= std::hash<float>{}(e) + 0x9e3779b9 + (h<<6) + (h>>2);
        }
        return h;
    }
};

std::array<float, 3> test = {1, 2, 3};
std::array<float, 3> test0 = {1, 2, 3};
std::array<float, 3> test1 = {1, 1, 1};
std::array<float, 3> test2 = {1, 0, 0};
std::array<float, 3> test3 = {1, 2, 3};

std::unordered_map< std::array<float, 3>, float, ArrayHasher> counter;

int main( int argc, char *argv[] )
{
    std::string filepath = "meshes/test.stl";

    if (argc < 2) {
        printf("not stl file given use default \n");
    } else {
        filepath = argv[1];
    }

    MyMesh mesh;
    bool successfulLoadMesh = loadMesh(mesh, filepath);
    if (not successfulLoadMesh) {
        return 1;
    }

    vcg::tri::UpdateNormal<MyMesh>::PerFace(mesh);

    for (auto fi = mesh.face.begin(); fi!=mesh.face.end(); ++fi) {
        // printf("normal 0 %f ", (*fi).N()[0]);
        // printf("normal 1 %f ", (*fi).N()[1]);
        // printf("normal 2 %f \n", (*fi).N()[2]);
        std::array<float, 3> test = {(*fi).N()[0], (*fi).N()[1], (*fi).N()[2]};
        const float doubleArea=DoubleArea(*fi);
        counter[test]+=doubleArea;
    }

    for ( auto it = counter.begin(); it != counter.end(); ++it ) {
        if (it->second > 1)
            std::cout << ":" << it->second;
    }
    return 0;
}
