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
  float4 pos;
  float4 cameraRight;
  float4 cameraUp;
  float2 scale;
};

cbuffer Constants : register(b0)
{
  Scene scene;
};


VSOut VSMain(Vertex vertex)
{
  VSOut ret = (VSOut) 0;

  #if 0
  //ret.pos = mul(scene.mvp, float4(vertex.pos, 1.0));
  ret.pos = float4(vertex.pos, 1.0);
#endif
  //float3 position = mul(scene.mvp, float4(0.0, 0.0, 0.0, 1.0)).xyz;
  float2 scale = float2(2.0, 2.0);
  //ret.pos = float4(scene.pos.xyz 
  ret.pos = mul(scene.mvp, float4(scene.pos.xyz
    + scene.cameraRight.xyz * vertex.pos.x * scale.x
    + scene.cameraUp.xyz * vertex.pos.y * scale.y, 1.0));

  float s = 0.7;
  ret.texCoord = vertex.texCoord * float2(s, s);
  
  
  return ret;
}

Texture2D tex : register(t0);
Texture2D depthTex : register(t1);
SamplerState samp : register(s0);

cbuffer PSConstants : register(b1)
{
  float4x4 inverseVP;
  float4x4 VP;
};

struct PSOut
{
  float4 color : SV_TARGET;
  float depth : SV_DEPTH;
};

PSOut PSMain(VSOut vsOut)
{
  PSOut psOut = (PSOut) 0;
  psOut.color = float4(tex.Sample(samp, vsOut.texCoord).rgb, 1.0);
  //psOut.color = 1.0.xxxx;

  psOut.depth = depthTex.Sample(samp, vsOut.texCoord).r;

  //psOut.depth = 0.5;
  //psOut.depth *= vsOut.pos.z;
  //psOut.color.a = vsOut.pos.z;
  //psOut.color = float4(psOut.depth.rrr, 1.0);
  //return 1.0.xxxx;
  return psOut;
}
