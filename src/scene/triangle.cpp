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
bool Triangle::intersect(const Ray& r, real_t& t_out, Intersection& inter) {
   
    float u, v, w;

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

    // barycentric coordinate calculation
    float n_len_sqr = dot (n,n);
    inter.bary.y = v / n_len_sqr;
    inter.bary.z = w / n_len_sqr;
    inter.bary.x = 1 - inter.bary.y - inter.bary.z;
    
    v = v / n_len_sqr;
    w = w / n_len_sqr;
    u = 1 - v - w;
    // update time t  
    // epsilon can be computed better with normal in direct illumination?
    // also this might not work well with refractions?
    t_out = t - EPS;    
    //t_out = t;    
    return true;
}

void Triangle::getPositionInfo (Intersection& inter) {
    inter.normal = vertices[0].normal * inter.bary.x +
                   vertices[1].normal * inter.bary.y +
                   vertices[2].normal * inter.bary.z;
    inter.normal = normalize(normMat * inter.normal);

    inter.ambient = vertices[0].material->ambient * inter.bary.x +
                    vertices[1].material->ambient * inter.bary.y +
                    vertices[2].material->ambient * inter.bary.z;

    inter.nt = vertices[0].material->refractive_index * inter.bary.x +
               vertices[1].material->refractive_index * inter.bary.y +
               vertices[2].material->refractive_index * inter.bary.z;

    inter.specular = vertices[0].material->specular * inter.bary.x +
                     vertices[1].material->specular * inter.bary.y +
                     vertices[2].material->specular * inter.bary.z;

    inter.diffuse = vertices[0].material->diffuse * inter.bary.x +
                     vertices[1].material->diffuse * inter.bary.y +
                     vertices[2].material->diffuse * inter.bary.z;

    Vector2 tex_coord_ = vertices[0].tex_coord * inter.bary.x +
                         vertices[1].tex_coord * inter.bary.y +
                         vertices[2].tex_coord * inter.bary.z;
    inter.texture = 
           vertices[0].material->texture.sample (tex_coord_) * inter.bary.x +
           vertices[1].material->texture.sample (tex_coord_) * inter.bary.y +
           vertices[2].material->texture.sample (tex_coord_) * inter.bary.z;
}
} /* _462 */
