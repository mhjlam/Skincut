// main.ps.hlsl
// Main skin rendering pixel shader.



#pragma pack_matrix(row_major)

#define NUM_LIGHTS 5
#define SHADOW_MAP_SIZE	2048


struct PSIN
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	float3 worldpos : TEXCOORD1;	// world position
	float3 viewdir  : TEXCOORD2;	// view direction
	float3 normal   : TEXCOORD3;
	float4 tangent  : TEXCOORD4;
};

struct PSOUT
{
	float4 color    : SV_TARGET0;
	float4 depth    : SV_TARGET1;
	float4 specular : SV_TARGET2;
	float4 discolor : SV_TARGET3;
};

struct Light
{
	float1 farplane;		// 10.0
	float1 falloffdist;		// 0.924
	float1 falloffwidth;	// 0.050
	float1 attenuation;		// 0.008

	float4 color;
	float4 position;
	float4 direction;

	matrix viewproj;		// float4x4
};

cbuffer cbGlobals : register(b0)
{
	bool EnableColor;
	bool EnableBumps;
	bool EnableShadows;
	bool EnableSpeculars;
	bool EnableOcclusion;
	bool EnableIrradiance;

	float1 Ambient;
	float1 Fresnel;
	float1 Specular;
	float1 Bumpiness;
	float1 Roughness;
	float1 ScatterWidth;
	float1 Translucency;
};

cbuffer cbLights : register(b1)
{
	Light Lights[5];
};

Texture2D ColorMap : register(t0);
Texture2D NormalMap	: register(t1);
Texture2D SpecularMap : register(t2);
Texture2D OcclusionMap : register(t3);
Texture2D DiscolorMap : register(t4);
Texture2D BeckmannMap : register(t5);
TextureCube IrradianceMap : register(t6);
Texture2D ShadowMaps[5] : register(t7);

SamplerState SamplerLinear : register(s0);
SamplerState SamplerAnisotropic : register(s1);
SamplerComparisonState SamplerShadowComparison : register(s2);


// Kelemen/Szirmay-Kalos Specular BRDF.
float1 SpecularKSK(float3 normal, float3 light, float3 view, float1 intensity, float1 roughness)
{
	// half-way vector
	float3 halfway = light + view;
	float3 halfn = normalize(halfway);

	float1 ndotl = saturate(dot(normal, light));
	float1 ndoth = saturate(dot(normal, halfn));

	// fraction of facets oriented towards h
	float1 ph = pow(abs(2.0 * BeckmannMap.Sample(SamplerLinear, float2(ndoth, 1.0 - roughness)).r), 10.0);

	float f0 = 0.028;											// reflectance of skin at normal incidence
	float f = (1.0 - f0) * pow(1.0 - dot(view, halfn), 5.0);	// Schlick's Fresnel approximation
	f = lerp(0.25, f, Fresnel);									// modulate by given Fresnel value

	float1 fr = max(0.0, ph * f / dot(halfway, halfway));		// dot(halfway, halfway) = length(halfway)^2
	return intensity * ndotl * fr;
}


// Percentage-closer filtering (anti-aliased shadows).
float1 ShadowPCF(float3 worldpos, matrix viewproj, float1 farplane, Texture2D shadowmap, int samples)
{
	float4 lightpos = mul(float4(worldpos, 1.0), viewproj);		// transform position into light space
	lightpos.xy = lightpos.xy / lightpos.w;						// perspective transform
	lightpos.z = lightpos.z - 0.01;								// shadow bias

	float1 shadow = 0.0;
	float1 offset = (samples - 1.0) / 2.0;
	float1 depth = lightpos.z / farplane;						// depth of position, remapped to [0,1]

	[unroll]
	for (float1 x = -offset; x <= offset; x += 1.0)
	{
		[unroll]
		for (float1 y = -offset; y <= offset; y += 1.0)
		{
			float2 pos = lightpos.xy + (float2(x, y) / SHADOW_MAP_SIZE);
			shadow += saturate(shadowmap.SampleCmp(SamplerShadowComparison, pos, depth).r);
		}
	}

	return shadow / (samples*samples);
}


// Screen-space translucency.
float3 Transmittance(float3 worldpos, float3 normal, float3 lightdir, Texture2D shadowmap, matrix viewproj, float1 farplane, float3 spotlight, float3 albedo)
{
	float scale = 8.25 * (1.0 - Translucency) / ScatterWidth;

	float4 pos = float4(worldpos - 0.005 * normal, 1.0);		// used to prevent aliasing artifacts around edges
	float4 lightpos = mul(pos, viewproj);						// light-space position of pixel
	float1 d1 = shadowmap.Sample(SamplerLinear,					// incoming point
		lightpos.xy / lightpos.w).r * farplane;							
	float1 d2 = lightpos.z;										// outgoing point
	float1 dist = abs(d1 - d2);									// distance between incoming and outgoing points

	float1 s = scale * dist;
	float1 ss = -s*s;

	// diffusion profile for skin
	float3 profile = float3(0, 0, 0);
	profile += float3(0.233, 0.455, 0.649) * exp(ss / 0.0064);
	profile += float3(0.100, 0.336, 0.344) * exp(ss / 0.0484);
	profile += float3(0.118, 0.198, 0.000) * exp(ss / 0.1870);
	profile += float3(0.113, 0.007, 0.007) * exp(ss / 0.5670);
	profile += float3(0.358, 0.004, 0.000) * exp(ss / 1.9900);
	profile += float3(0.078, 0.000, 0.000) * exp(ss / 7.4100);

	// transmittance = profile * (spotlight color * attenuation * falloff) * albedo * E;
	float3 E = saturate(0.3 + dot(-normal, lightdir)); // wrap lighting
	return profile * spotlight * albedo * E;
}


