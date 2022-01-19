#version 410 core

uniform vec3 objectColor;
//uniform vec3 lightDir;
uniform vec3 lightPos;
uniform vec3 cameraPos;

in vec3 interpNormal;
in vec3 fragPos;

void main()
{
	vec3 lightDir = normalize(lightPos-fragPos);
	vec3 V = normalize(cameraPos-fragPos);
	vec3 normal = normalize(interpNormal);
	vec3 R = reflect(-normalize(lightDir),normal);
	
	float specular = pow(max(0,dot(R,V)),10);
	float diffuse = max(0,dot(normal,normalize(lightDir)));
	gl_FragColor = vec4(mix(objectColor,objectColor*diffuse+vec3(1)*specular,0.9), 1.0);
}
