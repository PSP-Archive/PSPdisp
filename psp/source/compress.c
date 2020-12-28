/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  compress.c - frame decompression

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#include "compress.h"

#include "osk.h"
#include "msgdlg.h"


bool l_sceJpegAvailable = false; // For libjpeg fallback
bool l_buffersCleared = false; // Avoid clearing the buffers if they are already empty
bool l_lastFrameWasRotated = false; // Used to determine whether the Jpeg decoder has to be reset
SceUID l_jpegModuleId;

unsigned char __attribute__((aligned(16))) l_pixelBuffer_1[480 * 272 * 4];
unsigned char __attribute__((aligned(16))) l_pixelBuffer_2[480 * 272 * 4];


// libjpeg types and variables
void libjpeg_init_source(j_decompress_ptr cinfo);
int libjpeg_fill_input_buffer(j_decompress_ptr cinfo);
void libjpeg_skip_input_data(j_decompress_ptr cinfo, long num_bytes);
void libjpeg_term_source(j_decompress_ptr cinfo);
boolean libjpeg_resync_to_restart(j_decompress_ptr cinfo, int desired);

typedef struct
{
  struct jpeg_source_mgr sourceManager;
  unsigned char* buffer;
  unsigned char* outputBuffer;
  unsigned int bufferPosition;
  unsigned int bufferSize;
} 
libjpegSourceManager;

struct jpeg_decompress_struct l_libjpegCinfo;
struct jpeg_error_mgr l_libjpegJerr;



/*
  compressInit
  ---------------------------------------------------
  Initialize decompression functions.
  ---------------------------------------------------
*/
void compressInit()
{
  compressClearBuffers();

  compressInitJpegDecoding();
  compressInitPngDecoding();

  // Create decompression thread
  g_compressThreadId = sceKernelCreateThread("compressThread", (SceKernelThreadEntry)compressDecodingAndDrawingThread, 0x18, 0x10000, PSP_THREAD_ATTR_USER, 0);    
}





/*
  compressTerm
  ---------------------------------------------------
  Shutdown decompression functions.
  ---------------------------------------------------
*/
void compressTerm()
{
  sceKernelWaitThreadEnd(g_compressThreadId, NULL);
  sceKernelDeleteThread(g_compressThreadId);

  compressTermJpegDecoding();
  compressTermPngDecoding();
}



/*
  compressClearBuffers
  ---------------------------------------------------
  Resets the target buffers.
  ---------------------------------------------------
*/
void compressClearBuffers()
{
  if (l_buffersCleared)
    return;

  g_pixelBuffer = l_pixelBuffer_1;

  memset(l_pixelBuffer_1, 0, 480 * 272 * 4);
  memset(l_pixelBuffer_2, 0, 480 * 272 * 4);

  l_buffersCleared = true;
}




/*
  compressInitJpegDecoding
  ---------------------------------------------------
  Loads the PRX and inits the Jpeg decompression.
  ---------------------------------------------------
  Returns 0 on success, a negative value on error.
*/
int compressInitJpegDecoding() 
{
  int code = -1;

  l_jpegModuleId = sceUtilityLoadModule(PSP_MODULE_AV_AVCODEC);

  if (l_jpegModuleId < 0)
  {
    // Try loading the module directly
    l_jpegModuleId = utilsLoadStartModule("flash0:/kd/avcodec.prx");  
  }

  if (l_jpegModuleId > -1)
  {
    code = -2;
    if (sceJpegInitMJpeg() == 0) 
    {
      code = -3;
      if (sceJpegCreateMJpeg(480, 272) == 0) 
      {
        // Success!
        l_lastFrameWasRotated = false;
        l_sceJpegAvailable = true;
        return 0;
      }
    }
  }

  // Some step failed, use libjpeg
  l_sceJpegAvailable = false;

  l_libjpegCinfo.err = jpeg_std_error(&l_libjpegJerr);
  jpeg_create_decompress(&l_libjpegCinfo);

  libjpegSourceManager* source;
  l_libjpegCinfo.src = (struct jpeg_source_mgr*)malloc(sizeof(libjpegSourceManager));

  source = (libjpegSourceManager*)l_libjpegCinfo.src;
  source->buffer = NULL;
  source->outputBuffer = NULL;
  source->bufferPosition = 0;
  source->bufferSize = 0;

  source->sourceManager.init_source = libjpeg_init_source;
  source->sourceManager.fill_input_buffer = libjpeg_fill_input_buffer;
  source->sourceManager.skip_input_data = libjpeg_skip_input_data;
  source->sourceManager.resync_to_restart = libjpeg_resync_to_restart;
  source->sourceManager.term_source = libjpeg_term_source;

  return 0;
}





