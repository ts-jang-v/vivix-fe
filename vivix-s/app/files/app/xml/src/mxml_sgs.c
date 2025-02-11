/*
   Mini XML lib

   Self growing string routines

   Author: Giancarlo Niccolai <gian@niccolai.ws>

   $Id: mxml_sgs.c,v 1.1.1.1 2003/06/26 21:59:30 jonnymind Exp $
*/


#include <mxml.h>
#include <string.h>
#include <string.h>            // memset() memset/memcpy
#include <stdlib.h>            // malloc(), free(),exit(), EXIT_SUCCESS rand/rand 

/**
* Creates a new self growing string, with buffer set to
* minimal buffer length
*/

MXML_SGS *mxml_sgs_new()
{
   MXML_SGS * ret = (MXML_SGS* ) MXML_ALLOCATOR( sizeof( MXML_SGS ) );

   if ( ret == NULL )
      return NULL;

   ret->buffer = (char *) MXML_ALLOCATOR( MXML_ALLOC_BLOCK );
   if ( ret->buffer == NULL ) {
      MXML_DELETOR( ret );
      return NULL;
   }

   ret->allocated = MXML_ALLOC_BLOCK;
   ret->buffer[0] = 0;
   ret->length = 0;

   return ret;
}

void mxml_sgs_destroy( MXML_SGS *sgs )
{
   if ( sgs->buffer != NULL )
      MXML_DELETOR( sgs->buffer );

   MXML_DELETOR( sgs );
}

/****************************************/

MXML_STATUS mxml_sgs_append_char( MXML_SGS *sgs, char c )
{
   char *buf;
   sgs->buffer[ sgs->length++ ] = c;

   if ( sgs->length >= sgs->allocated ) {
      buf = (char *) MXML_REALLOCATOR( sgs->buffer, sgs->allocated + MXML_ALLOC_BLOCK );
      if ( buf == NULL ) {
         return MXML_STATUS_ERROR;
      }
      sgs->allocated += MXML_ALLOC_BLOCK;
      sgs->buffer = buf;
   }

   sgs->buffer[sgs->length] = '\0';

   return MXML_STATUS_OK;
}

MXML_STATUS mxml_sgs_append_string_len( MXML_SGS *sgs, char *s, int slen )
{
   char *buf;

   if ( slen == 0 )
      return MXML_STATUS_OK;

   if ( sgs->length + slen >= sgs->allocated ) {
      int blklen = ( ( sgs->length + slen ) / MXML_ALLOC_BLOCK + 1) * MXML_ALLOC_BLOCK;
      buf = (char *) MXML_REALLOCATOR( sgs->buffer, blklen );

      if ( buf == NULL ) {
         return MXML_STATUS_ERROR;
      }
      sgs->allocated = blklen;
      sgs->buffer = buf;
   }

   memcpy( sgs->buffer + sgs->length , s, slen + 1 ); // include also the trailing space
   sgs->length += slen;

   return MXML_STATUS_OK;
}


MXML_STATUS mxml_sgs_append_string( MXML_SGS *sgs, char *s )
{
   return mxml_sgs_append_string_len( sgs, s, strlen( s ) );
}

