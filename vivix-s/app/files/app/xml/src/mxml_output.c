/*
   Mini XML lib

   Virtual stream output routines

   Author: Giancarlo Niccolai <gian@niccolai.ws>

   $Id: mxml_output.c,v 1.3 2003/07/24 20:27:40 jonnymind Exp $
*/


#include <mxml.h>
#include <stdio.h>
#include <string.h>            // memset() memset/memcpy
#include <stdlib.h>            // malloc(), free(),exit(), EXIT_SUCCESS rand/rand 
#include <unistd.h>            // read(), write()

/**
* Creates a new output object
* In this case, the func member is required.
* Node count is optional, but highly wanted for progress indicators.
*/
MXML_OUTPUT *mxml_output_new( MXML_OUTPUT_FUNC func, int node_count)
{
   MXML_OUTPUT * ret = (MXML_OUTPUT* ) MXML_ALLOCATOR( sizeof( MXML_OUTPUT ) );

   if ( ret == NULL )
      return NULL;

   if ( mxml_output_setup( ret, func, node_count ) == MXML_STATUS_OK )
      return ret;

   MXML_DELETOR( ret );
   return NULL;
}

/**
* Sets up output parameters.
* In this case, the func member is required.
* Node count is optional, but highly wanted for progress indicators.
*/

MXML_STATUS mxml_output_setup( MXML_OUTPUT *out, MXML_OUTPUT_FUNC func, int node_count)
{
   if ( func == NULL ) {
      return MXML_STATUS_ERROR;
   }

   out->output_func = func;
   out->node_count = node_count;
   out->node_done = 0;

   out->status = MXML_STATUS_OK;
   out->error = MXML_ERROR_NONE;
   return MXML_STATUS_OK;
}

void mxml_output_destroy( MXML_OUTPUT *out )
{
   if ( out != NULL )
   {
      MXML_DELETOR( out );
   }
}

/**********************************************/
/* output functions                           */

MXML_STATUS mxml_output_char( MXML_OUTPUT *out, int c )
{
   char chr = (char) c;
   out->output_func( out, &chr, 1 );
   return out->status;
}

MXML_STATUS mxml_output_string_len( MXML_OUTPUT *out, char *s, int len )
{
   out->output_func( out, s, len );
   return out->status;
}

MXML_STATUS mxml_output_string( MXML_OUTPUT *out, char *s )
{
   return mxml_output_string_len( out, s, strlen( s ) );
}

MXML_STATUS mxml_output_string_escape( MXML_OUTPUT *out, char *s )
{
   while ( *s ) {
      switch ( *s ) {
         case '"': mxml_output_string_len( out, (char*)"&quot;", 6 ); break;
         case '\'': mxml_output_string_len( out, (char*)"&apos;", 6 ); break;
         case '&': mxml_output_string_len( out, (char*)"&amp;", 5 ); break;
         case '<': mxml_output_string_len( out, (char*)"&lt;", 4 ); break;
         case '>': mxml_output_string_len( out, (char*)"&gt;", 4 ); break;
         default: mxml_output_char( out, *s );
      }

      if ( out->status != MXML_STATUS_OK ) break;
      s++;
   }

   return out->status;
}


/**
* Useful function to output to streams
*/

void mxml_output_func_to_stream( MXML_OUTPUT *out, char *s, int len )
{
   FILE *fp = (FILE *) out->data;

   if ( len == 1 )
      fputc( *s, fp );
   else
      fwrite( s, 1, len, fp );

   if ( ferror( fp ) ) {
      out->status = MXML_STATUS_ERROR;
      out->error = MXML_ERROR_IO;
   }
}

/**
* Useful function to output to file handles
*/
void mxml_output_func_to_handle( MXML_OUTPUT *out, char *s, int len )
{
   int fh = (intptr_t) out->data;
   int olen;
   olen = write( fh, s, len );

   if ( olen < len ) {
      out->status = MXML_STATUS_ERROR;
      out->error = MXML_ERROR_IO;
   }
}

/**
* Useful function to output to self growing strings
*/
void mxml_output_func_to_sgs( MXML_OUTPUT *out, char *s, int len )
{
   MXML_SGS *sgs = (MXML_SGS *) out->data;
   MXML_STATUS stat;

   if ( len == 1 )
      stat = mxml_sgs_append_char( sgs, *s );
   else
      stat = mxml_sgs_append_string_len( sgs, s, len );

   if ( stat != MXML_STATUS_OK ) {
      out->status = MXML_STATUS_ERROR;
      out->error = MXML_ERROR_NOMEM;
   }
}
