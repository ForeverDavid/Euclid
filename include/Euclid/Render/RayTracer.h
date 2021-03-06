/** Render mesh using ray tracing.
 *
 *  Many times geometry algorithms require analysis on the rendered views.
 *  This package utilizes Embree to do fast CPU ray tracing.
 *
 *  @defgroup PkgRayTracer Ray-Tracer
 *  @ingroup PkgRender
 */
#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <limits>
#include <vector>

#include <Eigen/Dense>
#include <embree3/rtcore.h>

namespace Euclid
{
/** @{*/

/** The film plane.
 *
 */
struct Film
{
    float width;
    float height;
};

/** A simple positionable camera model.
 *
 *  This class uses the right-handed coordinate system.
 */
class Camera
{
public:
    /** Create a Camera with default paramters.*/
    Camera() = default;

    /** Create a Camera.
     *
     *  Position the camera using position, focus and up.
     *
     *  @param position Position.
     *  @param focus Focus.
     *  @param up Rough up direction.
     */
    Camera(const Eigen::Vector3f& position,
           const Eigen::Vector3f& focus = Eigen::Vector3f::Zero(),
           const Eigen::Vector3f& up = Eigen::Vector3f(0.0f, 1.0f, 0.0f));

    virtual ~Camera() = default;

    /** Position the camera according to the parameteres.*/
    void lookat(const Eigen::Vector3f& position,
                const Eigen::Vector3f& focus,
                const Eigen::Vector3f& up);

    /** Generate an embree rayhit structure.
     *
     *  Generate a ray for a pixel (s, t) on the film plane.
     *  The parameter of the ray ranges in [near, far). The returned RTCRayHit
     *  structure also has its hit.geomID filed set to RTC_INVALID_GEOMETRY_ID.
     *
     *  @param s The u coordinate on the film plane, ranges in [0, 1).
     *  @param t The v coordinate on the film plane, ranges in [0, 1).
     *  @param near The value of ray parameter for the near end point.
     *  @param far The value of ray parameter for the far end point.
     */
    virtual RTCRayHit gen_ray(
        float s,
        float t,
        float near = 0.0f,
        float far = std::numeric_limits<float>::max()) const = 0;

public:
    /** Camera position.*/
    Eigen::Vector3f pos{ 0.0f, 0.0f, 0.0f };

    /** U direction.*/
    Eigen::Vector3f u{ 1.0f, 0.0f, 0.0f };

    /** V direction.*/
    Eigen::Vector3f v{ 0.0f, 1.0f, 0.0f };

    /** Negative view direction.*/
    Eigen::Vector3f dir{ 0.0f, 0.0f, 1.0f };

    /** Film plane.*/
    Film film{ 256, 256 };
};

/** A persective camera.
 *
 *  The range of visible frustum of a perspective camera is determined
 *  by the field of view and aspect ratio.
 */
class PerspectiveCamera : public Camera
{
public:
    /** Create a PerspectiveCamera using default parameters.*/
    PerspectiveCamera() : Camera(){};

    /** Create a PerspectiveCamera.
     *
     *  In addition to camera position and orientation, a perspective camera
     *  uses field of view and apsect ratio to determine the extent of the
     *  film plane.
     *
     *  @param position Position.
     *  @param focus Focus.
     *  @param up Rough up direction.
     *  @param vfov Vertical field of view in degrees.
     *  @param aspect Aspect ratio.
     */
    PerspectiveCamera(const Eigen::Vector3f& position,
                      const Eigen::Vector3f& focus = Eigen::Vector3f::Zero(),
                      const Eigen::Vector3f& up = Eigen::Vector3f(0.0f,
                                                                  1.0f,
                                                                  0.0f),
                      float vfov = 90.0f,
                      float aspect = 1.0f);

    /** Set aspect ratio.*/
    void set_aspect(unsigned width, unsigned height);

    /** Set vertical fov in degrees.*/
    void set_fov(float vfov);

    /** Generate an embree rayhit structure.
     *
     *  The ray's origin be at the camera position and points to the pixel
     *  (s, t) on the film plane.
     */
    RTCRayHit gen_ray(
        float s,
        float t,
        float near = 0.0f,
        float far = std::numeric_limits<float>::max()) const override;
};

/** An orthogonal camera.
 *
 *  The range of visible frustum of an orthogonal camera is determined
 *  by the extent of film plane in world space.
 */
class OrthogonalCamera : public Camera
{
public:
    /** Create an OrthogonalCamera using default parameters.*/
    OrthogonalCamera() : Camera() {}

