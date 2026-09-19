#include "renderer/shader.h"

bool Shader::Compile(ShaderType type, std::string_view shaderFilepath)
{
	// Report compilation
    AmoLogger_Log("Compiling shader: %s", shaderFilepath.data());

	// Read the shader source code from the file
	std::ifstream shader_file(shaderFilepath.data(), std::ios::in | std::ios::binary);
	std::stringstream shader_stream;
	std::string shader_source_code;

	if( shader_file )
	{
		shader_stream << shader_file.rdbuf();
		shader_source_code = shader_stream.str();
	}
	else
        AmoLogger_Error("Could not open file: %s",  shaderFilepath.data());


	GLenum shaderType = toGlShaderType(type);
	if (shaderType == GL_INVALID_ENUM) {
		AmoLogger_Error("ShaderType is unknown\n");
		return false;
	}

	// Create an empty shader handle
	shaderId = glCreateShader(shaderType);

	// Send the shader source code to GL, and Compile the shader
	const char* source_c_str = shader_source_code.c_str();
	glShaderSource(shaderId, 1, &source_c_str, nullptr);
	glCompileShader(shaderId);


	// Check if the compilation succeeded
	GLint isCompiled = 0;
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		// If compilation failed, find out why and log the error
		GLint maxLength = 0;
		glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		std::vector<GLchar> infoLog(maxLength);
		glGetShaderInfoLog(shaderId, maxLength, &maxLength, &infoLog[0]);

		// We don't need the shader anymore if compilation failed
		glDeleteShader(shaderId);

		AmoLogger_Error("Shader Compilation failed: \n%s", infoLog.data());

		shaderId = UINT32_MAX;
		return false;
	}

	return true;
}

void Shader::Destroy()
{
	if (shaderId != UINT32_MAX)
	{
		glDeleteShader(shaderId);
		shaderId = UINT32_MAX;
	}
}


GLenum Shader::toGlShaderType(ShaderType type)
{
	switch (type)
	{
	case ShaderType::Vertex:
		return GL_VERTEX_SHADER;
	case ShaderType::Fragment:
		return GL_FRAGMENT_SHADER;
	}
	return GL_INVALID_ENUM;
}
