#!/usr/bin/env python3
"""Gives a TestFlight build to the testers' groups, through the App Store Connect API.

The nightly builds (.github/workflows/build.yml, after scripts/upload-testflight.sh)
run it so that each nightly reaches the same testers, through the same
invitations and public links, with no new link:

    python3 -I scripts/testflight_distribute.py \
        --api-key AuthKey_XXXX.p8 --api-key-id XXXX --api-issuer <issuer id> \
        --bundle-id com.hrvojekatic.Laprdus --version 2.0.0 --build 41.612 \
        --groups "Laprdus testers" --what-to-test notes.txt

It waits until App Store Connect has processed the build (it can take half an
hour), declares no non-exempt encryption if the build has no answer yet (the
app's Info.plist declares it: ITSAppUsesNonExemptEncryption = NO), sets
"What to Test", turns on the testers' notification, adds the build to each
group named in --groups (comma-separated; an internal group that gets every
build by itself is left alone) and, when one of them is external, submits it
to Beta App Review: Apple looks at the first build of a version; later builds
of it usually pass at once.

--dry-run makes the token and prints what would be sent, without sending.

Only the Python standard library; the token is signed (ES256) by openssl.
"""

from __future__ import annotations

import argparse
import base64
import json
import subprocess
import sys
import time
import urllib.error
import urllib.parse
import urllib.request

API = "https://api.appstoreconnect.apple.com"


def b64url(data: bytes) -> str:
    return base64.urlsafe_b64encode(data).rstrip(b"=").decode("ascii")


def der_to_raw(der: bytes) -> bytes:
    """An ECDSA signature from DER (what openssl writes) to the r||s of JWS."""
    if der[0] != 0x30:
        raise ValueError("not a DER ECDSA signature")
    i = 2 if der[1] < 0x80 else 2 + (der[1] & 0x7F)
    parts = []
    for _ in range(2):
        if der[i] != 0x02:
            raise ValueError("not a DER ECDSA signature")
        length = der[i + 1]
        value = der[i + 2:i + 2 + length].lstrip(b"\x00")
        if len(value) > 32:
            raise ValueError("not a P-256 signature")
        parts.append(value.rjust(32, b"\x00"))
        i += 2 + length
    return parts[0] + parts[1]


def make_token(key_path: str, key_id: str, issuer: str, lifetime: int = 1200) -> str:
    now = int(time.time())
    header = {"alg": "ES256", "kid": key_id, "typ": "JWT"}
    payload = {"iss": issuer, "iat": now, "exp": now + lifetime, "aud": "appstoreconnect-v1"}
    signing_input = f"{b64url(json.dumps(header).encode())}.{b64url(json.dumps(payload).encode())}"
    der = subprocess.run(["openssl", "dgst", "-sha256", "-sign", key_path],
                         input=signing_input.encode("ascii"), capture_output=True, check=True).stdout
    return f"{signing_input}.{b64url(der_to_raw(der))}"


class Connect:
    def __init__(self, args):
        self.args = args
        self.token = ""
        self.made = 0.0

    def request(self, method: str, path: str, body: dict | None = None, ok_codes=(200, 201, 204)):
        # A token lives 20 minutes; a new one every 15.
        if time.time() - self.made > 900:
            self.token = make_token(self.args.api_key, self.args.api_key_id, self.args.api_issuer)
            self.made = time.time()
        if self.args.dry_run and method != "GET":
            print(f"(dry run) {method} {path} {json.dumps(body) if body else ''}")
            return None
        data = json.dumps(body).encode() if body is not None else None
        req = urllib.request.Request(API + path, data=data, method=method, headers={
            "Authorization": f"Bearer {self.token}", "Content-Type": "application/json"})
        try:
            with urllib.request.urlopen(req, timeout=60) as response:
                text = response.read().decode() or "{}"
                return json.loads(text)
        except urllib.error.HTTPError as error:
            text = error.read().decode(errors="replace")
            if error.code in ok_codes:
                return {}
            raise SystemExit(f"testflight_distribute.py: {method} {path}: HTTP {error.code}\n{text}")


