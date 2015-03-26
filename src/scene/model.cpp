/**
 * @file model.cpp
 * @brief Model class
 *
 * @author Eric Butler (edbutler)
 * @author Zeyang Li (zeyangl)
 */

#include "scene/model.hpp"
#include "scene/material.hpp"
#include "application/opengl.hpp"
#include "scene/triangle.hpp"
#include <iostream>
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>


namespace _462 {

Model::Model() : mesh( 0 ), material( 0 ) { }
Model::~Model() { }

void Model::render() const
{
    if ( !mesh )
        return;
    if ( material )
        material->set_gl_state();
    mesh->render();
    if ( material )
        material->reset_gl_state();
}
bool Model::initialize(){
    Geometry::initialize();
    return true;
}

// added by m.ji
bool Model::intersect(const Ray& r, real_t& t_out, Intersection& inter) {
   
    real_t tempT = 999999.;
    bool hit = false;
    const MeshTriangle* triangles = mesh->get_triangles(); 
    const MeshVertex* vertices = mesh->get_vertices(); 

    float u_temp, v_temp, w_temp;
    float v, w; // for barycentric coord calculation
    unsigned int hitTriangleIdx = 0;

    for (unsigned int i = 0; i < mesh->num_triangles(); ++i) {
        
        Vector3 vA = vertices[triangles[i].vertices[0]].position;
        Vector3 vB = vertices[triangles[i].vertices[1]].position;
        Vector3 vC = vertices[triangles[i].vertices[2]].position;

        Vector3 edgeAB = vB - vA;
        Vector3 edgeAC = vC - vA;
        Vector3 n = cross(edgeAB, edgeAC);

        /////////////////////
        // Finding point p //
        /////////////////////
        float nDotRay = dot(n, r.d);
        if (nDotRay == 0)
            continue;   // they are parallel

        float d = - dot(n, vA); // d of plane equation. TODO: this can be precomputed later
        float t = - (dot(n, r.e) + d) / nDotRay; // t of point p equation
        if (t < 0)
            continue;

        // calculate intersection point p with the plane
        Vector3 p = r.e + t * r.d;

        /////////////////////
        // Testing p is in //
        /////////////////////

        // test with edge 1 - inside or outside?
        Vector3 VtoP = p - vA;
        w = dot(n, cross(edgeAB, VtoP));    // preserve for barycentric coordinate
        if (w < 0)
            continue;   // p is outside of the triangle

        // test with edge 2
        Vector3 edge;
        edge = vC - vB;
        VtoP = p - vB;
        if (dot(n, cross(edge, VtoP)) < 0)
            continue;   // p is outside of the triangle

        // test with edge 3
        edge = vA - vC;
        VtoP = p - vC;
        v = dot(n, cross(edge, VtoP));    // perserve for barycentric coordinate
        if (v < 0)
            continue;   // p is outside of the triangle

        // ray hits this triangle. find closest one
        if (tempT > t) 
        {
            tempT = t;
            hit = true;
            
            // barycentric coord calculation.
            float n_len_sqr = dot (n,n);
            v_temp = v / n_len_sqr;
            w_temp = w / n_len_sqr;
            u_temp = 1 - v_temp - w_temp;
            
            // preserve closest triangle
            hitTriangleIdx = i;
            hitTriangle = triangles[i];
            inter.hitTriangle = triangles[i];
        }
    }

    if (hit) {
        t_out = tempT;
        inter.bary.x = u_temp;
        inter.bary.y = v_temp;
        inter.bary.z = w_temp;
        inter.hitTriangle = triangles[hitTriangleIdx];
        return true;
    }
    else
        return false;
}

void Model::getPositionInfo (Intersection& inter) {
    const MeshVertex* vertices = mesh->get_vertices(); 
    Vector3 normal = vertices[inter.hitTriangle.vertices[0]].normal * inter.bary.x +
                     vertices[inter.hitTriangle.vertices[1]].normal * inter.bary.y +
                     vertices[inter.hitTriangle.vertices[2]].normal * inter.bary.z;

    inter.normal = normalize(normMat * normal); //must normalize after applying normaMat if there was scaling
    inter.ambient = material->ambient;
    inter.nt = material->refractive_index;
    inter.specular = material->specular;
    inter.diffuse = material->diffuse;

    Vector2 tex_coord_ = 
        vertices[inter.hitTriangle.vertices[0]].tex_coord * inter.bary.x 
        + vertices[inter.hitTriangle.vertices[1]].tex_coord * inter.bary.y 
        + vertices[inter.hitTriangle.vertices[2]].tex_coord * inter.bary.z;
             
    inter.texture = material->texture.sample (tex_coord_);    
}

} /* _462 */
