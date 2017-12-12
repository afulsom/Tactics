//--------------------------------------------------------------------------------------
struct vs_input_t
{
	float3 position : POSITION;
	float2 uv0 : UV0;
	float4 tint: TINT;
	// float4x4 test_mat : SOME_MATRIX;
};

//--------------------------------------------------------------------------------------
struct v2f_t
{
	float4 position : SV_Position;
	float2 uv : UV;
	float4 tint : TINT;
};

cbuffer matrix_cb : register(b0)
{
	float4x4 MODEL;
	float4x4 VIEW;
	float4x4 PROJECTION;
};

cbuffer time_cb : register(b1)
{
	float TIME;

	float3 PADDING;
};

Texture2D <float4> tTexture : register(t0);
SamplerState sSampler : register(s0);


float RangeMap(float d0, float d1, float r0, float r1, float value)
{
	return ((value - d0) / (d1 - d0)) * (r1 - r0) + r0;
}

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
v2f_t VertexFunction(vs_input_t input)
{
	v2f_t v2f = (v2f_t)0;

	float4 view = mul(float4(input.position, 1.f), VIEW);
	float4 finalPosition = mul(view, PROJECTION);

	v2f.position = finalPosition;
	v2f.uv = input.uv0;
	v2f.tint = input.tint;
	return v2f;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 FragmentFunction(v2f_t input) : SV_Target0 // semeantic of what I'm returning
{
	float4 diffuse = tTexture.Sample(sSampler, input.uv);
	return input.tint * diffuse;
}