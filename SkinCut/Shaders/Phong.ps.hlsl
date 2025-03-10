// phong.ps.hlsl
// Phong pixel shader.


Texture2D ColorTexture : register(t0);
sampler LinearSampler : register(s0);

cbuffer cb0 : register(b0)
{
	float1 AmbientColor;
	float1 DiffuseColor;
	float1 SpecularColor;
	float1 SpecularPower;
	float4 LightColor;
	float4 LightDirection;
};


float4 main(float4 position : SV_POSITION, 
			float2 texcoord : TEXCOORD0, 
			float3 normal   : TEXCOORD1, 
			float3 halfway  : TEXCOORD2) : SV_TARGET0
{
	normal = normalize(normal);
	halfway = normalize(halfway);
	
	float4 Ia = AmbientColor * float4(1,1,1,1);// * AmbientLight;
	float4 Id = DiffuseColor * saturate(dot(normal, -LightDirection.xyz));
	float4 Is = SpecularColor * pow(saturate(dot(normal, halfway)), SpecularPower);
	
	return Ia + (Id + Is) * LightColor * ColorTexture.Sample(LinearSampler, texcoord);
}
