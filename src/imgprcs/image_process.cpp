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

    const uchar* const  end     =   ptr + size;

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
    cv::imshow( "after histogram equalization", after_histogram );

    cv::waitKey(1);
}




/*******************************************************************************
ImageProcess::rgb_to_gray()
********************************************************************************/
void    ImageProcess::rgb_to_gray( cv::Mat yuvframe, int width, int height )
{
    cv::Mat     bgr_image;
    cv::cvtColor( yuvframe, bgr_image, cv::COLOR_YUV2BGR_I420 );

    cv::Mat     gray_image{ cv::Size(width,height), CV_8UC1 };  // note: 用 gray_image{ width, height, CV_8UC1 } 會被當initialize list造成失敗

    uchar   *src_ptr    =   bgr_image.data;
    uchar   *dst_ptr    =   gray_image.data;

    static int  src_size    =   width * height * 3;
    static int  dst_size    =   width * height;

    const uchar* const  src_end     =   src_ptr + src_size;
    const uchar* const  dst_end     =   dst_ptr + dst_size;

    int     tmp;
    for( ; dst_ptr != dst_end; src_ptr += 3, ++dst_ptr )
    {
        tmp         =   *(src_ptr + 1);
        tmp         =   (tmp << 1) + *src_ptr + *(src_ptr + 2);
        *dst_ptr    =   tmp >> 2;
    }
        
    cv::imshow( "bgr", bgr_image );
    cv::imshow( "gray", gray_image );

    cv::waitKey(1);
}
