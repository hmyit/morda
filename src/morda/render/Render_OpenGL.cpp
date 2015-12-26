#if M_MORDA_RENDER == M_MORDA_RENDER_OPENGLES
#	if M_OS_NAME == M_OS_NAME_IOS
#		include <OpenGLES/ES2/glext.h>
#	else
#		include <GLES2/gl2.h>
#	endif
#else
#	include <GL/glew.h>
#endif

#if M_OS == M_OS_WINDOWS
#	include <utki/windows.hpp>
#endif

#include <memory>
#include <sstream>

#include <utki/Exc.hpp>
#include <utki/Void.hpp>
#include <utki/PoolStored.hpp>
#include <utki/Buf.hpp>

#include "../Exc.hpp"
#include "../App.hpp"

using namespace morda;

namespace{



inline static void AssertOpenGLNoError(){
#ifdef DEBUG
	GLenum error = glGetError();
	switch(error){
		case GL_NO_ERROR:
			return;
		case GL_INVALID_ENUM:
			ASSERT_INFO(false, "OpenGL error: GL_INVALID_ENUM")
			break;
		case GL_INVALID_VALUE:
			ASSERT_INFO(false, "OpenGL error: GL_INVALID_VALUE")
			break;
		case GL_INVALID_OPERATION:
			ASSERT_INFO(false, "OpenGL error: GL_INVALID_OPERATION")
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			ASSERT_INFO(false, "OpenGL error: GL_INVALID_FRAMEBUFFER_OPERATION")
			break;
		case GL_OUT_OF_MEMORY:
			ASSERT_INFO(false, "OpenGL error: GL_OUT_OF_MEMORY")
			break;
		default:
			ASSERT_INFO(false, "Unknown OpenGL error, code = " << int(error))
			break;
	}
#endif
}
	



GLenum modeMap[] = {
	GL_TRIANGLES,			//TRIANGLES
	GL_TRIANGLE_FAN,		//TRIANGLE_FAN
	GL_LINE_LOOP			//LINE_LOOP
};



const char* shaderDefs =
#if M_MORDA_RENDER == M_MORDA_RENDER_OPENGLES
R"qwertyuiop(
		#define MAT4F highp mat4
		#define VEC2F highp vec2
		#define VEC4F highp vec4
	)qwertyuiop"
#else
R"qwertyuiop(
		#define MAT4F mat4
		#define VEC2F vec2
		#define VEC4F vec4
	)qwertyuiop"
#endif

	R"qwertyuiop(
		#define UNIFORM(type, name) uniform type name;

		#define UNIFORM_BEGIN
		#define UNIFORM_END

		#define ATTRIB(type, name) attribute type name;

		#define ATTRIB_BEGIN
		#define ATTRIB_END

		#define VARYING(type, name) varying type name;

		#define VARYING_BEGIN
		#define VARYING_END

		#define VERTEX_MAIN_BEGIN void main(void){

		#define VERTEX_MAIN_END }

		#define FRAG_MAIN_BEGIN void main(void){
		
		#define FRAG_MAIN_END }

		#define OUT(varying_name) varying_name

		#define VARYING_POS

		#define OUT_POS gl_Position
		#define OUT_FRAGCOLOR gl_FragColor

		#define TEXTURE2D(name) sampler2D name;
	)qwertyuiop";


struct ShaderWrapper{
	GLuint s;
	ShaderWrapper(const char* code, GLenum type){
		this->s = glCreateShader(type);
	
		if(this->s == 0){
			throw utki::Exc("glCreateShader() failed");
		}

		std::stringstream ss;
		ss << shaderDefs << code;
		auto cstr = ss.str();
		const char* c = cstr.c_str();

		glShaderSource(this->s, 1, &c, 0);
		glCompileShader(this->s);
		if(this->CheckForCompileErrors(this->s)){
			TRACE(<< "Error while compiling:\n" << c << std::endl)
			glDeleteShader(this->s);
			throw utki::Exc("Error compiling shader");
		}
	}
	~ShaderWrapper()noexcept{
		glDeleteShader(this->s);
	}

