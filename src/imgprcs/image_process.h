#ifndef IMAGE_PROCESS_H
#define IMAGE_PROCESS_H

#include <opencv2/highgui.hpp>


class ImageProcess
{
public:
    ImageProcess();
    ~ImageProcess();

    ImageProcess( const ImageProcess& ) = delete;
    ImageProcess( ImageProcess&& ) = delete;

    ImageProcess& operator = ( const ImageProcess& ) = delete;
    ImageProcess& operator = ( ImageProcess&& ) = delete;

    void    histogram( cv::Mat yuvframe, int width, int height );
    void    rgb_to_gray( cv::Mat yuvframe, int width, int height );
    void    sobel( cv::Mat yuvframe, int width, int height );

};



ImageProcess*   get_image_process_instance();


#endif