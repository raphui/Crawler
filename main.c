#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <tidy/tidy.h>
#include <tidy/buffio.h>

FILE *reader = NULL;
FILE *urlFile = NULL;

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

void openFile( void )
{
    urlFile = fopen("/tmp/urlFile.txt" , "w+");
    reader = fopen("/tmp/urlFile.txt" , "r");

    if( urlFile == NULL || reader == NULL )
    {
        printf("Cannot open file parse().\n");

        exit( 1 );
    }
}

void parse( TidyDoc doc , TidyNode node )
{
    TidyNode child;

    char str[] = "a";

    for( child = tidyGetChild( node ) ; child ; child = tidyGetNext( child ) )
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
                                fprintf( urlFile , "%s\n" , tidyAttrValue( attr ) );

                                //printf("%s\n" , tidyAttrValue( attr ) );
                            }
                        }

                    }
                }
            }

            parse( doc , child );
        }

    }
}

void crawl( const char *url )
{
    CURLcode ret;
    int err;
    char *goturl = ( char * )malloc( 255 * sizeof( char ) );

    memset( goturl , 0 , 255 );

    curl_easy_setopt( handle , CURLOPT_URL , url );
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
                    if( urlFile == NULL && reader == NULL )
                        openFile();

                    parse( tdoc , tidyGetRoot( tdoc ) );

                    while( fgets( goturl , 255 , reader ) != NULL )
                    {
                        printf("URL got : %s\n" , goturl );

                        crawl( goturl );
                    }

                    fclose( urlFile );
                    fclose( reader );
                }
            }
        }
    }
}

int main( void )
{
    tdoc = tidyCreate();

    tidyOptSetBool( tdoc , TidyForceOutput , yes );
    tidyOptSetInt( tdoc , TidyWrapLen , 4096 );
    tidySetErrorBuffer( tdoc , &errbuff );
    tidyBufInit( &docbuff );

    curl_global_init( CURL_GLOBAL_ALL );

    handle = curl_easy_init();

    crawl("http://www.racerxonline.com/");

    curl_easy_cleanup( handle );

    curl_global_cleanup();

    tidyBufFree( &docbuff );
    tidyBufFree( &errbuff );
    tidyRelease( tdoc );

    printf("\n");
    return 0;
}

