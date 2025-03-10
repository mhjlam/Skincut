// patch.ps.hlsl
// Pixel shader that generates wound patch texture.


#pragma pack_matrix(row_major)

#include "noise.h.hlsl"
#include "distance.h.hlsl"


cbuffer cb_patch : register(b0)
{
	float4 DarkColor;
	float4 LightColor;
	float4 InnerColor;
	float OffsetX;
	float OffsetY;
};



float parabola(float x, float a, float b) // parabola
{
	x = 2.0 * x - 1.0; // [0,1] to [-1,1]
	return -a * (x*x) + b;
}

float parabola(float x, float a, float b, int e) // parabola with exponent
{
	x = 2.0 * x - 1.0; // [0,1] to [-1,1]
	return -a * pow(x, e) + b;
}


float4 main(float4 position : SV_POSITION, float2 texcoord : TEXCOORD) : SV_TARGET
{
	// horizontal and vertical distance from center
	float distx = abs(texcoord.x - 0.5); // horizontal distance to center
	float disty = abs(texcoord.y - 0.5); // vertical distance to center

	// ranges of color layers
	float2 offset_inner = float2((texcoord.x + OffsetX) * 4.0, 0.5);
	float range_inner = max((pnoise(offset_inner) + 1.0) / 8.0, 0.05) * 
		parabola(texcoord.x, 0.5, 0.45); // divide by more than 2 to reduce amplitude
	float range_inner_fade = (range_inner + 0.05) * parabola(texcoord.x, 1.0, 1.00);
	float range_outer = parabola(texcoord.x, 0.3, 0.25, 4);
	float range_outer_fade = (range_outer + 0.05); //* parabola(texcoord.x, 0.2, 0.25);
	
	// colors of layers
	float4 color_inner = InnerColor;
	float4 color_outer = lerp(DarkColor, LightColor, 
		(fbm(texcoord + float2(OffsetX, OffsetY), 4, 0.5, 4.0) + 1.0) / 2.0);

	// texcoords 0.025 to 0.975
	if (distx < 0.475)
	{
		// inner layer
		if (disty < range_inner)
			return color_inner;

		// inner layer fade-off
		if (disty < range_inner_fade)
		{
			// color of noise at outer layer
			float1 offsety = range_inner_fade;
			if (texcoord.y > 0.5) offsety = -offsety;
			float2 offset = (float2(texcoord.x, 0.5 - offsety)) + float2(OffsetX, OffsetY);
			float4 samplecolor = lerp(DarkColor, LightColor, (fbm(offset, 4, 0.5, 4.0) + 1.0) / 2.0);

			// lerp from inner color to outer color
			float t = (disty - range_inner) / (range_inner_fade - range_inner);
			return lerp(color_inner, samplecolor, t);
		}
	}

	// outer layer
	if (disty < range_outer)
		return color_outer;

	// outer layer fade-off
	if (disty < range_outer_fade)
	{
		float1 offsety = range_outer;
		if (texcoord.y > 0.5) offsety = -offsety;
		float2 offset = (float2(texcoord.x, 0.5 - offsety)) + float2(OffsetX, OffsetY);
		float4 samplecolor = lerp(DarkColor, LightColor, (fbm(offset, 4, 0.5, 4.0) + 1.0) / 2.0);
		
		// lerp from outer color to transparency
		float t = (disty - range_outer) / (range_outer_fade - range_outer);
		return lerp(samplecolor, float4(0,0,0,0), t);
	}

	return float4(0,0,0,0);
}

