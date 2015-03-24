/**
 * @file triangle.cpp
 * @brief Function definitions for the Triangle class.
 *
 * @author Eric Butler (edbutler)
 */

#include "scene/triangle.hpp"
#include "application/opengl.hpp"
#include "math/math.hpp"

namespace _462 {

Triangle::Triangle()
{
    vertices[0].material = 0;
    vertices[1].material = 0;
    vertices[2].material = 0;
    isBig=true;
}

Triangle::~Triangle() { }

void Triangle::render() const
{
    bool materials_nonnull = true;
    for ( int i = 0; i < 3; ++i )
        materials_nonnull = materials_nonnull && vertices[i].material;

    // this doesn't interpolate materials. Ah well.
    if ( materials_nonnull )
        vertices[0].material->set_gl_state();

    glBegin(GL_TRIANGLES);

#if REAL_FLOAT
    glNormal3fv( &vertices[0].normal.x );
    glTexCoord2fv( &vertices[0].tex_coord.x );
    glVertex3fv( &vertices[0].position.x );

    glNormal3fv( &vertices[1].normal.x );
    glTexCoord2fv( &vertices[1].tex_coord.x );
    glVertex3fv( &vertices[1].position.x);

    glNormal3fv( &vertices[2].normal.x );
    glTexCoord2fv( &vertices[2].tex_coord.x );
    glVertex3fv( &vertices[2].position.x);
#else
    glNormal3dv( &vertices[0].normal.x );
    glTexCoord2dv( &vertices[0].tex_coord.x );
    glVertex3dv( &vertices[0].position.x );

    glNormal3dv( &vertices[1].normal.x );
    glTexCoord2dv( &vertices[1].tex_coord.x );
    glVertex3dv( &vertices[1].position.x);

    glNormal3dv( &vertices[2].normal.x );
    glTexCoord2dv( &vertices[2].tex_coord.x );
    glVertex3dv( &vertices[2].position.x);
#endif

    glEnd();

    if ( materials_nonnull )
        vertices[0].material->reset_gl_state();
}

// added by m.ji
bool Triangle::intersect(const Ray& r, real_t& t_out) {
   
    //std::cout << orientation << std::endl;

    Vector3 vA = vertices[0].position;
    Vector3 vB = vertices[1].position;
    Vector3 vC = vertices[2].position;

    Vector3 edgeAB = vB - vA;
    Vector3 edgeAC = vC - vA;
    Vector3 n = cross(edgeAB, edgeAC);
    //Vector3 n = cross(b, a);

    /////////////////////
    // Finding point p //
    /////////////////////
    float nDotRay = dot(n, r.d);
    if (nDotRay == 0)
        return false;   // they are parallel

    float d = - dot(n, vA); // d of plane equation. TODO: this can be precomputed later
    float t = - (dot(n, r.e) + d) / nDotRay; // t of point p equation
    //float t = -(dot(n, Vector3 (0,0,0)) + d) / nDotRay; // t of point p equation
    if (t < 0)
        return false;
    
    // calculate intersection point p with the plane
    Vector3 p = r.e + t * r.d;
    // Vector3 p = t * r.d;

    /////////////////////
    // Testing p is in //
    /////////////////////

    // test with edge 1 - inside or outside?
    Vector3 VtoP = p - vA;
    w = dot(n, cross(edgeAB, VtoP));    // preserve for barycentric coordinate
    if (w < 0)
        return false;   // p is outside of the triangle

    // test with edge 2
    Vector3 edge;
    edge = vC - vB;
    VtoP = p - vB;
    if (dot(n, cross(edge, VtoP)) < 0)
        return false;   // p is outside of the triangle

    // test with edge 3
    edge = vA - vC;
    VtoP = p - vC;
    v = dot(n, cross(edge, VtoP));    // perserve for barycentric coordinate
    if (v < 0)
        return false;   // p is outside of the triangle

    // ray intersects this triangle

    // n squared length for barycentric coord calculation
    float n_len_sqr = dot (n,n);
    v = v / n_len_sqr;
    w = w / n_len_sqr;
    u = 1 - v - w;
    
    // update time t  
    // epsilon can be computed better with normal in direct illumination?
    // also this might not work well with refractions?
    //t_out = t - EPS;    
    t_out = t;    
    return true;
}

// hitPos is not used but there for sphere's getNormal
Vector3 Triangle::getNormal (const Vector3& hitPos) {

    Vector3 normal = vertices[0].normal * u +
            vertices[1].normal * v +
            vertices[2].normal * w;

    return normMat * normal;
}

Color3 Triangle::getSpecular () {
    return vertices[0].material->specular * u +
           vertices[1].material->specular * v +
           vertices[2].material->specular * w;
}
    
real_t Triangle::getRefractionIdx () {
    return vertices[0].material->refractive_index * u +
           vertices[1].material->refractive_index * v +
           vertices[2].material->refractive_index * w;
}

Color3 Triangle::getAmbient() {
    return  vertices[0].material->ambient * u +
            vertices[1].material->ambient * v +
            vertices[2].material->ambient * w;
}

Color3 Triangle::getDiffuse() {
    return vertices[0].material->diffuse * u +
           vertices[1].material->diffuse * v +
           vertices[2].material->diffuse * w;
}

Color3 Triangle::getTexColor() {
    Vector2 tex_coord_ = vertices[0].tex_coord * u +
                         vertices[1].tex_coord * v +
                         vertices[2].tex_coord * w;
    return vertices[0].material->texture.sample (tex_coord_) * u +
           vertices[1].material->texture.sample (tex_coord_) * v +
           vertices[2].material->texture.sample (tex_coord_) * w;
}
} /* _462 */
