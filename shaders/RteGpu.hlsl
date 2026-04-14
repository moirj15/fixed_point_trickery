struct SceneData
{
  float4x4 mvp;
  float4 cameraPosHigh;
  float4 cameraPosLow;
  float4 worldHigh;
  float4 worldLow;
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
  float3 posHigh : POSITION_HIGH;
  float3 posLow : POSITION_LOW;
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
  VSOut ret = (VSOut) 0;

  float3 cameraPosHigh = sceneData.cameraPosHigh;
  float3 cameraPosLow = sceneData.cameraPosLow;

  float3 h = sceneData.worldHigh;
  float3 l = sceneData.worldLow;

  //float3 highDif = (h + vertex.posHigh) - cameraPosHigh;
  //float3 lowDif = (l + vertex.posLow) - cameraPosLow;

  float3 highDif = (h - cameraPosHigh) + vertex.posHigh;
  float3 lowDif = (l - cameraPosLow) + vertex.posLow;


  //float4x4 mvp = mul(sceneData.mvp, perMesh.transform);
  ret.pos = mul(sceneData.mvp, float4(highDif + lowDif, 1.0f));
  ret.color = (vertex.normal + 1.0) / 2.0;
  return ret;
}

float4 PSMain(VSOut vsOut) : SV_TARGET
{
  return float4(vsOut.color, 1.0);
  //return 1.0.xxxx;
}