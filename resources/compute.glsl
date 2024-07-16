#version 430 core
layout (local_size_x = 16, local_size_y = 16) in;

layout (rgba8, binding = 0) uniform image2D img_output;

uniform vec3 camera_position;
uniform mat4 projection_matrix;
uniform mat4 view_matrix;

uniform vec3 sphere_center;
uniform float sphere_radius;

float mandelbulbDistanceEstimator(vec3 p);
bool rayMarchMandelbulb(vec3 ray_origin, vec3 ray_direction, out float t, out vec3 p);
bool shadowMarchMandelbulb(vec3 p, vec3 light_dir);
vec3 calculateNormal(vec3 p);

void main() {
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);

    vec2 ndc_coords = (vec2(pixel_coords) + vec2(0.5)) / vec2(imageSize(img_output));
    vec4 clip_space_coords = vec4(ndc_coords * 2.0 - 1.0, -1.0, 1.0);
    vec4 view_space_coords = inverse(projection_matrix) * clip_space_coords;
    view_space_coords /= view_space_coords.w;
    vec3 ray_direction = normalize(mat3(view_matrix) * view_space_coords.xyz);

    float t;
    vec3 p;
    if (rayMarchMandelbulb(camera_position, ray_direction, t, p)) {
        vec3 normal = calculateNormal(p);

        // TODO: non static lighting
        vec3 light_dir = normalize(vec3(1.0, 1.0, 1.0));
        vec3 light_pos = vec3(2.0, 2.0, 2.0);
        float diffuse = max(dot(normal, light_dir), 0.0);

        // Check self shadow then apply shadow factor
        if (shadowMarchMandelbulb(p, light_dir)) { diffuse *= 0.2; }

        // Calculate color based on depth (t value) and diffuse shading
        vec4 color = vec4(t * vec3(1.0, 0.5, 0.2) * diffuse, 1.0);  // Example: depth-based coloring with shading

        imageStore(img_output, pixel_coords, color);
    }
    else {
        vec4 color = vec4(0.0, 0.0, 0.0, 1.0);  // Black color for no intersection
        imageStore(img_output, pixel_coords, color);
    }
}

float mandelbulbDistanceEstimator(vec3 p) {
    // mandelbulb constants
    vec3 z = p;
    float dr = 1.0;
    float r = 0.0;
    const int iterations = 10;
    const float power = 8.0;

    for (int i = 0; i < iterations; i++) {
        r = length(z);
        if (r > 2.0) break;

        // Convert to polar coordinates
        float theta = acos(z.z / r);
        float phi = atan(z.y, z.x);
        dr = pow(r, power - 1.0) * power * dr + 1.0;

        // Scale and rotate the point
        float zr = pow(r, power);
        theta = theta * power;
        phi = phi * power;

        // Convert back to cartesian coordinates
        z = zr * vec3(sin(theta) * cos(phi), sin(phi) * sin(theta), cos(theta));
        z += p;
    }

    return 0.5 * log(r) * r / dr;
}

vec3 calculateNormal(vec3 p) {
    float eps = 0.001;
    float d = mandelbulbDistanceEstimator(p);
    vec3 n = vec3(
        mandelbulbDistanceEstimator(p + vec3(eps, 0.0, 0.0)) - d,
        mandelbulbDistanceEstimator(p + vec3(0.0, eps, 0.0)) - d,
        mandelbulbDistanceEstimator(p + vec3(0.0, 0.0, eps)) - d
    );
    return normalize(n);
}

// out instead of passing by reference in glsl
bool rayMarchMandelbulb(vec3 ray_origin, vec3 ray_direction, out float t, out vec3 p) {
    float max_distance = 100.0;
    float min_distance = 0.002;
    t = 0.0;

    for (int i = 0; i < 100; i++) {
        p = ray_origin + t * ray_direction;
        float dist = mandelbulbDistanceEstimator(p);
        if (dist < min_distance) {
            return true;
        }
        t += dist;
        if (t > max_distance) {
            return false;
        }
    }

    return false;
}

bool shadowMarchMandelbulb(vec3 p, vec3 light_dir) {
    float t = 0.01;  // Start just above the surface to avoid self-intersection
    float max_distance = 100.0;
    const float min_distance = 0.002;

    for (int i = 0; i < 100; i++) {
        vec3 pos = p + t * light_dir;
        float dist = mandelbulbDistanceEstimator(pos);
        if (dist < min_distance) {
            return true;  // In shadow
        }
        t += dist;
        if (t > max_distance) {
            break;  // No shadow
        }
    }

    return false;  // No shadow
}