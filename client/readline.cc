/*
   Copyright (c) 2022, 2024, Oracle and/or its affiliates.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1335  USA
*/

/* readline for batch mode */

#include <my_global.h>
#include <my_sys.h>
#include <m_string.h>
#include <my_dir.h>
#include "my_readline.h"

static bool init_line_buffer(LINE_BUFFER *buffer,File file,ulong size,
			    ulong max_size);
static bool init_line_buffer_from_string(LINE_BUFFER *buffer,char * str);
static size_t fill_buffer(LINE_BUFFER *buffer);
static char *intern_read_line(LINE_BUFFER *buffer, ulong *out_length);


LINE_BUFFER *batch_readline_init(ulong max_size,FILE *file)
{
  LINE_BUFFER *line_buff;

#ifndef _WIN32
  MY_STAT input_file_stat;
  if (my_fstat(fileno(file), &input_file_stat, MYF(MY_WME)) ||
      MY_S_ISDIR(input_file_stat.st_mode) ||
      MY_S_ISBLK(input_file_stat.st_mode))
    return 0;
#endif

  if (!(line_buff=(LINE_BUFFER*)
        my_malloc(PSI_NOT_INSTRUMENTED, sizeof(*line_buff),
                  MYF(MY_WME | MY_ZEROFILL))))
    return 0;
  if (init_line_buffer(line_buff,my_fileno(file),IO_SIZE,max_size))
  {
    my_free(line_buff);
    return 0;
  }
  return line_buff;
}


char *batch_readline(LINE_BUFFER *line_buff, bool binary_mode)
{
  char *pos;
  ulong UNINIT_VAR(out_length);

  if (!(pos=intern_read_line(line_buff, &out_length)))
    return 0;
  if (out_length && pos[out_length-1] == '\n')
  {
    /* Remove '\n' */
    if (--out_length && !binary_mode && pos[out_length-1] == '\r')
      /* Remove '\r' */
      out_length--;                                 
  }
  line_buff->read_length=out_length;
  pos[out_length]=0;
  return pos;
}


void batch_readline_end(LINE_BUFFER *line_buff)
{
  if (line_buff)
  {
    my_free(line_buff->buffer);
    my_free(line_buff);
  }
}


LINE_BUFFER *batch_readline_command(LINE_BUFFER *line_buff, char * str)
{
  if (!line_buff)
    if (!(line_buff=(LINE_BUFFER*)
          my_malloc(PSI_NOT_INSTRUMENTED, sizeof(*line_buff),
                    MYF(MY_WME | MY_ZEROFILL))))
      return 0;
  if (init_line_buffer_from_string(line_buff,str))
  {
    my_free(line_buff);
    return 0;
  }
  return line_buff;
}


/*****************************************************************************
      Functions to handle buffered readings of lines from a stream
******************************************************************************/

static bool
init_line_buffer(LINE_BUFFER *buffer,File file,ulong size,ulong max_buffer)
{
  buffer->file=file;
  buffer->bufread=size;
  buffer->max_size=max_buffer;
  if (!(buffer->buffer = (char*) my_malloc(PSI_NOT_INSTRUMENTED,
                                    buffer->bufread+1, MYF(MY_WME | MY_FAE))))
    return 1;
  buffer->end_of_line=buffer->end=buffer->buffer;
  buffer->buffer[0]=0;				/* For easy start test */
  return 0;
}

/*
  init_line_buffer_from_string can be called on the same buffer
  several times. the resulting buffer will contain a
  concatenation of all strings separated by spaces
*/
static bool init_line_buffer_from_string(LINE_BUFFER *buffer,char * str)
{
  uint old_length=(uint)(buffer->end - buffer->buffer);
  uint length= (uint) strlen(str);
  if (!(buffer->buffer= buffer->start_of_line= buffer->end_of_line=
	(char*) my_realloc(PSI_NOT_INSTRUMENTED, buffer->buffer,
                           old_length+length+2, MYF(MY_FAE|MY_ALLOW_ZERO_PTR))))
    return 1;
  buffer->end= buffer->buffer + old_length;
  if (old_length)
    buffer->end[-1]=' ';
  memcpy(buffer->end, str, length);
  buffer->end[length]= '\n';
  buffer->end[length+1]= 0;
  buffer->end+= length+1;
  buffer->eof=1;
  buffer->max_size=1;
  return 0;
}


