#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <tidy/tidy.h>
#include <tidy/buffio.h>

#define HISTORY_SIZE	1024

typedef struct node
{
	unsigned char passed;
	char url[255];
	struct node *childs;
	struct node *siblings;

}node;


node *tree;
node *current;

int current_depth;

typedef struct record
{
	node *n;
	int depth;
}record;

record history[HISTORY_SIZE];

struct node *createTree( void )
{
	node *root = ( node * )malloc( sizeof( node ) );
	root->passed = 1;
	root->childs = NULL;
	root->siblings = NULL;

	return root;
}

struct node *addSiblings( node **n , node *item )
{
	(*n)->siblings = ( node * )malloc( sizeof( node ) );
	memcpy( (*n)->siblings , item , sizeof( node ) );

	return (*n)->siblings;
}

struct node *addChilds( node **n , node *item )
{
	node *tmp;

	if( (*n)->childs != NULL )
	{	
		tmp = (*n)->childs;
		
		if( tmp->siblings != NULL )	
			while( tmp->siblings != NULL )
				tmp = tmp->siblings;
		
	
		tmp->siblings = ( node * )malloc( sizeof( node ) );
		memcpy( tmp->siblings , item , sizeof( node ) );
		tmp->siblings->childs = NULL;
		tmp->siblings->siblings = NULL;
	}
	else
	{
		(*n)->childs = ( node * )malloc( sizeof( node ) );
		memcpy( (*n)->childs , item , sizeof( node ) );
		tmp = (*n)->childs;
		tmp->childs = NULL;
		tmp->siblings = NULL;
	}

	return tmp;
}

void addNodeInHistory( node *t )
{
	static int i = 0;
	record *tmp;

	tmp = &history[i];
	tmp->n = t;
	tmp->depth = current_depth;
	i++;

}

int isNodeInHistory( node *t )
{
	int j = 0;
	int ret = 0;
	record *tmp;

	for( j == 0 ; j < HISTORY_SIZE ; j++ )
	{
		tmp = &history[j];
		
		if( tmp->n == NULL )
			continue;
	
		if( !strcmp( t->url , tmp->n->url ) && ( current_depth >= tmp->depth ) )
		{
			return 1; 
		}
	}

	return 0;
}


CURL *handle = NULL;

TidyDoc tdoc;
TidyBuffer docbuff;
TidyBuffer errbuff;

uint write_cb(char *in, uint size, uint nmemb, TidyBuffer *out)
{
    uint r;

    r = size * nmemb;

    tidyBufAppend( out , in , r );

    return r;
}


void parse( TidyDoc doc , TidyNode n )
{
    TidyNode child;

	node *item;

    char str[] = "a";

    for( child = tidyGetChild( n ) ; child ; child = tidyGetNext( child ) )
    {
        ctmbstr name = tidyNodeGetName( child );

        if( name )
        {
            if( strcmp( ( char * )name , str ) == 0 )
            {
                TidyAttr attr;

                for( attr = tidyAttrFirst( child ) ; attr ; attr = tidyAttrNext( attr ) )
                {

                    if( strcmp( tidyAttrName( attr ) , "href") == 0 )
                    {
                        if( tidyAttrValue( attr )  != NULL )
                        {
                            if( strstr( tidyAttrValue( attr ) , "http://" ) != NULL )
                            {
								item = ( node * )malloc( sizeof( node ) );
								item->passed = 0;
								sprintf( item->url , "%s" , tidyAttrValue( attr ) );

								addChilds( &current , item );
								free( item );
                            }
                        }

                    }
                }
            }

            parse( doc , child );
        }

    }
}

void crawl( node *n )
{
    CURLcode ret;
    static int i = 0;
	//static int depth = 0;
    int err;
	node *tmp;

	current = n;
	addNodeInHistory( n );

	printf("Link number: %d     URL got : %s\n" , i++ , n->url );

    curl_easy_setopt( handle , CURLOPT_URL , n->url );
    curl_easy_setopt( handle , CURLOPT_WRITEFUNCTION , write_cb );
    curl_easy_setopt( handle , CURLOPT_WRITEDATA , &docbuff );

    ret = curl_easy_perform( handle );

    if( ret != 0 )
    {
        curl_easy_cleanup( handle );

        return;
    }
    else
    {
        err = tidyParseBuffer( tdoc , &docbuff );

        if( err >= 0 )
        {
            err = tidyCleanAndRepair( tdoc );

            if( err >= 0 )
            {
                err = tidyRunDiagnostics( tdoc );

                if( err >= 0 )
                {

					while( 1 )//( n->childs != NULL ) || ( n->siblings != NULL ) )
					{

                    	parse( tdoc , tidyGetRoot( tdoc ) );

						if( n->childs == NULL )
						{
							
							tmp = n->siblings;

							if( ( tmp->siblings == NULL ) || ( tmp == NULL ) )
								exit( 1 );
							
							
							while( isNodeInHistory( tmp ) == 1 )
							{
								if( tmp->siblings == NULL )
									break;
								else
									tmp = tmp->siblings;
							}
	
							crawl( tmp );
						}
						else
						{
							if( isNodeInHistory( n->childs ) == 1 )
							{

								tmp = n->childs->siblings;
								
								if( ( tmp->siblings == NULL ) || ( tmp == NULL ) )
									exit( 1 );

								while( isNodeInHistory( tmp ) == 1 )
								{
									if( tmp->siblings == NULL )
										break;
									else
										tmp = tmp->siblings;
								}
							}
							else
							{
								tmp = n->childs;
							}
							
							current_depth++;
							crawl( tmp );//n->childs );
							current_depth--;
						}
					}

				}
            }
        }
    }
}
	

int main( void )
{
	tree = NULL;
	tree = createTree();
	current = tree;		

    tdoc = tidyCreate();

    tidyOptSetBool( tdoc , TidyForceOutput , yes );
    tidyOptSetInt( tdoc , TidyWrapLen , 4096 );
    tidySetErrorBuffer( tdoc , &errbuff );
    tidyBufInit( &docbuff );

    curl_global_init( CURL_GLOBAL_ALL );

    handle = curl_easy_init();
	
	sprintf( tree->url , "http://www.lefigaro.fr" );

    //crawl("http://www.racerxonline.com/");
	//crawl("http://www.lefigaro.fr");
	crawl( tree );

    curl_easy_cleanup( handle );

    curl_global_cleanup();

    tidyBufFree( &docbuff );
    tidyBufFree( &errbuff );
    tidyRelease( tdoc );

    printf("\n");
    return 0;
}

