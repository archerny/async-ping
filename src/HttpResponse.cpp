#include "HttpResponse.h"

HttpResponse::HttpResponse(struct evhttp_request *req, const string &content)
{
  request = req;
  evhttp_add_header(evhttp_request_get_output_headers(req),
                    "Content-Type", "text/html");
  evb = evbuffer_new();
  evbuffer_expand(evb, content.size());
  evbuffer_add(evb, (const void *)content.c_str(), content.size());
  responseCode = 200;
}

HttpResponse::HttpResponse(struct evhttp_request *req, const JsonValue &json)
{
  request = req;
  string content = ""; // serialize
  evhttp_add_header(evhttp_request_get_output_headers(req),
                    "Content-Type", "application/json");
  evb = evbuffer_new();
  evbuffer_expand(evb, content.size());
  evbuffer_add(evb, (const void *)content.c_str(), content.size());
  responseCode = 200;
}

HttpResponse::HttpResponse(struct evhttp_request *req)
{
  request = req;
  evb = evbuffer_new();
  evhttp_add_header(evhttp_request_get_output_headers(req),
                    "Content-Type", "text/html");
  responseCode = 200;
}

HttpResponse::HttpResponse(struct evhttp_request *req, int code)
{
  request = req;
  responseCode = code;
  evb = evbuffer_new();
  if (code == 405)
  {
    evhttp_add_header(evhttp_request_get_output_headers(req),
                      "Allow", "POST");
  }
}

HttpResponse::~HttpResponse()
{
  if (evb)
  {
    evbuffer_free(evb);
    evb = NULL;
  }
}

void HttpResponse::doResponse()
{
  if (responseCode == 200)
  {
    evhttp_send_reply(request, responseCode, "OK", evb);
  }
  else if (responseCode == 405)
  {
    evhttp_send_error(request, responseCode, "Method Not Allowed");
  }
  else if (responseCode == 415)
  {
    evhttp_send_error(request, responseCode, "Unsupported Media Type");
  }
  else
  {
    evhttp_send_error(request, responseCode, "Unknown Error");
  }
}
