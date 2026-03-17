struct Vertex
{
  float4 pos : POSITION;
};

struct VSOut
{
  float4 pos : SV_Position;
};

VSOut VSMain(Vertex vertex)
{
  VSOut ret = (VSOut)0;

  ret.pos      = vertex.pos;
  return ret;
}

float4 PSMain(VSOut vsOut) : SV_TARGET
{
  return float4(0.0.xxx, 1.0);
}
