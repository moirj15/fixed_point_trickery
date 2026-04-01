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
  float3 color : COLOR;
};

VSOut VSMain(Vertex vertex)
{
  VSOut ret = (VSOut)0;

  //float4x4 mvp = mul(sceneData.mvp, perMesh.transform);
  float4x4 mvp = sceneData.mvp;
  ret.pos      = mul(mvp, float4(vertex.pos, 1.0f));
  ret.color = vertex.color;
  return ret;
}

float4 PSMain(VSOut vsOut) : SV_TARGET
{
  //return 0.0.xxxx;
  return float4(vsOut.color, 1.0);
}
