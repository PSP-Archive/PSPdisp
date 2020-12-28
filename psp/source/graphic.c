/*
  ***************************************************
  PSPdisp (c) 2008 - 2009 Jochen Schleu

  graphic.c - all drawing related functions

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/


#include "graphic.h"
#include "compress.h"
#include "msgdlg.h"


#define BUF_WIDTH (512)
#define SCR_WIDTH (480)
#define SCR_HEIGHT (272)
#define PIXEL_SIZE (4)
#define FRAME_SIZE (BUF_WIDTH * SCR_HEIGHT * PIXEL_SIZE)
#define ZBUF_SIZE (BUF_WIDTH * SCR_HEIGHT * 2)


#define GRAPHIC_BACKGROUND_COLOR RGBA_TO_ABGR(0x304065D0)
#define GRAPHIC_BORDER_COLOR RGBA_TO_ABGR(0x607495D0)


unsigned char* l_lastDrawnFrame; // Stores a pointer to the last drawn frame contents
unsigned int l_lastRotation; // Last drawn frames orientation

bool l_forceFrameDrawing = false;

ScePspFVector3 l_moveToScreenCenter = {240.0f, 136.0f, 0.0f};
   


/*
  graphicInit
  ---------------------------------------------------
  Initialize the graphics.
  ---------------------------------------------------
*/
void graphicInit()
{
  l_frameBuffer = (void*)0;

  l_lastDrawnFrame = NULL;
  l_lastRotation = 0;

  sceGuInit();

  sceGuStart(GU_DIRECT, list);
  sceGuDrawBuffer(GU_PSM_8888, (void*)0, BUF_WIDTH);
  sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void*)(BUF_WIDTH * SCR_HEIGHT * PIXEL_SIZE), BUF_WIDTH);
  sceGuDepthBuffer((void*)0x110000, BUF_WIDTH);
  sceGuOffset(2048 - (SCR_WIDTH / 2), 2048 - (SCR_HEIGHT / 2));
  sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);
  sceGuDepthRange(0xc350, 0x2710);
  sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
  sceGuEnable(GU_SCISSOR_TEST);
  sceGuAlphaFunc(GU_GREATER, 0, 0xff);
  sceGuEnable(GU_ALPHA_TEST);
  sceGuDepthFunc(GU_GEQUAL);
  sceGuDisable(GU_DEPTH_TEST);
  sceGuFrontFace(GU_CW);
  sceGuShadeModel(GU_SMOOTH);
  sceGuDisable(GU_CULL_FACE);
  sceGuEnable(GU_TEXTURE_2D);
  sceGuTexMode(GU_PSM_8888, 0, 0, 0);
  sceGuTexFunc(GU_TFX_DECAL, GU_TCC_RGB);
  sceGuTexFilter(GU_NEAREST, GU_NEAREST);
  sceGuAmbientColor(0xffffffff);
  sceGuTexOffset(0.0f, 0.0f);
  sceGuTexScale(1.0f, 1.0f);
  sceGuEnable(GU_BLEND);
  sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

  sceGuFinish();
  sceGuSync(0,0);

  sceDisplayWaitVblankStart();
  sceGuDisplay(GU_TRUE);
}




/*
  initFontLibrary
  ---------------------------------------------------
  Initialize the IntraFont library, load fonts etc.
  ---------------------------------------------------
  Returns true on success.
*/
void graphicInitFontLibrary()
{
  int result = intraFontInit();

  sansSerifLarge = intraFontLoad("flash0:/font/ltn0.pgf", INTRAFONT_CACHE_ASCII);
  sansSerifSmall = intraFontLoad("flash0:/font/ltn8.pgf", 0);

  if (!(sansSerifLarge && sansSerifSmall))
  {
    // Should never happen
    char message[100];
    sprintf(message, "Could not initialize IntraFont\n(%d, 0x%02lX, 0x%02lX).", result, (long unsigned int)sansSerifSmall, (long unsigned int)sansSerifLarge);
    msgShowError(message, true, false);
    sceKernelExitGame();
  }
}



/*
  switchBuffers
  ---------------------------------------------------
  Exchange the frame buffers for double buffering.
  ---------------------------------------------------
*/
void graphicSwitchBuffers()
{
  l_frameBuffer = sceGuSwapBuffers();
}




/*
  clearScreen
  ---------------------------------------------------
  Fill the screen with a solid color.
  ---------------------------------------------------
*/
void graphicClearScreen(unsigned int color)
{
  sceGuStart(GU_DIRECT, list);

  sceGuClearColor(color);
  sceGuClearDepth(0);
  sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

  sceGuFinish();
  sceGuSync(0,0);
}




