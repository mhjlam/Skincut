// phong.vs.hlsl
// Phong vertex shader.


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
	float2 texcoord : TEXCOORD0;
	float3 normal   : TEXCOORD1;
	float3 halfway  : TEXCOORD2;
};

cbuffer cb0 : register(b0)
{
	matrix World;
	matrix WorldIT;
	matrix WorldViewProjection;

	float4 ViewPosition;
	float4 LightDirection;
};


VSOUT main(VSIN input)
{
	VSOUT output;
	
	output.position = mul(input.position, WorldViewProjection);
	output.texcoord = input.texcoord;
	output.normal = mul(input.normal, (float3x3)WorldIT);
	output.halfway = -LightDirection.xyz + normalize(ViewPosition.xyz - input.position.xyz);
	
	return output;  
}
