struct VSOut
{
  float4 pos : SV_Position;
  float3 color : COLOR0;
};

VSOut VSMain(uint vertexID : SV_VertexID)
{
	//const array of positions for the triangle
  const float3 positions[3] =
  {
    float3(1.f, 1.f, 0.0f),
		float3(-1.f, 1.f, 0.0f),
		float3(0.f, -1.f, 0.0f)
  };

	//const array of colors for the triangle
  const float3 colors[3] =
  {
    float3(1.0f, 0.0f, 0.0f), //red
		float3(0.0f, 1.0f, 0.0f), //green
		float3(0.f, 0.0f, 1.0f) //blue


  };
  VSOut vsOut;
  vsOut.pos = float4(positions[vertexID].xyz, 1.0f);
  vsOut.color = colors[vertexID];

  return vsOut;
}
