{
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  compress.pas - wrapper for libjpeg

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
}


unit compress;

interface

uses
  Windows, Graphics, Classes, libJPEG;

const
  OUTPUT_BUF_SIZE = 500 * 1024;

type
  my_dest_mgr_ptr = ^my_dest_mgr;
  my_dest_mgr = record
    pub: jpeg_destination_mgr;
    DestBuffer: array [1..OUTPUT_BUF_SIZE] of byte;
  end;

  my_destination_mgr = record
    pub: jpeg_destination_mgr;
    outfile: TStream;
    Buffer: JOCTET_ptr;
  end;
  my_destination_ptr = ^my_destination_mgr;


function CompressToJPEG(Quality: Word; var InputBuffer: Array of Byte; var OutputBuffer: Array of Byte; Width: LongWord; Height: LongWord): LongWord;
function CompressInitLibrary: Boolean;
procedure CompressShutdownLibrary;


implementation

uses
  graphic;


var
  jc: jpeg_compress_struct;
  jpeg_err: jpeg_error_mgr;



{
  ---------------------------------------------------
  A couple of unused functions.
  ---------------------------------------------------
}
procedure JpegError(cinfo: j_common_ptr);   cdecl;
begin
end;


procedure EmitMessage(cinfo: j_common_ptr; msg_level: Integer); cdecl;
begin
end;

procedure OutputMessage(cinfo: j_common_ptr); cdecl;
begin
end;

procedure FormatMessage(cinfo: j_common_ptr; buffer: PChar); cdecl;
begin
end;

procedure ResetErrorMgr(cinfo: j_common_ptr); cdecl;
begin
end;


procedure init_destination(cinfo: j_compress_ptr); cdecl;
begin
end;


function empty_output_buffer(cinfo: j_compress_ptr): boolean; cdecl;
begin
  Result := True;
end;


procedure term_destination(cinfo: j_compress_ptr); cdecl;
begin
end;




{
  CompressInitLibrary
  ---------------------------------------------------
  Initialize the jpeg library.
  ---------------------------------------------------
  Returns TRUE on success.
}
function CompressInitLibrary: Boolean;
begin
  if not init_libJPEG() then
  begin
    Result := False;
    Exit;
  end;

  Result := True;

  FillChar(jc, sizeof(jc), 0);
  FillChar(jpeg_err, sizeof(jpeg_err), 0);

  jpeg_err.error_exit := JpegError;
  jpeg_err.emit_message := EmitMessage;
  jpeg_err.output_message := OutputMessage;
  jpeg_err.format_message := FormatMessage;
  jpeg_err.reset_error_mgr := ResetErrorMgr;

  jc.err := jpeg_std_error(@jpeg_err);
  jpeg_create_compress(@jc);

  if jc.dest = nil then
  begin
    // allocation space for streaming methods
    jc.dest := jc.mem^.alloc_small(@jc, JPOOL_PERMANENT, SizeOf(my_dest_mgr));

    // seeting up custom functions
    with my_dest_mgr_ptr(jc.dest)^ do begin
      pub.init_destination    := init_destination;
      pub.empty_output_buffer := empty_output_buffer;
      pub.term_destination    := term_destination;

      pub.next_output_byte  := @DestBuffer[1];
      pub.free_in_buffer    := Length(DestBuffer);
    end;
  end;

  jc.input_components := BitmapColorBytes;
  jc.in_color_space := JCS_RGB;

  jc.image_width := 480;
  jc.image_height := 272;
  jpeg_set_defaults(@jc);
end;



{
  CompressShutdownLibrary
  ---------------------------------------------------
  Unload the jpeg library.
  ---------------------------------------------------
}
procedure CompressShutdownLibrary;
begin
  jpeg_destroy(@jc);
  jc.err := nil;

  quit_libJPEG();
end;




{
  CompressToJPEG
  ---------------------------------------------------
  Compress the given bitmap to a jpeg.
  ---------------------------------------------------
  Returns the resulting jpeg file size.
}
function CompressToJPEG(Quality: Word; var InputBuffer: Array of Byte; var OutputBuffer: Array of Byte; Width: LongWord; Height: LongWord): LongWord;
var
  LinesWritten: Integer;
  SrcScanLine: Pointer;
  PtrInc: Integer;

begin
  jc.image_width := Width;
  jc.image_height := Height;
  jc.in_color_space := JCS_EXT_BGR;
  jpeg_set_defaults(@jc);
 // jpeg_set_colorspace(@jc, JCS_RGB);
  jpeg_set_quality(@jc, Quality, True);

  SrcScanline := @InputBuffer[Width * (Height - 1) * 3];//@InputBuffer[0];
  PtrInc := (Width * BitmapColorBytes) * (-1);

  with my_dest_mgr_ptr(jc.dest)^ do begin
      pub.next_output_byte  := @(OutputBuffer[0]);
      pub.free_in_buffer    := OUTPUT_BUF_SIZE;
  end;

  jpeg_start_compress(@jc, True);
  while (jc.next_scanline < jc.image_height) do begin
    LinesWritten := jpeg_write_scanlines(@jc, @SrcScanline, 1);
    Inc(Integer(SrcScanline), PtrInc * LinesWritten);
  end;
  jpeg_finish_compress(@jc);

  Result := OUTPUT_BUF_SIZE - my_dest_mgr_ptr(jc.dest)^.pub.free_in_buffer;
end;


end.
