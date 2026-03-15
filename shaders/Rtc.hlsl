struct SceneData
{
  float4x4 mvpRtc;
};

cbuffer Constants : register(b0)
{
  SceneData sceneData;
};


struct Vertex
{
  float3 pos: POSITION;
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

  ret.pos = mul(sceneData.mvpRtc, float4(vertex.pos, 1.0));
  ret.color = (vertex.normal + 1.0) / 2.0;

  return ret;
}

float4 PSMain(VSOut vsOut) : SV_TARGET
{
  return float4(vsOut.color, 1.0);
}
