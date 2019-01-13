#include <iostream>
#include <cstring>
#include <fstream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "geometry.h"
#include "model.h"


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (GLFW_RELEASE == action) {
        return;
    }
    std::cerr << key << std::endl;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
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
    is.read (buffer, size);
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
    std::vector<char> v(log_length, 0);
    glGetShaderInfoLog(hdlr, log_length, NULL, v.data());
    if (strlen(v.data())>0) {
        std::cerr << v.data() << std::endl;
    }

    delete [] buffer;
}

void set_shaders(GLuint &prog_hdlr, const char *vsfile, const char *fsfile) {
    GLuint vert_hdlr, frag_hdlr;
    read_n_compile_shader(vsfile, vert_hdlr, GL_VERTEX_SHADER);
    read_n_compile_shader(fsfile, frag_hdlr, GL_FRAGMENT_SHADER);

    prog_hdlr = glCreateProgram();
    glAttachShader(prog_hdlr, vert_hdlr);
    glAttachShader(prog_hdlr, frag_hdlr);
    glDeleteShader(vert_hdlr);
    glDeleteShader(frag_hdlr);

    glLinkProgram(prog_hdlr);

    std::cerr << "Linking shaders... ";

    GLint success;
    glGetProgramiv(prog_hdlr, GL_LINK_STATUS, &success);
    std::cerr << (success ? "ok" : "failed") << std::endl;

    GLint log_length;
    glGetProgramiv(prog_hdlr, GL_INFO_LOG_LENGTH, &log_length);
    std::vector<char> v(log_length);
    glGetProgramInfoLog(prog_hdlr, log_length, NULL, v.data());
    if (strlen(v.data())>0) {
        std::cerr << v.data() << std::endl;
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
    if (argc<2) {
        std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        return -1;
    }
    Model model(argv[1]);

    const GLuint width = 1024, height = 768;
    GLFWwindow* window;
    if (setup_window(window, width, height)) {
        glfwTerminate();
        return -1;
    }

    GLuint prog_hdlr;
    set_shaders(prog_hdlr, "../shaders/vertex.glsl", "../shaders/fragment.glsl");

    std::vector<GLfloat> vertices(3*model.nverts());
    for (int i=0; i<model.nverts(); i++) {
        for (int j=0; j<3; j++) {
            vertices[i*3+j] = model.point(i)[j]/1000.;
        }
    }
    std::vector<GLuint> indices(3*model.nfaces());
    for (int i=0; i<model.nfaces(); i++) {
         for (int j=0; j<3; j++) {
            indices[i*3+j] = model.vert(i, j);
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

    GLuint elementbuffer = 0;
    glGenBuffers(1, &elementbuffer);                      // allocate and assign one Element Buffer Object to our handle
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer); // bind our VBO as being the active buffer and storing indices
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW); // copy the indices to our buffer. The buffer contains sizeof(GLuint) * 3 * ntriangles bytes

    glViewport(0, 0, width, height);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);   // accept fragment if it closer to the camera than the former one
    glEnable(GL_CULL_FACE); // cull triangles whose normal is not towards the camera

    while (!glfwWindowShouldClose(window)) {
        glUseProgram(prog_hdlr);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 1st attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

        // draw the triangles!
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, (GLvoid*)0);

        glDisableVertexAttribArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // properly de-allocate all the resources once they have outlived their purpose
    glUseProgram(0);
    glDeleteProgram(prog_hdlr); // note that the shader objects are automatically detached and deleted, since they were flagged for deletion by a previous call to glDeleteShader
    glDisableVertexAttribArray(0);
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &elementbuffer);
    glDeleteVertexArrays(1, &vao);

    glfwTerminate();
    return 0;
}

