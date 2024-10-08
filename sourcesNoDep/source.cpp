#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <Shader.h>
#include <imgLoad.h>
#include <vector>
#include <stdlib.h>

#define MAX_LIFE_TIME 60.0f
//global variables
float lastX, lastY;
float mouseX = -70.0f;
float mouseY = -70.0f;
float sensitivity = 1.2f;
bool firstMouse = true;
float deltaTime;
float previousFrame = 0;
bool spaceHold = false;
bool f1Hold = false;
bool f2Hold = false;
bool debugging = false;
bool fullAuto = false;
float difficultyTweak = 1.0f;
bool leftMHold = false;
bool game =  false;
int activeButton = -1;
bool quitFlag = false;

//menu systen stuff
enum menuContext {MAIN_MENU, SETTINGS_MENU, PAUSE_MENU, NO_MENU};
menuContext previusMenu = MAIN_MENU;
menuContext currentMenu = MAIN_MENU;


//debugging stuff
void printWColor(const std::string &str, int color) {
  std::cout << "\033[" << color << "m";
  std::cout << str << std::endl;
  std::cout << "\033[0m";
}
void checkVecAccess(int size, int index) {
  if (size <= index || index < 0) {
    std::cout << "\033[" << 91 << "m";
    std::cout << "index " << index << " out of range: 0-" << size - 1 << std::endl;
    std::cout << "\033[0m";
  }
}
void checkVecAccess(int size, int index, const std::string &name) {
  if (size <= index || index < 0) {
    std::cout << "\033[" << 91 << "m";
    std::cout << "index " << index << " out of range: 0-" << size - 1 << "; acessing: " << name << std::endl;
    std::cout << "\033[0m";
  }
}
//player
struct playerS {
  glm::vec2 pos, vel;
  float moveSpeed, shotCharge, maxCharge, chargeSpeed, minimalCharge, shotStrength;
  int textureID, score;
};
playerS player;
//brain
struct BrainS {
  int damage;
};
BrainS brain;


//function declarations
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
glm::mat4 createProjection(int width, int height);
void playerUpdate();
void difficultyUpdate();
void start();
void launch();
void end();
void pause();
void resume();
void backSettings();

glm::vec2 norm(glm::vec2 v) {
  if (v.x == 0 && v.y == 0) return glm::vec2(0.0f);
  return glm::normalize(v);
}

//entity stuff commented out for now
/*
struct entity {
  glm::vec2 pos, vel, scale;
  int textureID;
  void (*update)(entity);
};
//global array of all entities
std::vector<entity> Entities;
entity createEntity(glm::vec2 pos, glm::vec2 scale, int textureID, void (*updateFunction)(entity));
*/

//bullet struct, I am getting carried away with these
struct bullet {
  glm::vec2 pos, vel;
  int ID;
  float lifeTime;
};
std::vector<bullet> Bullets;
void spawnBullet(glm::vec2 pos, glm::vec2 vel) {
  bullet b = { pos, vel, (int)Bullets.size(), 0.0f};
  Bullets.push_back(b);
};
void killBullet(int ID){
  Bullets[ID] = Bullets[Bullets.size() - 1];
  Bullets[ID].ID = ID;
  Bullets.pop_back();
}
void updateBullet(bullet *b) {
  b->pos += b->vel * deltaTime;
  b->lifeTime += deltaTime;
  if (b->lifeTime >= MAX_LIFE_TIME) {
    killBullet(b->ID);
  }
}
void clearBullets(){
  Bullets.clear();
}
//collision detection
struct line {
  glm::vec2 p1, p2;
  int ID;
  glm::vec4 color;
};
std::vector<line*> Lines;
//so simple I implemented it right here
line* addLine(glm::vec2 p1, glm::vec2 p2, glm::vec4 color) {
  line* l = (line*)malloc(sizeof(line));
  l->p1 = p1;
  l->p2 = p2;
  l->color = color;
  l->ID = Lines.size();
  Lines.push_back(l);
  return l;
}
void removeLine(int ID) {
  free(Lines[ID]);
  Lines[ID] = Lines[Lines.size() - 1];
  Lines[ID]->ID = ID;
  Lines.pop_back();
}
void clearLines() {
  for (int i = 0; i < Lines.size(); i++) {
    free(Lines[i]);
  }
  Lines.clear();
}
struct boxCollider {
  line* l1;
  line* l2;
  line* l3;
  line* l4;
  int ID, left, right, top, bottom;
};
std::vector<boxCollider*> BCs;
boxCollider* addBC(glm::vec2 pos, glm::vec2 size) {
  glm::vec4 color = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
  float left, right, top, bottom;
  left = pos.x - size.x/2;
  right = pos.x + size.x/2;
  bottom = pos.y - size.y * 0.75f;
  top = pos.y + size.y*0.8f;
  boxCollider *bc = (boxCollider*)malloc(sizeof(boxCollider));
  bc->l1 = addLine(glm::vec2(left, bottom), glm::vec2(right, bottom), color);
  bc->l2 = addLine(glm::vec2(right, bottom), glm::vec2(right, top), color);
  bc->l3 = addLine(glm::vec2(right, top), glm::vec2(left, top), color);
  bc->l4 = addLine(glm::vec2(left, top), glm::vec2(left, bottom), color);
  bc->ID = BCs.size();
  bc->left = left;
  bc->right = right;
  bc->top = top;
  bc->bottom = bottom;
  BCs.push_back(bc);
  return bc;
}
bool checkBC(boxCollider, glm::vec2);
bool checkBC2(int, int, int, int, glm::vec2);
void removeBC(int ID) {
  removeLine(BCs[ID]->l1->ID);
  removeLine(BCs[ID]->l2->ID);
  removeLine(BCs[ID]->l3->ID);
  removeLine(BCs[ID]->l4->ID);
  free(BCs[ID]);
  BCs[ID] = BCs[BCs.size() - 1];
  //updating ID
  BCs[ID]->ID = ID;
  BCs.pop_back();
}
void clearBCs() {
  for (int i = 0; i < BCs.size(); i++) {
    free(BCs[i]);
  }
  BCs.clear();
}
void drawLine(Shader s, glm::vec2 p1, glm::vec2 p2) {
  s.setVec2("p1", p1);
  s.setVec2("p2", p2);
  glDrawArrays(GL_LINES, 0, 2);
}

