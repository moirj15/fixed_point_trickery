struct Vertex
{
  float4 pos : POSITION;
  vec2 texCoord : TEXCOORD;
};

struct VSOut
{
  float4 pos : SV_Position;
  flaot2 texCoord : TEXCOORD;
};


VSOut VSMain(Vertex vertex)
{
  VSOut ret = (VSOut) 0;

  ret.pos = vertex.pos;
  return ret;
}

Texture2D tex : register(t0);
SamplerState sampler : register(s0);

float4 PSMain(VSOut vsOut) : SV_TARGET
{
  float4 texel = tex.Sample(sampler, vsOut.  texCoord);
  return float4(texel.rgb, 1.0);
}
