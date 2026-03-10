struct SceneData
{
  double4x4 mvp;
};

cbuffer Constants : register(b0)
{
  SceneData sceneData;
};

struct PerMesh
{
  double4x4 transform;
};

cbuffer Constants : register(b1)
{
  PerMesh perMesh;
};

struct Vertex
{
  uint3 posLow : POSITION_LOW;
  uint3 posHigh: POSITION_HIGH;
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

  double3 pos = { 
    asdouble(vertex.posLow.x, vertex.posHigh.x),
    asdouble(vertex.posLow.y, vertex.posHigh.y),
    asdouble(vertex.posLow.z, vertex.posHigh.z),
  };

  double4x4 mvp = mul(sceneData.mvp, perMesh.transform);
  ret.pos = (float4) mul(mvp, double4(pos, 1.0));
  ret.color = (vertex.normal + 1.0) / 2.0;
  return ret;
}

float4 PSMain(VSOut vsOut) : SV_TARGET
{
  return float4(vsOut.color, 1.0);
}
