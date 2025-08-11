#include "Shader.hpp"
#include <GL/glew.h>
#include <fstream>
#include <sstream>
#include <iostream>

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
    std::string vertexCode, fragmentCode;
    std::ifstream vFile(vertexPath), fFile(fragmentPath);
    std::stringstream vStream, fStream;

    vStream << vFile.rdbuf();
    fStream << fFile.rdbuf();
    vertexCode = vStream.str();
    fragmentCode = fStream.str();

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);

    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    int success;
    char infoLog[512];

    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cout << "Vertex Shader Compilation Failed:\n" << infoLog << std::endl;
}
}
auto checkCompile = [](GLuint s, const char* name){
    GLint ok=0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if(!ok){
        GLint len=0; glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetShaderInfoLog(s, len, nullptr, log.data());
        std::cerr << "[SHADER COMPILE ERROR] " << name << "\n" << log << "\n";
    }
};
auto checkLink = [](GLuint p){
    GLint ok=0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if(!ok){
        GLint len=0; glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetProgramInfoLog(p, len, nullptr, log.data());
        std::cerr << "[PROGRAM LINK ERROR]\n" << log << "\n";
    }
};

void Shader::use() const { glUseProgram(ID); }
void Shader::setBool(const std::string &name, bool val) const { glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)val); }
void Shader::setInt(const std::string &name, int val) const { glUniform1i(glGetUniformLocation(ID, name.c_str()), val); }
void Shader::setFloat(const std::string &name, float val) const { glUniform1f(glGetUniformLocation(ID, name.c_str()), val); }
void Shader::setVec3(const std::string &name, const glm::vec3 &v) const { glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &v[0]); }
void Shader::setMat4(const std::string &name, const glm::mat4 &m) const { glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &m[0][0]); }