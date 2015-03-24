 /*
 * @file model.hpp
 * @brief Model class
 *
 * @author Eric Butler (edbutler)
 */

#ifndef _462_SCENE_MODEL_HPP_
#define _462_SCENE_MODEL_HPP_

#include "scene/scene.hpp"
#include "scene/mesh.hpp"
#include "scene/meshtree.hpp"

namespace _462 {

/**
 * A mesh of triangles.
 */
class Model : public Geometry
{
public:

    const Mesh* mesh;
    const MeshTree *tree;
    const Material* material;

    Model();
    virtual ~Model();

    virtual void render() const;
    virtual bool initialize();

    bool intersect(const Ray& r, real_t& t); // overrided from Geometry. m.ji
    Vector3 getNormal(const Vector3& hitPos);  // these are overrided from Geometry class. m.ji   
    Color3 getSpecular();     
    real_t getRefractionIdx();    
    Color3 getAmbient();
    Color3 getDiffuse();
    Color3 getTexColor();
    
    float u, v, w; // for barycentric coord calculation
    MeshTriangle hitTriangle;
};


} /* _462 */

#endif /* _462_SCENE_MODEL_HPP_ */