// Main skin rendering shader.
PSOUT main(PSIN input)
{
	PSOUT output;
	output.color = float4(0,0,0,0);
	output.depth = float4(0,0,0,0);
	output.specular = float4(0,0,0,0);
	output.discolor = float4(0,0,0,0);

	// Obtain albedo, occlusion, and irradiance for ambient lighting.
	float4 albedo = (EnableColor) ? ColorMap.Sample(SamplerAnisotropic, input.texcoord) : float4(0.5, 0.5, 0.5, 1.0);
	float3 irradiance = (EnableIrradiance) ? IrradianceMap.Sample(SamplerLinear, input.normal).rgb : float3(1.0, 1.0, 1.0);
	float1 occlusion = (EnableOcclusion) ? OcclusionMap.Sample(SamplerLinear, input.texcoord).r : 1.0;

	// Obtain intensity and roughness for specular lighting.
	float1 intensity = (EnableSpeculars) ? SpecularMap.Sample(SamplerLinear, input.texcoord).r * Specular : Specular;
	float1 roughness = (EnableSpeculars) ? SpecularMap.Sample(SamplerLinear, input.texcoord).g * 3.3333 * Roughness : Roughness;
	
	// Normal mapping.
	float3 normal = input.normal;

	if (EnableBumps)
	{
		// Construct tangent-space basis (TBN).
		float3 tangent = input.tangent.xyz;
		float3 bitangent = input.tangent.w * cross(normal, tangent); // compute bitangent from normal, tangent, and its handedness
		float3x3 tbn = transpose(float3x3(tangent, bitangent, normal)); // transforms from tangent space to world space (due to transpose)

		// Determine bump vector that perturbs normal.
		float3 bump = NormalMap.Sample(SamplerAnisotropic, input.texcoord).rgb * 2.0 - 1.0; // remap from [0,1] to [-1,1]
		//bump.z = sqrt(1.0 - (bump.x * bump.x) - (bump.y * bump.y)); // z can be computed from xy (since normal map is normalized)

		// Perturb original normal by bump map in tangent space and transform to world space.
		float3 tangentNormal = lerp(float3(0.0, 0.0, 1.0), bump, Bumpiness.x); // lerp between original normal and bumped normal
		normal = mul(tbn, tangentNormal); // transform normal in tangent space to world space
	}

	
	// Compute and accumulate ambient lighting.
	float3 ambient = Ambient * occlusion * irradiance * albedo.rgb; // ambient light and occlusion
	output.color.rgb += ambient;
	

	// Diffuse and specular lighting.
	[unroll] for (int i = 0; i < NUM_LIGHTS; i++)
	{
		Light light = Lights[i];
		Texture2D shadowmap = ShadowMaps[i];

		// Compute vector from surface position to light position.
		float3 lightdir = light.position.xyz - input.worldpos;
		float1 lightdist = length(lightdir); // distance to light
		lightdir /= lightdist; // normalize surface-to-light vector

		// Cosine of angle between light direction and surface-to-light vector.
		float1 spotangle = dot(light.direction.xyz, -lightdir);

		// If pixel is inside spotlight's circle of influence...
		if (spotangle > light.falloffdist)
		{
			// Compute spotlight attenuation and falloff
			float1 curve = min(pow(lightdist / light.farplane, 6.0), 1.0);
			float1 attenuation = (1.0 - curve) * (1.0 / (1.0 + light.attenuation * lightdist*lightdist)); // if curve=1 => att=0
			float1 falloff = saturate((spotangle - light.falloffdist) / light.falloffwidth);

			// Compute wavelength-independent diffuse and specular lighting.
			float1 lambert = saturate(dot(normal, lightdir)); // Lambertian reflectance
			float1 kskspec = SpecularKSK(normal, lightdir, input.viewdir, intensity, roughness); // Kelemen/Szirmay-Kalos

			// Compute shadows (percentage-closer filtering).
			float1 shadow = (EnableShadows) ? ShadowPCF(input.worldpos, light.viewproj, light.farplane, shadowmap, 3) : 1.0;

			// Compute per-light diffuse and specular components.
			float3 spotlight = light.color.xyz * attenuation * falloff;
			float3 diffuse = spotlight * albedo.rgb * lambert * shadow;
			float3 specular = spotlight * kskspec * shadow;

			// Accumulate diffuse and specular reflectance.
			if (EnableSpeculars) // separate speculars
			{
				output.color.rgb += diffuse;
				output.specular.rgb += specular;
			}
			else // speculars included in output color
			{
				output.color.rgb += (diffuse + specular);
			}

			// Compute and accumulate transmittance (translucent shadow maps).
			if (EnableShadows)
			{
				output.color.rgb += Transmittance(input.worldpos, input.normal, 
					lightdir, shadowmap, light.viewproj, light.farplane, spotlight, albedo.rgb);
			}
		}
	}

	// render information to screen-space textures for later
	output.color.a = albedo.a;
	output.depth = input.position.w;
	output.discolor = DiscolorMap.Sample(SamplerAnisotropic, input.texcoord);
	
	return output;
}