//can struct, seriously, I need to stop, but I can't
struct can {
  glm::vec2 pos, size, vel, acc;
  int ID, type, reward;
  float lifeTime, weight, damage, maxDamage, hardness;
  boxCollider* collider;
};
std::vector<can*> Cans;
can* spawnCan(glm::vec2 pos, glm::vec2 size, int type) {
  can* c = (can*)malloc(sizeof(can));
  //initialise EVERYTHING
  c->pos = pos;
  c->vel = glm::vec2(0.0f);
  c->acc = glm::vec2(0.0f);
  c->size = size;
  c->collider = addBC(pos, size);
  c->damage = 0;
  c->ID = Cans.size();
  c->lifeTime = 0.0f;
  // type dependant
  int t;
  if(type == -1){    
    t = rand() % 100;
    t = (t>1)?0:1;
  }
  else{
    t = type;
  }
  c->type = t;
  switch(t){
    case 0: 
    c->maxDamage = 3.0f;
    c->reward = 1;
    c->weight = 2.0f;
    c->hardness = 1.0f;
    break;
    case 1:
    c->maxDamage = 3.0f;
    c->reward = 5;
    c->weight = 5.0f;
    c->hardness = 2.0f;
    break;
  }
  
  Cans.push_back(c);
  return c;
}
void autoCan(int type) {
  int axis = rand() % 2;
  glm::vec2 pos = glm::vec2(rand() % 1600 - 800, rand() % 1200 - 600);
  if (axis) {
    //horizontal
    pos = glm::vec2(rand() % 1600 - 800, rand() % 2 * 1200 - 600);
  }
  else{
    pos = glm::vec2(rand() % 2 * 1600 -800, rand() % 1200 - 600);
  }
  spawnCan(pos, glm::vec2(40), type);
}
void removeCan(int ID) {
  removeBC(Cans[ID]->collider->ID);
  free(Cans[ID]);
  Cans[ID] = Cans[Cans.size() - 1];
  //updating IDs
  Cans[ID]->ID = ID;
  Cans.pop_back();
}
void clearCans() {
  for (int i = 0; i < Cans.size(); i++) {
    free(Cans[i]);
  }
  Cans.clear();
}
void updateCan(can* c) {
  c->acc = norm(-(c->pos)) * 100.0f * difficultyTweak;//magic number
  c->vel += c->acc * deltaTime;
  //rudimentary friction
  c->vel *= 1.0f - (0.9f * deltaTime);
  c->pos += c->vel * deltaTime;
  c->lifeTime += deltaTime;
  if (c->lifeTime >= MAX_LIFE_TIME) {
    removeCan(c->ID);
    return;
  }
  float left, right, top, bottom;
  left = c->pos.x - c->size.x / 2;
  right = c->pos.x + c->size.x / 2;
  bottom = c->pos.y - c->size.y * 0.75f;
  top = c->pos.y + c->size.y * 0.8f;
  c->collider->left = left;
  c->collider->right = right;
  c->collider->top = top;
  c->collider->bottom = bottom;
}
//texture handling
struct atlas {
  unsigned int ID, tileSize, width, height;
  std::vector<std::string> paths;
  std::vector<glm::vec4> textureRects;
};

