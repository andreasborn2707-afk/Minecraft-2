#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>
#include <windows.h>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <map>
#include<FastNoiseLite.h>
#include<ft2build.h>
#include FT_FREETYPE_H  
#include <string>


glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

glm::vec3 forward;
glm::vec3 CamRight;

constexpr int chunksize = 16;
constexpr int chunkHeight = 200;

constexpr int maxchunks = 32;
int half = maxchunks / 2;
bool firstMouse = true;
float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch = 0.0f;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
float fov = 45.0f;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void RenderText(std::string text, float x, float y, float scale, glm::vec3 color);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

const char* vertexShaderSource = "#version 330 core\n"
"layout(location = 0) in vec3 aPos;\n"
"layout(location = 1) in vec2 aTexCoord;\n"

"// Die Uniforms, die wir vom C++ Code füttern\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"

"out vec2 TexCoord;\n"

"void main()\n"
"{\n"
// WICHTIG: Die Matrix-Multiplikation muss in dieser Reihenfolge sein
" gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
" TexCoord = aTexCoord;\n"
"}\n";

const char* fragmentShaderSource =
"#version 330 core\n"
"out vec4 FragColor;\n"

"in vec2 TexCoord;\n"

"uniform sampler2D ourTexture;\n"

"void main()\n"
"{\n"
"FragColor = texture(ourTexture, TexCoord);\n"
"}\n";


const char* TextVertexShader = 
"#version 330 core\n"
"layout(location = 0) in vec4 vertex; \n"
"out vec2 TexCoords; \n"

"uniform mat4 projection; \n"

"void main()\n"
"{\n"
"gl_Position = projection * vec4(vertex.xy, 0.0, 1.0); \n"
"TexCoords = vertex.zw; \n"
"}\n";

const char* TextFragmentShader =

"#version 330 core\n"

"in vec2 TexCoords;\n"

"out vec4 color;\n"



"uniform sampler2D text;\n"

"uniform vec3 textColor;\n"



"void main()\n"

"{\n"

"vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n"

"color = vec4(textColor, 1.0) * sampled;\n"

"}\n";


enum BlockType {
    AIR = 0,
    DIRT = 1,
    GRASS = 2,
    STONE = 3
};


unsigned int CHUNKVAO, CHUNKVBO, CHUNKEBO;

struct Chunk
{
    bool needsUpdate = false;
    glm::ivec3 chunkPos;
    std::vector<glm::vec3> cubePosition;
    unsigned char blocks[chunksize][chunkHeight][chunksize];
    std::vector<unsigned int> finalIndices;
    std::vector<float> verticesData;
    GLuint VAO;
    GLuint EBO;
    GLuint VBO;
    bool isGenerated;
};


struct Character {
    unsigned int TextureID;  // ID handle of the glyph texture
    glm::ivec2   Size;       // Size of glyph
    glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
    unsigned int Advance;    // Offset to advance to next glyph
};

std::map<char, Character> Characters;

bool grounded = false;


Chunk chunks[maxchunks + 1][maxchunks + 1];

glm::ivec3 chunkPositions[maxchunks +1][maxchunks + 1];



FastNoiseLite Heightnoise;



void GenerateChunk(Chunk& chunk, glm::ivec3 chunkPos)
{
    chunk.chunkPos = chunkPos;



    for (int x = 0; x < chunksize; x++) {
        
            for (int z = 0; z < chunksize; z++) {
                
                float globalX = chunkPos.x * chunksize + x;
                float globalZ = chunkPos.z * chunksize + z;
                
                float generativeHeightNoise = Heightnoise.GetNoise(globalX, globalZ);
                int surfaceY = (int)(generativeHeightNoise * 6.0f) + 170;


                

                for (int y = 0; y < chunkHeight; y++)
                {
                    if (y < 100)
                    {


                        float noise3D = Heightnoise.GetNoise(globalX, float(y), globalZ);
                        if (noise3D > 0.3f)
                        {
                            chunk.blocks[x][y][z] = AIR;
                        }
                        else if (y > surfaceY)
                        {

                            chunk.blocks[x][y][z] = AIR;
                        }
                        else if (y == surfaceY)
                        {
                            chunk.blocks[x][y][z] = GRASS;
                        }
                        else if (y >= surfaceY - 4 && y < surfaceY)
                        {
                            chunk.blocks[x][y][z] = DIRT;
                        }
                        else if (y < surfaceY - 4)
                        {
                            chunk.blocks[x][y][z] = STONE;
                        }
                        
                    }
                    else if (y > surfaceY)
                    {
                        
                        chunk.blocks[x][y][z] = AIR;
                    }
                    else if (y == surfaceY)
                    {
                        chunk.blocks[x][y][z] = GRASS;
                    }
                    else if (y >= surfaceY - 4 && y < surfaceY)
                    {
                        chunk.blocks[x][y][z] = DIRT;
                    }
                    else if (y < surfaceY - 4)
                    {
                        chunk.blocks[x][y][z] = STONE;
                    }
                    
                }
            }
        
    }
}






