#include "MyApp.h"
#include "SDL_GLDebugMessageCallback.h"
#include "ParametricSurfaceMesh.hpp"
#include "ObjParser.h"
#include <iostream>
#include <sstream>

#include <imgui.h>

CMyApp::CMyApp(SDL_Window* _win) : win(_win)
{
}

CMyApp::~CMyApp()
{
}

// tórusz kirajzolásához szükséges adatok
struct Torus
{
	float a, b;
	Torus(float _a = 1.0f, float _b = 2.0f) : a(_a), b(_b) { }

	glm::vec3 GetPos(float u, float v) const noexcept
	{
		u *= glm::two_pi<float>();
		v *= -glm::two_pi<float>();
		return glm::vec3(
			(a * cosf(v) + b) * cosf(u),
			 a * sinf(v),
			(a * cosf(v) + b) * sinf(u)
		);
	}
	glm::vec3 GetNorm(float u, float v) const noexcept
	{
		glm::vec3 du = GetPos(u + 0.01f, v) - GetPos(u - 0.01f, v);
		glm::vec3 dv = GetPos(u, v + 0.01f) - GetPos(u, v - 0.01f);

		return glm::normalize(glm::cross(du, dv));
	}
	glm::vec2 GetTex(float u, float v) const noexcept
	{
		return glm::vec2(u, v);
	}
};

// gömb parametrikus egyenlete
struct Sphere
{
	float r;
	Sphere(float _r = 1.f) : r(_r) { }

	glm::vec3 GetPos(float u, float v) const noexcept
	{
		u *= glm::two_pi<float>();
		v *= glm::pi<float>();

		return glm::vec3(
			r * sinf(v) * cosf(u),
			r * cosf(v),
			r * sinf(v) * sinf(u)
		);
	}
	glm::vec3 GetNorm(float u, float v) const noexcept
	{
		u *= glm::two_pi<float>();
		v *= glm::pi<float>();

		return glm::vec3(
			sinf(v) * cosf(u),
			cosf(v),
			sinf(v) * sinf(u)
		);
	}
	glm::vec2 GetTex(float u, float v) const noexcept
	{
		return glm::vec2(u, v);
	}
};

void CMyApp::SetupDebugCallback()
{
	// engedélyezzük és állítsuk be a debug callback függvényt ha debug context-ben vagyunk 
	GLint context_flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
	if (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
		glDebugMessageCallback(SDL_GLDebugMessageCallback, nullptr);
	}
}

void CMyApp::InitShaders()
{
	m_programID = glCreateProgram();
	AssembleProgram( m_programID, "Vert_PosNormTex.vert", "Frag_LightingSkeleton.frag" );
}

void CMyApp::CleanShaders()
{
	glDeleteProgram( m_programID );
}

void CMyApp::InitGeometry()
{
	// Suzanne betöltése
	MeshObject<Vertex> suzanneMeshCPU = ObjParser::parse("Assets/Suzanne.obj");
	m_SuzanneGPU = CreateGLObjectFromMesh( suzanneMeshCPU, vertexAttribList );

	InitParametricSurfaceGeometry();
	InitParametricSphereGeometry();
}

void CMyApp::InitParametricSurfaceGeometry() {
	// Patametrikus felület
	MeshObject<Vertex> surfaceMeshCPU = GetParamSurfMesh(Torus(), m_resolutionN, m_resolutionM);
	m_ParamSurfaceGPU = CreateGLObjectFromMesh(surfaceMeshCPU, vertexAttribList);
}

void CMyApp::InitParametricSphereGeometry() {
	MeshObject<Vertex> sphereMeshCPU = GetParamSurfMesh(Sphere(m_sphereRadius));
	m_ParamSphereGPU = CreateGLObjectFromMesh(sphereMeshCPU, vertexAttribList);
}

void CMyApp::CleanGeometry()
{
	CleanOGLObject( m_SuzanneGPU );
	CleanParametricSurfaceGeometry();
	CleanParametricSphereGeometry();
}

void CMyApp::CleanParametricSurfaceGeometry() {
	CleanOGLObject( m_ParamSurfaceGPU );
}

void CMyApp::CleanParametricSphereGeometry() {
	CleanOGLObject(m_ParamSphereGPU);
}

