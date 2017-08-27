// stretch.ps.hlsl
// Pixel shader that computes texture stretching due to UV distortion.


float2 stretchfactor(float3 worldpos)
{
	float3 du = ddx(worldpos);
	float3 dv = ddy(worldpos);
	float1 stretchu = 0.001 / length(du);
	float1 stretchv = 0.001 / length(dv);
	//float1 stretchu = length(du) * 10.0;
	//float1 stretchv = length(dv) * 10.0;
	//float1 stretchu = du;
	//float1 stretchv = dv;

	return float2(stretchu, stretchv);
}


float4 main(float4 position : SV_POSITION, float3 worldpos : TEXCOORD) : SV_TARGET
{
	float3 du = ddx(worldpos.xyz);
	float3 dv = ddy(worldpos.xyz);
	float1 stretchu = 0.001 / length(du);
	float1 stretchv = 0.001 / length(dv);
	//float1 stretchu = length(du) * 10.0;
	//float1 stretchv = length(dv) * 10.0;
	//float1 stretchu = du;
	//float1 stretchv = dv;
	
	return float4(stretchu, stretchv, 0.0, 1.0);
}