unsigned int TExvertexShader;
unsigned int TExfragmentShader;
unsigned int TEXshaderProgram;
unsigned int TEXVAO, TEXVBO;

void RenderText(unsigned int shader, std::string text, float x, float y, float scale, glm::vec3 color)
{
    // 1. Shader aktivieren
    glUseProgram(shader);
    glUniform3f(glGetUniformLocation(shader, "textColor"), color.x, color.y, color.z);

    // Erstelle die Matrix direkt hier drin, damit wir sicher sind!
    // Nutze 800.0f und 600.0f oder deine SCR_WIDTH/SCR_HEIGHT
    glm::mat4 projection = glm::ortho(0.0f, (float)SCR_WIDTH, 0.0f, (float)SCR_HEIGHT);
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(TEXVAO);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y);

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        // WICHTIG: Hier ch.TextureID nutzen, nicht shader!
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        glBindBuffer(GL_ARRAY_BUFFER, TEXVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        x += (ch.Advance >> 6) * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}











glm::ivec2 getChunkXZ(glm::vec3 cameraPos)
{
    return glm::ivec2(
        floor(cameraPos.x / chunksize),
        floor(cameraPos.z / chunksize)
    );
}


glm::vec3 getChunkCenterXZ(glm::vec3 cameraPos)
{
    glm::ivec2 chunk = getChunkXZ(cameraPos);

    float centerX = chunk.x * chunksize + chunksize * 0.5f;
    float centerZ = chunk.y * chunksize + chunksize * 0.5f;

    return glm::vec3(centerX, 0.0f, centerZ);
}


struct BlockData {
    int faces[6];
};

BlockData blockLibrary[4];

void RegisterBlock(int id, int top, int bottom, int sides1, int side2, int side3, int side4) {
    blockLibrary[id].faces[0] = sides1;  // Front
    blockLibrary[id].faces[1] = side2;  // Back
    blockLibrary[id].faces[2] = side3;  // Left
    blockLibrary[id].faces[3] = side4;  // Right
    blockLibrary[id].faces[4] = bottom; // Bottom
    blockLibrary[id].faces[5] = top;    // Top
}

void RegisterUniformBlock(int id, int textureIndex) {
    for (int i = 0; i < 6; i++) {
        blockLibrary[id].faces[i] = textureIndex;
    }
}

void SetupBlockLibrary() {
    // Gras: Oben Index 0, Unten Index 1, Seiten Index 2
    RegisterBlock(GRASS, 1, 3, 0,0,0,0);

    // Stein: Überall Index 3
    RegisterUniformBlock(STONE, 2);

    // Erde: Überall Index 1
    RegisterUniformBlock(DIRT, 3);
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 6; j++)
        {
            std::cout << blockLibrary[i].faces[j] << std::endl;
        }
    }
}

int getChunkCoord(int pos, int size) {
    if (pos >= 0) return pos / size;
    return (pos - size + 1) / size;
}

glm::ivec2 playerChunk;

bool isOpaqueGlobal(int x, int y, int z, Chunk* c) {

    if (c == nullptr) return true;

    if (!c || y < 0 || y >= chunkHeight) return false;
    return c->blocks[x][y][z] != AIR;
}


Chunk* GetNeighbor(int dx, int dz, glm::ivec3 currentPos) {
    int nx = currentPos.x + dx;
    int nz = currentPos.z + dz;

    int ax = nx % maxchunks; if (ax < 0) ax += maxchunks;
    int az = nz % maxchunks; if (az < 0) az += maxchunks;

    // Prüfen, ob der Chunk im Array wirklich die Position hat, die wir suchen
    if (chunks[ax][az].chunkPos.x == nx && chunks[ax][az].chunkPos.z == nz) {
        return &chunks[ax][az];
    }
    return nullptr; // Nachbar ist noch nicht generiert oder zu weit weg
}


