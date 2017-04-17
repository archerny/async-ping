#ifndef _HTTP_UTIL_H_
#define _HTTP_UTIL_H_

#include <string>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>

#include "JsonValue.h"

using common::JsonValue;
using std::string;

#define RECV_BUF_SIZE 4097

/**
 * some stateless utils
 */
class HttpUtil
{
  // for http server
public:
  static JsonValue *recvReqInputJson(struct evhttp_request *req);

  static string recvReqInput(struct evhttp_request *req);

  static bool sendStringResp(struct evhttp_request *req, string& content);

  static bool sendJsonResp(struct evhttp_request *req, JsonValue& jv);
};

#endif