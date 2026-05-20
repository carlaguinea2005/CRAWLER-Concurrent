#include "http_status.hpp"

bool is_http_success(long status_code) {
    return status_code >= 200 && status_code < 300;
}

bool is_http_redirect(long status_code) {
    return status_code >= 300 && status_code < 400;
}

bool is_http_client_error(long status_code) {
    return status_code >= 400 && status_code < 500;
}

bool is_http_server_error(long status_code) {
    return status_code >= 500 && status_code < 600;
}