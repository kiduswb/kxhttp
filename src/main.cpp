#include <iostream>
#include <fstream>
#include <string>
#include <vector>

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
            "  -c, --cookies [cookies]   Send custom cookies (e.g., -c \"value=xyz\")\n"
            "  -a, --auth [credentials]  Basic Authentication (e.g., -a \"username:password\")\n"
            "  --auth-digest [credentials]  Digest Authentication (e.g., --auth-digest \"username:password\")\n"
            "  --auth-token [credentials]  Bearer Token Authentication (e.g., --auth-token \"token\")\n"
            "  -o, --output [file]       Save output to a file (e.g., -o \"output.txt\")\n\n"
            "Example Usage:\n"
            "  kxh GET https://api.example.com -o response.txt\n"
            "  kxh POST https://api.example.com -j {\"name\": \"John\"}\n";

    app.set_version_flag("-v, --version", KXHTTP_VER);
    app.add_option("HTTP Method", methodStr, "HTTP method (GET, POST,...)")->required();
    app.add_option("URL", request.url, "URL to send the request to")->required();
    app.add_option("-f,--form", request.formData, "Send form data");
    app.add_option("--form-file", request.formFiles, "Form file uploads");
    app.add_option("-j,--json", request.jsonData, "Send raw JSON data");
    app.add_option("--json-file", request.jsonFile, "Upload a JSON file");
    app.add_option("-H,--headers", request.headers, "Send custom headers");
    app.add_option("-c,--cookies", request.cookies, "Send custom cookies");
    app.add_option("-a,--auth", request.authData, "Basic Authentication");
    app.add_option("--auth-digest", request.authDigest, "Digest Authentication");
    app.add_option("--auth-token", request.authBearerToken, "Bearer Token Authentication");
    app.add_option("-o,--output", request.outputFile, "Save output to a file");

    // Overriding CLI11's help message
    app.set_help_flag();
    app.add_flag_callback("-h,--help", [&customHelpMessage]() {
        std::cout << customHelpMessage << std::endl;
        exit(0);
    }, "Show help message");

    try {
        CLI11_PARSE(app, argc, argv);
        request.method = KxHTTP::stringToMethod(methodStr);
    } catch (const CLI::ParseError &e) {
        std::cerr << KXHTTP_CONSOLE_RED << "Parsing Error: " << e.what() << KXHTTP_CONSOLE_RESET;
    } catch (const std::exception &e) {
        std::cerr << KXHTTP_CONSOLE_RED << "Fatal Error: " << e.what() << KXHTTP_CONSOLE_RESET;
    }

    try {
        KxHTTP::HTTPRequest rq(request);
        rq.sendRequest();
        rq.processResponse();
    } catch(const std::exception &e) {
        std::cerr << KXHTTP_CONSOLE_RED << "Error: " << e.what() << KXHTTP_CONSOLE_RESET;
    }

    return 0;
}

//
// HTTPRequest Class Implementations
//

KxHTTP::HTTPRequest::HTTPRequest(KxHTTP::RequestData& rd)
{
    this->requestData = rd;
    this->fileOutputStatus = false;
    this->authTypeDefined = false;
}

void KxHTTP::HTTPRequest::sendRequest()
{
    // Process RequestData members individually
    // Send request, print errors if any
    // Then, processResponse() handles the output for that request

    auto *cli = new httplib::Client(KxHTTP::getProtocolAndDomain(this->requestData.url));

    switch (this->requestData.method)
    {
        case HTTP_GET:
            this->sendGET(cli);
            break;
        case HTTP_POST:
            this->sendPOST(cli);
            break;
        case HTTP_PUT:
            this->sendPUT(cli);
            break;
        case HTTP_DELETE:
            this->sendDELETE(cli);
            break;
        case HTTP_PATCH:
            this->sendPATCH(cli);
            break;
        case HTTP_OPTIONS:
            this->sendOPTIONS(cli);
            break;
        case HTTP_HEAD:
            this->sendHEAD(cli);
            break;
    }

    delete cli;
}

void KxHTTP::HTTPRequest::sendGET(httplib::Client *cli)
{
    httplib::Headers headers = constructHeaders();
    setAuth(cli);

    this->result = cli->Get(getPathFromUrl(this->requestData.url), headers);
    this->handleFileOutput();
}

