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



/*******************************************************************************
ImageProcess::sobel()
********************************************************************************/
void    ImageProcess::sobel( cv::Mat yuvframe, int width, int height )
{
    cv::Mat     gray_image{ cv::Size(width,height), CV_8UC1 };  // note: 用 gray_image{ width, height, CV_8UC1 } 會被當initialize list造成失敗

    uchar   *src_ptr    =   yuvframe.data;
    uchar   *dst_ptr    =   gray_image.data;

    static int  size    =   width * height;

    const uchar* const  src_end     =   src_ptr + size;
    const uchar* const  dst_end     =   dst_ptr + size;

    for( ; dst_ptr != dst_end; ++dst_ptr, ++src_ptr )
        *dst_ptr    =   *src_ptr;

    // start sobel
    cv::Mat     grad_x, grad_y, abs_grad_x, abs_grad_y, grad_sobel_image;

    int     kernel_size     =   1;
    int     scale           =   1;
    int     delta           =   0;

    Sobel( gray_image, grad_x, CV_16S, 1, 0, kernel_size, scale, delta, cv::BORDER_DEFAULT );
    Sobel( gray_image, grad_y, CV_16S, 0, 1, kernel_size, scale, delta, cv::BORDER_DEFAULT );
        
    // converting back to CV_8U
    convertScaleAbs( grad_x, abs_grad_x );
    convertScaleAbs( grad_y, abs_grad_y );
    addWeighted( abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad_sobel_image );

    cv::imshow( "gray", gray_image );
    cv::imshow( "sobel", grad_sobel_image );

    cv::waitKey(1);
}






/*******************************************************************************
ImageProcess::canny_edge()
********************************************************************************/
void    ImageProcess::canny_edge( cv::Mat yuvframe, int width, int height )
{
    cv::Mat     bgr, canny, gray, blur;
    cv::cvtColor( yuvframe, bgr, cv::COLOR_YUV2BGR_I420 );

    int     low_thr     =   60;
    int     ratio       =   3;
    int     kernel_size =   3;
      
    cvtColor( bgr, gray, cv::COLOR_BGR2GRAY );    
    cv::blur( gray, blur, cv::Size(3,3) );

    Canny( blur, canny, low_thr, low_thr*ratio, kernel_size );

    imshow( "edge map", canny );
    imshow( "bgr", bgr );

    cv::waitKey(1);    
}
