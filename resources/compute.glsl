#version 430 core
layout (local_size_x = 16, local_size_y = 16) in;

layout (rgba8, binding = 0) uniform image2D img_output;

uniform vec3 camera_position;
uniform mat4 projection_matrix;
uniform mat4 view_matrix;

float mandelbulbDistanceEstimator(vec3 p);
float juliaDistanceEstimator(vec3 p);
float sierpinskiDistanceEstimator(vec3 p);
bool rayMarchMandelbulb(vec3 ray_origin, vec3 ray_direction, out float t, out vec3 p, out int iterationCount);
bool shadowMarchMandelbulb(vec3 p, vec3 light_dir);
vec3 calculateNormal(vec3 p);
vec3 getIterationColor(int iteration, int maxIteration);
vec3 blinnPhongShading(vec3 normal, vec3 lightDir, vec3 viewDir, vec3 lightColor, vec3 baseColor);

void main() {
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);

    vec2 ndc_coords = (vec2(pixel_coords) + vec2(0.5)) / vec2(imageSize(img_output));
    vec4 clip_space_coords = vec4(ndc_coords * 2.0 - 1.0, -1.0, 1.0);
    vec4 view_space_coords = inverse(projection_matrix) * clip_space_coords;
    view_space_coords /= view_space_coords.w;
    vec3 ray_direction = normalize((inverse(view_matrix) * vec4(view_space_coords.xyz, 0.0)).xyz);

    float t;
    vec3 p;
    int iterationCount;
    if (rayMarchMandelbulb(camera_position, ray_direction, t, p, iterationCount)) {
        vec3 normal = calculateNormal(p);

        vec3 light_dir = normalize(vec3(1.0, 1.0, 1.0));
        vec3 light_color = vec3(1.0);
        vec3 view_dir = normalize(camera_position - p);
        vec3 base_color = getIterationColor(iterationCount, 100);

        vec3 color = blinnPhongShading(normal, light_dir, view_dir, light_color, base_color);

        // Check self shadow and apply shadow factor
        if (shadowMarchMandelbulb(p, light_dir)) { color *= 0.2; }

        imageStore(img_output, pixel_coords, vec4(color, 1.0));
    }
    else {
        vec4 color = vec4(0.0, 0.0, 0.0, 1.0);  // Black color for no intersection
        imageStore(img_output, pixel_coords, color);
    }
}

float mandelbulbDistanceEstimator(vec3 p) {
    vec3 z = p;
    float dr = 1.0;
    float r = 0.0;
    const int iterations = 30;
    const float power = 30.0;

    for (int i = 0; i < iterations; i++) {
        r = length(z);
        if (r > 2.0) break;

        float theta = acos(z.z / r);
        float phi = atan(z.y, z.x);
        dr = pow(r, power - 1.0) * power * dr + 1.0;

        float zr = pow(r, power);
        theta = theta * power;
        phi = phi * power;

        z = zr * vec3(sin(theta) * cos(phi), sin(phi) * sin(theta), cos(theta));
        z += p;
    }

    return 0.5 * log(r) * r / dr;
}

float juliaDistanceEstimator(vec3 p) {
    vec4 z = vec4(p, 0.0);
    vec4 c = vec4(-0.8, 0.156, 0.0, 0.0); // This is a parameter that can be changed to get different Julia sets
    float dr = 1.0;
    float r = 0.0;

    const int iterations = 100;
    for (int i = 0; i < iterations; i++) {
        r = length(z);
        if (r > 2.0) break;

        // Quaternion multiplication z = z^2 + c
        float zr = z.x * z.x - z.y * z.y - z.z * z.z - z.w * z.w;
        float zi = 2.0 * z.x * z.y;
        float zj = 2.0 * z.x * z.z;
        float zk = 2.0 * z.x * z.w;
        z = vec4(zr, zi, zj, zk) + c;

        dr = 2.0 * r * dr + 1.0;
    }
    return 0.5 * log(r) * r / dr;
}

float sierpinskiDistanceEstimator(vec3 p) {
    const int iterations = 50;
    const float scale = 2.0;
    const vec3 scaleVec = vec3(scale);

    for (int i = 0; i < iterations; i++) {
        if (p.x + p.y < 0.0) p.xy = -p.yx;
        if (p.x + p.z < 0.0) p.xz = -p.zx;
        if (p.y + p.z < 0.0) p.yz = -p.zy;
        p = p * scaleVec - vec3(scale - 1.0);
    }

    return length(p) * pow(scale, -float(iterations));
}

vec3 calculateNormal(vec3 p) {
    float eps = 0.001;
    float d = sierpinskiDistanceEstimator(p);
    vec3 n = vec3(
        sierpinskiDistanceEstimator(p + vec3(eps, 0.0, 0.0)) - d,
        sierpinskiDistanceEstimator(p + vec3(0.0, eps, 0.0)) - d,
        sierpinskiDistanceEstimator(p + vec3(0.0, 0.0, eps)) - d
    );
    return normalize(n);
}

bool rayMarchMandelbulb(vec3 ray_origin, vec3 ray_direction, out float t, out vec3 p, out int iterationCount) {
    float max_distance = 100.0;
    float min_distance = 0.00001;
    t = 0.0;
    iterationCount = 0;

    for (int i = 0; i < 100; i++) {
        p = ray_origin + t * ray_direction;
        float dist = sierpinskiDistanceEstimator(p);
        if (dist < min_distance) {
            iterationCount = i;
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
    float t = 0.005;  // Start just above the surface to avoid self-intersection
    float max_distance = 100.0;
    const float min_distance = 0.00001;

    for (int i = 0; i < 100; i++) {
        vec3 pos = p + t * light_dir;
        float dist = sierpinskiDistanceEstimator(pos);
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

vec3 getIterationColor(int iteration, int maxIteration) {
    //float t = float(iteration) / float(maxIteration);
    //float colorIndex = smoothstep(0.0, 1.0, t);
    //return vec3(sin(2.0 * 3.14159 * colorIndex), sin(2.0 * 3.14159 * (colorIndex + 0.33)), sin(2.0 * 3.14159 * (colorIndex + 0.67)));
    return vec3(1.0, 0.5, 0.2);
}

vec3 blinnPhongShading(vec3 normal, vec3 lightDir, vec3 viewDir, vec3 lightColor, vec3 baseColor) {
    vec3 ambient = 0.1 * baseColor;
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * baseColor;

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = spec * lightColor;

    return ambient + diffuse + specular;
}