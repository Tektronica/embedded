# PlatformIO downloads fail behind Zscaler (SSL cert verify)

**Status:** Resolved

## Symptom

- `pio run` / platform install fails immediately with `HTTPClientError:` (empty message).
- VSCode shows **"Could not start PIO Home server: Timeout error"**.
- `curl https://...platformio.org` works fine, but pio does not.

## Cause

The corporate network runs **Zscaler TLS interception**: it re-signs HTTPS with the
*Zscaler Root CA*. macOS trusts that root (so `curl`, which uses the system keychain, works), but
PlatformIO's bundled Python verifies against its own `certifi` bundle, which does **not** include
the Zscaler root — so every download fails with:

```
SSL: CERTIFICATE_VERIFY_FAILED — unable to get local issuer certificate
```

Confirm the interception:

```bash
echo | openssl s_client -connect api.registry.nm1.platformio.org:443 \
  -servername api.registry.nm1.platformio.org 2>/dev/null | openssl x509 -noout -issuer
# issuer=... O=Zscaler Inc. ... CN=Zscaler Intermediate Root CA ...
```

## Fix

Two independent strict-SSL switches must both be relaxed:

1. **PlatformIO Core** (Python downloads):
   ```bash
   pio settings set enable_proxy_strict_ssl false
   ```
2. **VSCode** (extension / PIO Home server): Settings (`⌘,`) → search `proxy strict` →
   uncheck **Http: Proxy Strict SSL** → restart VSCode.

After both, `pio run`, `pio test`, uploads, and PIO Home all work.

## Secure alternative (keeps TLS verification)

Instead of disabling verification, trust the Zscaler root in pio's bundle:

```bash
security find-certificate -a -p -c Zscaler /Library/Keychains/System.keychain > /tmp/zscaler.pem
cat /tmp/zscaler.pem >> ~/.platformio/penv/lib/python3.10/site-packages/certifi/cacert.pem
```

More correct, but **fragile**: a `certifi` update overwrites the bundle and you must re-append.
On a machine already behind Zscaler interception, disabling pio's strict SSL (above) is the
pragmatic choice.
