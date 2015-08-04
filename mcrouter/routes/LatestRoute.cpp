/*
 *  Copyright (c) 2015, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <folly/dynamic.h>

#include "mcrouter/lib/config/RouteHandleFactory.h"
#include "mcrouter/lib/FailoverErrorsSettings.h"
#include "mcrouter/lib/fbi/cpp/globals.h"
#include "mcrouter/routes/McRouteHandleBuilder.h"
#include "mcrouter/routes/McrouterRouteHandle.h"

namespace facebook { namespace memcache { namespace mcrouter {

McrouterRouteHandlePtr makeFailoverRoute(std::vector<McrouterRouteHandlePtr> rh,
                                         FailoverErrorsSettings failoverErrors,
                                         bool failoverTagging);

McrouterRouteHandlePtr makeLatestRoute(
  std::vector<McrouterRouteHandlePtr> targets,
  size_t failoverCount,
  FailoverErrorsSettings failoverErrors) {

  std::vector<McrouterRouteHandlePtr> failovers;
  failoverCount = std::min(failoverCount, targets.size());
  size_t curHash = folly::hash::hash_combine(0, globals::hostid());
  for (size_t i = 0; i < failoverCount; ++i) {
    auto id = curHash % targets.size();
    failovers.push_back(std::move(targets[id]));
    std::swap(targets[id], targets[targets.size() - 1]);
    targets.pop_back();
    curHash = folly::hash::hash_combine(curHash, i);
  }
  return makeFailoverRoute(std::move(failovers), std::move(failoverErrors),
                           /* failoverTagging */ false);
}

McrouterRouteHandlePtr makeLatestRoute(
  const folly::dynamic& json,
  std::vector<McrouterRouteHandlePtr> targets) {

  size_t failoverCount = 5;
  FailoverErrorsSettings failoverErrors;
  if (json.isObject()) {
    if (auto jfailoverCount = json.get_ptr("failover_count")) {
      checkLogic(jfailoverCount->isInt(),
                 "LatestRoute: failover_count is not an integer");
      failoverCount = jfailoverCount->getInt();
    }
    if (auto jFailoverErrors = json.get_ptr("failover_errors")) {
      failoverErrors = FailoverErrorsSettings(*jFailoverErrors);
    }
  }
  return makeLatestRoute(std::move(targets), failoverCount,
                         std::move(failoverErrors));
}

McrouterRouteHandlePtr makeLatestRoute(
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
  return makeLatestRoute(json, std::move(children));
}

}}}  // facebook::memcache::mcrouter