void KxHTTP::HTTPRequest::sendPOST(httplib::Client *cli)
{
    httplib::Headers headers = constructHeaders();
    setAuth(cli);
    std::string path = getPathFromUrl(this->requestData.url);

    // Handle JSON data or JSON file upload
    // Prioritizes JSON flags over form data flags

    if (!this->requestData.jsonData.empty()) {
        headers.emplace("Content-Type", "application/json");
        std::string jsonBody = this->requestData.jsonData[0];
        this->result = cli->Post(path, headers, jsonBody, "application/json");
    } else if (!this->requestData.jsonFile.empty()) {
        std::ifstream jsonFile(this->requestData.jsonFile);
        if (jsonFile) {
            std::string jsonContent((std::istreambuf_iterator<char>(jsonFile)), std::istreambuf_iterator<char>());
            headers.emplace("Content-Type", "application/json");
            this->result = cli->Post(path, headers, jsonContent, "application/json");
        } else {
            throw std::runtime_error("Failed to open JSON file: " + this->requestData.jsonFile);
        }
    }

    // Multipart/form-data POST request (files and/or form data)
    else
    {
        httplib::MultipartFormDataItems items;

        // Add form files to multipart form data
        for (const auto& formFile : this->requestData.formFiles) {
            auto delimiterPos = formFile.find('=');
            if (delimiterPos != std::string::npos) {
                std::string key = formFile.substr(0, delimiterPos);
                std::string filePath = formFile.substr(delimiterPos + 1);
                // Open file and read content
                std::ifstream file(filePath, std::ios::binary);
                if (!file) {
                    throw std::runtime_error("File '" + filePath + "' not found!");
                }

                else
                {
                    std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                    std::string filename = filePath.substr(filePath.find_last_of("/\\") + 1);
                    // Determine MIME type based on file extension (basic implementation)
                    std::string mimeType = "application/octet-stream"; // default MIME type
                    items.push_back({ key, fileContent, filename, mimeType });
                }
            }
        }

        // Add URL encoded form data to multipart form data
        for (const auto& data : this->requestData.formData) {
            auto delimiterPos = data.find('=');
            if (delimiterPos != std::string::npos) {
                std::string key = data.substr(0, delimiterPos);
                std::string value = data.substr(delimiterPos + 1);
                items.push_back({ key, value, "", "" });
            }
        }

        // If there are items to send
        if (!items.empty()) {
            this->result = cli->Post(path, headers, items);
        } else {
            // Simple POST request with no Body
            this->result = cli->Post(path, headers, "", "text/plain");
        }
    }

    this->handleFileOutput();
}

void KxHTTP::HTTPRequest::sendPUT(httplib::Client *cli)
{
    httplib::Headers headers = constructHeaders();
    setAuth(cli);
    std::string path = getPathFromUrl(this->requestData.url);

    if (!this->requestData.jsonData.empty()) {
        headers.emplace("Content-Type", "application/json");
        std::string jsonBody = this->requestData.jsonData[0];
        this->result = cli->Put(path, headers, jsonBody, "application/json");
    }

    else if (!this->requestData.formData.empty()) {
        std::string formBody;
        for (const auto& data : this->requestData.formData) {
            if (!formBody.empty()) {
                formBody += "&";
            }
            formBody += data;
        }
        headers.emplace("Content-Type", "application/x-www-form-urlencoded");
        this->result = cli->Put(path, headers, formBody, "application/x-www-form-urlencoded");
    }

    else
        this->result = cli->Put(path, headers, "", "text/plain");

    handleFileOutput();
}

void KxHTTP::HTTPRequest::sendDELETE(httplib::Client *cli)
{
    httplib::Headers headers = constructHeaders();
    setAuth(cli);
    std::string path = getPathFromUrl(this->requestData.url);

    this->result = cli->Delete(path, headers);
    this->handleFileOutput();
}

void KxHTTP::HTTPRequest::sendPATCH(httplib::Client *cli)
{
    httplib::Headers headers = constructHeaders();
    setAuth(cli);
    std::string path = getPathFromUrl(this->requestData.url);

    if (!this->requestData.jsonData.empty()) {
        headers.emplace("Content-Type", "application/json");
        std::string jsonBody = this->requestData.jsonData[0];
        this->result = cli->Patch(path, headers, jsonBody, "application/json");
    }

    else if (!this->requestData.formData.empty()) {
        std::string formBody;
        for (const auto& data : this->requestData.formData) {
            if (!formBody.empty()) {
                formBody += "&";
            }
            formBody += data;
        }
        headers.emplace("Content-Type", "application/x-www-form-urlencoded");
        this->result = cli->Patch(path, headers, formBody, "application/x-www-form-urlencoded");
    }

    else {
        this->result = cli->Patch(path, headers, "", "text/plain");
    }

    handleFileOutput();
}

void KxHTTP::HTTPRequest::sendOPTIONS(httplib::Client *cli)
{
    httplib::Headers headers = constructHeaders();
    setAuth(cli);
    std::string path = getPathFromUrl(this->requestData.url);

    this->result = cli->Options(path, headers);
    handleFileOutput();
}

void KxHTTP::HTTPRequest::sendHEAD(httplib::Client *cli)
{
    httplib::Headers headers = constructHeaders();
    setAuth(cli);
    std::string path = getPathFromUrl(this->requestData.url);

    this->result = cli->Head(path, headers);
    handleFileOutput();
}

