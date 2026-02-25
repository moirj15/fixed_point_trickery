

struct SceneData
{
  float4x4 modelView;
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
StructuredBuffer<float4x4> transforms;

struct VSOut
{
  float4 pos : SV_Position;
  float3 color : COLOR;
  uint vid : COLOR1 ;
  float4x4 f : COLOR2;
};

VSOut VSMain(uint vertexID: SV_VertexID, uint transformIndex : SV_InstanceID)
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
  float4x4 mvp = mul(sceneData.modelView, transforms[transformIndex]);
  ret.pos = mul(mvp, float4(vertices[vertexID].pos, 1.0f));
  //ret.pos = float4(vertices[vertexID].pos, 1.0f);
  //ret.color = colors[vertexID % 8];
  ret.color = (vertices[vertexID].normal + 1.0) / 2.0;
  ret.vid = vertexID;
  ret.f = transforms[transformIndex];
  return ret;
}

float4 PSMain(VSOut vsOut) : SV_TARGET
{
  return float4(vsOut.color, 1.0);
}