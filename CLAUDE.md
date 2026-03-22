# Claude Code Switcher

CLI tool for switching Claude Code between API providers and model tiers.

## File Map
- `cc` — Main bash script (~600 lines). Provider switches (incl. Ollama), model tier changes, setup wizards, status, check, help.
- `cc-scan` — Python script. Tests all 5 models × 5 providers in parallel, renders color matrix. Requires `httpx`.
- `cc-test` — Bash test suite (70 local state checks, no API calls).
- `cc-api-test` — Original curl-based API test (superseded by `cc scan`).
- `install.sh` — Adds project dir to PATH.

## State Files
| File | Purpose |
|------|---------|
| `~/.config/claude-code/active-backend` | Current provider: teams\|direct\|bedrock\|vertex\|foundry\|ollama |
| `~/.config/claude-code/env.sh` | Sourced by shell for provider env vars |
| `~/.config/claude-code/direct.env` | Anthropic API key |
| `~/.config/claude-code/foundry.env` | Azure endpoint + key |
| `~/.config/claude-code/ollama.env` | Ollama URL, model, tier mappings |
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

## Integration with cloud-chat-assistant

The `cc-scan` script reads credentials from `~/.config/cloud-chat-assistant/config.json` as fallback when provider-specific credential files are missing. This means AWS/Azure creds configured for the chat assistant are automatically available to `cc scan`.