void CMyApp::InitTextures()
{
	glGenTextures( 1, &m_SuzanneTextureID );
	TextureFromFile( m_SuzanneTextureID, "Assets/Wood_Table_Texture.png" );
	SetupTextureSampling( GL_TEXTURE_2D, m_SuzanneTextureID );

	glGenTextures(1, &m_ParamSurfaceTextureID);
	TextureFromFile(m_ParamSurfaceTextureID, "Assets/Wood_Table_Texture.png");
	SetupTextureSampling(GL_TEXTURE_2D, m_ParamSurfaceTextureID);
}

void CMyApp::CleanTextures()
{
	glDeleteTextures( 1, &m_SuzanneTextureID );
	glDeleteTextures( 1, &m_ParamSurfaceTextureID );
}

bool CMyApp::Init()
{
	SetupDebugCallback();

	// törlési szín legyen kékes
	glClearColor(0.125f, 0.25f, 0.5f, 1.0f);

	InitShaders();
	InitGeometry();
	InitTextures();

	//
	// egyéb inicializálás
	//

	glEnable(GL_CULL_FACE); // kapcsoljuk be a hátrafelé néző lapok eldobását
	glCullFace(GL_BACK);    // GL_BACK: a kamerától "elfelé" néző lapok, GL_FRONT: a kamera felé néző lapok

	glEnable(GL_DEPTH_TEST); // mélységi teszt bekapcsolása (takarás)

	// kamera
	m_camera.SetView(
		glm::vec3(0.0, 0.0,	m_camera.GetDistance()),					// honnan nézzük a színteret	   - eye
		glm::vec3(0.0, 0.0, 0.0),										// a színtér melyik pontját nézzük - at
		glm::vec3(0.0, 1.0, 0.0));										// felfelé mutató irány a világban - up

	std::cout << m_camera.GetDistance() << '\n';

	return true;
}

void CMyApp::Clean()
{
	CleanShaders();
	CleanGeometry();
	CleanTextures();
}

void CMyApp::Update( const SUpdateInfo& updateInfo )
{
	m_ElapsedTimeInSec = updateInfo.ElapsedTimeInSec;

	m_camera.Update( updateInfo.DeltaTimeInSec );
}

void CMyApp::Render()
{
	// töröljük a frampuffert (GL_COLOR_BUFFER_BIT)...
	// ... és a mélységi Z puffert (GL_DEPTH_BUFFER_BIT)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// kamera forgatása az objektum körül 
	m_camera.UpdateU();
	
	glUseProgram( m_programID );

	// ******* SUZANNE ********
	glBindVertexArray( m_SuzanneGPU.vaoID );

	// - Textúrák beállítása, minden egységre külön
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_SuzanneTextureID);

	glm::mat4 matWorld = glm::identity<glm::mat4>();
	matWorld = glm::translate( SUZANNE_POS );

	glUniformMatrix4fv( ul( "viewProj" ), 1, GL_FALSE, glm::value_ptr( m_camera.GetViewProj() ) );
	glUniformMatrix4fv( ul( "world" ),    1, GL_FALSE, glm::value_ptr( matWorld ) );
	glUniformMatrix4fv( ul( "worldIT" ),  1, GL_FALSE, glm::value_ptr( glm::transpose( glm::inverse( matWorld ) ) ) );
	// - textúraegységek beállítása
	glUniform1i( ul( "texImage" ), 0 );

	glDrawElements( GL_TRIANGLES,    
					m_SuzanneGPU.count,			 
					GL_UNSIGNED_INT,
					nullptr );

	// - Textúrák kikapcsolása, minden egységre külön
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, 0 );

	// VAO kikapcsolása
	glBindVertexArray( 0 );
	// ************************************************************************************ 

	// ******* Parametric ********
	RenderParametricSurface();
	// ************************************************************************************ 

	// ******* Generált objektum ********
	for (auto pos : m_newPositionVector) {
		RenderGeneratedObject(pos);
	}
	// ************************************************************************************ 

	// shader kikapcsolasa
	glUseProgram(0);
}