	//return true if not compiled
	static bool CheckForCompileErrors(GLuint shader){
		GLint value = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &value);
		if(value == 0){ //if not compiled
			GLint logLen = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
			if(logLen > 1){//1 char is a terminating 0
				std::vector<char> log(logLen);
				GLint len;
				glGetShaderInfoLog(shader, GLsizei(log.size()), &len, &*log.begin());
				TRACE(<< "===Compile log===\n" << &*log.begin() << std::endl)
			}else{
				TRACE(<< "Shader compile log is empty" << std::endl)
			}
			return true;
		}
		return false;
	}
};


struct ProgramWrapper : public utki::Void{
	ShaderWrapper vertexShader;
	ShaderWrapper fragmentShader;
	GLuint p;
	ProgramWrapper(const char* vertexShaderCode, const char* fragmentShaderCode) :
			vertexShader(vertexShaderCode, GL_VERTEX_SHADER),
			fragmentShader(fragmentShaderCode, GL_FRAGMENT_SHADER)
	{
		this->p = glCreateProgram();
		glAttachShader(this->p, vertexShader.s);
		glAttachShader(this->p, fragmentShader.s);
		glLinkProgram(this->p);
		if(this->CheckForLinkErrors(this->p)){
			TRACE(<< "Error while linking shader program" << vertexShaderCode << std::endl << fragmentShaderCode << std::endl)
			glDeleteProgram(this->p);
			throw utki::Exc("Error linking shader program");
		}
	}

	virtual ~ProgramWrapper()noexcept{
		glDeleteProgram(this->p);
	}

	//return true if not linked
	static bool CheckForLinkErrors(GLuint program){
		GLint value = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &value);
		if(value == 0){ //if not linked
			GLint logLen = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
			if(logLen > 1){ //1 is for terminating 0 character.
				std::vector<char> log(logLen);
				GLint len;
				glGetProgramInfoLog(program, GLsizei(log.size()), &len, &*log.begin());
				TRACE(<< "===Link log===\n" << &*log.begin() << std::endl)
			}
			return true;
		}
		return false;
	}
};

}



void Render::renderArrays(EMode mode, size_t numElements) {
	GLenum m = modeMap[unsigned(mode)];
	
	glDrawArrays(m, 0, GLsizei(numElements));
	AssertOpenGLNoError();
}



void Render::renderElements(EMode mode, const utki::Buf<std::uint16_t>& i) {
	GLenum m = modeMap[unsigned(mode)];
	
	glDrawElements(m, GLsizei(i.size()), GL_UNSIGNED_SHORT, &*i.begin());
	AssertOpenGLNoError();
}

void Render::bindShader(utki::Void& p) {
	glUseProgram(static_cast<ProgramWrapper&>(p).p);
	AssertOpenGLNoError();
}

std::unique_ptr<utki::Void> Render::compileShader(const char* vertexShaderCode, const char* fragmentShaderCode) {
	return std::unique_ptr<ProgramWrapper>(new ProgramWrapper(vertexShaderCode, fragmentShaderCode));
}


Render::InputID Render::getAttribute(utki::Void& p, const char* n) {
	GLint ret = glGetAttribLocation(static_cast<ProgramWrapper&>(p).p, n);
	if(ret < 0){
		std::stringstream ss;
		ss << "No attribute found in the shader program: " << n;
		throw utki::Exc(ss.str());
	}
	return InputID(ret);
}

Render::InputID Render::getUniform(utki::Void& p, const char* n) {
	GLint ret = glGetUniformLocation(static_cast<ProgramWrapper&>(p).p, n);
	if(ret < 0){
		throw utki::Exc("No uniform found in the shader program");
	}
	return InputID(ret);
}

