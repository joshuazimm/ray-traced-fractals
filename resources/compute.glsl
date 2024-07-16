#version 430 core
layout (local_size_x = 16, local_size_y = 16) in;

layout (rgba8, binding = 0) uniform image2D img_output;

void main() {
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
    vec4 color = vec4(1.0, 0.0, 0.0, 1.0); // Red color
    imageStore(img_output, pixel_coords, color);
}