void CMyApp::RenderGeneratedObject(glm::vec3 objectPosition) {
	glUseProgram(m_programID); // shader bekapcsolás
	glBindVertexArray(m_ParamSphereGPU.vaoID);

	// Textúrázás
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_ParamSurfaceTextureID);

	glm::mat4 matWorld = glm::translate(objectPosition); // objektum eltranszformálása az adott pozícióba
	// leküldjük a world-ot és annak inverzét
	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv(ul("worldIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(matWorld))));

	// Suzanne kirajzolása
	glDrawElements(GL_TRIANGLES,
				   m_SuzanneGPU.count,
				   GL_UNSIGNED_INT,
				   nullptr);

	// Textúrák kikapcsolása
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	// VAO kikapcsolása
	glBindVertexArray(0);
	
	// Shader kikapcsolása
	glUseProgram(0);
}

void CMyApp::RenderParametricSurface() {
	glBindVertexArray(m_ParamSurfaceGPU.vaoID);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_ParamSurfaceTextureID);

	glm::mat4 matWorld = glm::translate(glm::vec3(0.0, -3.0, 0.0));

	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv(ul("worldIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(matWorld))));

	glDrawElements(GL_TRIANGLES,
		m_ParamSurfaceGPU.count,
		GL_UNSIGNED_INT,
		nullptr);

	// - Textúrák kikapcsolása, minden egységre külön
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	// VAO kikapcsolása
	glBindVertexArray(0);
}

void CMyApp::RenderGUI()
{
	if (ImGui::Begin("Teleporting objects")) {
		// ********* KAMERA TÁVOLSÁG ********* 
		m_radius = m_camera.GetDistance(); // kiszedjük a kamerából a jelenlegi távolságot, ez lesz az érték a csúnkán
		ImGui::SliderFloat("Távolság", &m_radius, 2.0, 100);
		m_camera.SetDistance(m_radius); // beállítjuk távolságnak a megadott sugarat

		// ********* SZÖVEG *********
		char buffer[256]; // buffer az title beolvasáságoz
		strcpy(buffer, title.c_str()); // az alap címet belemásoljuk

		// beolvassuk a beírt szöveget, majd módosítjuk a tartalmát az ablak fejlécének
		if (ImGui::InputText("Ablak címe", buffer, IM_ARRAYSIZE(buffer))) {
			title = buffer;
			ChangeTitle();
		}

		// ********* TELEPORT *********
		ImGui::SliderFloat3("(X, Y, Z) koordináták", glm::value_ptr(m_newObjectPosition), -10, 30);
		if (ImGui::Button("Alakzat létrehozása")) {
			// az új pozíciót csak akkor vesszük fel, ha nincs olyan objektum, amivel ütközne
			if (!HasCollidingSpheres(m_newObjectPosition)) {
				m_newPositionVector.push_back(m_newObjectPosition);
				// ha még nem volt következő objektum, és sikerült létrehozni egyet,
				// akkor beállítjuk azt, vagyis az első, mint a kövi objektum
				if (m_nextPosition == -1) {
					m_nextPosition = 0;
				}
				// ez az az eset, amikor az utolsó objektumon állunk, nem tudunk tovább teleportálni,
				// és létrehozunk egy újat
				else if (m_nextPosition == m_newPositionVector.size() - 2) {
					m_nextPosition++;
				}

			}
		}

		if (ImGui::Button("TELEPORT!")) {
			TeleportToNextObject();
		}

		// ********* FELBONTÁS *********
		const bool isNChanged = ImGui::SliderInt("Folbontás N", &m_resolutionN, 1, 100);
		const bool isMChanged = ImGui::SliderInt("Folbontás M", &m_resolutionM, 1, 100);
		if (isNChanged || isMChanged) 
		{
			CleanParametricSurfaceGeometry();
			InitParametricSurfaceGeometry();
		}
	}
	ImGui::End();
}

void CMyApp::ChangeTitle() {
	// kérdezzük le az OpenGL verziót
	int glVersion[2] = { -1, -1 };
	glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
	glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);

	// mindent ugyanúgy csinálunk, mint a mainban
	std::stringstream window_title;
	window_title << "OpenGL " << glVersion[0] << "." << glVersion[1] << " " << title;
	SDL_SetWindowTitle(win, window_title.str().c_str());
}

bool CMyApp::HasCollidingSpheres(glm::vec3 newPositions) {
	// végigiterálunk az összes eddig pozíción, megnézzük, hogy
	// bármelyikkel ütküzik-e az újonnan felvenni kívánt gömbünk,
	// erre az alábbi algoritmust használjuk
	for (auto sphere : m_newPositionVector) {
		float distance = std::sqrt(
			std::pow(newPositions.x - sphere.x, 2) +
			std::pow(newPositions.y - sphere.y, 2) +
			std::pow(newPositions.z - sphere.z, 2)
		);

		if (distance <= 2 * m_sphereRadius) {
			return true;
		}
	}

	return false;
}

void CMyApp::TeleportToNextObject() {
	// megnézzük, hogy van-e hova teleportálnunk
	if (m_nextPosition != -1) {
		glm::vec3 nextPositionCoordinates = m_newPositionVector[m_nextPosition]; // következő gömb pozíciója, ettől sugár távolságra helyezzük el a kamerát
		m_camera.SetView(glm::vec3(nextPositionCoordinates.x, nextPositionCoordinates.y, nextPositionCoordinates.z - m_radius),
					     nextPositionCoordinates,
					     glm::vec3(0.0, 1.0, 0.0));

		// ha van következő objektumunk, akkor beállítjuk annak az indexét mint next
		if (m_newPositionVector.size() > m_nextPosition + 1) {
			++m_nextPosition;
		}
	}
}

GLint CMyApp::ul( const char* uniformName ) noexcept
{
	GLuint programID = 0;

	// Kérdezzük le az aktuális programot!
	// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGet.xhtml
	glGetIntegerv( GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>( &programID ) );
	// A program és a uniform név ismeretében kérdezzük le a location-t!
	// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetUniformLocation.xhtml
	return glGetUniformLocation( programID, uniformName );
}

// https://wiki.libsdl.org/SDL2/SDL_KeyboardEvent
// https://wiki.libsdl.org/SDL2/SDL_Keysym
// https://wiki.libsdl.org/SDL2/SDL_Keycode
// https://wiki.libsdl.org/SDL2/SDL_Keymod

void CMyApp::KeyboardDown(const SDL_KeyboardEvent& key)
{	
	if ( key.repeat == 0 ) // Először lett megnyomva
	{
		if ( key.keysym.sym == SDLK_F5 && key.keysym.mod & KMOD_CTRL )
		{
			CleanShaders();
			InitShaders();
		}
		if ( key.keysym.sym == SDLK_F1 )
		{
			GLint polygonModeFrontAndBack[ 2 ] = {};
			// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGet.xhtml
			glGetIntegerv( GL_POLYGON_MODE, polygonModeFrontAndBack ); // Kérdezzük le a jelenlegi polygon módot! Külön adja a front és back módokat.
			GLenum polygonMode = ( polygonModeFrontAndBack[ 0 ] != GL_FILL ? GL_FILL : GL_LINE ); // Váltogassuk FILL és LINE között!
			// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glPolygonMode.xhtml
			glPolygonMode( GL_FRONT_AND_BACK, polygonMode ); // Állítsuk be az újat!
		}
		if (key.keysym.sym == SDLK_SPACE) {
			TeleportToNextObject();
		}
	}
	m_camera.KeyboardDown( key );
}

void CMyApp::KeyboardUp(const SDL_KeyboardEvent& key)
{
	m_camera.KeyboardUp( key );
}

// https://wiki.libsdl.org/SDL2/SDL_MouseMotionEvent

void CMyApp::MouseMove(const SDL_MouseMotionEvent& mouse)
{
	m_camera.MouseMove( mouse );
}

// https://wiki.libsdl.org/SDL2/SDL_MouseButtonEvent

void CMyApp::MouseDown(const SDL_MouseButtonEvent& mouse)
{
}

void CMyApp::MouseUp(const SDL_MouseButtonEvent& mouse)
{
}

// https://wiki.libsdl.org/SDL2/SDL_MouseWheelEvent

void CMyApp::MouseWheel(const SDL_MouseWheelEvent& wheel)
{
	m_camera.MouseWheel( wheel );
}


// a két paraméterben az új ablakméret szélessége (_w) és magassága (_h) található
void CMyApp::Resize(int _w, int _h)
{
	glViewport(0, 0, _w, _h);
	m_camera.Resize( _w, _h );
}

