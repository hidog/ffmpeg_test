#ifndef IMAGE_PROCESS_H
#define IMAGE_PROCESS_H



class ImageProcess
{
public:
    ImageProcess();
    ~ImageProcess();

    ImageProcess( const ImageProcess& ) = delete;
    ImageProcess( ImageProcess&& ) = delete;

    ImageProcess& operator = ( const ImageProcess& ) = delete;
    ImageProcess& operator = ( ImageProcess&& ) = delete;

    int     process();

private:

};



ImageProcess*   get_image_process_instance();


#endif