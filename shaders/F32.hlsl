

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

struct PerMesh
{
  float4x4 transform;
  uint vertexOffset;
};

StructuredBuffer<PerMesh> perMeshData;

struct VSOut
{
  float4 pos : SV_Position;
  float3 color : COLOR;
  uint vid : COLOR1 ;
  uint f : COLOR2;
};

VSOut VSMain(uint vertexID: SV_VertexID, uint meshID: SV_InstanceID)
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

  PerMesh perMesh = perMeshData[meshID];
  uint index = vertexID + perMesh.vertexOffset;
  //uint index = vertexID + 838;
  float4x4 mvp = mul(sceneData.modelView, perMesh.transform);
  ret.pos = mul(mvp, float4(vertices[index].pos, 1.0f));
  //ret.pos = float4(vertices[vertexID].pos, 1.0f);
  //ret.color = colors[vertexID % 8];
  //ret.color = (vertices[index].normal + 1.0) / 2.0;
  ret.color = colors[meshID];
  ret.vid = index;
  ret.f = meshID;
  return ret;
}

float4 PSMain(VSOut vsOut) : SV_TARGET
{
  return float4(vsOut.color, 1.0);
}