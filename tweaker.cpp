#include <iostream>
#include <array>
#include <unordered_map>

#include <vcg/complex/complex.h>
#include <wrap/io_trimesh/import_stl.h>
#include <wrap/io_trimesh/export_stl.h>
#include <vcg/complex/algorithms/clean.h>
#include <vcg/complex/algorithms/update/position.h>

#include <Eigen/Geometry>

class MyVertex; class MyFace;
struct MyUsedTypes : public vcg::UsedTypes<vcg::Use<MyVertex>   ::AsVertexType,
                            vcg::Use<MyFace>     ::AsFaceType>{};
class MyVertex  : public vcg::Vertex< MyUsedTypes,
    vcg::vertex::Coord3f,
    vcg::vertex::Normal3f,
    vcg::vertex::Mark,
    vcg::vertex::BitFlags  >{};

class MyFace    : public vcg::Face< MyUsedTypes,
    vcg::face::Normal3f,
    vcg::face::VertexRef
     > {};
class MyMesh    : public vcg::tri::TriMesh< std::vector<MyVertex>, std::vector<MyFace> > {};

float v[18][3] = {
    {0, 0, -1}, {0.70710678, 0, -0.70710678}, {0, 0.70710678, -0.70710678},
    {-0.70710678, 0, -0.70710678}, {0, -0.70710678, -0.70710678},
    {1, 0, 0}, {0.70710678, 0.70710678, 0}, {0, 1, 0}, {-0.70710678, 0.70710678, 0},
    {-1, 0, 0}, {-0.70710678, -0.70710678, 0}, {0, -1, 0}, {0.70710678, -0.70710678, 0},
    {0.70710678, 0, 0.70710678}, {0, 0.70710678, 0.70710678},
    {-0.70710678, 0, 0.70710678}, {0, -0.70710678, 0.70710678}, {0, 0, 1}
};

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

float bottom_area(MyMesh & mesh,
        MyMesh::PerFaceAttributeHandle<float> face_area,
        float first_layer_height=0.2) {
    vcg::tri::UpdateBounding<MyMesh>::Box(mesh);
    float min_z = mesh.bbox.min.Z();
    float z = min_z + first_layer_height;

    float bottom_area = 0;
    for (auto fi = mesh.face.begin(); fi!=mesh.face.end(); ++fi) {

        float v0z = (*fi).V(0)->cP()[2];
        float v1z = (*fi).V(1)->cP()[2];
        float v2z = (*fi).V(2)->cP()[2];

        if (v0z < z or v1z < z or v2z < z)
            bottom_area += face_area[*fi];
    }

    return bottom_area;
}

struct ArrayHasher { // do it directly on point3f
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

    MyMesh::PerFaceAttributeHandle<float> face_area = vcg::tri::Allocator<MyMesh>::GetPerFaceAttribute<float> (mesh, std::string("Area"));

    bool successfulLoadMesh = loadMesh(mesh, filepath);
    if (not successfulLoadMesh) {
        return 1;
    }

    vcg::tri::UpdateNormal<MyMesh>::PerFaceNormalized(mesh);

    for (auto fi = mesh.face.begin(); fi!=mesh.face.end(); ++fi) {
        // printf("normal 0 %f ", (*fi).N()[0]);
        // printf("normal 1 %f ", (*fi).N()[1]);
        // printf("normal 2 %f \n", (*fi).N()[2]);
        std::array<float, 3> test = {(*fi).N()[0], (*fi).N()[1], (*fi).N()[2]};
        const float doubleArea=DoubleArea(*fi);
        face_area[fi] = doubleArea;
        counter[test]+=doubleArea;
    }

    std::vector<std::pair<std::array<float, 3>, float>> top_normal(10);
    std::partial_sort_copy(counter.begin(), counter.end(),
            top_normal.begin(),
            top_normal.end(),
            [](std::pair<std::array<float, 3>, float> const& l,
               std::pair<std::array<float, 3>, float> const& r)
            {
            return l.second >= r.second;
            }
    );

    for ( auto it = top_normal.begin(); it != top_normal.end(); ++it ) {
            printf("normal %f %f %f %f \n",it->first[0], it->first[1], it->first[2], it->second);
    }

    // vcg::tri::UpdatePosition<MyMesh>::Matrix(mesh, )

    const Eigen::Vector3f to = {0, 0, -1};
    const Eigen::Vector3f from = {top_normal[0].first[0], top_normal[0].first[1], top_normal[0].first[2]};

    Eigen::Matrix3f r = Eigen::Quaternionf().setFromTwoVectors(from,to).toRotationMatrix();
    Eigen::Matrix4f mat4 = Eigen::Matrix4f::Identity(); mat4.block(0,0,3,3) = r;

    vcg::Matrix44<float> transformation_matrix;
    transformation_matrix.FromEigenMatrix(mat4);

    // int count = 0;
    // for (auto vi = mesh.vert.begin(); vi!=mesh.vert.end(); ++vi) {
        // count++;
        // if (count == 10) break;
        // printf("before v %f %f %f\n", vi->cP()[0], vi->cP()[1], vi->cP()[2]);
    // }
    bool update_normals = false;
    vcg::tri::UpdatePosition<MyMesh>::Matrix(mesh, transformation_matrix, update_normals);
    std::cout << r << std::endl;

    // count = 0;
    // for (auto vi = mesh.vert.begin(); vi!=mesh.vert.end(); ++vi) {
        // count++;
        // if (count == 10) break;
        // printf("after v %f %f %f\n", vi->cP()[0], vi->cP()[1], vi->cP()[2]);
    // }

    printf("bottom area %f \n", bottom_area(mesh, face_area));

    vcg::tri::io::ExporterSTL<MyMesh>::Save(mesh, "./out/tweaked.stl");
    return 0;
}
