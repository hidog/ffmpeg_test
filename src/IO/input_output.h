#ifndef INPUT_OUTPUT_H
#define INPUT_OUTPUT_H



class InputOutput
{
public:
    InputOutput();
    virtual ~InputOutput();

    InputOutput( const InputOutput& ) = delete;
    InputOutput( InputOutput&& ) = delete;

    InputOutput& operator = ( const InputOutput& ) = delete;
    InputOutput& operator = ( InputOutput&& ) = delete;

private:
};




#endif