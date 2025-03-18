/**
* Author: Yujia Hou
* Assignment: Lunar Lander
* Date due: 2025-3-15, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define PLATFORM_COUNT 12

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include <cstdlib>
#include "Entity.h"

// ––––– STRUCTS AND ENUMS ––––– //
struct GameState
{
    Entity* player;
    Entity* platforms;
};

// ––––– CONSTANTS ––––– //
constexpr int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480;

constexpr float BG_RED     = 1.0f,
            BG_BLUE    = 1.0f,
            BG_GREEN   = 1.0f,
            BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;
constexpr char FONT_FILEPATH[] = "assets/font1.png";
constexpr char SPRITESHEET_FILEPATH[] = "assets/Pixel_Sprite.jpeg";//https://www.pinterest.com/pin/471963235942128810/
constexpr char PLATFORM_FILEPATH[]    = "assets/platform_tile_032.png";
constexpr char PLATFORM_2_FILEPATH[]    = "assets/platform_tile_005.png";
//https://opengameart.org/content/platform-tile-set-free

constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL  = 0;
constexpr GLint TEXTURE_BORDER   = 0;

glm::vec3 g_velocity = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_gravity = glm::vec3(0.0f, -0.45f, 0.0f);


// ––––– GLOBAL VARIABLES ––––– //
GameState g_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

constexpr int FONTBANK_SIZE = 16;
GLuint font_texture_id;

bool win = false;
bool lose = false;
float timer = 0.0f;
void draw_text(ShaderProgram *program, GLuint font_texture_id, std::string text,
               float font_size, float spacing, glm::vec3 position)
{

    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;


    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    for (int i = 0; i < text.size(); i++) {

        int spritesheet_index = (int) text[i];
        float offset = (font_size + spacing) * i;
        
        float u_coordinate = (float) (spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float) (spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        vertices.insert(vertices.end(), {
            offset + (-0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
        });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
        });
    }

    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);
    
    program->set_model_matrix(model_matrix);
    glUseProgram(program->get_program_id());
    
    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0,
                          vertices.data());
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0,
                          texture_coordinates.data());
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());
    
    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int) (text.size() * 6));
    
    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}






// ––––– GENERAL FUNCTIONS ––––– //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("Hello, Parachuting",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    font_texture_id = load_texture(FONT_FILEPATH);
    // ––––– VIDEO ––––– //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_program.set_projection_matrix(g_projection_matrix);
    g_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    // ––––– PLATFORMS ––––– //
    GLuint platform_texture_id_1 = load_texture(PLATFORM_FILEPATH);
    GLuint platform_texture_id_2 = load_texture(PLATFORM_2_FILEPATH);

    g_state.platforms = new Entity[PLATFORM_COUNT];

    // Set the type of every platform entity to PLATFORM
    for (int i = 0; i < PLATFORM_COUNT; i++)
    {
        if (i <= 10){
            g_state.platforms[i].set_texture_id(platform_texture_id_1);
            g_state.platforms[i].set_position(glm::vec3(i - PLATFORM_COUNT / 2.0f, -3.0f, 0.0f));
            g_state.platforms[i].set_width(0.8f);
            g_state.platforms[i].set_height(1.0f);
            g_state.platforms[i].set_entity_type(PLATFORM);
            g_state.platforms[i].update(0.0f, NULL, NULL, 0);
        }
        else if (i==11){
            g_state.platforms[i].set_texture_id(platform_texture_id_2);
            g_state.platforms[i].set_position(glm::vec3(i - PLATFORM_COUNT / 2.0f, -3.0f, 0.0f));
            g_state.platforms[i].set_velocity(glm::vec3(3.0f, 0.0f, 0.0f));
            LOG(g_state.platforms[i].get_velocity().x);
            g_state.platforms[i].set_width(0.8f);
            g_state.platforms[i].set_height(1.0f);
            g_state.platforms[i].set_entity_type(PLATFORM);
            g_state.platforms[i].update(0.0f, NULL, NULL, 0);
        }
    }

    g_state.platforms[11].set_velocity(glm::vec3(3.0f, 0.0f, 0.0f));
    GLuint player_texture_id = load_texture(SPRITESHEET_FILEPATH);

    int player_walking_animation[4][4] =
    {
        { 8, 9, 10, 11 },
        { 4, 5, 6, 7 },
        { 12, 13, 14, 15 },
        { 0, 1, 2, 3 }
    };

    glm::vec3 g_gravity = glm::vec3(0.0f,-0.45f, 0.0f);

    g_state.player = new Entity(
        player_texture_id,         // texture id
        2.0f,                      // speed
        500.0f,                      // fuel
        g_gravity,              // acceleration
        3.0f,                      // jumping power
        player_walking_animation,  // animation index sets
        0.0f,                      // animation time
        4,                         // animation frame amount
        0,                         // current animation index
        4,                         // animation column amount
        4,                         // animation row amount
        1.0f,                      // width
        1.0f,                       // height
        PLAYER
    );
    
    g_state.player->set_fuel(500.0f);


    // Jumping
    g_state.player->set_jumping_power(3.0f);
    g_state.player->set_position(glm::vec3(-3.0f,4.0f, 0.0f));

    // ––––– GENERAL ––––– //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_state.player->set_movement(glm::vec3(0.0f));

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_game_is_running = false;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        // Quit the game with a keystroke
                        g_game_is_running = false;
                        break;

                    default:
                        break;
                }

            default:
                break;
        }
    }

    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    if (g_state.player->get_fuel()>0.0f){
        if (key_state[SDL_SCANCODE_LEFT])
        {
            g_state.player->move_left();
            glm::vec3 acceleration = g_state.player->get_acceleration();
            float new_acceleration_x = g_state.player->get_acceleration().x -1.0f;
            acceleration.x = new_acceleration_x;
            g_state.player->set_acceleration(acceleration);
            float curr_fuel = g_state.player->get_fuel();
            curr_fuel -= 1.0f;
            g_state.player->set_fuel(curr_fuel);
        }
        else if (key_state[SDL_SCANCODE_RIGHT])
        {
            g_state.player->move_right();
            glm::vec3 acceleration = g_state.player->get_acceleration();
            float new_acceleration_x = g_state.player->get_acceleration().x + 1.0f;
            acceleration.x = new_acceleration_x;
            g_state.player->set_acceleration(acceleration);
            float curr_fuel = g_state.player->get_fuel();
            curr_fuel -= 1.0f;
            g_state.player->set_fuel(curr_fuel);
        }
        else{
            if (win==true || lose == true){
                g_state.player->set_velocity(glm::vec3(0.0f, 0.0f, 0.0f));
                g_state.player->set_acceleration(glm::vec3(0.0f, 0.0f, 0.0f));
            }
            else{
                glm::vec3 current_velocity = g_state.player->get_velocity();
                float new_velocity_x = current_velocity.x * 0.9f;  // Slow down velocity
                current_velocity.x=new_velocity_x;
                g_state.player->set_velocity(current_velocity);  // Update the velocity
            }
        }
    }

    if (glm::length(g_state.player->get_movement()) > 1.0f)
    {
        g_state.player->normalise_movement();
    }
    
    if (win==true||lose==true){
        timer += 1.0f;
        if (timer > 30.0f){
            SDL_Quit();
            exit(1);
        }
    }
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_accumulator;

    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }

    while (delta_time >= FIXED_TIMESTEP)
    {
        g_state.player->update(FIXED_TIMESTEP, NULL, g_state.platforms, PLATFORM_COUNT);
        delta_time -= FIXED_TIMESTEP;
    }

    g_accumulator = delta_time;
    
    glm::vec3 new_acceleration = g_state.player->get_acceleration();
    float new_acceleration_y = g_state.player->get_acceleration().y + g_gravity.y * delta_time;
    new_acceleration.y = new_acceleration_y;
    g_state.player->set_acceleration(new_acceleration);
    
    
    glm::vec3 new_velocity = g_state.player->get_velocity();
    float new_velocity_y = g_state.player->get_velocity().y + g_state.player->get_acceleration().y * delta_time;
    new_velocity.y = new_velocity_y;
    g_state.player->set_velocity(new_velocity);
    
    
    glm::vec3 new_position = g_state.player->get_position();
    float new_position_y = g_state.player->get_position().y + g_state.player->get_velocity().y * delta_time;
    new_position.y = new_position_y;
    g_state.player->set_position(new_position);
    
    glm::vec3 curr_velocity = g_state.platforms[11].get_velocity();
    if (g_state.platforms[11].get_position().x >= 5.0f || g_state.platforms[11].get_position().x <= -5.0) {
        float curr_velocity_x = -g_state.platforms[11].get_velocity().x;
        curr_velocity.x = curr_velocity_x;
        g_state.platforms[11].set_velocity(curr_velocity);
    }
    if (win==false&&lose==false){
        glm::vec3 curr_position = g_state.platforms[11].get_position();
        curr_position.x += curr_velocity.x * delta_time;
        g_state.platforms[11].set_position(curr_position);
    }
    
    for (int i = 0; i < PLATFORM_COUNT; i++){
        if (g_state.player->check_collision(&g_state.platforms[11])){
            win=true;
        }
        else if (i<11 && g_state.player->check_collision(&g_state.platforms[i])){
            lose=true;
        }
    }
    
    if (win==true || lose == true){
        g_state.player->set_velocity(glm::vec3(0.0f, 0.0f, 0.0f));
        g_state.player->set_acceleration(glm::vec3(0.0f, 0.0f, 0.0f));
    }
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    g_state.player->render(&g_program);
    g_state.platforms[11].render(&g_program);


    for (int i = 0; i < PLATFORM_COUNT; i++) g_state.platforms[i].render(&g_program);

    if (win){
        draw_text(&g_program, font_texture_id, "Mission completed!", 0.5f, 0.05f, glm::vec3(-4.5f, 2.0f, 0.0f));
    }
    else if (lose){
        draw_text(&g_program, font_texture_id, "Mission failed!", 0.5f, 0.05f, glm::vec3(-3.8f, 2.0f, 0.0f));
    }
    else{
        std::string fuel_text = "Fuel: " + std::to_string(g_state.player->get_fuel());
        draw_text(&g_program, font_texture_id, fuel_text, 0.5f, 0.05f, glm::vec3(-3.8f, 2.0f, 0.0f));

    }

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();

    delete [] g_state.platforms;
    delete g_state.player;
}

// ––––– GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
