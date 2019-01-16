#include <iostream>
#include <cstring>
#include <fstream>
#include <chrono>
#include <thread>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "geometry.h"
#include "model.h"

bool animate = true;

GLuint load_texture(const char * imagepath) {
    printf("Reading image %s\n", imagepath);

    stbi_set_flip_vertically_on_load(1);
    int width, height, bpp;
    unsigned char* rgb = stbi_load( imagepath, &width, &height, &bpp, 3 );

    // Create one OpenGL texture
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,  0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(rgb);
    return textureID;
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (GLFW_RELEASE == action) {
        return;
    }
    std::cerr << key << std::endl;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        animate = !animate;
    }
}

void read_n_compile_shader(const char *filename, GLuint &hdlr, GLenum shaderType) {
    std::cerr << "Loading " << filename << "... ";
    std::ifstream is(filename, std::ios::in|std::ios::binary|std::ios::ate);
    if (!is.is_open()) {
        std::cerr << "failed" << std::endl;
        return;
    }
    std::cerr << "ok" << std::endl;

    long size = is.tellg();
    char *buffer = new char[size+1];
    is.seekg(0, std::ios::beg);
    is.read(buffer, size);
    is.close();
    buffer[size] = 0;

    std::cerr << "Compiling " << filename << "... ";
    hdlr = glCreateShader(shaderType);
    glShaderSource(hdlr, 1, (const GLchar**)&buffer, NULL);
    glCompileShader(hdlr);
    GLint success;
    glGetShaderiv(hdlr, GL_COMPILE_STATUS, &success);
    std::cerr << (success ? "ok" : "failed") << std::endl;

    GLint log_length;
    glGetShaderiv(hdlr, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length>0) {
        std::vector<char> v(log_length, 0);
        glGetShaderInfoLog(hdlr, log_length, NULL, v.data());
        if (strlen(v.data())>0) {
            std::cerr << v.data() << std::endl;
        }
    }

    delete [] buffer;
}

void set_shaders(GLuint &prog_hdlr, const char *vsfile, const char *fsfile) {
    GLuint vert_hdlr, frag_hdlr;
    read_n_compile_shader(vsfile, vert_hdlr, GL_VERTEX_SHADER);
    read_n_compile_shader(fsfile, frag_hdlr, GL_FRAGMENT_SHADER);

    std::cerr << "Linking shaders... ";
    prog_hdlr = glCreateProgram();
    glAttachShader(prog_hdlr, vert_hdlr);
    glAttachShader(prog_hdlr, frag_hdlr);
    glDeleteShader(vert_hdlr);
    glDeleteShader(frag_hdlr);
    glLinkProgram(prog_hdlr);

    GLint success;
    glGetProgramiv(prog_hdlr, GL_LINK_STATUS, &success);
    std::cerr << (success ? "ok" : "failed") << std::endl;

    GLint log_length;
    glGetProgramiv(prog_hdlr, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length>0) {
        std::vector<char> v(log_length);
        glGetProgramInfoLog(prog_hdlr, log_length, NULL, v.data());
        if (strlen(v.data())>0) {
            std::cerr << v.data() << std::endl;
        }
    }
}

int setup_window(GLFWwindow* &window, const GLuint width, const GLuint height) {
    std::cerr << "Starting GLFW context, OpenGL 3.3" << std::endl;
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    window = glfwCreateWindow(width, height, "OpenGL starter pack", NULL, NULL);
    glfwMakeContextCurrent(window);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        return -1;
    }

    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    return 0;
}