void KxHTTP::HTTPRequest::processResponse() const
{
    std::cout << std::flush;
    std::cout << KXHTTP_CONSOLE_YELLOW << "Sending " << KxHTTP::methodToString(this->requestData.method) << " request to "
              << KXHTTP_CONSOLE_BLUE << this->requestData.url << KXHTTP_CONSOLE_RESET << "\n";

    std::cout << (this->result->status == 200 ? KXHTTP_CONSOLE_GREEN : KXHTTP_CONSOLE_YELLOW) <<
    "Request returned Status Code " << this->result->status << KXHTTP_CONSOLE_RESET << "\n";
    std::cout << "\nHeaders: \n\n";
    for (const auto& header : this->result->headers)
        std::cout << header.first << ": " << header.second << "\n";

    if(this->fileOutputStatus && this->result->status == 200)
        std::cout << KXHTTP_CONSOLE_GREEN << "\nOutput saved to: "
                  << this->requestData.outputFile << KXHTTP_CONSOLE_RESET;
    else
        std::cout << "\nResponse Received:\n\n" << this->result->body << "\n\n";
}

KxHTTP::HTTPRequest::~HTTPRequest() = default;

//
// Utility Functions
//

void KxHTTP::HTTPRequest::setAuth(httplib::Client *cli)
{
    if (!this->requestData.authData.empty() && !this->authTypeDefined)
    {
        auto colonPos = this->requestData.authData.find(':');
        if (colonPos != std::string::npos) {
            cli->set_basic_auth(this->requestData.authData.substr(0, colonPos),
                                this->requestData.authData.substr(colonPos + 1));
        }

        this->authTypeDefined = true;
    }

    else if (!this->requestData.authDigest.empty() && !this->authTypeDefined)
    {
        auto colonPos = this->requestData.authDigest.find(':');
        if (colonPos != std::string::npos) {
            cli->set_digest_auth(this->requestData.authDigest.substr(0, colonPos),
                                 this->requestData.authDigest.substr(colonPos + 1));
        }

        this->authTypeDefined = true;
    }

    else if (!this->requestData.authBearerToken.empty() && !this->authTypeDefined)
    {
        cli->set_bearer_token_auth(this->requestData.authBearerToken);
        this->authTypeDefined = true;
    }
}

httplib::Headers KxHTTP::HTTPRequest::constructHeaders()
{
    httplib::Headers headers;
    for (const auto& header : this->requestData.headers) {
        auto pos = header.find(':');
        if (pos != std::string::npos) {
            headers.emplace(header.substr(0, pos), header.substr(pos + 1));
        }
    }
    for (const auto& cookie : this->requestData.cookies) {
        headers.emplace("Cookie", cookie);
    }
    return headers;
}

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

void KxHTTP::HTTPRequest::handleFileOutput()
{
    if (this->result->status == 200 && !this->requestData.outputFile.empty()) {
        std::ofstream outFile(this->requestData.outputFile, std::ios::binary);
        if (outFile.is_open()) {
            outFile << this->result->body;
            this->fileOutputStatus = true;
        } else {
            throw std::runtime_error("Failed to open " + this->requestData.outputFile + " for writing.\n");
        }
    }
}

std::string KxHTTP::methodToString(KxHTTP::Method m)
{
    switch (m) {
        case KxHTTP::HTTP_GET: return "GET";
        case KxHTTP::HTTP_POST: return "POST";
        case KxHTTP::HTTP_PUT: return "PUT";
        case KxHTTP::HTTP_DELETE: return "DELETE";
        case KxHTTP::HTTP_PATCH: return "PATCH";
        case KxHTTP::HTTP_OPTIONS: return "OPTIONS";
        case KxHTTP::HTTP_HEAD: return "HEAD";
        default: return "UNKNOWN";
    }
}

std::string KxHTTP::getPathFromUrl(const std::string& url)
{
    // Find position after the protocol delimiter (://)
    size_t protocolPos = url.find("://");
    if (protocolPos != std::string::npos) {
        // Start searching for the first '/' after the protocol
        size_t pathStart = url.find('/', protocolPos + 3);
        if (pathStart != std::string::npos) {
            // Return the path and everything after
            return url.substr(pathStart);
        }
    }
    return "/";
}

std::string KxHTTP::getProtocolAndDomain(const std::string& url)
{
    size_t protocolEnd = url.find("://");
    if (protocolEnd != std::string::npos) {
        size_t domainEnd = url.find('/', protocolEnd + 3); // skip "://"
        if (domainEnd != std::string::npos) {
            return url.substr(0, domainEnd);
        } else {
            return url; // No path part found, return the whole URL
        }
    }

    return ""; // No protocol found, return empty string
}