void Render::setUniformMatrix4f(InputID id, const kolme::Matr4f& m) {
	glUniformMatrix4fv(GLint(id.id), 1, GL_FALSE, reinterpret_cast<const GLfloat*>(&m));
	AssertOpenGLNoError();
}

void Render::setUniform1i(InputID id, int i) {
	glUniform1i(GLint(id.id), i);
	AssertOpenGLNoError();
}

void Render::setUniform2f(InputID id, kolme::Vec2f v) {
	glUniform2f(GLint(id.id), v.x, v.y);
	AssertOpenGLNoError();
}

void Render::setUniform4f(InputID id, float x, float y, float z, float a) {
	glUniform4f(GLint(id.id), x, y, z, a);
	AssertOpenGLNoError();
}

void Render::setUniform4f(InputID id, const utki::Buf<kolme::Vec4f> v) {
	static_assert(sizeof(v[0]) == sizeof(GLfloat) * 4, "size mismatch");
	glUniform4fv(GLint(id.id), GLsizei(v.size()), reinterpret_cast<const GLfloat*>(&*v.begin()));
	AssertOpenGLNoError();
}

void Render::setVertexAttribArray(InputID id, const kolme::Vec3f* a) {
	glEnableVertexAttribArray(GLint(id.id));
	AssertOpenGLNoError();
	ASSERT(a)
	glVertexAttribPointer(GLint(id.id), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<const GLfloat*>(a));
	AssertOpenGLNoError();
}

void Render::setVertexAttribArray(InputID id, const kolme::Vec2f* a) {
	glEnableVertexAttribArray(GLint(id.id));
	AssertOpenGLNoError();
	ASSERT(a)
	glVertexAttribPointer(GLint(id.id), 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<const GLfloat*>(a));
	AssertOpenGLNoError();
}

void Render::setViewport(kolme::Recti r){
	glViewport(r.p.x, r.p.y, r.d.x, r.d.y);
	AssertOpenGLNoError();
}

kolme::Recti Render::getViewport() {
	GLint vp[4];

	glGetIntegerv(GL_VIEWPORT, vp);
	
	return kolme::Recti(vp[0], vp[1], vp[2], vp[3]);
}



namespace {

#if M_OS == M_OS_WINDOWS

struct OpenGLContext : public utki::Void{
	HGLRC hrc;
public:
	OpenGLContext(HDC hdc) {
		//	TRACE_AND_LOG(<< "App::GLContextWrapper::GLContextWrapper(): enter" << std::endl)

		this->hrc = wglCreateContext(hdc);
		if (!this->hrc) {
			throw morda::Exc("Failed to create OpenGL rendering context");
		}

		//	TRACE_AND_LOG(<< "App::GLContextWrapper::GLContextWrapper(): GL rendering context created" << std::endl)

		if (!wglMakeCurrent(hdc, this->hrc)) {
			this->Destroy();
			throw morda::Exc("Failed to activate OpenGL rendering context");
		}

		//	TRACE_AND_LOG(<< "App::GLContextWrapper::GLContextWrapper(): GL rendering context created" << std::endl)
	}

	~OpenGLContext()noexcept {
		this->Destroy();
	}

	void Destroy() {
		if (!wglMakeCurrent(NULL, NULL)) {
			ASSERT_INFO(false, "Deactivating OpenGL rendering context failed")
		}

		if (!wglDeleteContext(this->hrc)) {
			ASSERT_INFO(false, "Releasing OpenGL rendering context failed")
		}
	}
};

#else

#endif

}//~namespace

Render::Render() :
		pimpl(
#if M_OS == M_OS_WINDOWS
				new OpenGLContext(morda::App::inst().deviceContext.hdc)
#endif
			)
{
#if M_MORDA_RENDER == M_MORDA_RENDER_OPENGL
	if(glewInit() != GLEW_OK){
		throw morda::Exc("GLEW initialization failed");
	}
#endif
	AssertOpenGLNoError();
	//TODO: uncomment
	TRACE(<< "OpenGL version: " << glGetString(GL_VERSION) << std::endl)
}

