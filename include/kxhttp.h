#ifndef KXHTTP_H
#define KXHTTP_H

#include <iostream>
#include <string>

#include "cli11/CLI11.hpp"
#include "httplib/httplib.h"

#define KXHTTP_VER "0.1.0"

// Let's define the colors for CLI output

#define KXHTTP_CONSOLE_RED "\033[91m"
#define KXHTTP_CONSOLE_YELLOW "\033[93m"
#define KXHTTP_CONSOLE_GREEN "\033[92m"
#define KXHTTP_CONSOLE_BLUE "\033[94m"
#define KXHTTP_CONSOLE_RESET "\033[0m"

namespace KxHTTP
{
    enum Method
    {
        HTTP_GET, HTTP_POST, HTTP_PUT, HTTP_DELETE,
        HTTP_PATCH, HTTP_OPTIONS, HTTP_HEAD
    };

    struct RequestData
    {
        Method method;
        std::string url;
        std::vector<std::string> formData;
        std::vector<std::string> formFiles;
        std::vector<std::string> jsonData;
        std::vector<std::string> jsonFiles;
        std::vector<std::string> headers;
        std::vector<std::string> cookies;
        std::string authData;
        std::string outputFile;
    };

    class HTTPRequest
    {
        public:
            explicit HTTPRequest(RequestData& rd);
            ~HTTPRequest();
            void sendRequest();
            void processResponse();

        private:
            RequestData requestData;
            httplib::Result result;
    };

    Method stringToMethod(std::string& m);
}


#endif // KXHTTP_H