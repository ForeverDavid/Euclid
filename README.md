# Introduction

Euclid is a header only library for geometry processing and shape analysis.

It contains some utilities and algorithms which extend and cooperate with other popular libraries around there, like Eigen(libigl), CGAL, OpenCV, to name a few.

The purpose of Euclid is not to replace any of the above packages, but to glue things together as well as to provide more algorithms which do not appear in those libraries.

# Modules Overview

## Math

- Common operations in linear algebra, matrix analysis, etc.
- Robust floating point comparisons.

## IO

- Mesh I/O, currently supporting .ply and .off file format.
- Mesh fixers, fixing degeneracies in input meshes.

## Geometry

- Convert raw mesh arrays read from the IO package into mesh data structures in CGAL and libigl and vice versa.
- Generate common mesh primitives.
- Discrete differential and geometric properties.
- Geodesic distance.

## Analysis

- Shape bounding boxes.
- Geometric shape segmentation.
- Shape descriptors.
- View selection.

## Render

- Fast cpu ray tracing.

## ImgProc

- Histogram processing.

# Dependencies

Some simple third-party libraries has already been included along the distribution of Euclid.
However, you'll need the following libraries when you use headers in Euclid that work with them, including

- [CGAL](https://www.cgal.org/index.html) for its mesh data structures and tons of algorithms.
- [Eigen](http://eigen.tuxfamily.org/index.php?title=Main_Page) for matrix manipulation as well as solving linear systems.
- [Libigl](http://libigl.github.io/libigl/) yet another simple but powerful geometry processing library in C++.
- [OpenCV](opencv.org) for algorithms which require operating on images.
- [Embree](embree.github.io) for fast cpu ray tracing.
- [Doxygen](http://www.stack.nl/~dimitri/doxygen/) for generating documentations.

Also, Euclid uses features in the C++17 standard, so you'll need a C++17 enabled compiler.

# Installation

Since it's a header only library, simply include the needed headers. Although be sure to configure dependencies properly.

If you want to use Euclid with CMake in your own projects, there is a simple find module file in cmake/Modules/FindEuclid.cmake.

# Getting Started

Here's an example which reads a mesh file, converts it to a CGAL::Surface_mesh data structure, computes its discrete gaussian curvatures and ouput the values into mesh colors.

```cpp
#include <algorithm>
#include <vector>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Surface_mesh.h>
#include <Euclid/IO/OffIO.h>
#include <Euclid/IO/PlyIO.h>
#include <Euclid/Geometry/MeshHelpers.h>
#include <Euclid/Geometry/MeshProperties.h>
#include <igl/colormap.h>

using Kernel = CGAL::Simple_cartesian<float>;
using Point_3 = Kernel::Point_3;
using Mesh = CGAL::Surface_mesh<Point_3>;

int main()
{
    // Read triangle mesh into buffers
    std::vector<float> positions;
    std::vector<unsigned> indices;
    Euclid::read_off<3>("Euclid_root/data/bumpy.off", positions, indices);

    // Generate a CGAL::Surface_mesh
    Mesh mesh;
    Euclid::make_mesh<3>(mesh, positions, indices);

    // Compute gaussian cuvatrues
    std::vector<float> curvatures;
    for (const auto& v : vertices(mesh)) {
        curvatures.push_back(Euclid::gaussian_curvature(v, mesh));
    }

    // Turn cuvatures into colors and output to a file
    auto [cmin, cmax] =
        std::minmax_element(curvatures.begin(), curvatures.end());
    std::vector<unsigned char> colors;
    for (auto c : curvatures) {
        auto curvature = (c - *cmin) / (*cmax - *cmin);
        float r, g, b;
        igl::colormap(igl::COLOR_MAP_TYPE_JET, curvature, r, g, b);
        colors.push_back(static_cast<unsigned char>(r * 255));
        colors.push_back(static_cast<unsigned char>(g * 255));
        colors.push_back(static_cast<unsigned char>(b * 255));
    }
    Euclid::write_ply<3>(
        "outdir/bumpy.ply", positions, nullptr, nullptr, &indices, &colors);
}
```

And here's the result rendered using MeshLab.

![](./resource/bumpy_gaussian.png)

# Examples

Currently there's no tutorial-like examples. However, you could check the test cases to see the usage of most functions.

# License

MIT for code not related to any third-party libraries.

Otherwise it should follow whatever license the third-party libraries require, I guess?
