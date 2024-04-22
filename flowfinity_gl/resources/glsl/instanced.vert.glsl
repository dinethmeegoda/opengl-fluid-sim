#version 150

uniform mat4 u_Model;
uniform mat4 u_ViewProj;
uniform mat3 u_ModelInvTr;
uniform vec3 u_CamPos;

uniform vec3 u_Offsets[1000];
uniform int u_NumInstances;
uniform int u_Time;
uniform float u_DeltaTime;

in vec4 vs_Pos;
in vec4 vs_Col;

out vec3 fs_Pos;
out vec4 fs_Col;

void main() {
  fs_Col = vs_Col;

  // vec4 pos = vec4(vs_Pos.xyz + vec3(gl_InstanceID * 0.25, 0, 0), 1.f);

  vec4 pos = vec4(vs_Pos.xyz + u_Offsets[gl_InstanceID], 1.f);
  
  vec4 modelposition = u_Model * pos;
  fs_Pos = modelposition.xyz;

  gl_Position = u_ViewProj * modelposition;
}