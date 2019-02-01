cbuffer ConstantBuffer : register( b0 )
{
	matrix World;
	matrix View;
	matrix Projection;
}

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VS( float4 pos : POSITION, float4 color : COLOR )
{
	VS_OUTPUT output = (VS_OUTPUT)0;
	
	output.pos = mul( pos, World );
    output.pos = mul( output.pos, View );
    output.pos = mul( output.pos, Projection );
	output.color = color;
    return output;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( VS_OUTPUT input ) : SV_Target
{
    return input.color;    // Yellow, with Alpha = 1
}