Render::~Render()noexcept {

}

void Render::clearColor(kolme::Vec4f c) {
	glClearColor(c.x, c.y, c.z, c.w);
	AssertOpenGLNoError();
	glClear(GL_COLOR_BUFFER_BIT);
	AssertOpenGLNoError();
}

void Render::clearDepth(float d) {
#if M_OS_NAME == M_OS_NAME_IOS
	glClearDepthf(d);
#else
	glClearDepth(d);
#endif
	glClear(GL_DEPTH_BUFFER_BIT);
	AssertOpenGLNoError();
}


bool Render::isScissorEnabled() {
	return glIsEnabled(GL_SCISSOR_TEST) ? true : false; //?true:false is to avoid warning under MSVC
}

kolme::Recti Render::getScissorRect() {
	GLint osb[4];
	glGetIntegerv(GL_SCISSOR_BOX, osb);
	return kolme::Recti(osb[0], osb[1], osb[2], osb[3]);
}

void Render::setScissorEnabled(bool enabled) {
	if(enabled){
		glEnable(GL_SCISSOR_TEST);
	}else{
		glDisable(GL_SCISSOR_TEST);
	}
}

void Render::setScissorRect(kolme::Recti r) {
	glScissor(r.p.x, r.p.y, r.d.x, r.d.y);
	AssertOpenGLNoError();
}


namespace{

GLint texFilterMap[] = {
	GL_NEAREST,
	GL_LINEAR
};

struct GLTexture2D : public utki::Void, public utki::PoolStored<GLTexture2D, 32>{
	GLuint tex;
	
	GLTexture2D(){
		glGenTextures(1, &this->tex);
		AssertOpenGLNoError();
		ASSERT(this->tex != 0)
	}
	
	virtual ~GLTexture2D()noexcept{
		glDeleteTextures(1, &this->tex);
	}
	
	void bind(unsigned unitNum){
		glActiveTexture(GL_TEXTURE0 + unitNum);
		AssertOpenGLNoError();
		glBindTexture(GL_TEXTURE_2D, this->tex);
		AssertOpenGLNoError();
	}
};

}//~namespace

std::unique_ptr<utki::Void> Render::create2DTexture(kolme::Vec2ui dim, unsigned numChannels, const utki::Buf<std::uint8_t> data, ETexFilter minFilter, ETexFilter magFilter){
	ASSERT(data.size() == 0 || data.size() >= dim.x * dim.y * numChannels)
	
	GLint minFilterGL = texFilterMap[unsigned(minFilter)];
	GLint magFilterGL = texFilterMap[unsigned(magFilter)];
	
	std::unique_ptr<GLTexture2D> ret(new GLTexture2D());
	
	ret->bind(0);
	
	GLint internalFormat;
	switch(numChannels){
		default:
			ASSERT(false)
		case 1:
			internalFormat = GL_LUMINANCE;
			break;
		case 2:
			internalFormat = GL_LUMINANCE_ALPHA;
			break;
		case 3:
			internalFormat = GL_RGB;
			break;
		case 4:
			internalFormat = GL_RGBA;
			break;
	}

	//we will be passing pixels to OpenGL which are 1-byte aligned.
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	AssertOpenGLNoError();

	glTexImage2D(
			GL_TEXTURE_2D,
			0,//0th level, no mipmaps
			internalFormat, //internal format
			dim.x,
			dim.y,
			0,//border, should be 0!
			internalFormat, //format of the texel data
			GL_UNSIGNED_BYTE,
			data.size() == 0 ? nullptr : &*data.begin()
		);
	AssertOpenGLNoError();

	//NOTE: on OpenGL ES 2 it is necessary to set the filter parameters
	//      for every texture!!! Otherwise it may not work!
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilterGL);
	AssertOpenGLNoError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilterGL);
	AssertOpenGLNoError();
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	return std::move(ret);
}

void Render::bindTexture(utki::Void& tex, unsigned unitNum){
	static_cast<GLTexture2D&>(tex).bind(unitNum);
}

