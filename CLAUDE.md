<!-- claude-md-version: 84f2f1c | updated: 2026-04-07 -->
# Claude Code Switcher

CLI tool for switching Claude Code between API providers, model tiers, and Teams accounts.

## File Map
- `cc` — Main bash script (795 lines). Provider switches (incl. Ollama), model tier changes, multi-account management, setup wizards, status, check, help.
- `cc-scan` — Python script. Tests all 5 models × 5 providers in parallel, renders color matrix. Requires `httpx`.
- `cc-usage` — Python script. Checks usage/rate limits across multiple Teams accounts. Probes `api.anthropic.com/api/oauth/usage`. Requires `httpx`.
- `cc-test` — Bash test suite (70 local state checks, no API calls).
- `cc-api-test` — Original curl-based API test (superseded by `cc scan`).
- `install.sh` — Adds project dir to PATH.

## State Files
| File | Purpose |
|------|---------|
| `~/.config/claude-code/active-backend` | Current provider: teams\|direct\|bedrock\|vertex\|foundry\|ollama |
| `~/.config/claude-code/env.sh` | Sourced by shell for provider env vars + account selection |
| `~/.config/claude-code/direct.env` | Anthropic API key |
| `~/.config/claude-code/foundry.env` | Azure endpoint + key |
| `~/.config/claude-code/ollama.env` | Ollama URL, model, tier mappings |
| `~/.config/claude-code/accounts/<name>/` | Per-account config dirs (each has `.credentials.json`) |
| `~/.aws/credentials` | AWS credentials for Bedrock |
| `~/.claude/settings.json` | Claude Code settings (model field) |

## Provider Model IDs
Model IDs vary by provider. The `_opus_model`, `_opus45_model`, `_sonnet_model`, `_sonnet45_model`, `_haiku_model` functions in `cc` handle the mapping. Key differences:
- **bedrock**: `us.anthropic.` prefix, version suffixes like `-v1` or `-v1:0`
- **vertex**: `@` instead of `-` for date versions (e.g. `claude-opus-4-5@20251101`)
- **direct/foundry/teams**: Standard Anthropic IDs
- **ollama**: User-specified model names (e.g. `qwen3-coder`, `deepseek-r1:70b`). Tier mapping via `ANTHROPIC_DEFAULT_*_MODEL` env vars

## Auto-Failover Instructions

When Claude Code encounters API errors, use `cc` to switch providers or models:

### Rate Limit (429 / overloaded)
```bash
cc opus45    # Fall back to Opus 4.5
cc sonnet    # Or switch to Sonnet 4.6 for speed
cc bedrock   # Or switch entire provider
```

### Auth Failure (401/403)
```bash
cc check     # Diagnose which providers are working
cc scan      # Full matrix of model availability
cc teams     # Switch to Teams (OAuth, always works)
```

### Model Not Available (404)
```bash
cc scan      # See which models are available where
cc bedrock   # Bedrock typically has all models
```

### Recommended Failover Chain
1. `cc opus` (primary) → rate limited →
2. `cc opus45` (same provider, older model) → still limited →
3. `cc bedrock` then `cc opus` (switch provider) → no AWS creds →
4. `cc teams` (OAuth fallback, always works)
5. `cc ollama` (fully local, no rate limits, no auth — but reduced model quality)
6. `cc usage --probe` → `cc account <least-used>` (rotate Teams accounts)

After any switch: open a new Claude Code session for it to take effect.

## Ollama Integration

Requires Ollama v0.14+ (native Anthropic API at `/v1/messages`). No proxy needed.

```bash
cc setup-ollama              # First-time: configure URL, model, tier mappings
cc ollama                    # Switch to Ollama (uses configured default model)
cc ollama deepseek-r1:70b   # Switch + override model
```

Tier mapping: `cc opus`/`cc sonnet`/`cc haiku` work on the ollama provider by mapping to models configured in `ollama.env` via `ANTHROPIC_DEFAULT_*_MODEL` env vars.

Known limitations: streaming tool call timeouts (Ollama #14858), `/v1/messages/count_tokens` 404 degradation (#13949), 64K+ context required (`num_ctx`).

## Multi-Account Teams Rotation

For rotating across multiple Teams accounts to avoid rate limits. Each account gets its own isolated `CLAUDE_CONFIG_DIR` with separate OAuth tokens and usage quotas.

```bash
cc setup-accounts            # Guided setup for multiple account directories
cc usage                     # Quick status (token validity, tier, sessions)
cc usage --probe             # Probe API for rate limit utilization + reset times
cc usage --json              # JSON output for scripting
cc account                   # List available accounts
cc account claude3           # Switch to a specific account
```

### How It Works

Account directories live at `~/.config/claude-code/accounts/<name>/`, each containing its own `.credentials.json`. Also discovers ccs profiles from `~/.ccs/instances/`.

`cc account <name>` writes `CLAUDE_CONFIG_DIR=<path>` into `env.sh`, which Claude Code reads on next session start.

### Usage API

`cc usage --probe` hits `GET api.anthropic.com/api/oauth/usage` with header `anthropic-beta: oauth-2025-04-20`. Response includes utilization windows:

| Window | What it measures |
|--------|-----------------|
| `five_hour` | 5-hour rolling session burst limit (0-100%) |
| `seven_day` | 7-day rolling weekly limit, all models |
| `seven_day_opus` | 7-day Opus-specific limit |
| `seven_day_sonnet` | 7-day Sonnet-specific limit |
| `extra_usage` | Monthly overage credits (Teams/Enterprise) |

Each window has `utilization` (percentage) and `resets_at` (ISO timestamp). A 429 response means the account is rate-limited; `retry-after` header gives reset time.

### Current Accounts

| Name | Org |
|------|-----|
| `default` | ~/.claude (primary) |
| `jphein` | JP's personal Teams |
| `claude` | Teams org 1 |
| `claude2` | Teams org 2 |
| `claude3` | Teams org 3 |
| `claude4` | Teams org 4 |

## Integration with cloud-chat-assistant

The `cc-scan` script reads credentials from `~/.config/cloud-chat-assistant/config.json` as fallback when provider-specific credential files are missing. This means AWS/Azure creds configured for the chat assistant are automatically available to `cc scan`.
