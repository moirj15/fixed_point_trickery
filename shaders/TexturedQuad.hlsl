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

  //ret.pos = mul(scene.mvp, float4(vertex.pos, 1.0));
  ret.pos = float4(vertex.pos, 1.0);
  ret.texCoord = float2(vertex.texCoord.x, 1.0 - vertex.texCoord.y); // * scene.scale;
  return ret;
}

Texture2D tex : register(t0);
Texture2D depthTex : register(t1);
SamplerState samp : register(s0);

struct PSOut
{
  float4 color : SV_TARGET;
  float depth : SV_DEPTH;
};

PSOut PSMain(VSOut vsOut)
{
  PSOut psOut = (PSOut) 0;
  psOut.color = float4(tex.Sample(samp, vsOut.texCoord).rgb, 1.0);
  psOut.depth = depthTex.Sample(samp, vsOut.texCoord).r;
  //psOut.color = float4(psOut.depth.rrr, 1.0);
  //return 1.0.xxxx;
  return psOut;
}
