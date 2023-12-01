#version 430

// pipeline-ból bejövő per-fragment attribútumok
in vec3 vs_out_pos;
in vec3 vs_out_norm;
in vec2 vs_out_tex;

// kimenő érték - a fragment színe
out vec4 fs_out_col;

// textúra mintavételező objektum
uniform sampler2D texImage;

uniform vec3 cameraPos;

// fenyforras tulajdonsagok
uniform vec4 lightPos = vec4( 0.0, 1.0, 0.0, 0.0 );
uniform vec3 spotDir = vec3( 0.0, 0.0, 0.0 );

uniform float cutoff = 0.0;

// komponensenként meg lehet adni a fénynek a színét
uniform vec3 La = vec3(0.0, 0.0, 0.0 ); // az ambiens komponens színe
uniform vec3 Ld = vec3(1.0, 1.0, 1.0 );
uniform vec3 Ls = vec3(1.0, 1.0, 1.0 );

 uniform float lightConstantAttenuation    = 1.0;
 uniform float lightLinearAttenuation      = 0.0;
 uniform float lightQuadraticAttenuation   = 0.0;

// anyag tulajdonsagok

uniform vec3 Ka = vec3( 1.0 ); // hogyan reagál a felület az adott fénykomponensre
uniform vec3 Kd = vec3( 1.0 );
uniform vec3 Ks = vec3( 1.0 );

uniform float Shininess = 9.0;

/* segítség:
	    - normalizálás: http://www.opengl.org/sdk/docs/manglsl/xhtml/normalize.xml
	    - skaláris szorzat: http://www.opengl.org/sdk/docs/manglsl/xhtml/dot.xml
	    - clamp: http://www.opengl.org/sdk/docs/manglsl/xhtml/clamp.xml
		- reflect: http://www.opengl.org/sdk/docs/manglsl/xhtml/reflect.xml
				reflect(beérkező_vektor, normálvektor);
		- pow: http://www.opengl.org/sdk/docs/manglsl/xhtml/pow.xml
				pow(alap, kitevő);
*/

void main()
{
	vec3 ambient = La * Ka;
	vec3 toLight;
	float attenuation = 1.0f;

	if (lightPos.w == 0.0) {
		toLight = normalize(-lightPos.xyz);
	} else {
		toLight = lightPos.xyz - vs_out_pos;
		float lightDistance = length(toLight);
		toLight = normalize(toLight);

		// attenuation = 1.0 / (lightDistance * lightDistance); // valós, fizika környezetben ez a képlet a fénycsillapodásra
		attenuation = 1.0 / (lightConstantAttenuation + 
							 lightLinearAttenuation * lightDistance + 
							 lightQuadraticAttenuation * lightDistance * lightDistance);

		if (lightPos.w == 2.0 && dot(toLight, spotDir) < cutoff) {
			attenuation *= 0;
		}
	} 

	vec3 normal = normalize(vs_out_norm);
	toLight = normalize(toLight);
	float di = max(dot(toLight, normal), 0.0); // ne mehessünk nulla alá, ezért kell a max itt
	vec3 diffuse = Ld * Kd * di * attenuation;

	vec3 reflectLight = reflect(-toLight, normal);
	vec3 toCamera = normalize(cameraPos.xyz - vs_out_pos.xyz);
	float scalarValue = pow(max(dot(reflectLight, toCamera), 0.0), Shininess); // itt is fontos, hogy ne menjünk 0 alá
	vec3 specular = Ls * Ks * scalarValue * attenuation;

	fs_out_col = vec4(ambient + diffuse + specular, 1) * texture(texImage, vs_out_tex); // ez a Phong-fénymodell
}