/*
  graphicDrawBackground
  ---------------------------------------------------
  Draw the background for the menus.
  ---------------------------------------------------
*/
void graphicDrawBackground(bool drawBars, bool transparent)
{
  if (drawBars)
  {
    if (transparent && configTransparentMenu && g_comRunning)
      graphicRedrawLastFrame();
    else
      graphicClearScreen(GRAPHIC_BACKGROUND_COLOR);
    
    sceGuStart(GU_DIRECT, list);
    graphicDrawColoredRectangle(GRAPHIC_BACKGROUND_COLOR, GRAPHIC_BACKGROUND_COLOR, 0, 30, 480, 212);
    graphicDrawColoredRectangle(GRAPHIC_BORDER_COLOR, GRAPHIC_BORDER_COLOR, 0, 0, 480, 30);
    graphicDrawColoredRectangle(GRAPHIC_BORDER_COLOR, GRAPHIC_BORDER_COLOR, 0, 242, 480, 30);
    sceGuFinish();
    sceGuSync(0,0);
  }
  else
    graphicClearScreen(GRAPHIC_BACKGROUND_COLOR);
}




/*
  graphicSetupOrtho
  ---------------------------------------------------
  Setup an orthographic projection.
  ---------------------------------------------------
*/
void graphicSetupOrtho()
{
  sceGumMatrixMode(GU_PROJECTION);
  sceGumLoadIdentity();
  sceGumOrtho(0, 480, 272, 0, -1, 1);

  sceGumMatrixMode(GU_VIEW);
  sceGumLoadIdentity();

  sceGumMatrixMode(GU_MODEL);
  sceGumLoadIdentity();
}



/*
  graphicFill*Vertex
  ---------------------------------------------------
  Put values into a vertex struct.
  ---------------------------------------------------
*/
void graphicFillPlainVertex(plainVertex* vertex, int x, int y, int z)
{
  vertex->x = x; 
  vertex->y = y; 
  vertex->z = z;
}

void graphicFillColoredVertex(coloredVertex* vertex, int color, int x, int y, int z)
{
  vertex->color = color;
  vertex->x = x; 
  vertex->y = y; 
  vertex->z = z;
}

void graphicFillTexturedVertex(texturedVertex* vertex, int color, int u, int v, int x, int y, int z)
{
  vertex->color = color;
  vertex->u = u;
  vertex->v = v;
  vertex->x = x; 
  vertex->y = y; 
  vertex->z = z;
}




/*
  graphicDrawColoredRectangle
  ---------------------------------------------------
  Draw a quad filled with a solid color.
  ---------------------------------------------------
*/
void graphicDrawColoredRectangle(int colorTop, int colorBottom, int startX, int startY, int width, int height)
{
  graphicSetupOrtho();

  sceGuDisable(GU_TEXTURE_2D);
  sceGuEnable(GU_BLEND);
  sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

  coloredVertex* vertices = (coloredVertex*)sceGuGetMemory(4 * sizeof(coloredVertex));

  graphicFillColoredVertex(&vertices[0], colorTop, startX, startY, 0);
  graphicFillColoredVertex(&vertices[1], colorTop, startX + width, startY, 0);
  graphicFillColoredVertex(&vertices[2], colorBottom, startX + width, startY + height, 0);
  graphicFillColoredVertex(&vertices[3], colorBottom, startX, startY + height, 0);
  
  sceGumDrawArray(GU_TRIANGLE_FAN, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 4, 0, vertices);

  sceGuEnable(GU_TEXTURE_2D);
}




/*
  graphicDrawTexturedRectangle
  ---------------------------------------------------
  Draw a textured quad, used for drawing the button glyphs.
  ---------------------------------------------------
*/
void graphicDrawTexturedRectangle(void* texture, int startX, int startY, int width, int height)
{                           
  graphicSetupOrtho();

  sceGuEnable(GU_TEXTURE_2D);
  sceGuEnable(GU_BLEND);
  sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

  texturedVertex* vertices = (texturedVertex*)sceGuGetMemory(4 * sizeof(texturedVertex));
  graphicFillTexturedVertex(&vertices[0], 0xFFFFFFFF, 0, 0, startX, startY, 0);
  graphicFillTexturedVertex(&vertices[1], 0xFFFFFFFF, 1, 0, startX + width, startY, 0);
  graphicFillTexturedVertex(&vertices[2], 0xFFFFFFFF, 1, 1, startX + width, startY + height, 0);
  graphicFillTexturedVertex(&vertices[3], 0xFFFFFFFF, 0, 1, startX, startY + height, 0);

  sceGuTexMode(GU_PSM_8888, 0, 0, GU_FALSE);	
  sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
  sceGuTexFilter(GU_NEAREST, GU_NEAREST);
  sceGuTexOffset( 0.0f, 0.0f );
  sceGuTexImage(0, width, height, width, texture);

  sceGumDrawArray(GU_TRIANGLE_FAN, GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 4, 0, vertices);
}





