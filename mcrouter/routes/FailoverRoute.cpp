/*
 *  Copyright (c) 2015, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "FailoverRoute.h"

#include <folly/dynamic.h>

#include "mcrouter/lib/config/RouteHandleFactory.h"
#include "mcrouter/routes/McRouteHandleBuilder.h"
#include "mcrouter/routes/McrouterRouteHandle.h"

namespace facebook { namespace memcache { namespace mcrouter {

McrouterRouteHandlePtr makeNullRoute();

McrouterRouteHandlePtr makeFailoverRoute(std::vector<McrouterRouteHandlePtr> rh,
                                         FailoverErrorsSettings failoverErrors,
                                         bool failoverTagging) {
  if (rh.empty()) {
    return makeNullRoute();
  }

  if (rh.size() == 1) {
    return std::move(rh[0]);
  }

  return makeMcrouterRouteHandle<FailoverRoute>(std::move(rh),
                                                std::move(failoverErrors),
                                                failoverTagging);
}

McrouterRouteHandlePtr makeFailoverRoute(
    const folly::dynamic& json,
    std::vector<McrouterRouteHandlePtr> children) {

  FailoverErrorsSettings failoverErrors;
  bool failoverTagging = false;
  if (json.isObject()) {
    if (auto jFailoverErrors = json.get_ptr("failover_errors")) {
      failoverErrors = FailoverErrorsSettings(*jFailoverErrors);
    }
    if (auto jfailoverTag = json.get_ptr("failover_tag")) {
      checkLogic(jfailoverTag->isBool(),
                 "FailoverWithExptime: failover_tag is not bool");
      failoverTagging = jfailoverTag->getBool();
    }
  }
  return makeFailoverRoute(std::move(children), std::move(failoverErrors),
                           failoverTagging);
}

McrouterRouteHandlePtr makeFailoverRoute(
    RouteHandleFactory<McrouterRouteHandleIf>& factory,
    const folly::dynamic& json) {
  std::vector<McrouterRouteHandlePtr> children;
  if (json.isObject()) {
    if (auto jchildren = json.get_ptr("children")) {
      children = factory.createList(*jchildren);
    }
  } else {
    children = factory.createList(json);
  }
  return makeFailoverRoute(json, std::move(children));
}

}}}  // facebook::memcache::mcrouter
