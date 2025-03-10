#include "Noise.h.hlsl"
#include "Distance.h.hlsl"

cbuffer cb_falloff : register(b1)
{
	float2 Point0;
	float2 Point1;
	float MaxDistance;
};


float4 main(float4 position : SV_POSITION, float2 texcoord : TEXCOORD) : SV_TARGET
{
	float noise = clamp(1.0 + fbm(texcoord * 8.0f, 4, 0.5f, 1.0f) * 0.5, 0.01, 0.99);
	float dist = distance(Point0, Point1, texcoord);
	float invdist = 1.0 - dist;

	float l0 = MaxDistance * 0.75;
	float l1 = MaxDistance * 0.85;

	if (dist > l0)
	{
		noise = lerp(noise, 1.0, abs(invdist - l1) / abs(l1 - l0));
	}
	else
	{
		float x = dist / l0;
		noise *= -1.0 / (0.666*sqrt(6.28319)) * exp(-pow((x-0), 2) / (2.0*0.333*0.333)) + 1.0; // upside down bell curve
	}

	// bell curve: 1.0/(0.666*sqrt(sqrt(2pi)))*e^(-(x-0)^2/(2.0*0.333^2))
	// upside down: -1.0/(0.666*sqrt(sqrt(2pi)))*e^(-(x-0)^2/(2.0*0.333^2)) + 1.0

	//float random = clamp(rand(input.texcoord), 0.5, 1.0);
	//float alpha = random * 0.08333; // max: 0.08333
	return float4(1.00, 0.37, 0.30, noise);
}
