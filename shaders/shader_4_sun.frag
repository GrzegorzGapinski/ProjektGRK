#version 410 core

uniform vec3 objectColor;
//uniform vec3 lightDir;
uniform vec3 lightPos;
uniform vec3 cameraPos;

in vec3 interpNormal;
in vec3 fragPos;

void main()
{
	vec3 normal = normalize(interpNormal);
	vec3 V = normalize(cameraPos-fragPos);
	float coef = max(0,dot(V,normal));
	gl_FragColor = vec4(mix(objectColor,vec3(1,0.5,0.1),1-coef), 1.0);
}