    /** Create an OrthogonalCamera.
     *
     *  In addition to camera position and orientation, an orthogonal camera
     *  specifies width and height of the film plane directly.
     *
     *  @param position Position.
     *  @param focus Focus.
     *  @param up Rough up direction.
     *  @param width Width of the film plane in world space.
     *  @param height Height of the film plane in world space.
     */
    OrthogonalCamera(const Eigen::Vector3f& position,
                     const Eigen::Vector3f& focus = Eigen::Vector3f::Zero(),
                     const Eigen::Vector3f& up = Eigen::Vector3f(0.0f,
                                                                 1.0f,
                                                                 0.0f),
                     float width = 256,
                     float height = 256);

    /** Set the extent of the film plane.*/
    void set_extent(float width, float height);

    /** Generate an embree rayhit structure.
     *
     *  The ray's origin be at the pixel (s, t) on the film plane and points
     *  to the camera viewing direction.
     */
    RTCRayHit gen_ray(
        float s,
        float t,
        float near = 0.0f,
        float far = std::numeric_limits<float>::max()) const override;
};

/** A simple Phong material model.
 *
 */
struct Material
{
    Eigen::Array3f ambient;
    Eigen::Array3f diffuse;
};

/** A simple ray tracer.
 *
 *  This ray tracer could render the shaded, depth or silhouette image of a
 *  single mesh model.
 */
class RayTracer
{
public:
    /** Create a ray tracer.
     *
     *  @param threads Set to 0 to use number of hardware threads.
     */
    explicit RayTracer(int threads = 0);

    ~RayTracer();

    /** Attach geoemtry to the ray tracer.
     *
     *  @param positions The geometry's positions buffer.
     *  @param indices The geometry's indices buffer.
     *  @param type The geometry's type, be either RTC_GEOMETRY_TYPE_TRIANGLE or
     *  RTC_GEOMETRY_TYPE_QUAD.
     *
     *  #### Note
     *  This class can only render one mesh at a time. Attach geometry once
     *  again will automatically release the previously attached geometry.
     */
    template<typename FT, typename IT>
    void attach_geometry(const std::vector<FT>& positions,
                         const std::vector<IT>& indices,
                         RTCGeometryType type = RTC_GEOMETRY_TYPE_TRIANGLE);

    /** Attach geoemtry to the ray tracer.
     *
     *  When using a shared geometry buffer, the user is responsible of padding
     *  the positions buffer with one more float for Embree's SSE instructions
     *  to work correctly.
     *
     *  @param positions The geometry's positions buffer.
     *  @param indices The geometry's indices buffer.
     *  @param type The geometry's type, be either RTC_GEOMETRY_TYPE_TRIANGLE or
     *  RTC_GEOMETRY_TYPE_QUAD.
     *
     *  #### Note
     *  This class can only render one mesh at a time. Attach geometry once
     *  again will automatically release the previously attached geometry.
     */
    void attach_geometry_shared(
        const std::vector<float>& positions,
        const std::vector<unsigned>& indices,
        RTCGeometryType type = RTC_GEOMETRY_TYPE_TRIANGLE);

    /** Release geometry.*/
    void release_geometry();

    /** Change the material of the model.*/
    void set_material(const Material& material);

    /** Render the mesh into a shaded image.
     *
     *  This function renders the mesh with simple lambertian shading and store
     *  the pixel values to an array, using a point light located at the camera
     *  position.
     *
     *  @param pixels Output pixels
     *  @param camera Camera.
     *  @param width Image width.
     *  @param height Image height.
     *  @param samples Number of samples per pixel.
     *  @param interleaved If true, pixels are stored like [RGBRGBRGB...],
     *  otherwise pixels are stored like [RRR...GGG...BBB...].
     */
    template<typename T>
    void render_shaded(T* pixels,
                       const Camera& camera,
                       int width,
                       int height,
                       int samples = 1,
                       bool interleaved = true);

    /** Render the mesh into a depth image.
     *
     *  @param pixels Output pixels.
     *  @param camera Camera.
     *  @param width Image width.
     *  @param height Image height.
     *  @param tone_mapped If true, depth values are mapped into range
     *  [0, 255), otherwise they will not be modified.
     */
    template<typename T>
    void render_depth(T* pixels,
                      const Camera& camera,
                      int width,
                      int height,
                      bool tone_mapped = true);

    /** Render the mesh into a silhouette image.
     *
     *  @param pixels Output pixels.
     *  @param camera Camera.
     *  @param width Image width.
     *  @param height Image height.
     */
    template<typename T>
    void render_silhouette(T* pixels,
                           const Camera& camera,
                           int width,
                           int height);

private:
    RTCDevice _device;
    RTCScene _scene;
    RTCGeometry _geometry;
    int _geom_id = -1;
    Material _material;
};

/** @}*/
} // namespace Euclid

#include "src/RayTracer.cpp"
