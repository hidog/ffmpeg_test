#include "image_process.h"

#include <math.h>
#include <opencv2/imgproc.hpp>



namespace {

ImageProcess    image_process;

}  // end ananymous namespace





/*******************************************************************************
get_image_process_instance
********************************************************************************/
ImageProcess*   get_image_process_instance()
{
    return  &image_process;
}








/*******************************************************************************
ImageProcess::ImageProcess()
********************************************************************************/
ImageProcess::ImageProcess()
{}





/*******************************************************************************
ImageProcess::~ImageProcess()
********************************************************************************/
ImageProcess::~ImageProcess()
{}





/*******************************************************************************
ImageProcess::histogram()
********************************************************************************/
void     ImageProcess::histogram( cv::Mat yuvframe, int width, int height )
{
    cv::Mat     htg_image;
    yuvframe.copyTo(htg_image);

    // do histogram
    int     size    =   width * height;
    int     sum     =   0;
    uchar*  ptr     =   htg_image.ptr();

    const uchar const*  end     =   ptr + size;

    int     histogram[256]  =   {0};
    int     mapping[256]    =   {0};

    for( ; ptr != end; ++ptr )
        ++histogram[ *ptr ];

    for( int i = 0; i < 256; i++ )
    {
        sum         +=  histogram[i];
        mapping[i]  =   std::round( 255.0 * sum / size );
    }

    for( ptr = htg_image.ptr(); ptr != end; ++ptr )    
        *ptr    =   mapping[ *ptr ];
    

    cv::Mat     original, after_histogram;
    cv::cvtColor( yuvframe,  original,        cv::COLOR_YUV2BGR_I420 );
    cv::cvtColor( htg_image, after_histogram, cv::COLOR_YUV2BGR_I420 );

    cv::imshow( "original", original );
    //cv::resizeWindow( cv::String("original"), cv::Size(1280,720) );

    cv::imshow( "after histogram equalization", after_histogram );
    //cv::resizeWindow( cv::String("after histogram equalization"), cv::Size(1280,720) );


    cv::waitKey(1);
    
}
