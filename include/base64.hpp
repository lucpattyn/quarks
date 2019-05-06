#ifndef _BASE64_H_
#define _BASE64_H_

#include <string>

/**
 * @brief Helpers namespace aggregates functions that are not part of core functionalities but are involved in some
 * code procedure.
 * 
 */
namespace Helpers {

    /**
     * @brief Base64 encoder and decoder.
     * 
     */
    class Base64 {
        public:

        /**
         * @brief Encode string to base64 string.
         *
         * @param std::string data data to be encoded.
         * @return std::string encoded data.
         */
        static std::string Encode(const std::string &data);

        /**
         * @brief Decode base64 string.
         *
         * @param data Encoded data.
         * @return std::string Decoded data.
         */
        static std::string Decode(const std::string &data);

    };
}

#endif