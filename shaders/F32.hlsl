

struct SceneData
{
  float4x4 mvp;
  float3 color;
};

cbuffer Constants
{
  SceneData sceneData;
};

struct Vertex
{
  float3 pos;
};

StructuredBuffer<Vertex> vertices;

struct VSOut
{
  float4 pos : SV_Position;
  nointerpolation float3 color : COLOR;
};

VSOut VSMain(uint vertexID: SV_VertexID)
{
  VSOut ret;
  ret.pos = mul(sceneData.mvp, float4(vertices[vertexID].pos, 1.0f));
  ret.color = sceneData.color;
  return ret;
}

float4 PSMain(float3 color : COLOR) : SV_TARGET
{
  return float4(color, 1.0);

}