int main(int argc, char** argv) {
    std::cout << "Usage: " << argv[0] << " model.obj diffuse.jpg tangentnormals.jpg" << std::endl;
    std::string file_obj("../models/diablo3_pose.obj");
    std::string file_diff("../models/diablo3_pose_diffuse.jpg");
    std::string file_nm("../models/diablo3_pose_nm_tangent.jpg");
    if (4==argc) {
        file_obj  = std::string(argv[1]);
        file_diff = std::string(argv[2]);
        file_nm   = std::string(argv[3]);
    }
    Model model(file_obj.c_str());

    const GLuint width = 800, height = 800;
    GLFWwindow* window;
    if (setup_window(window, width, height)) {
        glfwTerminate();
        return -1;
    }

    GLuint prog_hdlr;
    set_shaders(prog_hdlr, "../shaders/vertex.glsl", "../shaders/fragment.glsl");

    Matrix M = Matrix::identity();
    Matrix V = Matrix::identity();
    Matrix P = Matrix::identity();

    // Get handles to our uniforms
    GLuint MatrixID = glGetUniformLocation(prog_hdlr, "MVP");
    GLuint ViewMatrixID = glGetUniformLocation(prog_hdlr, "V");
    GLuint ModelMatrixID = glGetUniformLocation(prog_hdlr, "M");
    GLuint LightID = glGetUniformLocation(prog_hdlr, "LightPosition_worldspace");
    GLuint Texture0ID  = glGetUniformLocation(prog_hdlr, "diffuse");
    GLuint Texture1ID  = glGetUniformLocation(prog_hdlr, "tangentnm");


    std::vector<GLfloat> vertices(3*3*model.nfaces(), 0);
    std::vector<GLfloat>      uvs(2*3*model.nfaces(), 0);
    std::vector<GLfloat>     normals(3*3*model.nfaces(), 0);
    std::vector<GLfloat>    tangents(3*3*model.nfaces(), 0);
    std::vector<GLfloat>  bitangents(3*3*model.nfaces(), 0);

    for (int i=0; i<model.nfaces(); i++) {
        Vec3f v0 = model.point(model.vert(i, 0));
        Vec3f v1 = model.point(model.vert(i, 1));
        Vec3f v2 = model.point(model.vert(i, 2));

        Vec3f v01 = proj<3>(M*(embed<4>(v1 - v0)));
        Vec3f v02 = proj<3>(M*(embed<4>(v2 - v0)));
        mat<3, 3, float> A;
        A[0] = v01;
        A[1] = v02;
        A[2] = cross(v01, v02).normalize();

        Vec3f tgt   = A.invert() * Vec3f(model.uv(i, 1).x - model.uv(i, 0).x, model.uv(i, 2).x - model.uv(i, 0).x, 0);
        Vec3f bitgt = A.invert() * Vec3f(model.uv(i, 1).y - model.uv(i, 0).y, model.uv(i, 2).y - model.uv(i, 0).y, 0);
        tgt.normalize();
        bitgt.normalize();

        for (int j=0; j<3; j++) {
            for (int k=0; k<2; k++)      uvs[(i*3+j)*2 + k] = model.uv    (i, j)[k];
            for (int k=0; k<3; k++)  normals[(i*3+j)*3 + k] = model.normal(i, j)[k];
            for (int k=0; k<3; k++) vertices[(i*3+j)*3 + k] = model.point(model.vert(i, j))[k];
            for (int k=0; k<3; k++)    tangents[(i*3+j)*3 + k] = tgt[k];
            for (int k=0; k<3; k++)  bitangents[(i*3+j)*3 + k] = bitgt[k];
        }
    }

    // create the VAO that we use when drawing
    GLuint vao = 0;
    glGenVertexArrays(1, &vao); // allocate and assign a Vertex Array Object to our handle
    glBindVertexArray(vao);     // bind our Vertex Array Object as the current used object

    GLuint vertexbuffer = 0;
    glGenBuffers(1, &vertexbuffer);              // allocate and assign one Vertex Buffer Object to our handle
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer); // bind our VBO as being the active buffer and storing vertex attributes (coordinates)
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*vertices.size(), vertices.data(), GL_STATIC_DRAW); // copy the vertex data to our buffer. The buffer contains sizeof(GLfloat) * 3 * nverts bytes

    GLuint uvbuffer;
    glGenBuffers(1, &uvbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
    glBufferData(GL_ARRAY_BUFFER, uvs.size()*sizeof(GLfloat), uvs.data(), GL_STATIC_DRAW);

    GLuint normalbuffer;
    glGenBuffers(1, &normalbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
    glBufferData(GL_ARRAY_BUFFER, normals.size()*sizeof(GLfloat), normals.data(), GL_STATIC_DRAW);

    GLuint tangentbuffer;
    glGenBuffers(1, &tangentbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, tangentbuffer);
    glBufferData(GL_ARRAY_BUFFER, tangents.size() * sizeof(GLfloat), tangents.data(), GL_STATIC_DRAW);

    GLuint bitangentbuffer;
    glGenBuffers(1, &bitangentbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, bitangentbuffer);
    glBufferData(GL_ARRAY_BUFFER, bitangents.size() * sizeof(GLfloat), bitangents.data(), GL_STATIC_DRAW);

    // Load the textures
    GLuint tex_diffuse = load_texture(file_diff.c_str());
    GLuint tex_normals = load_texture(file_nm.c_str());

    glViewport(0, 0, width, height);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glClearDepth(0);
    glDepthFunc(GL_GREATER);   // accept fragment if it is closer to the camera than the former one

    auto start = std::chrono::steady_clock::now();
    while (!glfwWindowShouldClose(window)) {
        auto end = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() < 20) {
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
            continue;
        }
        start = end;
        Matrix R = Matrix::identity();
        R[0][0] = R[2][2] = cos(0.01);
        R[2][0] = sin(0.01);
        R[0][2] = -sin(0.01);
        if (animate) M = R*M;
        glUseProgram(prog_hdlr);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float tmp[16] = {0};
        // Send our transformation to the currently bound shader
        M.export_row_major(tmp);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, tmp);
        V.export_row_major(tmp);
        glUniformMatrix4fv(ViewMatrixID,  1, GL_FALSE, tmp);
        (P*V*M).export_row_major(tmp);
        glUniformMatrix4fv(MatrixID,      1, GL_FALSE, tmp);
        float lightpos[3] = {40, 40, 40};
        glUniform3fv(LightID, 1, lightpos);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_diffuse);
        glUniform1i(Texture0ID, 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex_normals);
        glUniform1i(Texture1ID, 1);

        // 1st attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // 2nd attribute buffer : UVs
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // 3rd attribute buffer : normals
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // 4th attribute buffer : tangents
        glEnableVertexAttribArray(3);
        glBindBuffer(GL_ARRAY_BUFFER, tangentbuffer);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // 5th attribute buffer : bitangents
        glEnableVertexAttribArray(4);
        glBindBuffer(GL_ARRAY_BUFFER, bitangentbuffer);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // draw the triangles!
        glDrawArrays(GL_TRIANGLES, 0, vertices.size());

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(3);
        glDisableVertexAttribArray(4);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // properly de-allocate all the resources once they have outlived their purpose
    glUseProgram(0);
    glDeleteProgram(prog_hdlr); // note that the shader objects are automatically detached and deleted, since they were flagged for deletion by a previous call to glDeleteShader
    glDisableVertexAttribArray(0);
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &uvbuffer);
    glDeleteBuffers(1, &normalbuffer);
    glDeleteBuffers(1, &tangentbuffer);
    glDeleteBuffers(1, &bitangentbuffer);
    glDeleteTextures(1, &tex_diffuse);
    glDeleteTextures(1, &tex_normals);
    glDeleteVertexArrays(1, &vao);

    glfwTerminate();
    return 0;
}