void Render::unbindTexture(unsigned unitNum){
	glActiveTexture(GL_TEXTURE0 + unitNum);
	AssertOpenGLNoError();
	glBindTexture(GL_TEXTURE_2D, 0);
	AssertOpenGLNoError();
}


void Render::copyColorBufferToTexture(kolme::Vec2i dst, kolme::Recti src){
	glCopyTexSubImage2D(
		GL_TEXTURE_2D,
		0, //level
		dst.x, //xoffset
		dst.y, //yoffset
		src.p.x,
		src.p.y,
		src.d.x,
		src.d.y
	);
	AssertOpenGLNoError();
}

unsigned Render::getMaxTextureSize(){
	GLint val;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &val);
	ASSERT(val > 0)
	return unsigned(val);
}

//TODO: move it to App class?
void Render::swapFrameBuffers() {
#if M_OS == M_OS_WINDOWS
	SwapBuffers(morda::App::inst().deviceContext.hdc);
#elif M_OS == M_OS_LINUX
#	if M_OS_NAME == M_OS_NAME_ANDROID
	eglSwapBuffers(morda::App::inst().eglDisplay.d, morda::App::inst().eglSurface.s);
#	else
	glXSwapBuffers(morda::App::inst().xDisplay.d, morda::App::inst().xWindow.w);
#	endif
#elif M_OS == M_OS_MACOSX
#	if M_OS_NAME == M_OS_NAME_IOS
	ASSERT(false)
#	else
	morda::App::inst().macosx_SwapFrameBuffers();
#	endif
#else
#	error "unknown OS"
#endif
}

void Render::setCullEnabled(bool enable) {
	if(enable){
		glEnable(GL_CULL_FACE);
	}else{
		glDisable(GL_CULL_FACE);
	}
}



namespace{

struct OpenGLFrameBuffer : public utki::Void, public utki::Unique{
	GLuint fbo;

	OpenGLFrameBuffer(){
		glGenFramebuffers(1, &this->fbo);
		AssertOpenGLNoError();
	}
	
	~OpenGLFrameBuffer()noexcept override{
		glDeleteFramebuffers(1, &this->fbo);
		AssertOpenGLNoError();
	}
};

}

std::unique_ptr<utki::Void> Render::createFrameBuffer(){
	return  utki::makeUnique<OpenGLFrameBuffer>();
}

void Render::bindFrameBuffer(utki::Void* fbo){
	if(!fbo){
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		AssertOpenGLNoError();
		return;
	}
	ASSERT(fbo)
	OpenGLFrameBuffer& fb = *static_cast<OpenGLFrameBuffer*>(fbo);
	
	glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);
	AssertOpenGLNoError();
}

void Render::attachColorTexture2DToFrameBuffer(utki::Void* tex){
	if(!tex){
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
		AssertOpenGLNoError();
		return;
	}
	ASSERT(tex)
	GLTexture2D& t = static_cast<GLTexture2D&>(*tex);
	
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.tex, 0);
	AssertOpenGLNoError();
}

void Render::setBlendEnabled(bool enable){
	if(enable){
		glEnable(GL_BLEND);
	}else{
		glDisable(GL_BLEND);
	}
}

namespace{

GLenum blendFunc[] = {
	GL_ONE,
	GL_SRC_ALPHA,
	GL_ONE_MINUS_SRC_ALPHA,
	GL_ONE_MINUS_DST_ALPHA
};

}

void Render::setBlendFunc(EBlendFactor srcClr, EBlendFactor dstClr, EBlendFactor srcAlpha, EBlendFactor dstAlpha) {
	glBlendFuncSeparate(
			blendFunc[unsigned(srcClr)],
			blendFunc[unsigned(dstClr)],
			blendFunc[unsigned(srcAlpha)],
			blendFunc[unsigned(dstAlpha)]
		);
}



bool Render::isBoundFrameBufferComplete(){
	return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}
