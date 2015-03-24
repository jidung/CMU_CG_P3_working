#include "p3/util.hpp"
namespace _462{


//ensures that the light has a maximum intensity of 1, and in order
//to cancel this scaling factor, returns the probability of continuing.
real_t montecarlo(Color3& light){
    real_t factor=std::max(light.r,std::max(light.g,light.b));
    light*=1/factor;
    return factor;
}

real_t computeFresnelCoefficient(Intersection &next, Ray &ray, real_t index, real_t newIndex) {

    // Schlick approximation
    real_t cos_theta = dot (-ray.d, next.normal);
    real_t r0 = ( (newIndex - index)*(newIndex - index) ) / ( (newIndex + index)*(newIndex + index) );
    real_t r = r0 + (1.0 - r0) * pow ( (1.0 - cos_theta), 5.0 );

    return r;
}

Vector3 refract(Vector3 norm, Vector3 inc, real_t ratio) {

    Vector3 refractionRay;
    float dotProduct = dot (inc, norm);

    refractionRay = ratio * (inc - norm * dotProduct) 
                    - norm * sqrt( 1.0 - ratio * ratio * (1.0 - dotProduct*dotProduct ));
   
    return refractionRay;
}

}
