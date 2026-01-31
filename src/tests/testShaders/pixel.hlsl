
struct PSIn
{
  float3 color : COLOR0;
};

float4 PSMain(PSIn input) : SV_Target
{
  return float4(input.color, 1.0f);
}
