struct VS_OUTPUT
{
	float4 position:SV_POSITION;
	//float4 col:COLOR;
};

struct VS_INPUT
{
	float3 pos:POSITION;
	//float4 col:COLOR;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
	VS_OUTPUT output;

	//output.position = mul(gWorldViewProj,vPos);
	output.position = float4(input.pos,1.0f);

	return output;
}

struct PS_OUTPUT
{
	float4 color:SV_TARGET;
};

PS_OUTPUT PSMain(VS_OUTPUT input)
{
	PS_OUTPUT output;
	output.color = float4(1.0f,0.0f,0.0f,1.0f);

	return output;
}