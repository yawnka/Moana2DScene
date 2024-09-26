/**
* Author:  Yanka Sikder
* Assignment: Simple 2D Scene
* Date due: 2024-10-28, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH  = 640 * 2,
              WINDOW_HEIGHT = 480 * 2;

constexpr float BG_RED     = 0.9765625f,
                BG_GREEN   = 0.97265625f,
                BG_BLUE    = 0.9609375f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X      = 0,
              VIEWPORT_Y      = 0,
              VIEWPORT_WIDTH  = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
                LEVEL_OF_DETAIL    = 0, // mipmap reduction image level
                TEXTURE_BORDER     = 0; // this value MUST be zero

constexpr char MOANA_SPRITE_FILEPATH[]    = "moana.png",
               KAKAMORA_SPRITE_FILEPATH[]    = "coco.png",
               MAUI_SPRITE_FILEPATH[]    = "maui.png",
               BOAT_SPRITE_FILEPATH[]    = "boat.png";
            

constexpr glm::vec3 INIT_SCALE       = glm::vec3(2.0f, 1.98f, 0.0f),
                    BOAT_SCALE       = glm::vec3(3.0f, 2.98f, 0.0f),
                    MOANA_SCALE       = glm::vec3(1.5f, 0.98f, 0.0f),
                    INIT_POS_BOAT    = glm::vec3(-3.0f, 0.0f, 0.0f),
                    INIT_POS_KAKAMORA    = glm::vec3(-2.0f, 0.0f, 0.0f),
                    INIT_POS_MOANA    = glm::vec3(-2.2f, -0.5f, 0.0f),
                    INIT_POS_MAUI    = glm::vec3(-4.0f, -0.5f, 0.0f);

constexpr float ROT_INCREMENT = 1.0f;

// value from notes on Delta Time to get objects to move to the right slowly
float g_triangle_x = 0.0f;

// kakamora translating around boat (origin)
float angle = 0.0f;  // the angle of rotation
float radius = 2.5f;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix,
          g_boat_matrix,
          g_kakamora_matrix,
          g_moana_matrix,
          g_maui_matrix,
          g_projection_matrix;

float g_previous_ticks = 0.0f;

glm::vec3 g_rotation_boat    = glm::vec3(0.0f, 0.0f, 0.0f),
          g_rotation_kakamora    = glm::vec3(0.0f, 0.0f, 0.0f),
          g_rotation_moana    = glm::vec3(0.0f, 0.0f, 0.0f),
          g_rotation_maui    = glm::vec3(0.0f, 0.0f, 0.0f);

GLuint g_boat_texture_id,
       g_kakamora_texture_id,
       g_moana_texture_id,
       g_maui_texture_id;


GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}


void initialise()
{
    // Initialise video and joystick subsystems
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("Hello, Textures!",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_boat_matrix       = glm::mat4(1.0f);
    g_kakamora_matrix       = glm::mat4(1.0f);
    g_moana_matrix       = glm::mat4(1.0f);
    g_maui_matrix       = glm::mat4(1.0f);
    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_boat_texture_id   = load_texture(BOAT_SPRITE_FILEPATH);
    g_kakamora_texture_id   = load_texture(KAKAMORA_SPRITE_FILEPATH);
    g_moana_texture_id   = load_texture(MOANA_SPRITE_FILEPATH);
    g_maui_texture_id   = load_texture(MAUI_SPRITE_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
    }
}


void update()
{
    /* Delta time calculations */
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    g_triangle_x += 0.2f * delta_time;
    
    /* Update the angle to make the kakamora move in a circle */
        angle += ROT_INCREMENT * delta_time;

    /* Game logic */
    g_rotation_boat.y += 0.0 * delta_time;
    g_rotation_kakamora.y += ROT_INCREMENT * delta_time;
    g_rotation_moana.y += ROT_INCREMENT * delta_time;
    g_rotation_maui.y += -1 * ROT_INCREMENT * delta_time;

    /* Model matrix reset */
    g_boat_matrix    = glm::mat4(1.0f);
    g_kakamora_matrix    = glm::mat4(1.0f);
    g_moana_matrix    = glm::mat4(1.0f);
    g_maui_matrix    = glm::mat4(1.0f);

    /* Transformations */
    
    //BOAT
    //g_boat_matrix = glm::translate(g_boat_matrix, INIT_POS_BOAT);
    g_boat_matrix = glm::translate(g_boat_matrix, INIT_POS_BOAT + glm::vec3(g_triangle_x, 0.0f, 0.0f));
    g_boat_matrix = glm::rotate(g_boat_matrix,
                                 g_rotation_boat.y,
                                 glm::vec3(0.0f, 1.0f, 0.0f));
    g_boat_matrix = glm::scale(g_boat_matrix, BOAT_SCALE);
    
    // KAKAMORA
    //calculating position of kakamora (do I need to add delta time here)
    float kakamora_x = (INIT_POS_BOAT.x + g_triangle_x) + radius * cos(angle);
    float kakamora_y = INIT_POS_BOAT.y + radius * sin(angle);
    g_kakamora_matrix = glm::translate(g_kakamora_matrix, glm::vec3(kakamora_x, kakamora_y, 0.0f));
    g_kakamora_matrix = glm::scale(g_kakamora_matrix, INIT_SCALE);
    //g_kakamora_matrix = glm::rotate(g_kakamora_matrix, g_rotation_kakamora.y, glm::vec3(0.0f, 0.0f, 1.0f));

    // MOANA
    //g_moana_matrix = glm::translate(g_moana_matrix, INIT_POS_MOANA);
    g_moana_matrix = glm::translate(g_moana_matrix, INIT_POS_MOANA + glm::vec3(g_triangle_x, 0.0f, 0.0f));
    g_moana_matrix = glm::rotate(g_moana_matrix,
                                 g_rotation_moana.y,
                                 glm::vec3(0.0f, 1.0f, 0.0f));
    g_moana_matrix = glm::scale(g_moana_matrix, MOANA_SCALE);
    
    // MAUI
    //g_maui_matrix = glm::translate(g_maui_matrix, INIT_POS_MAUI);
    g_maui_matrix = glm::translate(g_maui_matrix, INIT_POS_MAUI + glm::vec3(g_triangle_x, 0.0f, 0.0f));
    g_maui_matrix = glm::rotate(g_maui_matrix,
                                 g_rotation_maui.y,
                                 glm::vec3(0.0f, 1.0f, 0.0f));
    g_maui_matrix = glm::scale(g_maui_matrix, INIT_SCALE);
}


void draw_object(glm::mat4 &object_g_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}


void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] =
    {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false,
                          0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
                          false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(g_boat_matrix, g_boat_texture_id);
    draw_object(g_kakamora_matrix, g_kakamora_texture_id);
    draw_object(g_moana_matrix, g_moana_texture_id);
    draw_object(g_maui_matrix, g_maui_texture_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
