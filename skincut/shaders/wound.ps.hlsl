// wound.ps.hlsl
// Pixel shader for wound patch painting.


#pragma pack_matrix(row_major)

cbuffer cb_wound : register(b0)
{
	float2 P0;
	float2 P1;

	float1 Offset;
	float1 CutLength;
	float1 CutHeight;
};

Texture2D Texture : register(t0);
SamplerState LinearSampler : register(s0);


float4 main(float4 position : SV_POSITION, float2 texcoord : TEXCOORD) : SV_TARGET
{
	// parallel and orthogonal vectors to cutline segment
	float2 vx = normalize(P1 - P0);
	float2 vy = float2(-vx.y, vx.x);

	// direction vector from origin to texcoord
	float2 vt = (texcoord - P0);

	// x and y components of the direction vector
	float1 dx = dot(vt, vx);
	float1 dy = dot(vt, vy);

	float2 sample = float2(0, 0);
	sample.x = (Offset + dx) / CutLength;
	sample.y = 0.5 - (dy / CutHeight);

	return Texture.Sample(LinearSampler, sample);
}
