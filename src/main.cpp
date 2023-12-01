#include "kxhttp.h"

int main(int argc, char ** argv)
{
    CLI::App app("KxHTTP");
    KxHTTP::RequestData request;

    std::string methodStr;
    const std::string customHelpMessage =
            "KxHTTP " + std::string(KXHTTP_VER) + "\n"
            "Usage: kxh [HTTP Method] [URL] [Options...]\n\n"
            "HTTP Methods:\n"
            "  GET, POST, PUT, DELETE, PATCH, OPTIONS, HEAD\n\n"
            "Options:\n"
            "  -f, --form [data]         Send form data (e.g., -f \"name=John\" -f \"lastname=Doe\")\n"
            "  --form-file [file]        Form file upload (e.g., --form-file \"photo=/path/to/photo.png\")\n"
            "  -j, --json [data]         Send raw JSON data (e.g., -j {\"key\": \"value\"}\n"
            "  --json-file [file]        Upload a JSON file (e.g., --json-file \"/path/data.json\")\n"
            "  -H, --headers [headers]   Send custom headers (e.g., -H \"Content-Type: application/json\")\n"
            "  -c, --cookies [cookies]   Send custom cookies (e.g., -c \"sessionid=xyz\")\n"
            "  -a, --auth [credentials]  Authentication (e.g., -a \"username:password\")\n"
            "  -o, --output [file]       Save output to a file (e.g., -o \"output.txt\")\n\n"
            "Example Usage:\n"
            "  kxh GET https://api.example.com -o response.txt\n"
            "  kxh POST https://api.example.com -j {\"name\": \"John\"}\n";

    app.set_version_flag("-v, --version", KXHTTP_VER);
    app.add_option("[Method]", methodStr, "HTTP method (GET, POST,...)")->required();
    app.add_option("[URL]", request.url, "URL to send the request to")->required();
    app.add_option("-f,--form", request.formData, "Send form data");
    app.add_option("--form-file", request.formFiles, "Form file uploads");
    app.add_option("-j,--json", request.jsonData, "Send raw JSON data");
    app.add_option("--json-file", request.jsonFiles, "Upload JSON files");
    app.add_option("-H,--headers", request.headers, "Send custom headers");
    app.add_option("-c,--cookies", request.cookies, "Send custom cookies");
    app.add_option("-a,--auth", request.authData, "Authentication ex. -a \"username:passwd\"");
    app.add_option("-o,--output", request.outputFile, "Save output to a file");

    app.set_help_flag(); // Disable default help flag
    app.add_flag_callback("-h,--help", [&app, &customHelpMessage]() {
        std::cout << customHelpMessage << std::endl;
        exit(0);  // Exit after displaying help
    }, "Show help message");


    try {
        app.parse(argc, argv);
        request.method = KxHTTP::stringToMethod(methodStr);
    } catch (const CLI::ParseError &e) {
        std::cerr << KXHTTP_CONSOLE_YELLOW << "Parsing Error: " << e.what() << KXHTTP_CONSOLE_RESET;
    } catch (const std::exception &e) {
        std::cerr << KXHTTP_CONSOLE_RED << "Fatal Error: " << e.what() << KXHTTP_CONSOLE_RESET;
    }

    //! DO: Process request and handle result functions called here
    KxHTTP::HTTPRequest rq(request);
    rq.sendRequest();
    rq.processResponse();

    return 0;
}

//
// HTTPRequest Class Implementations
//

void KxHTTP::HTTPRequest::sendRequest()
{
    // Process RequestData members individually
    // Send request, print errors if any
    // Then, processResponse() handles the output for that request
}

KxHTTP::HTTPRequest::HTTPRequest(KxHTTP::RequestData& rd)
{
    this->requestData = rd;
}

void KxHTTP::HTTPRequest::processResponse()
{
    // Format and output the appropriate data
}

KxHTTP::HTTPRequest::~HTTPRequest() = default;

//
// Utility Functions
//

KxHTTP::Method KxHTTP::stringToMethod(std::string& m)
{
    if (m == "GET") return KxHTTP::HTTP_GET;
    if (m == "POST") return KxHTTP::HTTP_POST;
    if (m == "PUT") return KxHTTP::HTTP_PUT;
    if (m == "DELETE") return KxHTTP::HTTP_DELETE;
    if (m == "PATCH") return KxHTTP::HTTP_PATCH;
    if (m == "OPTIONS") return KxHTTP::HTTP_OPTIONS;
    if (m == "HEAD") return KxHTTP::HTTP_HEAD;
    // Failsafe, return GET as default method
    return HTTP_GET;
}
