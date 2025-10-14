struct Vertex
{
  float3 pos;
  float3 normal;
};

struct Constants
{
  float4x4 mvp;
};

cbuffer Constants0
{
  Constants constants;
};

StructuredBuffer<Vertex> vertices;

struct VSOut
{
  float4 pos : SV_Position;
  float3 color : COLOR;
};

VSOut VSMain(uint vertexID : SV_VertexID)
{
  VSOut ret;
  ret.pos = mul(constants.mvp, float4(vertices[vertexID].pos, 1.0f));
  ret.color = abs(vertices[vertexID].normal);
  return ret;
}

struct PSIn
{
  float3 color : COLOR;
};

float4 PSMain(VSOut input) : SV_Target
{
    //return 1.0.xxxx;
  return float4(input.color, 1.0f);
}