glm::vec2 GetTextureCoordinates(char BlockId , int FaceNumber, float originalU, float originalV)
{
    int atlasIndex = blockLibrary[(unsigned char)BlockId].faces[FaceNumber];

    const int atlasSize = 2;
    const float tileSize = 1.0f / atlasSize;

    int tx = atlasIndex % atlasSize;
    int ty = atlasIndex / atlasSize;


    float u = (originalU * tileSize) + (tx * tileSize);
    float v = (originalV * tileSize) + (ty * tileSize);
    return glm::vec2(u, v);

}



unsigned int texture;

void GnerateChunkvbovaoveo(Chunk& chunk, Chunk* left, Chunk* right, Chunk* back, Chunk* front)
{
   
   

    


    std::vector<float> verticesData;
    chunk.finalIndices.clear();

    // Basis-Layout eines Cubes (0.5 Abstand vom Zentrum)
    // Format: X, Y, Z, U, V
    float unitCube[] = {
        0.5f, -0.5f, -0.5f,  1.0f, 0.0f,   0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
    // Back (+Z)
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,   0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     // Left (-X)
     -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
     -0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  -0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     // Right (+X)
      0.5f, -0.5f,  0.5f,  0.0f, 0.0f,   0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
      0.5f,  0.5f, -0.5f,  1.0f, 1.0f,   0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
      // Bottom (-Y)
      -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,   0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
       0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
       // Top (+Y)
       -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,   0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };

    unsigned int vOffset = 0; // Zähler für Vertices

    size_t estimatedVertices = chunksize * chunksize * 10 * 5;
    // 6 Indices pro Quad (2 Dreiecke)
    size_t estimatedIndices = chunksize * chunksize * 10 * 6;

    verticesData.reserve(estimatedVertices);
    chunk.finalIndices.reserve(estimatedIndices);


    for (int x = 0; x < chunksize; x++) {
        for (int y = 0; y < chunkHeight; y++) {
            for (int z = 0; z < chunksize; z++) {
                if (chunk.blocks[x][y][z] == 0) continue; // Luft überspringen

                glm::vec3 worldpos(chunk.chunkPos.x * chunksize + x, y, chunk.chunkPos.z * chunksize + z);
                int wx = chunk.chunkPos.x * chunksize + x;
                int wy = y;
                int wz = chunk.chunkPos.z * chunksize + z;
                
                bool sideVisible[6];
                // FRONT (Z-1)
                if (z > 0) sideVisible[0] = (chunk.blocks[x][y][z - 1] == AIR);
                else       sideVisible[0] = !isOpaqueGlobal(x, y, chunksize - 1, front); // Korrekt: nimm Rand vom Nachbarn

                // BACK (Z+1)
                if (z < chunksize - 1) sideVisible[1] = (chunk.blocks[x][y][z + 1] == AIR);
                else                   sideVisible[1] = !isOpaqueGlobal(x, y, 0, back); // Korrekt: nimm Rand vom Nachbarn

                // LEFT (X-1)
                if (x > 0) sideVisible[2] = (chunk.blocks[x - 1][y][z] == AIR);
                else       sideVisible[2] = !isOpaqueGlobal(chunksize - 1, y, z, left);

                // RIGHT (X+1)
                if (x < chunksize - 1) sideVisible[3] = (chunk.blocks[x + 1][y][z] == AIR);
                else                   sideVisible[3] = !isOpaqueGlobal(0, y, z, right);

                // BOTTOM & TOP (Y ist immer lokal, da Chunks die volle Höhe haben)
                sideVisible[4] = (y > 0) && (chunk.blocks[x][y - 1][z] == AIR);
                sideVisible[5] = (y < chunkHeight - 1) ? (chunk.blocks[x][y + 1][z] == AIR) : true;

                for (int s = 0; s < 6; s++) {
                    if (sideVisible[s]) { // Nur wenn die Seite NICHT verdeckt (opaque) ist
                        // 1. Vertices für diese eine Seite (4 Stück)
                        for (int v = 0; v < 4; v++) {
                            int idx = (s * 20) + (v * 5);
                            verticesData.push_back(unitCube[idx] + x);
                            verticesData.push_back(unitCube[idx + 1] + y);
                            verticesData.push_back(unitCube[idx + 2] + z);


                            float OriginalU = unitCube[idx + 3]; // U
                            float OriginalV = unitCube[idx + 4]; // V

                            glm::vec2 uv = GetTextureCoordinates(chunk.blocks[x][y][z], s, OriginalU, OriginalV);
                            verticesData.push_back(uv.x);
                            verticesData.push_back(uv.y);
                        }

                        // 2. Indizes für die 2 Dreiecke dieser Seite
                        chunk.finalIndices.push_back(vOffset + 0);
                        chunk.finalIndices.push_back(vOffset + 1);
                        chunk.finalIndices.push_back(vOffset + 2);
                        chunk.finalIndices.push_back(vOffset + 0);
                        chunk.finalIndices.push_back(vOffset + 2);
                        chunk.finalIndices.push_back(vOffset + 3);

                        vOffset += 4; // Nur erhöhen, wenn wir wirklich Vertices hinzugefügt haben!
                    } // Ende if(!sides[s])
                } // Ende for(s)
            }
        }
    }
    
               



    

    if (chunk.VAO == 0) { // Nur wenn noch kein Buffer existiert
        glGenVertexArrays(1, &chunk.VAO);
        glGenBuffers(1, &chunk.VBO);
        glGenBuffers(1, &chunk.EBO);
    }

    glBindVertexArray(chunk.VAO);

    // 2. VBO (Vertex-Daten: Positionen & UVs)
    glBindBuffer(GL_ARRAY_BUFFER, chunk.VBO);
    // glBufferData reserviert den Speicher neu und lädt die Daten hoch
    glBufferData(GL_ARRAY_BUFFER, verticesData.size() * sizeof(float), verticesData.data(), GL_STATIC_DRAW);

    // 3. EBO (Indices: Welche Vertices bilden Dreiecke)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, chunk.finalIndices.size() * sizeof(unsigned int), chunk.finalIndices.data(), GL_STATIC_DRAW);

    // 4. Attribute definieren (muss nach glBufferData stehen, wenn der Puffer neu erstellt wurde)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    
    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    //glBindBuffer(GL_ARRAY_BUFFER, 0);

    // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    //glBindVertexArray(0);



}