/*
  graphicSlicedBlit
  ---------------------------------------------------
  Similar to advanceBlit() from the blit example.
  ---------------------------------------------------
*/
void graphicSlicedBlit(float startX, float startY, float imageWidth, float imageHeight, float textureWidth, float textureHeight)
{
  float xPos = 0.0f;
  float increment = 32.0f;

  sceGuTexScale(1.0f / textureHeight, 1.0f / textureWidth);

  for (xPos = 0.0f; xPos < imageWidth; xPos += increment)
  {
    texturedVertex* vertices = (texturedVertex*)sceGuGetMemory(4 * sizeof(texturedVertex));

    vertices[0].color = 0xFFFFFFFF;
    vertices[0].u = xPos;
    vertices[0].v = 0;
    vertices[0].x = startX + xPos; 
    vertices[0].y = startY; 
    vertices[0].z = 0;

    vertices[1].color = 0xFFFFFFFF;
    vertices[1].u = xPos + increment;
    vertices[1].v = 0;
    vertices[1].x = startX + xPos + increment; 
    vertices[1].y = startY; 
    vertices[1].z = 0;    

    vertices[2].color = 0xFFFFFFFF;
    vertices[2].u = xPos + increment;
    vertices[2].v = 512.0f;
    vertices[2].x = startX + xPos + increment;
    vertices[2].y = startY + 512.0f; 
    vertices[2].z = 0;    

    vertices[3].color = 0xFFFFFFFF;
    vertices[3].u = xPos;
    vertices[3].v = 512.0f;
    vertices[3].x = startX + xPos; 
    vertices[3].y = startY + 512.0f; 
    vertices[3].z = 0;    

    sceGumDrawArray(GU_TRIANGLE_FAN, GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 4, 0, vertices);
  }

  sceGuTexScale(1.0f, 1.0f);
}





/*
  graphicRedrawLastFrame
  ---------------------------------------------------
  Draw from the stored last frame buffer.
  ---------------------------------------------------
*/
void graphicRedrawLastFrame()
{
  if (l_lastDrawnFrame == NULL)
    l_lastDrawnFrame = g_pixelBuffer;

  graphicDrawFrame((unsigned int*)l_lastDrawnFrame, l_lastRotation, true);
}




/*
  drawFrame
  ---------------------------------------------------
  Copy the decoded image into the framebuffer.
  ---------------------------------------------------
*/
void graphicDrawFrame(unsigned int* textureData, unsigned int rotation, bool forceDrawing)
{
  l_lastDrawnFrame = (unsigned char*)textureData;
  l_lastRotation = rotation;

  // Don't overwrite the menu
  if (g_menuActive && !forceDrawing)
    return;

  sceGuStart(GU_DIRECT, list);

  float imageWidth = 0.0f;
  float imageHeight = 0.0f;
  float rotate = 0.0f;

  switch (rotation)
  {
    case 0:
      rotate = 0.0f;
      imageWidth = 480.0f;
      imageHeight = 272.0f;
      break;
    case COM_FLAGS_IMAGE_IS_ROTATED_90_DEG:
      rotate = GU_PI * 0.5f;
      imageWidth = 272.0f;
      imageHeight = 480.0f;
      break;
    case COM_FLAGS_IMAGE_IS_ROTATED_180_DEG:
      rotate = GU_PI;
      imageWidth = 480.0f;
      imageHeight = 272.0f;
      break;
    case COM_FLAGS_IMAGE_IS_ROTATED_270_DEG:
      rotate = GU_PI * 1.5f;
      imageWidth = 272.0f;
      imageHeight = 480.0f;
      break;
  }

  graphicSetupOrtho();

  sceGumTranslate(&l_moveToScreenCenter);
  sceGumRotateZ(rotate);

  sceGuTexMode(GU_PSM_8888, 0, 0, GU_FALSE);	
  sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
  sceGuTexFilter(GU_NEAREST,GU_NEAREST);
  sceGuTexOffset(0.0f, 0.0f);

  sceGuTexImage(0, 512, 512, imageWidth, (void*)textureData);

  graphicSlicedBlit(-0.5f * imageWidth, -0.5 * imageHeight, imageWidth, imageHeight, 512.0f, 512.0f);

  sceGuFinish();
  sceGuSync(0,0);
}




/*
  graphicStartDrawing
  ---------------------------------------------------
  Begin a new list.
  ---------------------------------------------------
*/
void graphicStartDrawing()
{
  sceGuStart(GU_DIRECT, list);
}




/*
  graphicFinishDrawing
  ---------------------------------------------------
  Complete drawing the list, switch buffer if 
  necessary.
  ---------------------------------------------------
*/
void graphicFinishDrawing(bool switchBuffer)
{
  sceGuFinish();
  sceGuSync(0,0);
  
  if (switchBuffer)
    graphicSwitchBuffers();
}




/*
  graphicDrawDanzeff
  ---------------------------------------------------
  Draw the danzeff keyboard.
  ---------------------------------------------------
*/
void graphicDrawDanzeff()
{
  if (g_menuActive)
    return;

  sceGuStart(GU_DIRECT, list);
  sceGuTexScale(1.0f, 1.0f);
  sceGuClear(GU_DEPTH_BUFFER_BIT);
  danzeff_render();
  sceGuFinish();
  sceGuSync(0,0);
}
