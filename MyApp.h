#pragma once

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform2.hpp>

// GLEW
#include <GL/glew.h>

// SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

// Utils
#include "GLUtils.hpp"
#include "Camera.h"

static std::string title = "Alap fejlec";

struct SUpdateInfo
{
	float ElapsedTimeInSec = 0.0f; // Program indulása óta eltelt idő
	float DeltaTimeInSec   = 0.0f; // Előző Update óta eltelt idő
};

const std::initializer_list<VertexAttributeDescriptor> vertexAttribList =
{
	{ 0, offsetof(Vertex, position), 3, GL_FLOAT },
	{ 1, offsetof(Vertex, normal), 3, GL_FLOAT },
	{ 2, offsetof(Vertex, texcoord), 2, GL_FLOAT },
};

class CMyApp
{
private:
	SDL_Window *win;

	void ChangeTitle(); // metódus a fejléc módosításához 
	void TeleportToNextObject(); // teleport metódus

public:
	CMyApp(SDL_Window *_win);
	~CMyApp();

	bool Init();
	void Clean();

	void Update( const SUpdateInfo& );
	void Render();
	void RenderGUI();

	void KeyboardDown(const SDL_KeyboardEvent&);
	void KeyboardUp(const SDL_KeyboardEvent&);
	void MouseMove(const SDL_MouseMotionEvent&);
	void MouseDown(const SDL_MouseButtonEvent&);
	void MouseUp(const SDL_MouseButtonEvent&);
	void MouseWheel(const SDL_MouseWheelEvent&);
	void Resize(int, int);
protected:
	void SetupDebugCallback();

	//
	// Adat változók
	//

	glm::vec3 m_newObjectPosition{ 0.0f, 0.0f, 0.0f }; // az új objektum pozíciója, ezt olvassuk be a UI-ból
	std::vector<glm::vec3> m_newPositionVector{}; // ebben tároljuk az újonnan létrehozott gömbök koordinátáit
	int m_nextPosition = -1; // teleport esetén a következő pozíció, kezdetben -1, mert nincs ilyen

	float m_ElapsedTimeInSec = 0.0f;

	int m_resolutionN = 50; // kezdeti felbontása a fánknak
	int m_resolutionM = 50; // szintén

	const float m_sphereRadius = 2.0f; // gömb sugara

	// Suzanne params
	static constexpr glm::vec3 SUZANNE_POS = glm::vec3( 0.0f, 0.0f, 0.0f );

	// Kamera
	Camera m_camera;
	float m_radius = 10; // sugár, hogy az objektumtól milyen távol forogjon a kamera

	// Fényforrások

	//static constexpr glm::vec3 BUG_COLOR = glm::vec3( 0.53f, 1.0f, 0.3f );

	//
	// OpenGL-es dolgok
	//
	
	// uniform location lekérdezése
	GLint ul( const char* uniformName ) noexcept;

	// shaderekhez szükséges változók
	GLuint m_programID = 0; // shaderek programja

	// Fényforrás- ...
	glm::vec4 m_lightPos = glm::vec4( 0.0f, 1.0f, 0.0f, 0.0f );
	glm::vec3 m_spotDir = glm::vec3( 0.0f, 0.0f, 0.0f );
	// 
	glm::vec3 m_La = glm::vec3(0.0, 0.0, 0.0 );
	// glm::vec3 m_Ld = glm::vec3(1.0, 1.0, 1.0 );
	// glm::vec3 m_Ls = glm::vec3(1.0, 1.0, 1.0 );
	// 
	 float m_lightConstantAttenuation    = 1.0;
	 float m_lightLinearAttenuation      = 0.0;
	 float m_lightQuadraticAttenuation   = 0.0;
	// 
	// float     m_spotCosCutoff = 0.0;
	// float     m_spotExponent  = 0.0;
	// glm::vec3 m_spotDirection = glm::vec3(0.0, -1.0, 0.0);
	// 
	// // ... és anyagjellemzők
	glm::vec3 m_Ka = glm::vec3( 1.0 );
	// glm::vec3 m_Kd = glm::vec3( 1.0 );
	// glm::vec3 m_Ks = glm::vec3( 1.0 );
	// 
	// float m_Shininess = 1.0;

	// Shaderek inicializálása, és törlése
	void InitShaders();
	void CleanShaders();

	// Geometriával kapcsolatos változók
	OGLObject m_SuzanneGPU = {};	  // Suzanne
	OGLObject m_ParamSurfaceGPU = {}; // Parametrikus felület
	OGLObject m_ParamSphereGPU = {};
	std::vector<OGLObject> m_generatedObjects{}; // vektorban eltároljuk a helyét az újonnan generált objektumoknak

	// Geometria inicializálása, és törlése
	void InitGeometry();
	void InitParametricSurfaceGeometry();
	void InitParametricSphereGeometry();
	void CleanGeometry();
	void CleanParametricSurfaceGeometry(); 
	void CleanParametricSphereGeometry();

	void RenderGeneratedObject(glm::vec3 objectPosition); // metódus, ami meghívódik minden egyes alkalommal, mikor a 
														  // felhasználó létre akar hozni egy új objektumot az adott pozícióba
	void RenderParametricSurface();
	bool HasCollidingSpheres(glm::vec3 newCoordinates);

	// Textúrázás, és változói
	GLuint m_SuzanneTextureID = 0;
	GLuint m_ParamSurfaceTextureID = 0;

	float m_cutoff = 0.0f;
	int lightType = 0;

	void InitTextures();
	void CleanTextures();
};