void addImgToAtlas(atlas *atl, const std::string& path);
void finaliseAtlas(atlas *atl);
atlas createAtlas(int tileSize);
//int rendering
//textureShader, numbers, tint should already be set
void renderInt(int i, atlas atl, Shader s, glm::vec2 pos);
//menu elements

struct Button{
  int x, y, width, height, textureID, state;
  bool enabled, visible;
  void (*onClick)();
};
std::vector<Button*> Buttons;
std::vector<Button*> menuButtons;
std::vector<Button*> pauseButtons;
std::vector<Button*> settingsButtons;
Button* createButton(int x, int y, int width, int height, int texture, void (*onClick)()){
  Button* b = (Button*)malloc(sizeof(Button));
  b->x = x;
  b->y = y;
  b->width = width;
  b->height = height;
  b->textureID = texture;
  b->state = 0;
  b->enabled = false;
  b->visible = false;
  b->onClick = onClick;
  Buttons.push_back(b);
  return b;
}
void clearButtons(){
  for(int i = 0; i < Buttons.size(); i++){
    free(Buttons[i]);
  }
}

int main() {
  glfwInit();
  //settings
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  //create window
  GLFWwindow* window = glfwCreateWindow(800, 600, "Tesco Juice hit different", NULL, NULL);
  if (window == NULL) {
    std::cout << "fuck shit gflwwindow" << std::endl;
    glfwTerminate();
    return -1;
  }

  //set up
  glfwMakeContextCurrent(window);

  //load gl functions
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "glad commited suicide" << std::endl;
    glfwTerminate();
    return -1;
  }

  //glfw settings
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  //callbacks
  glfwSetCursorPosCallback(window, mouse_callback);


  //openGL setting
  glViewport(0, 0, 800, 600);

  //shaders
  Shader textureShader = Shader("./shaders/textureV.glsl", "./shaders/textureF.glsl");
  Shader solidShader = Shader("./shaders/vertex.glsl", "./shaders/solidF.glsl");
  Shader lineShader = Shader("./shaders/lineV.glsl", "./shaders/lineF.glsl");

  //buffers
  float verts[] = {
    -1.0f, -1.0f,
    1.0f, -1.0f, 
    1.0f, 1.0f,  

    -1.0f, -1.0f,
    1.0f, 1.0f,  
    -1.0f, 1.0f,

    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,

    0.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f,
  };
  float lineVerts[] = {
    5.0f, 5.0f,
    5.0f, -5.0f,

    5.0f, -5.0f,
    -5.0f, -5.0f,
    
    -5.0f, -5.0f,
    -5.0f, 5.0f,
    
    -5.0f, 5.0f,
    5.0f, 5.0f,

    5.0f, 5.0f,
    -5.0f, -5.0f,
  };
  for (int i = 0; i < 20; i++) {
    lineVerts[i] /= 5;
  }
  unsigned int VBO, VAO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

  //layout
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*) 0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*) (12 * sizeof(float)));
  glEnableVertexAttribArray(1);

  //line
  unsigned int lineVBO, lineVAO;
  glGenVertexArrays(1, &lineVAO);
  glGenBuffers(1, &lineVBO);
  glBindVertexArray(lineVAO);
  glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(lineVerts), lineVerts, GL_STATIC_DRAW);

  //layout
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  //textures
  atlas images = createAtlas(32);
  addImgToAtlas(&images, "./img/cursor.png");
  addImgToAtlas(&images, "./img/cursor2.png");
  addImgToAtlas(&images, "./img/brain.png");
  finaliseAtlas(&images);
  atlas cansTex = createAtlas(32);
  addImgToAtlas(&cansTex, "./img/can.png");
  addImgToAtlas(&cansTex, "./img/canDent.png");
  addImgToAtlas(&cansTex, "./img/canHole.png");
  addImgToAtlas(&cansTex, "./img/Gcan.png");
  addImgToAtlas(&cansTex, "./img/GcanDent.png");
  addImgToAtlas(&cansTex, "./img/GcanHole.png");
  finaliseAtlas(&cansTex);
  atlas numbers = createAtlas(5);
  addImgToAtlas(&numbers, "./img/letter0.png");
  addImgToAtlas(&numbers, "./img/letter1.png");
  addImgToAtlas(&numbers, "./img/letter2.png");
  addImgToAtlas(&numbers, "./img/letter3.png");
  addImgToAtlas(&numbers, "./img/letter4.png");
  addImgToAtlas(&numbers, "./img/letter5.png");
  addImgToAtlas(&numbers, "./img/letter6.png");
  addImgToAtlas(&numbers, "./img/letter7.png");
  addImgToAtlas(&numbers, "./img/letter8.png");
  addImgToAtlas(&numbers, "./img/letter9.png");
  finaliseAtlas(&numbers);
  atlas buttonTex = createAtlas(32);
  addImgToAtlas(&buttonTex, "./img/start.png");
  addImgToAtlas(&buttonTex, "./img/startHover.png");
  addImgToAtlas(&buttonTex, "./img/startPress.png");
  addImgToAtlas(&buttonTex, "./img/quit.png");
  addImgToAtlas(&buttonTex, "./img/quitHover.png");
  addImgToAtlas(&buttonTex, "./img/quitPress.png");
  addImgToAtlas(&buttonTex, "./img/resume.png");
  addImgToAtlas(&buttonTex, "./img/resumeHover.png");
  addImgToAtlas(&buttonTex, "./img/resumePress.png");
  addImgToAtlas(&buttonTex, "./img/options.png");
  addImgToAtlas(&buttonTex, "./img/optionsHover.png");
  addImgToAtlas(&buttonTex, "./img/optionsPress.png");
  addImgToAtlas(&buttonTex, "./img/back.png");
  addImgToAtlas(&buttonTex, "./img/backHover.png");
  addImgToAtlas(&buttonTex, "./img/backPress.png");
  finaliseAtlas(&buttonTex);
  
  //set only once
  textureShader.use();
  textureShader.setInt("tex", 0);


  glm::mat4 projection = createProjection(800, 600);

  glm::mat4 model = glm::mat4(1.0f);
  glm::mat4 identity = glm::mat4(1.0f);

  //initialising random
  srand(1);


  //for debugging
  glm::vec4 identityRect = glm::vec4(0.0f ,0.0f, 1.0f, 1.0f);
  //prev frame
  previousFrame = glfwGetTime();
  
  //start, later swap for launch
  launch();
  
  while (!glfwWindowShouldClose(window) && !quitFlag) {
    glfwPollEvents();
    // buttons
    if(activeButton >= 0) Buttons[activeButton]->state = 0;
    activeButton = -1;
    for(int i = 0; i < Buttons.size(); i++){
      Button b = *Buttons[i];
      if(!b.enabled) continue;
      if(checkBC2(b.x - b.width, b.x + b.width, b.y - b.height, b.y + b.height, glm::vec2(mouseX,mouseY))){
        activeButton = i;
        Buttons[i]->state = 1;
        break;
      }
    }
    processInput(window);
    float time = glfwGetTime();
    deltaTime = time - previousFrame;
    previousFrame = time;
    
    //updates first
    if(game){
    playerUpdate();
    for (int i = 0; i < Cans.size(); i++) {
      updateCan(Cans[i]);
    }
    for (int i = 0; i < Bullets.size(); i++) {
      checkVecAccess(Bullets.size(), i);
      updateBullet(&Bullets[i]);
    }
    //collision check
    for (int i = 0; i < BCs.size(); i++) {
      for (int j = 0; j < Bullets.size(); j++) {
        checkVecAccess(BCs.size(), i, "BCs");
        checkVecAccess(Bullets.size(), j, "Bullets");
        if (checkBC(*BCs[i], Bullets[j].pos)) {
          checkVecAccess(Cans.size(), i, "Cans");
          Cans[i]->damage += glm::length(Bullets[j].vel) / player.shotStrength / Cans[i]->hardness * 2.0f;
          Cans[i]->vel += Bullets[j].vel / Cans[i]->weight;
          killBullet(j);
          j--; //step back, because killBullet puts a different bullet at j
          if (Cans[i]->damage >= Cans[i]->maxDamage) {
            //error probably here:
            player.score += Cans[i]->reward;
            difficultyUpdate();
            removeCan(i);
            i--; // step back, -,,-
            autoCan(-1);
            //stop checking
            break;
          }
        }
      }
    }
    //brainDamage check
    for (int i = 0; i < BCs.size(); i++) {
      if (checkBC(*BCs[i], glm::vec2(0.0f))) {
        removeCan(i);
        brain.damage++;
        if(brain.damage >= 10) {end(); break;}
        autoCan(-1);
        i--;
      }
    }
    }
   
    glClear(GL_COLOR_BUFFER_BIT);
    //shader setup
    textureShader.use();
    glActiveTexture(GL_TEXTURE0);
    textureShader.setMat4("projection", projection);
    textureShader.setVec4("tint", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    glBindVertexArray(VAO);
    
    //numbers
    //brain damage
    glBindTexture(GL_TEXTURE_2D, numbers.ID);
    textureShader.setVec4("tint", glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
    renderInt(brain.damage, numbers, textureShader, glm::vec2(100.0f));
    //score
    textureShader.setVec4("tint", glm::vec4(0.5f, 1.0f, 0.5f, 1.0f));
    renderInt(player.score, numbers, textureShader, glm::vec2(100.0f, 0.0f));
    textureShader.setVec4("tint", glm::vec4(1.0f));
    // game object rendering
  if(game){
    glBindTexture(GL_TEXTURE_2D, cansTex.ID);
    //cans for now
    
    for (int i = 0; i < Cans.size(); i++) {
      checkVecAccess(Cans.size(), i , "CansRender");
      model = glm::mat4(1.0f);
      model = glm::translate(model, glm::vec3(Cans[i]->pos, 0.0f));
      model = glm::scale(model, glm::vec3(Cans[i]->size.x, Cans[i]->size.y, 1.0f));
      textureShader.setMat4("model", model);
      textureShader.setVec4("rect", cansTex.textureRects[std::floor(Cans[i]->damage/Cans[i]->maxDamage*3.0f) + Cans[i]->type * 3]);
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    

    //other objects
    glBindTexture(GL_TEXTURE_2D, images.ID);
    textureShader.setVec4("tint", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    //player
    textureShader.setVec4("rect", images.textureRects[player.textureID]);
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(player.pos, 0.0f));
    model = glm::scale(model, glm::vec3(32.0f, 32.0f, 1.0f));
    textureShader.setMat4("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    //brain
    textureShader.setVec4("rect", images.textureRects[2]);
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(64.0f, 64.0f, 0.0f));
    textureShader.setMat4("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    glBindTexture(GL_TEXTURE_2D, buttonTex.ID);
    //Buttons
    for(int i = 0; i < Buttons.size(); i++){
      if(!Buttons[i]->visible)continue;
      textureShader.setVec4("rect", buttonTex.textureRects[Buttons[i]->textureID * 3  + Buttons[i]->state]);
      model = glm::mat4(1.0f);
      model = glm::translate(model, glm::vec3(Buttons[i]->x, Buttons[i]->y, 0.0f));
      model = glm::scale(model, glm::vec3(64.0f, 64.0f, 1.0f));
      textureShader.setMat4("model", model);
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    glBindTexture(GL_TEXTURE_2D, images.ID);
    //mouse
    textureShader.setVec4("rect", images.textureRects[0]);
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(mouseX, mouseY, 0.0f));
    model = glm::scale(model, glm::vec3(32.0f, 32.0f, 1.0f));
    textureShader.setMat4("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    //entities
    /*
    for (int i = 0; i < Entities.size(); i++) {
      model = glm::mat4(1.0f);
      model = glm::translate(model, glm::vec3(Entities[i].pos, 0.0f));
      model = glm::scale(model, glm::vec3(Entities[i].scale, 1.0f));
      textureShader.setMat4("model", model);
      textureShader.setVec4("rect", images.textureRects[Entities[i].textureID]);
    }
    */
    //canIDs
    if (debugging) {
      glBindTexture(GL_TEXTURE_2D, numbers.ID);
      textureShader.setVec4("tint", glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
      for (int i = 0; i < Cans.size(); i++) {
        renderInt(Cans[i]->ID, numbers, textureShader, glm::vec2(std::floor(Cans[i]->pos.x + Cans[i]->size.x + 20),std::floor(Cans[i]->pos.y)));
      }
    }
    
    //line shader setup
    lineShader.use(); 
    lineShader.setMat4("projection", projection);
    //BoxColliders
    if (debugging) {
      //only when debugging
      //lines
      for (int i = 0; i < BCs.size(); i++) {
        glm::vec2 p1, p2, p3, p4;
        p1 = glm::vec2(BCs[i]->left, BCs[i]->bottom);
        p2 = glm::vec2(BCs[i]->right, BCs[i]->bottom);
        p3 = glm::vec2(BCs[i]->right, BCs[i]->top);
        p4 = glm::vec2(BCs[i]->left, BCs[i]->top);
        lineShader.setVec4("Color", glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
        drawLine(lineShader, p1, p2);
        drawLine(lineShader, p2, p3);
        drawLine(lineShader, p3, p4);
        drawLine(lineShader, p4, p1);
      }
      /*
      for (int i = 0; i < Lines.size(); i++) {
        checkVecAccess(Lines.size(), i, "lines");
        lineShader.setVec2("p1", Lines[i]->p1);
        lineShader.setVec2("p2", Lines[i]->p2);
        lineShader.setVec4("Color", Lines[i]->color);
        glDrawArrays(GL_LINES, 0, 2);
      }
      */
    }
    //bullets
    for (int i = 0; i < Bullets.size(); i++) {
      checkVecAccess(Bullets.size(), i, "bullets render");
      lineShader.setVec2("p1", Bullets[i].pos);
      lineShader.setVec2("p2", Bullets[i].pos - (Bullets[i].vel * deltaTime * 3.0f));
      lineShader.setVec4("Color", glm::vec4(1.0f));
      glDrawArrays(GL_LINES, 0, 2);
    }
    //need this blud
    glfwSwapBuffers(window);
  }
  //cleanup
  glDeleteBuffers(1, &VBO);
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &lineVBO);
  glDeleteVertexArrays(1, &lineVAO);
  glDeleteProgram(textureShader.ID);
  glDeleteProgram(solidShader.ID);
  glDeleteTextures(1, &images.ID);
  glDeleteTextures(1, &cansTex.ID);
  glDeleteTextures(1, &numbers.ID);
  glDeleteTextures(1, &buttonTex.ID);
  glfwTerminate();

  //free
  clearCans();
  clearBCs();
  clearLines();
  clearButtons();
  return 0;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
  //this check may be redundant
  if (firstMouse) {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
    srand(xpos);
  }
  float xoffset = (xpos - lastX) * sensitivity;
  float yoffset = (lastY - ypos) * sensitivity; //y is flipped (gl y coordinates go from bottom to top)
  lastX = xpos;
  lastY = ypos;
  mouseX += xoffset;
  mouseY += yoffset;
  mouseX = std::min(mouseX, 800.0f);
  mouseY = std::min(mouseY, 600.0f);
  mouseX = std::max(mouseX, -800.0f);
  mouseY = std::max(mouseY, -600.0f);
}
void addImgToAtlas(atlas *atl, const std::string &path) {
  atl->paths.push_back(path);
}
void finaliseAtlas(atlas* atl) {
  int atlasWidth = (int)std::ceil(std::sqrt(atl->paths.size()));
  int atlasHeight = std::ceil(atl->paths.size() / (float)atlasWidth);
  stbi_set_flip_vertically_on_load(true);
  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  //I don't know what this is supposed to do and it doesn't change anything, so commented out for now
  //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atl->tileSize * atlasWidth, atl->tileSize * atlasHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

  for (int i = 0; i < atl->paths.size(); i++) {
    int width, height, nrComp;
    unsigned char* data = stbi_load(atl->paths[i].c_str(), &width, &height, &nrComp, 0);
    if (data) {
      GLenum format = GL_RGBA;
      if (nrComp == 1) {
        format = GL_RED;
      }
      else if (nrComp == 3) {
        format = GL_RGB;
      }
      else if (nrComp == 4) {
        format = GL_RGBA;
      }
      int posX, posY;
      posX = i % atlasWidth;
      posY = (i - posX) / atlasWidth;
      glTexSubImage2D(GL_TEXTURE_2D, 0, posX * atl->tileSize, posY * atl->tileSize, atl->tileSize, atl->tileSize, format, GL_UNSIGNED_BYTE, data);
      //std::cout << "image good: " << atl->paths[i] << std::endl;
    }
    else {
      std::cout << "image fucked: " << atl->paths[i] << std::endl;
    }
    stbi_image_free(data);
  }

  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  atl->ID = texture;
  atl->width = atlasWidth;
  atl->height = atlasHeight;
  //unbind for safety
  glBindTexture(GL_TEXTURE_2D, 0);

  //atlas Rects
  float Xratio, Yratio;
  Xratio = 1.0f / atl->width;
  Yratio = 1.0f / atl->height;
  int posX, posY;
  for (int i = 0; i < atl->paths.size(); i++) {
    posX = i % atl->width;
    posY = (i - posX) / atl->width;
    atl->textureRects.push_back(glm::vec4(Xratio * (float) posX, Yratio * (float) posY, Xratio, Yratio));
  }
}
void processInput(GLFWwindow* window) {
  glm::vec2 moveDir = glm::vec2(0.0f);
  int input = 0;
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    switch(currentMenu){
      case NO_MENU:
        pause();
      break;
      case SETTINGS_MENU:
        backSettings();
      break;
      //do nothing
      default:
      break;
    }
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    moveDir.x -= 1;
    input += 1;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    moveDir.x += 1;
    input -= 1;
  }
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    moveDir.y += 1;
    input += 2;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    moveDir.y -= 1;
    input -= 2;
  }
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    //signal high
    if (!spaceHold && game) {
      //rising edge
      //currently nothing special
    }
    player.shotCharge += player.chargeSpeed * deltaTime;
    player.shotCharge = std::min(player.shotCharge, player.maxCharge);
    spaceHold = true;
  }
  else {
    //signal low
    if(spaceHold){
      //falling edge
      if(player.shotCharge >= player.minimalCharge){
        spawnBullet(player.pos, glm::normalize(glm::vec2(mouseX-player.pos.x, mouseY-player.pos.y)) * player.shotCharge * player.shotStrength);
        player.shotCharge = 0.0f;
      }
    }
    spaceHold = false;
  }
  if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) {
    if (!f1Hold) {
      debugging = !debugging;
    }
    f1Hold = true;
  }
  else { f1Hold = false; }
  if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS) {
    if (!f2Hold) {
      fullAuto = !fullAuto;
    }
    f2Hold = true;
  }
  else { f2Hold = false; }
  player.vel = input ? glm::normalize(moveDir) * player.moveSpeed : glm::vec2(0.0f);
  // buttons
  if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS){
    if(activeButton >= 0){
      Buttons[activeButton]->state = 2;
    }
    leftMHold = true;
  }else{
    if(leftMHold && activeButton >= 0){
      if(Buttons[activeButton]->enabled) (*Buttons[activeButton]->onClick)();
    }
    leftMHold = false;
  }
}
glm::mat4 createProjection(int width, int height) {
  glm::mat4 proj = glm::mat4(1.0f);
  proj = glm::scale(proj, glm::vec3(1.0f / (float)width, 1.0f / (float)height, 1.0f));
  return proj;
}
atlas createAtlas(int tileSize) {
  atlas atl;
  atl.tileSize = tileSize;
  return atl;
}
void playerUpdate(){
  player.pos += player.vel * deltaTime;
}
/*
entity createEntity(glm::vec2 pos, glm::vec2 scale, int textureID, void (*updateFunction)(entity)) {
  entity ent;
  ent.update = updateFunction;
  ent.pos = pos;
  ent.scale = scale;
  ent.textureID = textureID;
  return ent;
}
*/

