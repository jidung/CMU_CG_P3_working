/**
 * @file raytacer.cpp
 * @brief Raytracer class
 *
 * Implement these functions for project 4.
 *
 * @author H. Q. Bovik (hqbovik)
 * @bug Unimplemented
 */



#include "raytracer.hpp"
#include "scene/scene.hpp"
#include "math/quickselect.hpp"
#include "p3/randomgeo.hpp"
#include <SDL_timer.h>

// added by m.ji
#include <typeinfo>
#include "scene/triangle.hpp"
#include "scene/sphere.hpp"
#include "scene/model.hpp"
namespace _462 {


//number of rows to render before updating the result
static const unsigned STEP_SIZE = 1;
static const unsigned CHUNK_SIZE = 1;

Raytracer::Raytracer() {
        scene = 0;
        width = 0;
        height = 0;
    }

Raytracer::~Raytracer() { }

/**
 * Initializes the raytracer for the given scene. Overrides any previous
 * initializations. May be invoked before a previous raytrace completes.
 * @param scene The scene to raytrace.
 * @param width The width of the image being raytraced.
 * @param height The height of the image being raytraced.
 * @return true on success, false on error. The raytrace will abort if
 *  false is returned.
 */
bool Raytracer::initialize(Scene* scene, size_t num_samples,
        size_t width, size_t height)
{
    this->scene = scene;
    this->num_samples = num_samples;

    this->num_samples = 5; // temp code. jd
    this->width = width;
    this->height = height;

    current_row = 0;

    projector.init(scene->camera);
    scene->initialize();
    photonMap.initialize(scene);
    return true;
}
//compute ambient lighting
Color3 Raytracer::trace_ray(Ray &ray, const unsigned int &depth/*maybe some more arguments*/){

    /* things to revisit:
       Slop Factor
       Sphere texture uv
     */

    Color3 color (0,0,0);
    Intersection intersection;

    if (depth < MAX_DEPTH) {
        Ray newRay;
        real_t interTime = 99999.;   // will get intersection time
        real_t shadowInterTime = 99999.;   // will get intersection time
        real_t minInterTime = 999999.;   // will get intersection time
        bool hit = false;
        Geometry* const* geometries = scene->get_geometries();
        const SphereLight* lights = scene->get_lights();

        Vector3 normal;
        uint hitGeometryIdx;

        Color3 reflectionColor (0,0,0);
        Color3 refractionColor (0,0,0);

        Vector3 hitPos (0,0,0);

        for (unsigned int i = 0; i < scene->num_geometries(); ++i) {
            newRay = ray.translate (geometries[i]->invMat);

            // ray hit object in the object's local space
            if ( geometries[i]->intersect (newRay, interTime) ) {
                if (minInterTime > interTime) {
                    hit = true;
                    minInterTime = interTime;
                    hitGeometryIdx = i;
                }
            }
        }

        if (hit) {
            intersection.hitPos = ray.atTime(minInterTime);
            intersection.normal = geometries[hitGeometryIdx]->getNormal (intersection.hitPos); 
            //intersection.normal = normalize(intersection.normal); 

            real_t n = scene->refractive_index;
            real_t nt = geometries[hitGeometryIdx]->getRefractionIdx();
            
            // should be before shadow rays because intersec() will change member variables of geometries that used for barycentric coord interpolation
            Color3 ambient = geometries[hitGeometryIdx]->getAmbient() * scene->ambient_light;
            Color3 specular = geometries[hitGeometryIdx]->getSpecular();
            Color3 texture = geometries[hitGeometryIdx]->getTexColor();
            Color3 diffuseKd = geometries[hitGeometryIdx]->getDiffuse();
            Color3 diffuse(0, 0, 0);

            // diffuse object?
            if ( specular.isBlack() ) {
              
                // Shadow rays
                for (unsigned int i = 0; i < scene->num_lights(); ++i) {
                    Ray rayToLight;

                    real_t light_x, light_y, light_z;
                    light_x = random_gaussian();
                    light_y = random_gaussian();
                    light_z = random_gaussian();

                    Vector3 light_position = 
                        ( 1.0 / sqrt ( light_x * light_x 
                                     + light_y * light_y 
                                     + light_z * light_z ) ) 
                        * Vector3 (light_x, light_y, light_z);

                    light_position *= lights[i].radius;
                    light_position += lights[i].position;
                    
                    rayToLight.e = intersection.hitPos + intersection.normal * EPS;
                    rayToLight.d = light_position - rayToLight.e;

                    real_t distanceToLight = length(rayToLight.d);
                    rayToLight.d = normalize(rayToLight.d);

                    for (unsigned int j = 0; j < scene->num_geometries(); ++j) {
                        newRay = rayToLight.translate (geometries[j]->invMat);

                        // ray hit object in the object's local space
                        if ( geometries[j]->intersect (newRay, shadowInterTime) ) {
                            //std::cout << timeToRay << std::endl;
                            if (shadowInterTime < distanceToLight)
                                return Color3::Black();
                                //color = Color3::Black();
                        }
                    }
                    
                    //this shadow ray doesn't hit any objects
                    Color3 ci = lights[i].color 
                        * ( 1.0 / (lights[i].attenuation.constant           
                           + distanceToLight*lights[i].attenuation.linear 
                           + distanceToLight*distanceToLight*lights[i].attenuation.quadratic) ); 
                    diffuse += ci * diffuseKd * std::max( dot(intersection.normal, rayToLight.d), (real_t)0 );
                }// end of lights loop
            
                color = texture * (ambient + diffuse);
            }// end of diffuse objects
            
            Color3 k;                       // attenuation. not used
            Color3 a (0.0, 0.0, 0.0);       // attenuation. not used
            float fresnel;

            // reflection and / or refraction
            if ( !specular.isBlack() )   
            { // should be fixed

                Ray reflectRay;
                reflectRay = ray.reflect (intersection.hitPos + intersection.normal * EPS, intersection.normal);
                reflectionColor = trace_ray (reflectRay, depth + 1) * specular;

                Ray refractRay;

                // ray is entering
                if ( dot (intersection.normal, ray.d) < 0) {

                    Intersection temp = intersection;

                    temp.hitPos -= intersection.normal * EPS;

                    if (nt <= 0)
                        return reflectionColor;

                    refractRay.e = temp.hitPos;
                    refractRay.d = refract (intersection.normal, ray.d, n / nt);
                    //std::cout << refractRay.d << std::endl;
                    
                    k = Color3::White();

                    fresnel = computeFresnelCoefficient (temp, ray, n, nt);

                    refractionColor = trace_ray (refractRay, depth + 1);
                    //std::cout << fresnel << std::endl;
                } 
                else // ray is exiting
                {
                    k.r = exp(-a.r * minInterTime);
                    k.g = exp(-a.g * minInterTime);
                    k.b = exp(-a.b * minInterTime);

                    Intersection temp = intersection;

                    temp.hitPos += intersection.normal * EPS;

                    if (nt <= 0)
                        return reflectionColor;

                    refractRay.e = temp.hitPos;
                    refractRay.d = refract (-intersection.normal, ray.d, nt / n);

                    Ray reverseRefractRay;
                    reverseRefractRay.e = refractRay.e;
                    reverseRefractRay.d = -refractRay.d;

                    if (refractRay.d.x == refractRay.d.x) {
                        fresnel = computeFresnelCoefficient (intersection, reverseRefractRay, n, nt);
                        //std::cout << "n: " << n << " nt: " << nt << " fresnel: " << fresnel << std::endl;
                    } else  { // total internal reflection
                        //std::cout << "Total Internal Reflection" << std::endl;
                        fresnel = 1.0f;
                    }
                    refractionColor = trace_ray (refractRay, depth + 1);
                }
            
                color = (fresnel * reflectionColor) + ((1.0 - fresnel) * refractionColor);
            }// end of dielectric
            //color = color + (reflectionColor + refractionColor) * k;
            //color = color + (reflectionColor);
        } else // not hit
            color = scene->background_color;

    } 
    
// for debugging. jd
//#define NORMAL_SHADING
#ifdef NORMAL_SHADING            
    color.r = fabs(intersection.normal.x);
    color.g = fabs(intersection.normal.y);
    color.b = fabs(intersection.normal.z);
#endif

    //return color + scene->ambient_light;
    return color;

    // original code
    //TODO: render something more interesting
    // return Color3(fabs(sin(10*ray.d.x)),fabs(10*cos(ray.d.y)),fabs(10*tan(ray.d.y)));
}

/**
 * Performs a raytrace on the given pixel on the current scene.
 * The pixel is relative to the bottom-left corner of the image.
 * @param scene The scene to trace.
 * @param x The x-coordinate of the pixel to trace.
 * @param y The y-coordinate of the pixel to trace.
 * @param width The width of the screen in pixels.
 * @param height The height of the screen in pixels.
 * @return The color of that pixel in the final image.
 */
Color3 Raytracer::trace_pixel(size_t x,
                  size_t y,
                  size_t width,
                  size_t height)
{
    assert(x < width);
    assert(y < height);

    real_t dx = real_t(1)/width;
    real_t dy = real_t(1)/height;

    Color3 res = Color3::Black();
    unsigned int iter;

    unsigned int depth = 0;

    for (iter = 0; iter < num_samples; iter++)
    {
        // pick a point within the pixel boundaries to fire our
        // ray through.
        real_t i = real_t(2)*(real_t(x)+random_uniform())*dx - real_t(1);
        real_t j = real_t(2)*(real_t(y) + random_uniform())*dy - real_t(1);

        Ray r = Ray(scene->camera.get_position(), projector.get_pixel_dir(i, j));
    
        res += trace_ray(r, depth);
        // TODO return the color of the given pixel
        // you don't have to use this stub function if you prefer to
        // write your own version of Raytracer::raytrace.

    }
    return res*(real_t(1)/num_samples);
}

/**
 * Raytraces some portion of the scene. Should raytrace for about
 * ax_time duration and then return, even if the raytrace is not copmlete.
 * The results should be placed in the given buffer.
 * @param buffer The buffer into which to place the color data. It is
 *  32-bit RGBA (4 bytes per pixel), in row-major order.
 * @param max_time, If non-null, the maximum suggested time this
 *  function raytrace before returning, in seconds. If null, the raytrace
 *  should run to completion.
 * @return true if the raytrace is complete, false if there is more
 *  work to be done.
 */
bool Raytracer::raytrace(unsigned char* buffer, real_t* max_time)
{
    
    static const size_t PRINT_INTERVAL = 64;

    // the time in milliseconds that we should stop
    unsigned int end_time = 0;
    bool is_done;

    if (max_time)
    {
        // convert duration to milliseconds
        unsigned int duration = (unsigned int) (*max_time * 1000);
        end_time = SDL_GetTicks() + duration;
    }

    // until time is up, run the raytrace. we render an entire group of
    // rows at once for simplicity and efficiency.
    for (; !max_time || end_time > SDL_GetTicks(); current_row += STEP_SIZE)
    {
        // we're done if we finish the last row
        is_done = current_row >= height;
        // break if we finish
        if (is_done) break;

        int loop_upper = std::min(current_row + STEP_SIZE, height);

        for (int c_row = current_row; c_row < loop_upper; c_row++)
        {
            /*
             * This defines a critical region of code that should be
             * executed sequentially.
             */
#pragma omp critical
            {
                if (c_row % PRINT_INTERVAL == 0)
                    printf("Raytracing (Row %d)\n", c_row);
            }
            
        // This tells OpenMP that this loop can be parallelized.
#pragma omp parallel for schedule(dynamic, CHUNK_SIZE)
            for (size_t x = 0; x < width; x++)
            {
                // trace a pixel
                Color3 color = trace_pixel(x, c_row, width, height);
                // write the result to the buffer, always use 1.0 as the alpha
                color.to_array4(&buffer[4 * (c_row * width + x)]);
            }
#pragma omp barrier

        }
    }

    if (is_done) printf("Done raytracing!\n");

    return is_done;
}

} /* _462 */
