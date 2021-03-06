#include <Euclid/Geometry/MeshHelpers.h>
#include <Euclid/Math/Vector.h>
#include <CGAL/subdivision_method_3.h>
#include <algorithm>

namespace Euclid
{

template<typename Mesh, typename Point_3>
void make_subdivision_sphere(
    Mesh& mesh,
    const Point_3& center,
    typename CGAL::Kernel_traits<Point_3>::Kernel::FT radius,
    int iterations)
{
    CGAL::make_icosahedron(mesh, Point_3(0, 0, 0));
    CGAL::Subdivision_method_3::Loop_subdivision(mesh, iterations);

    // Transform the sphere after subdivision
    auto vpmap = get(boost::vertex_point, mesh);
    for (const auto& v : vertices(mesh)) {
        auto len = length(vpmap[v] - CGAL::ORIGIN);
        auto x = vpmap[v].x() * radius / len + center.x();
        auto y = vpmap[v].y() * radius / len + center.y();
        auto z = vpmap[v].z() * radius / len + center.z();
        vpmap[v] = Point_3(x, y, z);
    }
}

} // namespace Euclid