//debug rendering
//assumes LineShader is in use; VAO already bound; projection matrix set
//probably redundant
void renderLine(line *f, Shader s) {
  s.setVec2("p1", f->p1.x, f->p1.y);
  s.setVec2("p2", f->p2.x, f->p2.y);
  s.setVec4("Color", f->color);
  glDrawArrays(GL_LINES, 0, 2);
}
void renderInt(int i, atlas atl, Shader s, glm::vec2 pos) {
  std::vector<int> digits;
  while (i >= 10) {
    int dig = i % 10;
    digits.push_back(dig);
    i -= dig;
    i /= 10;
  }
  digits.push_back(i);
  for (int i = 0; i < digits.size(); i++) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(pos.x + i * 64, pos.y, 0.0f));
    model = glm::scale(model, glm::vec3(32.0f, 32.0f, 1.0f));
    s.setMat4("model", model);
    s.setVec4("rect", atl.textureRects[digits[digits.size() - i - 1]]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
  }
}

//intersect check
bool onSegment(glm::vec2 p, glm::vec2 q, glm::vec2 r){
  if (q.x <= std::max(p.x, r.x) && q.x >= std::min(p.x, r.x) &&
    q.y <= std::max(p.y, r.y) && q.y >= std::min(p.y, r.y)) {
    return true;
  }
  return false;
}
int orientation(glm::vec2 p, glm::vec2 q, glm::vec2 r){
  // See https://www.geeksforgeeks.org/orientation-3-ordered-points/ 
  // for details of below formula. 
  int val = (q.y - p.y) * (r.x - q.x) -
    (q.x - p.x) * (r.y - q.y);
  if (val == 0) return 0;  // collinear 
  return (val > 0) ? 1 : 2; // clock or counterclock wise 
}
bool check2lines(glm::vec2 p1, glm::vec2 p2, glm::vec2 q1, glm::vec2 q2) {
  int o1 = orientation(p1, q1, p2);
  int o2 = orientation(p1, q1, q2);
  int o3 = orientation(p2, q2, p1);
  int o4 = orientation(p2, q2, q1);
  if (o1 != o2 && o3 != o4) return true;

  if (o1 == 0 && onSegment(p1, p2, q1)) return true;
  // p1, q1 and q2 are collinear and q2 lies on segment p1q1 
  if (o2 == 0 && onSegment(p1, q2, q1)) return true;
  // p2, q2 and p1 are collinear and p1 lies on segment p2q2 
  if (o3 == 0 && onSegment(p2, p1, q2)) return true;
  // p2, q2 and q1 are collinear and q1 lies on segment p2q2 
  if (o4 == 0 && onSegment(p2, q1, q2)) return true;
  return false; // Doesn't fall in any of the above cases 
}
bool checkBC(boxCollider bc, glm::vec2 point) {
  if (point.x < bc.left) return false;
  if (point.x > bc.right) return false;
  if (point.y < bc.bottom) return false;
  if (point.y > bc.top) return false;
  return true;
}
bool checkBC2(int left, int right, int bottom, int top, glm::vec2 point) {
  if (point.x < left) return false;
  if (point.x > right) return false;
  if (point.y < bottom) return false;
  if (point.y > top) return false;
  return true;
}
void difficultyUpdate(){
   //cans number
   //magic numbers
    if (Cans.size() < std::floor(std::sqrt(player.score)) + 1) {
      autoCan((player.score % 50 == 0)?1:-1);
      difficultyTweak += 0.05f;
    }
}
void batchEnableDisable(std::vector<Button*> list, bool value){
  for(int i = 0; i < list.size(); i++){
    list[i]->enabled = value;
    list[i]->visible = value;
  }
}
void settings(){
  switch(currentMenu){
    case MAIN_MENU:
      batchEnableDisable(menuButtons, false);      
      break;
    case PAUSE_MENU:
      batchEnableDisable(pauseButtons, false);
      break;
  }
  previusMenu = currentMenu;
  currentMenu = SETTINGS_MENU;
  batchEnableDisable(settingsButtons, true);
  
}
void backSettings(){
  batchEnableDisable(settingsButtons, false);
  currentMenu = previusMenu;
  switch(previusMenu){
    case PAUSE_MENU:
      batchEnableDisable(pauseButtons, true);
    break;
    case MAIN_MENU:
      batchEnableDisable(menuButtons, true);
    break;
  }
}
void start(){ 
  //game setup
  //player
  player.moveSpeed = 500;
  player.pos = glm::vec2(80.0f);
  player.vel = glm::vec2(0.0f);
  player.score = 0;
  player.textureID = 1;
  player.shotCharge = 0.0f;
  player.maxCharge = 1.0f;
  player.minimalCharge = 0.2f;
  player.chargeSpeed = 2.0f;
  player.shotStrength = 1000.0f;
  //brain
  brain.damage = 0;

  //spawning a few cans
  for (int i = 0; i < 3; i++) autoCan(-1);
  //disable menu
  game = true;
  currentMenu = NO_MENU;
  for(int i = 0; i < menuButtons.size(); i++){
    menuButtons[i]->visible = false;
    menuButtons[i]->enabled = false;
  }
}
void end(){
  clearBCs();
  clearCans();
  clearLines();
  clearBullets();
  game = false;
  currentMenu = MAIN_MENU;
  //enable menu
  for(int i = 0; i < pauseButtons.size(); i++){
    pauseButtons[i]->visible = false;
    pauseButtons[i]->enabled = false;
  }
  for(int i = 0; i < menuButtons.size(); i++){
    menuButtons[i]->visible = true;
    menuButtons[i]->enabled = true;
  }
}
void quit(){
  quitFlag = true;
}
void pause(){
  game = false;
  currentMenu = PAUSE_MENU;
  for(int i = 0; i < pauseButtons.size(); i++){
    pauseButtons[i]->enabled = true;
    pauseButtons[i]->visible = true;
  }
}
void resume(){
  game = true;
  currentMenu = NO_MENU;
  for(int i = 0; i < pauseButtons.size(); i++){
    pauseButtons[i]->enabled = false;
    pauseButtons[i]->visible = false;
  }
}
void launch(){
  //Main menu
  Button* startButton = createButton(0, 0, 64, 24, 0, &start);
  menuButtons.push_back(startButton);
  Button* quitButton = createButton(0, -200, 64, 24, 1, &quit);
  menuButtons.push_back(quitButton);
  Button* enterSettings = createButton(0, -100, 64, 24, 3, &settings);
  menuButtons.push_back(enterSettings);
  
  //pause
  Button* resumeButton = createButton(0, 0, 64, 24, 2, &resume);
  pauseButtons.push_back(resumeButton);
  Button* endButton = createButton(0, -200, 64, 24, 1, &end);
  pauseButtons.push_back(endButton);
  pauseButtons.push_back(enterSettings);
  
  //settings
  Button* exitSettings = createButton(0, -100, 64, 24, 4, &backSettings);
  settingsButtons.push_back(exitSettings);
  
  for(int i = 0; i < menuButtons.size(); i++){
    menuButtons[i]->enabled = true;
    menuButtons[i]->visible = true;
  }
}