def query(params: dict) -> str:
    return "?" + urllib.parse.urlencode(params)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--api-key", required=True)
    parser.add_argument("--api-key-id", required=True)
    parser.add_argument("--api-issuer", required=True)
    parser.add_argument("--bundle-id", required=True)
    parser.add_argument("--version", required=True, help="CFBundleShortVersionString, e.g. 2.0.0")
    parser.add_argument("--build", required=True, help="CFBundleVersion, e.g. 41.612")
    parser.add_argument("--groups", required=True, help="the TestFlight groups' names, comma-separated")
    parser.add_argument("--what-to-test", help="a text file: the build's \"What to Test\"")
    parser.add_argument("--locale", default="en-US")
    parser.add_argument("--wait-minutes", type=int, default=75)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    wanted = [g.strip() for g in args.groups.split(",") if g.strip()]
    connect = Connect(args)

    if args.dry_run and args.api_key_id == "DRYRUN":
        token = make_token(args.api_key, args.api_key_id, args.api_issuer)
        print(f"(dry run) token {token.split('.')[0]}... ({len(token)} characters)")
        print(f"(dry run) the build {args.version} ({args.build}) of {args.bundle_id} to {wanted}")
        return

    apps = connect.request("GET", "/v1/apps" + query({"filter[bundleId]": args.bundle_id, "limit": 20}))
    app = next((a for a in apps["data"] if a["attributes"]["bundleId"] == args.bundle_id), None)
    if not app:
        raise SystemExit(f"testflight_distribute.py: App Store Connect has no app {args.bundle_id}")
    print(f"App: {app['attributes']['name']} ({app['id']})")

    # The build, once it is processed.
    build = None
    deadline = time.time() + args.wait_minutes * 60
    while True:
        builds = connect.request("GET", "/v1/builds" + query({
            "filter[app]": app["id"], "filter[version]": args.build,
            "filter[preReleaseVersion.version]": args.version, "limit": 5}))
        build = builds["data"][0] if builds["data"] else None
        state = build["attributes"]["processingState"] if build else "not there yet"
        print(f"Build {args.version} ({args.build}): {state}")
        if state == "VALID":
            break
        if state in ("FAILED", "INVALID"):
            raise SystemExit(f"testflight_distribute.py: App Store Connect could not process the build ({state})")
        if time.time() > deadline:
            raise SystemExit(f"testflight_distribute.py: the build was not processed within {args.wait_minutes} minutes")
        time.sleep(60)
    build_id = build["id"]
    ref = {"type": "builds", "id": build_id}

    if build["attributes"].get("usesNonExemptEncryption") is None:
        print("Declaring no non-exempt encryption, as the app's Info.plist does")
        connect.request("PATCH", f"/v1/builds/{build_id}", {"data": {**ref, "attributes": {"usesNonExemptEncryption": False}}})

    if args.what_to_test:
        notes = open(args.what_to_test, encoding="utf-8").read().strip()[:4000]
        found = connect.request("GET", f"/v1/builds/{build_id}/betaBuildLocalizations")
        existing = next((l for l in found["data"] if l["attributes"]["locale"] == args.locale), None)
        if existing:
            connect.request("PATCH", f"/v1/betaBuildLocalizations/{existing['id']}", {"data": {
                "type": "betaBuildLocalizations", "id": existing["id"], "attributes": {"whatsNew": notes}}})
        else:
            connect.request("POST", "/v1/betaBuildLocalizations", {"data": {
                "type": "betaBuildLocalizations", "attributes": {"locale": args.locale, "whatsNew": notes},
                "relationships": {"build": {"data": ref}}}})
        print("What to Test is set")

    detail = connect.request("GET", f"/v1/builds/{build_id}/buildBetaDetail")
    if detail and detail.get("data"):
        connect.request("PATCH", f"/v1/buildBetaDetails/{detail['data']['id']}", {"data": {
            "type": "buildBetaDetails", "id": detail["data"]["id"], "attributes": {"autoNotifyEnabled": True}}})
        print("The testers will be notified")

    groups = connect.request("GET", f"/v1/apps/{app['id']}/betaGroups" + query({"limit": 200}))["data"]
    by_name = {g["attributes"]["name"].casefold(): g for g in groups}
    missing = [name for name in wanted if name.casefold() not in by_name]
    if missing:
        names = ", ".join(g["attributes"]["name"] for g in groups)
        raise SystemExit(f"testflight_distribute.py: no TestFlight group {missing}; the app has: {names}")
    external = False
    for name in wanted:
        group = by_name[name.casefold()]
        attributes = group["attributes"]
        if attributes.get("isInternalGroup") and attributes.get("hasAccessToAllBuilds"):
            print(f"Group {attributes['name']}: internal, gets every build by itself")
            continue
        connect.request("POST", f"/v1/betaGroups/{group['id']}/relationships/builds", {"data": [ref]})
        print(f"Group {attributes['name']}: the build is added")
        external = external or not attributes.get("isInternalGroup")

    if external:
        # 409: submitted already (or approved).
        connect.request("POST", "/v1/betaAppReviewSubmissions", {"data": {
            "type": "betaAppReviewSubmissions", "relationships": {"build": {"data": ref}}}}, ok_codes=(409,))
        print("Submitted to Beta App Review (external testers get the build when Apple has passed it)")
    print("Done")


if __name__ == "__main__":
    main()
