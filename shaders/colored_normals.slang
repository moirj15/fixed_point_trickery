struct Vertex {
    float3 pos;
    float3 normal;
}

struct Constants {
    float4x4 mvp;
}

ConstantBuffer<Constants> constants;

StructuredBuffer<Vertex> vertices;

struct VSOut {
    float4 pos : SV_Position;
    float3 color : COLOR0;
}

[shader("vertex")]
VSOut VSMain(uint vertexID: SV_VertexID) 
{
    return {mul(constants.mvp, float4(vertices[vertexID].pos, 1.0f)), vertices[vertexID].normal};
}

struct PSIn {
    float3 color : COLOR0;
}

[shader("pixel")]
float4 PSMain(PSIn input) : SV_Target
{
	return float4(input.color, 1.0f);
}