unsigned int VBO, VAO, EBO;
double currentTime;


glm::vec3 directions[6] = {
    glm::vec3(1,0,0), glm::vec3(-1,0,0),
    glm::vec3(0,1,0), glm::vec3(0,-1,0),
    glm::vec3(0,0,1), glm::vec3(0,0,-1)
};


bool chunkIsLoaded[maxchunks][maxchunks] = { false };


int main()
{


    Heightnoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    Heightnoise.SetFractalType(FastNoiseLite::FractalType_FBm); 
    Heightnoise.SetFractalOctaves(5);   
    Heightnoise.SetFractalLacunarity(2.0f);
    Heightnoise.SetFractalGain(0.3f);
    Heightnoise.SetSeed(3574);
    Heightnoise.SetFrequency(0.01f);


   
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Minecraft", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwSetCursorPosCallback(window, mouse_callback);

    SetupBlockLibrary();
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

  








    int viewDist = half;

    // SCHRITT 1: Etwas weiter generieren (Sicherheitsrand)
    for (int x = -viewDist - 1; x <= viewDist +1; x++) {
        for (int z = -viewDist -1; z <= viewDist + 1; z++) {
            int ax = x % maxchunks; if (ax < 0) ax += maxchunks;
            int az = z % (maxchunks); if (az < 0) az += (maxchunks);
            GenerateChunk(chunks[ax][az], glm::ivec3(x, 0, z));
        }
    }

    for (int x = -viewDist; x < viewDist; x++) {
        for (int z = -viewDist; z < viewDist; z++) {
            int ax = x % maxchunks; if (ax < 0) ax += maxchunks;
            int az = z % maxchunks; if (az < 0) az += maxchunks;

            
            Chunk* nLeft = GetNeighbor(-1, 0, chunks[ax][az].chunkPos);
            Chunk* nRight = GetNeighbor(1, 0, chunks[ax][az].chunkPos);
            Chunk* nFront = GetNeighbor(0, -1, chunks[ax][az].chunkPos);
            Chunk* nBack = GetNeighbor(0, 1, chunks[ax][az].chunkPos);

           
            GnerateChunkvbovaoveo(chunks[ax][az], nLeft, nRight, nBack, nFront);
        }
    }
    
   

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // link shaders
    unsigned int shaderProgram = glCreateProgram();
    ;
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    std::cout << shaderProgram << std::endl;
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }



    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------





    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load and generate the texture
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);



    unsigned char* data = stbi_load("pics\\spritesheet.png", &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        GLenum internalFormat = (format == GL_RGBA) ? GL_RGBA8 : GL_RGB8;

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
        std::cout << "Loaded texture: " << width << "x" << height << ", channels: " << nrChannels << std::endl;
    }
    stbi_image_free(data);
    std::cout << shaderProgram << std::endl;



    glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);


    

    // uncomment this call to draw in wireframe polygons.


    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return -1;
    }

    FT_Face face;
    if (FT_New_Face(ft, "C:/Windows/Fonts/arial.ttf", 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font from Windows folder" << std::endl;
        return -1;
    }
    
    FT_Set_Pixel_Sizes(face, 0, 48);

    if (FT_Load_Char(face, 'X', FT_LOAD_RENDER))
    {
        std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
        return -1;
    }

    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (unsigned char c = 0; c < 128; c++)
    {
        // load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // generate texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x
        };
        Characters.insert(std::pair<char, Character>(c, character));
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);


    glm::vec3 cubePositionso[] = {
        glm::vec3(0.0f,  0.0f,  0.0f),
        glm::vec3(1.0f,  0.0f,  0.0f),
        glm::vec3(0.0f,  1.0f,  0.0f),
        glm::vec3(1.0f,  1.0f,  0.0f),
    };
    cameraPos = glm::vec3(0.0f, 200.0f, 0.0f);
    glm::ivec2 lastchunkPos = getChunkXZ(cameraPos);
    // render loop
    // 




    
    glGenVertexArrays(1, &TEXVAO);
    glGenBuffers(1, &TEXVBO);
    glBindVertexArray(TEXVAO);
    glBindBuffer(GL_ARRAY_BUFFER, TEXVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);



    TExvertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(TExvertexShader, 1, &TextVertexShader, NULL);

    glCompileShader(TExvertexShader);
    TExfragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(TExfragmentShader, 1, &TextFragmentShader, NULL);
    glCompileShader(TExfragmentShader);
    TEXshaderProgram = glCreateProgram();
    glAttachShader(TEXshaderProgram, TExvertexShader);
    glAttachShader(TEXshaderProgram, TExfragmentShader);
    glLinkProgram(TEXshaderProgram);




    glEnable(GL_DEPTH_TEST); 

    double lastTime = glfwGetTime();
    int nbFrames = 0;
    std::string fpsString = "FPS: 0";

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        processInput(window);
        // render
        // ------
        glClearColor(0.3f, 0.4f, 1.0f, 1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glUseProgram(shaderProgram);
        glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);

        // bind textures on corresponding texture 

        
        forward = cameraFront;
        forward.y = 0.0f;
        forward = glm::normalize(forward);
        
        glm::mat4 Texprojection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f);

        

        playerChunk = getChunkXZ(cameraPos);

        // SCHRITT 1: Chunks nur als "leer" markieren, wenn sie neu sind (sehr schnell)
        if (lastchunkPos != playerChunk) {
            for (int x = -half - 1; x <= half + 1; x++) {
                for (int z = -half - 1; z <= half + 1; z++) {
                    int worldCX = x + playerChunk.x;
                    int worldCZ = z + playerChunk.y;

                    int ax = worldCX % maxchunks; if (ax < 0) ax += maxchunks;
                    int az = worldCZ % maxchunks; if (az < 0) az += maxchunks;

                    glm::ivec3 newPos(worldCX, 0, worldCZ);

                    if (chunks[ax][az].chunkPos != newPos) {
                        chunks[ax][az].chunkPos = newPos;
                        chunks[ax][az].isGenerated = false; // Neues Flag!
                        chunks[ax][az].needsUpdate = false;
                    }
                }
            }
            lastchunkPos = playerChunk;
        }
        






        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 cameraDirection = glm::normalize(cameraPos - cameraTarget);

        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 cameraRight = glm::normalize(glm::cross(up, cameraDirection));

        CamRight = glm::normalize(glm::cross(cameraFront, cameraUp));
        CamRight.y = 0.0f;
        CamRight = glm::normalize(CamRight);

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

       
        glm::mat4 view;
        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        

        glm::mat4 projection;
        projection = glm::perspective(glm::radians(45.0f), float(SCR_WIDTH) / float(SCR_HEIGHT), 0.1f, 200.0f);



        int projLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


        int viewLoc = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        
        int updatesThisFrame = 0;
        const int MAX_UPDATES_PER_FRAME = 1;

        bool actionTaken = false;

        for (int x = 0; x < maxchunks && !actionTaken; x++) {
            for (int z = 0; z < maxchunks && !actionTaken; z++) {

                // A: Zuerst Block-Daten generieren (Noise)
                if (!chunks[x][z].isGenerated) {
                    memset(chunks[x][z].blocks, 0, sizeof(chunks[x][z].blocks));
                    GenerateChunk(chunks[x][z], chunks[x][z].chunkPos);
                    chunks[x][z].isGenerated = true;
                    chunks[x][z].needsUpdate = true; // Jetzt bereit fürs Meshing

                    // Nachbarn triggern
                    // (Code für nx, nz... wie du ihn hattest)

                    actionTaken = true;
                }

                // B: Wenn Daten da sind, Mesh bauen (VBO/VAO)
                else if (chunks[x][z].needsUpdate) {
                    Chunk* nLeft = GetNeighbor(-1, 0, chunks[x][z].chunkPos);
                    Chunk* nRight = GetNeighbor(1, 0, chunks[x][z].chunkPos);
                    Chunk* nFront = GetNeighbor(0, -1, chunks[x][z].chunkPos);
                    Chunk* nBack = GetNeighbor(0, 1, chunks[x][z].chunkPos);

                    if (nLeft && nRight && nFront && nBack &&
                        nLeft->isGenerated && nRight->isGenerated &&
                        nFront->isGenerated && nBack->isGenerated) {

                        GnerateChunkvbovaoveo(chunks[x][z], nLeft, nRight, nBack, nFront);
                        chunks[x][z].needsUpdate = false;
                        actionTaken = true;
                    }
                }
            }
        }
        

        // 2️⃣ Draw
        int modelLoc = glGetUniformLocation(shaderProgram, "model");

        for (int x = 0; x < maxchunks; x++) {
            for (int z = 0; z < maxchunks; z++) {

                if (chunks[x][z].VAO == 0) continue;
                if (chunks[x][z].finalIndices.empty()) continue;

                glm::vec3 worldPos(
                    chunks[x][z].chunkPos.x * chunksize,
                    0.0f,
                    chunks[x][z].chunkPos.z * chunksize
                );

                glm::mat4 model = glm::translate(glm::mat4(1.0f), worldPos);
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

                glBindVertexArray(chunks[x][z].VAO);
                glDrawElements(
                    GL_TRIANGLES,
                    chunks[x][z].finalIndices.size(),
                    GL_UNSIGNED_INT,
                    0
                );
            }
        }
        
        

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);   
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        nbFrames++;
        currentTime = glfwGetTime();
        int Frames;
        if (currentTime - lastTime >= 1.0)
        {
            Frames = nbFrames;
            fpsString = "FPS: " + std::to_string(int(double(nbFrames) / (currentTime - lastTime)));

            nbFrames = 0;
            lastTime = currentTime;
        }

        RenderText(TEXshaderProgram, fpsString, 10.0f, (float)SCR_HEIGHT - 25.0f, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f));
        RenderText(TEXshaderProgram, "Coordinates: "+ std::to_string(float(cameraPos.x)) + ", " + std::to_string(float(cameraPos.y)) + ", " + std::to_string(float(cameraPos.z)), 10.0f, (float)SCR_HEIGHT - 45.0f, 0.3f, glm::vec3(1.0f, 1.0f, 1.0f));


        glEnable(GL_DEPTH_TEST);

        glBindVertexArray(0);
        glUseProgram(0);




        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
   

        // glfw: terminate, clearing all previously allocated GLFW resources.
        // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}


