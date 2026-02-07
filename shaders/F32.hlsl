

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
  float3 normal;
  float2 textureCoord;
};

StructuredBuffer<Vertex> vertices;

struct VSOut
{
  float4 pos : SV_Position;
  float3 color : COLOR;
};

VSOut VSMain(uint vertexID: SV_VertexID)
{
  const float3 colors[] =
  {
    float3(0.0, 0.0, 0.0),
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0),
    float3(1.0, 1.0, 0.0),
    float3(1.0, 0.0, 1.0),
    float3(0.0, 1.0, 1.0),
    float3(1.0, 1.0, 1.0),
  };
  VSOut ret;
  ret.pos = mul(sceneData.mvp, float4(vertices[vertexID].pos, 1.0f));
  //ret.pos = float4(vertices[vertexID].pos, 1.0f);
  ret.color = colors[vertexID % 8];
  return ret;
}

float4 PSMain(VSOut vsOut) : SV_TARGET
{
  return float4(vsOut.color, 1.0);
}