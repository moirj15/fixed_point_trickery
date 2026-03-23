struct Vertex
{
  float3 pos : POSITION;
  float2 texCoord : TEXCOORD;
};

struct VSOut
{
  float4 pos : SV_Position;
  float2 texCoord : TEXCOORD;
};

struct Scene 
{
  float4x4 mvp;
  float2 scale;
};

cbuffer Constants : register(b0)
{
  Scene scene;
};


VSOut VSMain(Vertex vertex)
{
  VSOut ret = (VSOut) 0;

  ret.pos = mul(scene.mvp, float4(vertex.pos, 1.0));
  //ret.pos = float4(vertex.pos, 1.0);
  ret.texCoord = vertex.texCoord;// * scene.scale;
  return ret;
}

Texture2D tex : register(t0);
SamplerState samp : register(s0);

float4 PSMain(VSOut vsOut) : SV_TARGET
{
  float4 texel = tex.Sample(samp, vsOut.texCoord);
  //return 1.0.xxxx;
  return float4(texel.rgb, 1.0);
}
