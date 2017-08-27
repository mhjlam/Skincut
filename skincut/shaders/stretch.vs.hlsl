// stretch.vs.hlsl
// Vertex shader that computes texture stretching due to UV distortion.


#pragma pack_matrix(row_major)

struct VSIN
{
	float4 position  : POSITION0;
	float2 texcoord  : TEXCOORD0;
	float3 normal    : NORMAL0;
	float4 tangent   : TANGENT0;
};

struct VSOUT
{
	float4 position : SV_POSITION;
	float3 worldpos : TEXCOORD0;
};


cbuffer cbMatrix : register(b0)
{
	matrix World;
	matrix WorldInverse;
	matrix WorldViewProjection;
};


VSOUT main(VSIN input)
{
	VSOUT output;

	float1 x = input.texcoord.x * 2.0 - 1.0;
	float1 y = (1.0 - input.texcoord.y) * 2.0 - 1.0;

	output.position = float4(x, y, 0.0, 1.0);
	//output.position = mul(input.position, WorldViewProjection);
	output.worldpos = mul(input.position, World).xyz;

	return output;
}
