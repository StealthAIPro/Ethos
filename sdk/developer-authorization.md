# Developer access service

Vision Core signs in through its Discord OAuth application. A registered resource can request a short-lived assertion with the signed-in Discord user ID and that product’s device ID. A developer can use the assertion to check a license on their own server.

## Service setup

Configure the protected runtime Worker with a SQLite-backed `DEVELOPER_LEDGER` binding, the existing `RUNTIME_RATE` binding, and these settings.

| Setting | Value |
| --- | --- |
| `DEVELOPER_ISSUER` | Public HTTPS origin of the Worker, without a path |
| `DEVELOPER_DISCORD_CLIENT_ID` | Vision Core’s public Discord OAuth application ID |
| `DEVELOPER_PRODUCTS` | JSON registry keyed by each approved product ID |
| `DEVELOPER_ASSERTION_KEY_ID` | Public signing-key identifier for rotation, with `v1` as the default |
| `DEVELOPER_ASSERTION_KEY` | Worker secret with the P-256 PKCS#8 private key encoded as unpadded base64url |

Pass the corresponding public verification key and key ID to each developer through a trusted channel. For example, the registry below requires one specified role in the first product’s server and locks each subscribed account to its first device. The second product can apply its own license check on its server.

```json
{
  "studio.paid-tool": {
    "guildId": "123456789012345678",
    "roles": ["223456789012345678", "323456789012345678"],
    "requireAll": false,
    "lockDevice": true
  },
  "studio.another-tool": {}
}
```

Specify a `guildId` alone to require server membership. Set `requireAll` to `true` to require every listed role. Device enrollment is separate for each product and Discord account. Changing Windows accounts, reinstalling Windows, or deleting the product’s Windows signing key can change the device ID. A support workflow should confirm ownership before clearing an enrolled device in the service’s binding store.

## Host protocol

1. The host sends an empty `POST /v1/developer/<product>/challenge` with the Discord OAuth bearer. The service rechecks the Discord application's identity, the user, and any configured membership and roles. A successful request returns `challenge` and `expiresAt`, an epoch-millisecond timestamp. The challenge expires in 60 seconds. Issuing a new challenge replaces the account’s previous challenge for that product.
2. The host signs the exact UTF-8 JSON `{ "v": 1, "productId": "studio.paid-tool", "challenge": "…" }` with its per-product Windows P-256 key. `POST /v1/developer/<product>/activate` uses the same OAuth bearer and sends `proof`, `signature`, `publicKey`, and `deviceId` as JSON. `proof` is the signed JSON, `signature` is a raw 64-byte ECDSA signature, and `publicKey` is the DER SubjectPublicKeyInfo. These three fields use unpadded base64url. `deviceId` is lowercase SHA-256 hex of the DER public key.
3. The service verifies the proof, consumes the account’s challenge once, and enforces the device enrollment. A successful activation returns `assertion`, `expiresAt`, `discordId`, and `deviceId`. The assertion is an ES256 JWT with a maximum lifetime of 120 seconds.

The dedicated `ethos.2kcv` service checks Discord identity and scopes at challenge time. It verifies membership and roles during activation before enrolling a device or issuing an assertion.

An activation returns `401` for an absent or invalid Discord session, `403` for an ineligible account or invalid proof, `409` for a missing, expired or used challenge, `423` for a different enrolled device, and `429` if a rate limit applies. Discord's numeric retry delay is forwarded as `Retry-After` when available.

Send the assertion over HTTPS to your product backend. Configure a maintained JWT verifier with the issuer origin and public signing key obtained from Vision Core’s operator. Select only a pinned trusted public key associated with the expected `kid`, and require ES256, the configured issuer in `iss`, your registered product ID in `aud`, and a current `exp`. Treat `sub` as the Discord ID and `deviceId` as the enrolled device identifier. Compare both with your license records before granting access. Preserve Discord IDs as strings. Use `jti` for one-time exchange if the assertion grants a longer-lived product session.

The device ID identifies a product-scoped key in the signed-in Windows profile. It represents possession of that software-backed key at the time of issuance. Verification of the assertion does not establish hardware attestation or stop an authorized user from forwarding a valid bearer assertion during its lifetime. An app’s local identity values are suitable for display. The signed assertion is the input to server-side license decisions.

## Verification

```powershell
node --test protected_runtime/developer/authorization.test.mjs
```
