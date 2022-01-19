#version 410 core

uniform vec3 objectColor;
uniform vec3 lightDir;

in vec3 interpNormal;

void main()
{
	vec3 normal = normalize(interpNormal);
	float ambient = 0.2;
	float diffuse = max(dot(normal, -lightDir), 0.0);
	gl_FragColor = vec4(objectColor * (ambient + (1-ambient) * diffuse), 1.0);
}
