#version 430

uniform mat4 u_Model;
uniform mat4 u_ViewProj;
uniform mat3 u_ModelInvTr;
uniform vec3 u_CamPos;

uniform int u_NumInstances;
uniform int u_Time;
uniform float u_DeltaTime;
uniform float u_MaxVelocity;
uniform vec3[6] u_Colors;

in vec4 vs_Pos;
in vec4 vs_Col;

layout(std430, binding = 0) buffer PositionBuffer {
    float positions[];
};

layout(std430, binding = 1) buffer VelocityBuffer {
    float velocities[];
};

out vec3 fs_Pos;
out vec4 fs_Col;

float saturate(float value) {
    return clamp(value, 0.0, 1.0);
}

void main() {
  // Mix the color based on the velocity of the instance
  vec3 velocity = vec3(abs(velocities[3 * gl_InstanceID]), abs(velocities[3 * gl_InstanceID + 1]), abs(velocities[3 * gl_InstanceID + 2]));
  float speed = length(velocity);
  float normalizedSpeed = saturate(speed / u_MaxVelocity);

  // Determine mix factor such that we have four equal intervals for five colors
  float interval = 1.0 / 5.0;

  // Mix colors based on the speed
  vec3 mixedColor;
  if (normalizedSpeed < interval) {
      // Mix between color1 and color2
      mixedColor = mix(u_Colors[0], u_Colors[1], normalizedSpeed / interval);
  } else if (normalizedSpeed < 2.0 * interval) {
      // Mix between color2 and color3
      mixedColor = mix(u_Colors[1], u_Colors[2], normalizedSpeed / (interval * 2.0));
  } else if (normalizedSpeed < 3.0 * interval) {
      // Mix between color3 and color4
      mixedColor = mix(u_Colors[2], u_Colors[3], (normalizedSpeed / (interval * 3.0)));
  } else if (normalizedSpeed < 4.0 * interval) {
      // Mix between color4 and color5
      mixedColor = mix(u_Colors[3], u_Colors[4], (normalizedSpeed / (interval * 4.0)));
  } else {
      // Mix between color5 and color6
      mixedColor = mix(u_Colors[4], u_Colors[5], (normalizedSpeed / (interval * 5.0)));
  }

  // Set the fragment's color
  fs_Col = vec4(mixedColor, 1.0);

  // Adjust vertex position with the offset for this instance
  vec4 pos = vec4(vs_Pos.xyz + vec3(positions[3 * gl_InstanceID], positions[3 * gl_InstanceID + 1], positions[3 * gl_InstanceID + 2]), 1.0);
  
  vec4 modelposition = u_Model * pos;
  fs_Pos = modelposition.xyz;

  gl_Position = u_ViewProj * modelposition;
}