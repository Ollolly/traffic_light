#include <iostream>
#include <unistd.h>

#include "traffic_light.hpp"

using namespace traffic_light;

int main()
{
    //traffic light's data
    size_t data[10] = {1,1,2,2,3,3};
    size_t traffic_ligths_amount = 3;
    const char* file_path = "green.txt";
    
    Mng_Traffic_Lights tl_mng(file_path, traffic_ligths_amount, data);

    //tl_mng will run 30 seconds
    sleep(30);

} //main