/*
  compressTermJpegDecoding
  ---------------------------------------------------
  Shutdown the Jpeg decompression functions.
  ---------------------------------------------------
*/
void compressTermJpegDecoding()
{
  if (l_sceJpegAvailable)
  {
    sceJpegDeleteMJpeg();
    sceJpegFinishMJpeg();

    // If the library was loaded by the user mode utility function,
    // the module Id variable just holds a "0"
    if (l_jpegModuleId == 0)
      sceUtilityUnloadAvModule(PSP_AV_MODULE_AVCODEC);
    else
      utilsStopUnloadModule(l_jpegModuleId);
  }
  else
  {
    jpeg_destroy_decompress(&l_libjpegCinfo);
  }
}





/*
  compressInitPngDecoding
  ---------------------------------------------------
  Initialize the PNG decompression.
  ---------------------------------------------------
*/
int compressInitPngDecoding()
{
  return 0;
}




/*
  compressTermPngDecoding
  ---------------------------------------------------
  Shut down the PNG decompression.
  ---------------------------------------------------
*/
void compressTermPngDecoding()
{
  return;
}




/*
  compressConvertBmp
  ---------------------------------------------------
  Bring an uncompressed frame into the right format
  for the framebuffer.
  ---------------------------------------------------
*/
void compressConvertBmp(comFrameHeader* frameHeader)
{
  // Bloat the 24 Bit values to 32 Bit
  // Windows Bitmaps are in BGR order and upside down

  unsigned int imageWidth;
  unsigned int imageHeight;

  unsigned char* pixelsBackup = (unsigned char*)g_pixelBuffer;
  unsigned char* dataBackup = g_comImageReceiveBuffer;
  
  if ((frameHeader->flags & COM_FLAGS_IMAGE_IS_ROTATED_90_DEG) || (frameHeader->flags & COM_FLAGS_IMAGE_IS_ROTATED_270_DEG))
  {
    imageWidth = 272;
    imageHeight = 480;
  }
  else
  {
    imageWidth = 480;
    imageHeight = 272;
  }

  unsigned char* inPixels;
  unsigned char* outPixels;
  
  int i, j;

  for (i = imageHeight - 1; i >= 0; i--)
  {
    outPixels = 1 + pixelsBackup + (imageWidth * (imageHeight - 1 - i) * 4);
    inPixels = 1 + dataBackup + (imageWidth * i * 3);

    for (j = 0; j < imageWidth; j++)
    {
      *(outPixels - 1) = *(inPixels + 1);
      *(outPixels + 1) = *(inPixels - 1);
      *(outPixels) = *(inPixels);

      outPixels += 4;
      inPixels += 3;
    }
  }

/*
  unsigned char* inPixels = &g_comImageReceiveBuffer[0] + 1;
  unsigned char* outPixels = (unsigned char*)g_pixelBuffer + 1;

  int i = 0;
  while (i < 480 * 272)
  {
    *(outPixels - 1) = *(inPixels + 1);
    *(outPixels + 1) = *(inPixels - 1);
    *(outPixels) = *(inPixels);

    outPixels += 4;
    inPixels += 3;
    i++;
  }
*/

  sceKernelDcacheWritebackInvalidateAll();
}





/*
  compressDecompressJpeg
  ---------------------------------------------------
  Decompress a Jpeg frame.
  ---------------------------------------------------
*/
void compressDecompressJpeg(comFrameHeader* frameHeader)
{
  static bool lastFrameRotated = false;
  bool rotated = ((frameHeader->flags & COM_FLAGS_IMAGE_IS_ROTATED_90_DEG) || (frameHeader->flags & COM_FLAGS_IMAGE_IS_ROTATED_270_DEG));

  if (l_sceJpegAvailable)
  {
    // Use sceJpeg* functions
    if (lastFrameRotated != rotated)
    {
      sceJpegDeleteMJpeg();

      if (rotated)
        sceJpegCreateMJpeg(272, 480);
      else
        sceJpegCreateMJpeg(480, 272);

      lastFrameRotated = rotated;
    }

    sceJpegDecodeMJpeg(&g_comImageReceiveBuffer[0], frameHeader->imageSize, g_pixelBuffer, 0);
  }
  else
  {
    libjpegSourceManager* source;
    source = (libjpegSourceManager*)l_libjpegCinfo.src;
    source->buffer = &g_comImageReceiveBuffer[0];
    source->outputBuffer = g_pixelBuffer;
    source->bufferPosition = 0;
    source->bufferSize = frameHeader->imageSize;

    // Point the buffer to the whole file in memory
    source->sourceManager.bytes_in_buffer = source->bufferSize;
    source->sourceManager.next_input_byte = source->buffer;

    if (jpeg_read_header(&l_libjpegCinfo, TRUE))
    {
      l_libjpegCinfo.do_fancy_upsampling = FALSE;
      l_libjpegCinfo.do_block_smoothing = FALSE;
      l_libjpegCinfo.dct_method = JDCT_FASTEST;


      if (jpeg_start_decompress(&l_libjpegCinfo))
      {
        unsigned int linesRead = 0;

        while (l_libjpegCinfo.output_scanline < l_libjpegCinfo.output_height)
        {
          linesRead = jpeg_read_scanlines(&l_libjpegCinfo, &source->outputBuffer, 1);
          source->outputBuffer += (4 * l_libjpegCinfo.output_width) * linesRead;
        }
        jpeg_finish_decompress(&l_libjpegCinfo);
      }
      sceKernelDcacheWritebackInvalidateRange(source->outputBuffer, source->bufferSize);
    }
  }
}