bool isSolid(float x, float y, float z) {
    // 1. Umrechnen von Welt-Koordinate (z.B. 15.7) zu Block-Index (15)
    int gx = (int)std::floor(x);
    int gy = (int)std::floor(y);
    int gz = (int)std::floor(z);

    // 2. Bauhöhe checken (über den Wolken oder unter dem Boden ist kein Block)
    if (gy < 0 || gy >= chunkHeight) return false;

    // 3. Welcher Chunk ist das?
    int cX = (int)std::floor((float)gx / chunksize);
    int cZ = (int)std::floor((float)gz / chunksize);

    // 4. Index im Array (dein Ring-Buffer)
    int arrayX = ((cX % maxchunks) + maxchunks) % maxchunks;
    int arrayZ = ((cZ % maxchunks) + maxchunks) % maxchunks;

    // 5. Lokale Position im Chunk (0 bis 15)
    int lx = gx - (cX * chunksize);
    int lz = gz - (cZ * chunksize);

    // 6. Rückgabe: Ist es Luft (0)?
    return chunks[arrayX][arrayZ].blocks[lx][gy][lz] != AIR; // 0 ist AIR
}










int loaded = 0;
float verticalVelocity = 0.0f;
const float GRAVITY = -21.0f; 
const float JUMP_FORCE = 8.0f; 
float playerRadius = 0.3f;
// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
bool spaceWasPressedLastFrame = false;
void processInput(GLFWwindow* window)
{
    glm::vec3 wishMove = glm::vec3(0.0f);
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if(glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
        
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    float cameraSpeed = 10 * deltaTime;// adjust accordingly
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraSpeed = 12 * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        wishMove += forward * cameraSpeed;
        
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        wishMove -= cameraSpeed * forward;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        wishMove -=  CamRight * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        wishMove += CamRight * cameraSpeed;
    

    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        cameraPos.y -= cameraSpeed;
    


    bool spaceIsDown = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);
    if (spaceIsDown && !spaceWasPressedLastFrame && grounded)
    {
        verticalVelocity = JUMP_FORCE; // Gib einen Impuls nach oben
        grounded = false;
    }
    spaceWasPressedLastFrame = spaceIsDown;

    if (grounded == false)
    {
        verticalVelocity += GRAVITY * deltaTime;
    }

    cameraPos.y += verticalVelocity * deltaTime;
    if (cameraPos.y < -10.0f) cameraPos.y = 500.0f;



    float nextX = cameraPos.x + wishMove.x;

    if (!isSolid(nextX + (wishMove.x > 0 ? playerRadius : -playerRadius), cameraPos.y, cameraPos.z) &&!isSolid(nextX + (wishMove.x > 0 ? playerRadius : -playerRadius), cameraPos.y - 1.5f, cameraPos.z))
    {
        cameraPos.x = nextX;
    }

    float nextZ = cameraPos.z + wishMove.z;

    if (!isSolid(cameraPos.x, cameraPos.y, nextZ + (wishMove.z > 0 ? playerRadius : -playerRadius)) &&!isSolid(cameraPos.x, cameraPos.y - 1.5f, nextZ + (wishMove.z > 0 ? playerRadius : -playerRadius)))
    {
        cameraPos.z = nextZ;
    }














    //unten
    int cX = static_cast<int>(std::floor(static_cast<float>((int)cameraPos.x) / (float)chunksize));
    int cZ = static_cast<int>(std::floor(static_cast<float>((int)cameraPos.z) / (float)chunksize));
    int arrayX = cX % maxchunks;
    if (arrayX < 0) arrayX += maxchunks;
    int arrayZ = cZ % maxchunks;
    if (arrayZ < 0) arrayZ += maxchunks;
    int lx = (int)cameraPos.x - (cX * chunksize);
    int lz = (int)cameraPos.z - (cZ * chunksize);
    int ly = (int)cameraPos.y - 1.6f;
    

    
    
   
    
    
    
    if (chunks[arrayX][arrayZ].blocks[lx][ly][lz] == AIR || chunks[arrayX][arrayZ].blocks[lx][ly][lz] == NULL || ly >= chunkHeight)
    {
        grounded = false;

    }
    else if (chunks[arrayX][arrayZ].blocks[lx][ly][lz] != AIR && chunks[arrayX][arrayZ].blocks[lx][ly][lz] != NULL)
    {
        if (verticalVelocity <= 0.0f)
        {
            grounded = true;
            verticalVelocity = 0.0f;
           
            float floorY = (float)ly + 1.0f + 1.6f;
            if (cameraPos.y <= floorY)
            {
                grounded = true;
                verticalVelocity = 0.0f;
                cameraPos.y = floorY; 
            }
        }

    }
    
}


void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.05f; // change this value to your liking
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

