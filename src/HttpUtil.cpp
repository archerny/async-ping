#include "HttpUtil.h"

string HttpUtil::recvReqInput(struct evhttp_request *req)
{
  struct evbuffer *buf = evhttp_request_get_input_buffer(req);
  char recvbuf[RECV_BUF_SIZE];
  string retData;
  while (evbuffer_get_length(buf))
  {
    int nrecvd = evbuffer_remove(buf, recvbuf, RECV_BUF_SIZE - 1);
    if (nrecvd > 0)
    {
      recvbuf[nrecvd] = '\0';
      retData += recvbuf;
    }
  }
  return retData;
}

JsonValue *HttpUtil::recvReqInputJson(struct evhttp_request *req)
{
  struct evbuffer *buf = evhttp_request_get_input_buffer(req);
  char recvbuf[RECV_BUF_SIZE];
  string retData;
  while (evbuffer_get_length(buf))
  {
    int nrecvd = evbuffer_remove(buf, recvbuf, RECV_BUF_SIZE - 1);
    if (nrecvd > 0)
    {
      recvbuf[nrecvd] = '\0';
      retData += recvbuf;
    }
  }
  JsonValue *jv = new JsonValue();
  if (jv->parse(retData))
  {
    return jv;
  }
  else
  {
    // parse json value error, log it
    delete jv;
    return NULL;
  }
}

bool HttpUtil::sendStringResp(struct evhttp_request *req, string &content)
{
  struct evbuffer *evb = NULL;
  evhttp_add_header(evhttp_request_get_output_headers(req),
                    "Content-Type", "text/html");
  evb = evbuffer_new();
  evbuffer_expand(evb, content.size());
  evbuffer_add(evb, (const void *)content.c_str(), content.size());
  evhttp_send_reply(req, 200, "OK", evb);
  evbuffer_free(evb);
  return true;
}

bool HttpUtil::sendJsonResp(struct evhttp_request *req, JsonValue &jv)
{
  string serialized = "";
  struct evbuffer *evb = NULL;
  ;
  evhttp_add_header(evhttp_request_get_output_headers(req),
                    "Content-Type", "application/json");
  evb = evbuffer_new();
  evbuffer_expand(evb, serialized.size());
  evbuffer_add(evb, (const void *)serialized.c_str(), serialized.size());
  evhttp_send_reply(req, 200, "OK", evb);
  evbuffer_free(evb);
  return true;
}