/*
  compressPngReadDataCallback
  ---------------------------------------------------
  Callback for the PNG decompressor, managing reading
  the input data.
  ---------------------------------------------------
*/
void compressPngReadDataCallback(png_structp png_ptr, png_bytep data, png_size_t length)
{
  unsigned int* bufferPosition = (unsigned int*)(png_ptr->io_ptr);
  memcpy(data, &g_comImageReceiveBuffer[*bufferPosition], length);
  *bufferPosition += length;
}




/*
  compressDecompressPng
  ---------------------------------------------------
  Decompress a PNG input image. Unusably slow.
  ---------------------------------------------------
*/
void compressDecompressPng(comFrameHeader* frameHeader)
{
  png_structp png_ptr;
  png_infop info_ptr;
  unsigned int sig_read = 0;
  png_uint_32 width;
  png_uint_32 height;
  int bit_depth;
  int color_type;
  int interlace_type;
  size_t x, y;
  u32* line;

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL) 
  {
    return;
  }

  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) 
  {
    png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
    return;
  }

  unsigned int bufferPosition = 0;
  png_set_read_fn(png_ptr, &bufferPosition, &compressPngReadDataCallback);

  png_set_sig_bytes(png_ptr, sig_read);
  png_read_info(png_ptr, info_ptr);
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, int_p_NULL, int_p_NULL);
  png_set_strip_16(png_ptr);
  png_set_packing(png_ptr);

  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png_ptr);

  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) 
    png_set_gray_1_2_4_to_8(png_ptr);

  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) 
    png_set_tRNS_to_alpha(png_ptr);

  png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);

  line = (u32*)malloc(width * 4);
  if (!line) 
  {
    png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
    return;
  }

  for (y = 0; y < height; y++)
  {
    png_read_row(png_ptr, (u8*) line, png_bytep_NULL);
    for (x = 0; x < width; x++)
    {
      ((unsigned int*)g_pixelBuffer)[y*width+x] = line[x];
    }
  }

  free(line);

  png_read_end(png_ptr, info_ptr);
  png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

  sceKernelDcacheWritebackInvalidateAll();
}





/*
  decodingAndDrawingThread
  ---------------------------------------------------
  Thread for decoding and drawing the last frame.
  ---------------------------------------------------
*/
void compressDecodingAndDrawingThread(SceSize args, void *argp)
{
  comFrameHeader* frameHeader = (comFrameHeader*)argp;

  if (frameHeader->flags & COM_FLAGS_IMAGE_CLEAR_SCREEN)
  {
    compressClearBuffers();

    if (!g_menuActive)
      graphicClearScreen(0xFF000000);
  }
  else
  {
    l_buffersCleared = false;

    compressSwitchTargetBuffer();

    // Decode jpeg
    if (frameHeader->flags & COM_FLAGS_IMAGE_IS_UNCOMPRESSED)
    {
      compressConvertBmp(frameHeader);
    }
    else if (frameHeader->flags & COM_FLAGS_IMAGE_IS_JPEG)
    {
      compressDecompressJpeg(frameHeader);
    }
    else if (frameHeader->flags & COM_FLAGS_IMAGE_IS_PNG)
    {
      compressDecompressPng(frameHeader);
    }

    // Draw image
    graphicDrawFrame((unsigned int*)g_pixelBuffer, (frameHeader->flags & COM_FLAGS_IMAGE_ROTATION_MASK), false);
  }

  // Don't draw when the menu is showing
  if (g_menuActive)
    return;

  // Draw danzeff keyboard on top of the frame
  g_oskDanzeffOpenOnSelect = (frameHeader->flags & COM_FLAGS_ENABLE_OSK);
  if (g_oskDanzeffActive)
    graphicDrawDanzeff();

  graphicSwitchBuffers();
}





/*
  compressSwitchTargetBuffer
  ---------------------------------------------------
  Switches between the two target buffers for the
  decompression.
  ---------------------------------------------------
*/
void compressSwitchTargetBuffer()
{
  g_pixelBuffer = (g_pixelBuffer == l_pixelBuffer_1) ? l_pixelBuffer_2 : l_pixelBuffer_1;
}





/*
  Dummy functions for the libjpeg custom source
  manager. They are not needed because the whole
  file is made available to libjpeg at once.
*/
void libjpeg_init_source(j_decompress_ptr cinfo)
{
  return;
}


int libjpeg_fill_input_buffer(j_decompress_ptr cinfo)
{
  return TRUE;
}


void libjpeg_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
  return;
}


void libjpeg_term_source(j_decompress_ptr cinfo)
{
  return;
}


boolean libjpeg_resync_to_restart(j_decompress_ptr cinfo, int desired)
{
  return TRUE;
}
