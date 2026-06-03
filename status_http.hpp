#ifndef STATUS_HTTP_HPP
#define STATUS_HTTP_HPP

bool is_http_ok(long status_code);
bool is_http_redirect(long status_code);
bool is_http_client_error(long status_code);
bool is_http_server_error(long status_code);

#endif