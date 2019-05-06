#ifndef _FILTER_H_
#define _FILTER_H_

#include <string>

/**
 * @brief This namespace refers to implementation of core functionalities from the service
 * 
 */
namespace Core {

    /**
     * @brief Filter class reunite just just statich filtering functions that are used to process request bodies
     * 
     */
    class Filter {
        public:

        /**
         * @brief this function uses OpenCV to apply gaussian filter to an image
         * 
         * @param image_data binary data of the image data
         * @return std::string binary png data of filtered image
         */
        static std::string gaussian(std::string & image_data);
        static std::string adjust(std::string & image_data);
        static std::string clahe(std::string & image_data);
        //void imadjust(const cv::Mat1b& src, cv::Mat1b& dst, int tol = 1, cv::Vec2i in = cv::Vec2i(0, 255), cv::Vec2i out = cv::Vec2i(0, 255));
    };
}

#endif
