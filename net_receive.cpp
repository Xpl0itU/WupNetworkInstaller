#include "net_receive.h"
#include <cstdio>
#include <curl/curl.h>

static size_t write_data(void* ptr, size_t size, size_t nmemb, void* stream) {
    size_t written = fwrite(ptr, size, nmemb, static_cast<FILE*>(stream));
    return written;
}

int downloadFile(const std::string &remoteUrl, const std::string &localPath) {
    int rc = 0;
    CURLcode res;

    CURL *curl = curl_easy_init();
    if (curl == nullptr) {
        return -1;
    }

    FILE *fp = fopen(localPath.c_str(), "wb");
    if (!fp) {
        rc = -1;
        goto downloadCleanup;
    }

    curl_easy_setopt(curl, CURLOPT_URL, remoteUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // follow redirects
    curl_easy_setopt(curl, CURLOPT_HTTPPROXYTUNNEL, 1L); // corp. proxies etc.

    res = curl_easy_perform(curl);
    if (!res) {
        rc = -1;
        goto downloadCleanup;
    }

    downloadCleanup:
    if (fp != nullptr) {
        fclose(fp);
    }
    curl_easy_cleanup(curl);
    return rc;
}
