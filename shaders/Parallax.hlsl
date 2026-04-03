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
  float3 normal : NORMAL;
  float2 textureCoord : TEXCOORD;
};

struct VSOut
{
  float4 pos : SV_Position;
  float3 color : COLOR;
};

VSOut VSMain(Vertex vertex)
{
  VSOut ret = (VSOut)0;

  float4x4 mvp = mul(sceneData.mvp, perMesh.transform);
  ret.pos      = mul(mvp, float4(vertex.pos, 1.0f));
  ret.color = (vertex.normal + 1.0) / 2.0;
  return ret;
}

float4 PSMain(VSOut vsOut) : SV_TARGET
{
  return float4(vsOut.color, 1.0);
  //return 1.0.xxxx;
}
