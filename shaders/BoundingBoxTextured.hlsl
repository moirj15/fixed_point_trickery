struct SceneData
{
  float4x4 mvp;
};

cbuffer Constants : register(b0)
{
  SceneData sceneData;
};

struct PerMesh
{
  float4x4 transform;
};

cbuffer Constants : register(b1)
{
  PerMesh perMesh;
};

struct Vertex
{
  float3 pos : POSITION;
  float3 color: COLOR;
};

struct VSOut
{
  float4 pos : SV_Position;
  float4 clipSpaceP : COLOR0;
  float3 color : COLOR1;
};

VSOut VSMain(Vertex vertex)
{
  VSOut ret = (VSOut)0;

  float4x4 mvp = mul(sceneData.mvp, perMesh.transform);
  ret.pos      = mul(mvp, float4(vertex.pos, 1.0f));
  ret.clipSpaceP= mul(mvp, float4(vertex.pos, 1.0f));
  ret.color = vertex.color;
  return ret;
}

Texture2D tex : register(t0);
Texture2D depthTex : register(t1);
SamplerState samp : register(s0);

float4 PSMain(VSOut vsOut) : SV_TARGET
{
  float2 uv = vsOut.clipSpaceP.xy / vsOut.clipSpaceP.w;
  uv = (uv + 1.0) / 2.0;
  uv.y = 1.0 - uv.y;
  //return 0.0.xxxx;
  //return float4(vsOut.color, 1.0);
  //return float4(uv, 0.0, 1.0);
  return tex.Sample(samp, uv);
}