/*
  Fill the buffer retaining the last n bytes at the beginning of the
  newly filled buffer (for backward context).	Returns the number of new
  bytes read from disk.
*/

static size_t fill_buffer(LINE_BUFFER *buffer)
{
  size_t read_count;
  uint bufbytes= (uint) (buffer->end - buffer->start_of_line);

  if (buffer->eof)
    return 0;					/* Everything read */

  /* See if we need to grow the buffer. */

  for (;;)
  {
    uint start_offset=(uint) (buffer->start_of_line - buffer->buffer);
    read_count=(buffer->bufread - bufbytes)/IO_SIZE;
    if ((read_count*=IO_SIZE))
      break;
    if (buffer->bufread * 2 > buffer->max_size)
    {
      /*
        So we must grow the buffer but we cannot due to the max_size limit.
        Return 0 w/o setting buffer->eof to signal this condition.
      */
      return 0;
    }
    buffer->bufread *= 2;
    if (!(buffer->buffer = (char*) my_realloc(PSI_NOT_INSTRUMENTED,
                                              buffer->buffer, buffer->bufread+1,
					      MYF(MY_WME | MY_FAE))))
    {
      buffer->error= my_errno;
      return (size_t) -1;
    }
    buffer->start_of_line=buffer->buffer+start_offset;
    buffer->end=buffer->buffer+bufbytes;
  }

  /* Shift stuff down. */
  if (buffer->start_of_line != buffer->buffer)
  {
    bmove(buffer->buffer,buffer->start_of_line,(uint) bufbytes);
    buffer->end=buffer->buffer+bufbytes;
  }

  /* Read in new stuff. */
  if ((read_count= my_read(buffer->file, (uchar*) buffer->end, read_count,
			   MYF(MY_WME))) == MY_FILE_ERROR)
  {
    buffer->error= my_errno;
    return (size_t) -1;
  }

  DBUG_PRINT("fill_buff", ("Got %lu bytes", (ulong) read_count));

  if (!read_count)
  {
    buffer->eof = 1;
    /* Kludge to pretend every nonempty file ends with a newline. */
    if (bufbytes && buffer->end[-1] != '\n')
    {
      read_count = 1;
      *buffer->end = '\n';
    }
  }
  buffer->end_of_line=(buffer->start_of_line=buffer->buffer)+bufbytes;
  buffer->end+=read_count;
  *buffer->end=0;				/* Sentinel */
  return read_count;
}


char *intern_read_line(LINE_BUFFER *buffer, ulong *out_length)
{
  char *pos;
  size_t length;
  DBUG_ENTER("intern_read_line");

  buffer->start_of_line=buffer->end_of_line;
  for (;;)
  {
    pos=buffer->end_of_line;
    while (*pos != '\n' && pos != buffer->end)
      pos++;
    if (pos == buffer->end)
    {
      /*
        fill_buffer() can return NULL on EOF (in which case we abort),
        on error, or when the internal buffer has hit the size limit.
        In the latter case return what we have read so far and signal
        string truncation.
      */
      if (!(length= fill_buffer(buffer)))
      {
        if (buffer->eof)
          DBUG_RETURN(0);
      }
      else if (length == (size_t) -1)
        DBUG_RETURN(NULL);
      else
        continue;
      pos--;					/* break line here */
      buffer->truncated= 1;
    }
    else
      buffer->truncated= 0;
    buffer->end_of_line=pos+1;
    *out_length=(ulong) (pos + 1 - buffer->eof - buffer->start_of_line);
    DBUG_RETURN(buffer->start_of_line);
  }
}
