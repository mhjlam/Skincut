// main.vs.hlsl
// Main skin rendering vertex shader.


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
	float4 svposition : SV_POSITION; // ndc position
	float2 texcoord   : TEXCOORD0;
	float3 position   : TEXCOORD1; // world position
	float3 viewdir    : TEXCOORD2; // view direction
	float3 normal     : TEXCOORD3;
	float4 tangent    : TEXCOORD4;
};


cbuffer cbGlobals : register(b0)
{
	matrix WVP;
	matrix World;
	matrix WorldIT;
	float3 Eye; // camera location
};


VSOUT main(VSIN input)
{	
	VSOUT output;
	output.svposition = mul(input.position, WVP); // position in normalized device coordinates
	output.texcoord = input.texcoord;	
	output.position = mul(input.position, World).xyz; // position in world coordinates
	output.viewdir = normalize(Eye - output.position); // direction from surface to camera
	output.normal = mul(input.normal, (float3x3)WorldIT);
	output.tangent.xyz = mul(input.tangent.xyz, (float3x3)WorldIT);
	output.tangent.w = input.tangent.w;
	return output;
}

