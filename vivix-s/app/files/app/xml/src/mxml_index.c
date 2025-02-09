/*
   Mini XML lib

   Document routines

   Author: Giancarlo Niccolai <gian@niccolai.ws>

   $Id: mxml_index.c,v 1.1.1.1 2003/06/26 21:59:30 jonnymind Exp $
*/

#include <mxml.h>
#include <stdio.h> // for NULL
#include <string.h>            // memset() memset/memcpy
#include <stdlib.h>            // malloc(), free(),exit(), EXIT_SUCCESS rand/rand 

MXML_INDEX *mxml_index_new()
{
   MXML_INDEX *index = (MXML_INDEX *) MXML_ALLOCATOR( sizeof( MXML_INDEX ) );

   if ( index != NULL )
      mxml_index_setup( index );

   return index;
}

MXML_STATUS mxml_index_setup( MXML_INDEX *index )
{
   index->length = 0;
   index->allocated = 0;
   index->data = NULL;

   return MXML_STATUS_OK;
}

void mxml_index_destroy( MXML_INDEX *index )
{
   /* Posix free() can be used with NULL, but nothing is known for other
      free() provided by users */
   if ( index->data != NULL )
      MXML_DELETOR( index->data );

   MXML_DELETOR( index );
}
