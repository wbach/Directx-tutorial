cbuffer ConstantBuffer : register( b0 )
{
    bool red;
    float3 inColor;
	matrix Projection;
}

cbuffer PerFrameBuffer : register( b1 )
{
	matrix View;
}

cbuffer PerObjectBuffer : register( b2 )
{
	matrix World;
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
	
	//output.pos = output.pos;
	output.pos = mul( pos, World );
    output.pos = mul( output.pos, View );
    output.pos = mul( output.pos, Projection );
	output.color = color;
    if (red)
    {
        output.color = float4(inColor, 1);
    }
    return output;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( VS_OUTPUT input ) : SV_Target
{
    return input.color;    // Yellow, with Alpha = 1
}
